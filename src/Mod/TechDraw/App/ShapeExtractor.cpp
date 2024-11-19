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
# include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
# include <BRepTools.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopoDS_Vertex.hxx>
# include <BRepBuilderAPI_Copy.hxx>
#endif

#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/FeaturePythonPyImp.h>
#include <App/PropertyPythonObject.h>
#include <App/Link.h>
#include <App/Part.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Placement.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PrimitiveFeature.h>
#include <Mod/Part/App/FeaturePartCircle.h>
#include <Mod/Part/App/TopoShapePy.h>
//#include <Mod/Sketcher/App/SketchObject.h>

#include "ShapeExtractor.h"
#include "DrawUtil.h"
#include "ShapeUtils.h"


using namespace TechDraw;
using DU = DrawUtil;
using SU = ShapeUtils;


//! pick out the 2d document objects in the list of links and return a vector of their shapes
//! Note that point objects will not make it through the hlr/projection process.
std::vector<TopoDS_Shape> ShapeExtractor::getShapes2d(const std::vector<App::DocumentObject*> links)
{
//    Base::Console().Message("SE::getShapes2d() - links: %d\n", links.size());

    std::vector<TopoDS_Shape> shapes2d;

    for (auto& l:links) {
        if (is2dObject(l)) {
            if (l->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
                TopoDS_Shape temp = getLocatedShape(l);
                if (!temp.IsNull()) {
                    shapes2d.push_back(temp);
                }
            }  // other 2d objects would go here - Draft objects? Arch Axis?
        }
    }
    return shapes2d;
}

//! get the located and oriented shapes corresponding to the the links. If the shapes are to be
//! fused, include2d should be false as 2d & 3d shapes may not fuse.
TopoDS_Shape ShapeExtractor::getShapes(const std::vector<App::DocumentObject*> links, bool include2d)
{
    // Base::Console().Message("SE::getShapes() - links in: %d\n", links.size());
    std::vector<TopoDS_Shape> sourceShapes;

    for (auto& l:links) {
        if (is2dObject(l) && !include2d) {
            // Base::Console().Message("SE::getShapes - skipping 2d link: %s\n", l->Label.getValue());
            continue;
        }

        // Copy the pointer as not const so it can be changed if needed.
        App::DocumentObject* obj = l;

        bool isExplodedView = false;
        auto proxy = dynamic_cast<App::PropertyPythonObject*>(l->getPropertyByName("Proxy"));
        Base::PyGILStateLocker lock;
        if (proxy && proxy->getValue().hasAttr("saveAssemblyAndExplode")) {
            isExplodedView = true;

            Py::Object explodedViewPy = proxy->getValue();
            Py::Object attr = explodedViewPy.getAttr("saveAssemblyAndExplode");

            if (attr.ptr() && attr.isCallable()) {
                Py::Tuple args(1);
                args.setItem(0, Py::asObject(l->getPyObject()));
                Py::Callable methode(attr);
                Py::Object pyResult = methode.apply(args);

                if (PyObject_TypeCheck(pyResult.ptr(), &(Part::TopoShapePy::Type))) {
                    auto* shapepy = static_cast<Part::TopoShapePy*>(pyResult.ptr());
                    const TopoDS_Shape& shape = shapepy->getTopoShapePtr()->getShape();
                    sourceShapes.push_back(shape);
                }
            }

            for (auto* inObj : l->getInList()) {
                if (inObj->isDerivedFrom(App::Part::getClassTypeId())) {
                    // we replace obj by the assembly
                    obj = inObj;
                    break;
                }
            }
        }

        if (obj->isDerivedFrom<App::Link>()) {
            App::Link* xLink = dynamic_cast<App::Link*>(obj);
            std::vector<TopoDS_Shape> xShapes = getXShapes(xLink);
            if (!xShapes.empty()) {
                sourceShapes.insert(sourceShapes.end(), xShapes.begin(), xShapes.end());
                continue;
            }
        }
        else {
            auto shape = Part::Feature::getShape(obj);
            // if link obj has a shape, we use that shape.
            if(!SU::isShapeReallyNull(shape) && !isExplodedView) {
                sourceShapes.push_back(getLocatedShape(obj));
            }
            else {
                std::vector<TopoDS_Shape> shapeList = getShapesFromObject(obj);
                sourceShapes.insert(sourceShapes.end(), shapeList.begin(), shapeList.end());
            }
        }

        if (isExplodedView) {
            Py::Object explodedViewPy = proxy->getValue();

            Py::Object attr = explodedViewPy.getAttr("restoreAssembly");
            if (attr.ptr() && attr.isCallable()) {
                Py::Tuple args(1);
                args.setItem(0, Py::asObject(l->getPyObject()));
                Py::Callable(attr).apply(args);
            }
        }
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    for (auto& s:sourceShapes) {
        if (SU::isShapeReallyNull(s)) {
            continue;
        } else if (s.ShapeType() < TopAbs_SOLID) {
            //clean up composite shapes
            TopoDS_Shape cleanShape = stripInfiniteShapes(s);
            if (!cleanShape.IsNull()) {
                builder.Add(comp, cleanShape);
            }
        } else if (Part::TopoShape(s).isInfinite()) {
            continue;    //simple shape is infinite
        } else {
            //a simple shape - add to compound
            builder.Add(comp, s);
        }
    }
    //it appears that an empty compound is !IsNull(), so we need to check a different way
    if (!SU::isShapeReallyNull(comp)) {
//    BRepTools::Write(comp, "SEResult.brep");            //debug
        return comp;
    }

//    Base::Console().Error("DEVEL: ShapeExtractor failed to get any shape.\n");
    return TopoDS_Shape();
}

std::vector<TopoDS_Shape> ShapeExtractor::getXShapes(const App::Link* xLink)
{
    // Base::Console().Message("SE::getXShapes() - %s\n", xLink->getNameInDocument());
    std::vector<TopoDS_Shape> xSourceShapes;
    if (!xLink) {
        return xSourceShapes;
    }

    bool needsTransform = false;
    std::vector<App::DocumentObject*> children = xLink->getLinkedChildren();
    Base::Placement xLinkPlacement;  // default constructor is an identity placement, i.e. no rotation nor translation
    if (xLink->hasPlacement()) {
        xLinkPlacement = xLink->getLinkPlacementProperty()->getValue();
        needsTransform = true;
    }
    Base::Matrix4D linkScale;  // default constructor is an identity matrix, possibly scale it with link's scale
    if(xLink->getScaleProperty() || xLink->getScaleVectorProperty()) {
        linkScale.scale(xLink->getScaleVector());
        needsTransform = true;
    }

    Base::Matrix4D netTransform;
    if (!children.empty()) {
        // this link points to other links???
        for (auto& l:children) {
            bool childNeedsTransform = false;
            Base::Placement childPlm;
            Base::Matrix4D childScale;
            if (l->isDerivedFrom<App::LinkElement>()) {
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
            auto shape = Part::Feature::getShape(l);    // TODO:  getTopoShape() ?
            Part::TopoShape ts(shape);
            if (ts.isInfinite()) {
                shape = stripInfiniteShapes(shape);
            }
            // copying the shape prevents "non-orthogonal GTrsf" errors in some versions
            // of OCC.  Something to do with triangulation of shape??
            // it may be that incremental mesh would work here too.
            BRepBuilderAPI_Copy copier(shape);
            ts = Part::TopoShape(copier.Shape());
            if(!ts.isNull()) {
                if (needsTransform || childNeedsTransform) {
                    // Multiplication is associative, but the braces show the idea of combining the two transforms:
                    // ( link placement and scale ) combined to ( child placement and scale )
                    netTransform = (xLinkPlacement.toMatrix() * linkScale) * (childPlm.toMatrix() * childScale);
                    ts.transformGeometry(netTransform);
                    shape = ts.getShape();
                }
                xSourceShapes.push_back(shape);
            } else {
                Base::Console().Message("SE::getXShapes - no shape from getXShape\n");
            }
        }
    } else {
        // link points to a regular object, not another link? no sublinks?
        TopoDS_Shape xLinkShape = getShapeFromXLink(xLink);
        if (!xLinkShape.IsNull()) {
            // copying the shape prevents "non-orthogonal GTrsf" errors in some versions
            // of OCC.  Something to do with triangulation of shape??
            BRepBuilderAPI_Copy copier(xLinkShape);
            xSourceShapes.push_back(copier.Shape());
        }
    }
    return xSourceShapes;
}

// get the located shape for a single childless App::Link
TopoDS_Shape ShapeExtractor::getShapeFromXLink(const App::Link* xLink)
{
    // Base::Console().Message("SE::getShapeFromXLink()\n");
    Base::Placement xLinkPlacement;
    if (xLink->hasPlacement()) {
        xLinkPlacement = xLink->getLinkPlacementProperty()->getValue();
    }
    Base::Matrix4D linkScale;  // default constructor is an identity matrix, possibly scale it with link's scale
    if(xLink->getScaleProperty() || xLink->getScaleVectorProperty()) {
        linkScale.scale(xLink->getScaleVector());
    }
    int depth = 0;   //0 is default value, related to recursion of Links???
    App::DocumentObject* linkedObject = xLink->getLink(depth);
    if (linkedObject) {
        // have a linked object, get the shape
        TopoDS_Shape shape = Part::Feature::getShape(linkedObject);
        if (shape.IsNull()) {
            // this is where we need to parse the target for objects with a shape??
            return TopoDS_Shape();
        }
        Part::TopoShape ts(shape);
        if (ts.isInfinite()) {
            shape = stripInfiniteShapes(shape);
            ts = Part::TopoShape(shape);
        }
        //ts might be garbage now, better check
        try {
            if (!ts.isNull()) {
                ts.setPlacement(xLinkPlacement);
            }
        }
        catch (...) {
            Base::Console().Error("ShapeExtractor failed to retrieve shape from %s\n", xLink->getNameInDocument());
            return TopoDS_Shape();
        }
        return ts.getShape();
    }
    return TopoDS_Shape();
}

std::vector<TopoDS_Shape> ShapeExtractor::getShapesFromObject(const App::DocumentObject* docObj)
{
//    Base::Console().Message("SE::getShapesFromObject(%s)\n", docObj->getNameInDocument());
    std::vector<TopoDS_Shape> result;

    const App::GroupExtension* gex = dynamic_cast<const App::GroupExtension*>(docObj);
    App::Property* gProp = docObj->getPropertyByName("Group");
    App::Property* sProp = docObj->getPropertyByName("Shape");
    if (docObj->isDerivedFrom<Part::Feature>()) {
        result.push_back(getLocatedShape(docObj));
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
            result.push_back(getLocatedShape(docObj));
        }
    }
    return result;
}

TopoDS_Shape ShapeExtractor::getShapesFused(const std::vector<App::DocumentObject*> links)
{
//    Base::Console().Message("SE::getShapesFused()\n");
    // get only the 3d shapes and fuse them
    TopoDS_Shape baseShape = getShapes(links, false);
    if (!baseShape.IsNull()) {
        TopoDS_Iterator it(baseShape);
        TopoDS_Shape fusedShape = it.Value();
        it.Next();
        for (; it.More(); it.Next()) {
            const TopoDS_Shape& aChild = it.Value();
            FCBRepAlgoAPI_Fuse mkFuse(fusedShape, aChild);
            // Let's check if the fusion has been successful
            if (!mkFuse.IsDone()) {
                Base::Console().Error("SE - Fusion failed\n");
                return baseShape;
            }
            fusedShape = mkFuse.Shape();
        }
        baseShape = fusedShape;
    }
    BRepTools::Write(baseShape, "SEbaseShape.brep");

    // if there are 2d shapes in the links they will not fuse with the 3d shapes,
    // so instead we return a compound of the fused 3d shapes and the 2d shapes
    std::vector<TopoDS_Shape> shapes2d = getShapes2d(links);
    BRepTools::Write(DrawUtil::shapeVectorToCompound(shapes2d, false), "SEshapes2d.brep");

    if (!shapes2d.empty()) {
        shapes2d.push_back(baseShape);
        return DrawUtil::shapeVectorToCompound(shapes2d, false);
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

bool ShapeExtractor::is2dObject(const App::DocumentObject* obj)
{
    if (isSketchObject(obj)) {
        return true;
    }

    if (isEdgeType(obj) || isPointType(obj)) {
        return true;
    }
    return false;
}

// just these for now
bool ShapeExtractor::isEdgeType(const App::DocumentObject* obj)
{
    Base::Type t = obj->getTypeId();
    if (t.isDerivedFrom(Part::Line::getClassTypeId()) ) {
        return true;
    } else if (t.isDerivedFrom(Part::Circle::getClassTypeId())) {
        return true;
    } else if (t.isDerivedFrom(Part::Ellipse::getClassTypeId())) {
        return true;
    } else if (t.isDerivedFrom(Part::RegularPolygon::getClassTypeId())) {
        return true;
    }
    return false;
}

bool ShapeExtractor::isPointType(const App::DocumentObject* obj)
{
    // Base::Console().Message("SE::isPointType(%s)\n", obj->getNameInDocument());
    if (obj) {
        Base::Type t = obj->getTypeId();
        if (t.isDerivedFrom(Part::Vertex::getClassTypeId())) {
            return true;
        } else if (isDraftPoint(obj)) {
            return true;
        } else if (isDatumPoint(obj)) {
            return true;
        }
    }
    return false;
}

bool ShapeExtractor::isDraftPoint(const App::DocumentObject* obj)
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

bool ShapeExtractor::isDatumPoint(const App::DocumentObject* obj)
{
    std::string objTypeName = obj->getTypeId().getName();
    std::string pointToken("Point");
    if (objTypeName.find(pointToken) != std::string::npos) {
        return true;
    }
    return false;
}


//! get the location of a point object
Base::Vector3d ShapeExtractor::getLocation3dFromFeat(const App::DocumentObject* obj)
{
    // Base::Console().Message("SE::getLocation3dFromFeat()\n");
    if (!isPointType(obj)) {
        return Base::Vector3d(0.0, 0.0, 0.0);
    }
//    if (isDraftPoint(obj) {
//        //Draft Points are not necc. Part::PartFeature??
//        //if Draft option "use part primitives" is not set are Draft points still PartFeature?

    const Part::Feature* pf = dynamic_cast<const Part::Feature*>(obj);
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

//! get the located and oriented version of docObj shape
TopoDS_Shape ShapeExtractor::getLocatedShape(const App::DocumentObject* docObj)
{
        Part::TopoShape shape = Part::Feature::getTopoShape(docObj);
        const Part::Feature* pf = dynamic_cast<const Part::Feature*>(docObj);
        if (pf) {
            shape.setPlacement(pf->globalPlacement());
        }
        return shape.getShape();
}

bool ShapeExtractor::isSketchObject(const App::DocumentObject* obj)
{
// TODO:: the check for an object being a sketch should be done as in the commented
// if statement below. To do this, we need to include Mod/Sketcher/SketchObject.h,
// but that makes TechDraw dependent on Eigen libraries which we don't use.  As a
// workaround we will inspect the object's class name.
//    if (obj->isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
    std::string objTypeName = obj->getTypeId().getName();
    std::string sketcherToken("Sketcher");
    if (objTypeName.find(sketcherToken) != std::string::npos) {
        return true;
    }
    return false;
}

