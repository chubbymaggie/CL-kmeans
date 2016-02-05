/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 * 
 * 
 * Copyright (c) 2016, Lutz, Clemens <lutzcle@cml.li>
 */

#ifndef KMEANS_NAIVE_HPP
#define KMEANS_NAIVE_HPP

#include "kmeans_common.hpp"

#include <vector>

namespace cle {

template <typename FP, typename INT, typename AllocFP, typename AllocINT>
class KmeansNaive {
public:
    int initialize();
    int finalize();

    void operator() (uint32_t const max_iterations,
                      std::vector<FP, AllocFP> const& points_x,
                      std::vector<FP, AllocFP> const& points_y,
                      std::vector<FP, AllocFP>& centroids_x,
                      std::vector<FP, AllocFP>& centroids_y,
                      std::vector<INT, AllocINT>& cluster_size,
                      std::vector<INT, AllocINT>& memberships,
                      KmeansStats& stats);
private:
    FP gaussian_distance(FP a_x, FP a_y, FP b_x, FP b_y);

};

using KmeansNaive32 =
    KmeansNaive<float, uint32_t, std::allocator<float>, std::allocator<uint32_t>>;
using KmeansNaive64 =
    KmeansNaive<double, uint64_t, std::allocator<double>, std::allocator<uint64_t>>;
using KmeansNaive32Aligned =
    KmeansNaive<float, uint32_t, AlignedAllocatorFP32, AlignedAllocatorINT32>;
using KmeansNaive64Aligned =
    KmeansNaive<double, uint64_t, AlignedAllocatorFP64, AlignedAllocatorINT64>;

}

extern template class cle::KmeansNaive<float, uint32_t, std::allocator<float>, std::allocator<uint32_t>>;
extern template class cle::KmeansNaive<double, uint64_t, std::allocator<double>, std::allocator<uint64_t>>;
extern template class cle::KmeansNaive<float, uint32_t, cle::AlignedAllocatorFP32, cle::AlignedAllocatorINT32>;
extern template class cle::KmeansNaive<double, uint64_t, cle::AlignedAllocatorFP64, cle::AlignedAllocatorINT64>;

#endif /* KMEANS_NAIVE_HPP */