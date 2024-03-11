/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Bnd_Box.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeShape.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepGProp.hxx>
# include <BRepIntCurveSurface_Inter.hxx>
# include <gce_MakeDir.hxx>
# include <gce_MakeLin.hxx>
# include <gp_Ax1.hxx>
# include <gp_Dir.hxx>
# include <gp_Pln.hxx>
# include <gp_Trsf.hxx>
# include <GProp_GProps.hxx>
# include <IntCurveSurface_IntersectionPoint.hxx>
# include <Precision.hxx>
# include <Standard_Failure.hxx>
# include <Standard_Version.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/FeaturePythonPyImp.h>
#include <App/Link.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/ElementNamingUtils.h>
#include <App/Placement.h>
#include <App/OriginFeature.h>
#include <Base/Exception.h>
#include <Base/Placement.h>
#include <Base/Rotation.h>
#include <Base/Stream.h>

#include "PartFeature.h"
#include "PartFeaturePy.h"
#include "PartPyCXX.h"
#include "TopoShapePy.h"

using namespace Part;
namespace sp = std::placeholders;

FC_LOG_LEVEL_INIT("Part",true,true)

PROPERTY_SOURCE(Part::Feature, App::GeoFeature)


Feature::Feature()
{
    ADD_PROPERTY(Shape, (TopoDS_Shape()));
}

Feature::~Feature() = default;

short Feature::mustExecute() const
{
    return GeoFeature::mustExecute();
}

App::DocumentObjectExecReturn *Feature::recompute()
{
    try {
        return App::GeoFeature::recompute();
    }
    catch (Standard_Failure& e) {

        App::DocumentObjectExecReturn* ret = new App::DocumentObjectExecReturn(e.GetMessageString());
        if (ret->Why.empty()) ret->Why = "Unknown OCC exception";
        return ret;
    }
}

App::DocumentObjectExecReturn *Feature::execute()
{
    this->Shape.touch();
    return GeoFeature::execute();
}

PyObject *Feature::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new PartFeaturePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

App::DocumentObject* Feature::getSubObject(const char* subname,
                                           PyObject** pyObj,
                                           Base::Matrix4D* pmat,
                                           bool transform,
                                           int depth) const
{
    // having '.' inside subname means it is referencing some children object,
    // instead of any sub-element from ourself
    if (subname && !Data::isMappedElement(subname) && strchr(subname, '.')) {
        return App::DocumentObject::getSubObject(subname, pyObj, pmat, transform, depth);
    }

    Base::Matrix4D _mat;
    auto& mat = pmat ? *pmat : _mat;
    if (transform) {
        mat *= Placement.getValue().toMatrix();
    }

    if (!pyObj) {
        // TopoShape::hasSubShape is kind of slow, let's cut outself some slack here.
        return const_cast<Feature*>(this);
    }

    try {
        TopoShape ts(Shape.getShape());
        bool doTransform = mat != ts.getTransform();
        if (doTransform) {
            ts.setShape(ts.getShape().Located(TopLoc_Location()));
        }
        if (subname && *subname && !ts.isNull()) {
            ts = ts.getSubShape(subname);
        }
        if (doTransform && !ts.isNull()) {
            static int sCopy = -1;
            if (sCopy < 0) {
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/Mod/Part/General");
                sCopy = hGrp->GetBool("CopySubShape", false) ? 1 : 0;
            }
            bool copy = sCopy ? true : false;
            if (!copy) {
                // Work around OCC bug on transforming circular edge with an
                // offset surface. The bug probably affect other shape type,
                // too.
                TopExp_Explorer exp(ts.getShape(), TopAbs_EDGE);
                if (exp.More()) {
                    auto edge = TopoDS::Edge(exp.Current());
                    exp.Next();
                    if (!exp.More()) {
                        BRepAdaptor_Curve curve(edge);
                        copy = curve.GetType() == GeomAbs_Circle;
                    }
                }
            }
            ts.transformShape(mat, copy, true);
        }
        *pyObj = Py::new_reference_to(shape2pyshape(ts));
        return const_cast<Feature*>(this);
    }
    catch (Standard_Failure& e) {
        // FIXME: Do not handle the exception here because it leads to a flood of irrelevant and
        // annoying error messages.
        // For example: https://forum.freecad.org/viewtopic.php?f=19&t=42216
        // Instead either raise a sub-class of Base::Exception and let it handle by the calling
        // instance or do simply nothing. For now the error message is degraded to a log message.
        std::ostringstream str;
        Standard_CString msg = e.GetMessageString();

        // Avoid name mangling
        str << e.DynamicType()->get_type_name() << " ";

        if (msg) {
            str << msg;
        }
        else {
            str << "No OCCT Exception Message";
        }
        str << ": " << getFullName();
        if (subname) {
            str << '.' << subname;
        }
        FC_LOG(str.str());
        return nullptr;
    }
}

static std::vector<std::pair<long, Data::MappedName>> getElementSource(App::DocumentObject* owner,
                                                                       TopoShape shape,
                                                                       const Data::MappedName& name,
                                                                       char type)
{
    std::set<std::pair<App::Document*, long>> tagSet;
    std::vector<std::pair<long, Data::MappedName>> ret;
    ret.emplace_back(0, name);
    int depth = 0;
    while (1) {
        Data::MappedName original;
        std::vector<Data::MappedName> history;
        // It is possible the name does not belong to the shape, e.g. when user
        // changes modeling order in PartDesign. So we try to assign the
        // document hasher here in case getElementHistory() needs to de-hash
        if (!shape.Hasher && owner) {
            shape.Hasher = owner->getDocument()->getStringHasher();
        }
        long tag = shape.getElementHistory(ret.back().second, &original, &history);
        if (!tag) {
            break;
        }
        auto obj = owner;
        App::Document* doc = nullptr;
        if (owner) {
            doc = owner->getDocument();
            for (;; ++depth) {
                auto linked = owner->getLinkedObject(false, nullptr, false, depth);
                if (linked == owner) {
                    break;
                }
                owner = linked;
                if (owner->getDocument() != doc) {
                    doc = owner->getDocument();
                    break;
                }
            }
            // TODO: 02/24 Toponaming project:  It appears that getElementOwner is always nullptr.
            //            if (owner->isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            //                auto o =
            //                static_cast<App::GeoFeature*>(owner)->getElementOwner(ret.back().second);
            //                if (o)
            //                    doc = o->getDocument();
            //            }
            obj = doc->getObjectByID(tag < 0 ? -tag : tag);
            if (type) {
                for (auto& hist : history) {
                    if (shape.elementType(hist) != type) {
                        return ret;
                    }
                }
            }
        }
        owner = 0;
        if (!obj) {
            // Object maybe deleted, but it is still possible to extract the
            // source element name from hasher table.
            shape.setShape(TopoDS_Shape());
            doc = nullptr;
        }
        else {
            shape = Part::Feature::getTopoShape(obj, 0, false, 0, &owner);
        }
        if (type && shape.elementType(original) != type) {
            break;
        }

        if (std::abs(tag) != ret.back().first && !tagSet.insert(std::make_pair(doc, tag)).second) {
            // Because an object might be deleted, which may be a link/binder
            // that points to an external object that contain element name
            // using external hash table. We shall prepare for circular element
            // map due to looking up in the wrong table.
            if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                FC_WARN("circular element mapping");
            }
            break;
        }
        ret.emplace_back(tag, original);
    }
    return ret;
}

std::list<Data::HistoryItem> Feature::getElementHistory(App::DocumentObject* feature,
                                                        const char* name,
                                                        bool recursive,
                                                        bool sameType)
{
    std::list<Data::HistoryItem> ret;
    TopoShape shape = getTopoShape(feature);
    Data::IndexedName idx(name);
    Data::MappedName element;
    Data::MappedName prevElement;
    if (idx) {
        element = shape.getMappedName(idx, true);
    }
    else if (Data::isMappedElement(name)) {
        element = Data::MappedName(Data::newElementName(name));
    }
    else {
        element = Data::MappedName(name);
    }
    char element_type = 0;
    if (sameType) {
        element_type = shape.elementType(element);
    }
    int depth = 0;
    do {
        Data::MappedName original;
        ret.emplace_back(feature, element);
        long tag = shape.getElementHistory(element, &original, &ret.back().intermediates);

        ret.back().index = shape.getIndexedName(element);
        if (!ret.back().index && prevElement) {
            ret.back().index = shape.getIndexedName(prevElement);
            if (ret.back().index) {
                ret.back().intermediates.insert(ret.back().intermediates.begin(), element);
                ret.back().element = prevElement;
            }
        }
        if (ret.back().intermediates.size()) {
            prevElement = ret.back().intermediates.back();
        }
        else {
            prevElement = Data::MappedName();
        }

        App::DocumentObject* obj = nullptr;
        if (tag) {
            App::Document* doc = feature->getDocument();
            for (;; ++depth) {
                auto linked = feature->getLinkedObject(false, nullptr, false, depth);
                if (linked == feature) {
                    break;
                }
                feature = linked;
                if (feature->getDocument() != doc) {
                    doc = feature->getDocument();
                    break;
                }
            }
            // TODO: 02/24 Toponaming project:  It appears that getElementOwner is always nullptr.
            //            if(feature->isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            //                auto owner =
            //                static_cast<App::GeoFeature*>(feature)->getElementOwner(element);
            //                if(owner)
            //                    doc = owner->getDocument();
            //            }
            obj = doc->getObjectByID(std::abs(tag));
        }
        if (!recursive) {
            ret.emplace_back(obj, original);
            ret.back().tag = tag;
            return ret;
        }
        if (!obj) {
            break;
        }
        if (element_type) {
            for (auto& hist : ret.back().intermediates) {
                if (shape.elementType(hist) != element_type) {
                    return ret;
                }
            }
        }
        feature = obj;
        shape = Feature::getTopoShape(feature);
        element = original;
        if (element_type && shape.elementType(original) != element_type) {
            break;
        }
    } while (feature);
    return ret;
}

QVector<Data::MappedElement> Feature::getElementFromSource(App::DocumentObject* obj,
                                                           const char* subname,
                                                           App::DocumentObject* src,
                                                           const char* srcSub,
                                                           bool single)
{
    QVector<Data::MappedElement> res;
    if (!obj || !src) {
        return res;
    }

    auto shape = getTopoShape(obj,
                              subname,
                              false,
                              nullptr,
                              nullptr,
                              true,
                              /*transform = */ false);
    App::DocumentObject* owner = nullptr;
    auto srcShape = getTopoShape(src, srcSub, false, nullptr, &owner);
    int tagChanges;
    Data::MappedElement element;
    Data::IndexedName checkingSubname;
    std::string sub = Data::noElementName(subname);
    auto checkHistory = [&](const Data::MappedName& name, size_t, long, long tag) {
        if (std::abs(tag) == owner->getID()) {
            if (!tagChanges) {
                tagChanges = 1;
            }
        }
        else if (tagChanges && ++tagChanges > 3) {
            // Once we found the tag, trace no more than 2 addition tag changes
            // to limited the search depth.
            return true;
        }
        if (name == element.name) {
            std::pair<std::string, std::string> objElement;
            std::size_t len = sub.size();
//            checkingSubname.toString(sub);
            checkingSubname.appendToStringBuffer(sub);
            GeoFeature::resolveElement(obj, sub.c_str(), objElement);
            sub.resize(len);
            if (objElement.second.size()) {
                res.push_back(Data::MappedElement(Data::MappedName(objElement.first),
                                                  Data::IndexedName(objElement.second.c_str())));
                return true;
            }
        }
        return false;
    };

    // obtain both the old and new style element name
    std::pair<std::string, std::string> objElement;
    GeoFeature::resolveElement(src, srcSub, objElement, false);

    element.index = Data::IndexedName(objElement.second.c_str());
    if (!objElement.first.empty()) {
        // Strip prefix and indexed based name at the tail of the new style element name
        auto mappedName = Data::newElementName(objElement.first.c_str());
        auto mapped = Data::isMappedElement(mappedName.c_str());
        if (mapped) {
            element.name = Data::MappedName(mapped);
        }
    }

    // Translate the element name for datum
    if (objElement.second == "Plane") {
        objElement.second = "Face1";
    }
    else if (objElement.second == "Line") {
        objElement.second = "Edge1";
    }
    else if (objElement.second == "Point") {
        objElement.second = "Vertex1";
    }

    // Use the old style name to obtain the shape type
    auto type = TopoShape::shapeType(Data::findElementName(objElement.second.c_str()));
    // If the given shape has the same number of sub shapes as the source (e.g.
    // a compound operation), then take a shortcut and assume the element index
    // remains the same. But we still need to trace the shape history to
    // confirm.
    if (element.name && shape.countSubShapes(type) == srcShape.countSubShapes(type)) {
        tagChanges = 0;
        checkingSubname = element.index;
        auto mapped = shape.getMappedName(element.index);
        shape.traceElement(mapped, checkHistory);
        if (res.size()) {
            return res;
        }
    }

    // Try geometry search first
    auto subShape = srcShape.getSubShape(objElement.second.c_str());
    std::vector<std::string> names;
    shape.findSubShapesWithSharedVertex(subShape, &names);
    if (names.size()) {
        for (auto& name : names) {
            Data::MappedElement e;
            e.index = Data::IndexedName(name.c_str());
            e.name = shape.getMappedName(e.index, true);
            res.append(e);
            if (single) {
                break;
            }
        }
        return res;
    }

    if (!element.name) {
        return res;
    }

    // No shortcut, need to search every element of the same type. This may
    // result in multiple matches, e.g. a compound of array of the same
    // instance.
    const char* shapetype = TopoShape::shapeName(type).c_str();
    for (int i = 0, count = shape.countSubShapes(type); i < count; ++i) {
        checkingSubname = Data::IndexedName::fromConst(shapetype, i + 1);
        auto mapped = shape.getMappedName(checkingSubname);
        tagChanges = 0;
        shape.traceElement(mapped, checkHistory);
        if (single && res.size()) {
            break;
        }
    }
    return res;
}

QVector<Data::MappedElement> Feature::getRelatedElements(App::DocumentObject* obj,
                                                         const char* name,
                                                         HistoryTraceType sameType,
                                                         bool withCache)
{
    auto owner = obj;
    auto shape = getTopoShape(obj, nullptr, false, 0, &owner);
    QVector<Data::MappedElement> ret;
    Data::MappedElement mapped = shape.getElementName(name);
    if (!mapped.name) {
        return ret;
    }
    if (withCache && shape.getRelatedElementsCached(mapped.name, sameType, ret)) {
        return ret;
    }

    char element_type = shape.elementType(mapped.name);
    TopAbs_ShapeEnum type = TopoShape::shapeType(element_type, true);
    if (type == TopAbs_SHAPE) {
        return ret;
    }

    auto source =
        getElementSource(owner,
                         shape,
                         mapped.name,
                         sameType == HistoryTraceType::followTypeChange ? element_type : 0);
    for (auto& src : source) {
        auto srcIndex = shape.getIndexedName(src.second);
        if (srcIndex) {
            ret.push_back(Data::MappedElement(src.second, srcIndex));
            shape.cacheRelatedElements(mapped.name, sameType, ret);
            return ret;
        }
    }

    std::map<int, QVector<Data::MappedElement>> retMap;

    const char* shapetype = TopoShape::shapeName(type).c_str();
    std::ostringstream ss;
    for (size_t i = 1; i <= shape.countSubShapes(type); ++i) {
        Data::MappedElement related;
        related.index = Data::IndexedName::fromConst(shapetype, i);
        related.name = shape.getMappedName(related.index);
        if (!related.name) {
            continue;
        }
        auto src =
            getElementSource(owner,
                             shape,
                             related.name,
                             sameType == HistoryTraceType::followTypeChange ? element_type : 0);
        int idx = (int)source.size() - 1;
        for (auto rit = src.rbegin(); idx >= 0 && rit != src.rend(); ++rit, --idx) {
            // TODO: shall we ignore source tag when comparing? It could cause
            // matching unrelated element, but it does help dealing with feature
            // reording in PartDesign::Body.
            if (rit->second != source[idx].second) {
                ++idx;
                break;
            }
        }
        if (idx < (int)source.size()) {
            retMap[idx].push_back(related);
        }
    }
    if (retMap.size()) {
        ret = retMap.begin()->second;
    }
    shape.cacheRelatedElements(mapped.name, sameType, ret);
    return ret;
}

TopoDS_Shape Feature::getShape(const App::DocumentObject *obj, const char *subname,
        bool needSubElement, Base::Matrix4D *pmat, App::DocumentObject **powner,
        bool resolveLink, bool transform)
{
    return getTopoShape(obj,subname,needSubElement,pmat,powner,resolveLink,transform,true).getShape();
}


// Toponaming project March 2024:  This method should be going away when we get to the python layer.
void Feature::clearShapeCache() {
//    _ShapeCache.cache.clear();
}

static TopoShape _getTopoShape(const App::DocumentObject* obj,
                               const char* subname,
                               bool needSubElement,
                               Base::Matrix4D* pmat,
                               App::DocumentObject** powner,
                               bool resolveLink,
                               bool noElementMap,
                               const std::set<std::string> hiddens,
                               const App::DocumentObject* lastLink)

{
    TopoShape shape;

    if (!obj) {
        return shape;
    }

    PyObject* pyobj = nullptr;
    Base::Matrix4D mat;
    if (powner) {
        *powner = nullptr;
    }

    std::string _subname;
    auto subelement = Data::findElementName(subname);
    if (!needSubElement && subname) {
        // strip out element name if not needed
        if (subelement && *subelement) {
            _subname = std::string(subname, subelement);
            subname = _subname.c_str();
        }
    }

    auto canCache = [&](const App::DocumentObject* o) {
        return !lastLink || (hiddens.empty() && !App::GeoFeatureGroupExtension::isNonGeoGroup(o));
    };

    if (canCache(obj) && PropertyShapeCache::getShape(obj, shape, subname)) {
        if (noElementMap) {
            shape.resetElementMap();
            shape.Tag = 0;
            if ( shape.Hasher ) {
                shape.Hasher->clear();
            }
        }
    }

    App::DocumentObject* linked = nullptr;
    App::DocumentObject* owner = nullptr;
    Base::Matrix4D linkMat;
    App::StringHasherRef hasher;
    long tag;
    {
        Base::PyGILStateLocker lock;
        owner = obj->getSubObject(subname, shape.isNull() ? &pyobj : nullptr, &mat, false);
        if (!owner) {
            return shape;
        }
        tag = owner->getID();
        hasher = owner->getDocument()->getStringHasher();
        linked = owner->getLinkedObject(true, &linkMat, false);
        if (pmat) {
            if (resolveLink && obj != owner) {
                *pmat = mat * linkMat;
            }
            else {
                *pmat = mat;
            }
        }
        if (!linked) {
            linked = owner;
        }
        if (powner) {
            *powner = resolveLink ? linked : owner;
        }

        if (!shape.isNull()) {
            return shape;
        }

        if (pyobj && PyObject_TypeCheck(pyobj, &TopoShapePy::Type)) {
            shape = *static_cast<TopoShapePy*>(pyobj)->getTopoShapePtr();
            if (!shape.isNull()) {
                if (canCache(obj)) {
                    if (obj->getDocument() != linked->getDocument()
                        || mat.hasScale() != Base::ScaleType::NoScaling
                        || (linked != owner && linkMat.hasScale() != Base::ScaleType::NoScaling)) {
                        PropertyShapeCache::setShape(obj, shape, subname);
                    }
                }
                if (noElementMap) {
                    shape.resetElementMap();
                    shape.Tag = 0;
                    if ( shape.Hasher ) {
                        shape.Hasher->clear();
                    }
                }
                Py_DECREF(pyobj);
                return shape;
            }
        }
        else {
            if (linked->isDerivedFrom(App::Line::getClassTypeId())) {
                static TopoDS_Shape _shape;
                if (_shape.IsNull()) {
                    BRepBuilderAPI_MakeEdge builder(gp_Lin(gp_Pnt(0, 0, 0), gp_Dir(1, 0, 0)));
                    _shape = builder.Shape();
                    _shape.Infinite(Standard_True);
                }
                shape = TopoShape(tag, hasher, _shape);
            }
            else if (linked->isDerivedFrom(App::Plane::getClassTypeId())) {
                static TopoDS_Shape _shape;
                if (_shape.IsNull()) {
                    BRepBuilderAPI_MakeFace builder(gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)));
                    _shape = builder.Shape();
                    _shape.Infinite(Standard_True);
                }
                shape = TopoShape(tag, hasher, _shape);
            }
            else if (linked->isDerivedFrom(App::Placement::getClassTypeId())) {
                auto element = Data::findElementName(subname);
                if (element) {
                    if (boost::iequals("x", element) || boost::iequals("x-axis", element)
                        || boost::iequals("y", element) || boost::iequals("y-axis", element)
                        || boost::iequals("z", element) || boost::iequals("z-axis", element)) {
                        static TopoDS_Shape _shape;
                        if (_shape.IsNull()) {
                            BRepBuilderAPI_MakeEdge builder(
                                gp_Lin(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)));
                            _shape = builder.Shape();
                            _shape.Infinite(Standard_True);
                        }
                        shape = TopoShape(tag, hasher, _shape);
                    }
                    else if (boost::iequals("o", element) || boost::iequals("origin", element)) {
                        static TopoDS_Shape _shape;
                        if (_shape.IsNull()) {
                            BRepBuilderAPI_MakeVertex builder(gp_Pnt(0, 0, 0));
                            _shape = builder.Shape();
                            _shape.Infinite(Standard_True);
                        }
                        shape = TopoShape(tag, hasher, _shape);
                    }
                }
                if (shape.isNull()) {
                    static TopoDS_Shape _shape;
                    if (_shape.IsNull()) {
                        BRepBuilderAPI_MakeFace builder(gp_Pln(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1)));
                        _shape = builder.Shape();
                        _shape.Infinite(Standard_True);
                    }
                    shape = TopoShape(tag, hasher, _shape);
                }
            }
            if (!shape.isNull()) {
                shape.transformShape(mat * linkMat, false, true);
                return shape;
            }
        }

        Py_XDECREF(pyobj);
    }

    // nothing can be done if there is sub-element references
    if (needSubElement && subelement && *subelement) {
        return shape;
    }

    if (obj != owner) {
        if (canCache(owner) && PropertyShapeCache::getShape(owner, shape)) {
            bool scaled = shape.transformShape(mat, false, true);
            if (owner->getDocument() != obj->getDocument()) {
                shape.reTagElementMap(obj->getID(), obj->getDocument()->getStringHasher());
                PropertyShapeCache::setShape(obj, shape, subname);
            }
            else if (scaled
                     || (linked != owner && linkMat.hasScale() != Base::ScaleType::NoScaling)) {
                PropertyShapeCache::setShape(obj, shape, subname);
            }
        }
        if (!shape.isNull()) {
            if (noElementMap) {
                shape.resetElementMap();
                shape.Tag = 0;
                if ( shape.Hasher) {
                    shape.Hasher->clear();
                }
            }
            return shape;
        }
    }

    bool cacheable = true;

    auto link = owner->getExtensionByType<App::LinkBaseExtension>(true);
    if (owner != linked
        && (!link || (!link->_ChildCache.getSize() && link->getSubElements().size() <= 1))) {
        // if there is a linked object, and there is no child cache (which is used
        // for special handling of plain group), obtain shape from the linked object
        shape = Feature::getTopoShape(linked, nullptr, false, nullptr, nullptr, false, false);
        if (shape.isNull()) {
            return shape;
        }
        if (owner == obj) {
            shape.transformShape(mat * linkMat, false, true);
        }
        else {
            shape.transformShape(linkMat, false, true);
        }
        shape.reTagElementMap(tag, hasher);
    }
    else {
        // Construct a compound of sub objects
        std::vector<TopoShape> shapes;

        // Acceleration for link array. Unlike non-array link, a link array does
        // not return the linked object when calling getLinkedObject().
        // Therefore, it should be handled here.
        TopoShape baseShape;
        Base::Matrix4D baseMat;
        std::string op;
        if (link && link->getElementCountValue()) {
            linked = link->getTrueLinkedObject(false, &baseMat);
            if (linked && linked != owner) {
                baseShape =
                    Feature::getTopoShape(linked, nullptr, false, nullptr, nullptr, false, false);
                if (!link->getShowElementValue()) {
                    baseShape.reTagElementMap(owner->getID(),
                                              owner->getDocument()->getStringHasher());
                }
            }
        }
        for (auto& sub : owner->getSubObjects()) {
            if (sub.empty()) {
                continue;
            }
            int visible;
            std::string childName;
            App::DocumentObject* parent = nullptr;
            Base::Matrix4D mat = baseMat;
            App::DocumentObject* subObj = nullptr;
            if (sub.find('.') == std::string::npos) {
                visible = 1;
            }
            else {
                subObj =
                    owner->resolve(sub.c_str(), &parent, &childName, nullptr, nullptr, &mat, false);
                if (!parent || !subObj) {
                    continue;
                }
                if (lastLink && App::GeoFeatureGroupExtension::isNonGeoGroup(parent)) {
                    visible = lastLink->isElementVisible(childName.c_str());
                }
                else {
                    visible = parent->isElementVisible(childName.c_str());
                }
            }
            if (visible == 0) {
                continue;
            }

            std::set<std::string> nextHiddens = hiddens;
            const App::DocumentObject* nextLink = lastLink;
            // Toponaming project March 2024:  This appears to be a non toponaming feature:
//            if (!checkLinkVisibility(nextHiddens, true, nextLink, owner, sub.c_str())) {
//                cacheable = false;
//                continue;
//            }

            TopoShape shape;

            bool doGetShape = (!subObj || baseShape.isNull());
            if (!doGetShape) {
                auto type = mat.hasScale();
                if (type != Base::ScaleType::NoScaling && type != Base::ScaleType::Uniform) {
                    doGetShape = true;
                }
            }
            if (doGetShape) {
                shape = _getTopoShape(owner,
                                      sub.c_str(),
                                      true,
                                      0,
                                      &subObj,
                                      false,
                                      false,
                                      nextHiddens,
                                      nextLink);
                if (shape.isNull()) {
                    continue;
                }
                if (visible < 0 && subObj && !subObj->Visibility.getValue()) {
                    continue;
                }
            }
            else {
                if (link && !link->getShowElementValue()) {
                    shape =
                        baseShape.makeElementTransform(mat,
                                                (Data::POSTFIX_INDEX + childName).c_str());
                }
                else {
                    shape = baseShape.makeElementTransform(mat);
                    shape.reTagElementMap(subObj->getID(),
                                          subObj->getDocument()->getStringHasher());
                }
            }
            shapes.push_back(shape);
        }

        if (shapes.empty()) {
            return shape;
        }
        shape.Tag = tag;
        shape.Hasher = hasher;
        shape.makeElementCompound(shapes);
    }

    if (cacheable && canCache(owner)) {
        PropertyShapeCache::setShape(owner, shape);
    }

    if (owner != obj) {
        bool scaled = shape.transformShape(mat, false, true);
        if (owner->getDocument() != obj->getDocument()) {
            shape.reTagElementMap(obj->getID(), obj->getDocument()->getStringHasher());
            scaled = true;  // force cache
        }
        if (canCache(obj) && scaled) {
            PropertyShapeCache::setShape(obj, shape, subname);
        }
    }
    if (noElementMap) {
        shape.resetElementMap();
        shape.Tag = 0;
        if ( shape.Hasher ) {
            shape.Hasher->clear();
        }
    }
    return shape;
}

TopoShape Feature::getTopoShape(const App::DocumentObject* obj,
                                const char* subname,
                                bool needSubElement,
                                Base::Matrix4D* pmat,
                                App::DocumentObject** powner,
                                bool resolveLink,
                                bool transform,
                                bool noElementMap)
{
    if (!obj || !obj->getNameInDocument()) {
        return TopoShape();
    }

    const App::DocumentObject* lastLink = 0;
    std::set<std::string> hiddens;
    // Toponaming project March 2024:  This appears to be a non toponaming feature:
//    if (!checkLinkVisibility(hiddens, false, lastLink, obj, subname)) {
//        return TopoShape();
//    }

    // NOTE! _getTopoShape() always return shape without top level
    // transformation for easy shape caching, i.e.  with `transform` set
    // to false. So we manually apply the top level transform if asked.

    if (needSubElement && (!pmat || *pmat == Base::Matrix4D())
        && obj->isDerivedFrom(Part::Feature::getClassTypeId())
        && !obj->hasExtension(App::LinkBaseExtension::getExtensionClassTypeId())) {
        // Some OCC shape making is very sensitive to shape transformation. So
        // check here if a direct sub shape is required, and bypass all extra
        // processing here.
        if (subname && *subname && Data::findElementName(subname) == subname) {
            TopoShape ts = static_cast<const Part::Feature*>(obj)->Shape.getShape();
            if (!transform) {
                ts.setShape(ts.getShape().Located(TopLoc_Location()), false);
            }
            if (noElementMap) {
                ts = ts.getSubShape(subname, true);
            }
            else {
                ts = ts.getSubTopoShape(subname, true);
            }
            if (!ts.isNull()) {
                if (powner) {
                    *powner = const_cast<App::DocumentObject*>(obj);
                }
                if (pmat && transform) {
                    *pmat = static_cast<const Part::Feature*>(obj)->Placement.getValue().toMatrix();
                }
                return ts;
            }
        }
    }

    Base::Matrix4D mat;
    auto shape = _getTopoShape(obj,
                               subname,
                               needSubElement,
                               &mat,
                               powner,
                               resolveLink,
                               noElementMap,
                               hiddens,
                               lastLink);

    Base::Matrix4D topMat;
    if (pmat || transform) {
        // Obtain top level transformation
        if (pmat) {
            topMat = *pmat;
        }
        if (transform) {
            obj->getSubObject(nullptr, nullptr, &topMat);
        }

        // Apply the top level transformation
        if (!shape.isNull()) {
            shape.transformShape(topMat, false, true);
        }

        if (pmat) {
            *pmat = topMat * mat;
        }
    }

    return shape;
}

App::DocumentObject *Feature::getShapeOwner(const App::DocumentObject *obj, const char *subname)
{
    if(!obj)
        return nullptr;
    auto owner = obj->getSubObject(subname);
    if(owner) {
        auto linked = owner->getLinkedObject(true);
        if(linked)
            owner = linked;
    }
    return owner;
}

void Feature::onChanged(const App::Property* prop)
{
    // if the placement has changed apply the change to the point data as well
    if (prop == &this->Placement) {
        this->Shape.setTransform(this->Placement.getValue().toMatrix());
    }
    // if the point data has changed check and adjust the transformation as well
    else if (prop == &this->Shape) {
        if (this->isRecomputing()) {
            this->Shape.setTransform(this->Placement.getValue().toMatrix());
        }
        else {
            Base::Placement p;
            // shape must not be null to override the placement
            if (!this->Shape.getValue().IsNull()) {
                try {
                    p.fromMatrix(this->Shape.getShape().getTransform());
                    if (p != this->Placement.getValue())
                        this->Placement.setValue(p);
                }
                catch (const Base::ValueError&) {
                }
            }
        }
    }

    GeoFeature::onChanged(prop);
}

TopLoc_Location Feature::getLocation() const
{
    Base::Placement pl = this->Placement.getValue();
    Base::Rotation rot(pl.getRotation());
    Base::Vector3d axis;
    double angle;
    rot.getValue(axis, angle);
    gp_Trsf trf;
    trf.SetRotation(gp_Ax1(gp_Pnt(), gp_Dir(axis.x, axis.y, axis.z)), angle);
    trf.SetTranslationPart(gp_Vec(pl.getPosition().x,pl.getPosition().y,pl.getPosition().z));
    return TopLoc_Location(trf);
}

Feature* Feature::create(const TopoShape& shape, const char* name, App::Document* document)
{
    if (!name || !name[0]) {
        name = "Shape";
    }
    if (!document) {
        document = App::GetApplication().getActiveDocument();
        if (!document) {
            document = App::GetApplication().newDocument();
        }
    }
    auto res = static_cast<Part::Feature*>(document->addObject("Part::Feature", name));
    res->Shape.setValue(shape);
    res->purgeTouched();
    return res;
}

ShapeHistory Feature::buildHistory(BRepBuilderAPI_MakeShape& mkShape, TopAbs_ShapeEnum type,
                                   const TopoDS_Shape& newS, const TopoDS_Shape& oldS)
{
    ShapeHistory history;
    history.type = type;

    TopTools_IndexedMapOfShape newM, oldM;
    TopExp::MapShapes(newS, type, newM); // map containing all old objects of type "type"
    TopExp::MapShapes(oldS, type, oldM); // map containing all new objects of type "type"

    // Look at all objects in the old shape and try to find the modified object in the new shape
    for (int i=1; i<=oldM.Extent(); i++) {
        bool found = false;
        TopTools_ListIteratorOfListOfShape it;
        // Find all new objects that are a modification of the old object (e.g. a face was resized)
        for (it.Initialize(mkShape.Modified(oldM(i))); it.More(); it.Next()) {
            found = true;
            for (int j=1; j<=newM.Extent(); j++) { // one old object might create several new ones!
                if (newM(j).IsPartner(it.Value())) {
                    history.shapeMap[i-1].push_back(j-1); // adjust indices to start at zero
                    break;
                }
            }
        }

        // Find all new objects that were generated from an old object (e.g. a face generated from an edge)
        for (it.Initialize(mkShape.Generated(oldM(i))); it.More(); it.Next()) {
            found = true;
            for (int j=1; j<=newM.Extent(); j++) {
                if (newM(j).IsPartner(it.Value())) {
                    history.shapeMap[i-1].push_back(j-1);
                    break;
                }
            }
        }

        if (!found) {
            // Find all old objects that don't exist any more (e.g. a face was completely cut away)
            if (mkShape.IsDeleted(oldM(i))) {
                history.shapeMap[i-1] = std::vector<int>();
            }
            else {
                // Mop up the rest (will this ever be reached?)
                for (int j=1; j<=newM.Extent(); j++) {
                    if (newM(j).IsPartner(oldM(i))) {
                        history.shapeMap[i-1].push_back(j-1);
                        break;
                    }
                }
            }
        }
    }

    return history;
}

ShapeHistory Feature::joinHistory(const ShapeHistory& oldH, const ShapeHistory& newH)
{
    ShapeHistory join;
    join.type = oldH.type;

    for (const auto & it : oldH.shapeMap) {
        int old_shape_index = it.first;
        if (it.second.empty())
            join.shapeMap[old_shape_index] = ShapeHistory::List();
        for (const auto& jt : it.second) {
            const auto& kt = newH.shapeMap.find(jt);
            if (kt != newH.shapeMap.end()) {
                ShapeHistory::List& ary = join.shapeMap[old_shape_index];
                ary.insert(ary.end(), kt->second.begin(), kt->second.end());
            }
        }
    }

    return join;
}

    /// returns the type name of the ViewProvider
const char* Feature::getViewProviderName() const {
    return "PartGui::ViewProviderPart";
}

const App::PropertyComplexGeoData* Feature::getPropertyOfGeometry() const
{
    return &Shape;
}

bool Feature::isElementMappingDisabled(App::PropertyContainer* container)
{
#ifdef FC_USE_TNP_FIX
    return false;
#else
    return true;
#endif
    // TODO:  March 2024 consider if any of this RT branch logic makes sense:
//    if (!container) {
//        return false;
//    }
//    auto prop = propDisableMapping(container, /*forced*/ false);
//    if (prop && prop->getValue()) {
//        return true;
//    }
//    if (auto obj = Base::freecad_dynamic_cast<App::DocumentObject>(container)) {
//        if (auto doc = obj->getDocument()) {
//            if (auto prop = propDisableMapping(doc, /*forced*/ false)) {
//                return prop->getValue();
//            }
//        }
//    }
//    return false;
}

// ---------------------------------------------------------

PROPERTY_SOURCE(Part::FilletBase, Part::Feature)

FilletBase::FilletBase()
{
    ADD_PROPERTY(Base,(nullptr));
    ADD_PROPERTY(Edges,(0,0,0));
    Edges.setSize(0);
}

short FilletBase::mustExecute() const
{
    if (Base.isTouched() || Edges.isTouched())
        return 1;
    return 0;
}

// ---------------------------------------------------------

PROPERTY_SOURCE(Part::FeatureExt, Part::Feature)



namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Part::FeaturePython, Part::Feature)
template<> const char* Part::FeaturePython::getViewProviderName() const {
    return "PartGui::ViewProviderPython";
}
template<> PyObject* Part::FeaturePython::getPyObject() {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<Part::PartFeaturePy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class PartExport FeaturePythonT<Part::Feature>;
}

std::vector<Part::cutFaces> Part::findAllFacesCutBy(
        const TopoDS_Shape& shape, const TopoDS_Shape& face, const gp_Dir& dir)
{
    // Find the centre of gravity of the face
    GProp_GProps props;
    BRepGProp::SurfaceProperties(face,props);
    gp_Pnt cog = props.CentreOfMass();

    // create a line through the centre of gravity
    gp_Lin line = gce_MakeLin(cog, dir);

    // Find intersection of line with all faces of the shape
    std::vector<cutFaces> result;
    BRepIntCurveSurface_Inter mkSection;
    // TODO: Less precision than Confusion() should be OK?

    for (mkSection.Init(shape, line, Precision::Confusion()); mkSection.More(); mkSection.Next()) {
        gp_Pnt iPnt = mkSection.Pnt();
        double dsq = cog.SquareDistance(iPnt);

        if (dsq < Precision::Confusion())
            continue; // intersection with original face

        // Find out which side of the original face the intersection is on
        gce_MakeDir mkDir(cog, iPnt);
        if (!mkDir.IsDone())
            continue; // some error (appears highly unlikely to happen, though...)

        if (mkDir.Value().IsOpposite(dir, Precision::Confusion()))
            continue; // wrong side of face (opposite to extrusion direction)

        cutFaces newF;
        newF.face = mkSection.Face();
        newF.distsq = dsq;
        result.push_back(newF);
    }

    return result;
}

bool Part::checkIntersection(const TopoDS_Shape& first, const TopoDS_Shape& second,
                             const bool quick, const bool touch_is_intersection) {

    Bnd_Box first_bb, second_bb;
    BRepBndLib::Add(first, first_bb);
    first_bb.SetGap(0);
    BRepBndLib::Add(second, second_bb);
    second_bb.SetGap(0);

    // Note: This test fails if the objects are touching one another at zero distance

    // Improving reliability: If it fails sometimes when touching and touching is intersection,
    // then please check further unless the user asked for a quick potentially unreliable result
    if (first_bb.IsOut(second_bb) && !touch_is_intersection)
        return false; // no intersection
    if (quick && !first_bb.IsOut(second_bb))
        return true; // assumed intersection

    if (touch_is_intersection) {
        // If both shapes fuse to a single solid, then they intersect
        BRepAlgoAPI_Fuse mkFuse(first, second);
        if (!mkFuse.IsDone())
            return false;
        if (mkFuse.Shape().IsNull())
            return false;

        // Did we get one or two solids?
        TopExp_Explorer xp;
        xp.Init(mkFuse.Shape(),TopAbs_SOLID);
        if (xp.More()) {
            // At least one solid
            xp.Next();
            return (xp.More() == Standard_False);
        } else {
            return false;
        }
    } else {
        // If both shapes have common material, then they intersect
        BRepAlgoAPI_Common mkCommon(first, second);
        if (!mkCommon.IsDone())
            return false;
        if (mkCommon.Shape().IsNull())
            return false;

        // Did we get a solid?
        TopExp_Explorer xp;
        xp.Init(mkCommon.Shape(),TopAbs_SOLID);
        return (xp.More() == Standard_True);
    }

}

/**
 * Override getElementName to support the Export type.  Other calls are passed to the original
 * method
 * @param name The name to search for, or if non existent, name of current Feature is returned
 * @param type An element type name.
 * @return The element name located, of
 */
std::pair<std::string, std::string> Feature::getElementName(const char* name,
                                                            ElementNameType type) const
{
    if (type != ElementNameType::Export) {
        return App::GeoFeature::getElementName(name, type);
    }

    // This function is overridden to provide higher level shape topo names that
    // are generated on demand, e.g. Wire, Shell, Solid, etc.

    auto prop = Base::freecad_dynamic_cast<PropertyPartShape>(getPropertyOfGeometry());
    if (!prop) {
        return App::GeoFeature::getElementName(name, type);
    }

    TopoShape shape = prop->getShape();
    Data::MappedElement mapped = shape.getElementName(name);
    auto res = shape.shapeTypeAndIndex(mapped.index);
    static const int MinLowerTopoNames = 3;
    static const int MaxLowerTopoNames = 10;
    if (res.second && !mapped.name) {
        // Here means valid index name, but no mapped name, check to see if
        // we shall generate the high level topo name.
        //
        // The general idea of the algorithm is to find the minimum number of
        // lower elements that can identify the given higher element, and
        // combine their names to generate the name for the higher element.
        //
        // In theory, all it takes to find one lower element that only appear
        // in the given higher element. To make the algorithm more robust
        // against model changes, we shall take minimum MinLowerTopoNames lower
        // elements.
        //
        // On the other hand, it may be possible to take too many elements for
        // disambiguation. We shall limit to maximum MaxLowerTopoNames. If the
        // chosen elements are not enough to disambiguate the higher element,
        // we'll include an index for disambiguation.

        auto subshape = shape.getSubTopoShape(res.first, res.second, true);
        TopAbs_ShapeEnum lower;
        Data::IndexedName idxName;
        if (!subshape.isNull()) {
            switch (res.first) {
                case TopAbs_WIRE:
                    lower = TopAbs_EDGE;
                    idxName = Data::IndexedName::fromConst("Edge", 1);
                    break;
                case TopAbs_SHELL:
                case TopAbs_SOLID:
                case TopAbs_COMPOUND:
                case TopAbs_COMPSOLID:
                    lower = TopAbs_FACE;
                    idxName = Data::IndexedName::fromConst("Face", 1);
                    break;
                default:
                    lower = TopAbs_SHAPE;
            }
            if (lower != TopAbs_SHAPE) {
                typedef std::pair<size_t, std::vector<int>> NameEntry;
                std::vector<NameEntry> indices;
                std::vector<Data::MappedName> names;
                std::vector<int> ancestors;
                int count = 0;
                for (auto& ss : subshape.getSubTopoShapes(lower)) {
                    auto name = ss.getMappedName(idxName);
                    if (!name) {
                        continue;
                    }
                    indices.emplace_back(name.size(),
                                         shape.findAncestors(ss.getShape(), res.first));
                    names.push_back(name);
                    if (indices.back().second.size() == 1 && ++count >= MinLowerTopoNames) {
                        break;
                    }
                }

                if (names.size() >= MaxLowerTopoNames) {
                    std::stable_sort(indices.begin(),
                                     indices.end(),
                                     [](const NameEntry& a, const NameEntry& b) {
                                         return a.second.size() < b.second.size();
                                     });
                    std::vector<Data::MappedName> sorted;
                    auto pos = 0;
                    sorted.reserve(names.size());
                    for (auto& v : indices) {
                        size_t size = ancestors.size();
                        if (size == 0) {
                            ancestors = v.second;
                        }
                        else if (size > 1) {
                            for (auto it = ancestors.begin(); it != ancestors.end();) {
                                if (std::find(v.second.begin(), v.second.end(), *it)
                                    == v.second.end()) {
                                    it = ancestors.erase(it);
                                    if (ancestors.size() == 1) {
                                        break;
                                    }
                                }
                                else {
                                    ++it;
                                }
                            }
                        }
                        auto itPos = sorted.end();
                        if (size == 1 || size != ancestors.size()) {
                            itPos = sorted.begin() + pos;
                            ++pos;
                        }
                        sorted.insert(itPos, names[v.first]);
                        if (size == 1 && sorted.size() >= MinLowerTopoNames) {
                            break;
                        }
                    }
                }

                names.resize(std::min((int)names.size(), MaxLowerTopoNames));
                if (names.size()) {
                    std::string op;
                    if (ancestors.size() > 1) {
                        // The current chosen elements are not enough to
                        // identify the higher element, generate an index for
                        // disambiguation.
                        auto it = std::find(ancestors.begin(), ancestors.end(), res.second);
                        if (it == ancestors.end()) {
                            assert(0 && "ancestor not found");  // this shouldn't happened
                        }
                        else {
                            op = Data::POSTFIX_TAG + std::to_string(it - ancestors.begin());
                        }
                    }

                    // Note: setting names to shape will change its underlying
                    // shared element name table. This actually violates the
                    // const'ness of this function.
                    //
                    // To be const correct, we should have made the element
                    // name table to be implicit sharing (i.e. copy on change).
                    //
                    // Not sure if there is any side effect of indirectly
                    // change the element map inside the Shape property without
                    // recording the change in undo stack.
                    //
                    mapped.name = shape.setElementComboName(mapped.index,
                                                            names,
                                                            mapped.index.getType(),
                                                            op.c_str());
                }
            }
        }
        return App::GeoFeature::_getElementName(name, mapped);
    }

    if (!res.second && mapped.name) {
        const char* dot = strchr(name, '.');
        if (dot) {
            ++dot;
            // Here means valid mapped name, but cannot find the corresponding
            // indexed name. This usually means the model has been changed. The
            // original indexed name is usually appended to the mapped name
            // separated by a dot. We use it as a clue to decode the combo name
            // set above, and try to single out one sub shape that has all the
            // lower elements encoded in the combo name. But since we don't
            // always use all the lower elements for encoding, this can only be
            // consider a heuristics.
            if (Data::hasMissingElement(dot)) {
                dot += strlen(Data::MISSING_PREFIX);
            }
            std::pair<TopAbs_ShapeEnum, int> occindex = shape.shapeTypeAndIndex(dot);
            if (occindex.second > 0) {
                auto idxName = Data::IndexedName::fromConst(shape.shapeName(occindex.first).c_str(),
                                                            occindex.second);
                std::string postfix;
                auto names =
                    shape.decodeElementComboName(idxName, mapped.name, idxName.getType(), &postfix);
                std::vector<int> ancestors;
                for (auto& name : names) {
                    auto index = shape.getIndexedName(name);
                    if (!index) {
                        ancestors.clear();
                        break;
                    }
                    auto oidx = shape.shapeTypeAndIndex(index);
                    auto subshape = shape.getSubShape(oidx.first, oidx.second);
                    if (subshape.IsNull()) {
                        ancestors.clear();
                        break;
                    }
                    auto current = shape.findAncestors(subshape, occindex.first);
                    if (ancestors.empty()) {
                        ancestors = std::move(current);
                    }
                    else {
                        for (auto it = ancestors.begin(); it != ancestors.end();) {
                            if (std::find(current.begin(), current.end(), *it) == current.end()) {
                                it = ancestors.erase(it);
                            }
                            else {
                                ++it;
                            }
                        }
                        if (ancestors.empty()) {  // model changed beyond recognition, bail!
                            break;
                        }
                    }
                }
                if (ancestors.size() > 1 && boost::starts_with(postfix, Data::POSTFIX_INDEX)) {
                    std::istringstream iss(postfix.c_str() + strlen(Data::POSTFIX_INDEX));
                    int idx;
                    if (iss >> idx && idx >= 0 && idx < (int)ancestors.size()) {
                        ancestors.resize(1, ancestors[idx]);
                    }
                }
                if (ancestors.size() == 1) {
                    idxName.setIndex(ancestors.front());
                    mapped.index = idxName;
                    return App::GeoFeature::_getElementName(name, mapped);
                }
            }
        }
    }

    return App::GeoFeature::getElementName(name, type);
}
