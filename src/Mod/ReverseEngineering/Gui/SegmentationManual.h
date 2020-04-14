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


#ifndef REVERSEENGINEERINGGUI_SEGMENTATIONMANUAL_H
#define REVERSEENGINEERINGGUI_SEGMENTATIONMANUAL_H

#include <QDialog>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <Mod/Mesh/Gui/MeshSelection.h>
#include <memory>

namespace ReverseEngineeringGui {
class Ui_SegmentationManual;

/**
 * Dialog to create segments from components, regions, the complete or single faces
 * of a mesh.
 * @author Werner Mayer
 */
class SegmentationManual : public QWidget
{
    Q_OBJECT

public:
    SegmentationManual(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~SegmentationManual();
    void reject();
    void createSegment();

public Q_SLOTS:
    void on_selectRegion_clicked();
    void on_selectAll_clicked();
    void on_selectComponents_clicked();
    void on_selectTriangle_clicked();
    void on_deselectAll_clicked();
    void on_visibleTriangles_toggled(bool);
    void on_screenTriangles_toggled(bool);
    void on_cbSelectComp_toggled(bool);

protected:
    void changeEvent(QEvent *e);

private:
    std::unique_ptr<Ui_SegmentationManual> ui;
    MeshGui::MeshSelection meshSel;
};

/**
 * Embed the panel into a task dialog.
 */
class TaskSegmentationManual : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskSegmentationManual();
    ~TaskSegmentationManual();

public:
    bool accept();
    void clicked(int);

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Close; }
    virtual bool isAllowedAlterDocument(void) const
    { return true; }
    virtual void modifyStandardButtons(QDialogButtonBox*);

private:
    SegmentationManual* widget;
    Gui::TaskView::TaskBox* taskbox;
};

}

#endif // REVERSEENGINEERINGGUI_SEGMENTATIONMANUAL_H
