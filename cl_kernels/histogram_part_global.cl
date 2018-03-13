/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 * 
 * 
 * Copyright (c) 2016-2017, Lutz, Clemens <lutzcle@cml.li>
 */

// #define LOCAL_STRIDE
// Default: global stride access

#ifdef TYPE32
#define CL_INT uint
#else
#ifdef TYPE64
#define CL_INT ulong
#pragma OPENCL EXTENSION cl_khr_int64_base_atomics : enable
#endif
#endif

/*
 * Calculate histogram in paritions per work group
 * 
 * g_in: global_size
 * g_out: num_work_groups * NUM_BINS
 * l_bins: NUM_BINS
 */
__kernel
void histogram_part_global(
            __global CL_INT const *const restrict g_in,
            __global CL_INT *const restrict g_out,
            const CL_INT NUM_ITEMS,
            const CL_INT NUM_BINS
       ) {
    CL_INT group_offset = get_group_id(0) * NUM_BINS;

    for (
            CL_INT r = get_local_id(0);
            r < NUM_BINS;
            r += get_local_size(0)
        )
    {
        g_out[group_offset + r] = 0;
    }

    barrier(CLK_GLOBAL_MEM_FENCE);

#ifdef LOCAL_STRIDE
    CL_INT stride = get_local_size(0);
    CL_INT block_size =
        (NUM_ITEMS + get_num_groups(0) - 1) / get_num_groups(0);
    CL_INT group_start_offset = get_group_id(0) * block_size;
    CL_INT start_offset = group_start_offset + get_local_id(0);
    CL_INT real_block_size =
        (group_start_offset + block_size > NUM_ITEMS)
        ? sub_sat(NUM_ITEMS, group_start_offset)
        : block_size
        ;

    for (
            CL_INT p = start_offset;
            p < group_start_offset + real_block_size;
            p += stride
        )
#else
    for (
            CL_INT p = get_global_id(0);
            p < NUM_ITEMS;
            p += get_global_size(0)
        )
#endif
    {
        CL_INT bin_indx = g_in[p];
#ifdef TYPE32
        atomic_inc(&g_out[group_offset + bin_indx]);
#else
#ifdef TYPE64
        atom_inc(&g_out[group_offset + bin_indx]);
#endif
#endif
    }
}
