// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2017 Shai Seger <shaise at gmail>                       *
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

#ifndef PATHSIMULATOR_CAMSimulatorGui_H
#define PATHSIMULATOR_CAMSimulatorGui_H

#ifdef _MSC_VER
# pragma warning(disable : 4251)
#endif

#include <queue>
#include <functional>
#include <chrono>

#include <QOpenGLWidget>
#include <QPainter>
#include <QTimer>
#include <QExposeEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QOpenGLContext>

#include <Mod/Part/App/TopoShape.h>

class SoCamera;

namespace Gui
{
class MDIView;
}

namespace CAMSimulator
{

// use short declaration as using 'include' causes a header loop
class MillSimulation;
class MillSimulationState;
struct Vertex;
class ViewCAMSimulator;
class GuiDisplay;
class Dummy3DViewer;

struct SimShape
{
public:
    float maxDimension() const;

public:
    std::vector<Vertex> verts;
    std::vector<GLushort> indices;
    bool needsUpdate = false;
};

struct SimTool
{
public:
    std::vector<float> profile;
    int id;
    float diameter;
    float resolution;
};

class DlgCAMSimulator: public QOpenGLWidget
{
    Q_OBJECT

    typedef std::chrono::steady_clock clock;

public:
    explicit DlgCAMSimulator(QWidget* parent = nullptr);
    ~DlgCAMSimulator() override;

    void connectTo(GuiDisplay& gui, Dummy3DViewer& dv);
    void cloneFrom(const DlgCAMSimulator& from);

    static DlgCAMSimulator* instance();

    void setAnimating(bool animating);
    void startSimulation(const Part::TopoShape& stock, float quality);
    void resetSimulation();

    void addGcodeCommand(const char* cmd);
    void addTool(
        const std::vector<float>& toolProfilePoints,
        int toolNumber,
        float diameter,
        float resolution
    );

    void setStockShape(const Part::TopoShape& shape, float resolution);
    void setStockVisible(bool b);
    void setBaseShape(const Part::TopoShape& shape, float resolution);
    void setBaseVisible(bool b);

    void setRotateEnabled(bool b);

    void setBackgroundColor(const QColor& c);
    void setPathColor(const QColor& normal, const QColor& rapid);

Q_SIGNALS:
    void simulationStarted();

protected:
    void timerEvent(QTimerEvent* event) override;

    void updateResources();
    void updateWindowScale();
    void updateCamera();

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

    void updateGui();

private:
    bool mNeedsInitialize = false;
    bool mNeedsClear = false;
    bool mAnimating = false;
    int mAnimatingTimer = 0;

    std::unique_ptr<MillSimulation> mMillSimulator;
    float mQuality = 10;

    std::vector<std::string> mGCode;
    std::size_t mLastGCode = 0;

    std::vector<SimTool> mTools;

    const SoCamera* mCamera = nullptr;
    SimShape mStock;
    SimShape mBase;

    std::unique_ptr<MillSimulationState> mState;
    clock::time_point mLastProcessSim = clock::time_point::min();

    GuiDisplay* mGui = nullptr;
    Dummy3DViewer* mDummyViewer = nullptr;
};

}  // namespace CAMSimulator

#endif  // PATHSIMULATOR_PathSim_H
