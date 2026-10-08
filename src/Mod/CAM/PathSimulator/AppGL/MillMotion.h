// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
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

#include "linmath.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace CAMSimulator
{

enum eEndMillType
{
    eEndmillFlat,
    eEndmillV,
    eEndmillBall,
    eEndmillFillet
};

enum eCmdType
{
    eNop,
    eMoveLiner,
    eRotateCW,
    eRotateCCW,
    eDril,
    eChangeTool
};

struct MillMotion
{
    eCmdType cmd = eNop;
    int tool = -1;
    float x = 0.0f, y = 0.0f, z = 0.0f;
    float i = 0.0f, j = 0.0f, k = 0.0f;
    float r = 0.0f;
    char retract_mode = '\0';
    int frame = 0;             // index into the parser's frame table; 0 is the world
    float feed = 0;            // feed rate in mm/s; 0 when not known
    bool rapid = false;        // G0, or the rapid part of a canned cycle
    int op = -1;               // index into the parser's operation names; -1 before the first
    float rot[3] = {0, 0, 0};  // A, B and C in degrees, when the program positions the rotaries
    bool hasRot = false;  // the operation's moves carry rotary words; x, y, z are machine coords
};

// A work plane frame: paths are in frame coordinates, tool along Z, and mat (column-major, as in
// linmath) places them in the world. rot is the frame's rotation, pose the rotation the rotary
// table gives the part here. Quaternions are ordered x, y, z, w.
struct MillFrame
{
    mat4x4 mat;
    quat rot;
    quat pose;
    float indexRate = 0;  // rotary speed when indexing into this frame, deg/s; 0 when not known
    std::vector<float> angles;  // rotary positions in degrees, in the order of the rotary axis list
};

// A span of program time, as fractions of the total time, during which one rotary axis turns,
// or, with axis -1, the table turns as a whole
struct SimTimeSpan
{
    float start = 0;
    float end = 0;
    int axis = -1;
    bool continuous = false;  // the axis turns while the tool cuts, not in an index

    bool operator==(const SimTimeSpan&) const = default;
};

// A rotary axis of the machine. Table axes rotate the part, head axes tilt the tool. The
// simulation keeps them in the order their rotations are applied, first applied first.
struct SimRotaryAxis
{
    std::string name;
    vec3 axis = {0, 0, 1};
    float rate = 0;     // degrees per second
    int sequence = 0;   // order the axes move in; axes with equal values move together
    bool head = false;  // tilts the tool rather than turning the part
};

static inline void MotionPosToVec(vec3 vec, const MillMotion& motion)
{
    vec[0] = motion.x;
    vec[1] = motion.y;
    vec[2] = motion.z;
}

// Transform a point from frame coordinates to world coordinates
static inline void FramePosToWorld(vec3 out, const mat4x4 frame, float x, float y, float z)
{
    vec4 local = {x, y, z, 1.f};
    vec4 world;
    mat4x4_mul_vec4(world, frame, local);
    vec3_set(out, world[0], world[1], world[2]);
}

// The frame's rotation without its translation: the normal matrix for anything rendered in it
static inline void FrameRotation(mat4x4 rot, const mat4x4 frame)
{
    mat4x4_dup(rot, frame);
    rot[3][0] = rot[3][1] = rot[3][2] = 0.f;
}

// Re-express a motion's position in another frame, keeping its world position
static inline void MotionToFrame(MillMotion& motion, const std::vector<MillFrame>& frames, int frame)
{
    if (motion.frame == frame) {
        return;
    }
    vec3 world;
    FramePosToWorld(world, frames[motion.frame].mat, motion.x, motion.y, motion.z);
    mat4x4 inv;
    mat4x4_invert(inv, frames[frame].mat);
    vec3 local;
    FramePosToWorld(local, inv, world[0], world[1], world[2]);
    motion.x = local[0];
    motion.y = local[1];
    motion.z = local[2];
    motion.frame = frame;
}

}  // namespace CAMSimulator
