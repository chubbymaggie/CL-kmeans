/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * Copyright (c) 2017, Lutz, Clemens <lutzcle@cml.li>
 */

#include "../matrix.hpp"
#include "../binary_format.hpp"
#include "../timer.hpp"

#include <armadillo>

#include <iostream>
#include <string>
#include <chrono>
#include <cstdlib> // atoi

#ifndef FLOAT_T
#define FLOAT_T double
#endif

template <typename T>
class ArmadilloKmeans {
public:

  void load(cle::Matrix<T, std::allocator<T>, size_t, true> const& points)
  {
    arma::Mat<T> new_points(
            points.data(),
            points.rows(),
            points.cols());

    arma::inplace_trans(new_points);

    points_ = std::move(new_points);

  }

  uint64_t operator() (
      uint32_t max_iterations,
      uint32_t num_clusters
      )
  {
    // No data copy, reference existing centroids
    arma::Mat<T> arma_centroids(
        points_.n_rows,
        num_clusters
        );

    Timer::Timer cpu_timer;
    cpu_timer.start();

    arma::kmeans(
        arma_centroids,
        points_,
        num_clusters,
        // arma::keep_existing,
        arma::static_subset,
        max_iterations,
        false);

    uint64_t total_time =
        cpu_timer.stop<std::chrono::milliseconds>();

    return total_time;
  }

private:

  arma::Mat<T> points_;
};

int main(int argc, char **argv) {

  uint32_t k = 0;
  uint32_t max_iterations = 0;
  uint32_t repititions = 5;
  uint64_t time = 0;
  std::string file_path;
  ArmadilloKmeans<FLOAT_T> armakmeans;

  if (argc != 4 || (argc == 2 && (std::string(argv[1]) == "--help"))) {
    std::cout
      << "Usage: kmeans_armadillo [file] [max iterations] [k]"
      << std::endl;
    std::exit(1);
  }

  file_path = argv[1];
  max_iterations = std::atoi(argv[2]);
  k = std::atoi(argv[3]);

  {
    cle::Matrix<FLOAT_T, std::allocator<FLOAT_T>, size_t> matrix;
    Clustering::BinaryFormat binformat;

    binformat.read(file_path.c_str(), matrix);
    armakmeans.load(matrix);
  }

  std::cout << "Time (ms)" << std::endl;
  for (uint32_t r = 0; r < repititions; ++r) {
    time = armakmeans(max_iterations, k);
    std::cout << time << std::endl;
  }

}
