/***************************************************************************
 *   Copyright (c) 2023 Peter McB                                          *
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <Gui/TaskView/TaskView.h>
#include <Mod/Fem/App/FemSetElementNodesObject.h>
#include <QMessageBox>


class Ui_TaskCreateElementSet;
class SoEventCallback;

namespace Base
{
class Polygon2d;
}
namespace App
{
class Property;
}

namespace Gui
{
class ViewProvider;
class ViewVolumeProjection;
}  // namespace Gui

namespace FemGui
{

class ViewProviderFemMesh;


class TaskCreateElementSet: public Gui::TaskView::TaskBox, public Gui::SelectionObserver
{
    Q_OBJECT

public:
    explicit TaskCreateElementSet(Fem::FemSetElementNodesObject* pcObject, QWidget* parent = nullptr);
    ~TaskCreateElementSet() override;

    std::set<long> elementTempSet;
    ViewProviderFemMesh* MeshViewProvider;
    static std::string currentProject;

private Q_SLOTS:
    void Poly();
    void Restore();
    void CopyResultsMesh();

protected:
    Fem::FemSetElementNodesObject* pcObject;
    static void DefineElementsCallback(void* ud, SoEventCallback* n);
    void DefineNodes(const Base::Polygon2d& polygon, const Gui::ViewVolumeProjection& proj, bool);

protected:
    void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    enum selectionModes
    {
        none,
        PickElement
    } selectionMode;

private:
    QWidget* proxy;
    Ui_TaskCreateElementSet* ui;
};

}  // namespace FemGui
