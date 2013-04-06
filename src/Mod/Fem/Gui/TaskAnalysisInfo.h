/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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


#ifndef FEMGUI_TaskAnalysisInfo_H
#define FEMGUI_TaskAnalysisInfo_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>

#include <Mod/Fem/App/FemSetNodesObject.h>


class Ui_TaskAnalysisInfo;
class SoEventCallback;

namespace Base {
    class Polygon2D;
}
namespace App {
    class Property;
}

namespace Gui {
class ViewProvider;
class ViewVolumeProjection;
}

namespace Fem{
    class FemAnalysis;
}

namespace FemGui { 

class ViewProviderFemMesh;


class TaskAnalysisInfo : public Gui::TaskView::TaskBox
{
    Q_OBJECT

public:
    TaskAnalysisInfo(Fem::FemAnalysis *pcObject,QWidget *parent = 0);
    ~TaskAnalysisInfo();

private Q_SLOTS:
    void SwitchMethod(int Value);

protected:
    Fem::FemAnalysis *pcObject;

private:
    QWidget* proxy;
    Ui_TaskAnalysisInfo* ui;
};

} //namespace FEMGUI_TaskAnalysisInfo_H

#endif // GUI_TASKVIEW_TaskAnalysisInfo_H
