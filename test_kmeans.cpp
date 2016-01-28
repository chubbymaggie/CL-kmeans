#include "csv.hpp"
#include "common.hpp"
#include "cle/common.hpp"
#include "clustering_benchmark.hpp"

#include "kmeans.hpp"

#include <iostream>
#include <vector>
#include <algorithm>
#include <memory>

int main(int argc, char **argv) {

  if (argc != 2) {
    std::cerr << "Usage: test_kmeans [input file]" << std::endl;
    return 1;
  }

  char *input_path = argv[1];

  cle::CSV csv;
  std::vector<double> points_x, points_y;

  cle::CLInitializer clinit;
  if (clinit.choose_platform_interactive() < 0) {
      return 0;
  }
  if (clinit.choose_device_interactive() < 0) {
      return 0;
  }


  csv.read_csv(input_path, points_x, points_y);
  std::cout << "Read " << input_path << std::endl;
  uint64_t bytes_read = (points_x.size() + points_y.size()) * sizeof(double);
  std::cout << "Read " << bytes_read / 1024 / 1024 << " MiB" << std::endl;

  constexpr uint32_t num_runs = 5;
  constexpr uint32_t max_iterations = 100;
  constexpr uint64_t num_clusters = 9;
  uint64_t num_points = points_x.size();

  cle::ClusteringBenchmark64 bm64(
          num_runs, num_points, max_iterations,
          std::move(points_x), std::move(points_y));
  bm64.initialize(num_clusters, cle::KmeansInitializer::first_x);

  cle::KmeansNaive64 kmeans_naive_64;
  kmeans_naive_64.initialize();

  cle::KmeansGPUAssisted kmeans_gpu_64(
          clinit.get_context(), clinit.get_commandqueue()
          );
  kmeans_gpu_64.initialize();

  bm64.setVerificationReference(kmeans_naive_64);

  int is_kmeans_gpu_64_correct = bm64.verify(kmeans_gpu_64);
  if (is_kmeans_gpu_64_correct) {
      std::cout << "GPU assisted is correct" << std::endl;
  }
  else {
      std::cout << "GPU assisted is wrong!!!" << std::endl;
  }

  cle::ClusteringBenchmarkStats bs_naive_64 = bm64.run(kmeans_naive_64);
  cle::ClusteringBenchmarkStats bs_gpu_64 = bm64.run(kmeans_gpu_64);

  kmeans_naive_64.finalize();
  kmeans_gpu_64.finalize();

  bm64.finalize();

  bs_naive_64.print_times();
  bs_gpu_64.print_times();

  // std::cout << "Point: Naive | GPU" << std::endl;
  // uint32_t num_diff = 0;
  // for (uint32_t p = 0; p < points_x.size(); ++p) {
  //     if (memberships_naive[p] != memberships_gpu[p]) {
  //         ++num_diff;
          // std::cout << p << ": " << memberships_naive[p] << " | " << memberships_gpu[p] << std::endl;
  //     }
  // }
  // std::cout << "[" << num_diff << " / " << points_x.size() << "]" << std::endl;
}
