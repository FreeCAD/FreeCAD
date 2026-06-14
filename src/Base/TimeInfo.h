// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2024 Ladislav Michl <ladis@linux-mips.org>              *
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

#include <chrono>
#include <sstream>
#include <string>

#include "Console.h"

namespace Base
{

using Clock = std::chrono::system_clock;

class TimeInfo: public std::chrono::time_point<Clock>
{
private:
    bool _null;

public:
    TimeInfo()
    {
        setCurrent();
    }

    TimeInfo(const TimeInfo&) = default;
    TimeInfo(TimeInfo&&) = default;
    ~TimeInfo() = default;

    void setCurrent()
    {
        static_cast<std::chrono::time_point<Clock>&>(*this) = Clock::now();
        _null = false;
    }

    void setTime_t(std::time_t time)
    {
        static_cast<std::chrono::time_point<Clock>&>(*this) = Clock::from_time_t(time);
        _null = false;
    }

    std::time_t getTime_t()
    {
        return Clock::to_time_t(*this);
    }

    static float diffTimeF(const TimeInfo& start, const TimeInfo& end = TimeInfo())
    {
        const std::chrono::duration<float> duration = end - start;
        return duration.count();
    }

    static std::string diffTime(const TimeInfo& start, const TimeInfo& end = TimeInfo())
    {
        std::stringstream ss;
        const std::chrono::duration<float> secs = end - start;
        ss << secs.count();
        return ss.str();
    }

    bool isNull() const
    {
        return _null;
    }

    static TimeInfo null()
    {
        TimeInfo ti;
        ti._null = true;
        return ti;
    }
};  // class TimeInfo

using Ticks = std::chrono::steady_clock;

class TimeElapsed: public std::chrono::time_point<Ticks>
{
public:
    TimeElapsed()
    {
        setCurrent();
    }

    TimeElapsed(const TimeElapsed&) = default;
    TimeElapsed(TimeElapsed&&) = default;
    ~TimeElapsed() = default;

    void setCurrent()
    {
        static_cast<std::chrono::time_point<Ticks>&>(*this) = Ticks::now();
    }

    static float diffTimeF(const TimeElapsed& start, const TimeElapsed& end = TimeElapsed())
    {
        const std::chrono::duration<float> duration = end - start;
        return duration.count();
    }

    static std::string diffTime(const TimeElapsed& start, const TimeElapsed& end = TimeElapsed())
    {
        std::stringstream ss;
        const std::chrono::duration<float> secs = end - start;
        ss << secs.count();
        return ss.str();
    }
};  // class TimeElapsed

/**
 * @class TimeTracker
 * @brief A utility class for tracking time intervals and logging checkpoints.
 *
 * This class facilitates time tracking by recording named checkpoints and
 * calculating time intervals between them. It logs the time elapsed between
 * checkpoints and the total time since the start. The time tracking automatically
 * begins upon object instantiation and ends when the object is destroyed.
 *
 * The recommended way to use this class for performance optimization is through
 * manual bisection. If a specific operation (e.g., a feature recompute) is slow,
 * instrument the top-level function with a `TimeTracker` and several checkpoints.
 *
 * Once the most time-consuming segment is identified, move the instrumentation
 * into the underlying functions called during that segment. Repeat this process
 * recursively to isolate the root cause of the performance bottleneck.
 *
 * @code
 * {
 *     Base::TimeTracker tracker("Document Loading");
 *     // ... heavy computation ...
 *     tracker.checkpoint("Parsing XML");
 *     // ... more work ...
 *     tracker.checkpoint("Building Topology");
 *     // ... even more ...
 * } // "finish" checkpoint is logged automatically here
 * @endcode
 */
class TimeTracker
{
public:
    explicit TimeTracker(std::string name)
        : name(std::move(name))
        , lastCheckpoint("start")
    {}

    FC_DISABLE_COPY_MOVE(TimeTracker);

    void checkpoint(const std::string& checkpoint = "")
    {
        Console().log(
            "(%s) %s -> %s: %f (%f total)\n",
            name,
            lastCheckpoint,
            checkpoint,
            TimeElapsed::diffTimeF(lastCheckpointTime),
            TimeElapsed::diffTimeF(start)
        );

        lastCheckpoint = checkpoint;
        lastCheckpointTime.setCurrent();
    }

    ~TimeTracker()
    {
        checkpoint("finish");
    }

private:
    std::string name;
    std::string lastCheckpoint;

    TimeElapsed start;
    TimeElapsed lastCheckpointTime;
};

}  // namespace Base
