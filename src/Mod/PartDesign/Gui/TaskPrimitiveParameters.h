/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefeantroeger@gmx.net>             *
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


#ifndef GUI_TASKVIEW_TaskPrimitiveParameters_H
#define GUI_TASKVIEW_TaskPrimitiveParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>
#include <Gui/TaskView/TaskDialog.h>

#include "TaskSketchBasedParameters.h"
#include "ViewProviderPrimitive.h"
#include "TaskDatumParameters.h"
#include <Mod/PartDesign/App/DatumCS.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include "ui_TaskPrimitiveParameters.h"

class Ui_TaskPrimitiveParameters;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui { 


class TaskBoxPrimitives : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskBoxPrimitives(PartDesign::FeaturePrimitive::Type t, QWidget* parent = 0);
    ~TaskBoxPrimitives();

private:
    QWidget* proxy;
    Ui_DlgPrimitives ui;
};

class TaskPrimitiveParameters : public Gui::TaskView::TaskDialog
{
    Q_OBJECT

public:
    TaskPrimitiveParameters(ViewProviderPrimitive *PrimitiveView);
    ~TaskPrimitiveParameters();
 
protected:
    virtual QDialogButtonBox::StandardButtons getStandardButtons(void) const;

    virtual bool accept();
    virtual bool reject();

private:  
    TaskBoxPrimitives*   primitive;
    TaskDatumParameters* parameter;
    PartDesign::CoordinateSystem* cs;
    bool cs_visibility;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
