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
#include "GCodeParser.h"
#include "Shader.h"
#include "linmath.h"
#include "GlUtils.h"
#include "StockObject.h"
#include "MillPathSegment.h"
#include "SimDisplay.h"
#include "GuiDisplay.h"
#include "MillPathLine.h"
#include "SolidObject.h"
#include <sstream>
#include <vector>

#define VIEWITEM_SIMULATION 1
#define VIEWITEM_BASE_SHAPE 2
#define VIEWITEM_MAX 4

namespace MillSim
{

struct MillSimulationState
{
    int mCurStep = 0;
    int mNTotalSteps = 0;
    int mPathStep = -1;
    int mSubStep = 0;
    int mNPathSteps = 0;
    int mSimSpeed = 1;
    int mViewItems = VIEWITEM_SIMULATION;
    bool mViewPath = false;
    bool mViewSSAO = false;

    bool mSimPlaying = false;
    bool mSingleStep = false;
};

class MillSimulation: private MillSimulationState
{
public:
    MillSimulation();
    ~MillSimulation();
    void ClearMillPathSegments();
    void Clear();
    void SimNext();
    void InitSimulation(float quality);
    void AddTool(EndMill* tool);
    void AddTool(const std::vector<float>& toolProfile, int toolid, float diameter);
    bool ToolExists(int toolid);
    void RenderSimulation();
    void RenderTool();
    void RenderPath();
    void RenderBaseShape();
    void Render();
    void ProcessSim(unsigned int time_ms);
    void HandleKeyPress(int key);
    void HandleGuiAction(eGuiItems actionItem, bool checked);
    bool LoadGCodeFile(const char* fileName);
    bool AddGcodeLine(const char* line);
    void SetSimulationStage(float stage);
    void SetState(const MillSimulationState& state);
    const MillSimulationState& GetState() const;
    void SetBoxStock(float x, float y, float z, float l, float w, float h);
    void SetArbitraryStock(const std::vector<Vertex>& verts, const std::vector<GLushort>& indices);
    void SetBaseObject(const std::vector<Vertex>& verts, const std::vector<GLushort>& indices);
    void MouseDrag(int buttons, int dx, int dy);
    void MouseMove(int px, int py, int modifiers);
    void MouseScroll(float dy);
    void MouseHover(int px, int py);
    void MousePress(int button, bool isPressed, int px, int py);
    void Zoom(float factor);
    void UpdateWindowScale(int width, int height);

protected:
    void InitDisplay(float quality);
    void GlsimStart();
    void GlsimToolStep1(void);
    void GlsimToolStep2(void);
    void GlsimClipBack(void);
    void GlsimRenderStock(void);
    void GlsimRenderTools(void);
    void GlsimEnd(void);
    void renderSegmentForward(int iSeg);
    void renderSegmentReversed(int iSeg);
    void CalcSegmentPositions();
    EndMill* GetTool(int tool);
    void RemoveTool(int toolId);

protected:
    std::vector<EndMill*> mToolTable;
    GCodeParser mCodeParser;
    GuiDisplay guiDisplay;
    SimDisplay simDisplay;
    MillPathLine millPathLine;
    std::vector<MillPathSegment*> MillPathSegments;
    std::ostringstream mFpsStream;

    // clang-format off
    MillMotion mZeroPos = {.cmd=eNop, .tool=-1, .x=0, .y=0, .z=100, .i=0, .j=0, .k=0, .r=0, .retract_mode='\0', .retract_z=0.0};
    MillMotion mCurMotion = {.cmd=eNop, .tool=-1, .x=0, .y=0, .z=0, .i=0, .j=0, .k=0, .r=0, .retract_mode='\0', .retract_z=0.0};
    MillMotion mDestMotion = {.cmd=eNop, .tool=-1, .x=0, .y=0, .z=0, .i=0, .j=0, .k=0, .r=0, .retract_mode='\0', .retract_z=0.0};
    // clang-format on

    int mWidth = -1;
    int mHeight = -1;

    StockObject mStockObject;
    SolidObject mBaseShape;

    vec3 bgndColor = {0.1f, 0.2f, 0.3f};
    vec3 stockColor = {0.5f, 0.55f, 0.9f};
    vec3 cutColor = {0.5f, 0.84f, 0.73f};
    vec3 toolColor = {0.5f, 0.4f, 0.3f};
    vec3 baseShapeColor = {0.7f, 0.6f, 0.5f};

    int mDebug = 0;
    int mDebug1 = 0;
    int mDebug2 = 12;

    int mLastMouseX = 0, mLastMouseY = 0;
    int mMouseButtonState = 0;
    int mLastModifiers = 0;
};

}  // namespace MillSim
