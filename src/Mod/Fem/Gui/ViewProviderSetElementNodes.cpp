/***************************************************************************
 *   Copyright (c) 2022 Peter McB                                          *
 *                                                                         *
 *   based on: ViewProviderSetNodes.cpp                                    *
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

#include "PreCompiled.h"

#include <Gui/Control.h>
#include <Mod/Fem/App/FemSetElementNodesObject.h>
#include <Mod/Fem/Gui/TaskDlgCreateElementSet.h>

#include "ViewProviderSetElementNodes.h"
using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderSetElementNodes, Gui::ViewProviderGeometryObject)

bool ViewProviderSetElementNodes::doubleClicked()
{
    Gui::TaskView::TaskDialog* dlg = new TaskDlgCreateElementSet(static_cast<Fem::FemSetElementNodesObject *>(getObject()));
    Gui::Control().showDialog(dlg);
    return true;
}


bool ViewProviderSetElementNodes::setEdit(int)
{
    Gui::TaskView::TaskDialog* dlg = new TaskDlgCreateElementSet(static_cast<Fem::FemSetElementNodesObject *>(getObject()));
    Gui::Control().showDialog(dlg);
    return true;
}

void ViewProviderSetElementNodes::unsetEdit(int)
{

}
