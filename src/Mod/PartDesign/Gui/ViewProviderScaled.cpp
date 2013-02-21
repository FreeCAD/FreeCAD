/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "ViewProviderScaled.h"
#include "TaskScaledParameters.h"
#include <Mod/PartDesign/App/FeatureScaled.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderScaled,PartDesignGui::ViewProvider)

bool ViewProviderScaled::setEdit(int ModNum)
{
    ViewProviderTransformed::setEdit(ModNum);

    if (ModNum == ViewProvider::Default ) {
        TaskDlgScaledParameters *scaledDlg = NULL;

        if (checkDlgOpen(scaledDlg)) {
            // start the edit dialog
            if (scaledDlg)
                Gui::Control().showDialog(scaledDlg);
            else
                Gui::Control().showDialog(new TaskDlgScaledParameters(this));

            return true;
        } else {
            return false;
        }
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}
