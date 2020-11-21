/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <boost/algorithm/string/predicate.hpp>
#include <boost_bind_bind.hpp>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>
#include <App/Document.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/DatumFeature.h>

#include "Body.h"
#include "Feature.h"
#include "FeatureSolid.h"
#include "AuxGroup.h"

using namespace PartDesign;
namespace bp = boost::placeholders;

PROPERTY_SOURCE(PartDesign::AuxGroup, App::DocumentObject)

AuxGroup::AuxGroup()
{
    ADD_PROPERTY_TYPE(_Body, (0), "Base",
            (App::PropertyType)(App::Prop_Hidden|App::Prop_ReadOnly|App::Prop_Output), 0);
    ADD_PROPERTY_TYPE(Group, (0), "Base",
            (App::PropertyType)(App::Prop_Hidden|App::Prop_ReadOnly|App::Prop_Output), 0);
}

void AuxGroup::onChanged(const App::Property* prop)
{
    auto doc = getDocument();
    if (doc && !doc->isPerformingTransaction()
            && !doc->testStatus(App::Document::Restoring)) {
        if(prop == &_Body) {
            attachBody();
            refresh();
        }
    }
    inherited::onChanged(prop);
}

void AuxGroup::onDocumentRestored()
{
    attachBody();
    inherited::onDocumentRestored();
}

void AuxGroup::attachBody()
{
    auto body = Base::freecad_dynamic_cast<Body>(_Body.getValue());
    if (!body)
        connBody.disconnect();
    else
        connBody = body->Group.signalChanged.connect(
                boost::bind(&AuxGroup::refresh, this));
}

AuxGroup::GroupType AuxGroup::getGroupType() const
{
    if (groupType != UnknownGroup)
        return groupType;
    if (!getNameInDocument())
        return UnknownGroup;
    if (boost::starts_with(getNameInDocument(), "Sketches"))
        groupType = SketchGroup;
    else if (boost::starts_with(getNameInDocument(), "Datums"))
        groupType = DatumGroup;
    else
        groupType = OtherGroup;
    return groupType;
}

bool AuxGroup::isObjectAllowed(const App::DocumentObject *obj) const
{
    if (!obj || boost::starts_with(obj->getNameInDocument(), "BaseFeature"))
        return false;
    switch (getGroupType()) {
    case SketchGroup:
        return obj->isDerivedFrom(Part::Part2DObject::getClassTypeId());
    case DatumGroup:
        return obj->isDerivedFrom(Part::Datum::getClassTypeId());
    case OtherGroup: {
        auto type = obj->getTypeId();
        return !type.isDerivedFrom(Part::Part2DObject::getClassTypeId())
            && !type.isDerivedFrom(Part::Datum::getClassTypeId())
            && !type.isDerivedFrom(Feature::getClassTypeId())
            && !type.isDerivedFrom(Solid::getClassTypeId())
            && !type.isDerivedFrom(AuxGroup::getClassTypeId());
    }
    default:
        return false;
    }
}

PartDesign::Body *AuxGroup::getBody() const
{
    return Base::freecad_dynamic_cast<Body>(_Body.getValue());
}

void AuxGroup::refresh()
{
    auto body = getBody();
    if (!body) {
        Group.setValues();
        return;
    }
    if (body->Group.find(getNameInDocument()) != this) {
        _Body.setValue(nullptr);
        Group.setValues();
        return;
    }
    std::vector<App::DocumentObject *> children;
    for (auto obj : body->Group.getValues()) {
        if (isObjectAllowed(obj))
            children.push_back(obj);
    }
    Group.setValues(children);
}
