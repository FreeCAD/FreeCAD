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

#include "MillSimulation.h"
#include "OpenGlWrapper.h"
#include <vector>
#include <iostream>

#define DRAG_ZOOM_FACTOR 10

namespace MillSim
{

MillSimulation::MillSimulation()
{
    mCurMotion = {eNop, -1, 0, 0, 0, 0, 0, 0, 0, '\0', 0.0};
    guiDisplay.SetMillSimulator(this);
}

MillSimulation::~MillSimulation()
{
    Clear();
}

void MillSimulation::ClearMillPathSegments()
{
    for (unsigned int i = 0; i < MillPathSegments.size(); i++) {
        delete MillPathSegments[i];
    }
    MillPathSegments.clear();
}

void MillSimulation::Clear()
{
    mCodeParser.Operations.clear();
    for (unsigned int i = 0; i < mToolTable.size(); i++) {
        delete mToolTable[i];
    }
    ClearMillPathSegments();
    mStockObject.~StockObject();
    mToolTable.clear();
    guiDisplay.ResetGui();
    simDisplay.CleanGL();
    mCurStep = 0;
    mPathStep = -1;
    mNTotalSteps = 0;
}


void MillSimulation::SimNext()
{
    static int simDecim = 0;

    simDecim++;
    if (simDecim < 1) {
        return;
    }

    simDecim = 0;

    if (mCurStep < mNTotalSteps) {
        mCurStep += mSimSpeed;
        if (mCurStep > mNTotalSteps) {
            mCurStep = mNTotalSteps;
        }
        CalcSegmentPositions();
        simDisplay.updateDisplay = true;
    }
}

void MillSimulation::InitSimulation(float quality)
{
    ClearMillPathSegments();
    millPathLine.Clear();
    simDisplay.applySSAO = guiDisplay.IsChecked(eGuiItemAmbientOclusion);

    mDestMotion = mZeroPos;
    // gDestPos = curMillOperation->startPos;
    mCurStep = 0;
    mPathStep = -1;
    mNTotalSteps = 0;
    mSimPlaying = false;
    mSimSpeed = 1;
    MillPathSegment::SetQuality(quality, simDisplay.maxFar);
    int nOperations = (int)mCodeParser.Operations.size();
    int segId = 0;
    for (int i = 0; i < nOperations; i++) {
        mCurMotion = mDestMotion;
        mDestMotion = mCodeParser.Operations[i];
        EndMill* tool = GetTool(mDestMotion.tool);
        if (tool != nullptr) {
            MillSim::MillPathSegment* segment =
                new MillSim::MillPathSegment(tool, &mCurMotion, &mDestMotion);
            segment->indexInArray = i;
            segment->segmentIndex = segId++;
            mNTotalSteps += segment->numSimSteps;
            MillPathSegments.push_back(segment);
            segment->AppendPathPoints(millPathLine.MillPathPointsBuffer);
        }
    }
    mNPathSteps = (int)MillPathSegments.size();
    millPathLine.GenerateModel();
    InitDisplay(quality);
}

EndMill* MillSimulation::GetTool(int toolId)
{
    for (unsigned int i = 0; i < mToolTable.size(); i++) {
        if (mToolTable[i]->toolId == toolId) {
            return mToolTable[i];
        }
    }
    return nullptr;
}

void MillSimulation::RemoveTool(const int toolId)
{
    EndMill* tool = GetTool(toolId);
    if (tool == nullptr) {
        return;
    }

    if (const auto it = std::ranges::find(mToolTable, tool); it != mToolTable.end()) {
        mToolTable.erase(it);
    }
    delete tool;
}


void MillSimulation::AddTool(EndMill* tool)
{
    // if we have another tool with same id, remove it
    RemoveTool(tool->toolId);
    mToolTable.push_back(tool);
}

void MillSimulation::AddTool(const std::vector<float>& toolProfile, int toolid, float diameter)
{
    // if we have another tool with same id, remove it
    RemoveTool(toolid);
    EndMill* tool = new EndMill(toolProfile, toolid, diameter);
    mToolTable.push_back(tool);
}

void MillSimulation::GlsimStart()
{
    glDisable(GL_BLEND);
    glEnable(GL_STENCIL_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
}

void MillSimulation::GlsimToolStep1(void)
{
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_ZERO, GL_ZERO, GL_REPLACE);
    glDepthMask(GL_FALSE);
}


void MillSimulation::GlsimToolStep2(void)
{
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDepthFunc(GL_GREATER);
    glCullFace(GL_FRONT);
    glDepthMask(GL_TRUE);
}

void MillSimulation::GlsimClipBack(void)
{
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_ZERO);
    glDepthFunc(GL_LESS);
    glCullFace(GL_FRONT);
    glDepthMask(GL_FALSE);
}


void MillSimulation::GlsimRenderStock(void)
{
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glDepthFunc(GL_EQUAL);
    glCullFace(GL_BACK);
}

void MillSimulation::GlsimRenderTools(void)
{
    glCullFace(GL_FRONT);
}

void MillSimulation::GlsimEnd(void)
{
    glCullFace(GL_BACK);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
}

void MillSimulation::renderSegmentForward(int iSeg)
{
    MillSim::MillPathSegment* p = MillPathSegments.at(iSeg);
    int step = iSeg == mPathStep ? mSubStep : p->numSimSteps;
    int start = p->isMultyPart ? 1 : step;
    for (int i = start; i <= step; i++) {
        GlsimToolStep1();
        p->render(i);
        GlsimToolStep2();
        p->render(i);
    }
}

void MillSimulation::renderSegmentReversed(int iSeg)
{
    MillSim::MillPathSegment* p = MillPathSegments.at(iSeg);
    int step = iSeg == mPathStep ? mSubStep : p->numSimSteps;
    int end = p->isMultyPart ? 1 : step;
    for (int i = step; i >= end; i--) {
        GlsimToolStep1();
        p->render(i);
        GlsimToolStep2();
        p->render(i);
    }
}

void MillSimulation::CalcSegmentPositions()
{
    mSubStep = mCurStep;
    for (mPathStep = 0; mPathStep < mNPathSteps; mPathStep++) {
        MillSim::MillPathSegment* p = MillPathSegments[mPathStep];
        if (mSubStep < p->numSimSteps) {
            break;
        }
        mSubStep -= p->numSimSteps;
    }
    if (mPathStep >= mNPathSteps) {
        mPathStep = mNPathSteps - 1;
        mSubStep = MillPathSegments[mPathStep]->numSimSteps;
    }
    else {
        mSubStep++;
    }
}

void MillSimulation::RenderSimulation()
{
    if ((mViewItems & VIEWITEM_SIMULATION) == 0) {
        return;
    }

    simDisplay.StartDepthPass();

    GlsimStart();
    mStockObject.render();

    GlsimToolStep2();

    for (int i = 0; i <= mPathStep; i++) {
        renderSegmentForward(i);
    }

    for (int i = mPathStep; i >= 0; i--) {
        renderSegmentForward(i);
    }

    for (int i = 0; i < mPathStep; i++) {
        renderSegmentReversed(i);
    }

    for (int i = mPathStep; i >= 0; i--) {
        renderSegmentReversed(i);
    }

    GlsimClipBack();
    mStockObject.render();

    // start coloring
    simDisplay.StartGeometryPass(stockColor, false);
    GlsimRenderStock();
    mStockObject.render();

    // render cuts (back faces of tools)
    simDisplay.StartGeometryPass(cutColor, true);
    GlsimRenderTools();
    for (int i = 0; i <= mPathStep; i++) {
        MillSim::MillPathSegment* p = MillPathSegments.at(i);
        int step = (i == mPathStep) ? mSubStep : p->numSimSteps;
        int start = p->isMultyPart ? 1 : step;
        for (int j = start; j <= step; j++) {
            MillPathSegments.at(i)->render(j);
        }
    }

    GlsimEnd();
}

void MillSimulation::RenderTool()
{
    if (mPathStep < 0) {
        return;
    }

    vec3 toolPos;
    MotionPosToVec(toolPos, &mDestMotion);
    MillSim::MillPathSegment* p = MillPathSegments.at(mPathStep);
    p->GetHeadPosition(toolPos);
    mat4x4 tmat;
    mat4x4_translate(tmat, toolPos[0], toolPos[1], toolPos[2]);
    // mat4x4_translate(tmat, toolPos.x, toolPos.y, toolPos.z);
    simDisplay.StartGeometryPass(toolColor, false);
    p->endmill->toolShape.Render(tmat, identityMat);
}

void MillSimulation::RenderPath()
{
    if (!guiDisplay.IsChecked(eGuiItemPath)) {
        return;
    }
    simDisplay.SetupLinePathPass(mPathStep, false);
    millPathLine.Render();
    simDisplay.SetupLinePathPass(mPathStep, true);
    millPathLine.Render();
    glDepthMask(GL_TRUE);
}

void MillSimulation::RenderBaseShape()
{
    if ((mViewItems & VIEWITEM_BASE_SHAPE) == 0) {
        return;
    }
    simDisplay.StartDepthPass();
    glPolygonOffset(0, -2);
    glEnable(GL_POLYGON_OFFSET_FILL);
    simDisplay.StartGeometryPass(baseShapeColor, false);
    mBaseShape.render();
    glDisable(GL_POLYGON_OFFSET_FILL);
}

void MillSimulation::Render()
{
    // set background
    glClearColor(bgndColor[0], bgndColor[1], bgndColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    simDisplay.PrepareDisplay(mStockObject.center);

    // render the simulation offscreen in an FBO
    if (simDisplay.updateDisplay) {
        simDisplay.PrepareFrameBuffer();
        RenderSimulation();
        RenderTool();
        RenderBaseShape();
        RenderPath();
        simDisplay.updateDisplay = false;
        simDisplay.RenderResult(true);
    }
    else {
        simDisplay.RenderResult(false);
    }

    /*   if (mDebug > 0) {
           mat4x4 test;
           mat4x4_identity(test);
           mat4x4_translate_in_place(test, 20, 20, 3);
           mat4x4_rotate_Z(test, test, 30.f * 3.14f / 180.f);
           int dpos = mNPathSteps - mDebug2;
           MillSim::MillPathSegment* p = MillPathSegments.at(dpos);
           if (mDebug > p->numSimSteps) {
               mDebug = 1;
           }
           p->render(mDebug);
       }*/

    float progress = (float)mCurStep / mNTotalSteps;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    guiDisplay.Render(progress);
}

void MillSimulation::ProcessSim(unsigned int time_ms)
{

    static int ancient = 0;
    static unsigned int last = 0;
    static unsigned int msec = 0xFFFFFFFF;
    static int fps = 0;
    static int renderTime = 0;

    last = msec == 0xFFFFFFFF ? time_ms : msec;
    msec = time_ms;
    if (guiDisplay.IsChecked(eGuiItemRotate)) {
        simDisplay.RotateEye((msec - last) / 4600.0f);
    }

    if (last / 1000 != msec / 1000) {
        float calcFps = 1000.0f * fps / (msec - ancient);
        mFpsStream.str("");
        mFpsStream << "fps: " << calcFps << "    rendertime:" << renderTime
                   << "    zpos:" << mDestMotion.z << std::ends;
        ancient = msec;
        fps = 0;
    }

    if (mSimPlaying || mSingleStep) {
        SimNext();
        mSingleStep = false;
    }

    Render();

    ++fps;
}

void MillSimulation::HandleKeyPress(int key)
{
    if (key >= '1' && key <= '9') {
        mSimSpeed = key - '0';
    }
    else if (key == 'D') {
        mDebug++;
    }
    else if (key == 'K') {
        mDebug2++;
        gDebug = mNPathSteps - mDebug2;
    }
    else {
        guiDisplay.HandleKeyPress(key);
    }
}

void MillSimulation::HandleGuiAction(eGuiItems actionItem, bool checked)
{
    switch (actionItem) {
        case eGuiItemPlay:
            mSimPlaying = true;
            break;

        case eGuiItemPause:
            mSimPlaying = false;
            break;

        case eGuiItemSingleStep:
            mSimPlaying = false;
            mSingleStep = true;
            break;

        case eGuiItemSlower:
            if (mSimSpeed == 50) {
                mSimSpeed = 25;
            }
            else if (mSimSpeed == 25) {
                mSimSpeed = 10;
            }
            else if (mSimSpeed == 10) {
                mSimSpeed = 5;
            }
            else {
                mSimSpeed = 1;
            }
            guiDisplay.UpdateSimSpeed(mSimSpeed);
            break;

        case eGuiItemFaster:
            if (mSimSpeed == 1) {
                mSimSpeed = 5;
            }
            else if (mSimSpeed == 5) {
                mSimSpeed = 10;
            }
            else if (mSimSpeed == 10) {
                mSimSpeed = 25;
            }
            else {
                mSimSpeed = 50;
            }
            guiDisplay.UpdateSimSpeed(mSimSpeed);
            break;

        case eGuiItemPath:
            simDisplay.updateDisplay = true;
            break;

        case eGuiItemAmbientOclusion:
            simDisplay.applySSAO = checked;
            simDisplay.updateDisplay = true;
            break;

        case eGuiItemView:
            mViewItems++;
            if (mViewItems >= VIEWITEM_MAX) {
                mViewItems = VIEWITEM_SIMULATION;
            }
            simDisplay.updateDisplay = true;
            break;

        case eGuiItemHome:
            simDisplay.MoveEyeCenter();
            break;

        default:
            break;
    }
    guiDisplay.UpdatePlayState(mSimPlaying);
}


void MillSimulation::InitDisplay(float quality)
{
    // generate tools
    for (unsigned int i = 0; i < mToolTable.size(); i++) {
        mToolTable[i]->GenerateDisplayLists(quality);
    }

    // init 3d display
    simDisplay.InitGL();

    // init gui elements
    guiDisplay.InitGui();
}

void MillSimulation::SetBoxStock(float x, float y, float z, float l, float w, float h)
{
    mStockObject.GenerateBoxStock(x, y, z, l, w, h);
    simDisplay.ScaleViewToStock(&mStockObject);
}

void MillSimulation::SetArbitraryStock(std::vector<Vertex>& verts, std::vector<GLushort>& indices)
{
    mStockObject.GenerateSolid(verts, indices);
    simDisplay.ScaleViewToStock(&mStockObject);
}

void MillSimulation::SetBaseObject(std::vector<Vertex>& verts, std::vector<GLushort>& indices)
{
    mBaseShape.GenerateSolid(verts, indices);
}

void MillSimulation::MouseDrag(int buttons, int dx, int dy)
{
    if (buttons == (MS_MOUSE_MID | MS_MOUSE_LEFT) || buttons == MS_KBD_ALT) {
        simDisplay.TiltEye((float)dy / 100.0f);
        simDisplay.RotateEye((float)dx / 100.0f);
    }
    else if (buttons == MS_MOUSE_MID || buttons == MS_KBD_SHIFT) {
        simDisplay.MoveEye(dx, -dy);
    }
    else if (buttons == (MS_KBD_CONTROL | MS_KBD_SHIFT)) {
        Zoom(0.003 * dy);
    }
    guiDisplay.MouseDrag(buttons, dx, dy);
}

void MillSimulation::MouseMove(int px, int py, int modifiers)
{
    if (modifiers != mLastModifiers) {
        mLastMouseX = px;
        mLastMouseY = py;
        mLastModifiers = modifiers;
    }

    int buttons = mMouseButtonState | modifiers;
    if (buttons > 0) {
        int dx = px - mLastMouseX;
        int dy = py - mLastMouseY;
        if (dx != 0 || dy != 0) {
            MouseDrag(buttons, dx, dy);
            mLastMouseX = px;
            mLastMouseY = py;
        }
    }
    else {
        MouseHover(px, py);
    }
}

void MillSimulation::MouseScroll(float dy)
{
    Zoom(-0.02f * dy);
}


void MillSimulation::MouseHover(int px, int py)
{
    guiDisplay.MouseCursorPos(px, py);
}

void MillSimulation::MousePress(int button, bool isPressed, int px, int py)
{
    if (isPressed) {
        mMouseButtonState |= button;
    }
    else {
        mMouseButtonState &= ~button;
    }

    if (mMouseButtonState > 0) {
        mLastMouseX = px;
        mLastMouseY = py;
    }
    guiDisplay.MousePressed(button, isPressed, mSimPlaying);
}

void MillSimulation::Zoom(float factor)
{
    factor += simDisplay.GetEyeFactor();
    if (factor > 0.6f) {
        factor = 0.6f;
    }
    else if (factor < 0.01f) {
        factor = 0.01f;
    }
    simDisplay.UpdateEyeFactor(factor);
}

void MillSimulation::UpdateWindowScale(int width, int height)
{
    if (width == gWindowSizeW && height == gWindowSizeH) {
        return;
    }
    gWindowSizeW = width;
    gWindowSizeH = height;
    simDisplay.UpdateWindowScale();
    guiDisplay.UpdateWindowScale();
    simDisplay.updateDisplay = true;
}


bool MillSimulation::LoadGCodeFile(const char* fileName)
{
    if (mCodeParser.Parse(fileName)) {
        std::cout << "GCode file loaded successfully" << std::endl;
        return true;
    }
    return false;
}

bool MillSimulation::AddGcodeLine(const char* line)
{
    return mCodeParser.AddLine(line);
}

void MillSimulation::SetSimulationStage(float stage)
{
    int newStep = (int)((float)mNTotalSteps * stage);
    if (newStep == mCurStep) {
        return;
    }
    mCurStep = newStep;
    simDisplay.updateDisplay = true;
    mSingleStep = true;
    CalcSegmentPositions();
}

}  // namespace MillSim
