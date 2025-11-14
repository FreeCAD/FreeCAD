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
#include "MillSimulation.h"
#include "Gui/View3DInventorViewer.h"
#include <Mod/Part/App/BRepMesh.h>
#include <QDateTime>
#include <QSurfaceFormat>
#include <QPoint>
#include <App/Document.h>

using namespace MillSim;

namespace CAMSimulator
{

static const float MouseScrollDelta = 120.0F;

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

    mMillSimulator.reset(new MillSimulation());
}

DlgCAMSimulator::~DlgCAMSimulator()
{
    // nothing to do but need to keep this destructor because of forward declared MillSimulation
}

DlgCAMSimulator* DlgCAMSimulator::instance()
{
    if (mInstance == nullptr) {
        mInstance = new DlgCAMSimulator();
        mInstance->resize(MillSim::gWindowSizeW, MillSim::gWindowSizeH);
        mInstance->setWindowModality(Qt::ApplicationModal);
        mInstance->setMinimumWidth(700);
        mInstance->setMinimumHeight(400);
    }

    return mInstance;
}

void DlgCAMSimulator::setAnimating(bool animating)
{
    mAnimating = animating;

    if (mAnimating) {
        update();
    }
}

void DlgCAMSimulator::startSimulation(const Part::TopoShape& stock, float quality)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    setWindowTitle(tr("%1 - New CAM Simulator").arg(QString::fromUtf8(doc->getName())));

    mQuality = quality;
    mNeedsInitialize = true;

    setStockShape(stock, 1);

    show();
    setAnimating(true);
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

static SimShape getMeshData(const Part::TopoShape& tshape, float resolution)
{
    SimShape ret;

    std::vector<int> normalCount;
    int nVerts = 0;
    for (auto& shape : tshape.getSubTopoShapes(TopAbs_FACE)) {
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
}

void DlgCAMSimulator::setBaseShape(const Part::TopoShape& tshape, float resolution)
{
    mBase = getMeshData(tshape, resolution);
}

void DlgCAMSimulator::mouseMoveEvent(QMouseEvent* ev)
{
    int modifiers = (ev->modifiers() & Qt::ShiftModifier) != 0 ? MS_KBD_SHIFT : 0;
    modifiers |= (ev->modifiers() & Qt::ControlModifier) != 0 ? MS_KBD_CONTROL : 0;
    modifiers |= (ev->modifiers() & Qt::AltModifier) != 0 ? MS_KBD_ALT : 0;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint pnt = ev->pos();
#else
    QPoint pnt = ev->position().toPoint();
#endif

    const qreal ratio = devicePixelRatio();
    mMillSimulator->MouseMove(pnt.x() * ratio, pnt.y() * ratio, modifiers);

    update();
}

void DlgCAMSimulator::mousePressEvent(QMouseEvent* ev)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint pnt = ev->pos();
#else
    QPoint pnt = ev->position().toPoint();
#endif

    const qreal ratio = devicePixelRatio();
    mMillSimulator->MousePress(ev->button(), true, pnt.x() * ratio, pnt.y() * ratio);

    update();
}

void DlgCAMSimulator::mouseReleaseEvent(QMouseEvent* ev)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint pnt = ev->pos();
#else
    QPoint pnt = ev->position().toPoint();
#endif

    const qreal ratio = devicePixelRatio();
    mMillSimulator->MousePress(ev->button(), false, pnt.x() * ratio, pnt.y() * ratio);

    update();
}

void DlgCAMSimulator::wheelEvent(QWheelEvent* ev)
{
    mMillSimulator->MouseScroll((float)ev->angleDelta().y() / MouseScrollDelta);

    update();
}

void DlgCAMSimulator::updateResources()
{
    // clear simulator

    if (mNeedsClear) {
        mMillSimulator->Clear();
        mNeedsClear = false;
    }

    // update gcode

    for (const auto& cmd : mGCode) {
        mMillSimulator->AddGcodeLine(cmd.c_str());
    }

    mGCode.clear();

    // update tools

    for (const auto& tool : mTools) {
        if (!mMillSimulator->ToolExists(tool.id)) {
            mMillSimulator->AddTool(tool.profile, tool.id, tool.diameter);
        }
    }

    mTools.clear();

    // initialize simulator

    if (mNeedsInitialize) {
        mMillSimulator->InitSimulation(mQuality);
        mNeedsInitialize = false;
    }

    // update stock and base

    if (mStock.needsUpdate) {
        mMillSimulator->SetArbitraryStock(mStock.verts, mStock.indices);
        mStock = {};
    }

    if (mBase.needsUpdate) {
        mMillSimulator->SetBaseObject(mBase.verts, mBase.indices);
        mBase = {};
    }
}

void DlgCAMSimulator::initializeGL()
{
    initializeOpenGLFunctions();
}

void DlgCAMSimulator::paintGL()
{
    updateResources();
    mMillSimulator->ProcessSim((unsigned int)(QDateTime::currentMSecsSinceEpoch()));

    if (mAnimating) {
        update();
    }
}

void DlgCAMSimulator::resizeGL(int w, int h)
{
    const qreal ratio = devicePixelRatio();
    mMillSimulator->UpdateWindowScale(w * ratio, h * ratio);
}

DlgCAMSimulator* DlgCAMSimulator::mInstance = nullptr;

//************************************************************************************************************
// stock
//************************************************************************************************************

SimStock::SimStock(float px, float py, float pz, float lx, float ly, float lz, float res)
    : mPx(px)
    , mPy(py)
    , mPz(pz + 0.005 * lz)
    , mLx(lx)
    , mLy(ly)
    , mLz(1.01 * lz)
{
    (void)res;
}

}  // namespace CAMSimulator
