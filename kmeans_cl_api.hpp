#ifndef KMEANS_CL_API_HPP
#define KMEANS_CL_API_HPP

#include "cle/common.hpp"

#include <functional>
#include <cassert>

#ifdef MAC
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.hpp>
#endif

namespace cle {

    class Kmeans_With_Host_Kernel {
    public:
        cl_int initialize(cl::Context& context) {
            cl_int error_code = CL_SUCCESS;

            cl::Program program = make_program(context, PROGRAM_FILE, error_code);
            if (error_code != CL_SUCCESS) {
                return error_code;
            }

            kernel_functor_ = Base_Kernel(program,KERNEL_NAME, &error_code);
            sanitize_make_kernel(error_code, context, program);

            return error_code;
        }

        /*
        * kernel prefix_sum
        * 
        * Performs exclusive prefix sum on input
        *
        * Input
        *       Buffer with cl_uint array
        *
        * Output
        *       Buffer with cl_uint array 
        *       cl_uint with total sum
        *
        */
        void operator() (
                cl::EnqueueArgs const& args,
                TypedBuffer<cl_char>& did_changes,
                TypedBuffer<cl_double>& points_x,
                TypedBuffer<cl_double>& points_y,
                TypedBuffer<cl_double>& centroids_x,
                TypedBuffer<cl_double>& centroids_y,
                TypedBuffer<cl_ulong>& memberships
                ) {

            // assert did_changes.size() == #global work items
            assert(points_x.size() == points_y.size());
            assert(points_x.size() == memberships.size());
            assert(centroids_x.size() == centroids_y.size());

            const size_t centroid_bytes = sizeof(cl_double) * centroids_x.size();
            cl::LocalSpaceArg local_centroids_x = cl::Local(centroid_bytes);
            cl::LocalSpaceArg local_centroids_y = cl::Local(centroid_bytes);
            cl::LocalSpaceArg local_old_centroids_x = cl::Local(centroid_bytes);
            cl::LocalSpaceArg local_old_centroids_y = cl::Local(centroid_bytes);

            kernel_functor_(
                    args,
                    did_changes,
                    points_x,
                    points_y,
                    centroids_x,
                    centroids_y,
                    memberships,
                    local_centroids_x,
                    local_centroids_y,
                    local_old_centroids_x,
                    local_old_centroids_y,
                    points_x.size(),
                    centroids_x.size()
            );


        }

    private:
        typedef cl::make_kernel<
            cl::Buffer&,
            cl::Buffer&,
            cl::Buffer&,
            cl::Buffer&,
            cl::Buffer&,
            cl::Buffer&,
            cl::LocalSpaceArg,
            cl::LocalSpaceArg,
            cl::LocalSpaceArg,
            cl::LocalSpaceArg,
            cl_ulong,
            cl_ulong
                > Base_Kernel;

        typedef std::function<Base_Kernel::type_> Kernel_Functor;

        static constexpr const char* PROGRAM_FILE = CL_KERNEL_FILE_PATH("kmeans.cl");
        static constexpr const char* KERNEL_NAME = "kmeans_with_host";

        Kernel_Functor kernel_functor_;
    };
}

#endif /* KMEANS_CL_API_HPP */