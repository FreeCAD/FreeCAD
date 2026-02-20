// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include <Bnd_Box.hxx>
#include <BRep_Tool.hxx>
#include <BRepMesh_IncrementalMesh.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <QMenu>


#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Mod/Part/App/Tools.h>
#include <Mod/PartDesign/App/FeatureMultiTransform.h>

#include "ViewProviderTransformed.h"
#include "TaskTransformedParameters.h"

#include <BRep_Builder.hxx>
#include <Inventor/nodes/SoTransform.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderTransformed, PartDesignGui::ViewProvider)

const std::string& ViewProviderTransformed::featureName() const
{
    static const std::string name = "undefined";
    return name;
}

std::string ViewProviderTransformed::featureIcon() const
{
    return fmt::format("PartDesign_{}", featureName());
}

void ViewProviderTransformed::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QString text = QString::fromStdString(getObject()->Label.getStrValue());
    addDefaultAction(menu, QObject::tr("Edit %1").arg(text));

    ViewProvider::setupContextMenu(menu, receiver, member);
}

Gui::ViewProvider* ViewProviderTransformed::startEditing(int ModNum)
{
    auto* pcTransformed = getObject<PartDesign::Transformed>();

    if (!pcTransformed->Originals.getSize()) {
        for (auto obj : pcTransformed->getInList()) {
            if (!obj->isDerivedFrom<PartDesign::MultiTransform>()) {
                continue;
            }

            if (auto vp = Gui::Application::Instance->getViewProvider(obj)) {
                return vp->startEditing(ModNum);
            }

            return nullptr;
        }
    }

    return ViewProvider::startEditing(ModNum);
}

bool ViewProviderTransformed::setEdit(int ModNum)
{
    recomputeFeature(false);

    return ViewProvider::setEdit(ModNum);
}

void ViewProviderTransformed::attachPreview()
{}

void ViewProviderTransformed::updatePreview()
{
    if (pcObject->getDocument()->testStatus(App::Document::Restoring)) {
        return;
    }

    try {
        if (auto feature = getObject<PartDesign::Transformed>()) {
            auto originals = feature->getOriginals();
            auto transforms = feature->getTransformations(originals);

            if (transforms.empty()) {
                return;
            }

            Gui::coinRemoveAllChildren(pcPreviewRoot);

            for (const auto& transform : transforms) {
                Base::Matrix4D transformMatrix;
                Part::TopoShape::convertToMatrix(transform, transformMatrix);

                auto sep = new SoSeparator;

                auto transformNode = new SoTransform;
                transformNode->setMatrix(convert(transformMatrix));

                sep->addChild(transformNode);
                sep->addChild(pcPreviewShape);

                pcPreviewRoot->addChild(sep);
            }
        }
    }
    catch (const Base::ValueError&) {
        // no-op - ignore misconfigured objects
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }

    ViewProvider::updatePreview();
}

void ViewProviderTransformed::handleTransformedResult(PartDesign::Transformed* pcTransformed)
{
    unsigned rejected = 0;

    TopoDS_Shape cShape = pcTransformed->rejected;
    TopExp_Explorer xp;
    xp.Init(cShape, TopAbs_SOLID);
    for (; xp.More(); xp.Next()) {
        rejected++;
    }

    if (rejected > 0) {
        if (rejected == 1) {
            Base::Console().translatedUserWarning(
                "ViewProviderTransformed",
                "One transformed shape does not intersect the support"
            );
        }
        else {
            Base::Console().translatedUserWarning(
                "ViewProviderTransformed",
                "%d transformed shapes do not intersect the support",
                rejected
            );
        }
    }

    auto error = pcTransformed->getDocument()->getErrorDescription(pcTransformed);
    if (error) {
        Base::Console().translatedUserError("ViewProviderTransformed", error);
    }
}

void ViewProviderTransformed::recomputeFeature(bool recompute)
{
    auto* pcTransformed = getObject<PartDesign::Transformed>();

    if (recompute || pcTransformed->isError() || pcTransformed->mustExecute()) {
        pcTransformed->recomputeFeature(true);
    }

    updatePreview();

    handleTransformedResult(pcTransformed);
}
