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

#include "ViewProviderTrajectoryDressUp.h"
#include <Gui/Control.h>
#include <Mod/Robot/Gui/TaskDlgTrajectoryDressUp.h>

using namespace Gui;
using namespace RobotGui;

PROPERTY_SOURCE(RobotGui::ViewProviderTrajectoryDressUp, RobotGui::ViewProviderTrajectory)

//bool ViewProviderTrajectoryDressUp::doubleClicked(void)
//{
//    Gui::TaskView::TaskDialog* dlg = new TaskDlgTrajectoryDressUp(dynamic_cast<Robot::TrajectoryDressUpObject *>(getObject()));
//    Gui::Control().showDialog(dlg);
//    return true;
//}
//

bool ViewProviderTrajectoryDressUp::setEdit(int ModNum)
{
    Gui::TaskView::TaskDialog* dlg = new TaskDlgTrajectoryDressUp(dynamic_cast<Robot::TrajectoryDressUpObject *>(getObject()));
    Gui::Control().showDialog(dlg);
    return true;
}

void ViewProviderTrajectoryDressUp::unsetEdit(int ModNum)
{
    // when pressing ESC make sure to close the dialog
    Gui::Control().closeDialog();


}

std::vector<App::DocumentObject*> ViewProviderTrajectoryDressUp::claimChildren(void)const
{
    std::vector<App::DocumentObject*> temp;
    temp.push_back(static_cast<Robot::TrajectoryDressUpObject*>(getObject())->Source.getValue());
 
    return temp;
}
