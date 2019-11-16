/***************************************************************************
 *   Copyright (c) 2019 WandererFan    <wandererfan@gmail.com>             *
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
# include <sstream>
#endif

#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepTools.hxx>
#include <BRepBuilderAPI_Copy.hxx>


#include <App/Application.h>
#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/Part.h>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PropertyTopoShape.h>

#include "ShapeExtractor.h"

using namespace TechDraw;

TopoDS_Shape ShapeExtractor::getShapes(const std::vector<App::DocumentObject*> links)
{
    TopoDS_Shape result;
    std::vector<TopoDS_Shape> sourceShapes;

    for (auto& l:links) {
        auto shape = Part::Feature::getShape(l);    //finds shape within DocObj??
        if(!shape.IsNull()) {
//            BRepTools::Write(shape, "DVPgetShape.brep");            //debug
            if (shape.ShapeType() > TopAbs_COMPSOLID)  {              //simple shape
                sourceShapes.push_back(shape);
            } else {                                                  //complex shape
                std::vector<TopoDS_Shape> drawable = extractDrawableShapes(shape);
                if (!drawable.empty()) {
                    sourceShapes.insert(sourceShapes.end(),drawable.begin(),drawable.end());
                }
            }
        } else {
            std::vector<TopoDS_Shape> shapeList = getShapesFromObject(l);
            sourceShapes.insert(sourceShapes.end(),shapeList.begin(),shapeList.end());
        }
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    bool found = false;
    for (auto& s:sourceShapes) {
        if (s.IsNull()) {
            continue;    //has no shape
        }
        found = true;
        BRepBuilderAPI_Copy BuilderCopy(s);
        TopoDS_Shape shape = BuilderCopy.Shape();
        builder.Add(comp, shape);
    }
    //it appears that an empty compound is !IsNull(), so we need to check a different way 
    //if we added anything to the compound.
    if (!found) {
        Base::Console().Error("SE::getSourceShapes - source shape is empty!\n");
    } else {
        result = comp;
    }
    return result;
}

std::vector<TopoDS_Shape> ShapeExtractor::getShapesFromObject(const App::DocumentObject* docObj)
{
//    Base::Console().Message("SE::getShapesFromObject(%s)\n", docObj->getNameInDocument());
    std::vector<TopoDS_Shape> result;
    
    const App::GroupExtension* gex = dynamic_cast<const App::GroupExtension*>(docObj);
    App::Property* gProp = docObj->getPropertyByName("Group");
    App::Property* sProp = docObj->getPropertyByName("Shape");
    if (docObj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        const Part::Feature* pf = static_cast<const Part::Feature*>(docObj);
        Part::TopoShape ts = pf->Shape.getShape();
        ts.setPlacement(pf->globalPlacement());
        result.push_back(ts.getShape());
    } else if (gex != nullptr) {           //is a group extension
        std::vector<App::DocumentObject*> objs = gex->Group.getValues();
        std::vector<TopoDS_Shape> shapes;
        for (auto& d: objs) {
            shapes = getShapesFromObject(d);
            if (!shapes.empty()) {
                result.insert(result.end(),shapes.begin(),shapes.end());
            }
        }
    //the next 2 bits are mostly for Arch module objects
    } else if (gProp != nullptr) {       //has a Group property
        App::PropertyLinkList* list = dynamic_cast<App::PropertyLinkList*>(gProp);
        if (list != nullptr) {
            std::vector<App::DocumentObject*> objs = list->getValues();
            std::vector<TopoDS_Shape> shapes;
            for (auto& d: objs) {
                shapes = getShapesFromObject(d);
                if (!shapes.empty()) {
                    result.insert(result.end(),shapes.begin(),shapes.end());
                }
            }
        } else {
                Base::Console().Log("SE::getShapesFromObject - Group is not a PropertyLinkList!\n");
        }
    } else if (sProp != nullptr) {       //has a Shape property
        Part::PropertyPartShape* shape = dynamic_cast<Part::PropertyPartShape*>(sProp);
        if (shape != nullptr) {
            TopoDS_Shape occShape = shape->getValue();
            result.push_back(occShape);
        } else {
            Base::Console().Log("SE::getShapesFromObject - Shape is not a PropertyPartShape!\n");
        }
    }
    return result;
}

TopoDS_Shape ShapeExtractor::getShapesFused(const std::vector<App::DocumentObject*> links)
{
//    Base::Console().Message("SE::getShapesFused()\n");
    TopoDS_Shape baseShape = getShapes(links);
    if (!baseShape.IsNull()) {
        TopoDS_Iterator it(baseShape);
        TopoDS_Shape fusedShape = it.Value();
        it.Next();
        for (; it.More(); it.Next()) {
            const TopoDS_Shape& aChild = it.Value();
            BRepAlgoAPI_Fuse mkFuse(fusedShape, aChild);
            // Let's check if the fusion has been successful
            if (!mkFuse.IsDone()) {
                Base::Console().Error("SE - Fusion failed\n");
                return baseShape;
            }
            fusedShape = mkFuse.Shape();
        }
        baseShape = fusedShape;
    }
    return baseShape;
}

std::vector<TopoDS_Shape> ShapeExtractor::extractDrawableShapes(const TopoDS_Shape shapeIn)
{
//    Base::Console().Message("SE::extractDrawableShapes()\n");
    std::vector<TopoDS_Shape> result;
    std::vector<TopoDS_Shape> extShapes;            //extracted Shapes (solids mostly)
    std::vector<TopoDS_Shape> extEdges;             //extracted loose Edges
    if (shapeIn.ShapeType() == TopAbs_COMPOUND) {          //Compound is most general shape type
        //getSolids from Compound
        TopExp_Explorer expSolid(shapeIn, TopAbs_SOLID);
        for (int i = 1; expSolid.More(); expSolid.Next(), i++) {
            TopoDS_Solid s = TopoDS::Solid(expSolid.Current());
            if (!s.IsNull()) {
                extShapes.push_back(s);
            }
        }
        //get edges not part of a solid
        //???? should this look for Faces(Wires?) before Edges?
        TopExp_Explorer expEdge(shapeIn, TopAbs_EDGE, TopAbs_SOLID);
        for (int i = 1; expEdge.More(); expEdge.Next(), i++) {
            TopoDS_Shape s = expEdge.Current();
            if (!s.IsNull()) {
                extEdges.push_back(s);
            }
        }
    } else if (shapeIn.ShapeType() == TopAbs_COMPSOLID) {
        //get Solids from compSolid
        TopExp_Explorer expSolid(shapeIn, TopAbs_SOLID);
        for (int i = 1; expSolid.More(); expSolid.Next(), i++) {
            TopoDS_Solid s = TopoDS::Solid(expSolid.Current());
            if (!s.IsNull()) {
                extShapes.push_back(s);
            }
        }
        //get edges not part of a solid
        //???? should this look for Faces(Wires?) before Edges?
        TopExp_Explorer expEdge(shapeIn, TopAbs_EDGE, TopAbs_SOLID);
        for (int i = 1; expEdge.More(); expEdge.Next(), i++) {
            TopoDS_Shape s = expEdge.Current();
            if (!s.IsNull()) {
                extEdges.push_back(s);
            }
        }
    } else {
        //not a Compound or a CompSolid just push_back shape_In)
        extShapes.push_back(shapeIn);
    }
    
    result = extShapes;
    if (!extEdges.empty()) {
        result.insert(std::end(result), std::begin(extEdges), std::end(extEdges));
    }
    return result;
}

