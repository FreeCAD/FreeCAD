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
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <Standard_Failure.hxx>
#endif

#include <App/Application.h>
#include <App/DocumentObject.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/modelRefine.h>

#include "FeatureBoolean.h"
#include "Body.h"


using namespace PartDesign;

namespace PartDesign {

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::Boolean, PartDesign::Feature)

const char* Boolean::TypeEnums[]= {"Fuse","Cut","Common",nullptr};

Boolean::Boolean()
{
    ADD_PROPERTY(Type,((long)0));
    Type.setEnums(TypeEnums);

    ADD_PROPERTY_TYPE(Refine,(0),"Part Design",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after adding/subtracting");
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    this->Refine.setValue(hGrp->GetBool("RefineModel", false));

    initExtension(this);
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
        if(!feature->isDerivedFrom(Part::Feature::getClassTypeId()))
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

    TopoDS_Shape result = baseTopShape.getShape();

    for (auto tool : tools)
    {
        if(!tool->isDerivedFrom(Part::Feature::getClassTypeId()))
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Cannot do boolean with anything but Part::Feature and its derivatives"));

        TopoDS_Shape shape = static_cast<Part::Feature*>(tool)->Shape.getValue();
        TopoDS_Shape boolOp;

        // Must not pass null shapes to the boolean operations
        if (result.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Base shape is null"));

        if (shape.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Tool shape is null"));

        if (type == "Fuse") {
            BRepAlgoAPI_Fuse mkFuse(result, shape);
            if (!mkFuse.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Fusion of tools failed"));
            // we have to get the solids (fuse sometimes creates compounds)
            boolOp = this->getSolid(mkFuse.Shape());
            // lets check if the result is a solid
            if (boolOp.IsNull())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Resulting shape is not a solid"));
        } else if (type == "Cut") {
            BRepAlgoAPI_Cut mkCut(result, shape);
            if (!mkCut.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Cut out failed"));
            boolOp = mkCut.Shape();
        } else if (type == "Common") {
            BRepAlgoAPI_Common mkCommon(result, shape);
            if (!mkCommon.IsDone())
                return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Common operation failed"));
            boolOp = mkCommon.Shape();
        }

        result = boolOp; // Use result of this operation for fuse/cut of next body
    }

    result = refineShapeIfActive(result);

    int solidCount = countSolids(result);
    if (solidCount > 1) {
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

TopoDS_Shape Boolean::refineShapeIfActive(const TopoDS_Shape& oldShape) const
{
    if (this->Refine.getValue()) {
        try {
            Part::BRepBuilderAPI_RefineModel mkRefine(oldShape);
            TopoDS_Shape resShape = mkRefine.Shape();
            return resShape;
        }
        catch (Standard_Failure&) {
            return oldShape;
        }
    }

    return oldShape;
}

}
