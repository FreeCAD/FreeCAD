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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Standard_Failure.hxx>
#endif

#include <App/FeaturePythonPyImp.h>
#include "Body.h"
#include "FeatureBase.h"
#include "FeaturePy.h"

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::FeatureBase,PartDesign::Feature)

FeatureBase::FeatureBase()
{
    BaseFeature.setScope(App::LinkScope::Global);
    BaseFeature.setStatus(App::Property::Hidden, false);
}

Part::Feature* FeatureBase::getBaseObject(bool) const {

    return nullptr;
}

short int FeatureBase::mustExecute() const {

    if(BaseFeature.isTouched())
        return 1;

    return Part::Feature::mustExecute();
}


App::DocumentObjectExecReturn* FeatureBase::execute() {

    if(!BaseFeature.getValue())
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "BaseFeature link is not set"));

    if(!BaseFeature.getValue()->isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "BaseFeature must be a Part::Feature"));

    auto shape = static_cast<Part::Feature*>(BaseFeature.getValue())->Shape.getValue();
    if (shape.IsNull())
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "BaseFeature has an empty shape"));

    Shape.setValue(shape);

    return StdReturn;
}

void FeatureBase::trySetBaseFeatureOfBody()
{
    if (auto body = getFeatureBody()) {
        if (BaseFeature.getValue()
                && body->BaseFeature.getValue()
                && body->BaseFeature.getValue() != BaseFeature.getValue()) {
            body->BaseFeature.setValue(BaseFeature.getValue());
        }
    }
}

void FeatureBase::onChanged(const App::Property* prop) {

    // the BaseFeature property should track the Body BaseFeature and vice-versa
    if (prop == &BaseFeature) {
        trySetBaseFeatureOfBody();
    }

    Part::Feature::onChanged(prop);
}

void FeatureBase::onDocumentRestored()
{
    // if the base is not part of a body then show its placement property again
    auto body = getFeatureBody();
    if (!body)
        Placement.setStatus(App::Property::Hidden, false);
}

}//namespace PartDesign

