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
#include "linmath.h"
#include <vector>

namespace CAMSimulator
{

class EndMill;
struct Point3D;

/**
 * The machine's rotary axes and what they do to the program: the pose the table gives the part,
 * the tilt the head gives the tool, how an index turns the axes over time, and the path segments
 * for moves between work plane frames and for moves that turn the axes while they cut.
 */
class SimRotary
{
public:
    explicit SimRotary(const std::vector<MillFrame>& frames);

    void SetAxes(const std::vector<SimRotaryAxis>& axes)
    {
        mAxes = axes;
    }
    const std::vector<SimRotaryAxis>& Axes() const
    {
        return mAxes;
    }
    // whether the part turns with the table; when false the part stays still and the tool tilts
    void SetTablePose(bool b)
    {
        mTablePose = b;
    }

    // The segment for a move between motions; a move into a new work plane starts where the last
    // ended, and one that changes the tool axis is an index, which does not cut.
    MillPathSegment* MakeSegment(const EndMill& tool, const MillMotion& from, const MillMotion& to) const;
    // the segments for a move that turns the rotary axes while it cuts
    std::vector<MillPathSegment*> MakeContinuousSegments(
        const EndMill& tool,
        const MillMotion& from,
        const MillMotion& to
    ) const;
    // whether a move turns the rotary axes while it cuts
    bool TurnsWhileCutting(const MillMotion& to) const;
    // the position of the first motion, in part coordinates
    void StartPosition(vec3 out, const MillMotion& first) const;
    // Marks rapids that turn the part on one table axis as not cutting when they stay farther
    // from the axis than the stock reaches.
    void MarkClearRapids(
        const std::vector<MillPathSegment*>& segments,
        const std::vector<Point3D>& stockPoints
    ) const;

    // whether the segment is an index with known rotary positions at both ends
    bool HasAxisAngles(const MillPathSegment* p) const;
    // each axis's start and duration in an index, in seconds from its start; returns the total
    float IndexSchedule(const MillPathSegment* p, std::vector<float>& begin, std::vector<float>& span) const;
    // the duration of an index; angles, if given, gets the axis positions s seconds in
    float IndexAngles(const MillPathSegment* p, float s, std::vector<float>* angles) const;
    // the axis positions during an index that turns the axes one by one, at the given step
    bool IndexAnglesAtStep(const MillPathSegment* p, int step, std::vector<float>& angles) const;
    // the fraction of the segment done at a step, clamped to [0, 1]
    static float StepFraction(const MillPathSegment* p, int step);

    // the rotation of the part by the table axes
    void PoseFromAngles(quat pose, const std::vector<float>& angles) const;
    // the combined rotation of the table axes (head false) or head axes (head true)
    void AxesRotation(quat rot, const std::vector<float>& angles, bool head) const;
    // the pose of the part in a frame: the frame's pose, or none with the table pose view off
    void FramePose(quat pose, const MillFrame& frame) const;
    // the pose of the part at a step of a segment
    void ScenePose(quat pose, const MillPathSegment* p, int step) const;
    // the tool tip at a step, in part coordinates, and its rotation, with the part in scenePose
    void ToolPose(MillPathSegment* p, int step, vec3 toolPos, mat4x4 rmat, const quat scenePose) const;

private:
    std::vector<float> MotionAngles(const MillMotion& m) const;
    void PartPos(vec3 out, const MillMotion& m, const std::vector<float>& angles) const;

    const std::vector<MillFrame>& mFrames;
    std::vector<SimRotaryAxis> mAxes;
    bool mTablePose = true;
};

}  // namespace CAMSimulator
