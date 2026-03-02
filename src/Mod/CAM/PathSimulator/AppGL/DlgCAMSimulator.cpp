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


#include "DlgCAMSimulator.h"

#include <queue>
#include <numeric>

#include <Gui/View3DInventorViewer.h>
#include <Mod/Part/App/BRepMesh.h>
#include <QDateTime>
#include <QSurfaceFormat>
#include <QPoint>
#include <QTimerEvent>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <QHBoxLayout>
#include <QPointer>
#include <Inventor/nodes/SoCamera.h>

#include "ViewCAMSimulator.h"
#include "MillSimulation.h"
#include "GuiDisplay.h"
#include "Dummy3DViewer.h"

using namespace std::literals;
using namespace CAMSimulator;

namespace CAMSimulator
{

float SimShape::maxDimension() const
{
    float xmin = NAN, ymin = NAN, zmin = NAN;
    float xmax = NAN, ymax = NAN, zmax = NAN;

    for (const auto& v : verts) {
        xmin = std::fmin(xmin, v.x);
        ymin = std::fmin(ymin, v.x);
        zmin = std::fmin(zmin, v.x);

        xmax = std::fmax(xmax, v.x);
        ymax = std::fmax(ymax, v.x);
        zmax = std::fmax(zmax, v.x);
    }

    const float xsize = xmax - xmin;
    const float ysize = ymax - ymin;
    const float zsize = zmax - zmin;

    return std::max(std::max(xsize, ysize), zsize);
}

QOpenGLExtraFunctions gOpenGLFunctions;

DlgCAMSimulator::DlgCAMSimulator(QWidget* parent)
    : QOpenGLWidget(parent)
{
    QSurfaceFormat format;
    format.setVersion(4, 1);                         // Request OpenGL 4.1 - for MacOS
    format.setProfile(QSurfaceFormat::CoreProfile);  // Use the core profile = for MacOS
    int samples = Gui::View3DInventorViewer::getNumSamples();
    if (samples > 1) {
        format.setSamples(samples);
    }
    format.setSwapInterval(2);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    setFormat(format);

    setMouseTracking(true);

    mMillSimulator.reset(new MillSimulation);
}

DlgCAMSimulator::~DlgCAMSimulator()
{
    makeCurrent();
    mMillSimulator = nullptr;
}

void DlgCAMSimulator::connectTo(GuiDisplay& gui, Dummy3DViewer& dv)
{
    // connect to gui

    mGui = &gui;
    updateGui();

    connect(&gui, &GuiDisplay::play, this, [this](bool b) { mMillSimulator->SetPlaying(b); });

    connect(&gui, &GuiDisplay::singleStep, this, [this] { mMillSimulator->SingleStep(); });

    connect(&gui, &GuiDisplay::speedChanged, this, [this](int speed) {
        mMillSimulator->SetSpeed(speed);
    });

    connect(&gui, &GuiDisplay::stageChanged, this, [this](float f) {
        mMillSimulator->SetSimulationStage(f);
    });

    connect(&gui, &GuiDisplay::rotateEnableChanged, this, &DlgCAMSimulator::setRotateEnabled);

    connect(&gui, &GuiDisplay::pathVisibleChanged, this, [this](bool b) {
        mMillSimulator->SetPathVisible(b);
    });

    connect(&gui, &GuiDisplay::ssaoEnableChanged, this, [this](bool b) {
        mMillSimulator->EnableSsao(b);
    });

    connect(&gui, &GuiDisplay::stockVisibleChanged, this, &DlgCAMSimulator::setStockVisible);
    connect(&gui, &GuiDisplay::baseVisibleChanged, this, &DlgCAMSimulator::setBaseVisible);

    // connect to dummy viewer

    mDummyViewer = &dv;

    mDummyViewer->setStockVisible(mMillSimulator->IsStockVisible());
    mDummyViewer->setBaseVisible(mMillSimulator->IsBaseVisible());

    // connect gui and dummy viewer

    connect(
        &gui,
        &GuiDisplay::viewAll,
        &dv,
        static_cast<void (Dummy3DViewer::*)()>(&Dummy3DViewer::viewAll)
    );
}

void DlgCAMSimulator::updateGui()
{
    if (!mGui) {
        return;
    }

    const auto state = mMillSimulator->GetState();

    mGui->setPlaying(state.mSimPlaying);
    mGui->setSpeed(state.mSimSpeed);

    const float stage = (float)state.mCurStep / state.mNTotalSteps;
    mGui->setStage(stage, state.mNTotalSteps);

    mGui->setStockVisible(state.mViewItems & VIEWITEM_SIMULATION);
    mGui->setBaseVisible(state.mViewItems & VIEWITEM_BASE_SHAPE);

    mGui->setPathVisible(state.mViewPath);
    mGui->setSsaoEnabled(state.mViewSSAO);

    if (mDummyViewer && !mDummyViewer->isAnimating()) {
        mGui->setRotateEnabled(false);
    }
}

void DlgCAMSimulator::cloneFrom(const DlgCAMSimulator& from)
{
    mNeedsInitialize = true;
    mNeedsClear = true;
    setAnimating(from.mAnimating);

    mQuality = from.mQuality;

    mGCode = from.mGCode;
    mTools = from.mTools;

    mStock = from.mStock;
    mStock.needsUpdate = true;

    mBase = from.mBase;
    mBase.needsUpdate = true;

    const auto state = from.mMillSimulator->GetState();
    mState = std::make_unique<CAMSimulator::MillSimulationState>(state);
}

DlgCAMSimulator* DlgCAMSimulator::instance()
{
    return &ViewCAMSimulator::instance().dlg();
}

void DlgCAMSimulator::setAnimating(bool animating)
{
    if (animating == mAnimating) {
        return;
    }

    mAnimating = animating;

    if (animating && mAnimatingTimer == 0) {
        mAnimatingTimer = startTimer(0);
    }
    else if (!animating && mAnimatingTimer != 0) {
        killTimer(mAnimatingTimer);
        mAnimatingTimer = 0;
    }
}

void DlgCAMSimulator::startSimulation(const Part::TopoShape& stock, float quality)
{
    mQuality = quality;
    mNeedsInitialize = true;

    setStockShape(stock, 1);
    setAnimating(true);

    Q_EMIT simulationStarted();
}

void DlgCAMSimulator::resetSimulation()
{
    mNeedsClear = true;

    mGCode.clear();
    mTools.clear();
    mStock = {};
    mBase = {};
}

void DlgCAMSimulator::addGcodeCommand(const char* cmd)
{
    mGCode.push_back(cmd);
}

void DlgCAMSimulator::addTool(
    const std::vector<float>& toolProfilePoints,
    int toolNumber,
    float diameter,
    float resolution
)
{
    Q_UNUSED(resolution)

    std::string toolCmd = "T" + std::to_string(toolNumber);
    addGcodeCommand(toolCmd.c_str());
    mTools.emplace_back(toolProfilePoints, toolNumber, diameter);
}

static SimShape getMeshData(const Part::TopoShape& shape, float resolution)
{
    SimShape ret;

    std::vector<int> normalCount;
    int nVerts = 0;
    for (auto& shape : shape.getSubTopoShapes(TopAbs_FACE)) {
        std::vector<Base::Vector3d> points;
        std::vector<Data::ComplexGeoData::Facet> facets;
        shape.getFaces(points, facets, resolution);

        std::vector<Base::Vector3d> normals(points.size());
        std::vector<int> normalCount(points.size());

        // copy triangle indices and calculate normals
        for (auto face : facets) {
            ret.indices.push_back(face.I1 + nVerts);
            ret.indices.push_back(face.I2 + nVerts);
            ret.indices.push_back(face.I3 + nVerts);

            // calculate normal
            Base::Vector3d vAB = points[face.I2] - points[face.I1];
            Base::Vector3d vAC = points[face.I3] - points[face.I1];
            Base::Vector3d vNorm = vAB.Cross(vAC).Normalize();

            normals[face.I1] += vNorm;
            normals[face.I2] += vNorm;
            normals[face.I3] += vNorm;

            normalCount[face.I1]++;
            normalCount[face.I2]++;
            normalCount[face.I3]++;
        }

        // copy points and set normals
        for (unsigned int i = 0; i < points.size(); i++) {
            Base::Vector3d& point = points[i];
            Base::Vector3d& normal = normals[i];
            int count = normalCount[i];
            normal /= count;
            ret.verts.push_back(Vertex(point.x, point.y, point.z, normal.x, normal.y, normal.z));
        }

        nVerts = ret.verts.size();
    }

    ret.needsUpdate = true;
    return ret;
}

void DlgCAMSimulator::setStockShape(const Part::TopoShape& shape, float resolution)
{
    mStock = getMeshData(shape, resolution);

    if (mDummyViewer) {
        mDummyViewer->setStockShape(shape);
    }

    update();
}

void DlgCAMSimulator::setStockVisible(bool b)
{
    if (b == mMillSimulator->IsStockVisible()) {
        return;
    }

    mMillSimulator->SetStockVisible(b);

    if (mDummyViewer) {
        mDummyViewer->setStockVisible(b);
    }

    update();
}

void DlgCAMSimulator::setBaseShape(const Part::TopoShape& shape, float resolution)
{
    mBase = getMeshData(shape, resolution);

    if (mDummyViewer) {
        mDummyViewer->setBaseShape(shape);
    }

    update();
}

void DlgCAMSimulator::setBaseVisible(bool b)
{
    if (b == mMillSimulator->IsBaseVisible()) {
        return;
    }

    mMillSimulator->SetBaseVisible(b);

    if (mDummyViewer) {
        mDummyViewer->setBaseVisible(b);
    }

    update();
}

// this is very similar to DemoMode::getDirection in Gui/DemoMode.cpp

static SbVec3f getRotationDirection(Gui::View3DInventorViewer* viewer)
{
    const SbVec3f viewAxis = {0, 0, -1};

    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam) {
        return viewAxis;
    }
    SbRotation rot = cam->orientation.getValue();
    SbRotation inv = rot.inverse();
    SbVec3f vec(viewAxis);
    inv.multVec(vec, vec);
    if (vec.length() < std::numeric_limits<float>::epsilon()) {
        vec = viewAxis;
    }
    vec.normalize();
    return vec;
}

void DlgCAMSimulator::setRotateEnabled(bool b)
{
    if (b) {
        mDummyViewer->startSpinningAnimation(getRotationDirection(mDummyViewer), 0.5f);
    }
    else {
        mDummyViewer->stopAnimating();
    }
}

void DlgCAMSimulator::setBackgroundColor(const QColor& c)
{
    mMillSimulator->SetBackgroundColor({c.redF(), c.greenF(), c.blueF()});
    update();
}

void DlgCAMSimulator::setPathColor(const QColor& normal, const QColor& rapid)
{
    const vec3 vnormal = {normal.redF(), normal.greenF(), normal.blueF()};
    const vec3 vrapid = {rapid.redF(), rapid.greenF(), rapid.blueF()};
    mMillSimulator->SetPathColor(vnormal, vrapid);
}

void DlgCAMSimulator::timerEvent(QTimerEvent* event)
{
    (void)event;

    update();

    // TODO: keep things simple for now, should probably only update gui if something changed

    updateGui();
}

void DlgCAMSimulator::updateResources()
{
    // clear simulator

    if (mNeedsClear) {
        mMillSimulator->Clear();
        mLastGCode = 0;
        mNeedsClear = false;
    }

    // update gcode

    for (int i = mLastGCode; i < (int)mGCode.size(); i++) {
        const std::string& cmd = mGCode[i];
        mMillSimulator->AddGcodeLine(cmd.c_str());
    }

    mLastGCode = mGCode.size();

    // update tools

    for (const auto& tool : mTools) {
        if (!mMillSimulator->ToolExists(tool.id)) {
            mMillSimulator->AddTool(tool.profile, tool.id, tool.diameter);
        }
    }

    // initialize simulator

    if (mNeedsInitialize) {
        // TODO: for now we just hope this doesn't change afterwards...
        const float maxStockDimension = mStock.maxDimension();

        mMillSimulator->InitSimulation(mQuality, maxStockDimension);
        mNeedsInitialize = false;

        mLastProcessSim = clock::time_point::min();
    }

    // update stock and base

    if (mStock.needsUpdate) {
        mMillSimulator->SetArbitraryStock(mStock.verts, mStock.indices);
        mStock.needsUpdate = false;
    }

    if (mBase.needsUpdate) {
        mMillSimulator->SetBaseObject(mBase.verts, mBase.indices);
        mBase.needsUpdate = false;
    }

    // update state

    if (mState) {
        // TODO: update gui and dummy viewer?

        mMillSimulator->SetState(*mState);
        mState = nullptr;
    }
}

void DlgCAMSimulator::updateWindowScale()
{
    const qreal ratio = devicePixelRatioF();
    mMillSimulator->UpdateWindowScale(width() * ratio, height() * ratio);
}

void DlgCAMSimulator::updateCamera()
{
    if (!mDummyViewer) {
        return;
    }

    const SoCamera& camera = *mDummyViewer->getCamera();
    mMillSimulator->UpdateCamera(camera);
}

void DlgCAMSimulator::initializeGL()
{
    gOpenGLFunctions.initializeOpenGLFunctions();
}

void DlgCAMSimulator::paintGL()
{
    updateResources();

    // We need to call updateWindowScale on every render since the devicePixelRatio we get in
    // resizeGL might be wrong on the first resize.

    updateWindowScale();
    updateCamera();

    const auto now = clock::now();
    const auto elapsed = mLastProcessSim != clock::time_point::min() ? now - mLastProcessSim : 0s;

#if 0

    static std::deque<clock::duration> q;
    while (q.size() >= 60) {
        q.pop_front();
    }

    q.push_back(elapsed);

    const auto average = std::accumulate(q.begin(), q.end(), clock::duration()) / (float)q.size();

    std::cerr
        << "elapsed: "
        << std::chrono::duration_cast<std::chrono::duration<float, std::milli>>(average).count()
        << "ms" << std::endl;

#endif


    mMillSimulator->ProcessSim(elapsed);

    mLastProcessSim = now;
}

void DlgCAMSimulator::resizeGL(int w, int h)
{
    (void)w, (void)h;
}

}  // namespace CAMSimulator
