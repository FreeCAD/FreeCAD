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

namespace
{
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
    if (!isMainThread || !invokeSync) {
        throw std::invalid_argument(
            "Main-thread hooks must provide both an affinity predicate and a synchronous invoker"
        );
    }

    // Concurrent replacement is prohibited by the lifecycle contract. Publish
    // the complete hook pair with one release-store so readers never observe a
    // partially installed configuration.
    installedHooks = {isMainThread, invokeSync};
    activeHooks.store(&installedHooks, std::memory_order_release);
}

void MainThreadSignalConfig::clearHooks()
{
    // Emitters must already be stopped. Remove the complete hook snapshot with
    // one store and restore App-only inline behaviour.
    activeHooks.store(nullptr, std::memory_order_release);
}

bool MainThreadSignalConfig::isMainThread()
{
    if (const auto* hooks = activeHooks.load(std::memory_order_acquire)) {
        return hooks->isMainThread();
    }
    return true;  // no GUI hooks: preserve same-thread behavior
}

bool MainThreadSignalConfig::hasHooks()
{
    return activeHooks.load(std::memory_order_acquire) != nullptr;
}

bool MainThreadSignalConfig::invokeSync(TaskFn task, void* context)
{
    if (const auto* hooks = activeHooks.load(std::memory_order_acquire)) {
        return hooks->invokeSync(task, context);
    }

    task(context);
    return true;
}

}  // namespace App
