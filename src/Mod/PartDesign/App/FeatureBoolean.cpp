// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
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


#include <Mod/Part/App/FCBRepAlgoAPI_Common.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
#include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
#include <Standard_Failure.hxx>

#include <algorithm>

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Base/Tools.h>
#include <Mod/Part/App/modelRefine.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureBoolean.h"
#include "Body.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true);

using namespace PartDesign;

namespace PartDesign
{
extern bool getPDRefineModelParameter();

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::Boolean, PartDesign::FeatureRefine)

const char* Boolean::TypeEnums[] = {"Fuse", "Cut", "Common", nullptr};

Boolean::Boolean()
{
    ADD_PROPERTY(Type, ((long)0));
    Type.setEnums(TypeEnums);
    ADD_PROPERTY_TYPE(
        UseLegacyBodyPlacement,
        (App::GetApplication().isRestoring()),
        "Compatibility",
        App::Prop_Hidden,
        "Use legacy PartDesign Boolean body placement handling"
    );

    App::GeoFeatureGroupExtension::initExtension(this);
    // Boolean tools are references, not owned coordinate-system children.
    Group.setScope(App::LinkScope::Global);
}

short Boolean::mustExecute() const
{
    if (Group.isTouched() || UseLegacyBodyPlacement.isTouched()) {
        return 1;
    }
    return PartDesign::Feature::mustExecute();
}

TopoShape Boolean::getBooleanTopoShape(const App::DocumentObject* object) const
{
    if (UseLegacyBodyPlacement.getValue()) {
        return getTopoShape(object, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
    }
    return getTopoShapeInLocalCoordinates(object);
}

std::vector<App::DocumentObject*> Boolean::addObject(App::DocumentObject* object)
{
    return addObjects({object});
}

std::vector<App::DocumentObject*> Boolean::addObjects(std::vector<App::DocumentObject*> objects)
{
    auto tools = Group.getValues();
    std::vector<App::DocumentObject*> added;

    for (auto object : objects) {
        if (!object || std::ranges::find(tools, object) != tools.end()) {
            continue;
        }

        tools.push_back(object);
        added.push_back(object);
    }

    Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(App::Property::User3, &Group);
    Group.setValues(tools);
    return added;
}

std::vector<App::DocumentObject*> Boolean::setObjects(std::vector<App::DocumentObject*> objects)
{
    std::vector<App::DocumentObject*> tools;
    std::vector<App::DocumentObject*> added;

    for (auto object : objects) {
        if (!object || std::ranges::find(tools, object) != tools.end()) {
            continue;
        }

        tools.push_back(object);
        added.push_back(object);
    }

    Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(App::Property::User3, &Group);
    Group.setValues(tools);
    return added;
}

std::vector<App::DocumentObject*> Boolean::removeObject(App::DocumentObject* object)
{
    return removeObjects({object});
}

std::vector<App::DocumentObject*> Boolean::removeObjects(std::vector<App::DocumentObject*> objects)
{
    auto tools = Group.getValues();
    std::vector<App::DocumentObject*> removed;

    for (auto object : objects) {
        auto it = std::ranges::find(tools, object);
        if (it == tools.end()) {
            continue;
        }

        removed.push_back(object);
        tools.erase(it);
    }

    Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(App::Property::User3, &Group);
    Group.setValues(tools);
    return removed;
}

bool Boolean::hasObject(const App::DocumentObject* /*object*/, bool /*recursive*/) const
{
    // Boolean tools are references, not owned children. Returning false keeps
    // GeoFeatureGroupExtension from using this object as their coordinate system.
    return false;
}

App::DocumentObjectExecReturn* Boolean::execute()
{
    // Get the operation type
    std::string type = Type.getValueAsString();

    // Check the parameters
    const Part::Feature* baseFeature = this->getBaseObject(/* silent = */ true);

    if (!baseFeature && type == "Cut") {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Cannot do boolean cut without BaseFeature")
        );
    }

    std::vector<App::DocumentObject*> tools = Group.getValues();
    if (tools.empty()) {
        return App::DocumentObject::StdReturn;
    }

    // Get the base shape to operate on
    Part::TopoShape baseTopShape;
    if (baseFeature) {
        if (UseLegacyBodyPlacement.getValue()) {
            baseTopShape = baseFeature->Shape.getShape();
        }
        else {
            baseTopShape = getBooleanTopoShape(baseFeature);
        }
    }
    else {
        auto feature = tools.back();
        if (!feature->isDerivedFrom<Part::Feature>()) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
                "Exception",
                "Cannot do boolean with anything but Part::Feature and its derivatives"
            ));
        }

        if (UseLegacyBodyPlacement.getValue()) {
            baseTopShape = static_cast<Part::Feature*>(feature)->Shape.getShape();
        }
        else {
            baseTopShape = getBooleanTopoShape(feature);
        }
        tools.pop_back();
    }

    if (baseTopShape.getShape().IsNull()) {
        return new App::DocumentObjectExecReturn(
            QT_TRANSLATE_NOOP("Exception", "Cannot do boolean operation with invalid base shape")
        );
    }

    std::vector<TopoShape> shapes;
    shapes.push_back(baseTopShape);
    for (auto it = tools.begin(); it < tools.end(); ++it) {
        auto shape = getBooleanTopoShape(*it);
        if (shape.isNull()) {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Tool shape is null")
            );
        }
        shapes.push_back(shape);
    }
    TopoShape result(baseTopShape);

    if (!tools.empty()) {
        const char* op = nullptr;

        if (type == "Fuse") {
            op = Part::OpCodes::Fuse;
        }
        else if (type == "Cut") {
            op = Part::OpCodes::Cut;
        }
        else if (type == "Common") {
            op = Part::OpCodes::Common;
        }
        // LinkStage3 defines these other types of Boolean operations.  Removed for now pending
        // decision to bring them in or not.
        // else if(type == "Compound")
        //     op = Part::OpCodes::Compound;
        // else if(type == "Section")
        //     op = Part::OpCodes::Section;
        else {
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Unsupported boolean operation")
            );
        }

        try {
            result.makeElementBoolean(op, shapes, nullptr, FuzzyTolerance.getValue());
        }
        catch (Standard_Failure& e) {
            FC_ERR("Boolean operation failed: " << e.GetMessageString());
            return new App::DocumentObjectExecReturn(
                QT_TRANSLATE_NOOP("Exception", "Boolean operation failed")
            );
        }
    }

    result = refineShapeIfActive(result);

    if (!isSingleSolidRuleSatisfied(result.getShape())) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP(
            "Exception",
            "Result has multiple solids: enable 'Allow Compound' in the active body."
        ));
    }

    this->Shape.setValue(getSolid(result));

    return StdReturn;
}

void Boolean::updatePreviewShape()
{
    if (strcmp(Type.getValueAsString(), "Cut") == 0) {
        TopoShape base;
        if (UseLegacyBodyPlacement.getValue()) {
            base = getBaseTopoShape(true);
            base.move(getLocation().Inverted());
        }
        else {
            base = getBooleanTopoShape(BaseFeature.getValue());
        }
        TopoShape result = Shape.getShape();

        try {
            PreviewShape.setValue(base.makeElementCut(result.getShape()));
        }
        catch (Standard_Failure& e) {
            FC_ERR("Boolean preview failed: " << e.GetMessageString());
            PreviewShape.setValue(base);
        }
        catch (Base::Exception& e) {
            FC_ERR("Boolean preview failed: " << e.what());
            PreviewShape.setValue(base);
        }
        return;
    }

    if (strcmp(Type.getValueAsString(), "Fuse") == 0) {
        // if there are no other shapes to fuse just return itself
        if (Group.getValues().empty()) {
            PreviewShape.setValue(Shape.getShape());
            return;
        }

        std::vector<TopoShape> shapes;

        for (auto& obj : Group.getValues()) {
            shapes.push_back(getBooleanTopoShape(obj));
        }

        TopoShape result;
        result.makeCompound(shapes);

        PreviewShape.setValue(result.getShape());
        return;
    }

    PreviewShape.setValue(Shape.getShape());
}

void Boolean::onChanged(const App::Property* prop)
{

    if (strcmp(prop->getName(), "Group") == 0) {
        touch();
    }

    Feature::onChanged(prop);
}

void Boolean::handleChangedPropertyName(Base::XMLReader& reader, const char* TypeName, const char* PropName)
{
    // The App::PropertyLinkList property was Bodies in the past
    Base::Type type = Base::Type::fromName(TypeName);

    if (Group.getClassTypeId() == type && strcmp(PropName, "Bodies") == 0) {
        Group.Restore(reader);
    }
}

}  // namespace PartDesign
