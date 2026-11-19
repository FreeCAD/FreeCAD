// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Shai Seger
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
#include <vector>

namespace CAMSimulator
{

struct GCToken
{
    char letter;
    float fval;
    int ival;
};

class GCodeParser
{
public:
    GCodeParser()
    {}
    virtual ~GCodeParser();

    void Clear();

    bool Parse(const char* filename);
    bool AddLine(const char* ptr);

public:
    std::vector<MillMotion> Operations;
    MillMotion lastState;

protected:
    const char* GetNextToken(const char* ptr, GCToken* token);
    bool IsValidToken(char tok);
    const char* ParseFloat(const char* ptr, float* retFloat);
    bool ParseLine(const char* ptr);
    int lastTool = -1;
    bool mHasR = false;  // a drill cycle's R word was read since the last G80
};

}  // namespace CAMSimulator
