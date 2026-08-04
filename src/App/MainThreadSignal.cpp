// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "MainThreadSignal.h"

#include <atomic>
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#endif

namespace
{
using TraceClock = std::chrono::steady_clock;

const auto traceStart = TraceClock::now();
std::atomic<std::uint64_t> traceSequence {0};
std::mutex traceMutex;

bool traceEnabled()
{
    static const bool enabled = [] {
        const char* value = std::getenv("FREECAD_MAIN_THREAD_TRACE");
        return value && value[0] != '\0' && value[0] != '0';
    }();
    return enabled;
}

std::uint64_t nativeThreadId()
{
#ifdef _WIN32
    return static_cast<std::uint64_t>(GetCurrentThreadId());
#else
    return static_cast<std::uint64_t>(
        std::hash<std::thread::id> {}(std::this_thread::get_id())
    );
#endif
}

void traceEvent(const char* event, const void* context = nullptr, int value = -1)
{
    if (!traceEnabled()) {
        return;
    }

    const auto sequence = traceSequence.fetch_add(1, std::memory_order_relaxed) + 1;
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        TraceClock::now() - traceStart
    ).count();

    std::scoped_lock lock(traceMutex);
    std::fprintf(
        stderr,
        "[MTS][seq=%" PRIu64 "][ms=%lld][tid=%" PRIu64
        "][event=%s][context=%p][value=%d]\n",
        sequence,
        static_cast<long long>(elapsed),
        nativeThreadId(),
        event,
        context,
        value
    );
    std::fflush(stderr);
}

struct MainThreadHooks
{
    App::MainThreadSignalConfig::IsMainThreadFn isMainThread;
    App::MainThreadSignalConfig::InvokeSyncFn invokeSync;
};

MainThreadHooks installedHooks {};
std::atomic<const MainThreadHooks*> activeHooks {nullptr};
}

namespace App
{

void MainThreadSignalConfig::installHooks(
    IsMainThreadFn isMainThread,
    InvokeSyncFn invokeSync
)
{
    traceEvent("hooks.install.begin");
    if (!isMainThread || !invokeSync) {
        traceEvent("hooks.install.invalid");
        throw std::invalid_argument(
            "Main-thread hooks must provide both an affinity predicate and a synchronous invoker"
        );
    }

    // Concurrent replacement is prohibited by the lifecycle contract. Publish
    // the complete hook pair with one release-store so readers never observe a
    // partially installed configuration.
    installedHooks = {isMainThread, invokeSync};
    activeHooks.store(&installedHooks, std::memory_order_release);
    traceEvent("hooks.install.end");
}

void MainThreadSignalConfig::clearHooks()
{
    traceEvent("hooks.clear.begin");
    // Emitters must already be stopped. Remove the complete hook snapshot with
    // one store and restore App-only inline behaviour.
    activeHooks.store(nullptr, std::memory_order_release);
    traceEvent("hooks.clear.end");
}

bool MainThreadSignalConfig::isMainThread()
{
    if (const auto* hooks = activeHooks.load(std::memory_order_acquire)) {
        const bool result = hooks->isMainThread();
        if (!result) {
            traceEvent("is-main.false", nullptr, 0);
        }
        return result;
    }
    traceEvent("is-main.no-hooks", nullptr, 1);
    return true;  // no GUI hooks: preserve same-thread behavior
}

bool MainThreadSignalConfig::hasHooks()
{
    const bool result = activeHooks.load(std::memory_order_acquire) != nullptr;
    traceEvent("has-hooks", nullptr, result ? 1 : 0);
    return result;
}

bool MainThreadSignalConfig::invokeSync(TaskFn task, void* context)
{
    traceEvent("invoke.enter", context);
    if (const auto* hooks = activeHooks.load(std::memory_order_acquire)) {
        traceEvent("invoke.hook.begin", context);
        const bool result = hooks->invokeSync(task, context);
        traceEvent("invoke.hook.end", context, result ? 1 : 0);
        return result;
    }

    traceEvent("invoke.inline.begin", context);
    task(context);
    traceEvent("invoke.inline.end", context, 1);
    return true;
}

}  // namespace App
