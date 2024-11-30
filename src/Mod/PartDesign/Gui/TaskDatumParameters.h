/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender                                   *
 *                                   <jrheinlaender@users.sourceforge.net> *
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


#ifndef GUI_TASKVIEW_TaskDatumParameters_H
#define GUI_TASKVIEW_TaskDatumParameters_H

#include <Gui/TaskView/TaskView.h>
#include <Mod/Part/Gui/TaskAttacher.h>

#include "ViewProviderDatum.h"

class Ui_TaskDatumParameters;
class QLineEdit;

namespace App {
class Property;
}

namespace Gui {
class ViewProvider;
}

namespace PartDesignGui {



class TaskDatumParameters : public PartGui::TaskAttacher
{
    Q_OBJECT

public:
    explicit TaskDatumParameters(ViewProviderDatum *DatumView,QWidget *parent = nullptr);
    ~TaskDatumParameters() override;
};

/// simulation dialog for the TaskView
class TaskDlgDatumParameters : public PartGui::TaskDlgAttacher
{
    Q_OBJECT

public:
    explicit TaskDlgDatumParameters(ViewProviderDatum *DatumView);
    ~TaskDlgDatumParameters() override;

    bool accept() override;
    bool reject() override;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TASKAPPERANCE_H
