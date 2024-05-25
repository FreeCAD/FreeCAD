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

#ifndef __millsimulation__h__
#define __millsimulation__h__

#include "MillMotion.h"
#include "GCodeParser.h"
#include "Shader.h"
#include "linmath.h"
#include "GlUtils.h"
#include "StockObject.h"
#include "MillPathSegment.h"
#include "SimDisplay.h"
#include "GuiDisplay.h"
#include <sstream>
#include <vector>

namespace MillSim
{

class MillSimulation
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
    bool ToolExists(int toolid)
    {
        return GetTool(toolid) != nullptr;
    }
    void RenderSimulation();
    void Render();
    void ProcessSim(unsigned int time_ms);
    void HandleKeyPress(int key);
    bool LoadGCodeFile(const char* fileName);
    bool AddGcodeLine(const char* line);
    void SetSimulationStage(float stage);
    void SetBoxStock(float x, float y, float z, float l, float w, float h);
    void MouseDrag(int buttons, int dx, int dy);
    void MouseMove(int px, int py);
    void MouseScroll(float dy);
    void MouseHover(int px, int py);
    void MousePress(int button, bool isPressed, int px, int py);


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
    std::vector<MillPathSegment*> MillPathSegments;
    std::ostringstream mFpsStream;

    MillMotion mZeroPos = {eNop, -1, 0, 0, 100, 0, 0, 0};
    MillMotion mCurMotion = {eNop, -1, 0, 0, 0, 0, 0, 0};
    MillMotion mDestMotion = {eNop, -1, 0, 0, 0, 0, 0, 0};

    StockObject mStockObject;

    vec3 stockColor = {0.4f, 0.5f, 0.7f};
    vec3 cutColor = {0.5f, 0.8f, 0.5f};
    vec3 toolColor = {0.4f, 0.4f, 0.7f};

    int mCurStep = 0;
    int mNTotalSteps = 0;
    int mPathStep = 0;
    int mSubStep = 0;
    int mNPathSteps = 0;
    int mDebug = 0;
    int mDebug1 = 0;
    int mDebug2 = 12;
    int mSimSpeed = 1;

    int mLastMouseX = 0, mLastMouseY = 0;
    int mMouseButtonState = 0;

    bool mIsInStock = false;
    bool mIsRotate = false;
    bool mSimPlaying = false;
    bool mSingleStep = false;

};
}  // namespace MillSim
#endif