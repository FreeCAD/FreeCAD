/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
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

#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "Gui/Control.h"
#include <Mod/Fem/App/FemConstraintFixed.h>

#include "TaskFemConstraintFixed.h"
#include "ViewProviderFemConstraintFixed.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintFixed, FemGui::ViewProviderFemConstraintOnBoundary)


ViewProviderFemConstraintFixed::ViewProviderFemConstraintFixed()
{
    sPixmap = "FEM_ConstraintFixed";
    loadSymbol((resourceSymbolDir + "ConstraintFixed.iv").c_str());
}

ViewProviderFemConstraintFixed::~ViewProviderFemConstraintFixed() = default;

bool ViewProviderFemConstraintFixed::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgFemConstraintFixed(this));

        return true;
    }
    else {
        return ViewProviderFemConstraintOnBoundary::setEdit(ModNum);
    }
}

void ViewProviderFemConstraintFixed::updateData(const App::Property* prop)
{
    ViewProviderFemConstraint::updateData(prop);
}
