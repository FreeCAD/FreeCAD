// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Shai Seger <shaise at gmail>
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

#ifdef _MSC_VER
# ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
# endif
#endif

#include "GCodeParser.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <string>

namespace CAMSimulator
{

static char TokTypes[] = "GTXYZIJKRFABC";

GCodeParser::~GCodeParser()
{
    Clear();
}

void GCodeParser::Clear()
{
    Operations.clear();
    OpNames.clear();
    Frames.assign(1, MillFrame {});
    mat4x4_identity(Frames[0].mat);
    quat_identity(Frames[0].rot);
    quat_identity(Frames[0].pose);
    lastState = {};
    lastTool = -1;
    mHasR = false;
    mHasDepth = false;
    mInCycle = false;
    rapidFeed = 0;
    cutFeed = 0;
}

namespace
{

// True when two frames have the same placement, pose and rotary positions, within rounding
bool SameFrame(const MillFrame& a, const MillFrame& b)
{
    constexpr float matrixTolerance = 1e-5f;  // for matrix and quaternion elements
    constexpr float angleTolerance = 1e-4f;   // degrees
    for (int c = 0; c < 4; c++) {
        for (int r = 0; r < 4; r++) {
            if (!(std::fabs(a.mat[c][r] - b.mat[c][r]) < matrixTolerance)) {
                return false;
            }
        }
    }
    for (int q = 0; q < 4; q++) {
        if (!(std::fabs(a.pose[q] - b.pose[q]) < matrixTolerance)) {
            return false;
        }
    }
    // the rotary positions must match too: C0 and C360 are the same pose, not the same position
    return std::ranges::equal(a.angles, b.angles, [](float x, float y) {
        return std::fabs(x - y) < angleTolerance;
    });
}

}  // namespace

void GCodeParser::SetFrame(const MillFrame& frame)
{
    // reuse an equal frame, so operations sharing a frame keep a single frame index
    const auto same = std::ranges::find_if(Frames, [&frame](const MillFrame& f) {
        return SameFrame(f, frame);
    });
    const int index = (int)(same - Frames.begin());
    if (same == Frames.end()) {
        Frames.push_back(frame);
    }

    // Re-express the last position in the new frame so the tool does not jump: axis words the
    // next line leaves out keep their world position.
    MotionToFrame(lastState, Frames, index);
}

void GCodeParser::BeginOperation(const std::string& name)
{
    // Motions from here on belong to this operation, with no rotary positions until it sets them.
    OpNames.push_back(name);
    lastState.op = (int)OpNames.size() - 1;
    lastState.hasRot = false;
    lastState.rot[0] = lastState.rot[1] = lastState.rot[2] = 0;
    // nor feed rates: a tool controller without them leaves them unknown, not the last ones
    rapidFeed = cutFeed = 0;
}

bool GCodeParser::Parse(const char* filename)
{
    Clear();

    FILE* fl;
    if ((fl = fopen(filename, "rt")) == nullptr) {
        return false;
    }

    char line[120];

    while (!feof(fl)) {
        if (fgets(line, 120, fl) != NULL) {
            AddLine(line);
        }
    }
    fclose(fl);
    return false;
}

const char* GCodeParser::GetNextToken(const char* ptr, GCToken* token)
{
    float tokval;
    token->letter = '*';
    while (*ptr != 0) {
        char letter = toupper(*ptr);
        ptr++;

        if (letter == ' ') {
            continue;
        }
        if (letter == '(') {
            break;
        }

        if (IsValidToken(letter)) {
            ptr = ParseFloat(ptr, &tokval);
            token->letter = letter;
            token->fval = tokval;
            token->ival = (int)(tokval + 0.5);
            break;
        }
    }
    return ptr;
}

bool GCodeParser::IsValidToken(char tok)
{
    int len = (int)strlen(TokTypes);
    for (int i = 0; i < len; i++) {
        if (tok == TokTypes[i]) {
            return true;
        }
    }
    return false;
}

const char* GCodeParser::ParseFloat(const char* ptr, float* retFloat)
{
    float decPos = 10;
    float sign = 1;
    bool decimalPointFound = false;
    float res = 0;
    while (*ptr != 0) {
        char letter = toupper(*ptr);
        ptr++;

        if (letter == ' ') {
            continue;
        }

        if (letter == '-') {
            sign = -1;
        }
        else if (letter == '.') {
            decimalPointFound = true;
        }
        else if (letter >= '0' && letter <= '9') {
            float digitVal = (float)(letter - '0');
            if (decimalPointFound) {
                res = res + digitVal / decPos;
                decPos *= 10;
            }
            else {
                res = res * 10 + digitVal;
            }
        }
        else {
            ptr--;
            break;
        }
    }
    *retFloat = res * sign;
    return ptr;
}

bool GCodeParser::ParseLine(const char* ptr)
{
    // Truncate at first semicolon (annotations / comment)
    const char* comment = strchr(ptr, ';');
    std::string line;
    if (comment) {
        line = std::string(ptr, comment - ptr);
        ptr = line.c_str();
    }

    GCToken token;
    bool validMotion = false;
    bool exitLoop = false;
    bool hasR = false;
    bool hasZ = false;
    bool cancel = false;
    int cmd = 0;
    float feed = -1;

    // By default GCode words are not sticky, except for some exceptions. We copy the needed
    // parameters explicitly (instead of assigning the full lastState).

    MillMotion newState;

    newState.x = lastState.x;
    newState.y = lastState.y;
    newState.z = lastState.z;

    newState.tool = lastState.tool;
    newState.frame = lastState.frame;
    newState.op = lastState.op;
    newState.hasRot = lastState.hasRot;
    for (int k = 0; k < 3; k++) {
        newState.rot[k] = lastState.rot[k];
    }

    newState.retract_mode = lastState.retract_mode;
    // a drill cycle's R plane holds for the cycle's next holes
    newState.r = lastState.r;

    while (*ptr != 0 && !exitLoop) {
        ptr = GetNextToken(ptr, &token);
        switch (token.letter) {
            case '*':
                exitLoop = true;
                break;

            case 'G':
                cmd = token.ival;
                if (cmd == 0 || cmd == 1) {
                    newState.cmd = eMoveLiner;
                    newState.rapid = cmd == 0;
                }
                else if (cmd == 2) {
                    newState.cmd = eRotateCW;
                }
                else if (cmd == 3) {
                    newState.cmd = eRotateCCW;
                }
                else if (
                    cmd == 73 || cmd == 74 || cmd == 81 || cmd == 82 || cmd == 83 || cmd == 84
                    || cmd == 85 || cmd == 88 || cmd == 89
                ) {
                    newState.cmd = eDril;
                }
                else if (cmd == 98 || cmd == 99) {
                    newState.retract_mode = cmd;
                }
                // G0 to G3 and G80 end a cycle; G98 and G99 are modal on their own and stay
                cancel = cancel || cmd <= 3 || cmd == 80;
                break;

            case 'T':
                newState.tool = token.ival;
                break;

            case 'X':
                newState.x = token.fval;
                validMotion = true;
                break;

            case 'Y':
                newState.y = token.fval;
                validMotion = true;
                break;

            case 'Z':
                newState.z = token.fval;
                hasZ = true;
                validMotion = true;
                break;

            case 'I':
                newState.i = token.fval;
                break;

            case 'J':
                newState.j = token.fval;
                break;

            case 'K':
                newState.k = token.fval;
                break;

            case 'R':
                newState.r = token.fval;
                hasR = true;
                break;

            case 'F':
                feed = token.fval;
                break;

            case 'A':
            case 'B':
            case 'C':
                newState.rot[token.letter - 'A'] = token.fval;
                newState.hasRot = true;
                validMotion = true;
                break;
        }
    }

    // A drill cycle's R and depth hold for its next holes, and G98 goes back to the height from
    // before its first, until a G80 or a G0 to G3 ends it
    if (newState.cmd == eDril) {
        if (!mInCycle) {
            mInitialZ = lastState.z;
            mInCycle = true;
        }
        mHasR = mHasR || hasR;
        if (hasZ) {
            mDepth = newState.z;
            mHasDepth = true;
        }
        else if (mHasDepth && validMotion) {
            newState.z = mDepth;
        }
    }
    else if (cancel) {
        mHasR = false;
        mHasDepth = false;
        mInCycle = false;
    }

    // An F word sets the modal rate for its kind of move (rapid or feed). F0 is ignored, as in the
    // cycle time estimate: operations write it where the tool controller sets the rate.
    if (feed > 0) {
        (newState.rapid ? rapidFeed : cutFeed) = feed;
    }
    newState.feed = newState.rapid ? rapidFeed : cutFeed;

    lastState = newState;
    return validMotion;
}

bool GCodeParser::AddLine(const char* ptr)
{
    const float fromX = lastState.x;
    const float fromY = lastState.y;
    const float fromZ = lastState.z;
    bool res = ParseLine(ptr);
    if (res) {
        if (lastState.cmd == eDril) {
            // Split into moves as the control runs the cycle (LinuxCNC's preliminary motion): to R
            // if between it and the depth, across to the hole, to R, a feed to the depth, and out
            // to R under G99, else to the height from before the cycle or R, whichever is farther
            // from the depth. Away from the depth is not always up: R and Z move with the
            // operation's frame, so on the underside the depth is above R. Without an R word the
            // cycle runs from where it started.
            lastState.cmd = eMoveLiner;
            const float rPlane = mHasR ? lastState.r : fromZ;
            const float depth = lastState.z;
            const float holeX = lastState.x;
            const float holeY = lastState.y;
            const float away = rPlane < depth ? -1.0f : 1.0f;
            const float beyondR = (fromZ - rPlane) * away;
            const bool initialBeyondR = (mInitialZ - rPlane) * away > 0;
            const float out = lastState.retract_mode != 99 && initialBeyondR ? mInitialZ : rPlane;
            auto push = [this](float x, float y, float z, bool rapid) {
                lastState.x = x;
                lastState.y = y;
                lastState.z = z;
                lastState.rapid = rapid;
                lastState.feed = rapid ? rapidFeed : cutFeed;
                Operations.push_back(lastState);
            };
            float z = fromZ;
            if (beyondR < 0) {
                z = rPlane;
                push(fromX, fromY, z, true);
            }
            if (holeX != fromX || holeY != fromY) {
                push(holeX, holeY, z, true);
            }
            if (z != rPlane) {
                push(holeX, holeY, rPlane, true);
            }
            push(holeX, holeY, depth, false);
            push(holeX, holeY, out, true);
            lastState.cmd = eDril;
        }
        else {
            Operations.push_back(lastState);
        }
    }
    return res;
}

}  // namespace CAMSimulator
