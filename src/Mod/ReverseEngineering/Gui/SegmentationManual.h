// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#pragma once

#include <memory>

#include <QDialog>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Mesh/Gui/MeshSelection.h>


namespace ReverseEngineeringGui
{
class Ui_SegmentationManual;

/**
 * Dialog to create segments from components, regions, the complete or single faces
 * of a mesh.
 * @author Werner Mayer
 */
class SegmentationManual: public QWidget
{
    Q_OBJECT

public:
    explicit SegmentationManual(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~SegmentationManual() override;
    void reject();
    void createSegment();

public:
    void setupConnections();
    void onSelectRegionClicked();
    void onSelectAllClicked();
    void onSelectComponentsClicked();
    void onSelectTriangleClicked();
    void onDeselectAllClicked();
    void onVisibleTrianglesToggled(bool);
    void onScreenTrianglesToggled(bool);
    void onSelectCompToggled(bool);
    void onPlaneDetectClicked();
    void onCylinderDetectClicked();
    void onSphereDetectClicked();

protected:
    void changeEvent(QEvent* e) override;

private:
    class Private;

private:
    std::unique_ptr<Ui_SegmentationManual> ui;
    MeshGui::MeshSelection meshSel;
};

/**
 * Embed the panel into a task dialog.
 */
class TaskSegmentationManual: public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskSegmentationManual();

public:
    bool accept() override;
    void clicked(int) override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Close;
    }
    bool isAllowedAlterDocument() const override
    {
        return true;
    }
    void modifyStandardButtons(QDialogButtonBox*) override;

private:
    SegmentationManual* widget;
};

}  // namespace ReverseEngineeringGui
