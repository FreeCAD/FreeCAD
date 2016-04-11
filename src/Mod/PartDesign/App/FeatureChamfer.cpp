/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <BRepFilletAPI_MakeChamfer.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
# include <TopTools_ListOfShape.hxx>
#endif

#include <Base/Console.h>
#include <Base/Reader.h>
#include <Mod/Part/App/TopoShape.h>

#include "FeatureChamfer.h"


using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Chamfer, PartDesign::DressUp)

const App::PropertyQuantityConstraint::Constraints floatSize = {0.0,FLT_MAX,0.1};

Chamfer::Chamfer()
{
    ADD_PROPERTY(Size,(1.0));
    Size.setUnit(Base::Unit::Length);
    Size.setConstraints(&floatSize);
}

short Chamfer::mustExecute() const
{
    if (Placement.isTouched() || Size.isTouched())
        return 1;
    return DressUp::mustExecute();
}

App::DocumentObjectExecReturn *Chamfer::execute(void)
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    Part::Feature *base = static_cast<Part::Feature*>(Base.getValue());
    const Part::TopoShape& TopShape = base->Shape.getShape();
    if (TopShape._Shape.IsNull())
        return new App::DocumentObjectExecReturn("Cannot chamfer invalid shape");

    const std::vector<std::string>& SubVals = Base.getSubValuesStartsWith("Edge");
    if (SubVals.size() == 0)
        return new App::DocumentObjectExecReturn("No edges specified");

    double size = Size.getValue();

    this->positionByBase();
    // create an untransformed copy of the base shape
    Part::TopoShape baseShape(TopShape);
    baseShape.setTransform(Base::Matrix4D());
    try {
        BRepFilletAPI_MakeChamfer mkChamfer(baseShape._Shape);

        TopTools_IndexedMapOfShape mapOfEdges;
        TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
        TopExp::MapShapesAndAncestors(baseShape._Shape, TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
        TopExp::MapShapes(baseShape._Shape, TopAbs_EDGE, mapOfEdges);

        for (std::vector<std::string>::const_iterator it=SubVals.begin(); it != SubVals.end(); ++it) {
            TopoDS_Edge edge = TopoDS::Edge(baseShape.getSubShape(it->c_str()));
            const TopoDS_Face& face = TopoDS::Face(mapEdgeFace.FindFromKey(edge).First());
            mkChamfer.Add(size, edge, face);
        }

        mkChamfer.Build();
        if (!mkChamfer.IsDone())
            return new App::DocumentObjectExecReturn("Failed to create chamfer");

        TopoDS_Shape shape = mkChamfer.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");

        TopTools_ListOfShape aLarg;
        aLarg.Append(baseShape._Shape);
        if (!BRepAlgo::IsValid(aLarg, shape, Standard_False, Standard_False)) {
            return new App::DocumentObjectExecReturn("Resulting shape is invalid");
        }

        this->Shape.setValue(shape);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}

void Chamfer::Restore(Base::XMLReader &reader)
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
