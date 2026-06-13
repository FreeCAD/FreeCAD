// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2017 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include <Standard_Failure.hxx>


#include <App/Application.h>
#include <App/FeaturePythonPyImp.h>
#include "Body.h"
#include "FeatureBase.h"
#include "FeaturePy.h"

namespace PartDesign
{


PROPERTY_SOURCE(PartDesign::FeatureBase, PartDesign::Feature)

FeatureBase::FeatureBase()
{
    BaseFeature.setScope(App::LinkScope::Global);
    BaseFeature.setStatus(App::Property::Hidden, false);
    ADD_PROPERTY_TYPE(
        UseLegacyBaseFeaturePlacement,
        (App::GetApplication().isRestoring()),
        "Compatibility",
        App::Prop_Hidden,
        "Use legacy FeatureBase source placement handling"
    );
}

Part::Feature* FeatureBase::getBaseObject(bool) const
{

    return nullptr;
}

short int FeatureBase::mustExecute() const
{

    if (BaseFeature.isTouched() || UseLegacyBaseFeaturePlacement.isTouched()) {
        return 1;
    }

    return PartDesign::Feature::mustExecute();
}


App::DocumentObjectExecReturn* FeatureBase::execute()
{

    if (!BaseFeature.getValue()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "BaseFeature link is not set")
        );
    }

    if (!BaseFeature.getValue()->isDerivedFrom<Part::Feature>()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "BaseFeature must be a Part::Feature")
        );
    }

    auto* base = BaseFeature.getValue();
    if (UseLegacyBaseFeaturePlacement.getValue()) {
        auto shape = Part::Feature::getTopoShape(
            base,
            Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
        );
        if (shape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "BaseFeature has an empty shape")
            );
        }

        Shape.setValue(shape);
        return StdReturn;
    }

    const bool isBodyLocalFeature = base->isDerivedFrom<PartDesign::Feature>()
        && Body::findBodyOf(base);
    const bool isPartDesignBody = base->isDerivedFrom<PartDesign::Body>();

    auto shape = isBodyLocalFeature
        ? static_cast<Part::Feature*>(base)->Shape.getShape()
        : Part::Feature::getTopoShape(base, Part::ShapeOption::ResolveLink);
    if (shape.isNull()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "BaseFeature has an empty shape")
        );
    }

    if (isBodyLocalFeature || isPartDesignBody) {
        shape.transformShape(shape.getTransform(), true);
    }

    Shape.setValue(shape);

    return StdReturn;
}

void FeatureBase::trySetBaseFeatureOfBody()
{
    if (auto body = getFeatureBody()) {
        if (BaseFeature.getValue() && body->BaseFeature.getValue()
            && body->BaseFeature.getValue() != BaseFeature.getValue()) {
            body->BaseFeature.setValue(BaseFeature.getValue());
        }
    }
}

void FeatureBase::onChanged(const App::Property* prop)
{

    // the BaseFeature property should track the Body BaseFeature and vice-versa
    if (prop == &BaseFeature) {
        trySetBaseFeatureOfBody();
    }

    PartDesign::Feature::onChanged(prop);
}

void FeatureBase::onDocumentRestored()
{
    // if the base is not part of a body then show its placement property again
    auto body = getFeatureBody();
    if (!body) {
        Placement.setStatus(App::Property::Hidden, false);
    }
    PartDesign::Feature::onDocumentRestored();
}

}  // namespace PartDesign
