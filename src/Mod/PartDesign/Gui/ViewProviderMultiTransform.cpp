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

#include "ViewProviderMultiTransform.h"
#include "TaskMultiTransformParameters.h"
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderMultiTransform,PartDesignGui::ViewProvider)

bool ViewProviderMultiTransform::setEdit(int ModNum)
{
    ViewProviderTransformed::setEdit(ModNum);

    if (ModNum == ViewProvider::Default ) {
        TaskDlgMultiTransformParameters *multitransformDlg = NULL;

        if (checkDlgOpen(multitransformDlg)) {
            // start the edit dialog
            if (multitransformDlg)
                Gui::Control().showDialog(multitransformDlg);
            else
                Gui::Control().showDialog(new TaskDlgMultiTransformParameters(this));

            return true;
        } else {
            return false;
        }
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}

std::vector<App::DocumentObject*> ViewProviderMultiTransform::claimChildren(void) const
{
    PartDesign::MultiTransform* pcMultiTransform = static_cast<PartDesign::MultiTransform*>(getObject());
    if (pcMultiTransform == NULL)
        return std::vector<App::DocumentObject*>(); // TODO: Show error?

    std::vector<App::DocumentObject*> transformFeatures = pcMultiTransform->Transformations.getValues();
    return transformFeatures;
}

bool ViewProviderMultiTransform::onDelete(const std::vector<std::string> &svec) {
    // Delete the transformation features
    PartDesign::MultiTransform* pcMultiTransform = static_cast<PartDesign::MultiTransform*>(getObject());
    std::vector<App::DocumentObject*> transformFeatures = pcMultiTransform->Transformations.getValues();

    // if abort command deleted the object the transformed features must be deleted, too
    for (std::vector<App::DocumentObject*>::const_iterator it = transformFeatures.begin(); it != transformFeatures.end(); ++it)
    {
        if ((*it) != NULL)
            Gui::Command::doCommand(
                Gui::Command::Doc,"App.ActiveDocument.removeObject(\"%s\")", (*it)->getNameInDocument());
    }

    // Handle Originals
    return ViewProviderTransformed::onDelete(svec);
}
