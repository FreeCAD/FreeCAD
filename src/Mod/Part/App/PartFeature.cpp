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
# include <BRepBuilderAPI_MakeShape.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <BRepGProp.hxx>
# include <BRepIntCurveSurface_Inter.hxx>
# include <gce_MakeDir.hxx>
# include <gce_MakeLin.hxx>
# include <gp_Ax1.hxx>
# include <gp_Dir.hxx>
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

App::DocumentObject *Feature::getSubObject(const char *subname,
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int depth) const
{
    // having '.' inside subname means it is referencing some children object,
    // instead of any sub-element from ourself
    if(subname && !Data::isMappedElement(subname) && strchr(subname,'.'))
        return App::DocumentObject::getSubObject(subname,pyObj,pmat,transform,depth);

    Base::Matrix4D _mat;
    auto &mat = pmat?*pmat:_mat;
    if(transform)
        mat *= Placement.getValue().toMatrix();

    if(!pyObj) {
        // TopoShape::hasSubShape is kind of slow, let's cut outself some slack here.
        return const_cast<Feature*>(this);
    }

    try {
        TopoShape ts(Shape.getShape());
        bool doTransform = mat!=ts.getTransform();
        if(doTransform)
            ts.setShape(ts.getShape().Located(TopLoc_Location()));
        if(subname && *subname && !ts.isNull())
            ts = ts.getSubShape(subname);
        if(doTransform && !ts.isNull()) {
            static int sCopy = -1;
            if(sCopy<0) {
                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
                        "User parameter:BaseApp/Preferences/Mod/Part/General");
                sCopy = hGrp->GetBool("CopySubShape",false)?1:0;
            }
            bool copy = sCopy?true:false;
            if(!copy) {
                // Work around OCC bug on transforming circular edge with an
                // offset surface. The bug probably affect other shape type,
                // too.
                TopExp_Explorer exp(ts.getShape(),TopAbs_EDGE);
                if(exp.More()) {
                    auto edge = TopoDS::Edge(exp.Current());
                    exp.Next();
                    if(!exp.More()) {
                        BRepAdaptor_Curve curve(edge);
                        copy = curve.GetType() == GeomAbs_Circle;
                    }
                }
            }
            ts.transformShape(mat,copy,true);
        }
        *pyObj =  Py::new_reference_to(shape2pyshape(ts));
        return const_cast<Feature*>(this);
    }
    catch(Standard_Failure &e) {
        // FIXME: Do not handle the exception here because it leads to a flood of irrelevant and
        // annoying error messages.
        // For example: https://forum.freecad.org/viewtopic.php?f=19&t=42216
        // Instead either raise a sub-class of Base::Exception and let it handle by the calling
        // instance or do simply nothing. For now the error message is degraded to a log message.
        std::ostringstream str;
        Standard_CString msg = e.GetMessageString();

        // Avoid name mangling
        str << e.DynamicType()->get_type_name() << " ";

        if (msg) {str << msg;}
        else     {str << "No OCCT Exception Message";}
        str << ": " << getFullName();
        if (subname)
            str << '.' << subname;
        FC_LOG(str.str());
        return nullptr;
    }
}

TopoDS_Shape Feature::getShape(const App::DocumentObject *obj, const char *subname,
        bool needSubElement, Base::Matrix4D *pmat, App::DocumentObject **powner,
        bool resolveLink, bool transform)
{
    return getTopoShape(obj,subname,needSubElement,pmat,powner,resolveLink,transform,true).getShape();
}

struct ShapeCache {

    std::unordered_map<const App::Document*,
        std::map<std::pair<const App::DocumentObject*, std::string> ,TopoShape> > cache;

    bool inited = false;
    void init() {
        if(inited)
            return;
        inited = true;
        //NOLINTBEGIN
        App::GetApplication().signalDeleteDocument.connect(
                std::bind(&ShapeCache::slotDeleteDocument, this, sp::_1));
        App::GetApplication().signalDeletedObject.connect(
                std::bind(&ShapeCache::slotClear, this, sp::_1));
        App::GetApplication().signalChangedObject.connect(
                std::bind(&ShapeCache::slotChanged, this, sp::_1,sp::_2));
        //NOLINTEND
    }

    void slotDeleteDocument(const App::Document &doc) {
        cache.erase(&doc);
    }

    void slotChanged(const App::DocumentObject &obj, const App::Property &prop) {
        const char *propName = prop.getName();
        if(!App::Property::isValidName(propName))
            return;
        if(strcmp(propName,"Shape")==0
                || strcmp(propName,"Group")==0
                || strstr(propName,"Touched"))
            slotClear(obj);
    }

    void slotClear(const App::DocumentObject &obj) {
        auto it = cache.find(obj.getDocument());
        if(it==cache.end())
            return;
        auto &map = it->second;
        for(auto it2=map.lower_bound(std::make_pair(&obj,std::string()));
                it2!=map.end() && it2->first.first==&obj;)
        {
            it2 = map.erase(it2);
        }
    }

    bool getShape(const App::DocumentObject *obj, TopoShape &shape, const char *subname=nullptr) {
        init();
        auto &entry = cache[obj->getDocument()];
        if(!subname) subname = "";
        auto it = entry.find(std::make_pair(obj,std::string(subname)));
        if(it!=entry.end()) {
            shape = it->second;
            return !shape.isNull();
        }
        return false;
    }

    void setShape(const App::DocumentObject *obj, const TopoShape &shape, const char *subname=nullptr) {
        init();
        if(!subname) subname = "";
        cache[obj->getDocument()][std::make_pair(obj,std::string(subname))] = shape;
    }
};
static ShapeCache _ShapeCache;

void Feature::clearShapeCache() {
    _ShapeCache.cache.clear();
}

static TopoShape _getTopoShape(const App::DocumentObject *obj, const char *subname,
        bool needSubElement, Base::Matrix4D *pmat, App::DocumentObject **powner,
        bool resolveLink, bool noElementMap, std::vector<App::DocumentObject*> &linkStack)

{
    (void) noElementMap;

    TopoShape shape;

    if(!obj)
        return shape;

    PyObject *pyobj = nullptr;
    Base::Matrix4D mat;
    if(powner) *powner = nullptr;

    std::string _subname;
    auto subelement = Data::findElementName(subname);
    if(!needSubElement && subname) {
        // strip out element name if not needed
        if(subelement && *subelement) {
            _subname = std::string(subname,subelement);
            subname = _subname.c_str();
        }
    }

    if(_ShapeCache.getShape(obj,shape,subname)) {
    }

    App::DocumentObject *linked = nullptr;
    App::DocumentObject *owner = nullptr;
    Base::Matrix4D linkMat;
    {
        Base::PyGILStateLocker lock;
        owner = obj->getSubObject(subname,shape.isNull()?&pyobj:nullptr,&mat,false);
        if(!owner)
            return shape;
        linked = owner->getLinkedObject(true,&linkMat,false);
        if(pmat) {
            if(resolveLink && obj!=owner)
                *pmat = mat * linkMat;
            else
                *pmat = mat;
        }
        if(!linked)
            linked = owner;
        if(powner)
            *powner = resolveLink?linked:owner;

        if(!shape.isNull())
            return shape;

        if(pyobj && PyObject_TypeCheck(pyobj,&TopoShapePy::Type)) {
            shape = *static_cast<TopoShapePy*>(pyobj)->getTopoShapePtr();
            if(!shape.isNull()) {
                if(obj->getDocument() != linked->getDocument())
                    _ShapeCache.setShape(obj,shape,subname);
                Py_DECREF(pyobj);
                return shape;
            }
        }

        Py_XDECREF(pyobj);
    }

    // nothing can be done if there is sub-element references
    if(needSubElement && subelement && *subelement)
        return shape;

    bool scaled = false;
    if(obj!=owner) {
        if(_ShapeCache.getShape(owner,shape)) {
            auto scaled = shape.transformShape(mat,false,true);
            if(owner->getDocument()!=obj->getDocument()) {
                // shape.reTagElementMap(obj->getID(),obj->getDocument()->getStringHasher());
                _ShapeCache.setShape(obj,shape,subname);
            } else if(scaled)
                _ShapeCache.setShape(obj,shape,subname);
        }
        if(!shape.isNull()) {
            return shape;
        }
    }

    auto link = owner->getExtensionByType<App::LinkBaseExtension>(true);
    if(owner!=linked
            && (!link || (!link->_ChildCache.getSize()
                            && link->getSubElements().size()<=1)))
    {
        // if there is a linked object, and there is no child cache (which is used
        // for special handling of plain group), obtain shape from the linked object
        shape = Feature::getTopoShape(linked,nullptr,false,nullptr,nullptr,false,false);
        if(shape.isNull())
            return shape;
        if(owner==obj)
            shape.transformShape(mat*linkMat,false,true);
        else
            shape.transformShape(linkMat,false,true);

    } else {

        if(link || owner->getExtensionByType<App::GeoFeatureGroupExtension>(true))
            linkStack.push_back(owner);

        // Construct a compound of sub objects
        std::vector<TopoShape> shapes;

        // Acceleration for link array. Unlike non-array link, a link array does
        // not return the linked object when calling getLinkedObject().
        // Therefore, it should be handled here.
        TopoShape baseShape;
        Base::Matrix4D baseMat;
        std::string op;
        if(link && link->getElementCountValue()) {
            linked = link->getTrueLinkedObject(false,&baseMat);
            if(linked && linked!=owner) {
                baseShape = Feature::getTopoShape(linked,nullptr,false,nullptr,nullptr,false,false);
            }
        }
        for(auto &sub : owner->getSubObjects()) {
            if(sub.empty()) continue;
            int visible;
            std::string childName;
            App::DocumentObject *parent=nullptr;
            Base::Matrix4D mat = baseMat;
            App::DocumentObject *subObj=nullptr;
            if(sub.find('.')==std::string::npos)
                visible = 1;
            else {
                subObj = owner->resolve(sub.c_str(), &parent, &childName,nullptr,nullptr,&mat,false);
                if(!parent || !subObj)
                    continue;
                if(!linkStack.empty()
                    && parent->getExtensionByType<App::GroupExtension>(true,false))
                {
                    visible = linkStack.back()->isElementVisible(childName.c_str());
                }else
                    visible = parent->isElementVisible(childName.c_str());
            }
            if(visible==0)
                continue;
            TopoShape shape;
            if(!subObj || baseShape.isNull()) {
                shape = _getTopoShape(owner,sub.c_str(),true,nullptr,&subObj,false,false,linkStack);
                if(shape.isNull())
                    continue;
                if(visible<0 && subObj && !subObj->Visibility.getValue())
                    continue;
            }else{
                if(link && !link->getShowElementValue())
                    shape = baseShape.makeTransform(mat,(Data::POSTFIX_INDEX + childName).c_str());
                else {
                    shape = baseShape.makeTransform(mat);
                }
            }
            shapes.push_back(shape);
        }

        if(!linkStack.empty() && linkStack.back()==owner)
            linkStack.pop_back();

        if(shapes.empty())
            return shape;

        shape.makeCompound(shapes);
    }

    _ShapeCache.setShape(owner,shape);

    if(owner!=obj) {
        scaled = shape.transformShape(mat,false,true);
        if(owner->getDocument()!=obj->getDocument()) {
            _ShapeCache.setShape(obj,shape,subname);
        }else if(scaled)
            _ShapeCache.setShape(obj,shape,subname);
    }
    return shape;
}

TopoShape Feature::getTopoShape(const App::DocumentObject *obj, const char *subname,
        bool needSubElement, Base::Matrix4D *pmat, App::DocumentObject **powner,
        bool resolveLink, bool transform, bool noElementMap)
{
    if(!obj || !obj->getNameInDocument())
        return {};

    std::vector<App::DocumentObject*> linkStack;

    // NOTE! _getTopoShape() always return shape without top level
    // transformation for easy shape caching, i.e.  with `transform` set
    // to false. So we manually apply the top level transform if asked.

    Base::Matrix4D mat;
    auto shape = _getTopoShape(obj, subname, needSubElement, &mat,
            powner, resolveLink, noElementMap, linkStack);

    Base::Matrix4D topMat;
    if(pmat || transform) {
        // Obtain top level transformation
        if(pmat)
            topMat = *pmat;
        if(transform)
            obj->getSubObject(nullptr,nullptr,&topMat);

        // Apply the top level transformation
        if(!shape.isNull())
            shape.transformShape(topMat,false,true);

        if(pmat)
            *pmat = topMat * mat;
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
