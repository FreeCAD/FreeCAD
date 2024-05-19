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

// #include <memory>
// #include <TopoDS_Shape.hxx>

// #include <Mod/CAM/App/Command.h>
// #include <Mod/Part/App/TopoShape.h>
// #include <Mod/CAM/PathGlobal.h>
#include <QtGui/qwindow.h>
#include <QOpenGLExtraFunctions>
#include <QtGui/qpainter.h>
#include <QtGui/qopenglpaintdevice.h>


namespace MillSim
{
class MillSimulation;  // use short declaration as using 'include' causes a header loop
}

namespace CAMSimulator
{

struct SimStock
{
public:
    SimStock(float px, float py, float pz, float lx, float ly, float lz, float res);
    ~SimStock();

public:
    float mPx, mPy, mPz;  // stock zero position
    float mLx, mLy, mLz;  // stock dimensions
};

class DlgCAMSimulator: public QWindow, public QOpenGLExtraFunctions
{
    Q_OBJECT
public:
    explicit DlgCAMSimulator(QWindow* parent = nullptr);
    ~DlgCAMSimulator()
    {}

    virtual void render(QPainter* painter);
    virtual void render();
    virtual void initialize();

    void setAnimating(bool animating);
    static DlgCAMSimulator* GetInstance();

public:  // slots:
    void renderLater();
    void renderNow();
    void startSimulation(const SimStock* stock, float quality);
    void resetSimulation();
    void addGcodeCommand(const char* cmd);
    void addTool(const std::vector<float> toolProfilePoints,
                 int toolNumber,
                 float diameter,
                 float resolution);

protected:
    bool event(QEvent* event) override;

    void checkInitialization();
    void exposeEvent(QExposeEvent* event) override;
    void mouseMoveEvent(QMouseEvent* ev) override;
    void mousePressEvent(QMouseEvent* ev) override;
    void mouseReleaseEvent(QMouseEvent* ev) override;
    void wheelEvent(QWheelEvent* ev) override;
    void hideEvent(QHideEvent* ev) override;

private:
    bool mAnimating = false;
    bool mNeedsInitialize = false;

    QOpenGLContext* mContext = nullptr;
    QOpenGLPaintDevice* mDevice = nullptr;
    MillSim::MillSimulation* mMillSimulator = nullptr;
    static DlgCAMSimulator* mInstance;
    SimStock mStock = {0, 0, 0, 1, 1, 1, 1};
    float mQuality = 10;
};


}  // namespace CAMSimulator


#endif  // PATHSIMULATOR_PathSim_H
