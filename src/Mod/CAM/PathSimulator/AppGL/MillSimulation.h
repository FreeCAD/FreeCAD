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
#include "GuiDisplay.h"
#include <sstream>
#include <vector>

namespace MillSim
{

class MillSimulation
{
public:
    MillSimulation();
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
    void Render();
    void ProcessSim(unsigned int time_ms);
    void HandleKeyPress(int key);
    void UpdateEyeFactor(float factor);
    void TiltEye(float tiltStep);
    void RotateEye(float rotStep);
    void MoveEye(float x, float y);
    void UpdateProjection();
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
    Shader shader3D, shaderInv3D, shaderFlat;
    GCodeParser mCodeParser;
    GuiDisplay guiDisplay;
    std::vector<MillPathSegment*> MillPathSegments;
    std::ostringstream mFpsStream;

    MillMotion mZeroPos = {eNop, -1, 0.0F, 0.0F, 100.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    MillMotion mCurMotion = {eNop, -1, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};
    MillMotion mDestMotion = {eNop, -1, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F, 0.0F};

    StockObject mStockObject;
    StockObject mlightObject;

    vec3 lightColor = {0.8f, 0.9f, 1.0f};
    vec3 lightPos = {20.0f, 20.0f, 10.0f};
    vec3 ambientCol = {0.3f, 0.3f, 0.5f};

    vec3 eye = {0, 100, 40};
    vec3 target = {0, 0, -10};
    vec3 upvec = {0, 0, 1};

    vec3 stockColor = {0.7f, 0.7f, 0.7f};
    vec3 cutColor = {0.4f, 0.7f, 0.4f};
    vec3 toolColor = {0.4f, 0.4f, 0.7f};

    float mEyeDistance = 30;
    float mEyeRoration = 0;
    float mEyeInclination = PI / 6;  // 30 degree
    float mEyeStep = PI / 36;        // 5 degree

    float mMaxStockDim = 100;
    float mMaxFar = 100;
    float mEyeDistFactor = 0.4f;
    float mEyeXZFactor = 0.01f;
    float mEyeXZScale = 0;
    float mEyeX = 0.0f;
    float mEyeZ = 0.0f;


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
    bool mIsRotate = true;
    bool mSimPlaying = false;
    bool mSingleStep = false;
};
}  // namespace MillSim
#endif
