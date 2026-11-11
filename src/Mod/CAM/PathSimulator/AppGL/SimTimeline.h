// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Billy Huddleston <billy@ivdc.com>
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

#pragma once

#include "MillMotion.h"
#include "MillPathSegment.h"
#include <chrono>
#include <vector>

namespace CAMSimulator
{

class SimRotary;

/**
 * The program on a time line: segment durations, operation starts, rotary turns and skip marks.
 * Times are seconds of machine time from the program start.
 */
class SimTimeline
{
public:
    // times closer than this, in seconds, are the same
    static constexpr float TimeTolerance = 1e-4f;

    SimTimeline(
        const std::vector<MillPathSegment*>& segments,
        const SimRotary& rotary,
        const std::vector<MillFrame>& frames
    );

    // forgets the previous program's operation starts
    void Clear();
    // Times every segment and records operation starts, rotary turns and marks; returns the
    // program's time.
    float ComputeTimes();

    // The machine time `seconds` of real time after simTime at speed; table turns are slowed so
    // they can be seen.
    float Advance(float simTime, float seconds, float speed) const;
    // the step running at t (nTotalSteps at or past the end)
    int StepAtTime(float t, int nTotalSteps) const;
    // the time a step starts at
    float TimeOfStep(int step, int nTotalSteps) const;

    // operation starts, as fractions of the program time
    const std::vector<float>& OperationStarts() const
    {
        return mOpStarts;
    }
    // rotary turns, in indexes and while cutting, as fractions of the program time
    const std::vector<SimTimeSpan>& IndexSpans() const
    {
        return mIndexSpans;
    }
    // the marks the skip buttons stop at, sorted
    const std::vector<float>& Marks() const
    {
        return mMarks;
    }

    // pauses briefly after a back skip, so another press goes further back
    void HoldPlayback();
    bool IsHolding() const;

private:
    using clock = std::chrono::steady_clock;

    const MillPathSegment* SegmentAtTime(float t) const;

    const std::vector<MillPathSegment*>& mSegments;
    const SimRotary& mRotary;
    const std::vector<MillFrame>& mFrames;
    float mTotalTime = 0;
    std::vector<float> mOpStarts;
    std::vector<SimTimeSpan> mIndexSpans;
    std::vector<float> mMarks;     // skip marks in seconds, sorted
    clock::time_point mHoldUntil;  // end of the pause after a back skip
};

}  // namespace CAMSimulator
