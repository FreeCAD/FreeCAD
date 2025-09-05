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
#include <BRepCheck_Analyzer.hxx>
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
#include <Mod/Measure/App/ShapeFinder.h>
//#include <Mod/Sketcher/App/SketchObject.h>

#include "ShapeExtractor.h"
#include "DrawUtil.h"
#include "ShapeUtils.h"
#include "Preferences.h"

using namespace TechDraw;
using namespace Measure;
using DU = DrawUtil;
using SU = ShapeUtils;


//! pick out the 2d document objects in the list of links and return a vector of their shapes
//! Note that point objects will not make it through the hlr/projection process.
std::vector<TopoDS_Shape> ShapeExtractor::getShapes2d(const std::vector<App::DocumentObject*> links)
{
    std::vector<TopoDS_Shape> shapes2d;

    for (auto& l:links) {
        if (is2dObject(l)) {
            if (l->isDerivedFrom<Part::Feature>()) {
                TopoDS_Shape temp = getLocatedShape(l);
                // checkShape on 2d objs?
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
    std::vector<TopoDS_Shape> sourceShapes;

    for (auto& l:links) {
        if (is2dObject(l) && !include2d) {
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
                if (inObj->isDerivedFrom<App::Part>()) {
                    // we replace obj by the assembly
                    obj = inObj;
                    break;
                }
            }
        }

        if (obj->isDerivedFrom<App::Link>()) {
            App::Link* xLink = static_cast<App::Link*>(obj);
            std::vector<TopoDS_Shape> xShapes = getXShapes(xLink);
            if (!xShapes.empty()) {
                sourceShapes.insert(sourceShapes.end(), xShapes.begin(), xShapes.end());
                continue;
            }
        }
        else {
            auto shape = Part::Feature::getShape(obj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
            // if source obj has a shape, we use that shape.
            if(!SU::isShapeReallyNull(shape)) {
                if (checkShape(obj, shape)) {
                    sourceShapes.push_back(getLocatedShape(obj));
                }
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
        }

        if (s.ShapeType() < TopAbs_SOLID) {
            //clean up TopAbs_COMPOUND & TopAbs_COMPSOLID
            TopoDS_Shape cleanShape = ShapeFinder::stripInfiniteShapes(s);
            if (!cleanShape.IsNull()) {
                builder.Add(comp, cleanShape);
            }
        } else if (Part::TopoShape(s).isInfinite()) {
            continue;    //simple shape is infinite
        }

        //a simple shape - add to compound
        builder.Add(comp, s);
    }

    //it appears that an empty compound is !IsNull(), so we need to check a different way
    if (SU::isShapeReallyNull(comp)) {
        return {};
    }

    // BRepTools::Write(comp, "SEgetShapesOut.brep");

    return comp;
}

std::vector<TopoDS_Shape> ShapeExtractor::getXShapes(const App::Link* xLink)
{
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
            // TODO:  getTopoShape() ?
            auto shape = Part::Feature::getShape(l, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
            Part::TopoShape ts(shape);
            if (ts.isInfinite()) {
                shape = ShapeFinder::stripInfiniteShapes(shape);
            }
            if (!checkShape(l, shape)) {
                continue;
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
                Base::Console().message("SE::getXShapes - no shape from getXShape\n");
            }
        }
    } else {
        // link points to a regular object, not another link? no sublinks?
        TopoDS_Shape xLinkShape = getShapeFromXLink(xLink);
        if (!xLinkShape.IsNull() &&
            checkShape(xLink, xLinkShape)) {
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
        TopoDS_Shape shape = Part::Feature::getShape(linkedObject, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
        if (shape.IsNull()) {
            // this is where we need to parse the target for objects with a shape??
            return TopoDS_Shape();
        }
        Part::TopoShape ts(shape);
        if (ts.isInfinite()) {
            shape = ShapeFinder::stripInfiniteShapes(shape);
            ts = Part::TopoShape(shape);
        }
        //ts might be garbage now, better check
        try {
            if (!ts.isNull()) {
                ts.setPlacement(xLinkPlacement);
            }
        }
        catch (...) {
            Base::Console().error("ShapeExtractor failed to retrieve shape from %s\n", xLink->getNameInDocument());
            return TopoDS_Shape();
        }
        if (checkShape(linkedObject, ts.getShape())) {
            return ts.getShape();
        }
    }
    return TopoDS_Shape();
}

std::vector<TopoDS_Shape> ShapeExtractor::getShapesFromObject(const App::DocumentObject* docObj)
{
    std::vector<TopoDS_Shape> result;

    const App::GroupExtension* gex = dynamic_cast<const App::GroupExtension*>(docObj);
    App::Property* gProp = docObj->getPropertyByName("Group");
    App::Property* sProp = docObj->getPropertyByName("Shape");
    if (docObj->isDerivedFrom<Part::Feature>()) {
        if (checkShape(docObj, getLocatedShape(docObj))) {
            result.push_back(getLocatedShape(docObj));
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
            std::vector<App::DocumentObject*> objsAll = list->getValues();
            std::vector<TopoDS_Shape> shapesAll;
            for (auto& obj : objsAll) {
                shapesAll = getShapesFromObject(obj);
                result.insert(result.end(), shapesAll.begin(), shapesAll.end());
            }
        }
    } else if (sProp) {       //has a Shape property
        Part::PropertyPartShape* shapeProperty = dynamic_cast<Part::PropertyPartShape*>(sProp);
        if (shapeProperty &&
            checkShape(docObj, getLocatedShape(docObj))) {
            result.push_back(getLocatedShape(docObj));
        }
    }
    return result;
}

TopoDS_Shape ShapeExtractor::getShapesFused(const std::vector<App::DocumentObject*> links)
{
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
                Base::Console().error("SE - Fusion failed\n");
                return baseShape;
            }
            fusedShape = mkFuse.Shape();
        }
        baseShape = fusedShape;
    }

    // if there are 2d shapes in the links they will not fuse with the 3d shapes,
    // so instead we return a compound of the fused 3d shapes and the 2d shapes
    std::vector<TopoDS_Shape> shapes2d = getShapes2d(links);

    if (!shapes2d.empty()) {
        shapes2d.push_back(baseShape);
        return DrawUtil::shapeVectorToCompound(shapes2d, false);
    }

    return baseShape;
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
    return t.isDerivedFrom(Part::Line::getClassTypeId())
           || t.isDerivedFrom(Part::Circle::getClassTypeId())
           || t.isDerivedFrom(Part::Ellipse::getClassTypeId())
           || t.isDerivedFrom(Part::RegularPolygon::getClassTypeId());
}

bool ShapeExtractor::isPointType(const App::DocumentObject* obj)
{
    if (!obj) {
        return false;
    }
    return obj->isDerivedFrom<Part::Vertex>()
           || isDraftPoint(obj)
           || isDatumPoint(obj);
}

bool ShapeExtractor::isDraftPoint(const App::DocumentObject* obj)
{
    //if the docObj doesn't have a Proxy property, it definitely isn't a Draft point
    App::PropertyPythonObject* proxy = dynamic_cast<App::PropertyPythonObject*>(obj->getPropertyByName("Proxy"));
    if (proxy) {
        std::string  pp = proxy->toString();
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
    if (!isPointType(obj)) {
        return Base::Vector3d(0.0, 0.0, 0.0);
    }
//    if (isDraftPoint(obj) {
//        //Draft Points are not necc. Part::Feature??
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

    return Base::Vector3d(0.0, 0.0, 0.0);
}

//! get the located and oriented version of docObj shape
TopoDS_Shape ShapeExtractor::getLocatedShape(const App::DocumentObject* docObj)
{
        Part::TopoShape shape = Part::Feature::getTopoShape(docObj, Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform);
        const Part::Feature* pf = dynamic_cast<const Part::Feature*>(docObj);
        if (pf) {
            shape.setPlacement(pf->globalPlacement());
        }
        return shape.getShape();
}

bool ShapeExtractor::isSketchObject(const App::DocumentObject* obj)
{
    // Use name to lookup to avoid dependency on Sketcher module
    return obj->isDerivedFrom(Base::Type::fromName("Sketcher::SketchObject"));
}


//! true if shape fails validity check.  A fail here is not a guarantee of later
//! problems, but invalid shapes are known to cause issues with HLR_Algo and boolean ops.
bool ShapeExtractor::checkShape(const App::DocumentObject* shapeObj, TopoDS_Shape shape)
{
    if (!Preferences::checkShapesBeforeUse()) {
        return true;
    }

    if (!BRepCheck_Analyzer(shape).IsValid()) {
        if (Preferences::debugBadShape()) {
            std::stringstream ssFileName;
            ssFileName << "BadShape"  << shapeObj->Label.getValue() << ".brep";
            BRepTools::Write(shape, ssFileName.str().c_str());
        }
        // this is ok for devs, but there must be a better way to inform the user from somewhere deep in the
        // call stack. notification area from App?
        Base::Console().warning(
            "ShapeExtractor found a problem shape in %s.  Results may be incorrect.\n",
            shapeObj->getNameInDocument());
        return false;
    }
    return true;
}


