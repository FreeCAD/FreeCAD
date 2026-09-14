// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>    *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <vector>

#include <QObject>
#include <QPointer>

#include <Base/Vector3D.h>
#include <Mod/Part/PartGlobal.h>

class QToolButton;
class QWidget;
class SoNodeSensor;
class SoSensor;

namespace Gui
{
class View3DInventorViewer;
}

namespace PartGui
{

class PartGuiExport PatternInstanceControls: public QObject
{
    Q_OBJECT

public:
    struct Instance
    {
        int index = -1;
        Base::Vector3d center;
        bool suppressed = false;
    };

    explicit PatternInstanceControls(Gui::View3DInventorViewer* viewer, QObject* parent = nullptr);
    ~PatternInstanceControls() override;

    void setViewer(Gui::View3DInventorViewer* viewer);
    void setInstances(const std::vector<Instance>& instances);
    void clear();

public Q_SLOTS:
    void updatePositions();

Q_SIGNALS:
    void toggleRequested(int index, bool suppress);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    struct ButtonInfo
    {
        Instance instance;
        QPointer<QToolButton> button;
    };

    void refreshHostWidget();
    void updateButton(ButtonInfo& info) const;
    void attachCameraSensor();
    void detachCameraSensor();
    static void cameraSensorCallback(void* data, SoSensor* sensor);

    QPointer<Gui::View3DInventorViewer> viewer;
    QPointer<QWidget> hostWidget;
    SoNodeSensor* cameraSensor = nullptr;
    std::vector<ButtonInfo> buttons;
};

}  // namespace PartGui
