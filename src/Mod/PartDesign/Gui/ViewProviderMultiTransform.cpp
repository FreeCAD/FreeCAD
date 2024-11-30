/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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

#include "ViewProviderMultiTransform.h"
#include "TaskMultiTransformParameters.h"
#include <Mod/PartDesign/App/FeatureMultiTransform.h>
#include <App/Document.h>
#include <Gui/Command.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderMultiTransform,PartDesignGui::ViewProviderTransformed)

TaskDlgFeatureParameters *ViewProviderMultiTransform::getEditDialog() {
    return new TaskDlgMultiTransformParameters (this);
}

const std::string & ViewProviderMultiTransform::featureName() const
{
    static const std::string name = "MultiTransform";
    return name;
}

void ViewProviderMultiTransform::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit multi-transform"));
    PartDesignGui::ViewProvider::setupContextMenu(menu, receiver, member); // clazy:exclude=skipped-base-method
}

std::vector<App::DocumentObject*> ViewProviderMultiTransform::claimChildren() const
{
    PartDesign::MultiTransform* pcMultiTransform = static_cast<PartDesign::MultiTransform*>(getObject());
    if (!pcMultiTransform)
        return {}; // TODO: Show error?

    std::vector<App::DocumentObject*> transformFeatures = pcMultiTransform->Transformations.getValues();
    return transformFeatures;
}

bool ViewProviderMultiTransform::onDelete(const std::vector<std::string> &svec) {
    // Delete the transformation features
    PartDesign::MultiTransform* pcMultiTransform = static_cast<PartDesign::MultiTransform*>(getObject());
    std::vector<App::DocumentObject*> transformFeatures = pcMultiTransform->Transformations.getValues();

    // if the multitransform object was deleted the transformed features must be deleted, too
    for (auto it : transformFeatures) {
        if (it) {
            Gui::Command::doCommand(
                Gui::Command::Doc,"App.getDocument('%s').removeObject(\"%s\")", \
                    it->getDocument()->getName(), it->getNameInDocument());
        }
    }

    // Handle Originals
    return ViewProviderTransformed::onDelete(svec);
}
