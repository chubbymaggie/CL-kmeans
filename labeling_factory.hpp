#ifndef LABELING_FACTORY_HPP
#define LABELING_FACTORY_HPP

#include "temp.hpp"

#include "labeling_configuration.hpp"

#include "cl_kernels/labeling_unroll_vector.hpp"

#include <functional>
#include <string>

#include <boost/compute/core.hpp>
#include <boost/compute/container/vector.hpp>

namespace Clustering {

template <typename PointT, typename LabelT, bool ColMajor>
class LabelingFactory {
public:
    template <typename T>
    using Vector = boost::compute::vector<T>;

    using LabelingFunction = std::function<
        boost::compute::event(
                boost::compute::command_queue queue,
                size_t num_features,
                size_t num_points,
                size_t num_clusters,
                Vector<char>& did_changes,
                Vector<PointT>& points,
                Vector<PointT>& centroids,
                Vector<LabelT>& labels,
                MeasurementLogger& logger,
                boost::compute::wait_list const& events
            )
        >;

    LabelingFunction create(std::string flavor, boost::compute::context context, LabelingConfiguration config) {

        if (flavor.compare("unroll_vector")) {
            LabelingUnrollVector<PointT, LabelT, ColMajor> flavor;
            flavor.prepare(context, config);
            return flavor;
        }
    }
};

}

#endif /* LABELING_FACTORY_HPP */
