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
#endif

#include <stack>

#include <Base/Console.h>
#include <Base/Placement.h>
#include <Base/Tools.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>

#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/PartFeature.h>

#include "Feature.h"
#include "FeatureExtrusion.h"
#include "FeatureSolid.h"
#include "FeatureSketchBased.h"
#include "FeatureTransformed.h"
#include "FeatureMultiTransform.h"
#include "DatumPoint.h"
#include "DatumLine.h"
#include "DatumPlane.h"
#include "AuxGroup.h"
#include "ShapeBinder.h"

#include "Body.h"
#include "FeatureBase.h"
#include "BodyPy.h"

FC_LOG_LEVEL_INIT("PartDesign", true, true);

using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Body, Part::BodyBase)

Body::Body() {
    ADD_PROPERTY_TYPE(SingleSolid,(true),"Base",(App::PropertyType)(App::Prop_None),
            "Enforce single solid on each feature");
    _GroupTouched.setStatus(App::Property::Output,true);
    BaseFeature.setScope(App::LinkScope::Global);
}

/*
// Note: The following code will catch Python Document::removeObject() modifications. If the object removed is
// a member of the Body::Group, then it will be automatically removed from the Group property which triggers the
// following two methods
// But since we require the Python user to call both Document::addObject() and Body::addObject(), we should
// also require calling both Document::removeObject and Body::removeFeature() in order to be consistent
void Body::onBeforeChange(const App::Property *prop)
{
    // Remember the feature before the current Tip. If the Tip is already at the first feature, remember the next feature
    if (prop == &Group) {
        std::vector<App::DocumentObject*> features = Group.getValues();
        if (features.empty()) {
            rememberTip = NULL;
        } else {
            std::vector<App::DocumentObject*>::iterator it = std::find(features.begin(), features.end(), Tip.getValue());
            if (it == features.begin()) {
                it++;
                if (it == features.end())
                    rememberTip = NULL;
                else
                    rememberTip = *it;
            } else {
                it--;
                rememberTip = *it;
            }
        }
    }

    return Part::Feature::onBeforeChange(prop);
}

void Body::onChanged(const App::Property *prop)
{
    if (prop == &Group) {
        std::vector<App::DocumentObject*> features = Group.getValues();
        if (features.empty()) {
            Tip.setValue(NULL);
        } else {
            std::vector<App::DocumentObject*>::iterator it = std::find(features.begin(), features.end(), Tip.getValue());
            if (it == features.end()) {
                // Tip feature was deleted
                Tip.setValue(rememberTip);
            }
        }
    }

    return Part::Feature::onChanged(prop);
}
*/

short Body::mustExecute() const
{
    if ( Tip.isTouched() ) {
        return 1;
    }
    return Part::BodyBase::mustExecute();
}

App::DocumentObject* Body::getPrevSolidFeature(App::DocumentObject *start)
{
    if (!start) { // default to tip
        start = Tip.getValue();
    }

    int index;
    if (!start || !start->getNameInDocument()
               || start->isDerivedFrom(PartDesign::Extrusion::getClassTypeId())
               || start->isDerivedFrom(PartDesign::Solid::getClassTypeId())
               || !this->Group.find(start->getNameInDocument(), &index)) { // No Tip
        return nullptr;
    }

    const auto & objs = this->Group.getValues();
    for (--index; index>=0; --index) {
        if (isSolidFeature(objs[index]))
            return objs[index];
    }

    if(BaseFeature.getValue() && BaseFeature.getValue()!=start)
        return BaseFeature.getValue();
    
    return nullptr;
}

App::DocumentObject* Body::getNextSolidFeature(App::DocumentObject *start)
{
    if (!start) { // default to tip
        start = Tip.getValue();
    }

    int index;
    if (!start || !start->getNameInDocument()
               || !this->Group.find(start->getNameInDocument(), &index)) { // No Tip
        return nullptr;
    }

    const auto & objs = this->Group.getValues();
    int count = this->Group.getSize();
    for (++index; index<count; ++index) {
        if (isSolidFeature(objs[index]))
            return objs[index];
    }

    return nullptr;
}

bool Body::isAfterInsertPoint(App::DocumentObject* feature) {
    App::DocumentObject *nextSolid = getNextSolidFeature ();
    assert (feature);

    if (feature == nextSolid) {
        return true;
    } else if (!nextSolid) { // the tip is last solid, we can't be plased after it
        return false;
    } else {
        return isAfter ( feature, nextSolid );
    }
}

bool Body::isMemberOfMultiTransform(const App::DocumentObject* f)
{
    if (!f || !f->isDerivedFrom(PartDesign::Transformed::getClassTypeId()))
        return false;

    for (auto obj : f->getInList()) {
        if (obj->isDerivedFrom(PartDesign::MultiTransform::getClassTypeId()))
            return true;
    }
    return false;
}

bool Body::isSolidFeature(const App::DocumentObject* f)
{
    if (f == NULL)
        return false;

    if (f->isDerivedFrom(PartDesign::Extrusion::getClassTypeId()))
        return static_cast<const PartDesign::Extrusion*>(f)->NewSolid.getValue();

    if (f->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId())
            && !PartDesign::Feature::isDatum(f)) {
        // Transformed Features inside a MultiTransform are not solid features
        return !isMemberOfMultiTransform(f);
    }
    return false;//DeepSOIC: work-in-progress?
}

bool Body::isAllowed(const App::DocumentObject* f)
{
    if (f == NULL)
        return false;
    return isAllowed(f->getTypeId());
}

bool Body::isAllowed(const Base::Type &type)
{
    // TODO: Should we introduce a PartDesign::FeaturePython class? This should then also return true for isSolidFeature()
    return (type.isDerivedFrom(PartDesign::Feature::getClassTypeId()) ||
            type.isDerivedFrom(Part::Datum::getClassTypeId())   ||
            type.isDerivedFrom(PartDesign::Solid::getClassTypeId())   ||
            // TODO Shouldn't we replace it with Sketcher::SketchObject? (2015-08-13, Fat-Zer)
            type.isDerivedFrom(Part::Part2DObject::getClassTypeId()) ||
            type.isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId()) ||
            type.isDerivedFrom(PartDesign::SubShapeBinder::getClassTypeId()) ||
            type.isDerivedFrom(PartDesign::AuxGroup::getClassTypeId())
            // TODO Why this lines was here? why should we allow anything of those? (2015-08-13, Fat-Zer)
            //type.isDerivedFrom(Part::FeaturePython::getClassTypeId()) // trouble with this line on Windows!? Linker fails to find getClassTypeId() of the Part::FeaturePython...
            //type.isDerivedFrom(Part::Feature::getClassTypeId())
            );
}


Body* Body::findBodyOf(const App::DocumentObject* feature)
{
    if(!feature)
        return nullptr;

    if (feature->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        return static_cast<const PartDesign::Feature*>(feature)->getFeatureBody();
    if (feature->isDerivedFrom(PartDesign::AuxGroup::getClassTypeId()))
        return static_cast<const PartDesign::AuxGroup*>(feature)->getBody();
    
    return Base::freecad_dynamic_cast<Body>(BodyBase::findBodyOf(feature));
}


std::vector<App::DocumentObject*> Body::addObject(App::DocumentObject *feature)
{
    if(!isAllowed(feature))
        throw Base::ValueError("Body: object is not allowed");
    
    //TODO: features should not add all links
    
    //only one group per object. If it is in a body the single feature will be removed
    auto *group = App::GroupExtension::getGroupOfObject(feature);
    if(group && group != getExtendedObject())
        group->getExtensionByType<GroupExtension>()->removeObject(feature);
      
    // It is not safe to insert after tip (which is what we get by calling
    // getNextSolidFeature(nullptr)). For example, padding from a sketch, but
    // the sketch is based on a feature after the tip.
    //
    // It is only absolutely safe to insert at the very end. We'll have to rely
    // up on PartGui commands to determine the new object insertion point based
    // on user selection at the time of command invoking.
    //
    // insertObject (feature, getNextSolidFeature (), [>after = <] false);
    //
    insertObject (feature, nullptr, false);
    setTip(feature);
    return {feature};
}

void Body::checkChild(const App::Property &prop)
{
    auto feature = Base::freecad_dynamic_cast<PartDesign::Feature>(prop.getContainer());
    if (!feature || !feature->Visibility.getValue() || !isSolidFeature(feature))
        return;
    for (auto obj : getSiblings(feature)) {
        if (obj != feature && obj->Visibility.getValue() && isSolidFeature(obj))
            obj->Visibility.setValue(false);
    }
}

void Body::checkChildren()
{
    ++childrenFlag;
    for (auto obj : Group.getValue()) {
        auto feature = Base::freecad_dynamic_cast<PartDesign::Feature>(obj);
        if (!feature)
            continue;

        auto &info = this->childrenConns[feature];
        info.flag = childrenFlag;

        if (info.visibilityConn.connected())
            continue;

        info.visibilityConn = feature->Visibility.signalChanged.connect(
                boost::bind(&Body::checkChild, this, _1));
        info.baseFeatureConn = feature->BaseFeature.signalChanged.connect(
                boost::bind(&Body::checkChild, this, _1));

        checkChild(feature->Visibility);
    }

    for (auto it=childrenConns.begin(); it!=childrenConns.end();) {
        if (it->second.flag != childrenFlag)
            it = childrenConns.erase(it);
        else
            ++it;
    }
}

std::vector< App::DocumentObject* > Body::addObjects(std::vector< App::DocumentObject* > objs) {
    
    for(auto obj : objs)
        addObject(obj);
    
    return objs;
}

App::DocumentObject *
Body::getInsertionPosition(const std::vector<App::DocumentObject*> &objs)
{
    App::DocumentObject * tip = this->Tip.getValue();
    if (objs.empty() || !tip)
        return getNextSolidFeature();

    // Use the tip to determine which sibling group to insert the new feature
    const auto siblings = getSiblings(tip);

    int index = -1;
    for (auto obj : App::Document::getDependencyList(objs)) {
        int idx;
        if (!obj || !Group.find(obj->getNameInDocument(), &idx) || index>=idx)
            continue;
        // Only consider dependency within the same sibling group, and use the
        // last feature for insertion point.
        if (std::find(siblings.begin(), siblings.end(), obj) != siblings.end())
            index = idx;
    }

    if (index < 0)
        return getNextSolidFeature();
    return getNextSolidFeature(Group.getValues()[index]);
}

App::DocumentObject *
Body::newObjectAt(const char *type,
                  const char *name,
                  const std::vector<App::DocumentObject *> &deps)
{
    if (!isAllowed(Base::Type::fromName(type)))
        FC_THROWM(Base::TypeError, "Type '" << (type?type:"?")  << "' is not allowed in Body");

    App::DocumentObject *obj = getDocument()->addObject(type, name);
    if (!obj)
        FC_THROWM(Base::RuntimeError, "Failed to create object");
    insertObject(obj, getInsertionPosition(deps));

    auto tip = Tip.getValue();
    if (!tip || !isSibling(obj, tip))
        setTip(obj);
    else {
        int i=-1, j=-1;
        Group.find(tip->getNameInDocument(), &i);
        Group.find(obj->getNameInDocument(), &j);
        if (i < j)
            setTip(obj);
    }
    obj->Visibility.setValue(true);
    return obj;
}

void Body::insertObject(App::DocumentObject* feature, App::DocumentObject* target, bool after)
{
    if (target && !Group.find(target->getNameInDocument())) {
        throw Base::ValueError("Body: the feature we should insert relative to is not part of that body");
    }
    
    //ensure that all origin links are ok
    relinkToOrigin(feature);

    std::vector<App::DocumentObject*> model = Group.getValues();
    std::vector<App::DocumentObject*>::iterator insertInto;

    // Find out the position there to insert the feature
    if (!target) {
        if (after) {
            insertInto = model.begin();
        } else {
            insertInto = model.end();
        }
    } else {
        std::vector<App::DocumentObject*>::iterator targetIt = std::find (model.begin(), model.end(), target);
        assert (targetIt != model.end());
        if (after) {
            insertInto = targetIt + 1;
        } else {
            insertInto = targetIt;
        }
    }

    // Insert the new feature after the given
    model.insert (insertInto, feature);

    {
        // User3 is to skip GeoFeatureGroupExtension group check
        Base::ObjectStatusLocker<App::Property::Status, App::Property>
            guard(App::Property::User3, &Group);
        Group.setValues (model);
    }

    if(feature->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        static_cast<PartDesign::Feature*>(feature)->_Body.setValue(this);

    // Set the BaseFeature property
    setBaseProperty(feature);
}

void Body::setTip(App::DocumentObject *feature)
{
    if (!feature || Tip.getValue() == feature
                 || !isSolidFeature(feature)
                 || !Group.find(feature->getNameInDocument()))
        return;

    Tip.setValue(feature);
}

void Body::setBaseProperty(App::DocumentObject* feature)
{
    int index;
    if (!feature || !this->Group.find(feature->getNameInDocument(), &index))
        throw Base::RuntimeError("Feature not found in body");

    if (feature->isDerivedFrom(PartDesign::Feature::getClassTypeId())
        && !static_cast<PartDesign::Feature*>(feature)->NewSolid.getValue()
        && Body::isSolidFeature(feature))
    {
        // Set BaseFeature property to previous feature (this might be the Tip feature)
        App::DocumentObject* prevSolidFeature = getPrevSolidFeature(feature);

        // NULL is ok here, it just means we made the current one fiature the base solid
        static_cast<PartDesign::Feature*>(feature)->BaseFeature.setValue(prevSolidFeature);

        // Set the next feature's base to this feature
        for (int i=index+1, count=Group.getSize(); i<count; ++i) {
            auto obj = Group.getValues()[i];
            if (!obj || !obj->isDerivedFrom(PartDesign::Feature::getClassTypeId())
                     || !isSolidFeature(obj))
                continue;
            auto feat = static_cast<PartDesign::Feature*>(obj);
            if (!feat->NewSolid.getValue()
                    && feat->BaseFeature.getValue() == prevSolidFeature)
            {
                feat->BaseFeature.setValue(feature);
                break;
            }
        }
    }
}

std::vector<App::DocumentObject*> Body::removeObject(App::DocumentObject* feature)
{
    App::DocumentObject* nextSolidFeature = getNextSolidFeature(feature);
    App::DocumentObject* prevSolidFeature = getPrevSolidFeature(feature);
    // This method must be called BEFORE the feature is removed from the Document!
    if (isSolidFeature(feature)) {
        // This is a solid feature
        // If the next feature is solid, reroute its BaseFeature property to the previous solid feature
        if (nextSolidFeature) {
            assert ( nextSolidFeature->isDerivedFrom ( PartDesign::Feature::getClassTypeId () ) );
            // Note: It's ok to remove the first solid feature, that just mean the next feature become the base one
            static_cast<PartDesign::Feature*>(nextSolidFeature)->BaseFeature.setValue(prevSolidFeature);
        }
    }

    std::vector<App::DocumentObject*> model = Group.getValues();
    std::vector<App::DocumentObject*>::iterator it = std::find(model.begin(), model.end(), feature);

    // Adjust Tip feature if it is pointing to the deleted object
    if (Tip.getValue()== feature) {
        if (prevSolidFeature) {
            Tip.setValue(prevSolidFeature);
        } else {
            Tip.setValue(nextSolidFeature);
        }
    }

    // Erase feature from Group
    if (it != model.end()) {
        model.erase(it);
        // User3 is to skip GeoFeatureGroupExtension group check
        Base::ObjectStatusLocker<App::Property::Status, App::Property>
            guard(App::Property::User3, &Group);
        Group.setValues(model);
    }
    std::vector<App::DocumentObject*> result = {feature};
    return result;
}


App::DocumentObjectExecReturn *Body::execute(void)
{
    /*
    Base::Console().Error("Body '%s':\n", getNameInDocument());
    App::DocumentObject* tip = Tip.getValue();
    Base::Console().Error("   Tip: %s\n", (tip == NULL) ? "None" : tip->getNameInDocument());
    std::vector<App::DocumentObject*> model = Group.getValues();
    Base::Console().Error("   Group:\n");
    for (std::vector<App::DocumentObject*>::const_iterator m = model.begin(); m != model.end(); m++) {
        if (*m == NULL) continue;
        Base::Console().Error("      %s", (*m)->getNameInDocument());
        if (Body::isSolidFeature(*m)) {
            App::DocumentObject* baseFeature = static_cast<PartDesign::Feature*>(*m)->BaseFeature.getValue();
            Base::Console().Error(", Base: %s\n", baseFeature == NULL ? "None" : baseFeature->getNameInDocument());
        } else {
            Base::Console().Error("\n");
        }
    }
    */

    App::DocumentObject* tip = Tip.getValue();

    Part::TopoShape tipShape;
    if ( tip ) {
        if ( !tip->getTypeId().isDerivedFrom ( PartDesign::Feature::getClassTypeId() ) ) {
            return new App::DocumentObjectExecReturn ( "Linked object is not a PartDesign feature" );
        }

        // get the shape of the tip
        tipShape = static_cast<Part::Feature *>(tip)->Shape.getShape();

        if ( tipShape.getShape().IsNull () ) {
            return new App::DocumentObjectExecReturn ( "Tip shape is empty" );
        }

        // We should hide here the transformation of the baseFeature
        tipShape.transformShape (tipShape.getTransform(), true );

    } else {
        tipShape = Part::TopoShape();
    }

    Shape.setValue ( tipShape );
    return App::DocumentObject::StdReturn;

}

void Body::onSettingDocument() {

    Part::BodyBase::onSettingDocument();
}

void Body::onChanged (const App::Property* prop) {
    // we neither load a project nor perform undo/redo
    if (!this->isRestoring() 
            && this->getDocument()
            && !this->getDocument()->isPerformingTransaction()) {
        if (prop == &BaseFeature) {
            FeatureBase* bf = nullptr;
            auto first = Group.getValues().empty() ? nullptr : Group.getValues().front();

            if (BaseFeature.getValue()
                    && !Group.find(BaseFeature.getValue()->getNameInDocument())) {
                //setup the FeatureBase if needed
                if (!first || (!first->isDerivedFrom(FeatureBase::getClassTypeId())
                                && !first->isDerivedFrom(SubShapeBinder::getClassTypeId())))
                {
                    bf = static_cast<FeatureBase*>(getDocument()->addObject("PartDesign::FeatureBase", "BaseFeature"));
                    insertObject(bf, first, false);

                    if (!Tip.getValue())
                        Tip.setValue(bf);
                }
                else {
                    bf = Base::freecad_dynamic_cast<FeatureBase>(first);
                }
            }

            if (bf && (bf->BaseFeature.getValue() != BaseFeature.getValue()))
                bf->BaseFeature.setValue(BaseFeature.getValue());
        }
        else if( prop == &Group ) {
            checkChildren();

            //if the FeatureBase was deleted we set the BaseFeature link to nullptr
            if (BaseFeature.getValue() &&
               (Group.getValues().empty() || 
                (!Group.getValues().front()->isDerivedFrom(FeatureBase::getClassTypeId())
                 && Group.getValues().front()!=BaseFeature.getValue())))
            {
                BaseFeature.setValue(nullptr);
            }
        }
        else if( prop == &SingleSolid ) {
            for(auto obj : Group.getValues()) {
                if(obj->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
                    obj->touch();
            }
        }
        else if (prop == &Tip) {
            FC_TRACE("tip changed");
        }
    }

    Part::BodyBase::onChanged(prop);
}

void Body::setupObject () {
    Part::BodyBase::setupObject ();
    auto hGrp = App::GetApplication().GetParameterGroupByPath (
                "User parameter:BaseApp/Preferences/Mod/PartDesign");
    SingleSolid.setValue(hGrp->GetBool("SingleSolid",false));

    // make sure the origins are created
    getOrigin()->getX();
}

void Body::unsetupObject () {
    Part::BodyBase::unsetupObject ();
}

PyObject *Body::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new BodyPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

std::vector<std::string> Body::getSubObjects(int reason) const {
    if(reason==GS_SELECT && !showTip)
        return Part::BodyBase::getSubObjects(reason);
    return {};
}

App::DocumentObject *Body::getSubObject(const char *subname, 
        PyObject **pyObj, Base::Matrix4D *pmat, bool transform, int depth) const
{
    // PartDesign::Feature now support grouping sibling features, and the user
    // is free to expand/collapse at any time. To not disrupt subname path
    // because of this, the body will peek the next two sub-objects reference,
    // and skip the first sub-object if possible.
    if(subname) {
        const char * firstDot = strchr(subname,'.');
        if (firstDot) {
            const char * secondDot = strchr(firstDot+1, '.');
            if (secondDot) {
                auto firstObj = Group.find(std::string(subname, firstDot).c_str());
                if (!firstObj || firstObj->isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
                    auto secondObj = Group.find(std::string(firstDot+1, secondDot).c_str());
                    if (secondObj) {
                        // we support only one level of sibling grouping, so no
                        // recursive call to our own getSubObject()
                        return Part::BodyBase::getSubObject(firstDot+1,pyObj,pmat,transform,depth+1);
                    }
                }
            }
        }
    }
#if 1
    return Part::BodyBase::getSubObject(subname,pyObj,pmat,transform,depth);
#else
    // The following code returns Body shape only if there is at least one
    // child visible in the body (when show through, not show tip). The
    // original intention is to sync visual to shape returned by
    // Part.getShape() when the body is included in some other group. But this
    // interfere with direct modeling using body shape. Therefore it is
    // disabled here.

    if(!pyObj || showTip ||
       (subname && !Data::ComplexGeoData::isMappedElement(subname) && strchr(subname,'.')))
        return Part::BodyBase::getSubObject(subname,pyObj,pmat,transform,depth);

    // We return the shape only if there are feature visible inside
    for(auto obj : Group.getValues()) {
        if(obj->Visibility.getValue() && 
           obj->isDerivedFrom(PartDesign::Feature::getClassTypeId())) 
        {
            return Part::BodyBase::getSubObject(subname,pyObj,pmat,transform,depth);
        }
    }
    if(pmat && transform)
        *pmat *= Placement.getValue().toMatrix();
    return const_cast<Body*>(this);
#endif
}

void Body::onDocumentRestored()
{
    ++childrenFlag;
    for(auto obj : Group.getValues()) {
        auto feature = Base::freecad_dynamic_cast<PartDesign::Feature>(obj);
        if (!feature)
            continue;

        feature->_Body.setValue(this);

        auto &info = this->childrenConns[feature];
        info.visibilityConn = feature->Visibility.signalChanged.connect(
                boost::bind(&Body::checkChild, this, _1));
        info.baseFeatureConn = feature->BaseFeature.signalChanged.connect(
                boost::bind(&Body::checkChild, this, _1));
        info.flag = childrenFlag;
    }
    _GroupTouched.setStatus(App::Property::Output,true);
    DocumentObject::onDocumentRestored();
}

Body::Relation
Body::getRelation(const App::DocumentObject *obj, const App::DocumentObject *other) const
{
    if (!obj || !obj->getNameInDocument() || !other || !other->getNameInDocument())
        return RelationStranger;

    if (obj == other)
        return RelationSibling;

    int index;
    if (!Group.find(obj->getNameInDocument(), &index))
        return RelationStranger;

    int otherIndex;
    if (!Group.find(other->getNameInDocument(), &otherIndex))
        return RelationStranger;
    
    if (index < otherIndex) {
        std::swap(obj, other);
        std::swap(index, otherIndex);
    }

    if (isSolidFeature(obj)) {
        auto feature = Base::freecad_dynamic_cast<const PartDesign::Feature>(obj);
        while (feature) {
            auto base = feature->BaseFeature.getValue();
            if (base == other)
                return RelationSibling;
            if (!base)
                break;
            if (!isSolidFeature(base))
                break;
            feature = Base::freecad_dynamic_cast<PartDesign::Feature>(base);
        }
    }
    return RelationCousin;
}

std::deque<App::DocumentObject*>
Body::getSiblings(App::DocumentObject *obj, bool all, bool reversed) const
{
    std::deque<App::DocumentObject *> res;
    if (!obj || !obj->getNameInDocument()
             || !obj->isDerivedFrom(PartDesign::Feature::getClassTypeId()))
        return res;

    int index = -1;
    if (!Group.find(obj->getNameInDocument(), &index))
        return res;

    if (isSolidFeature(obj)) {
        auto feature = static_cast<PartDesign::Feature*>(obj);
        while (feature) {
            auto base = feature->BaseFeature.getValue();
            if (!base)
                break;
            if (reversed)
                res.push_back(base);
            else
                res.push_front(base);
            if (res.size() >= Group.getValues().size())
                break;
            if (!isSolidFeature(base))
                break;
            feature = Base::freecad_dynamic_cast<PartDesign::Feature>(base);
        }
        if (!all)
            return res;
        if (reversed)
            res.push_front(obj);
        else
            res.push_back(obj);
        if (res.size() >= Group.getValues().size()) {
            // This means cyclic dependency actually
            return res;
        }
    }

    const auto & objs = Group.getValues();
    for (int count=Group.getSize(); index<count; ++index) {
        App::DocumentObject * o = objs[index];
        if(!o || !o->getNameInDocument()
                || !o->isDerivedFrom(PartDesign::Feature::getClassTypeId())
                || !isSolidFeature(o))
            continue;

        if (static_cast<PartDesign::Feature*>(o)->BaseFeature.getValue() == obj) {
            obj = o;
            if (reversed)
                res.push_front(o);
            else
                res.push_back(o);
        }
    }
    return res;
}

int Body::isElementVisible(const char *element) const
{
    auto obj = Group.find(element);
    if (obj && obj->isDerivedFrom(AuxGroup::getClassTypeId())) {
        auto group = static_cast<AuxGroup*>(obj);
        for (auto child : group->Group.getValues()) {
            if (child->Visibility.getValue())
                return 1;
        }
        return 0;
    }
    return -1;
}

int Body::setElementVisible(const char *element, bool visible)
{
    auto obj = Group.find(element);
    if (obj && obj->isDerivedFrom(AuxGroup::getClassTypeId())) {
        auto group = static_cast<AuxGroup*>(obj);
        for (auto child : group->Group.getValues())
            child->Visibility.setValue(visible);
        return 1;
    }
    return -1;
}
