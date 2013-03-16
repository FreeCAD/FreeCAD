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


#ifndef GUI_TASKVIEW_TaskCreateNodeSet_H
#define GUI_TASKVIEW_TaskCreateNodeSet_H

#include <Gui/TaskView/TaskView.h>
#include <Gui/Selection.h>

#include <Mod/Fem/App/FemSetNodesObject.h>


class Ui_TaskCreateNodeSet;
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

namespace FemGui { 

class ViewProviderFemMesh;


class TaskCreateNodeSet : public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    TaskCreateNodeSet(Fem::FemSetNodesObject *pcObject,QWidget *parent = 0);
    ~TaskCreateNodeSet();

    std::set<long> tempSet;
    ViewProviderFemMesh * MeshViewProvider;

private Q_SLOTS:
    void Poly(void);
    void Pick(void);
    void SwitchMethod(int Value);

protected:
    Fem::FemSetNodesObject *pcObject;
    static void DefineNodesCallback(void * ud, SoEventCallback * n);
    void DefineNodes(const Base::Polygon2D &polygon,const Gui::ViewVolumeProjection &proj,bool);

protected:
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);
    enum selectionModes { none, PickElement} selectionMode;

private:
    QWidget* proxy;
    Ui_TaskCreateNodeSet* ui;
};

} //namespace PartDesignGui

#endif // GUI_TASKVIEW_TaskCreateNodeSet_H
