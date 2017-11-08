/*
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * Copyright (c) 2017, Lutz, Clemens <lutzcle@cml.li>
 */

#ifndef SINGLE_DEVICE_SCHEDULER_HPP
#define SINGLE_DEVICE_SCHEDULER_HPP

#include <buffer_cache.hpp>
#include <device_scheduler.hpp>

#include <array>
#include <functional>
#include <future>
#include <deque>
#include <memory>
#include <cstdint>

#include <boost/compute/buffer.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/event.hpp>

namespace Clustering {
    class SingleDeviceScheduler : public DeviceScheduler {
    public:

        using Buffer = boost::compute::buffer;
        using Context = boost::compute::context;
        using Device = boost::compute::device;
        using Event = boost::compute::event;
        using FunUnary = typename DeviceScheduler::FunUnary;
        using FunBinary = typename DeviceScheduler::FunBinary;
        using Queue = boost::compute::command_queue;

        int add_buffer_cache(std::shared_ptr<BufferCache> buffer_cache);
        int add_device(Context context, Device device);

        int enqueue(FunUnary kernel_function, uint32_t object_id, std::future<std::deque<Event>>& kernel_events);
        int enqueue(FunBinary function, uint32_t fst_object_id, uint32_t snd_object_id, std::future<std::deque<Event>>& kernel_events);
        int enqueue_barrier();

    private:

        struct DeviceInfo {
            std::array<Queue, 2> qpair;
        } device_info_i;

        std::shared_ptr<BufferCache> buffer_cache_i;
    };
} // namespace Clustering

#endif /* SINGLE_DEVICE_SCHEDULER_HPP */
