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
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <gp_Trsf.hxx>
# include <gp_Pnt.hxx>
# include <gp_Dir.hxx>
# include <gp_Vec.hxx>
# include <gp_Ax1.hxx>
# include <Standard_Failure.hxx>
#endif

#include "Body.h"
#include "FeatureBoolean.h"

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Mod/Part/App/TopoShapeOpCode.h>

FC_LOG_LEVEL_INIT("PartDesign", true, true);

using namespace PartDesign;

namespace PartDesign {

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesign::Boolean, PartDesign::Feature)

const char* Boolean::TypeEnums[]= {"Fuse","Cut","Common","Compound", NULL};

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

App::DocumentObjectExecReturn *Boolean::execute(void)
{
    // Get the operation type
    std::string type = Type.getValueAsString();
   
    std::vector<App::DocumentObject*> tools = Group.getValues();
    auto itBegin = tools.begin();
    auto itEnd = tools.end();

    // Get the base shape to operate on
    TopoShape baseTopShape;
    try {
        if (!NewSolid.getValue()) {
            App::DocumentObject * base = getBaseObject(true);
            // In case the base is inside the tools, just ignore the base for now.
            if (std::find(tools.begin(), tools.end(), base) == tools.end())
                baseTopShape = getBaseShape();
        }
    } catch (const Base::Exception&) {
        if (type == "Cut")
            return new App::DocumentObjectExecReturn("Cannot do boolean cut without BaseFeature");
    }

    // If not base shape, use the first tool shape as base
    if(baseTopShape.isNull()) {
        if (tools.empty())
            return new App::DocumentObjectExecReturn("No tool objects");

        App::DocumentObject *feature;
        if (type == "Cut") {
            feature = tools.front();
            ++itBegin;
        }
        else {
            feature = tools.back();
            if (tools.size() > 1)
                --itEnd;
            else
                ++itBegin;
        }
        baseTopShape = getTopoShape(feature);
        if (baseTopShape.isNull()) {
            return new App::DocumentObjectExecReturn(
                    "Cannot do boolean operation with invalid base shape");
        }
    }
        
    std::vector<TopoShape> shapes;
    shapes.push_back(baseTopShape);
    for(auto it=itBegin; it<itEnd; ++it) {
        auto shape = getTopoShape(*it);
        if (shape.isNull())
            return new App::DocumentObjectExecReturn("Tool shape is null");
        shapes.push_back(shape);
    }

    if (shapes.size() == 1) {
        this->Shape.setValue(shapes.front());
        return App::DocumentObject::StdReturn;
    }

    const char *op = 0;
    if (type == "Fuse")
        op = TOPOP_FUSE;
    else if(type == "Cut")
        op = TOPOP_CUT;
    else if(type == "Common")
        op = TOPOP_COMMON;
    else if(type == "Compound")
        op = TOPOP_COMPOUND;
    else
        return new App::DocumentObjectExecReturn("Unsupported boolean operation");

    TopoShape result(0,getDocument()->getStringHasher());
    try {
        result.makEShape(op, shapes);
    } catch (Standard_Failure &e) {
        FC_ERR("Boolean operation failed: " << e.GetMessageString());
        return new App::DocumentObjectExecReturn("Boolean operation failed");
    }
    result = this->getSolid(result);
        // lets check if the result is a solid
    if (result.isNull())
        return new App::DocumentObjectExecReturn("Resulting shape is not a solid");

    if (this->Refine.getValue())
        result = result.makERefine();

    this->Shape.setValue(result);
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
