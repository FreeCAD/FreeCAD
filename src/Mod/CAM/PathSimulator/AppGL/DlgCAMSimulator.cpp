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
#include <QtGui/QMatrix4x4>
#include <QtGui/qscreen.h>
#include "MillSimulation.h"
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
EndMillFlat endMillFlat02(2, 1.5f, 16);
EndMillBall endMillBall03(4, 1, 16, 4, 0.2f);
EndMillTaper endMillTaper04(3, 1, 16, 90, 0.2f);

MillSim::MillSimulation gMillSimulator;

QOpenGLContext *gOpenGlContext;

namespace CAMSimulator
{

    OpenGLWindow::OpenGLWindow(QWindow* parent)
        : QWindow(parent)
    {
        setSurfaceType(QWindow::OpenGLSurface);
    }

    void OpenGLWindow::render(QPainter* painter)
    {
        Q_UNUSED(painter);
    }

    void OpenGLWindow::initialize()
    {
        for (int i = 0; i < NUM_DEMO_MOTIONS; i++) {
            gMillSimulator.AddGcodeLine(demoCode[i]);
        }
        gMillSimulator.AddTool(&endMillFlat01);
        gMillSimulator.AddTool(&endMillFlat02);
        gMillSimulator.AddTool(&endMillBall03);
        gMillSimulator.AddTool(&endMillTaper04);
        gMillSimulator.InitSimulation();
        // gMillSimulator.SetBoxStock(0, 0, -8.7f, 50, 50, 8.7f);
        gMillSimulator.SetBoxStock(-20, -20, 0.001f, 50, 50, 2);
        gMillSimulator.InitDisplay();

        const qreal retinaScale = devicePixelRatio();
        glViewport(0, 0, width() * retinaScale, height() * retinaScale);
    
    }

    void OpenGLWindow::render()
    {
        gMillSimulator.ProcessSim((unsigned int)(QDateTime::currentMSecsSinceEpoch()));
    }

    void OpenGLWindow::renderLater()
    {
        requestUpdate();
    }

    bool OpenGLWindow::event(QEvent* event)
    {
        switch (event->type()) {
            case QEvent::UpdateRequest:
                renderNow();
                return true;
            default:
                return QWindow::event(event);
        }
    }

    void OpenGLWindow::exposeEvent(QExposeEvent* event)
    {
        Q_UNUSED(event);

        if (isExposed()) {
            renderNow();
        }
    }

    void OpenGLWindow::mouseMoveEvent(QMouseEvent* ev)
    {
        gMillSimulator.MouseMove(ev->x(), ev->y());
    }

    void OpenGLWindow::mousePressEvent(QMouseEvent* ev)
    {
        gMillSimulator.MousePress(ev->button(), true, ev->x(), ev->y());
    }

    void OpenGLWindow::mouseReleaseEvent(QMouseEvent* ev)
    {
        gMillSimulator.MousePress(ev->button(), false, ev->x(), ev->y());
    }

    void OpenGLWindow::hideEvent(QHideEvent* ev)
    {
        m_animating = false;
    }



    void OpenGLWindow::renderNow()
    {
        if (!isExposed()) {
            return;
        }

        bool needsInitialize = false;

        if (!m_context) {
            m_context = new QOpenGLContext(this);
            m_context->setFormat(requestedFormat());
            m_context->create();
            gOpenGlContext = m_context;
            needsInitialize = true;
        }

        m_context->makeCurrent(this);

        if (needsInitialize) {
            initializeOpenGLFunctions();
            initialize();
        }

        render();

        m_context->swapBuffers(this);

        if (m_animating) {
            renderLater();
        }
    }

    void OpenGLWindow::ShowWindow()
    {
        show();
        setAnimating(true);
    }

    void OpenGLWindow::setAnimating(bool animating)
    {
        m_animating = animating;

        if (animating) {
            renderLater();
        }
    }

    OpenGLWindow* OpenGLWindow::GetInstance()
    {
        if (mInstance == nullptr)
        {
            QSurfaceFormat format;
            format.setSamples(16);
            format.setSwapInterval(1);
            mInstance = new OpenGLWindow();
            mInstance->setFormat(format);
            mInstance->resize(800, 600);
            mInstance->show();
        }
        return mInstance;
    }

    OpenGLWindow* OpenGLWindow::mInstance = nullptr;

    //************************************************************************************************************
    // stock
    //************************************************************************************************************
    cStock::cStock(float px, float py, float pz, float lx, float ly, float lz, float res)
        : m_px(px), m_py(py), m_pz(pz), m_lx(lx), m_ly(ly), m_lz(lz)
    {}

    cStock::~cStock()
    {}

}  // namespace CAMSimulator