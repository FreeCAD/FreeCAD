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

#include <FCGlobal.h>

#include <cstddef>


namespace Base
{

/**
 * @brief Number of worker threads a parallel algorithm may use.
 *
 * @param limit Upper bound requested by the caller, 0 means "no explicit limit".
 * @return At least 1, never more than the size of the shared worker pool.
 */
BaseExport unsigned int maxParallelThreads(unsigned int limit = 0);

/**
 * @brief Grain size that keeps @p count items balanced over the worker pool.
 *
 * Handing out single items costs one atomic operation each, which dominates when the
 * per-item work is small. This returns a block size that still gives every worker
 * several blocks to steal from, so load stays balanced without that overhead.
 */
BaseExport std::size_t balancedGrainSize(std::size_t count, unsigned int threadLimit = 0);

namespace Detail
{

/// Type-erased body of a parallel loop, invoked as body(context, index).
using ParallelBody = void (*)(void*, std::size_t);

/**
 * @brief Run @p body over [0, count) on the shared worker pool and wait for it.
 *
 * The calling thread takes part in the work. Returns false without running anything if
 * the work should be done serially by the caller instead, which happens when the pool
 * has no workers, when @p threadLimit is 1, when there is less than one block per
 * worker, or when the caller is itself a pool worker (nested parallelism is flattened
 * so it cannot deadlock).
 *
 * Exceptions escaping @p body are caught; the first one is rethrown on the calling
 * thread once every worker has finished.
 */
BaseExport bool runParallel(
    std::size_t count,
    std::size_t grainSize,
    unsigned int threadLimit,
    ParallelBody body,
    void* context
);

}  // namespace Detail

/**
 * @brief Apply @p func to every index in [begin, end) using the shared worker pool.
 *
 * Indices are handed out dynamically in blocks of @p grainSize, so uneven work per index
 * does not leave workers idle. @p func must be safe to call concurrently for different
 * indices; writing to disjoint slices of a preallocated buffer is the intended use.
 *
 * The pool is created once and reused, so a call costs a condition-variable wake rather
 * than thread creation. Small ranges still run inline on the calling thread.
 *
 * @param begin First index, inclusive.
 * @param end Last index, exclusive.
 * @param func Callable invoked as func(index).
 * @param grainSize Number of consecutive indices claimed per block.
 * @param threadLimit Upper bound on participating threads, 0 means the whole pool.
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

    struct Context
    {
        Function* func;
        Index begin;
    } context {&func, begin};

    const auto body = [](void* ctx, std::size_t offset) {
        auto* self = static_cast<Context*>(ctx);
        (*self->func)(self->begin + static_cast<Index>(offset));
    };

    if (Detail::runParallel(count, grainSize, threadLimit, body, &context)) {
        return;
    }

    for (Index i = begin; i < end; ++i) {
        func(i);
    }
}

}  // namespace Base
