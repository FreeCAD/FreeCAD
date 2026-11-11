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

#include "SimTimeline.h"

#include "SimRotary.h"
#include <algorithm>
#include <cmath>
#include <optional>

namespace CAMSimulator
{

namespace
{

// Rotary axis moves smaller than this, in degrees, are ignored.
constexpr float MinAxisTurn = 1e-4f;
// Steps per second for a move with no known feed rate (the old fixed playback pace).
constexpr float StepsPerSecondNoFeed = 60.f;
// most segments Advance walks in one call, a guard against endless loops
constexpr int MaxAdvanceSegments = 100000;
// Fastest shown table turn at x1, in degrees per real second, scaled by the playback speed; real
// turns are often too fast to see.
constexpr float MaxShownTurnRate = 90.f;
// How long playback pauses at a mark after a back skip, so a second press can go further back.
constexpr std::chrono::milliseconds BackSkipHold(750);

}  // namespace

SimTimeline::SimTimeline(
    const std::vector<MillPathSegment*>& segments,
    const SimRotary& rotary,
    const std::vector<MillFrame>& frames
)
    : mSegments(segments)
    , mRotary(rotary)
    , mFrames(frames)
{}

void SimTimeline::Clear()
{
    mOpStarts.clear();
}

float SimTimeline::ComputeTimes()
{
    // The machine time of each segment: its length at its feed rate, as the cycle time estimate
    // counts it. A move with no known feed rate keeps the old fixed pace of steps per second. An
    // index takes at least as long as the rotary axes need to turn.
    const std::vector<SimRotaryAxis>& axes = mRotary.Axes();
    mTotalTime = 0;
    for (MillPathSegment* p : mSegments) {
        // A continuous rotary move is timed as the control does: by its X, Y, Z travel, or by
        // the turn in degrees when only the rotary axes move.
        const float length = !p->continuous ? p->Length()
            : p->machineLength > 1e-6f      ? p->machineLength
                                            : p->rotaryTravel;
        float duration = p->feed > 0 ? length / p->feed
                                     : (float)p->numSimSteps / StepsPerSecondNoFeed;
        if (p->continuous) {
            // and no faster than the rotary axes can turn
            for (size_t k = 0; k < p->angFrom.size() && k < axes.size(); k++) {
                if (axes[k].rate > 0) {
                    const float turn = std::fabs(p->angTo[k] - p->angFrom[k]);
                    duration = std::max(duration, turn / axes[k].rate);
                }
            }
        }
        p->rotaryTime = 0;
        if (mRotary.HasAxisAngles(p)) {
            p->rotaryTime = mRotary.IndexAngles(p, 0, nullptr);
        }
        else if (p->turn > 0 && mFrames[p->frameTo].indexRate > 0) {
            p->rotaryTime = p->turn / mFrames[p->frameTo].indexRate;
        }
        p->duration = std::max(duration, p->rotaryTime);
        p->startTime = mTotalTime;
        mTotalTime += p->duration;
    }

    // Record when each operation starts and when the rotary axes turn (per axis, or the whole
    // table). The skip buttons stop at each of these times.
    mOpStarts.clear();
    mIndexSpans.clear();
    mMarks.clear();
    const float total = mTotalTime > 0 ? mTotalTime : 1.f;
    int op = -1;
    for (const MillPathSegment* p : mSegments) {
        if (p->op != op) {
            op = p->op;
            mOpStarts.push_back(p->startTime / total);
            mMarks.push_back(p->startTime);
        }
        if (p->rotaryTime <= 0) {
            continue;
        }
        if (mRotary.HasAxisAngles(p)) {
            const std::vector<float>& a = p->angFrom;
            const std::vector<float>& b = p->angTo;
            std::vector<float> begin, span;
            mRotary.IndexSchedule(p, begin, span);
            for (size_t k = 0; k < a.size(); k++) {
                if (std::fabs(b[k] - a[k]) > MinAxisTurn && span[k] > 0) {
                    const float t0 = p->startTime + begin[k];
                    const float t1 = t0 + span[k];
                    mIndexSpans.push_back({t0 / total, t1 / total, (int)k});
                    mMarks.push_back(t0);
                    mMarks.push_back(t1);
                }
            }
        }
        else {
            const float t1 = p->startTime + p->rotaryTime;
            mIndexSpans.push_back({p->startTime / total, t1 / total, -1});
            mMarks.push_back(p->startTime);
            mMarks.push_back(t1);
        }
    }
    // the axes that turn while cutting, each turn as one span over the moves it runs through
    const size_t axisCount = mRotary.Axes().size();
    std::vector<std::optional<SimTimeSpan>> runs(axisCount);
    for (const MillPathSegment* p : mSegments) {
        const bool sized = p->angFrom.size() == axisCount && p->angTo.size() == axisCount;
        for (size_t k = 0; k < axisCount; k++) {
            const bool turns = p->continuous && sized
                && std::fabs(p->angTo[k] - p->angFrom[k]) > MinAxisTurn;
            const float from = p->startTime / total;
            const float to = (p->startTime + p->duration) / total;
            if (turns && runs[k] && runs[k]->end >= from - 1e-6f) {
                runs[k]->end = to;
                continue;
            }
            if (runs[k]) {
                mIndexSpans.push_back(*runs[k]);
                runs[k].reset();
            }
            if (turns) {
                runs[k] = SimTimeSpan {from, to, (int)k, true};
            }
        }
    }
    for (const std::optional<SimTimeSpan>& run : runs) {
        if (run) {
            mIndexSpans.push_back(*run);
        }
    }
    std::sort(mMarks.begin(), mMarks.end());
    return mTotalTime;
}

const MillPathSegment* SimTimeline::SegmentAtTime(float t) const
{
    // the last segment starting at or before t
    auto it = std::upper_bound(
        mSegments.begin(),
        mSegments.end(),
        t,
        [](float time, const MillPathSegment* p) { return time < p->startTime; }
    );
    return *(it == mSegments.begin() ? it : it - 1);
}

float SimTimeline::Advance(float simTime, float seconds, float speed) const
{
    if (mSegments.empty()) {
        return simTime;
    }
    for (int guard = 0; seconds > 0 && simTime < mTotalTime && guard < MaxAdvanceSegments; guard++) {
        const MillPathSegment* p = SegmentAtTime(simTime);
        float rate = speed;
        const float shown = mRotary.HasAxisAngles(p) ? p->axisTurn : p->turn;
        if (shown > 0 && p->duration > 0) {
            rate = std::min(rate, speed * p->duration * MaxShownTurnRate / shown);
        }
        const float end = p->startTime + p->duration;
        const float needed = (end - simTime) / rate;  // real seconds to finish the segment
        if (needed > seconds) {
            return simTime + seconds * rate;
        }
        simTime = end;
        seconds -= needed;
        if (p == mSegments.back()) {
            break;
        }
        if (p->duration <= 0) {
            // step past a zero-length segment, or the lookup finds it again
            simTime = std::nextafter(simTime, mTotalTime);
        }
    }
    return std::min(simTime, mTotalTime);
}

int SimTimeline::StepAtTime(float t, int nTotalSteps) const
{
    // the running segment, and how far along it is
    if (mSegments.empty() || t >= mTotalTime) {
        return nTotalSteps;
    }
    const MillPathSegment* p = SegmentAtTime(t);
    const float frac = p->duration > 0 ? (t - p->startTime) / p->duration : 1.f;
    const int sub = std::clamp((int)(frac * (float)p->numSimSteps), 0, p->numSimSteps - 1);
    return p->firstStep + sub;
}

float SimTimeline::TimeOfStep(int step, int nTotalSteps) const
{
    if (mSegments.empty() || step >= nTotalSteps) {
        return mTotalTime;
    }
    auto it = std::upper_bound(
        mSegments.begin(),
        mSegments.end(),
        step,
        [](int s, const MillPathSegment* p) { return s < p->firstStep; }
    );
    const MillPathSegment* p = *(it == mSegments.begin() ? it : it - 1);
    return p->startTime + p->duration * (float)(step - p->firstStep) / (float)p->numSimSteps;
}

void SimTimeline::HoldPlayback()
{
    mHoldUntil = clock::now() + BackSkipHold;
}

bool SimTimeline::IsHolding() const
{
    return clock::now() < mHoldUntil;
}

}  // namespace CAMSimulator
