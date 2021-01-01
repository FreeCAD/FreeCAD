/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAlgo.hxx>
# include <BRepFilletAPI_MakeFillet.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_ListOfShape.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
# include <BRep_Tool.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureFillet.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Fillet, PartDesign::DressUp)

const App::PropertyQuantityConstraint::Constraints floatRadius = {0.0,FLT_MAX,0.1};

Fillet::Fillet()
{
    ADD_PROPERTY(Radius,(1.0));
    Radius.setUnit(Base::Unit::Length);
    Radius.setConstraints(&floatRadius);
}

short Fillet::mustExecute() const
{
    if (Placement.isTouched() || Radius.isTouched())
        return 1;
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Fillet::execute(void)
{
    Part::TopoShape TopShape;
    try {
        TopShape = getBaseShape();
    } catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    std::vector<std::string> SubNames = std::vector<std::string>(Base.getSubValues());
    getContiniusEdges(TopShape, SubNames);

    if (SubNames.size() == 0)
        return new App::DocumentObjectExecReturn("Fillet not possible on selected shapes");
    
    double radius = Radius.getValue();
    
    if(radius <= 0)
        return new App::DocumentObjectExecReturn("Fillet radius must be greater than zero");

    this->positionByBaseFeature();

    // create an untransformed copy of the base shape
    Part::TopoShape baseShape(TopShape);
    baseShape.setTransform(Base::Matrix4D());
    try {
        BRepFilletAPI_MakeFillet mkFillet(baseShape.getShape());

        for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
            TopoDS_Edge edge = TopoDS::Edge(baseShape.getSubShape(it->c_str()));
            mkFillet.Add(radius, edge);
        }

        mkFillet.Build();
        if (!mkFillet.IsDone())
            return new App::DocumentObjectExecReturn("Failed to create fillet");

        TopoDS_Shape shape = mkFillet.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");

        TopTools_ListOfShape aLarg;
        aLarg.Append(baseShape.getShape());
        if (!BRepAlgo::IsValid(aLarg, shape, Standard_False, Standard_False)) {
            ShapeFix_ShapeTolerance aSFT;
            aSFT.LimitTolerance(shape, Precision::Confusion(), Precision::Confusion(), TopAbs_SHAPE);
            Handle(ShapeFix_Shape) aSfs = new ShapeFix_Shape(shape);
            aSfs->Perform();
            shape = aSfs->Shape();
            if (!BRepAlgo::IsValid(aLarg, shape, Standard_False, Standard_False)) {
                return new App::DocumentObjectExecReturn("Resulting shape is invalid");
            }
        }

        int solidCount = countSolids(shape);
        if (solidCount > 1) {
            return new App::DocumentObjectExecReturn("Fillet: Result has multiple solids. This is not supported at this time.");
        }

        shape = refineShapeIfActive(shape);
        this->Shape.setValue(getSolid(shape));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

void Fillet::Restore(Base::XMLReader &reader)
{
    reader.readElement("Properties");
    int Cnt = reader.getAttributeAsInteger("Count");

    for (int i=0 ;i<Cnt ;i++) {
        reader.readElement("Property");
        const char* PropName = reader.getAttribute("name");
        const char* TypeName = reader.getAttribute("type");
        App::Property* prop = getPropertyByName(PropName);

        try {
            if (prop && strcmp(prop->getTypeId().getName(), TypeName) == 0) {
                prop->Restore(reader);
            }
            else if (prop && strcmp(TypeName,"App::PropertyFloatConstraint") == 0 &&
                     strcmp(prop->getTypeId().getName(), "App::PropertyQuantityConstraint") == 0) {
                App::PropertyFloatConstraint p;
                p.Restore(reader);
                static_cast<App::PropertyQuantityConstraint*>(prop)->setValue(p.getValue());
            }
        }
        catch (const Base::XMLParseException&) {
            throw; // re-throw
        }
        catch (const Base::Exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        catch (const std::exception &e) {
            Base::Console().Error("%s\n", e.what());
        }
        reader.readEndElement("Property");
    }
    reader.readEndElement("Properties");
}
