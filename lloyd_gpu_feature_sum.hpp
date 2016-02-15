/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 * 
 * 
 * Copyright (c) 2016, Lutz, Clemens <lutzcle@cml.li>
 */

#ifndef LLOYD_GPU_FEATURE_SUM_HPP
#define LLOYD_GPU_FEATURE_SUM_HPP

#include "cl_kernels/lloyd_labeling_api.hpp"
#include "cl_kernels/lloyd_feature_sum_api.hpp"
#include "cl_kernels/mass_sum_global_atomic_api.hpp"
#include "cl_kernels/mass_sum_merge_api.hpp"
#include "cl_kernels/aggregate_sum_api.hpp"

#include "kmeans_common.hpp"
#include "matrix.hpp"

#include <cstdint>
#include <type_traits>
#include <vector>

#ifdef MAC
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

namespace cle {

template <typename FP, typename INT, typename AllocFP, typename AllocINT>
class LloydGPUFeatureSum {
public:
    enum class MassSumStrategy {
        GlobalAtomic,
        Merge
    };

    LloydGPUFeatureSum(
            cl::Context const& context,
            cl::CommandQueue const& queue
            );

    char const* name() const;

    int initialize();
    int finalize();

    int operator() (
            uint32_t const max_iterations,
            cle::Matrix<FP, AllocFP, INT, true> const& points,
            cle::Matrix<FP, AllocFP, INT, true>& centroids,
            std::vector<INT, AllocINT>& mass,
            std::vector<INT, AllocINT>& labels,
            KmeansStats& stats
            );

private:
    using CL_FP = typename std::conditional<
        std::is_same<FP, float>::value, cl_float, cl_double>::type;
    using CL_INT = typename std::conditional<
        std::is_same<INT, uint32_t>::value, cl_uint, cl_ulong>::type;

    cle::LloydLabelingAPI<CL_FP, CL_INT> labeling_kernel_;
    cle::LloydFeatureSumAPI<CL_FP, CL_INT> feature_sum_kernel_;
    cle::MassSumGlobalAtomicAPI<CL_FP, CL_INT> mass_sum_kernel_;
    cle::MassSumMergeAPI<CL_INT> mass_sum_merge_kernel_;
    cle::AggregateSumAPI<CL_INT> aggregate_sum_kernel_;
    cl::Context context_;
    cl::CommandQueue queue_;

    MassSumStrategy mass_sum_strategy_ = MassSumStrategy::Merge;
    cl_uint warp_size_;
};

using LloydGPUFeatureSum32Aligned = LloydGPUFeatureSum<float, uint32_t, AlignedAllocatorFP32, AlignedAllocatorINT32>;
using LloydGPUFeatureSum64Aligned = LloydGPUFeatureSum<double, uint64_t, AlignedAllocatorFP64, AlignedAllocatorINT64>;

}

extern template class cle::LloydGPUFeatureSum<float, uint32_t, cle::AlignedAllocatorFP32, cle::AlignedAllocatorINT32>;
extern template class cle::LloydGPUFeatureSum<double, uint64_t, cle::AlignedAllocatorFP64, cle::AlignedAllocatorINT64>;

#endif /* LLOYD_GPU_FEATURE_SUM_HPP */
