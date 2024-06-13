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

#ifndef __csgcodeparser_h__
#define __csgcodeparser_h__
#include "MillMotion.h"
#include <vector>

namespace MillSim
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
    bool Parse(const char* filename);
    bool AddLine(const char* ptr);

public:
    std::vector<MillMotion> Operations;
    MillMotion lastState = {eNop, 0, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    MillMotion lastLastState = {eNop, 0, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};

protected:
    const char* GetNextToken(const char* ptr, GCToken* token);
    bool IsValidToken(char tok);
    const char* ParseFloat(const char* ptr, float* retFloat);
    bool ParseLine(const char* ptr);
    int lastTool = -1;
};
}  // namespace MillSim
#endif
