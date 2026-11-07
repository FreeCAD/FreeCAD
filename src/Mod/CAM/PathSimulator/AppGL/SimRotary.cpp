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

#include "SimRotary.h"

#include <Base/Rotation.h>

#include "EndMill.h"
#include "SimDisplay.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <numbers>

namespace CAMSimulator
{

namespace
{

// Tolerance when comparing tool axis directions (unit vector components).
constexpr float AxisTolerance = 1e-4f;
// Frames closer than this share their tool axis (their Z): a straight move keeps its orientation.
constexpr float FrameAxisTolerance = 1e-5f;
// An arc of a continuous rotary move is drawn in straight pieces no farther than this, in mm,
// from the arc, each between these many radians long.
constexpr float ArcChordTolerance = 0.01f;
constexpr float MaxArcPiece = 0.2f;
constexpr float MinArcPiece = 0.01f;
// An index gets one simulation step per this many degrees of table turn, so it can be seen.
constexpr float IndexDegreesPerStep = 1.5f;

// The largest turn, in degrees, of one piece of a continuous rotary move, drawn straight in a
// frame turned by its middle angle: off the true path by r * (1 - cos(a / 2)), under 0.004 mm at
// 25 mm. Rapids use coarser pieces, as they are drawn only where they reach the stock.
constexpr float MaxContinuousTurn = 2.f;
constexpr float MaxContinuousRapidTurn = 20.f;

// The rotary positions a fraction t of the way from the segment's start angles to its end angles.
std::vector<float> InterpolateAngles(const MillPathSegment* p, float t)
{
    std::vector<float> angles(p->angFrom.size());
    for (size_t k = 0; k < angles.size(); k++) {
        angles[k] = p->angFrom[k] + (p->angTo[k] - p->angFrom[k]) * t;
    }
    return angles;
}

// Whether two directions match within tolerance in each component
bool SameDirection(const float* a, const float* b, float tolerance)
{
    return std::fabs(a[0] - b[0]) < tolerance && std::fabs(a[1] - b[1]) < tolerance
        && std::fabs(a[2] - b[2]) < tolerance;
}

// From a (t = 0) to b (t = 1) along the shorter arc; both keep quaternions as x, y, z, w
void QuatSlerp(quat r, const quat a, const quat b, float t)
{
    const Base::Rotation from(a[0], a[1], a[2], a[3]);
    const Base::Rotation to(b[0], b[1], b[2], b[3]);
    double q[4];
    Base::Rotation::slerp(from, to, t).getValue(q[0], q[1], q[2], q[3]);
    std::copy(q, q + 4, r);
}

}  // namespace

SimRotary::SimRotary(const std::vector<MillFrame>& frames)
    : mFrames(frames)
{}

float SimRotary::StepFraction(const MillPathSegment* p, int step)
{
    return std::clamp((float)step / (float)p->numSimSteps, 0.f, 1.f);
}

std::vector<float> SimRotary::MotionAngles(const MillMotion& m) const
{
    // the rotary positions at a motion, in SetAxes order: its A/B/C words, else its frame's
    // angles, none when the machine cannot reach the frame
    if (!m.hasRot) {
        return mFrames[m.frame].angles;
    }
    const size_t n = mAxes.size();
    std::vector<float> angles(n, 0.f);
    for (size_t k = 0; k < n; k++) {
        const std::string& name = mAxes[k].name;
        const char letter = name.empty() ? '\0' : (char)std::toupper(name[0]);
        if (letter >= 'A' && letter <= 'C') {
            angles[k] = m.rot[letter - 'A'];
        }
    }
    return angles;
}

void SimRotary::PartPos(vec3 out, const MillMotion& m, const std::vector<float>& angles) const
{
    // A motion's position in part coordinates, converted from its frame. With rotary words it is
    // in machine coordinates, so the table rotation is undone.
    FramePosToWorld(out, mFrames[m.frame].mat, m.x, m.y, m.z);
    if (m.hasRot) {
        quat pose, unposed;
        PoseFromAngles(pose, angles);
        quat_conj(unposed, pose);
        vec3 machine;
        vec3_dup(machine, out);
        quat_mul_vec3(out, unposed, machine);
    }
}

void SimRotary::StartPosition(vec3 out, const MillMotion& first) const
{
    PartPos(out, first, MotionAngles(first));
}

bool SimRotary::TurnsWhileCutting(const MillMotion& to) const
{
    return to.hasRot && !mAxes.empty();
}

MillPathSegment* SimRotary::MakeSegment(
    const EndMill& tool,
    const MillMotion& from,
    const MillMotion& to
) const
{
    // A move into a new work plane, or after a continuous rotary move, starts where the last one
    // ended and is drawn in the new frame. An index cannot be shown as a sweep, so it does not cut.
    const std::vector<float> angFrom = MotionAngles(from);
    const std::vector<float> angTo = MotionAngles(to);
    const bool leavingRotary = from.hasRot && !mAxes.empty();
    MillMotion fromMotion = from;
    bool isCutting = true;
    if (leavingRotary) {
        // where the rotaries left the tool, in the world (frame 0), then in the new frame
        vec3 world;
        PartPos(world, from, angFrom);
        fromMotion.x = world[0];
        fromMotion.y = world[1];
        fromMotion.z = world[2];
        fromMotion.frame = 0;
        fromMotion.hasRot = false;
        MotionToFrame(fromMotion, mFrames, to.frame);
        // it cuts only if the tool axis, where the rotaries left it, is the new frame's Z axis
        quat pose, unposed;
        PoseFromAngles(pose, angFrom);
        quat_conj(unposed, pose);
        vec3 z = {0, 0, 1}, axis;
        quat_mul_vec3(axis, unposed, z);
        isCutting = SameDirection(axis, mFrames[to.frame].mat[2], AxisTolerance);
    }
    else if (fromMotion.frame != to.frame) {
        const float* fromZ = mFrames[fromMotion.frame].mat[2];
        isCutting = SameDirection(fromZ, mFrames[to.frame].mat[2], FrameAxisTolerance);
        MotionToFrame(fromMotion, mFrames, to.frame);
    }
    auto segment = new MillPathSegment(tool, fromMotion, to, mFrames[to.frame].mat);
    segment->isCutting = isCutting;
    segment->frameFrom = from.frame;
    segment->frameTo = to.frame;
    segment->index = from.frame != to.frame || leavingRotary;
    segment->angFrom = angFrom;
    segment->angTo = angTo;
    // How far the table turns in an index: the angle between the poses, and the axes' summed
    // travel when known. The segment gets enough steps for the turn to be seen.
    float turn = 0;
    float axisTurn = 0;
    if (segment->index) {
        quat a, b;
        if (HasAxisAngles(segment)) {
            PoseFromAngles(a, angFrom);
            PoseFromAngles(b, angTo);
            for (size_t k = 0; k < angFrom.size(); k++) {
                axisTurn += std::fabs(angTo[k] - angFrom[k]);
            }
        }
        else {
            vec4_dup(a, mFrames[from.frame].pose);
            vec4_dup(b, mFrames[to.frame].pose);
        }
        turn = 2.f * std::acos(std::fmin(1.f, std::fabs(quat_mul_inner(a, b)))) * 180.f
            / std::numbers::pi_v<float>;
    }
    segment->SetMinSimSteps((int)(std::max(turn, axisTurn) / IndexDegreesPerStep));
    segment->op = to.op;
    segment->turn = turn;
    segment->axisTurn = axisTurn;
    segment->feed = to.feed;
    segment->isRapid = to.rapid;
    return segment;
}

std::vector<MillPathSegment*> SimRotary::MakeContinuousSegments(
    const EndMill& tool,
    const MillMotion& from,
    const MillMotion& to
) const
{
    // The tool moves straight while the part turns under it, so the move is split into pieces,
    // each drawn in a frame turned by its middle angle, where the path is close to straight.
    std::vector<float> angFrom = MotionAngles(from);
    const std::vector<float> angTo = MotionAngles(to);
    angFrom.resize(angTo.size(), 0.f);  // from a frame the machine cannot reach: from zero
    const size_t n = angFrom.size();
    vec3 partFrom, machineStart, machineEnd;
    PartPos(partFrom, from, angFrom);
    quat startPose;
    PoseFromAngles(startPose, angFrom);
    quat_mul_vec3(machineStart, startPose, partFrom);
    FramePosToWorld(machineEnd, mFrames[to.frame].mat, to.x, to.y, to.z);

    float maxTurn = 0;
    for (size_t k = 0; k < n; k++) {
        maxTurn = std::max(maxTurn, std::fabs(angTo[k] - angFrom[k]));
    }
    const float maxPiece = to.rapid ? MaxContinuousRapidTurn : MaxContinuousTurn;
    int pieces = std::max(1, (int)std::ceil(maxTurn / maxPiece));

    // A G2 or G3 is an arc on the machine, in its frame's XY plane, rising as a helix: its pieces
    // follow the arc, not the chord between its ends.
    const MillFrame& toFrame = mFrames[to.frame];
    const bool arc = (to.cmd == eRotateCW || to.cmd == eRotateCCW)
        && (std::fabs(to.i) > 1e-6f || std::fabs(to.j) > 1e-6f);
    vec3 arcCenter = {0, 0, 0};
    float arcRadius = 0, arcFrom = 0, arcSweep = 0, arcRise = 0;
    if (arc) {
        mat4x4 unframe;
        mat4x4_invert(unframe, toFrame.mat);
        const vec4 world = {machineStart[0], machineStart[1], machineStart[2], 1.f};
        vec4 start;
        mat4x4_mul_vec4(start, unframe, world);
        arcCenter[0] = start[0] + to.i;
        arcCenter[1] = start[1] + to.j;
        arcCenter[2] = start[2];
        arcRadius = std::hypot(to.i, to.j);
        arcFrom = std::atan2(-to.j, -to.i);
        arcSweep = std::atan2(to.y - arcCenter[1], to.x - arcCenter[0]) - arcFrom;
        // G3 turns counterclockwise, G2 clockwise; ends that meet make a full circle
        if (to.cmd == eRotateCCW && arcSweep <= 1e-6f) {
            arcSweep += 2.f * std::numbers::pi_v<float>;
        }
        else if (to.cmd == eRotateCW && arcSweep >= -1e-6f) {
            arcSweep -= 2.f * std::numbers::pi_v<float>;
        }
        arcRise = to.z - start[2];
        const float ratio = std::min(ArcChordTolerance / std::max(arcRadius, 1e-6f), 1.f);
        const float step = std::clamp(2.f * std::acos(1.f - ratio), MinArcPiece, MaxArcPiece);
        pieces = std::max(pieces, (int)std::ceil(std::fabs(arcSweep) / step));
    }
    // where the tool is on the machine a fraction t of the way along the move
    auto machineAt = [&](float t, float* out) {
        if (t <= 0.f) {
            vec3_dup(out, machineStart);
        }
        else if (t >= 1.f) {
            vec3_dup(out, machineEnd);
        }
        else if (arc) {
            const float a = arcFrom + arcSweep * t;
            FramePosToWorld(
                out,
                toFrame.mat,
                arcCenter[0] + arcRadius * std::cos(a),
                arcCenter[1] + arcRadius * std::sin(a),
                arcCenter[2] + arcRise * t
            );
        }
        else {
            for (int c = 0; c < 3; c++) {
                out[c] = machineStart[c] + (machineEnd[c] - machineStart[c]) * t;
            }
        }
    };

    std::vector<MillPathSegment*> segments;
    for (int s = 0; s < pieces; s++) {
        const float t0 = (float)s / (float)pieces;
        const float t1 = (float)(s + 1) / (float)pieces;
        // the angles at the piece's start, end and middle
        std::vector<float> angStart(n), angEnd(n), angMid(n);
        for (size_t k = 0; k < n; k++) {
            angStart[k] = angFrom[k] + (angTo[k] - angFrom[k]) * t0;
            angEnd[k] = angFrom[k] + (angTo[k] - angFrom[k]) * t1;
            angMid[k] = 0.5f * (angStart[k] + angEnd[k]);
        }
        // the piece's start and end on the machine
        vec3 pieceStart, pieceEnd;
        machineAt(t0, pieceStart);
        machineAt(t1, pieceEnd);
        // A machine point m at angles a is at Tmid * Ta^-1 * m in the piece's frame, where T is
        // the table pose at the given angles.
        quat poseMid, poseStart, poseEnd, unposeStart, unposeEnd, startToPiece, endToPiece;
        PoseFromAngles(poseMid, angMid);
        PoseFromAngles(poseStart, angStart);
        PoseFromAngles(poseEnd, angEnd);
        quat_conj(unposeStart, poseStart);
        quat_conj(unposeEnd, poseEnd);
        quat_mul(startToPiece, poseMid, unposeStart);
        quat_mul(endToPiece, poseMid, unposeEnd);
        vec3 localStart, localEnd;
        quat_mul_vec3(localStart, startToPiece, pieceStart);
        quat_mul_vec3(localEnd, endToPiece, pieceEnd);

        MillMotion fromPiece = to;
        MillMotion toPiece = to;
        fromPiece.x = localStart[0];
        fromPiece.y = localStart[1];
        fromPiece.z = localStart[2];
        toPiece.x = localEnd[0];
        toPiece.y = localEnd[1];
        toPiece.z = localEnd[2];
        fromPiece.cmd = toPiece.cmd = eMoveLiner;

        // the piece's frame maps its coordinates onto the part: Tmid^-1
        quat unposed;
        quat_conj(unposed, poseMid);
        mat4x4 frame;
        mat4x4_from_quat(frame, unposed);

        auto segment = new MillPathSegment(tool, fromPiece, toPiece, frame);
        segment->continuous = true;
        segment->isCutting = true;
        segment->frameFrom = segment->frameTo = to.frame;
        segment->angFrom = angStart;
        segment->angTo = angEnd;
        vec3 travel;
        vec3_sub(travel, pieceEnd, pieceStart);
        segment->machineLength = vec3_len(travel);
        vec3_dup(segment->machineFrom, pieceStart);
        vec3_dup(segment->machineTo, pieceEnd);
        segment->rotaryTravel = maxTurn / (float)pieces;
        segment->op = to.op;
        segment->feed = to.feed;
        segment->isRapid = to.rapid;
        segments.push_back(segment);
    }
    return segments;
}

void SimRotary::MarkClearRapids(
    const std::vector<MillPathSegment*>& segments,
    const std::vector<Point3D>& stockPoints
) const
{
    // A rapid turning the part on one table axis cuts nothing if the tool stays farther from the
    // axis than the stock reaches; it is marked not cutting, so the CSG render skips it.
    if (mAxes.size() != 1 || mAxes[0].head || stockPoints.empty()) {
        return;
    }
    vec3 u;
    vec3_norm(u, mAxes[0].axis);
    // the component of v perpendicular to the axis
    auto offAxis = [&u](vec3 out, const float* v) {
        const float along = vec3_mul_inner(v, u);
        for (int c = 0; c < 3; c++) {
            out[c] = v[c] - along * u[c];
        }
    };
    // how far the stock reaches from the axis: the distance of its farthest vertex
    float reach = 0;
    for (const Point3D& pt : stockPoints) {
        const vec3 v = {pt.x, pt.y, pt.z};
        vec3 d;
        offAxis(d, v);
        reach = std::max(reach, vec3_len(d));
    }
    for (MillPathSegment* p : segments) {
        if (!p->continuous || !p->isRapid) {
            continue;
        }
        // the closest point to the axis on the tool's straight path on the machine
        vec3 d0, d1, dd;
        offAxis(d0, p->machineFrom);
        offAxis(d1, p->machineTo);
        vec3_sub(dd, d1, d0);
        const float len2 = vec3_mul_inner(dd, dd);
        const float t = len2 > 1e-12f ? std::clamp(-vec3_mul_inner(d0, dd) / len2, 0.f, 1.f) : 0.f;
        vec3 closest;
        vec3_scale(closest, dd, t);
        vec3_add(closest, closest, d0);
        // the tool's own radius reaches past its centerline toward the axis
        const float toolRadius = p->endmill != nullptr ? p->endmill->radius : 0.f;
        p->isCutting = vec3_len(closest) - toolRadius <= reach;
    }
}

bool SimRotary::HasAxisAngles(const MillPathSegment* p) const
{
    return p->index && !mAxes.empty() && p->angFrom.size() == mAxes.size()
        && p->angTo.size() == mAxes.size();
}

float SimRotary::IndexSchedule(
    const MillPathSegment* p,
    std::vector<float>& begin,
    std::vector<float>& span
) const
{
    // Axes with the same sequence number move together at the slowest one's pace; the groups run
    // lowest first.
    const std::vector<float>& a = p->angFrom;
    const std::vector<float>& b = p->angTo;
    const size_t n = mAxes.size();
    std::vector<int> sequences;
    for (const SimRotaryAxis& axis : mAxes) {
        sequences.push_back(axis.sequence);
    }
    std::vector<int> order = sequences;
    std::sort(order.begin(), order.end());
    order.erase(std::unique(order.begin(), order.end()), order.end());

    begin.assign(n, 0.f);
    span.assign(n, 0.f);
    float start = 0;
    for (int seq : order) {
        float group = 0;
        for (size_t k = 0; k < n; k++) {
            if (sequences[k] == seq && mAxes[k].rate > 0) {
                group = std::max(group, std::fabs(b[k] - a[k]) / mAxes[k].rate);
            }
        }
        for (size_t k = 0; k < n; k++) {
            if (sequences[k] == seq) {
                begin[k] = start;
                span[k] = group;
            }
        }
        start += group;
    }
    return start;
}

float SimRotary::IndexAngles(const MillPathSegment* p, float s, std::vector<float>* angles) const
{
    std::vector<float> begin, span;
    const float total = IndexSchedule(p, begin, span);
    if (angles) {
        const std::vector<float>& a = p->angFrom;
        const std::vector<float>& b = p->angTo;
        *angles = a;
        for (size_t k = 0; k < a.size(); k++) {
            const float frac = span[k] > 0 ? std::clamp((s - begin[k]) / span[k], 0.f, 1.f)
                                           : (s >= begin[k] ? 1.f : 0.f);
            (*angles)[k] = a[k] + (b[k] - a[k]) * frac;
        }
    }
    return total;
}

bool SimRotary::IndexAnglesAtStep(const MillPathSegment* p, int step, std::vector<float>& angles) const
{
    if (!HasAxisAngles(p) || p->rotaryTime <= 0) {
        return false;
    }
    const float t = StepFraction(p, step);
    IndexAngles(p, t * p->duration, &angles);
    return true;
}

void SimRotary::AxesRotation(quat rot, const std::vector<float>& angles, bool head) const
{
    // The first axis is applied first, as the post processor composes them.
    quat_identity(rot);
    for (size_t k = 0; k < mAxes.size() && k < angles.size(); k++) {
        if (mAxes[k].head != head) {
            continue;
        }
        quat r, total;
        quat_rotate(r, angles[k] * std::numbers::pi_v<float> / 180.f, mAxes[k].axis);
        quat_mul(total, r, rot);
        vec4_dup(rot, total);
    }
}

void SimRotary::PoseFromAngles(quat pose, const std::vector<float>& angles) const
{
    AxesRotation(pose, angles, false);
}

void SimRotary::FramePose(quat pose, const MillFrame& frame) const
{
    if (mTablePose) {
        vec4_dup(pose, frame.pose);
    }
    else {
        quat_identity(pose);
    }
}

void SimRotary::ScenePose(quat pose, const MillPathSegment* p, int step) const
{
    // the table turns during a move between frames, and holds its pose otherwise
    quat_identity(pose);
    const float t = StepFraction(p, step);
    if (p->continuous) {
        // the part turns with the rotary axes while the tool cuts
        if (mTablePose) {
            PoseFromAngles(pose, InterpolateAngles(p, t));
        }
        return;
    }
    if (mTablePose && HasAxisAngles(p)) {
        // axis by axis, as the machine turns
        std::vector<float> angles;
        IndexAngles(p, t * p->duration, &angles);
        PoseFromAngles(pose, angles);
        return;
    }
    if (p->frameFrom == p->frameTo && !p->index) {
        // within a frame the table keeps the frame's pose exactly
        FramePose(pose, mFrames[p->frameTo]);
        return;
    }
    quat from, to;
    FramePose(from, mFrames[p->frameFrom]);
    FramePose(to, mFrames[p->frameTo]);
    QuatSlerp(pose, from, to, t);
}

void SimRotary::ToolPose(MillPathSegment* p, int step, vec3 toolPos, mat4x4 rmat, const quat scenePose) const
{
    p->SetStepNumber(step);
    p->GetHeadPosition(toolPos);
    p->GetToolRotation(rmat);
    if (p->continuous) {
        // on the true path, not the piece's chord: straight on the machine, turned onto the part
        const float t = StepFraction(p, step);
        const std::vector<float> angles = InterpolateAngles(p, t);
        vec3 machine;
        for (int c = 0; c < 3; c++) {
            machine[c] = p->machineFrom[c] + (p->machineTo[c] - p->machineFrom[c]) * t;
        }
        quat pose, unposed;
        PoseFromAngles(pose, angles);
        quat_conj(unposed, pose);
        quat_mul_vec3(toolPos, unposed, machine);
        mat4x4_from_quat(rmat, unposed);
    }
    else if (HasAxisAngles(p)) {
        // axis by axis: on the part, the tool rotation is the head tilt with the table turn undone
        std::vector<float> angles;
        const float t = StepFraction(p, step);
        IndexAngles(p, t * p->duration, &angles);
        quat table, head, onPart;
        PoseFromAngles(table, angles);
        AxesRotation(head, angles, true);
        quat_conj(table, table);
        quat_mul(onPart, table, head);
        mat4x4_from_quat(rmat, onPart);
    }
    else if (p->frameFrom != p->frameTo && !p->isCutting) {
        // While the table turns, the tool turns from the old frame's orientation to the new one's
        // on the machine; the table rotation in the scene pose is undone.
        const MillFrame& from = mFrames[p->frameFrom];
        const MillFrame& to = mFrames[p->frameTo];
        quat poseFrom, poseTo, seenFrom, seenTo, seen, unscene, unposed;
        FramePose(poseFrom, from);
        FramePose(poseTo, to);
        quat_mul(seenFrom, poseFrom, from.rot);
        quat_mul(seenTo, poseTo, to.rot);
        const float t = StepFraction(p, step);
        QuatSlerp(seen, seenFrom, seenTo, t);
        quat_conj(unscene, scenePose);
        quat_mul(unposed, unscene, seen);
        mat4x4_from_quat(rmat, unposed);
    }
}

}  // namespace CAMSimulator
