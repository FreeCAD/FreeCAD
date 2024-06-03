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
#include <QDateTime>
#include <QSurfaceFormat>
#include <QPoint>

QOpenGLContext* gOpenGlContext;

using namespace MillSim;

namespace CAMSimulator
{

DlgCAMSimulator::DlgCAMSimulator(QWindow* parent)
    : QWindow(parent)
{
    setSurfaceType(QWindow::OpenGLSurface);
    mMillSimulator = new MillSimulation();
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
            return QWindow::event(event);
    }
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
    mMillSimulator->MouseMove(ev->x(), ev->y());
}

void DlgCAMSimulator::mousePressEvent(QMouseEvent* ev)
{
    mMillSimulator->MousePress(ev->button(), true, ev->x(), ev->y());
}

void DlgCAMSimulator::mouseReleaseEvent(QMouseEvent* ev)
{
    mMillSimulator->MousePress(ev->button(), false, ev->x(), ev->y());
}

void DlgCAMSimulator::wheelEvent(QWheelEvent* ev)
{
    mMillSimulator->MouseScroll((float)ev->angleDelta().y() / 120.0f);
}

void DlgCAMSimulator::resetSimulation()
{
    mMillSimulator->Clear();
}

void DlgCAMSimulator::addGcodeCommand(const char* cmd)
{
    mMillSimulator->AddGcodeLine(cmd);
}

void DlgCAMSimulator::addTool(const std::vector<float> toolProfilePoints,
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
    Q_UNUSED(ev)
    mAnimating = false;
}

void DlgCAMSimulator::startSimulation(const SimStock* stock, float quality)
{
    mStock = *stock;
    mQuality = quality;
    mNeedsInitialize = true;
    show();
    setAnimating(true);
}

void DlgCAMSimulator::initialize()
{
    mMillSimulator
        ->SetBoxStock(mStock.mPx, mStock.mPy, mStock.mPz, mStock.mLx, mStock.mLy, mStock.mLz);
    mMillSimulator->InitSimulation(mQuality);

    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
}

void DlgCAMSimulator::checkInitialization()
{
    if (!mContext) {
        mContext = new QOpenGLContext(this);
        mContext->setFormat(requestedFormat());
        mContext->create();
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
        format.setSamples(16);
        format.setSwapInterval(2);
        format.setDepthBufferSize(24);
        format.setStencilBufferSize(8);
        mInstance = new DlgCAMSimulator();
        mInstance->setFormat(format);
        mInstance->resize(800, 600);
        mInstance->setModality(Qt::ApplicationModal);
        mInstance->show();
    }
    return mInstance;
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
