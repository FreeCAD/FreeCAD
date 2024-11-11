/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Authors: Michael Hindley <hindlemp@eskom.co.za>                       *
 *            Ruan Olwagen <olwager@eskom.co.za>                           *
 *            Oswald van Ginkel <vginkeo@eskom.co.za>                      *
 *   Based on Force constraint by Jan Rheinländer                          *
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

#include "Mod/Fem/App/FemConstraintInitialTemperature.h"
#include <Gui/Control.h>

#include "TaskFemConstraintInitialTemperature.h"
#include "ViewProviderFemConstraintInitialTemperature.h"


using namespace FemGui;

PROPERTY_SOURCE(FemGui::ViewProviderFemConstraintInitialTemperature,
                FemGui::ViewProviderFemConstraint)

ViewProviderFemConstraintInitialTemperature::ViewProviderFemConstraintInitialTemperature()
{
    sPixmap = "FEM_ConstraintInitialTemperature";
    ShapeAppearance.setDiffuseColor(0.2f, 0.3f, 0.2f);
}

ViewProviderFemConstraintInitialTemperature::~ViewProviderFemConstraintInitialTemperature() =
    default;

bool ViewProviderFemConstraintInitialTemperature::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::Control().closeDialog();
        // clear the selection (convenience)
        Gui::Selection().clearSelection();
        Gui::Control().showDialog(new TaskDlgFemConstraintInitialTemperature(this));

        return true;
    }
    else {
        return ViewProviderFemConstraint::setEdit(ModNum);
    }
}

void ViewProviderFemConstraintInitialTemperature::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    ViewProviderFemConstraint::updateData(prop);
}
