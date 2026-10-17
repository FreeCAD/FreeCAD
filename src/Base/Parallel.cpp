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

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <system_error>
#include <thread>
#include <vector>

#include "Parallel.h"


using namespace Base;

namespace
{

/**
 * Fixed set of worker threads shared by every Base::parallelFor call.
 *
 * Creating and joining threads per call costs far more than the loops this is used for
 * take to run, so the threads are created once and parked on a condition variable
 * between loops. Work is claimed from a single atomic cursor, which keeps the
 * distribution dynamic without needing a queue per worker.
 */
class ThreadPool
{
public:
    static ThreadPool& instance()
    {
        // Deliberately never destroyed. Joining worker threads from a static destructor
        // risks deadlocking against the loader lock when a module is unloaded, and the
        // pool is meant to live as long as the process anyway. The workers are parked on
        // a condition variable at exit and hold no resources the OS will not reclaim.
        static ThreadPool* pool = new ThreadPool();
        return *pool;
    }

    unsigned int size() const
    {
        return static_cast<unsigned int>(_workers.size()) + 1;
    }

    static bool isWorker()
    {
        return _isWorker;
    }

    void run(
        std::size_t count,
        std::size_t grainSize,
        unsigned int participants,
        Detail::ParallelBody body,
        void* context
    )
    {
        {
            const std::lock_guard<std::mutex> lock(_mutex);
            _body = body;
            _context = context;
            _count = count;
            _grainSize = grainSize;
            _cursor.store(0, std::memory_order_relaxed);
            _slots.store(static_cast<int>(participants) - 1, std::memory_order_relaxed);
            _failed.store(false, std::memory_order_relaxed);
            _pending = _workers.size();
            _error = nullptr;
            ++_generation;
        }
        _workAvailable.notify_all();

        drain();

        std::unique_lock<std::mutex> lock(_mutex);
        _workFinished.wait(lock, [this] { return _pending == 0; });

        if (_error) {
            std::exception_ptr error = _error;
            _error = nullptr;
            lock.unlock();
            std::rethrow_exception(error);
        }
    }

private:
    ThreadPool()
    {
        unsigned int count = std::thread::hardware_concurrency();
        if (count < 2) {
            count = 1;
        }

        _workers.reserve(count - 1);
        try {
            for (unsigned int i = 1; i < count; ++i) {
                _workers.emplace_back([this] { workerLoop(); });
            }
        }
        catch (const std::system_error&) {
            // Fewer workers than requested is not fatal; callers still get the work done,
            // just with less of it running in parallel.
        }
    }

    void drain()
    {
        try {
            while (!_failed.load(std::memory_order_relaxed)) {
                const std::size_t first = _cursor.fetch_add(_grainSize, std::memory_order_relaxed);
                if (first >= _count) {
                    return;
                }

                const std::size_t last = std::min(first + _grainSize, _count);
                for (std::size_t offset = first; offset < last; ++offset) {
                    _body(_context, offset);
                }
            }
        }
        catch (...) {
            const std::lock_guard<std::mutex> lock(_mutex);
            if (!_error) {
                _error = std::current_exception();
            }
            _failed.store(true, std::memory_order_relaxed);
        }
    }

    void workerLoop()
    {
        _isWorker = true;

        unsigned int seen = 0;
        for (;;) {
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _workAvailable.wait(lock, [this, &seen] { return _generation != seen; });
                seen = _generation;
            }

            // Only as many workers as the caller asked for take part; the rest go
            // straight back to sleep after reporting in.
            if (_slots.fetch_sub(1, std::memory_order_relaxed) > 0) {
                drain();
            }

            {
                const std::lock_guard<std::mutex> lock(_mutex);
                if (--_pending == 0) {
                    _workFinished.notify_one();
                }
            }
        }
    }

    static thread_local bool _isWorker;

    std::vector<std::thread> _workers;
    std::mutex _mutex;
    std::condition_variable _workAvailable;
    std::condition_variable _workFinished;

    Detail::ParallelBody _body {nullptr};
    void* _context {nullptr};
    std::size_t _count {0};
    std::size_t _grainSize {1};
    std::atomic<std::size_t> _cursor {0};
    std::atomic<int> _slots {0};
    std::atomic<bool> _failed {false};
    std::size_t _pending {0};
    std::exception_ptr _error;
    unsigned int _generation {0};
};

thread_local bool ThreadPool::_isWorker = false;

}  // namespace


unsigned int Base::maxParallelThreads(unsigned int limit)
{
    const unsigned int available = ThreadPool::instance().size();
    if (limit == 0) {
        return available;
    }

    return std::min(limit, available);
}

std::size_t Base::balancedGrainSize(std::size_t count, unsigned int threadLimit)
{
    const unsigned int threads = maxParallelThreads(threadLimit);
    if (threads < 2 || count == 0) {
        return 1;
    }

    // Several blocks per worker keeps stealing effective when the per-item cost varies.
    constexpr std::size_t blocksPerThread = 8;
    const std::size_t blocks = static_cast<std::size_t>(threads) * blocksPerThread;
    return std::max<std::size_t>(1, count / blocks);
}

bool Base::Detail::runParallel(
    std::size_t count,
    std::size_t grainSize,
    unsigned int threadLimit,
    ParallelBody body,
    void* context
)
{
    if (count == 0 || body == nullptr) {
        return false;
    }

    if (grainSize == 0) {
        grainSize = 1;
    }

    // Nested parallelism is flattened. A worker that blocked waiting for the pool it is
    // itself part of could deadlock, and the outer loop already saturates the machine.
    if (ThreadPool::isWorker()) {
        return false;
    }

    ThreadPool& pool = ThreadPool::instance();
    unsigned int participants = pool.size();
    if (threadLimit > 0) {
        participants = std::min(threadLimit, participants);
    }
    if (participants < 2) {
        return false;
    }

    const std::size_t blocks = (count + grainSize - 1) / grainSize;
    if (blocks < 2) {
        return false;
    }
    if (blocks < participants) {
        participants = static_cast<unsigned int>(blocks);
    }

    pool.run(count, grainSize, participants, body, context);
    return true;
}
