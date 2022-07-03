/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#include <BRep_Builder.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepTools.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/Part.h>
#include <App/Link.h>

#include <Base/BoundBox.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Base/Placement.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/FeaturePartCircle.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PropertyTopoShape.h>

#include "ShapeExtractor.h"
#include "DrawUtil.h"

using namespace TechDraw;

std::vector<TopoDS_Shape> ShapeExtractor::getShapes2d(const std::vector<App::DocumentObject*> links)
{
//    Base::Console().Message("SE::getShapes2d()\n");

    std::vector<TopoDS_Shape> shapes2d;
    if (!prefAdd2d()) {
        return shapes2d;
    }
    for (auto& l:links) {
        const App::GroupExtension* gex = dynamic_cast<const App::GroupExtension*>(l);
        if (gex != nullptr) {
            std::vector<App::DocumentObject*> objs = gex->Group.getValues();
            for (auto& d: objs) {
                if (is2dObject(d)) {
                    if (d->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                        //need to apply global placement here.  ??? because 2d shapes (Points so far)
                        //don't get gp from Part::feature::getShape() ????
                        const Part::Feature* pf = static_cast<const Part::Feature*>(d);
                        Part::TopoShape ts = pf->Shape.getShape();
                        ts.setPlacement(pf->globalPlacement());
                        shapes2d.push_back(ts.getShape());
                    }
                }
            }
        } else {
            if (is2dObject(l)) {
                if (l->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                    //need to apply placement here
                    const Part::Feature* pf = static_cast<const Part::Feature*>(l);
                    Part::TopoShape ts = pf->Shape.getShape();
                    ts.setPlacement(pf->globalPlacement());
                    shapes2d.push_back(ts.getShape());
                }
            }
        }
    }
    return shapes2d;
}

TopoDS_Shape ShapeExtractor::getShapes(const std::vector<App::DocumentObject*> links)
{
//    Base::Console().Message("SE::getShapes() - links in: %d\n", links.size());
    TopoDS_Shape result;
    std::vector<TopoDS_Shape> sourceShapes;

    for (auto& l:links) {
        if (l->getTypeId().isDerivedFrom(App::Link::getClassTypeId())) {
            App::Link* xLink = dynamic_cast<App::Link*>(l);
            std::vector<TopoDS_Shape> xShapes = getXShapes(xLink);
            if (!xShapes.empty()) {
                sourceShapes.insert(sourceShapes.end(), xShapes.begin(), xShapes.end());
                continue;
            }
        } else {
            auto shape = Part::Feature::getShape(l);
            if(!shape.IsNull()) {
                sourceShapes.push_back(shape);
            } else {
                std::vector<TopoDS_Shape> shapeList = getShapesFromObject(l);
                sourceShapes.insert(sourceShapes.end(),shapeList.begin(),shapeList.end());
            }
        }
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    bool found = false;
    for (auto& s:sourceShapes) {
        if (s.ShapeType() < TopAbs_SOLID) {
            //clean up composite shapes
            TopoDS_Shape cleanShape = stripInfiniteShapes(s);
            if (!cleanShape.IsNull()) {
                builder.Add(comp, cleanShape);
                found = true;
            }
        } else if (s.IsNull()) {
            continue;    // has no shape
        } else if (Part::TopoShape(s).isInfinite()) {
            continue;    //shape is infinite
        } else {
            //a simple shape - add to compound
            builder.Add(comp, s);
            found = true;
        }
    }
    //it appears that an empty compound is !IsNull(), so we need to check a different way
    //if we added anything to the compound.
    if (!found) {
        Base::Console().Error("SE::getShapes - source shape is empty!\n");
    } else {
        result = comp;
    }
//    BRepTools::Write(result, "SEresult.brep");            //debug
    return result;
}

std::vector<TopoDS_Shape> ShapeExtractor::getXShapes(const App::Link* xLink)
{
//    Base::Console().Message("SE::getXShapes(%X) - %s\n", xLink, xLink->getNameInDocument());
    std::vector<TopoDS_Shape> xSourceShapes;
    if (xLink == nullptr) {
        return xSourceShapes;
    }

    bool needsTransform = false;
    std::vector<App::DocumentObject*> children = xLink->getLinkedChildren();
    Base::Placement linkPlm;  // default constructor is an identity placement, i.e. no rotation nor translation
    if (xLink->hasPlacement()) {
        linkPlm = xLink->getLinkPlacementProperty()->getValue();
        needsTransform = true;
    }
    Base::Matrix4D linkScale;  // default constructor is an identity matrix, possibly scale it with link's scale
    if(xLink->getScaleProperty() || xLink->getScaleVectorProperty()) {
        linkScale.scale(xLink->getScaleVector());
        needsTransform = true;
    }

    Base::Matrix4D netTransform;
    if (!children.empty()) {
        for (auto& l:children) {
            bool childNeedsTransform = false;
            Base::Placement childPlm;
            Base::Matrix4D childScale;
            if (l->getTypeId().isDerivedFrom(App::LinkElement::getClassTypeId())) {
                App::LinkElement* cLinkElem = static_cast<App::LinkElement*>(l);
                if (cLinkElem->hasPlacement()) {
                    childPlm = cLinkElem->getLinkPlacementProperty()->getValue();
                    childNeedsTransform = true;
                }
                if(cLinkElem->getScaleProperty() || cLinkElem->getScaleVectorProperty()) {
                    childScale.scale(cLinkElem->getScaleVector());
                    childNeedsTransform = true;
                }
            }
            auto shape = Part::Feature::getShape(l);
            if(!shape.IsNull()) {
                if (needsTransform || childNeedsTransform) {
                    // Multiplication is associative, but the braces show the idea of combining the two transforms:
                    // ( link placement and scale ) combined to ( child placement and scale )
                    netTransform = (linkPlm.toMatrix() * linkScale) * (childPlm.toMatrix() * childScale);
                    Part::TopoShape ts(shape);
                    ts.transformGeometry(netTransform);
                    shape = ts.getShape();
                }
                xSourceShapes.push_back(shape);
            } else {
                Base::Console().Message("SE::getXShapes - no shape from getXShape\n");
            }
        }
    } else {
        int depth = 1;   //0 is default value, related to recursion of Links???
        App::DocumentObject* link = xLink->getLink(depth);
        if (link != nullptr) {
            auto shape = Part::Feature::getShape(link);
            if(!shape.IsNull()) {
                if (needsTransform) {
                    // Transform is just link placement and scale, no child objects
                    netTransform = linkPlm.toMatrix() * linkScale;
                    Part::TopoShape ts(shape);
                    ts.transformGeometry(netTransform);
                    shape = ts.getShape();
                }
                xSourceShapes.push_back(shape);
            }
        }
    }
    return xSourceShapes;
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

//inShape is a compound
//The shapes of datum features (Axis, Plan and CS) are infinite
//Infinite shapes can not be projected, so they need to be removed.
TopoDS_Shape ShapeExtractor::stripInfiniteShapes(TopoDS_Shape inShape)
{
//    Base::Console().Message("SE::stripInfiniteShapes() - shapeType: %d\n", inShape.ShapeType());
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    TopoDS_Iterator it(inShape);
    for (; it.More(); it.Next()) {
        TopoDS_Shape s = it.Value();
        if (s.ShapeType() < TopAbs_SOLID) {
            //look inside composite shapes
            s = stripInfiniteShapes(s);
        } else if (Part::TopoShape(s).isInfinite()) {
            continue;
        } else {
            //simple shape
        }
        builder.Add(comp, s);
    }
    return TopoDS_Shape(std::move(comp));
}

bool ShapeExtractor::is2dObject(App::DocumentObject* obj)
{
    bool result = false;
    if (isEdgeType(obj) || isPointType(obj)) {
        result = true;
    }
    return result;
}

//skip edges for now.
bool ShapeExtractor::isEdgeType(App::DocumentObject* obj)
{
    (void) obj;
    bool result = false;
//    Base::Type t = obj->getTypeId();
//    if (t.isDerivedFrom(Part::Line::getClassTypeId()) ) {
//        result = true;
//    } else if (t.isDerivedFrom(Part::Circle::getClassTypeId())) {
//        result = true;
//    } else if (t.isDerivedFrom(Part::Ellipse::getClassTypeId())) {
//        result = true;
//    } else if (t.isDerivedFrom(Part::RegularPolygon::getClassTypeId())) {
//        result = true;
//    }
    return result;
}

bool ShapeExtractor::isPointType(App::DocumentObject* obj)
{
//    Base::Console().Message("SE::isPointType(%s)\n", obj->getNameInDocument());
    bool result = false;
    if (obj) {
        Base::Type t = obj->getTypeId();
        if (t.isDerivedFrom(Part::Vertex::getClassTypeId())) {
            result = true;
        } else if (isDraftPoint(obj)) {
            result = true;
        }
    }
    return result;
}

bool ShapeExtractor::isDraftPoint(App::DocumentObject* obj)
{
//    Base::Console().Message("SE::isDraftPoint()\n");
    bool result = false;
    //if the docObj doesn't have a Proxy property, it definitely isn't a Draft point
    App::PropertyPythonObject* proxy = dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));
    if (proxy != nullptr) {
        std::string  pp = proxy->toString();
//        Base::Console().Message("SE::isDraftPoint - pp: %s\n", pp.c_str());
        if (pp.find("Point") != std::string::npos) {
            result = true;
        }
    }
    return result;
}

Base::Vector3d ShapeExtractor::getLocation3dFromFeat(App::DocumentObject* obj)
{
//    Base::Console().Message("SE::getLocation3dFromFeat()\n");
    Base::Vector3d result(0.0, 0.0, 0.0);
    if (!isPointType(obj)) {
        return result;
    }
//    if (isDraftPoint(obj) {
//        //Draft Points are not necc. Part::PartFeature??
//        //if Draft option "use part primitives" is not set are Draft points still PartFeature?

    Part::Feature* pf = dynamic_cast<Part::Feature*>(obj);
    if (pf != nullptr) {
        Part::TopoShape pts = pf->Shape.getShape();
        pts.setPlacement(pf->globalPlacement());
        TopoDS_Shape ts = pts.getShape();
        if (ts.ShapeType() == TopAbs_VERTEX)  {
            TopoDS_Vertex v = TopoDS::Vertex(ts);
            result = DrawUtil::vertex2Vector(v);
        }
    }

//    Base::Console().Message("SE::getLocation3dFromFeat - returns: %s\n",
//                            DrawUtil::formatVector(result).c_str());
    return result;
}

bool ShapeExtractor::prefAdd2d(void)
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
          .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    bool result = hGrp->GetBool("ShowLoose2d", false);
    return result;
}

