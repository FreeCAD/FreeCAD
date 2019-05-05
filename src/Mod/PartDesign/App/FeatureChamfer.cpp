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
# include <BRep_Tool.hxx>
# include <ShapeFix_Shape.hxx>
# include <ShapeFix_ShapeTolerance.hxx>
# include <Standard_Version.hxx>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Reader.h>
#include <App/Document.h>
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
    // NOTE: Normally the Base property and the BaseFeature property should point to the same object.
    // The only difference is that the Base property also stores the edges that are to be chamfered
    Part::TopoShape baseShape;
    try {
        baseShape = getBaseShape();
    } catch (Base::Exception& e) {
        return new App::DocumentObjectExecReturn(e.what());
    }
    baseShape.setTransform(Base::Matrix4D());

    auto edges = getContiniusEdges(baseShape);

    if (edges.size() == 0)
        return new App::DocumentObjectExecReturn("No edges specified");

    double size = Size.getValue();
    if (size <= 0)
        return new App::DocumentObjectExecReturn("Size must be greater than zero");

    this->positionByBaseFeature();
    try {
        TopoShape shape(0,getDocument()->getStringHasher());
        shape.makEChamfer(baseShape,edges,size,size);
        if (shape.isNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");

        TopTools_ListOfShape aLarg;
        aLarg.Append(baseShape.getShape());
        if (!BRepAlgo::IsValid(aLarg, shape.getShape(), Standard_False, Standard_False)) {
            ShapeFix_ShapeTolerance aSFT;
            aSFT.LimitTolerance(shape.getShape(), Precision::Confusion(), Precision::Confusion(), TopAbs_SHAPE);
            Handle(ShapeFix_Shape) aSfs = new ShapeFix_Shape(shape.getShape());
            aSfs->Perform();
            shape.setShape(aSfs->Shape(),false);
            if (!BRepAlgo::IsValid(aLarg, shape.getShape(), Standard_False, Standard_False)) {
                return new App::DocumentObjectExecReturn("Resulting shape is invalid");
            }
        }

        this->Shape.setValue(getSolid(shape));
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

void Chamfer::handleChangedPropertyType(
        Base::XMLReader &reader, const char * TypeName, App::Property * prop) 
{
    if (prop && strcmp(TypeName,"App::PropertyFloatConstraint") == 0 &&
            strcmp(prop->getTypeId().getName(), "App::PropertyQuantityConstraint") == 0) 
    {
        App::PropertyFloatConstraint p;
        p.Restore(reader);
        static_cast<App::PropertyQuantityConstraint*>(prop)->setValue(p.getValue());
    }
}
