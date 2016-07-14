#include <cl_kernels/histogram_part_local_api.hpp>
#include <cl_kernels/histogram_part_global_api.hpp>
#include <cl_kernels/histogram_part_private_api.hpp>
#include <cl_kernels/reduce_vector_parcol_api.hpp>
#include <matrix.hpp>
#include <measurement/measurement.hpp>

#include <cstdint>
#include <memory>
#include <vector>
#include <algorithm>
#include <random>

#include <gtest/gtest.h>

#include "data_generator.hpp"
#include "opencl_setup.hpp"

#define MEGABYTE (1024 * 1024)
#define GIGABYTE (1024 * 1024 * 1024)

void histogram_verify(
        std::vector<uint32_t> const& data,
        std::vector<uint32_t>& histogram
        ) {

    std::fill(histogram.begin(), histogram.end(), 0);

    for_each(data.begin(), data.end(), [&](uint32_t x){ ++histogram[x]; });
}

template <typename Kernel>
class Histogram {
public:
    void set_work_group_size(int size) {
        work_group_size_ = size;
    }

    void set_num_bins(int bins) {
        num_bins_ = bins;
    }

    void test(
            std::vector<uint32_t> const& data,
            std::vector<uint32_t>& histogram
       ) {
        histogram.resize(num_bins_);
        uint32_t num_work_groups = data.size() / work_group_size_;

        cle::TypedBuffer<uint32_t> d_data(clenv->context, CL_MEM_READ_WRITE, data.size());
        cle::TypedBuffer<uint32_t> d_histogram(clenv->context, CL_MEM_READ_WRITE, num_bins_ * num_work_groups);

        Kernel kernel;
        kernel.initialize(clenv->context);
        cl::Event histogram_event;

        cle::ReduceVectorParcolAPI<cl_uint, cl_uint> reduce;
        reduce.initialize(clenv->context);
        cl::Event reduce_event;

        clenv->queue.enqueueWriteBuffer(
                d_data,
                CL_FALSE,
                0,
                d_data.bytes(),
                data.data(),
                NULL,
                NULL);

        kernel(
                cl::EnqueueArgs(
                    clenv->queue,
                    cl::NDRange(data.size()),
                    cl::NDRange(work_group_size_)),
                data.size(),
                num_bins_,
                d_data,
                d_histogram,
                histogram_event
              );

        reduce(
                cl::EnqueueArgs(
                    clenv->queue,
                    cl::NDRange(num_bins_ * num_work_groups),
                    cl::NullRange),
                num_work_groups,
                num_bins_,
                d_histogram,
                reduce_event
              );

        clenv->queue.enqueueReadBuffer(
                d_histogram,
                CL_TRUE,
                0,
                num_bins_ * sizeof(uint32_t),
                histogram.data(),
                NULL,
                NULL);
    }

    void performance(
            std::vector<uint32_t> const& data,
            int num_runs,
            Measurement::Measurement& measurement
            ) {

        uint32_t num_work_groups = data.size() / work_group_size_;

        cle::TypedBuffer<uint32_t> d_data(clenv->context, CL_MEM_READ_WRITE, data.size());
        cle::TypedBuffer<uint32_t> d_histogram(clenv->context, CL_MEM_READ_WRITE, num_bins_ * num_work_groups);

        Kernel kernel;
        kernel.initialize(clenv->context);

        clenv->queue.enqueueWriteBuffer(
                d_data,
                CL_FALSE,
                0,
                d_data.bytes(),
                data.data(),
                NULL,
                NULL);

        measurement.start();
        for (int r = 0; r < num_runs; ++r) {

            kernel(
                    cl::EnqueueArgs(
                        clenv->queue,
                        cl::NDRange(data.size()),
                        cl::NDRange(work_group_size_)),
                    data.size(),
                    num_bins_,
                    d_data,
                    d_histogram,
                    measurement.add_datapoint(Measurement::DataPointType::LloydMassSumMerge, r).add_opencl_event()
                  );

        }
        measurement.end();
    }

private:
    size_t work_group_size_ = 32;
    size_t num_bins_ = 2;
};

template <typename Kernel>
class HistogramTest :
    public ::testing::TestWithParam<std::tuple<size_t, size_t>>
{
protected:
    virtual void SetUp() {
        size_t work_group_size, num_bins;
        std::tie(work_group_size, num_bins) = GetParam();

        histogram_.set_work_group_size(work_group_size);
        histogram_.set_num_bins(num_bins);
    }

    virtual void TearDown() {
    }

    Histogram<Kernel> histogram_;
};

template <typename Kernel>
class HistogramPerformance :
    public ::testing::TestWithParam<std::tuple<size_t, size_t, size_t>>
{
protected:
    virtual void SetUp() {
        size_t work_group_size, num_bins, num_data;
        std::tie(work_group_size, num_bins, num_data) = GetParam();

        histogram_.set_work_group_size(work_group_size);
        histogram_.set_num_bins(num_bins);
    }

    virtual void TearDown() {
    }

    Histogram<Kernel> histogram_;
};

#define HistogramGenericTest(TypeName)                                      \
TEST_P(TypeName, UniformDistribution) {                                     \
                                                                            \
    size_t const num_data = 1 * MEGABYTE;                                    \
                                                                            \
    size_t work_group_size, num_bins;                                       \
    std::tie(work_group_size, num_bins) = GetParam();                       \
                                                                            \
    std::vector<uint32_t> data(num_data);                                   \
    std::vector<uint32_t> test_output(num_bins), verify_output(num_bins);   \
                                                                            \
    std::default_random_engine rgen;                                        \
    std::uniform_int_distribution<uint32_t> uniform(0, num_bins - 1);       \
    std::generate(                                                          \
            data.begin(),                                                   \
            data.end(),                                                     \
            [&](){ return uniform(rgen); }                                  \
            );                                                              \
                                                                            \
    histogram_.test(data, test_output);                                     \
    histogram_verify(data, verify_output);                                  \
                                                                            \
    EXPECT_TRUE(std::equal(                                                 \
                test_output.begin(),                                        \
                test_output.end(),                                          \
                verify_output.begin()));                                    \
}


using HistogramPartPrivateTest = HistogramTest<cle::HistogramPartPrivateAPI<cl_uint>>;
HistogramGenericTest(HistogramPartPrivateTest);
INSTANTIATE_TEST_CASE_P(StandardParameters,
        HistogramPartPrivateTest,
        ::testing::Combine(
            ::testing::Values(32, 64),
            ::testing::Values(2, 4)));

using HistogramPartLocalTest = HistogramTest<cle::HistogramPartLocalAPI<cl_uint>>;
HistogramGenericTest(HistogramPartLocalTest);
INSTANTIATE_TEST_CASE_P(StandardParameters,
        HistogramPartLocalTest,
        ::testing::Combine(
            ::testing::Values(32, 64),
            ::testing::Values(2, 4, 8, 16)));

using HistogramPartGlobalTest = HistogramTest<cle::HistogramPartGlobalAPI<cl_uint>>;
HistogramGenericTest(HistogramPartGlobalTest);
INSTANTIATE_TEST_CASE_P(StandardParameters,
        HistogramPartGlobalTest,
        ::testing::Combine(
            ::testing::Values(32, 64),
            ::testing::Values(2, 4, 8, 16)));

#define HistogramGenericPerformance(TypeName)                               \
TEST_P(TypeName, UniformDistribution) {                                     \
                                                                            \
    const size_t num_runs = 5;                                              \
                                                                            \
    size_t work_group_size, num_bins, num_data;                             \
    std::tie(work_group_size, num_bins, num_data) = GetParam();             \
                                                                            \
    std::vector<uint32_t> data(num_data);                                   \
    std::vector<uint32_t> test_output(num_bins), verify_output(num_bins);   \
                                                                            \
    std::default_random_engine rgen;                                        \
    std::uniform_int_distribution<uint32_t> uniform(0, num_bins - 1);       \
    std::generate(                                                          \
            data.begin(),                                                   \
            data.end(),                                                     \
            [&](){ return uniform(rgen); }                                  \
            );                                                              \
                                                                            \
    Measurement::Measurement measurement;                                   \
    measurement_setup(measurement, clenv->device, num_runs);                \
    measurement.set_parameter(                                              \
            Measurement::ParameterType::NumFeatures,                        \
            std::to_string(1));                                             \
    measurement.set_parameter(                                              \
            Measurement::ParameterType::NumPoints,                          \
            std::to_string(num_data));                                      \
    measurement.set_parameter(                                              \
            Measurement::ParameterType::NumClusters,                        \
            std::to_string(num_bins));                                      \
    measurement.set_parameter(                                              \
            Measurement::ParameterType::IntType,                            \
            "uint32_t");                                                   \
    measurement.set_parameter(                                              \
            Measurement::ParameterType::CLLocalSize,                        \
            std::to_string(work_group_size));                               \
                                                                            \
    histogram_.performance(data, num_runs, measurement);                    \
                                                                            \
    measurement.write_csv(#TypeName ".csv");                                \
                                                                            \
    SUCCEED();                                                      \
}

using HistogramPartPrivatePerformance = HistogramPerformance<cle::HistogramPartPrivateAPI<cl_uint>>;
HistogramGenericPerformance(HistogramPartPrivatePerformance);
INSTANTIATE_TEST_CASE_P(StandardParameters,
        HistogramPartPrivatePerformance,
        ::testing::Combine(
            ::testing::Values(32, 64),
            ::testing::Values(2, 4),
            ::testing::Values(64 * MEGABYTE, 128 * MEGABYTE)));

using HistogramPartLocalPerformance = HistogramPerformance<cle::HistogramPartLocalAPI<cl_uint>>;
HistogramGenericPerformance(HistogramPartLocalPerformance);
INSTANTIATE_TEST_CASE_P(StandardParameters,
        HistogramPartLocalPerformance,
        ::testing::Combine(
            ::testing::Values(32, 64),
            ::testing::Values(2, 4, 8, 16),
            ::testing::Values(64 * MEGABYTE, 128 * MEGABYTE)));

using HistogramPartGlobalPerformance = HistogramPerformance<cle::HistogramPartGlobalAPI<cl_uint>>;
HistogramGenericPerformance(HistogramPartGlobalPerformance);
INSTANTIATE_TEST_CASE_P(StandardParameters,
        HistogramPartGlobalPerformance,
        ::testing::Combine(
            ::testing::Values(32, 64),
            ::testing::Values(2, 4, 8, 16),
            ::testing::Values(64 * MEGABYTE, 128 * MEGABYTE)));

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    clenv = new CLEnvironment;
    ::testing::AddGlobalTestEnvironment(clenv);
    return RUN_ALL_TESTS();
}