/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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
# include <gp_Lin.hxx>
# include <gp_Pln.hxx>
# include <BRep_Builder.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
#endif

#include <unordered_map>
#include <unordered_set>
#include <boost/algorithm/string/predicate.hpp>

#include <App/Application.h>
#include <App/Document.h>
#include <App/GroupExtension.h>
#include <App/Link.h>
#include <App/OriginFeature.h>
#include <App/ElementNamingUtils.h>
#include <Mod/Part/App/TopoShape.h>

#include "ShapeBinder.h"

FC_LOG_LEVEL_INIT("PartDesign",true,true)

#ifndef M_PI
# define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;
namespace sp = std::placeholders;

// ============================================================================

PROPERTY_SOURCE(PartDesign::ShapeBinder, Part::Feature)

ShapeBinder::ShapeBinder()
{
    ADD_PROPERTY_TYPE(Support, (nullptr, nullptr), "", (App::PropertyType)(App::Prop_None), "Support of the geometry");
    Placement.setStatus(App::Property::Hidden, true);
    ADD_PROPERTY_TYPE(TraceSupport, (false), "", App::Prop_None, "Trace support shape");
}

ShapeBinder::~ShapeBinder()
{
    this->connectDocumentChangedObject.disconnect();
}

void ShapeBinder::onChanged(const App::Property* prop)
{
    Feature::onChanged(prop);
}

short int ShapeBinder::mustExecute() const {

    if (Support.isTouched())
        return 1;
    if (TraceSupport.isTouched())
        return 1;

    return Part::Feature::mustExecute();
}

Part::TopoShape ShapeBinder::updatedShape() const
{
    Part::TopoShape shape;
    App::GeoFeature* obj = nullptr;
    std::vector<std::string> subs;

    ShapeBinder::getFilteredReferences(&Support, obj, subs);

    //if we have a link we rebuild the shape, but we change nothing if we are a simple copy
    if (obj) {
        shape = ShapeBinder::buildShapeFromReferences(obj, subs);
        //now, shape is in object's CS, and includes local Placement of obj but nothing else.

        if (TraceSupport.getValue()) {
            //compute the transform, and apply it to the shape.
            Base::Placement sourceCS = //full placement of container of obj
                obj->globalPlacement() * obj->Placement.getValue().inverse();
            Base::Placement targetCS = //full placement of container of this shapebinder
                this->globalPlacement() * this->Placement.getValue().inverse();
            Base::Placement transform = targetCS.inverse() * sourceCS;
            shape.setPlacement(transform * shape.getPlacement());
        }
    }

    return shape;
}

bool ShapeBinder::hasPlacementChanged() const
{
    Part::TopoShape shape(updatedShape());
    Base::Placement placement(shape.getTransform());
    return this->Placement.getValue() != placement;
}

App::DocumentObjectExecReturn* ShapeBinder::execute() {

    if (!this->isRestoring()) {
        Part::TopoShape shape(updatedShape());
        if (!shape.isNull()) {
            this->Placement.setValue(shape.getTransform());
            this->Shape.setValue(shape);
        }
        else {
            this->Shape.setValue(shape);
        }
    }

    return Part::Feature::execute();
}

void ShapeBinder::getFilteredReferences(const App::PropertyLinkSubList* prop,
                                        App::GeoFeature*& obj,
                                        std::vector< std::string >& subobjects)
{
    obj = nullptr;
    subobjects.clear();

    auto objs = prop->getValues();
    auto subs = prop->getSubValues();

    if (objs.empty()) {
        return;
    }

    //we only allow one part feature, so get the first one we find
    size_t index = 0;
    for (auto* it : objs) {
        if (it && it->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            obj = static_cast<Part::Feature*>(it);
            break;
        }
        index++;
    }

    //do we have any part feature?
    if (obj) {
        //if we have no subshpape we use the whole shape
        if (subs[index].empty()) {
            return;
        }

        //collect all subshapes for the object
        for (index = 0; index < objs.size(); index++) {
            //we only allow subshapes from a single Part::Feature
            if (objs[index] != obj)
                continue;

            //in this mode the full shape is not allowed, as we already started the subshape
            //processing
            if (subs[index].empty())
                continue;

            subobjects.push_back(subs[index]);
        }
    }
    else {
        // search for Origin features
        for (auto* it : objs) {
            if (it && it->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
                obj = static_cast<App::GeoFeature*>(it);
                break;
            }
            else if (it && it->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
                obj = static_cast<App::GeoFeature*>(it);
                break;
            }
        }
    }
}

Part::TopoShape ShapeBinder::buildShapeFromReferences(App::GeoFeature* obj, std::vector< std::string > subs) {

    if (!obj)
        return TopoDS_Shape();

    if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        Part::Feature* part = static_cast<Part::Feature*>(obj);
        if (subs.empty())
            return part->Shape.getValue();

        std::vector<TopoDS_Shape> shapes;
        for (const std::string& sub : subs) {
            shapes.push_back(part->Shape.getShape().getSubShape(sub.c_str()));
        }

        if (shapes.size() == 1) {
            //single subshape. Return directly.
            return shapes[0];
        }
        else {
            //multiple subshapes. Make a compound.
            BRep_Builder builder;
            TopoDS_Compound cmp;
            builder.MakeCompound(cmp);
            for (const TopoDS_Shape& sh : shapes) {
                builder.Add(cmp, sh);
            }
            return cmp;
        }
    }
    else if (obj->getTypeId().isDerivedFrom(App::Line::getClassTypeId())) {
        gp_Lin line;
        BRepBuilderAPI_MakeEdge mkEdge(line);
        Part::TopoShape shape(mkEdge.Shape());
        shape.setPlacement(obj->Placement.getValue());
        return shape;
    }
    else if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
        gp_Pln plane;
        BRepBuilderAPI_MakeFace mkFace(plane);
        Part::TopoShape shape(mkFace.Shape());
        shape.setPlacement(obj->Placement.getValue());
        return shape;
    }

    return TopoDS_Shape();
}

void ShapeBinder::handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop)
{
    // The type of Support was App::PropertyLinkSubList in the past
    if (prop == &Support && strcmp(TypeName, "App::PropertyLinkSubList") == 0) {
        Support.Restore(reader);
    }
    else {
        Part::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

void ShapeBinder::onSettingDocument()
{
    App::Document* document = getDocument();
    if (document) {
        //NOLINTBEGIN
        this->connectDocumentChangedObject = document->signalChangedObject.connect(std::bind
            (&ShapeBinder::slotChangedObject, this, sp::_1, sp::_2));
        //NOLINTEND
    }
}

void ShapeBinder::slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
{
    App::Document* doc = getDocument();
    if (!doc || doc->testStatus(App::Document::Restoring))
        return;
    if (this == &Obj)
        return;
    if (!TraceSupport.getValue())
        return;
    if (!Prop.getTypeId().isDerivedFrom(App::PropertyPlacement::getClassTypeId()))
        return;

    App::GeoFeature* obj = nullptr;
    std::vector<std::string> subs;
    ShapeBinder::getFilteredReferences(&Support, obj, subs);
    if (obj) {
        if (obj == &Obj) {
            // the directly referenced object has changed
            if (hasPlacementChanged())
                enforceRecompute();
        }
        else if (Obj.hasExtension(App::GroupExtension::getExtensionClassTypeId())) {
            // check if the changed property belongs to a group-like object
            // like Body or Part
            std::vector<App::DocumentObject*> chain;
            std::vector<App::DocumentObject*> list = getInListRecursive();
            chain.insert(chain.end(), list.begin(), list.end());
            list = obj->getInListRecursive();
            chain.insert(chain.end(), list.begin(), list.end());

            auto it = std::find(chain.begin(), chain.end(), &Obj);
            if (it != chain.end()) {
                if (hasPlacementChanged())
                    enforceRecompute();
            }
        }
    }
}

// ============================================================================

PROPERTY_SOURCE(PartDesign::SubShapeBinder, Part::Feature)

SubShapeBinder::SubShapeBinder()
{
    ADD_PROPERTY_TYPE(Support, (nullptr), "", (App::PropertyType)(App::Prop_None), "Support of the geometry");
    Support.setStatus(App::Property::ReadOnly, true);
    ADD_PROPERTY_TYPE(Fuse, (false), "Base", App::Prop_None, "Fuse solids from bound shapes");
    ADD_PROPERTY_TYPE(MakeFace, (true), "Base", App::Prop_None, "Create face using wires from bound shapes");
    ADD_PROPERTY_TYPE(Offset, (0.0), "Offsetting", App::Prop_None, "2D offset face or wires, 0.0 = no offset");
    ADD_PROPERTY_TYPE(OffsetJoinType, ((long)0), "Offsetting", App::Prop_None, "Arcs, Tangent, Intersection");
    static const char* JoinTypeEnum[] = { "Arcs", "Tangent", "Intersection", nullptr };
    OffsetJoinType.setEnums(JoinTypeEnum);
    ADD_PROPERTY_TYPE(OffsetFill, (false), "Offsetting", App::Prop_None, "True = make face between original wire and offset.");
    ADD_PROPERTY_TYPE(OffsetOpenResult, (false), "Offsetting", App::Prop_None, "False = make closed offset from open wire.");
    ADD_PROPERTY_TYPE(OffsetIntersection, (false), "Offsetting", App::Prop_None, "False = offset child wires independently.");
    ADD_PROPERTY_TYPE(ClaimChildren, (false), "Base", App::Prop_Output, "Claim linked object as children");
    ADD_PROPERTY_TYPE(Relative, (true), "Base", App::Prop_None, "Enable relative sub-object binding");
    ADD_PROPERTY_TYPE(BindMode, ((long)0), "Base", App::Prop_None,
        "Synchronized: auto update binder shape on changed of bound object.\n"
        "Frozen: disable auto update, but can be updated manually using context menu.\n"
        "Detached: copy the shape of bound object and then remove the binding immediately.");
    ADD_PROPERTY_TYPE(PartialLoad, (false), "Base", App::Prop_None,
        "Enable partial loading, which disables auto loading of external document for"
        "external bound object.");
    PartialLoad.setStatus(App::Property::PartialTrigger, true);
    static const char* BindModeEnum[] = { "Synchronized", "Frozen", "Detached", nullptr };
    BindMode.setEnums(BindModeEnum);

    ADD_PROPERTY_TYPE(Context, (nullptr), "Base", App::Prop_Hidden,
        "Stores the context of this binder. It is used for monitoring and auto updating\n"
        "the relative placement of the bound shape");

    static const char* BindCopyOnChangeEnum[] = { "Disabled", "Enabled", "Mutated", nullptr };
    BindCopyOnChange.setEnums(BindCopyOnChangeEnum);
    ADD_PROPERTY_TYPE(BindCopyOnChange, ((long)0), "Base", App::Prop_None,
        "Disabled: disable copy on change.\n"
        "Enabled: duplicate properties from binding object that are marked with 'CopyOnChange'.\n"
        "         Make internal copy of the object with any changed properties to obtain the\n"
        "         shape of an alternative configuration\n"
        "Mutated: indicate the binder has already mutated by changing any properties marked with\n"
        "         'CopyOnChange'. Those properties will not longer be kept in sync between the\n"
        "         binder and the binding object");

    ADD_PROPERTY_TYPE(Refine, (true), "Base", (App::PropertyType)(App::Prop_None),
        "Refine shape (clean up redundant edges) after adding/subtracting");

    Context.setScope(App::LinkScope::Hidden);

    ADD_PROPERTY_TYPE(_Version, (0), "Base", (App::PropertyType)(
        App::Prop_Hidden | App::Prop_ReadOnly), "");

    _CopiedLink.setScope(App::LinkScope::Hidden);
    ADD_PROPERTY_TYPE(_CopiedLink, (nullptr), "Base", (App::PropertyType)(
        App::Prop_Hidden | App::Prop_ReadOnly | App::Prop_NoPersist), "");
}

SubShapeBinder::~SubShapeBinder() {
    try {
        clearCopiedObjects();
    }
    catch (const Base::ValueError& e) {
        e.ReportException();
    }
}

void SubShapeBinder::setupObject() {
    _Version.setValue(2);
    checkPropertyStatus();

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    this->Refine.setValue(hGrp->GetBool("RefineModel", false));
}

App::DocumentObject* SubShapeBinder::getSubObject(const char* subname, PyObject** pyObj,
    Base::Matrix4D* mat, bool transform, int depth) const
{
    auto sobj = Part::Feature::getSubObject(subname, pyObj, mat, transform, depth);
    if (sobj)
        return sobj;
    if (Data::findElementName(subname) == subname)
        return nullptr;

    const char* dot = strchr(subname, '.');
    if (!dot)
        return nullptr;

    App::GetApplication().checkLinkDepth(depth);
    std::string name(subname, dot - subname);
    for (auto& l : Support.getSubListValues()) {
        auto obj = l.getValue();
        if (!obj || !obj->getNameInDocument())
            continue;
        for (auto& sub : l.getSubValues()) {
            auto sobj = obj->getSubObject(sub.c_str());
            if (!sobj || !sobj->getNameInDocument())
                continue;
            if (subname[0] == '$') {
                if (sobj->Label.getStrValue() != name.c_str() + 1)
                    continue;
            }
            else if (!boost::equals(sobj->getNameInDocument(), name))
                continue;
            name = Data::noElementName(sub.c_str());
            name += dot + 1;
            if (mat && transform)
                *mat *= Placement.getValue().toMatrix();
            return obj->getSubObject(name.c_str(), pyObj, mat, true, depth + 1);
        }
    }
    return nullptr;
}

void SubShapeBinder::setupCopyOnChange() {
    copyOnChangeConns.clear();

    const auto& support = Support.getSubListValues();
    if (BindCopyOnChange.getValue() == 0 || support.size() != 1) {
        if (hasCopyOnChange) {
            hasCopyOnChange = false;
            std::vector<App::Property*> props;
            getPropertyList(props);
            for (auto prop : props) {
                if (App::LinkBaseExtension::isCopyOnChangeProperty(this, *prop)) {
                    try {
                        removeDynamicProperty(prop->getName());
                    }
                    catch (Base::Exception& e) {
                        e.ReportException();
                    }
                    catch (...) {
                    }
                }
            }
        }
        return;
    }

    auto linked = support.front().getValue();
    hasCopyOnChange = App::LinkBaseExtension::setupCopyOnChange(this, linked,
        BindCopyOnChange.getValue() == 1 ? &copyOnChangeConns : nullptr, hasCopyOnChange);
    if (hasCopyOnChange) {
        copyOnChangeConns.emplace_back(linked->signalChanged.connect(
            [this](const App::DocumentObject&, const App::Property& prop) {
            if (!prop.testStatus(App::Property::Output)
                && !prop.testStatus(App::Property::PropOutput))
            {
                if (!this->_CopiedObjs.empty()) {
                    FC_LOG("Clear binder " << getFullName() << " cache on change of "
                        << prop.getFullName());
                    this->clearCopiedObjects();
                }
            }
        }
        ));
    }
}

void SubShapeBinder::clearCopiedObjects() {
    std::vector<App::DocumentObjectT> objs;
    objs.swap(_CopiedObjs);
    for (auto& o : objs) {
        auto obj = o.getObject();
        if (obj)
            obj->getDocument()->removeObject(obj->getNameInDocument());
    }
    _CopiedLink.setValue(nullptr);
}

void SubShapeBinder::update(SubShapeBinder::UpdateOption options) {
    Part::TopoShape result;
    std::vector<Part::TopoShape> shapes;
    std::vector<const Base::Matrix4D*> shapeMats;

    bool forced = (Shape.getValue().IsNull() || (options & UpdateForced)) ? true : false;
    bool init = (!forced && (options & UpdateForced)) ? true : false;

    std::string errMsg;
    auto parent = Context.getValue();
    std::string parentSub = Context.getSubName(false);
    if (!Relative.getValue())
        parent = nullptr;
    else {
        if (parent && parent->getSubObject(parentSub.c_str()) == this) {
            auto parents = parent->getParents();
            if (!parents.empty()) {
                parent = parents.begin()->first;
                parentSub = parents.begin()->second + parentSub;
            }
        }
        else
            parent = nullptr;
        if (!parent && parentSub.empty()) {
            auto parents = getParents();
            if (!parents.empty()) {
                parent = parents.begin()->first;
                parentSub = parents.begin()->second;
            }
        }
        if (parent && (parent != Context.getValue() || parentSub != Context.getSubName(false)))
            Context.setValue(parent, parentSub.c_str());
    }

    bool first = false;
    std::unordered_map<const App::DocumentObject*, Base::Matrix4D> mats;
    for (auto& l : Support.getSubListValues()) {
        auto obj = l.getValue();
        if (!obj || !obj->getNameInDocument())
            continue;
        auto res = mats.emplace(obj, Base::Matrix4D());
        if (parent && res.second) {
            std::string resolvedSub = parentSub;
            std::string linkSub;
            auto link = obj;
            auto resolved = parent->resolveRelativeLink(resolvedSub, link, linkSub);
            if (!resolved) {
                if (!link) {
                    FC_WARN(getFullName() << " cannot resolve relative link of "
                        << parent->getFullName() << '.' << parentSub
                        << " -> " << obj->getFullName());
                }
            }
            else {
                Base::Matrix4D mat;
                auto sobj = resolved->getSubObject(resolvedSub.c_str(), nullptr, &mat);
                if (sobj != this) {
                    FC_LOG(getFullName() << " skip invalid parent " << resolved->getFullName()
                        << '.' << resolvedSub);
                }
                else if (_Version.getValue() == 0) {
                    // For existing legacy SubShapeBinder, we use its Placement
                    // to store the position adjustment of the first Support
                    if (first) {
                        auto pla = Placement.getValue() * Base::Placement(mat).inverse();
                        Placement.setValue(pla);
                        first = false;
                    }
                    else {
                        // The remaining support will cancel the Placement
                        mat.inverseGauss();
                        res.first->second = mat;
                    }
                }
                else {
                    // For newer SubShapeBinder, the Placement property is free
                    // to use by the user to add additional offset to the
                    // binding object
                    mat.inverseGauss();
                    res.first->second = Placement.getValue().toMatrix() * mat;
                }
            }
        }
        if (init)
            continue;

        App::DocumentObject* copied = nullptr;

        if (BindCopyOnChange.getValue() == 2 && Support.getSubListValues().size() == 1) {
            if (!_CopiedObjs.empty())
                copied = _CopiedObjs.front().getObject();

            bool recomputeCopy = false;
            int copyerror = 0;
            if (!copied || !copied->isValid()) {
                recomputeCopy = true;
                clearCopiedObjects();

                auto tmpDoc = App::GetApplication().newDocument(
                    "_tmp_binder", nullptr, false, true);
                tmpDoc->setUndoMode(0);
                auto objs = tmpDoc->copyObject({ obj }, true, true);
                if (!objs.empty()) {
                    for (auto it = objs.rbegin(); it != objs.rend(); ++it)
                        _CopiedObjs.emplace_back(*it);
                    copied = objs.back();
                    // IMPORTANT! must make a recomputation first before any
                    // further change so that we can generate the correct
                    // geometry element map.
                    if (!copied->recomputeFeature(true))
                        copyerror = 1;
                }
            }

            if (copied) {
                if (!copyerror) {
                    std::vector<App::Property*> props;
                    getPropertyList(props);
                    for (auto prop : props) {
                        if (!App::LinkBaseExtension::isCopyOnChangeProperty(this, *prop))
                            continue;
                        auto p = copied->getPropertyByName(prop->getName());
                        if (p && p->getContainer() == copied
                            && p->getTypeId() == prop->getTypeId()
                            && !p->isSame(*prop))
                        {
                            recomputeCopy = true;
                            std::unique_ptr<App::Property> pcopy(prop->Copy());
                            p->Paste(*pcopy);
                        }
                    }
                    if (recomputeCopy && !copied->recomputeFeature(true))
                        copyerror = 2;
                }
                obj = copied;
                _CopiedLink.setValue(copied, l.getSubValues(false));
                if (copyerror) {
                    FC_THROWM(Base::RuntimeError,
                        (copyerror == 1 ? "Initial copy failed." : "Copy on change failed.")
                        << " Please check report view for more details.\n"
                        "You can show temporary document to reveal the failed objects using\n"
                        "tree view context menu.");
                }
            }
        }

        const auto& subvals = copied ? _CopiedLink.getSubValues() : l.getSubValues();
        std::set<std::string> subs(subvals.begin(), subvals.end());
        static std::string none;
        if (subs.empty())
            subs.insert(none);
        else if (subs.size() > 1)
            subs.erase(none);
        for (const auto& sub : subs) {
            try {
                auto shape = Part::Feature::getTopoShape(obj, sub.c_str(), true);
                if (!shape.isNull()) {
                    shapes.push_back(shape);
                    shapeMats.push_back(&res.first->second);
                }
            }
            catch (Base::Exception& e) {
                e.ReportException();
                FC_ERR(getFullName() << " failed to obtain shape from "
                    << obj->getFullName() << '.' << sub);
                if (errMsg.empty()) {
                    std::ostringstream ss;
                    ss << "Failed to obtain shape " <<
                        obj->getFullName() << '.'
                        << Data::oldElementName(sub.c_str());
                    errMsg = ss.str();
                }
            }
        }
    }

    std::string objName;
    // lambda function for generating matrix cache property
    auto cacheName = [this, &objName](const App::DocumentObject* obj) {
        objName = "Cache_";
        objName += obj->getNameInDocument();
        if (obj->getDocument() != getDocument()) {
            objName += "_";
            objName += obj->getDocument()->getName();
        }
        return objName.c_str();
    };

    if (!init) {

        if (!errMsg.empty()) {
            if (!(options & UpdateInit))
                FC_THROWM(Base::RuntimeError, errMsg);
            if (!Shape.getValue().IsNull())
                return;
        }

        // If not forced, only rebuild when there is any change in
        // transformation matrix
        if (!forced) {
            bool hit = true;
            for (auto& v : mats) {
                auto prop = Base::freecad_dynamic_cast<App::PropertyMatrix>(
                    getDynamicPropertyByName(cacheName(v.first)));
                if (!prop || prop->getValue() != v.second) {
                    hit = false;
                    break;
                }
            }
            if (hit)
                return;
        }

        if (shapes.size() == 1 && !Relative.getValue())
            shapes.back().setPlacement(Base::Placement());
        else {
            for (size_t i = 0; i < shapes.size(); ++i) {
                auto& shape = shapes[i];
                shape = shape.makeTransform(*shapeMats[i]);
                // if(shape.Hasher
                //         && shape.getElementMapSize()
                //         && shape.Hasher != getDocument()->getStringHasher())
                // {
                //     shape.reTagElementMap(getID(),
                //             getDocument()->getStringHasher(),TOPOP_SHAPEBINDER);
                // }
            }
        }

        if (shapes.empty()) {
            // Shape.resetElementMapVersion();
            return;
        }

        result.makeCompound(shapes);

        bool fused = false;
        if (Fuse.getValue()) {
            // If the compound has solid, fuse them together, and ignore other type of
            // shapes
            std::vector<TopoDS_Shape> solids;
            Part::TopoShape solid;
            for (auto& s : result.getSubTopoShapes(TopAbs_SOLID)) {
                if (solid.isNull())
                    solid = s;
                else
                    solids.push_back(s.getShape());
            }
            if (!solids.empty()) {
                result = solid.fuse(solids);
                fused = true;
            }
            else if (!solid.isNull()) {
                // wrap the single solid in compound to keep its placement
                result.makeCompound({ solid });
                fused = true;
            }
        }

        if (!fused && (MakeFace.getValue() || Offset.getValue() != 0.0)
            && !result.hasSubShape(TopAbs_FACE)
            && result.hasSubShape(TopAbs_EDGE))
        {
            result = result.makeWires();
            if (MakeFace.getValue()) {
                try {
                    result = result.makeFace(nullptr);
                }
                catch (...) {}
            }
        }

        if (!fused && result.hasSubShape(TopAbs_WIRE)
            && Offset.getValue() != 0.0) {
            try {
                result = result.makeOffset2D(Offset.getValue(),
                                             OffsetJoinType.getValue(),
                                             OffsetFill.getValue(),
                                             OffsetOpenResult.getValue(),
                                             OffsetIntersection.getValue());
            }
            catch (...) {
                std::ostringstream msg;
                msg << Label.getValue() << ": failed to make 2D offset" << std::endl;
                Base::Console().Error(msg.str().c_str());
            }
        }

        if (Refine.getValue())
            result = result.makeRefine();

        result.setPlacement(Placement.getValue());
        Shape.setValue(result);
    }

    // collect transformation matrix cache entries
    std::unordered_set<std::string> caches;
    for (const auto& name : getDynamicPropertyNames()) {
        if (boost::starts_with(name, "Cache_"))
            caches.emplace(name);
    }

    // update transformation matrix cache
    for (auto& v : mats) {
        const char* name = cacheName(v.first);
        auto prop = getDynamicPropertyByName(name);
        if (!prop || !prop->isDerivedFrom(App::PropertyMatrix::getClassTypeId())) {
            if (prop)
                removeDynamicProperty(name);
            prop = addDynamicProperty("App::PropertyMatrix", name, "Cache", nullptr, 0, false, true);
        }
        caches.erase(name);
        static_cast<App::PropertyMatrix*>(prop)->setValue(v.second);
    }

    // remove any non-used cache entries.
    for (const auto& name : caches) {
        try {
            removeDynamicProperty(name.c_str());
        }
        catch (...) {}
    }
}

void SubShapeBinder::slotRecomputedObject(const App::DocumentObject& Obj) {
    if (Context.getValue() == &Obj
        && !this->testStatus(App::ObjectStatus::Recompute2))
    {
        try {
            update();
        }
        catch (Base::Exception& e) {
            e.ReportException();
        }
    }
}

App::DocumentObjectExecReturn* SubShapeBinder::execute() {

    setupCopyOnChange();

    if (BindMode.getValue() == 0)
        update(UpdateForced);

    return inherited::execute();
}

void SubShapeBinder::onDocumentRestored() {
    if (_Version.getValue() < 2)
        update(UpdateInit);
    inherited::onDocumentRestored();
}

void SubShapeBinder::onChanged(const App::Property* prop) {
    if (prop == &Context || prop == &Relative) {
        if (!Context.getValue() || !Relative.getValue()) {
            connRecomputedObj.disconnect();
        }
        else if (contextDoc != Context.getValue()->getDocument()
            || !connRecomputedObj.connected())
        {
            //NOLINTBEGIN
            contextDoc = Context.getValue()->getDocument();
            connRecomputedObj = contextDoc->signalRecomputedObject.connect(
                std::bind(&SubShapeBinder::slotRecomputedObject, this, sp::_1));
            //NOLINTEND
        }
    }
    else if (!isRestoring()) {
        if (prop == &Support) {
            clearCopiedObjects();
            setupCopyOnChange();
            if (!Support.getSubListValues().empty()) {
                update();
                if (BindMode.getValue() == 2)
                    Support.setValue(nullptr);
            }
        }
        else if (prop == &BindCopyOnChange) {
            setupCopyOnChange();
        }
        else if (prop == &BindMode) {
            if (BindMode.getValue() == 2)
                Support.setValue(nullptr);
            else if (BindMode.getValue() == 0)
                update();
            checkPropertyStatus();
        }
        else if (prop == &PartialLoad) {
            checkPropertyStatus();
        }
        else if (prop && !prop->testStatus(App::Property::User3))
            checkCopyOnChange(*prop);
    }
    inherited::onChanged(prop);
}

void SubShapeBinder::checkCopyOnChange(const App::Property& prop) {
    if (BindCopyOnChange.getValue() != 1
        || getDocument()->isPerformingTransaction()
        || !App::LinkBaseExtension::isCopyOnChangeProperty(this, prop)
        || Support.getSubListValues().size() != 1)
        return;

    auto linked = Support.getSubListValues().front().getValue();
    if (!linked)
        return;
    auto linkedProp = linked->getPropertyByName(prop.getName());
    if (linkedProp && linkedProp->getTypeId() == prop.getTypeId() && !linkedProp->isSame(prop))
        BindCopyOnChange.setValue(2);
}

void SubShapeBinder::checkPropertyStatus() {
    Support.setAllowPartial(PartialLoad.getValue());

    // Make Shape transient can reduce some file size, and maybe reduce file
    // loading time as well. But there maybe complication arise when doing
    // TopoShape version upgrade. So we DO NOT set trasient at the moment.
    //
    // Shape.setStatus(App::Property::Transient, !PartialLoad.getValue() && BindMode.getValue()==0);
}

void SubShapeBinder::setLinks(std::map<App::DocumentObject*, std::vector<std::string> >&& values, bool reset)
{
    if (values.empty()) {
        if (reset) {
            Support.setValue(nullptr);
            Shape.setValue(Part::TopoShape());
        }
        return;
    }
    auto inSet = getInListEx(true);
    inSet.insert(this);

    for (auto& v : values) {
        if (!v.first || !v.first->getNameInDocument())
            FC_THROWM(Base::ValueError, "Invalid document object");
        if (inSet.find(v.first) != inSet.end())
            FC_THROWM(Base::ValueError, "Cyclic reference to " << v.first->getFullName());

        if (v.second.empty()) {
            v.second.emplace_back("");
            continue;
        }

        std::vector<std::string> wholeSubs;
        for (auto& sub : v.second) {
            if (sub.empty()) {
                wholeSubs.clear();
                v.second.resize(1);
                v.second[0].clear();
                break;
            }
            else if (sub[sub.size() - 1] == '.')
                wholeSubs.push_back(sub);
        }
        for (auto& whole : wholeSubs) {
            for (auto it = v.second.begin(); it != v.second.end();) {
                auto& sub = *it;
                if (!boost::starts_with(sub, whole) || sub.size() == whole.size())
                    ++it;
                else {
                    FC_LOG("Remove subname " << sub << " because of whole selection " << whole);
                    it = v.second.erase(it);
                }
            }
        }
    }

    if (!reset) {
        for (auto& link : Support.getSubListValues()) {
            auto subs = link.getSubValues();
            auto& s = values[link.getValue()];
            if (s.empty()) {
                s = std::move(subs);
                continue;
            }
            else if (subs.empty() || s[0].empty())
                continue;

            for (auto& sub : s) {
                for (auto it = subs.begin(); it != subs.end();) {
                    if (sub[sub.size() - 1] == '.') {
                        if (boost::starts_with(*it, sub)) {
                            FC_LOG("Remove subname " << *it << " because of whole selection " << sub);
                            it = subs.erase(it);
                        }
                        else
                            ++it;
                    }
                    else if (it->empty()
                        || (it->back() == '.' && boost::starts_with(sub, *it)))
                    {
                        FC_LOG("Remove whole subname " << *it << " because of " << sub);
                        it = subs.erase(it);
                    }
                    else
                        ++it;
                }
            }
            subs.insert(subs.end(), s.begin(), s.end());
            s = std::move(subs);
        }
    }
    Support.setValues(std::move(values));
}

void SubShapeBinder::handleChangedPropertyType(
    Base::XMLReader& reader, const char* TypeName, App::Property* prop)
{
    if (prop == &Support) {
        Support.upgrade(reader, TypeName);
    }
    else {
        inherited::handleChangedPropertyType(reader, TypeName, prop);
    }
}

////////////////////////////////////////////////////////////////////////////////////////

namespace App {
    PROPERTY_SOURCE_TEMPLATE(PartDesign::SubShapeBinderPython, PartDesign::SubShapeBinder)
        template<> const char* PartDesign::SubShapeBinderPython::getViewProviderName() const {
        return "PartDesignGui::ViewProviderSubShapeBinderPython";
    }
    template class PartDesignExport FeaturePythonT<PartDesign::SubShapeBinder>;
}

