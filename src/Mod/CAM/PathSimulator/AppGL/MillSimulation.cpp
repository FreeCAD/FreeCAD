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

#include "MillSimulation.h"

#include "GlUtils.h"
#include <algorithm>
#include <iostream>
#include <numbers>

// include this last as the defines can mess up other includes
#include "OpenGlWrapper.h"

#define DRAG_ZOOM_FACTOR 10

namespace CAMSimulator
{

MillSimulation::MillSimulation()
{}

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
    mCodeParser.Clear();

    ClearMillPathSegments();
    mTimeline.Clear();

    for (unsigned int i = 0; i < mToolTable.size(); i++) {
        delete mToolTable[i];
    }
    mToolTable.clear();

    mStockObject.Clear();
    simDisplay.CleanGL();

    mCurStep = 0;
    mPathStep = -1;
    mNTotalSteps = 0;

    simulationInitiated = false;
}

void MillSimulation::InitSimulation(float quality, float maxStockDimension)
{
    ClearMillPathSegments();
    mStockCache.Invalidate();
    millPathLine.Clear();
    // mViewSSAO = guiDisplay.IsChecked(eGuiItemAmbientOclusion);

    // gDestPos = curMillOperation->startPos;
    mCurStep = 0;
    mPathStep = -1;
    mNTotalSteps = 0;
    mSimPlaying = false;
    mSimSpeed = 1;
    mSimTime = 0;
    mTotalTime = 0;

    MillPathSegment::SetQuality(quality, maxStockDimension);

    int nOperations = (int)mCodeParser.Operations.size();
    int segId = 0;

    MillMotion prevMotion = !mCodeParser.Operations.empty() ? mCodeParser.Operations.front()
                                                            : MillMotion();

    vec3 startPos;
    mRotary.StartPosition(startPos, prevMotion);
    MillPathPosition mpPos;
    mpPos.X = startPos[0];
    mpPos.Y = startPos[1];
    mpPos.Z = startPos[2];
    mpPos.SegmentId = segId++;
    millPathLine.MillPathPointsBuffer.push_back(mpPos);

    for (int i = 1; i < nOperations; i++) {
        const MillMotion curMotion = mCodeParser.Operations[i];
        const EndMill* tool = GetTool(curMotion.tool);
        if (tool != nullptr) {
            // a move that turns the rotary axes while it cuts is drawn in pieces
            const std::vector<MillPathSegment*> segments = mRotary.TurnsWhileCutting(curMotion)
                ? mRotary.MakeContinuousSegments(*tool, prevMotion, curMotion)
                : std::vector<MillPathSegment*> {mRotary.MakeSegment(*tool, prevMotion, curMotion)};
            for (MillPathSegment* segment : segments) {
                segment->firstStep = mNTotalSteps;
                segment->indexInArray = i;
                segment->segmentIndex = segId++;
                mNTotalSteps += segment->numSimSteps;
                MillPathSegments.push_back(segment);
                segment->AppendPathPoints(millPathLine.MillPathPointsBuffer);
            }
        }

        prevMotion = curMotion;
    }

    assert(mNTotalSteps >= 0);

    mTotalTime = mTimeline.ComputeTimes();
    mRotary.MarkClearRapids(MillPathSegments, mStockPoints);

    mNPathSteps = (int)MillPathSegments.size();
    millPathLine.GenerateModel();

    InitDisplay(quality);

    simulationInitiated = true;
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

bool MillSimulation::ToolExists(int toolid)
{
    return GetTool(toolid) != nullptr;
}

void MillSimulation::GlsimStart()
{
    glDisable(GL_BLEND);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_STENCIL_TEST);
}

void MillSimulation::GlsimToolStep1(void)
{
    glCullFace(GL_BACK);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_ZERO, GL_ZERO, GL_REPLACE);
}

void MillSimulation::GlsimToolStep2(void)
{
    glCullFace(GL_FRONT);
    glDepthFunc(GL_GREATER);
    glDepthMask(GL_TRUE);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void MillSimulation::GlsimClipBack(void)
{
    glCullFace(GL_FRONT);
    glDepthFunc(GL_LESS);
    glDepthMask(GL_FALSE);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
    glStencilOp(GL_REPLACE, GL_REPLACE, GL_ZERO);
}

void MillSimulation::GlsimRenderStock(void)
{
    glCullFace(GL_BACK);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthFunc(GL_EQUAL);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_EQUAL, 1, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
}

void MillSimulation::GlsimRenderTools(void)
{
    glCullFace(GL_FRONT);
}

void MillSimulation::GlsimEnd(void)
{
    glCullFace(GL_BACK);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);

    glDisable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 1, 0xFF);
}

void MillSimulation::renderSegmentForward(int iSeg)
{
    MillPathSegment* p = MillPathSegments.at(iSeg);
    if (!p->isCutting) {
        return;
    }
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
    MillPathSegment* p = MillPathSegments.at(iSeg);
    if (!p->isCutting) {
        return;
    }
    int step = iSeg == mPathStep ? mSubStep : p->numSimSteps;
    int end = p->isMultyPart ? 1 : step;
    for (int i = step; i >= end; i--) {
        GlsimToolStep1();
        p->render(i);
        GlsimToolStep2();
        p->render(i);
    }
}

const std::vector<SimTimeSpan>& MillSimulation::GetIndexSpans() const
{
    return mTimeline.IndexSpans();
}

const std::vector<SimRotaryAxis>& MillSimulation::GetRotaryAxes() const
{
    return mRotary.Axes();
}

bool MillSimulation::GetIndexAngles(std::vector<float>& angles) const
{
    // the rotary positions during an index that turns the axes one by one; false when none runs
    if (mPathStep < 0 || mPathStep >= (int)MillPathSegments.size()) {
        return false;
    }
    return mRotary.IndexAnglesAtStep(MillPathSegments[mPathStep], mSubStep, angles);
}

void MillSimulation::SetRotaryAxes(const std::vector<SimRotaryAxis>& axes)
{
    mRotary.SetAxes(axes);
}

void MillSimulation::StepFromTime()
{
    mCurStep = mTimeline.StepAtTime(mSimTime, mNTotalSteps);
}

float MillSimulation::TimeOfStep(int step) const
{
    return mTimeline.TimeOfStep(step, mNTotalSteps);
}

void MillSimulation::CalcSegmentPositions()
{
    mSubStep = mCurStep;
    for (mPathStep = 0; mPathStep < mNPathSteps; mPathStep++) {
        MillPathSegment* p = MillPathSegments[mPathStep];
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
    if (mPathStep >= 0) {
        mCurFeed = MillPathSegments[mPathStep]->feed;
        mCurRapid = MillPathSegments[mPathStep]->isRapid;
    }
}

void MillSimulation::GetScenePose(quat pose)
{
    quat_identity(pose);
    if (MillPathSegments.empty()) {
        return;
    }
    // At the end of the program the part is shown in the main work coordinates, axes at home.
    if (mCurStep >= mNTotalSteps) {
        return;
    }
    if (mPathStep < 0) {
        mRotary.FramePose(pose, mCodeParser.Frames[MillPathSegments[0]->frameFrom]);
        return;
    }
    mRotary.ScenePose(pose, MillPathSegments.at(mPathStep), mSubStep);
}

void MillSimulation::UpdateScene()
{
    // the scene turns with the table, about the stock center so the stock stays in view
    quat pose;
    GetScenePose(pose);
    const vec3& c = mStockObject.center;
    mat4x4 scene, rot;
    mat4x4_from_quat(rot, pose);
    mat4x4_translate(scene, c[0], c[1], c[2]);
    mat4x4_mul(scene, scene, rot);
    mat4x4_translate_in_place(scene, -c[0], -c[1], -c[2]);
    simDisplay.SetSceneMatrix(scene);
}

void MillSimulation::RenderSimulation(int first, bool fromScratch)
{
    // Draws the stock minus the tool sweeps from segment `first` to the current step. Without
    // fromScratch the depth buffer already holds the earlier sweeps (see StockCache).
    if ((mViewItems & VIEWITEM_SIMULATION) == 0) {
        return;
    }

    simDisplay.StartDepthPass();

    GlsimStart();
    if (fromScratch) {
        mStockObject.render();
    }

    GlsimToolStep2();

    for (int i = first; i <= mPathStep; i++) {
        renderSegmentForward(i);
    }

    for (int i = mPathStep; i >= first; i--) {
        renderSegmentForward(i);
    }

    for (int i = first; i < mPathStep; i++) {
        renderSegmentReversed(i);
    }

    for (int i = mPathStep; i >= first; i--) {
        renderSegmentReversed(i);
    }

    GlsimClipBack();
    mStockObject.render();

    if (!fromScratch) {
        // clear the cached pixels where the stock is now cut through
        simDisplay.ClearCacheHoles();
    }

    // start coloring
    simDisplay.StartGeometryPass(stockColor, false);
    GlsimRenderStock();
    mStockObject.render();

    // render cuts (back faces of tools)
    simDisplay.StartGeometryPass(cutColor, true);
    GlsimRenderTools();
    for (int i = first; i <= mPathStep; i++) {
        MillPathSegment* p = MillPathSegments.at(i);
        if (!p->isCutting) {
            continue;
        }
        int step = (i == mPathStep) ? mSubStep : p->numSimSteps;
        int start = p->isMultyPart ? 1 : step;
        for (int j = start; j <= step; j++) {
            MillPathSegments.at(i)->render(j);
        }
    }

    GlsimEnd();
}

void MillSimulation::ToolPose(MillPathSegment* p, int step, vec3 toolPos, mat4x4 rmat)
{
    quat scene;
    GetScenePose(scene);
    mRotary.ToolPose(p, step, toolPos, rmat, scene);
}

void MillSimulation::RenderTool()
{
    if (mPathStep < 0) {
        return;
    }

    MillPathSegment* p = MillPathSegments.at(mPathStep);
    vec3 toolPos;
    mat4x4 tmat, rmat;
    ToolPose(p, mSubStep, toolPos, rmat);
    mat4x4_translate(tmat, toolPos[0], toolPos[1], toolPos[2]);
    mat4x4_mul(tmat, tmat, rmat);
    // mat4x4_translate(tmat, toolPos.x, toolPos.y, toolPos.z);
    simDisplay.StartGeometryPass(toolColor, false);
    p->endmill->toolShape.Render(tmat, rmat);
}

void MillSimulation::RenderPath()
{
    if (!mViewPath) {
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
    if (!simulationInitiated) {
        return;
    }

    // set background
    glClearColor(bgndColor[0], bgndColor[1], bgndColor[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // render the simulation offscreen in an FBO

    if (simDisplay.updateDisplay) {
        UpdateScene();
        simDisplay.PrepareFrameBuffer();
        if (mIncremental && (mViewItems & VIEWITEM_SIMULATION) != 0 && simDisplay.MakeCache()) {
            mStockCache.Render(simDisplay, mPathStep, mSubStep, [this](int first, bool fromScratch) {
                RenderSimulation(first, fromScratch);
            });
        }
        else {
            RenderSimulation();
        }
        RenderTool();
        RenderBaseShape();
        RenderPath();
        simDisplay.updateDisplay = false;
        simDisplay.RenderResult(true, mViewSSAO);
    }
    else {
        simDisplay.RenderResult(false, mViewSSAO);
    }

    /*   if (mDebug > 0) {
           mat4x4 test;
           mat4x4_identity(test);
           mat4x4_translate_in_place(test, 20, 20, 3);
           mat4x4_rotate_Z(test, test, 30.f * 3.14f / 180.f);
           int dpos = mNPathSteps - mDebug2;
           MillPathSegment* p = MillPathSegments.at(dpos);
           if (mDebug > p->numSimSteps) {
               mDebug = 1;
           }
           p->render(mDebug);
       }*/

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void MillSimulation::ProcessSim(const clock::duration& elapsed)
{
    SimNext(elapsed);
    Render();
}

void MillSimulation::SimNext(const clock::duration& elapsed)
{
    // calculate number of steps based on elapsed time

    const int oldStep = mCurStep;
    const float oldTime = mSimTime;

    if (mSimPlaying) {
        if (mTimeline.IsHolding()) {
            return;  // paused after a back skip
        }
        // at the programmed feeds, speed times real time
        const float seconds = std::chrono::duration_cast<std::chrono::duration<float>>(elapsed).count();
        mSimTime = mTimeline.Advance(mSimTime, seconds, mSimSpeed);
        StepFromTime();
    }
    else if (mSingleStep) {
        mCurStep = std::min(mCurStep + 1, mNTotalSteps);
        mSimTime = TimeOfStep(mCurStep);
        mSingleStep = false;
    }
    else {
        return;
    }

    if (mCurStep == mNTotalSteps) {
        mSimPlaying = false;
    }

    if (mCurStep != oldStep || mSimTime != oldTime) {
        CalcSegmentPositions();
        simDisplay.updateDisplay = true;
    }
}

void MillSimulation::InitDisplay(float quality)
{
    // generate tools
    for (unsigned int i = 0; i < mToolTable.size(); i++) {
        mToolTable[i]->GenerateDisplayLists(quality);
    }

    // Make sure the next call to UpdateWindowScale will not return early.
    mWidth = -1;
    mHeight = -1;

    // init 3d display
    simDisplay.InitGL();
}

void MillSimulation::SetBoxStock(float x, float y, float z, float l, float w, float h)
{
    mStockObject.GenerateBoxStock(x, y, z, l, w, h);
    simDisplay.ScaleViewToStock(&mStockObject);
    mStockCache.Invalidate();
}

void MillSimulation::SetArbitraryStock(
    const std::vector<Vertex>& verts,
    const std::vector<GLushort>& indices
)
{
    mStockObject.GenerateSolid(verts, indices);
    simDisplay.ScaleViewToStock(&mStockObject);
    mStockCache.Invalidate();
    mStockPoints.clear();
    for (const Vertex& v : verts) {
        mStockPoints.push_back({v.x, v.y, v.z});
    }
    mRotary.MarkClearRapids(MillPathSegments, mStockPoints);
}

void MillSimulation::SetStockVisible(bool b)
{
    if (b == IsStockVisible()) {
        return;
    }

    mViewItems ^= VIEWITEM_SIMULATION;
    simDisplay.updateDisplay = true;
}

bool MillSimulation::IsStockVisible() const
{
    return mViewItems & VIEWITEM_SIMULATION;
}

void MillSimulation::SetBaseObject(const std::vector<Vertex>& verts, const std::vector<GLushort>& indices)
{
    mBaseShape.GenerateSolid(verts, indices);
}

void MillSimulation::SetBaseVisible(bool b)
{
    if (b == IsBaseVisible()) {
        return;
    }

    mViewItems ^= VIEWITEM_BASE_SHAPE;
    simDisplay.updateDisplay = true;
}

bool MillSimulation::IsBaseVisible() const
{
    return mViewItems & VIEWITEM_BASE_SHAPE;
}

void MillSimulation::UpdateWindowScale(int width, int height)
{
    if (width == mWidth && height == mHeight) {
        return;
    }

    mWidth = width;
    mHeight = height;

    simDisplay.UpdateWindowScale(width, height);
}

void MillSimulation::SetPathVisible(bool b)
{
    if (b == mViewPath) {
        return;
    }

    mViewPath = b;
    simDisplay.updateDisplay = true;
}

void MillSimulation::EnableSsao(bool b)
{
    if (b == mViewSSAO) {
        return;
    }

    mViewSSAO = b;
    simDisplay.updateDisplay = true;
}

void MillSimulation::EnableIncremental(bool b)
{
    if (b == mIncremental) {
        return;
    }

    mIncremental = b;
    mStockCache.Invalidate();
    simDisplay.updateDisplay = true;
}

void MillSimulation::EnableTablePose(bool b)
{
    if (b == mViewTablePose) {
        return;
    }

    mViewTablePose = b;
    mRotary.SetTablePose(b);
    simDisplay.updateDisplay = true;
}

void MillSimulation::UpdateCamera(const SoCamera& camera)
{
    simDisplay.UpdateCamera(camera);
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

void MillSimulation::SetFrame(const MillFrame& frame)
{
    mCodeParser.SetFrame(frame);
}

void MillSimulation::BeginOperation(const std::string& name)
{
    mCodeParser.BeginOperation(name);
}

const std::vector<float>& MillSimulation::GetOperationStarts() const
{
    return mTimeline.OperationStarts();
}

std::string MillSimulation::GetCurrentOperation() const
{
    if (mPathStep < 0 || mPathStep >= (int)MillPathSegments.size()) {
        return {};
    }
    const int op = MillPathSegments[mPathStep]->op;
    if (op < 0 || op >= (int)mCodeParser.OpNames.size()) {
        return {};
    }
    return mCodeParser.OpNames[op];
}

void MillSimulation::SkipToPreviousMark()
{
    // Jumps back to the start of the current section, or to the mark before. While playing it
    // pauses there a moment, so another press goes further back.
    if (mSimPlaying) {
        mTimeline.HoldPlayback();
    }
    const std::vector<float>& marks = mTimeline.Marks();
    auto it = std::lower_bound(marks.begin(), marks.end(), mSimTime - SimTimeline::TimeTolerance);
    SkipTo(it == marks.begin() ? 0.f : *(it - 1));
}

void MillSimulation::SkipToNextMark()
{
    // Jumps to the next mark: an operation start, or a rotary axis starting or stopping. After
    // the last mark, jumps to the end of the program.
    const std::vector<float>& marks = mTimeline.Marks();
    auto it = std::upper_bound(marks.begin(), marks.end(), mSimTime + SimTimeline::TimeTolerance);
    SkipTo(it == marks.end() ? mTotalTime : *it);
}

void MillSimulation::SkipTo(float time)
{
    if (time == mSimTime) {
        return;
    }
    mSimTime = time;
    StepFromTime();
    CalcSegmentPositions();
    simDisplay.updateDisplay = true;
}

void MillSimulation::SetPlaying(bool b)
{
    if (b == mSimPlaying) {
        return;
    }

    mSimPlaying = b;
    simDisplay.updateDisplay = true;
}

void MillSimulation::SingleStep()
{
    if (!mSimPlaying && mSingleStep) {
        return;
    }

    mSimPlaying = false;
    mSingleStep = true;
    simDisplay.updateDisplay = true;
}

void MillSimulation::SetSpeed(int s)
{
    mSimSpeed = s;
}

void MillSimulation::SetSimulationStage(float stage)
{
    // stage is a fraction of the program time
    mSimTime = std::clamp(stage, 0.f, 1.f) * mTotalTime;
    const int oldStep = mCurStep;
    StepFromTime();
    if (mCurStep == oldStep) {
        return;
    }

    CalcSegmentPositions();

    simDisplay.updateDisplay = true;
}

void MillSimulation::SetState(const MillSimulationState& state)
{
    SetPlaying(state.mSimPlaying);
    if (state.mSingleStep) {
        SingleStep();
    }

    const float stage = state.mTotalTime > 0 ? state.mSimTime / state.mTotalTime : 0.f;
    SetSimulationStage(stage);

    SetSpeed(state.mSimSpeed);

    mViewItems = state.mViewItems;
    mViewPath = state.mViewPath;
    mViewSSAO = state.mViewSSAO;
    mViewTablePose = state.mViewTablePose;
    mRotary.SetTablePose(mViewTablePose);
    EnableIncremental(state.mIncremental);
}

const MillSimulationState& MillSimulation::GetState() const
{
    return *this;
}

void MillSimulation::SetBackgroundColor(const vec3& c)
{
    bgndColor[0] = c[0];
    bgndColor[1] = c[1];
    bgndColor[2] = c[2];
}

void MillSimulation::SetPathColor(const vec3& normal, const vec3& rapid)
{
    simDisplay.SetPathColor(normal, rapid);
}


}  // namespace CAMSimulator
