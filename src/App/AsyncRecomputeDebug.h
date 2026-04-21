// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

#pragma once

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

namespace App
{

inline std::filesystem::path asyncRecomputeDebugLogPath()
{
    static const std::filesystem::path path
        = std::filesystem::temp_directory_path() / "freecad-async-recompute-debug.log";
    return path;
}

inline bool asyncRecomputeDebugLogEnabled()
{
    static const bool enabled = []() {
        const char* value = std::getenv("FC_ASYNC_RECOMPUTE_DEBUG_LOG");
        if (!value || !*value) {
            return false;
        }

        const std::string_view text(value);
        return text != "0" && text != "false" && text != "False" && text != "FALSE";
    }();
    return enabled;
}

inline void appendAsyncRecomputeDebugLog(const std::string& message)
{
    if (!asyncRecomputeDebugLogEnabled()) {
        return;
    }

    static std::mutex mutex;

    std::lock_guard<std::mutex> lock(mutex);
    std::ofstream stream(asyncRecomputeDebugLogPath(), std::ios::app);
    if (!stream) {
        return;
    }

    const auto now = std::chrono::system_clock::now();
    const auto nowTime = std::chrono::system_clock::to_time_t(now);
    stream << std::put_time(std::localtime(&nowTime), "%F %T");
    stream << " tid=" << std::this_thread::get_id() << ' ' << message << '\n';
}

}  // namespace App
