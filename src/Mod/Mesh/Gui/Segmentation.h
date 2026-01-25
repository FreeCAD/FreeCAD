// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QWidget>

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#ifndef MESH_GLOBAL_H
# include <Mod/Mesh/MeshGlobal.h>
#endif


// forward declarations
namespace Mesh
{
class Feature;
}

namespace MeshGui
{
class Ui_Segmentation;

class MeshGuiExport Segmentation: public QWidget
{
public:
    explicit Segmentation(
        Mesh::Feature* mesh,
        QWidget* parent = nullptr,
        Qt::WindowFlags fl = Qt::WindowFlags()
    );
    ~Segmentation() override;
    void accept();

protected:
    void changeEvent(QEvent* e) override;

private:
    Ui_Segmentation* ui;
    Mesh::Feature* myMesh;

    Q_DISABLE_COPY_MOVE(Segmentation)
};

/**
 * Embed the panel into a task dialog.
 */
class TaskSegmentation: public Gui::TaskView::TaskDialog
{
public:
    explicit TaskSegmentation(Mesh::Feature* mesh);

public:
    bool accept() override;

    QDialogButtonBox::StandardButtons getStandardButtons() const override
    {
        return QDialogButtonBox::Ok | QDialogButtonBox::Cancel;
    }

private:
    Segmentation* widget;
};

}  // namespace MeshGui
