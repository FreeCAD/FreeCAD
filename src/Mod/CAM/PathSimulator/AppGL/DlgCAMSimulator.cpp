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
#include <QtGui/QMatrix4x4>
#include <QtGui/qscreen.h>
#include <QDateTime>
#include <QSurfaceFormat>
#include <QMouseEvent>
#include <QPoint>

using namespace CAMSimulator;
using namespace MillSim;

const char* demoCode[] = {
    "T2",
    "G0 X-0.7 Y-0.7 Z10",
    "G0 X-0.7 Y-0.7 Z1",
    "G0 X0.7 Y0.7 Z1 I0.7 J0.7 K0",
    "G0 X0.7 Y0.7 Z10",

    "G0 X-3 Y-3 Z10",
    "G0 X-3 Y-3 Z0.5",
    "G3 X3 Y3 Z0.5 I3 J3 K0",
    "G0 X3 Y3 Z10",

    "G0 X15 Y15 Z10",
    "G0 X 15 Y15 Z1.5",
    "G0 X15 Y-15 Z1.5",
    "G0 X-15 Y-15 Z1.5",
    "G0 X-15 Y15 Z1.5",
    "G0 X15 Y15 Z1.5",

    "G0 X15 Y15 Z1",
    "G0 X15 Y-15 Z1",
    "G0 X-15 Y-15 Z1",
    "G0 X-15 Y15 Z1",
    "G0 X15 Y15 Z1",

    "G0 X15 Y15 Z0.5",
    "G0 X15 Y-15 Z0.5",
    "G0 X-15 Y-15 Z0.5",
    "G0 X-15 Y15 Z0.5",
    "G0 X15 Y15 Z0.5",

    "G0 X15 Y15 Z0",
    "G0 X15 Y-15 Z0",
    "G0 X-15 Y-15 Z0",
    "G0 X-15 Y15 Z0",
    "G0 X15 Y15 Z0",

    "G0 X15 Y15 Z10",

    "T1",
    "G0 X8 Y8 Z10",
    "G0 X8 Y8 Z1.5",
    "G0 X8 Y-8 Z1.5",
    "G0 X6.1 Y-8 Z1.5",
    "G0 X6.1 Y8 Z1.5",
    "G0 X4.2 Y8 Z1.5",
    "G0 X4.2 Y-8 Z1.5",
    "G0 X2.3 Y-8 Z1.5",
    "G0 X2.3 Y8 Z1.5",
    "G0 X0.4 Y8 Z1.5",
    "G0 X0.4 Y-8 Z1.5",
    "G0 X-1.5 Y-8 Z1.5",
    "G0 X-1.5 Y8 Z1.5",
    "G0 X-3.4 Y8 Z1.5",
    "G0 X-3.4 Y-8 Z1.5",
    "G0 X-5.3 Y-8 Z1.5",
    "G0 X-5.3 Y8 Z1.5",
    "G0 X-7.2 Y8 Z1.5",
    "G0 X-7.2 Y-8 Z1.5",
    "G0 X-8 Y-8 Z1.5",
    "G0 X-8 Y8 Z1.5",
    "G0 X 8 Y8 Z1.5",
    "G0 X 8 Y-8 Z1.5",
    "G0 X-8 Y-8 Z1.5",

    "G0 X-8 Y-8 Z10",

    // taper mill motion
    "T3",
    "G0 X14.2 Y14.2 Z10",
    "G0 X14.2 Y14.2 Z1.5",
    "G0 X14.2 Y-14.2 Z1.5",
    "G0 X-14.2 Y-14.2 Z1.5",
    "G0 X-14.2 Y14.2 Z1.5",
    "G0 X14.2 Y14.2 Z1.5",
    "G0 X14.2 Y14.2 Z10",
    "G0 X0 Y0 Z10",

    // ball mill motion
    "T4",
    "G0 X12 Y12 Z10",
    "G0 X12 Y12 Z1.5",
    "G0 X12 Y-12 Z2.5",
    "G0 X-12 Y-12 Z1.5",
    "G0 X-12 Y12 Z2.5",
    "G0 X12 Y12 Z1.5",
    "G0 X12 Y12 Z10",
    "G0 X0 Y0 Z10",
};

#define NUM_DEMO_MOTIONS (sizeof(demoCode) / sizeof(char*))

EndMillFlat endMillFlat01(1, 3.175f, 16);
EndMillFlat endMillFlat12(5, 12.0f, 16);
EndMillFlat endMillFlat02(2, 1.5f, 16);
EndMillBall endMillBall03(4, 1, 16, 4, 0.2f);
EndMillTaper endMillTaper04(3, 1, 16, 90, 0.2f);

QOpenGLContext *gOpenGlContext;

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

    void DlgCAMSimulator::ResetSimulation()
    {
        mMillSimulator->Clear();
        //for (int i = 0; i < NUM_DEMO_MOTIONS; i++) {
        //    mMillSimulator->AddGcodeLine(demoCode[i]);
        //}
        mMillSimulator->AddGcodeLine("T5");
        mMillSimulator->AddTool(&endMillFlat01);
        mMillSimulator->AddTool(&endMillFlat02);
        mMillSimulator->AddTool(&endMillBall03);
        mMillSimulator->AddTool(&endMillTaper04);
        mMillSimulator->AddTool(&endMillFlat12);
    }

    void DlgCAMSimulator::AddGcodeCommand(const char* cmd)
    {
        mMillSimulator->AddGcodeLine(cmd);
    }

    void DlgCAMSimulator::hideEvent(QHideEvent* ev)
    {
        mAnimating = false;
    }

    void DlgCAMSimulator::StartSimulation(const cStock* stock)
    {
        mStock = *stock;
        mNeedsInitialize = true;
        show();
        setAnimating(true);
    }

    void DlgCAMSimulator::initialize()
    {
        mMillSimulator->InitSimulation();
        // gMillSimulator->SetBoxStock(0, 0, -8.7f, 50, 50, 8.7f);
        mMillSimulator->SetBoxStock(mStock.mPx, mStock.mPy, mStock.mPz, mStock.mLx, mStock.mLy, mStock.mLz);
        mMillSimulator->InitDisplay();

        const qreal retinaScale = devicePixelRatio();
        glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    }

    void DlgCAMSimulator::CheckInitialization()
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
        if (!isExposed()) {
            return;
        }

        CheckInitialization();

        render();

        mContext->swapBuffers(this);

        if (mAnimating) {
            renderLater();
        }
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
        if (mInstance == nullptr)
        {
            QSurfaceFormat format;
            format.setSamples(16);
            format.setSwapInterval(1);
            mInstance = new DlgCAMSimulator();
            mInstance->setFormat(format);
            mInstance->resize(800, 600);
            mInstance->show();
        }
        return mInstance;
    }

    DlgCAMSimulator* DlgCAMSimulator::mInstance = nullptr;

    //************************************************************************************************************
    // stock
    //************************************************************************************************************
    cStock::cStock(float px, float py, float pz, float lx, float ly, float lz, float res)
        : mPx(px), mPy(py), mPz(pz + 0.005 * lz), mLx(lx), mLy(ly), mLz(1.01 * lz)
    {}

    cStock::~cStock()
    {}

}  // namespace CAMSimulator