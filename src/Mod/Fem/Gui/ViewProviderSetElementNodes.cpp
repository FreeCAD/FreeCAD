/***************************************************************************
 *   Copyright (c) 2023 Peter McB                                          *
 *                                                                         *
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

#include "PreCompiled.h"

#include <Gui/Control.h>
#include <Mod/Fem/App/FemSetElementNodesObject.h>
#include <Mod/Fem/Gui/TaskDlgCreateElementSet.h>

#include "ViewProviderSetElementNodes.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderSetElementNodes, Gui::ViewProviderGeometryObject)

bool ViewProviderSetElementNodes::doubleClicked()
{
    Gui::TaskView::TaskDialog* dlg =
        new TaskDlgCreateElementSet(getObject<Fem::FemSetElementNodesObject>());
    Gui::Control().showDialog(dlg);
    return true;
}


bool ViewProviderSetElementNodes::setEdit(int)
{
    doubleClicked();
    return true;
}

void ViewProviderSetElementNodes::unsetEdit(int)
{}
