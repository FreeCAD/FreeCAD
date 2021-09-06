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


#ifndef MESHGUI_SEGMENTATION_H
#define MESHGUI_SEGMENTATION_H

#include <QWidget>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>

// forward declarations
namespace Mesh { class Feature; }

namespace MeshGui {
class Ui_Segmentation;

class MeshGuiExport Segmentation : public QWidget
{
public:
    Segmentation(Mesh::Feature* mesh, QWidget* parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
    ~Segmentation();
    void accept();

protected:
    void changeEvent(QEvent *e);

private:
    Ui_Segmentation* ui;
    Mesh::Feature* myMesh;
};

/**
 * Embed the panel into a task dialog.
 */
class TaskSegmentation : public Gui::TaskView::TaskDialog
{
public:
    TaskSegmentation(Mesh::Feature* mesh);
    ~TaskSegmentation();

public:
    bool accept();

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Cancel; }

private:
    Segmentation* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}

#endif // MESHGUI_SEGMENTATION_H
