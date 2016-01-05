/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PARTGUI_DLGEXTRUSION_H
#define PARTGUI_DLGEXTRUSION_H

#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include <string>

class TopoDS_Shape;

namespace PartGui {

class Ui_DlgExtrusion;
class DlgExtrusion : public QDialog
{
    Q_OBJECT

public:
    DlgExtrusion(QWidget* parent = 0, Qt::WindowFlags fl = 0);
    ~DlgExtrusion();
    void accept();
    void apply();

protected:
    void findShapes();
    bool canExtrude(const TopoDS_Shape&) const;
    void changeEvent(QEvent *e);

private Q_SLOTS:
    void on_checkNormal_toggled(bool);

private:
    Ui_DlgExtrusion* ui;
    std::string document, label;
};

class TaskExtrusion : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskExtrusion();
    ~TaskExtrusion();

public:
    bool accept();
    bool reject();
    void clicked(int);

    virtual QDialogButtonBox::StandardButtons getStandardButtons() const
    { return QDialogButtonBox::Ok | QDialogButtonBox::Apply | QDialogButtonBox::Close; }

private:
    DlgExtrusion* widget;
    Gui::TaskView::TaskBox* taskbox;
};

} // namespace PartGui

#endif // PARTGUI_DLGEXTRUSION_H
