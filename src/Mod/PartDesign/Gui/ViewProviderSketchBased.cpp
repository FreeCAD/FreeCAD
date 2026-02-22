// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include <Inventor/nodes/SoAnnotation.h>

#include <App/Document.h>
#include <Base/ServiceProvider.h>
#include <Gui/Application.h>
#include <Gui/Utilities.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/PartDesign/App/FeatureSketchBased.h>

#include "ViewProviderSketchBased.h"
#include "StyleParameters.h"

#include <Gui/Inventor/So3DAnnotation.h>
#include <Mod/Part/App/BodyBase.h>


using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderSketchBased, PartDesignGui::ViewProvider)


ViewProviderSketchBased::ViewProviderSketchBased()
    : pcProfileToggle(new SoToggleSwitch)
    , pcProfileShape(new PartGui::SoPreviewShape)
{
    auto annotation = new Gui::So3DAnnotation;
    annotation->addChild(pcProfileShape);

    pcProfileToggle->addChild(annotation);

    const auto updateProfileVisibility = [this]() {
        pcProfileToggle->on = hGrp->GetBool("ShowProfilePreview", true);
    };

    handlers.addHandler(hGrp, "ShowProfilePreview", [updateProfileVisibility](const Gui::ParamKey*) {
        updateProfileVisibility();
    });

    updateProfileVisibility();

    auto* styleParametersManager = Base::provideService<Gui::StyleParameters::ParameterManager>();
    pcProfileShape->transparency = 1.0F
        - static_cast<float>(styleParametersManager->resolve(StyleParameters::PreviewProfileOpacity)
                                 .value);
    pcProfileShape->lineWidth = static_cast<float>(
        styleParametersManager->resolve(StyleParameters::PreviewProfileLineWidth).value
    );
}

ViewProviderSketchBased::~ViewProviderSketchBased() = default;


std::vector<App::DocumentObject*> ViewProviderSketchBased::claimChildren() const
{
    std::vector<App::DocumentObject*> temp;
    App::DocumentObject* sketch = getObject<PartDesign::ProfileBased>()->Profile.getValue();
    if (sketch && !sketch->isDerivedFrom<PartDesign::Feature>()) {
        temp.push_back(sketch);
    }

    return temp;
}

void ViewProviderSketchBased::attach(App::DocumentObject* pcObject)
{
    ViewProvider::attach(pcObject);

    pcPreviewRoot->addChild(pcProfileToggle);

    // we want the profile to be the same color as the preview
    pcProfileShape->color.connectFrom(&pcPreviewShape->color);
}

void ViewProviderSketchBased::updateProfileShape()
{
    auto document = pcObject->getDocument();
    if (document->testStatus(App::Document::Restoring)) {
        return;
    }

    auto profileBased = getObject<PartDesign::ProfileBased>();
    auto profileShape = profileBased->getTopoShapeVerifiedFace(true);

    // set the correct coordinate space for the profile shape
    profileShape.setPlacement(
        profileShape.getPlacement() * profileBased->Placement.getValue().inverse()
    );

    updatePreviewShape(profileShape, pcProfileShape);
}

void ViewProviderSketchBased::updateData(const App::Property* prop)
{
    ViewProvider::updateData(prop);

    auto profileBased = getObject<PartDesign::ProfileBased>();
    if (!profileBased) {
        return;
    }

    if (prop == &profileBased->Profile) {
        updateProfileShape();
    }
}
void ViewProviderSketchBased::updatePreview()
{
    ViewProvider::updatePreview();

    updateProfileShape();
}
