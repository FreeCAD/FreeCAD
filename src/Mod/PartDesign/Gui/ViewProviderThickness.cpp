/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
#endif

#include "ViewProviderThickness.h"
#include "TaskThicknessParameters.h"
#include <Mod/PartDesign/App/FeatureThickness.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderThickness,PartDesignGui::ViewProviderDressUp)

bool ViewProviderThickness::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default ) {
		TaskDlgDressUpParameters *dressUpDlg = NULL;

        if (checkDlgOpen(dressUpDlg)) {
            // always change to PartDesign WB, remember where we come from
            oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

            // start the edit dialog
            if (dressUpDlg)
                Gui::Control().showDialog(dressUpDlg);
            else
                Gui::Control().showDialog(new TaskDlgThicknessParameters(this));

            return true;
        } else {
            return false;
        }
    }
    else {
        return ViewProviderDressUp::setEdit(ModNum);
    }
}


