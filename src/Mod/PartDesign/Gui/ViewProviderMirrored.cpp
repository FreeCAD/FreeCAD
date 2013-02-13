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

#include "ViewProviderMirrored.h"
#include "TaskMirroredParameters.h"
#include <Mod/PartDesign/App/FeatureMirrored.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderMirrored,PartDesignGui::ViewProvider)

bool ViewProviderMirrored::setEdit(int ModNum)
{
    ViewProviderTransformed::setEdit(ModNum);

    if (ModNum == ViewProvider::Default ) {
        TaskDlgMirroredParameters *mirroredDlg = NULL;

        if (checkDlgOpen(mirroredDlg)) {
            // start the edit dialog
            if (mirroredDlg)
                Gui::Control().showDialog(mirroredDlg);
            else
                Gui::Control().showDialog(new TaskDlgMirroredParameters(this));

            return true;
        } else {
            return false;
        }
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}
