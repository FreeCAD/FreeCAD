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


#include "PreCompiled.h"
#ifndef _PreComp_
# include <Mod/Part/App/FCBRepAlgoAPI_Common.h>
# include <Mod/Part/App/FCBRepAlgoAPI_Cut.h>
# include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
# include <Standard_Failure.hxx>
#endif

#include <App/DocumentObject.h>
#include <Mod/Part/App/modelRefine.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

#include "FeatureBoolean.h"
#include "Body.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true);

using namespace PartDesign;

namespace PartDesign {

extern bool getPDRefineModelParameter();

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::Boolean, PartDesign::Feature)

const char* Boolean::TypeEnums[]= {"Fuse","Cut","Common",nullptr};

Boolean::Boolean()
{
    ADD_PROPERTY(Type,((long)0));
    Type.setEnums(TypeEnums);

    ADD_PROPERTY_TYPE(UsePlacement,(0),"Part Design",(App::PropertyType)(App::Prop_None),"Apply the placement of the second ( tool ) object");
    this->UsePlacement.setValue(false);

    App::GeoFeatureGroupExtension::initExtension(this);
}

short Boolean::mustExecute() const
{
    if (Group.isTouched())
        return 1;
    return PartDesign::Feature::mustExecute();
}

App::DocumentObjectExecReturn *Boolean::execute()
{
    // Get the operation type
    std::string type = Type.getValueAsString();

    // Check the parameters
    const Part::Feature* baseFeature = this->getBaseObject(/* silent = */ true);

    if (!baseFeature && type == "Cut") {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Cannot do boolean cut without BaseFeature"));
    }

    std::vector<App::DocumentObject*> tools = Group.getValues();
    if (tools.empty())
        return App::DocumentObject::StdReturn;

    // Get the base shape to operate on
    Part::TopoShape baseTopShape;
    if(baseFeature)
        baseTopShape = baseFeature->Shape.getShape();
    else {
        auto feature = tools.back();
        if(!feature->isDerivedFrom<Part::Feature>())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Cannot do boolean with anything but Part::Feature and its derivatives"));

        baseTopShape = static_cast<Part::Feature*>(feature)->Shape.getShape();
        tools.pop_back();
    }

    if (baseTopShape.getShape().IsNull())
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Cannot do boolean operation with invalid base shape"));

    //get the body this boolean feature belongs to
    Part::BodyBase* baseBody = Part::BodyBase::findBodyOf(this);

    if(!baseBody)
         return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Cannot do boolean on feature which is not in a body"));

    std::vector<TopoShape> shapes;
    shapes.push_back(baseTopShape);
    for(auto it=tools.begin(); it<tools.end(); ++it) {
        auto shape = getTopoShape(*it, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
        if (shape.isNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception","Tool shape is null"));
        shapes.push_back(shape);
    }
    TopoShape result(baseTopShape);

    Base::Placement  bodyPlacement = baseBody->globalPlacement().inverse();
    for (auto tool : tools)
    {
        if(!tool->isDerivedFrom<Part::Feature>())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Cannot do boolean with anything but Part::Feature and its derivatives"));

        Part::TopoShape toolShape = static_cast<Part::Feature*>(tool)->Shape.getShape();
        if ( UsePlacement.getValue() )
            toolShape.setPlacement(bodyPlacement * toolShape.getPlacement());
        TopoDS_Shape shape = toolShape.getShape();
        TopoDS_Shape boolOp;

        // Must not pass null shapes to the boolean operations
        if (result.isNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Base shape is null"));

        if (shape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Tool shape is null"));

        const char *op = nullptr;
        if (type == "Fuse")
            op = Part::OpCodes::Fuse;
        else if(type == "Cut")
            op = Part::OpCodes::Cut;
        else if(type == "Common")
            op = Part::OpCodes::Common;
        // LinkStage3 defines these other types of Boolean operations.  Removed for now pending
        // decision to bring them in or not.
       // else if(type == "Compound")
        //     op = Part::OpCodes::Compound;
        // else if(type == "Section")
        //     op = Part::OpCodes::Section;
        else
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Unsupported boolean operation"));

        try {
            result.makeElementBoolean(op, shapes);
        } catch (Standard_Failure &e) {
            FC_ERR("Boolean operation failed: " << e.GetMessageString());
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Boolean operation failed"));
        }
    }

    result = refineShapeIfActive(result);

    if (!isSingleSolidRuleSatisfied(result.getShape())) {
        return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Result has multiple solids: that is not currently supported."));
    }

    this->Shape.setValue(getSolid(result));
    return App::DocumentObject::StdReturn;
}

void Boolean::onChanged(const App::Property* prop) {

    if(strcmp(prop->getName(), "Group") == 0)
        touch();

    PartDesign::Feature::onChanged(prop);
}

void Boolean::handleChangedPropertyName(Base::XMLReader &reader, const char * TypeName, const char *PropName)
{
    // The App::PropertyLinkList property was Bodies in the past
    Base::Type type = Base::Type::fromName(TypeName);
    if (Group.getClassTypeId() == type && strcmp(PropName, "Bodies") == 0) {
        Group.Restore(reader);
    }
}

}
