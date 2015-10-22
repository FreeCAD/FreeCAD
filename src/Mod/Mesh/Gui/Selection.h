/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef MESHGUI_SELECTION_H
#define MESHGUI_SELECTION_H

#include <vector>
#include <QWidget>
#include <Inventor/nodes/SoEventCallback.h>
#include <Gui/SelectionObject.h>
#include <Gui/TaskView/TaskDialog.h>
#include <Gui/TaskView/TaskView.h>
#include "MeshSelection.h"

namespace MeshGui {

class Ui_Selection;
class Selection : public QWidget
{
    Q_OBJECT

public:
    Selection(QWidget* parent = 0);
    ~Selection();
    void setObjects(const std::vector<Gui::SelectionObject>&);
    std::vector<App::DocumentObject*> getObjects() const;
    bool eventFilter(QObject*, QEvent*);

private Q_SLOTS:
    void on_addSelection_clicked();
    void on_clearSelection_clicked();
    void on_visibleTriangles_toggled(bool);
    void on_screenTriangles_toggled(bool);

private:
    MeshSelection meshSel;
    Ui_Selection* ui;
};

}

#endif // MESHGUI_SELECTION_H
