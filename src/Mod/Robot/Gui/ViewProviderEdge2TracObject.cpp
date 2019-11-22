/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel (juergen.riegel@web.de)              *
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

#ifndef _PreComp_
#endif

#include "ViewProviderEdge2TracObject.h"
#include <Gui/Control.h>
#include <Mod/Robot/Gui/TaskDlgEdge2Trac.h>

using namespace Gui;
using namespace RobotGui;

PROPERTY_SOURCE(RobotGui::ViewProviderEdge2TracObject, RobotGui::ViewProviderTrajectory)

bool ViewProviderEdge2TracObject::doubleClicked(void)
{
    Gui::TaskView::TaskDialog* dlg = new TaskDlgEdge2Trac(static_cast<Robot::Edge2TracObject *>(getObject()));
    Gui::Control().showDialog(dlg);
    return true;
}


bool ViewProviderEdge2TracObject::setEdit(int)
{
    Gui::TaskView::TaskDialog* dlg = new TaskDlgEdge2Trac(static_cast<Robot::Edge2TracObject *>(getObject()));
    Gui::Control().showDialog(dlg);
    return true;
}

void ViewProviderEdge2TracObject::unsetEdit(int)
{

}
