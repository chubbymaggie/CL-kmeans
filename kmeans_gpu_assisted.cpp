/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 * 
 * 
 * Copyright (c) 2016, Lutz, Clemens <lutzcle@cml.li>
 */

#include "kmeans_gpu_assisted.hpp"

#include "cle/common.hpp"
#include "cl_kernels/lloyd_labeling_api.hpp"
#include "cl_kernels/lloyd_labeling_vp_clc_api.hpp"
#include "cl_kernels/lloyd_labeling_vp_clcp_api.hpp"

#include <cstdint>
#include <cstddef> // size_t
#include <vector>
#include <algorithm> // std::any_of
#include <type_traits>

#ifdef MAC
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

template <typename FP, typename INT, typename AllocFP, typename AllocINT>
cle::KmeansGPUAssisted<FP, INT, AllocFP, AllocINT>::KmeansGPUAssisted(
        cl::Context const& context, cl::CommandQueue const& queue)
    :
        context_(context), queue_(queue)
{}

template <typename FP, typename INT, typename AllocFP, typename AllocINT>
char const* cle::KmeansGPUAssisted<FP, INT, AllocFP, AllocINT>::name() const {

    return "Lloyd_GPU_Assisted";
}

template <typename FP, typename INT, typename AllocFP, typename AllocINT>
int cle::KmeansGPUAssisted<FP, INT, AllocFP, AllocINT>::initialize() {

    // Create kernel
    cle_sanitize_done_return(
            labeling_kernel_.initialize(context_));

    cle_sanitize_done_return(
            labeling_vp_clc_kernel_.initialize(context_, 1, 8));

    cle_sanitize_done_return(
            labeling_vp_clcp_kernel_.initialize(context_, 4, 8));

    cl::Device device;
    cle_sanitize_val_return(
            this->queue_.getInfo(
                CL_QUEUE_DEVICE,
                &device
                ));

    cle_sanitize_done_return(
            cle::device_warp_size(device, warp_size_));

    return 1;
}

template <typename FP, typename INT, typename AllocFP, typename AllocINT>
int cle::KmeansGPUAssisted<FP, INT, AllocFP, AllocINT>::finalize() {
    return 1;
}

template <typename FP, typename INT, typename AllocFP, typename AllocINT>
int cle::KmeansGPUAssisted<FP, INT, AllocFP, AllocINT>::operator() (
        uint32_t const max_iterations,
        cle::Matrix<FP, AllocFP, INT, true> const& points,
        cle::Matrix<FP, AllocFP, INT, true>& centroids,
        std::vector<INT, AllocINT>& cluster_size,
        std::vector<INT, AllocINT>& memberships,
        KmeansStats& stats
        ) {

    assert(points.cols() == centroids.cols());
    assert(points.rows() == memberships.size());

    uint32_t iterations;
    bool did_changes;

    size_t local_size = warp_size_ * 4;
    size_t global_size = local_size * 90 * 32;

    std::vector<cl_char> h_did_changes(1);

    cle::TypedBuffer<cl_char> d_did_changes(this->context_, CL_MEM_READ_WRITE, 1);
    cle::TypedBuffer<CL_FP> d_points(this->context_, CL_MEM_READ_ONLY, points.size());
    cle::TypedBuffer<CL_FP> d_centroids(this->context_, CL_MEM_READ_ONLY, centroids.size());
    cle::TypedBuffer<CL_INT> d_memberships(this->context_, CL_MEM_READ_WRITE, points.rows());


    // copy points (x,y) host -> device
    cle_sanitize_val_return(
            this->queue_.enqueueWriteBuffer(
                d_points,
                CL_FALSE,
                0,
                d_points.bytes(),
                points.data()
                ));

    cle_sanitize_val_return(
            this->queue_.enqueueFillBuffer(
                d_memberships,
                0,
                0,
                d_memberships.bytes()
                ));

    iterations = 0;
    did_changes = true;
    while (did_changes == true && iterations < max_iterations) {

        // set did_changes to false on device
        cle_sanitize_val(
                this->queue_.enqueueFillBuffer(
                    d_did_changes,
                    false,
                    0,
                    d_did_changes.bytes()
                    ));

        cle_sanitize_val_return(
                this->queue_.enqueueWriteBuffer(
                    d_centroids,
                    CL_FALSE,
                    0,
                    d_centroids.bytes(),
                    centroids.data()
                    ));

        // execute kernel
        cl::Event labeling_event;
        switch (labeling_strategy_) {
            case LabelingStrategy::Plain:
                cle_sanitize_done_return(
                        labeling_kernel_(
                            cl::EnqueueArgs(
                                queue_,
                                cl::NDRange(global_size),
                                cl::NDRange(warp_size_)
                                ),
                            d_did_changes,
                            points.cols(),
                            points.rows(),
                            centroids.rows(),
                            d_points,
                            d_centroids,
                            d_memberships,
                            labeling_event
                            ));
                break;
            case LabelingStrategy::VpClc:
                cle_sanitize_done_return(
                        labeling_vp_clc_kernel_(
                            cl::EnqueueArgs(
                                queue_,
                                cl::NDRange(global_size),
                                cl::NDRange(local_size)
                                ),
                            d_did_changes,
                            points.cols(),
                            points.rows(),
                            centroids.rows(),
                            d_points,
                            d_centroids,
                            d_memberships,
                            labeling_event
                            ));
                break;
            case LabelingStrategy::VpClcp:
                cle_sanitize_done_return(
                        labeling_vp_clcp_kernel_(
                            cl::EnqueueArgs(
                                queue_,
                                cl::NDRange(global_size),
                                cl::NDRange(local_size)
                                ),
                            d_did_changes,
                            points.cols(),
                            points.rows(),
                            centroids.rows(),
                            d_points,
                            d_centroids,
                            d_memberships,
                            labeling_event
                            ));
                break;
        }

        // copy did_changes device -> host
        cle_sanitize_val(
                this->queue_.enqueueReadBuffer(
                    d_did_changes,
                    CL_FALSE,
                    0,
                    d_did_changes.bytes(),
                    h_did_changes.data()
                    ));

        cle_sanitize_val(
                this->queue_.enqueueReadBuffer(
                    d_memberships,
                    CL_TRUE,
                    0,
                    d_memberships.bytes(),
                    memberships.data()
                    ));

        // inspect did_changes
        did_changes = std::any_of(
                h_did_changes.cbegin(),
                h_did_changes.cend(),
                [](cl_char i){ return i == 1; }
                );

        if (did_changes == true) {
            // Initialize to zero
            std::fill(centroids.begin(), centroids.end(), 0);
            std::fill(cluster_size.begin(), cluster_size.end(), 0);

            // Calculate new centroids
            for (INT p = 0; p < points.rows(); ++p) {
                INT c = memberships[p];
                assert(c < centroids.rows());

                for (INT f = 0; f < points.cols(); ++f) {
                    centroids(c, f) += points(p, f);
                }
                ++cluster_size[c];
            }

            for (INT f = 0; f < centroids.cols(); ++f) {
                for (INT c = 0; c < centroids.rows(); ++c) {
                    centroids(c, f) = centroids(c, f) / cluster_size[c];
                }
            }
        }

        ++iterations;
    }

    stats.iterations = iterations;

    return 1;
}

template class cle::KmeansGPUAssisted<float, uint32_t, std::allocator<float>, std::allocator<uint32_t>>;
template class cle::KmeansGPUAssisted<double, uint64_t, std::allocator<double>, std::allocator<uint64_t>>;
#ifdef USE_ALIGNED_ALLOCATOR
template class cle::KmeansGPUAssisted<float, uint32_t, cle::AlignedAllocatorFP32, cle::AlignedAllocatorINT32>;
template class cle::KmeansGPUAssisted<double, uint64_t, cle::AlignedAllocatorFP64, cle::AlignedAllocatorINT64>;
#endif
