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
# include <BRep_Builder.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepTools.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopoDS_Vertex.hxx>
#endif

#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/Link.h>
#include <App/Part.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Placement.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PrimitiveFeature.h>

#include "ShapeExtractor.h"
#include "DrawUtil.h"
#include "Preferences.h"


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
        if (gex) {
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
                sourceShapes.insert(sourceShapes.end(), shapeList.begin(), shapeList.end());
            }
        }
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    bool found = false;
    for (auto& s:sourceShapes) {
        if (s.IsNull()) {
            continue;
        } else if (s.ShapeType() < TopAbs_SOLID) {
            //clean up composite shapes
            TopoDS_Shape cleanShape = stripInfiniteShapes(s);
            if (!cleanShape.IsNull()) {
                builder.Add(comp, cleanShape);
                found = true;
            }
        } else if (Part::TopoShape(s).isInfinite()) {
            continue;    //simple shape is infinite
        } else {
            //a simple shape - add to compound
            builder.Add(comp, s);
            found = true;
        }
    }
    //it appears that an empty compound is !IsNull(), so we need to check a different way
    //if we added anything to the compound.
    if (found) {
//    BRepTools::Write(comp, "SEResult.brep");            //debug
        return comp;
    }

    Base::Console().Error("ShapeExtractor failed to get shape.\n");
    return TopoDS_Shape();
}

std::vector<TopoDS_Shape> ShapeExtractor::getXShapes(const App::Link* xLink)
{
//    Base::Console().Message("SE::getXShapes() - %s\n", xLink->getNameInDocument());
    std::vector<TopoDS_Shape> xSourceShapes;
    if (!xLink) {
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
            Base::Console().Message("SE::getXShapes - processing a child\n");
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
            Part::TopoShape ts(shape);
            if (ts.isInfinite()) {
                shape = stripInfiniteShapes(shape);
                ts = Part::TopoShape(shape);
            }
            if(!shape.IsNull()) {
                if (needsTransform || childNeedsTransform) {
                    // Multiplication is associative, but the braces show the idea of combining the two transforms:
                    // ( link placement and scale ) combined to ( child placement and scale )
                    netTransform = (linkPlm.toMatrix() * linkScale) * (childPlm.toMatrix() * childScale);
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
        if (link) {
            auto shape = Part::Feature::getShape(link);
            Part::TopoShape ts(shape);
            if (ts.isInfinite()) {
                shape = stripInfiniteShapes(shape);
                ts = Part::TopoShape(shape);
            }
            if(!shape.IsNull()) {
                if (needsTransform) {
                    // Transform is just link placement and scale, no child objects
                    netTransform = linkPlm.toMatrix() * linkScale;
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
        Part::TopoShape ts(pf->Shape.getShape());

        //ts might be garbage, better check
        try {
            if (!ts.isNull()) {
                ts.setPlacement(pf->globalPlacement());
                result.push_back(ts.getShape());
            }
        }
        catch (Standard_Failure& e) {
            Base::Console().Error("ShapeExtractor - %s encountered OCC error: %s \n", docObj->getNameInDocument(), e.GetMessageString());
            return result;
        }
        catch (...) {
            Base::Console().Error("ShapeExtractor failed to retrieve shape from %s\n", docObj->getNameInDocument());
            return result;
        }

    } else if (gex) {           //is a group extension
        std::vector<App::DocumentObject*> objs = gex->Group.getValues();
        std::vector<TopoDS_Shape> shapes;
        for (auto& d: objs) {
            shapes = getShapesFromObject(d);
            if (!shapes.empty()) {
                result.insert(result.end(), shapes.begin(), shapes.end());
            }
        }
    //the next 2 bits are mostly for Arch module objects
    } else if (gProp) {       //has a Group property
        App::PropertyLinkList* list = dynamic_cast<App::PropertyLinkList*>(gProp);
        if (list) {
            std::vector<App::DocumentObject*> objs = list->getValues();
            std::vector<TopoDS_Shape> shapes;
            for (auto& d: objs) {
                shapes = getShapesFromObject(d);
                if (!shapes.empty()) {
                    result.insert(result.end(), shapes.begin(), shapes.end());
                }
            }
        }
    } else if (sProp) {       //has a Shape property
        Part::PropertyPartShape* shape = dynamic_cast<Part::PropertyPartShape*>(sProp);
        if (shape) {
            TopoDS_Shape occShape = shape->getValue();
            result.push_back(occShape);
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
//    Base::Console().Message("SE::stripInfiniteShapes()\n");
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
    if (obj) {
        Base::Type t = obj->getTypeId();
        if (t.isDerivedFrom(Part::Vertex::getClassTypeId())) {
            return true;
        } else if (isDraftPoint(obj)) {
            return true;
        }
    }
    return false;
}

bool ShapeExtractor::isDraftPoint(App::DocumentObject* obj)
{
//    Base::Console().Message("SE::isDraftPoint()\n");
    //if the docObj doesn't have a Proxy property, it definitely isn't a Draft point
    App::PropertyPythonObject* proxy = dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));
    if (proxy) {
        std::string  pp = proxy->toString();
//        Base::Console().Message("SE::isDraftPoint - pp: %s\n", pp.c_str());
        if (pp.find("Point") != std::string::npos) {
            return true;
        }
    }
    return false;
}

Base::Vector3d ShapeExtractor::getLocation3dFromFeat(App::DocumentObject* obj)
{
//    Base::Console().Message("SE::getLocation3dFromFeat()\n");
    if (!isPointType(obj)) {
        return Base::Vector3d(0.0, 0.0, 0.0);
    }
//    if (isDraftPoint(obj) {
//        //Draft Points are not necc. Part::PartFeature??
//        //if Draft option "use part primitives" is not set are Draft points still PartFeature?

    Part::Feature* pf = dynamic_cast<Part::Feature*>(obj);
    if (pf) {
        Part::TopoShape pts = pf->Shape.getShape();
        pts.setPlacement(pf->globalPlacement());
        TopoDS_Shape ts = pts.getShape();
        if (ts.ShapeType() == TopAbs_VERTEX)  {
            TopoDS_Vertex v = TopoDS::Vertex(ts);
            return DrawUtil::vertex2Vector(v);
        }
    }

//    Base::Console().Message("SE::getLocation3dFromFeat - returns: %s\n",
//                            DrawUtil::formatVector(result).c_str());
    return Base::Vector3d(0.0, 0.0, 0.0);
}

bool ShapeExtractor::prefAdd2d()
{
    return Preferences::getPreferenceGroup("General")->GetBool("ShowLoose2d", false);
}

