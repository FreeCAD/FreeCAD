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

#include "MillMotion.h"
#include <string>
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
    {
        Clear();
    }
    virtual ~GCodeParser();

    void Clear();

    bool Parse(const char* filename);
    bool AddLine(const char* ptr);
    void SetFrame(const MillFrame& frame);
    void BeginOperation(const std::string& name);

public:
    std::vector<MillMotion> Operations;
    std::vector<MillFrame> Frames;
    std::vector<std::string> OpNames;
    MillMotion lastState;

protected:
    const char* GetNextToken(const char* ptr, GCToken* token);
    bool IsValidToken(char tok);
    const char* ParseFloat(const char* ptr, float* retFloat);
    bool ParseLine(const char* ptr);
    int lastTool = -1;
    // modal feed rates, kept apart for rapids (G0) and feed moves (G1)
    float rapidFeed = 0;
    float cutFeed = 0;
    bool mHasR = false;      // the drill cycle running has had an R word
    bool mHasDepth = false;  // and a Z, its depth, kept in mDepth
    float mDepth = 0.0f;
    bool mInCycle = false;  // a drill cycle is running; mInitialZ is the height from before it
    float mInitialZ = 0.0f;
};

}  // namespace CAMSimulator
