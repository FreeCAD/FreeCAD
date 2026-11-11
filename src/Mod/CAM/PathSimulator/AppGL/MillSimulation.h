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

#include "GCodeParser.h"
#include "MillPathLine.h"
#include "MillPathSegment.h"
#include "SimDisplay.h"
#include "SimRotary.h"
#include "SimTimeline.h"
#include "StockCache.h"
#include "SolidObject.h"
#include "StockObject.h"
#include "linmath.h"
#include <chrono>
#include <memory>
#include <vector>

#define VIEWITEM_SIMULATION 1
#define VIEWITEM_BASE_SHAPE 2
#define VIEWITEM_MAX 4

namespace CAMSimulator
{

struct MillSimulationState
{
    int mCurStep = 0;
    int mNTotalSteps = 0;
    int mPathStep = -1;
    int mSubStep = 0;
    int mNPathSteps = 0;
    int mSimSpeed = 1;
    float mSimTime = 0;    // machine time into the program, in seconds
    float mTotalTime = 0;  // machine time of the whole program, in seconds
    float mCurFeed = 0;    // feed rate of the current move in mm/s, 0 when unknown
    bool mCurRapid = false;
    int mViewItems = VIEWITEM_SIMULATION;
    bool mViewPath = false;
    bool mViewSSAO = false;
    bool mViewTablePose = true;  // the part turns with the rotary table, else the tool tilts
    bool mIncremental = true;    // keep the cut stock between frames and draw only the new moves

    bool mSimPlaying = false;
    bool mSingleStep = false;
};

class MillSimulation: private MillSimulationState
{
    typedef std::chrono::steady_clock clock;

public:
    MillSimulation();
    ~MillSimulation();
    void ClearMillPathSegments();
    void Clear();
    void InitSimulation(float quality, float maxStockDimension);
    void AddTool(EndMill* tool);
    void AddTool(const std::vector<float>& toolProfile, int toolid, float diameter);
    bool ToolExists(int toolid);
    void RenderSimulation(int first = 0, bool fromScratch = true);
    void RenderTool();
    void RenderPath();
    void RenderBaseShape();
    void Render();
    void ProcessSim(const clock::duration& elapsed);
    void SimNext(const clock::duration& elapsed);

    bool LoadGCodeFile(const char* fileName);
    bool AddGcodeLine(const char* line);
    void SetFrame(const MillFrame& frame);
    void BeginOperation(const std::string& name);
    void SkipToNextMark();
    void SkipToPreviousMark();
    const std::vector<SimTimeSpan>& GetIndexSpans() const;
    const std::vector<SimRotaryAxis>& GetRotaryAxes() const;
    bool GetIndexAngles(std::vector<float>& angles) const;
    const std::vector<float>& GetOperationStarts() const;
    std::string GetCurrentOperation() const;

    void SetPlaying(bool b);
    void SingleStep();
    void SetSpeed(int s);

    void SetSimulationStage(float stage);
    void SetState(const MillSimulationState& state);
    const MillSimulationState& GetState() const;

    void SetBoxStock(float x, float y, float z, float l, float w, float h);
    void SetArbitraryStock(const std::vector<Vertex>& verts, const std::vector<GLushort>& indices);
    void SetStockVisible(bool b);
    bool IsStockVisible() const;

    void SetBaseObject(const std::vector<Vertex>& verts, const std::vector<GLushort>& indices);
    void SetBaseVisible(bool b);
    bool IsBaseVisible() const;

    void SetPathVisible(bool b);
    void EnableSsao(bool b);
    void EnableIncremental(bool b);
    void EnableTablePose(bool b);
    void SetRotaryAxes(const std::vector<SimRotaryAxis>& axes);

    void UpdateWindowScale(int width, int height);
    void UpdateCamera(const SoCamera& camera);

    void SetBackgroundColor(const vec3& c);
    void SetPathColor(const vec3& normal, const vec3& rapid);

protected:
    void InitDisplay(float quality);
    void GlsimStart();
    void GlsimToolStep1(void);
    void GlsimToolStep2(void);
    void GlsimClipBack(void);
    void GlsimRenderStock(void);
    void GlsimRenderTools(void);
    void GlsimEnd(void);
    void ToolPose(MillPathSegment* p, int step, vec3 pos, mat4x4 rmat);
    void renderSegmentForward(int iSeg);
    void renderSegmentReversed(int iSeg);
    void CalcSegmentPositions();
    void StepFromTime();
    void SkipTo(float time);
    float TimeOfStep(int step) const;
    void GetScenePose(quat pose);
    void UpdateScene();
    EndMill* GetTool(int tool);
    void RemoveTool(int toolId);

protected:
    bool simulationInitiated = false;

    // protected:
public:
    std::vector<EndMill*> mToolTable;
    GCodeParser mCodeParser;
    SimRotary mRotary {mCodeParser.Frames};
    SimDisplay simDisplay;
    MillPathLine millPathLine;
    std::vector<MillPathSegment*> MillPathSegments;
    SimTimeline mTimeline {MillPathSegments, mRotary, mCodeParser.Frames};
    std::vector<Point3D> mStockPoints;  // stock vertices, to find how far it reaches from an axis

    StockCache mStockCache;

    int mWidth = -1;
    int mHeight = -1;

    StockObject mStockObject;
    SolidObject mBaseShape;

    vec3 bgndColor = {0.1f, 0.2f, 0.3f};
    vec3 stockColor = {0.5f, 0.55f, 0.9f};
    vec3 cutColor = {0.5f, 0.84f, 0.73f};
    vec3 toolColor = {0.5f, 0.4f, 0.3f};
    vec3 baseShapeColor = {0.7f, 0.6f, 0.5f};
};

}  // namespace CAMSimulator
