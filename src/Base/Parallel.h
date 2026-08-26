// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#pragma once

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <exception>
#include <mutex>
#include <system_error>
#include <thread>
#include <vector>


namespace Base
{

/**
 * @brief Number of worker threads a parallel algorithm may use.
 *
 * @param limit Upper bound requested by the caller, 0 means "no explicit limit".
 * @return At least 1, never more than the reported hardware concurrency.
 */
inline unsigned int maxParallelThreads(unsigned int limit = 0)
{
    unsigned int hardware = std::thread::hardware_concurrency();
    if (hardware == 0) {
        // hardware_concurrency() is allowed to return 0 when it cannot tell
        hardware = 1;
    }

    if (limit == 0) {
        return hardware;
    }

    return std::min(limit, hardware);
}

/**
 * @brief Apply @p func to every index in [begin, end) using several threads.
 *
 * Indices are handed out dynamically in blocks of @p grainSize, so uneven work
 * per index does not leave threads idle. @p func must be safe to call
 * concurrently for different indices; writing to disjoint slices of a
 * preallocated buffer is the intended use.
 *
 * The calling thread takes part in the work, so no threads are spawned when the
 * range is small enough to be handled serially. If @p func throws, the
 * remaining indices are skipped and the first exception is rethrown on the
 * calling thread once all workers have finished.
 *
 * @param begin First index, inclusive.
 * @param end Last index, exclusive.
 * @param func Callable invoked as func(index).
 * @param grainSize Number of consecutive indices claimed per block.
 * @param threadLimit Upper bound on worker threads, 0 means hardware concurrency.
 */
template<class Index, class Function>
void parallelFor(
    Index begin,
    Index end,
    Function&& func,
    std::size_t grainSize = 1,
    unsigned int threadLimit = 0
)
{
    if (end <= begin) {
        return;
    }

    if (grainSize == 0) {
        grainSize = 1;
    }

    const auto count = static_cast<std::size_t>(end - begin);
    const std::size_t blocks = (count + grainSize - 1) / grainSize;

    unsigned int threads = maxParallelThreads(threadLimit);
    if (blocks < threads) {
        threads = static_cast<unsigned int>(blocks);
    }

    if (threads < 2) {
        for (Index i = begin; i < end; ++i) {
            func(i);
        }
        return;
    }

    std::atomic<std::size_t> nextBlock {0};
    std::atomic<bool> failed {false};
    std::mutex errorMutex;
    std::exception_ptr firstError;

    auto worker = [&]() {
        try {
            while (!failed.load(std::memory_order_relaxed)) {
                const std::size_t first = nextBlock.fetch_add(grainSize, std::memory_order_relaxed);
                if (first >= count) {
                    return;
                }

                const std::size_t last = std::min(first + grainSize, count);
                for (std::size_t offset = first; offset < last; ++offset) {
                    func(begin + static_cast<Index>(offset));
                }
            }
        }
        catch (...) {
            const std::lock_guard<std::mutex> guard(errorMutex);
            if (!firstError) {
                firstError = std::current_exception();
            }
            failed.store(true, std::memory_order_relaxed);
        }
    };

    std::vector<std::thread> pool;
    pool.reserve(threads - 1);
    try {
        for (unsigned int i = 1; i < threads; ++i) {
            pool.emplace_back(worker);
        }
    }
    catch (const std::system_error&) {
        // Getting fewer threads than asked for is not fatal, the calling thread
        // drains whatever is left of the range on its own.
    }

    worker();

    for (auto& thread : pool) {
        thread.join();
    }

    if (firstError) {
        std::rethrow_exception(firstError);
    }
}

}  // namespace Base
