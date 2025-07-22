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

#include "PreCompiled.h"

#include "DlgCAMSimulator.h"
#include "MillSimulation.h"
#include "Gui/View3DInventorViewer.h"
#include <Mod/Part/App/BRepMesh.h>
#include <QDateTime>
#include <QSurfaceFormat>
#include <QPoint>
#include <App/Document.h>

QOpenGLContext* gOpenGlContext;

using namespace MillSim;

namespace CAMSimulator
{

static const float MouseScrollDelta = 120.0F;

DlgCAMSimulator::DlgCAMSimulator(QWindow* parent)
    : QWindow(parent)
{
    setSurfaceType(QWindow::OpenGLSurface);
    mMillSimulator = new MillSimulation();
}

DlgCAMSimulator::~DlgCAMSimulator()
{
    delete mMillSimulator;
}

void DlgCAMSimulator::render(QPainter* painter)
{
    Q_UNUSED(painter);
}

void DlgCAMSimulator::render()
{
    mMillSimulator->ProcessSim((unsigned int)(QDateTime::currentMSecsSinceEpoch()));
}

void DlgCAMSimulator::renderLater()
{
    requestUpdate();
}

bool DlgCAMSimulator::event(QEvent* event)
{
    switch (event->type()) {
        case QEvent::UpdateRequest:
            renderNow();
            return true;
        default:
            break;
    }
    return QWindow::event(event);
}

void DlgCAMSimulator::exposeEvent(QExposeEvent* event)
{
    Q_UNUSED(event);

    if (isExposed()) {
        renderNow();
    }
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
    mMillSimulator->MouseMove(pnt.x(), pnt.y(), modifiers);
}

void DlgCAMSimulator::mousePressEvent(QMouseEvent* ev)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint pnt = ev->pos();
#else
    QPoint pnt = ev->position().toPoint();
#endif
    mMillSimulator->MousePress(ev->button(), true, pnt.x(), pnt.y());
}

void DlgCAMSimulator::mouseReleaseEvent(QMouseEvent* ev)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QPoint pnt = ev->pos();
#else
    QPoint pnt = ev->position().toPoint();
#endif
    mMillSimulator->MousePress(ev->button(), false, pnt.x(), pnt.y());
}

void DlgCAMSimulator::wheelEvent(QWheelEvent* ev)
{
    mMillSimulator->MouseScroll((float)ev->angleDelta().y() / MouseScrollDelta);
}

void DlgCAMSimulator::resetSimulation()
{}

void DlgCAMSimulator::addGcodeCommand(const char* cmd)
{
    mMillSimulator->AddGcodeLine(cmd);
}

void DlgCAMSimulator::addTool(const std::vector<float>& toolProfilePoints,
                              int toolNumber,
                              float diameter,
                              float resolution)
{
    Q_UNUSED(resolution)
    std::string toolCmd = "T" + std::to_string(toolNumber);
    mMillSimulator->AddGcodeLine(toolCmd.c_str());
    if (!mMillSimulator->ToolExists(toolNumber)) {
        mMillSimulator->AddTool(toolProfilePoints, toolNumber, diameter);
    }
}

void DlgCAMSimulator::hideEvent(QHideEvent* ev)
{
    mMillSimulator->Clear();
    doGlCleanup();
    mAnimating = false;
    QWindow::hideEvent(ev);
    close();
    mInstance = nullptr;
}

void DlgCAMSimulator::resizeEvent(QResizeEvent* event)
{
    if (!mContext) {
        return;
    }
    QSize newSize = event->size();
    int newWidth = newSize.width();
    int newHeight = newSize.height();
    if (mMillSimulator != nullptr) {
        mMillSimulator->UpdateWindowScale(newWidth, newHeight);
    }
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, (int)(newWidth * retinaScale), (int)(newHeight * retinaScale));
}

void DlgCAMSimulator::GetMeshData(const Part::TopoShape& tshape,
                                  float resolution,
                                  std::vector<Vertex>& verts,
                                  std::vector<GLushort>& indices)
{
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
            indices.push_back(face.I1 + nVerts);
            indices.push_back(face.I2 + nVerts);
            indices.push_back(face.I3 + nVerts);

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
            verts.push_back(Vertex(point.x, point.y, point.z, normal.x, normal.y, normal.z));
        }

        nVerts = verts.size();
    }
}

void DlgCAMSimulator::startSimulation(const Part::TopoShape& stock, float quality)
{
    mQuality = quality;
    mNeedsInitialize = true;
    show();
    checkInitialization();
    SetStockShape(stock, 1);
    setAnimating(true);
}

void DlgCAMSimulator::initialize()
{
    mMillSimulator->InitSimulation(mQuality);

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
}

void DlgCAMSimulator::checkInitialization()
{
    if (!mContext) {
        mLastContext = QOpenGLContext::currentContext();
        mContext = new QOpenGLContext(this);
        mContext->setFormat(requestedFormat());
        mContext->create();
        QSurfaceFormat format;
        format.setSamples(16);
        format.setSwapInterval(2);
        mContext->setFormat(format);
        gOpenGlContext = mContext;
        mNeedsInitialize = true;
    }

    mContext->makeCurrent(this);

    if (mNeedsInitialize) {
        initializeOpenGLFunctions();
        initialize();
        mNeedsInitialize = false;
    }
}

void DlgCAMSimulator::doGlCleanup()
{
    if (mLastContext != nullptr) {
        mLastContext->makeCurrent(this);
    }
    if (mContext != nullptr) {
        mContext->deleteLater();
        mContext = nullptr;
    }
}

void DlgCAMSimulator::renderNow()
{
    static unsigned int lastTime = 0;
    static int frameCount = 0;
    static int fps = 0;
    if (!isExposed()) {
        return;
    }

    checkInitialization();

    frameCount++;
    unsigned int curtime = QDateTime::currentMSecsSinceEpoch();
    unsigned int timediff = curtime - lastTime;
    if (timediff > 10000) {
        fps = frameCount * 1000 / timediff;  // for debug only. not used otherwise.
        lastTime = curtime;
        frameCount = 0;
    }
    render();
    mContext->swapBuffers(this);

    if (mAnimating) {
        renderLater();
    }
    (void)fps;
}

void DlgCAMSimulator::setAnimating(bool animating)
{
    mAnimating = animating;

    if (animating) {
        renderLater();
    }
}

DlgCAMSimulator* DlgCAMSimulator::GetInstance()
{
    if (mInstance == nullptr) {
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
        mInstance = new DlgCAMSimulator();
        mInstance->setFormat(format);
        mInstance->resize(MillSim::gWindowSizeW, MillSim::gWindowSizeH);
        mInstance->setModality(Qt::ApplicationModal);
        mInstance->setMinimumWidth(700);
        mInstance->setMinimumHeight(400);

        App::Document* doc = App::GetApplication().getActiveDocument();
        mInstance->setTitle(tr("%1 - New CAM Simulator").arg(QString::fromUtf8(doc->getName())));
    }
    return mInstance;
}

void DlgCAMSimulator::SetStockShape(const Part::TopoShape& shape, float resolution)
{
    std::vector<Vertex> verts;
    std::vector<GLushort> indices;
    GetMeshData(shape, resolution, verts, indices);
    mMillSimulator->SetArbitraryStock(verts, indices);
}

void DlgCAMSimulator::SetBaseShape(const Part::TopoShape& tshape, float resolution)
{
    std::vector<Vertex> verts;
    std::vector<GLushort> indices;
    GetMeshData(tshape, resolution, verts, indices);
    mMillSimulator->SetBaseObject(verts, indices);
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

SimStock::~SimStock()
{}

}  // namespace CAMSimulator
