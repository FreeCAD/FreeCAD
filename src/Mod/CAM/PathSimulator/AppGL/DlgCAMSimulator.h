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
#pragma warning(disable : 4251)
#endif

#include <queue>
#include <functional>

#include <Mod/Part/App/TopoShape.h>
#include <QOpenGLWidget>
#include <QOpenGLExtraFunctions>
#include <QPainter>
#include <QTimer>
#include <QExposeEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QOpenGLContext>

class SoCamera;

namespace MillSim
{
// use short declaration as using 'include' causes a header loop
class MillSimulation;
class MillSimulationState;
struct Vertex;
}  // namespace MillSim

namespace Gui
{
class MDIView;
}

namespace CAMSimulator
{

class ViewCAMSimulator;

struct SimShape
{
public:
    float maxDimension() const;

public:
    std::vector<MillSim::Vertex> verts;
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

class DlgCAMSimulator: public QOpenGLWidget, public QOpenGLExtraFunctions
{
    Q_OBJECT

public:
    explicit DlgCAMSimulator(ViewCAMSimulator& view, QWidget* parent = nullptr);
    ~DlgCAMSimulator() override;

    void cloneFrom(const DlgCAMSimulator& from);

    static DlgCAMSimulator* instance();

    void setCamera(const SoCamera& camera);
    void setAnimating(bool animating);
    void startSimulation(const Part::TopoShape& stock, float quality);
    void resetSimulation();

    void addGcodeCommand(const char* cmd);
    void addTool(const std::vector<float>& toolProfilePoints,
                 int toolNumber,
                 float diameter,
                 float resolution);

    void setStockShape(const Part::TopoShape& shape, float resolution);
    void setBaseShape(const Part::TopoShape& shape, float resolution);

Q_SIGNALS:
    void stockChanged(const Part::TopoShape& shape);
    void baseChanged(const Part::TopoShape& shape);

protected:
    void timerEvent(QTimerEvent* event) override;

    void updateResources();
    void updateWindowScale();
    void updateCamera();

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    bool mNeedsInitialize = false;
    bool mNeedsClear = false;
    bool mAnimating = false;
    int mAnimatingTimer = 0;

    std::unique_ptr<MillSim::MillSimulation> mMillSimulator;
    float mQuality = 10;

    std::vector<std::string> mGCode;
    std::size_t mLastGCode = 0;

    std::vector<SimTool> mTools;

    SimShape mStock;
    SimShape mBase;

    const SoCamera* mCamera = nullptr;
    ViewCAMSimulator& mView;

    std::unique_ptr<MillSim::MillSimulationState> mState;
};

}  // namespace CAMSimulator

#endif  // PATHSIMULATOR_PathSim_H
