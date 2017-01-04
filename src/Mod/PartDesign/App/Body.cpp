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
#include <boost/bind.hpp>
#endif

#include <Base/Console.h>
#include <Base/Placement.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Origin.h>

#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/PartFeature.h>


#include "Feature.h"
#include "FeatureSketchBased.h"
#include "FeatureTransformed.h"
#include "DatumPoint.h"
#include "DatumLine.h"
#include "DatumPlane.h"
#include "ShapeBinder.h"

#include "Body.h"
#include "BodyPy.h"

using namespace PartDesign;


PROPERTY_SOURCE(PartDesign::Body, Part::BodyBase)

Body::Body() {
    ADD_PROPERTY_TYPE (Origin, (0), 0, App::Prop_Hidden, "Origin linked to the body" );
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
    if ( Tip.isTouched() ||
         BaseFeature.isTouched() ) {
        return 1;
    }
    return Part::BodyBase::mustExecute();
}

App::DocumentObject* Body::getPrevFeature(App::DocumentObject *start) const
{
    std::vector<App::DocumentObject*> features = Group.getValues();
    if (features.empty()) return NULL;
    App::DocumentObject* st = (start == NULL ? Tip.getValue() : start);
    if (st == NULL)
        return st; // Tip is NULL

    std::vector<App::DocumentObject*>::iterator it = std::find(features.begin(), features.end(), st);
    if (it == features.end()) return NULL; // Invalid start object

    it--;
    return *it;
}

App::DocumentObject* Body::getPrevSolidFeature(App::DocumentObject *start)
{
    App::DocumentObject* baseFeature = BaseFeature.getValue();

    if ( !start ) { // default to tip
        start = Tip.getValue();
    }

    if ( !start ) { // No Tip
        return nullptr;
    } else if ( start == baseFeature ) { // The base feature always considered as the first solid
        return nullptr;
    }

    assert ( hasObject ( start ) );

    const std::vector<App::DocumentObject*> & features = Group.getValues();

    auto startIt = std::find ( features.rbegin(), features.rend(), start );
    assert ( startIt != features.rend() );
    auto rvIt = std::find_if ( startIt + 1, features.rend(), isSolidFeature );

    if (rvIt != features.rend()) { // the solid found in model list
        return *rvIt;
    } else { // if solid is not present in the list the previous one is baseFeature
        return baseFeature; // return it either it's set or nullptr
    }
}

App::DocumentObject* Body::getNextSolidFeature(App::DocumentObject *start)
{
    App::DocumentObject* baseFeature = BaseFeature.getValue();

    if ( !start ) { // default to tip
        start = Tip.getValue();
    }

    if ( !start ) { // no tip
        return nullptr;
    }

    assert ( hasObject ( start ) );

    const std::vector<App::DocumentObject*> & features = Group.getValues();
    std::vector<App::DocumentObject*>::const_iterator startIt;

    if ( start == baseFeature ) {
        // Handle the base feature, it's always considered to be solid
        startIt = features.begin();
    } else {
        startIt = std::find ( features.begin(), features.end(), start );
        assert ( startIt != features.end() );
        startIt++;
    }

    if (startIt == features.end() ) { // features list is empty
        return nullptr;
    }

    auto rvIt = std::find_if ( startIt, features.end(), isSolidFeature );

    if (rvIt != features.end()) { // the solid found in model list
        return *rvIt;
    } else {
        return nullptr;
    }
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
    if (f == NULL)
        return false;

    // This can be recognized because the Originals property is empty (it is contained
    // in the MultiTransform instead)
    return (f->getTypeId().isDerivedFrom(PartDesign::Transformed::getClassTypeId()) &&
            static_cast<const PartDesign::Transformed*>(f)->Originals.getValues().empty());
}

bool Body::isSolidFeature(const App::DocumentObject* f)
{
    if (f == NULL)
        return false;

    if (f->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId()) &&
        !PartDesign::Feature::isDatum(f)) {
        // Transformed Features inside a MultiTransform are not solid features
        return !isMemberOfMultiTransform(f);
    }
    return false;//DeepSOIC: work-in-progress?
}

bool Body::isAllowed(const App::DocumentObject* f)
{
    if (f == NULL)
        return false;

    // TODO: Should we introduce a PartDesign::FeaturePython class? This should then also return true for isSolidFeature()
    return (f->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId()) ||
            f->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())   ||
            // TODO Shouldn't we replace it with Sketcher::SketchObject? (2015-08-13, Fat-Zer)
            f->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId()) ||
            f->getTypeId().isDerivedFrom(PartDesign::ShapeBinder::getClassTypeId())
            // TODO Why this lines was here? why should we allow anything of those? (2015-08-13, Fat-Zer)
            //f->getTypeId().isDerivedFrom(Part::FeaturePython::getClassTypeId()) // trouble with this line on Windows!? Linker fails to find getClassTypeId() of the Part::FeaturePython...
            //f->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())
            );
}


Body* Body::findBodyOf(const App::DocumentObject* feature)
{
    if(!feature)
        return nullptr;
    
    return static_cast<Body*>(BodyBase::findBodyOf(feature));
}


void Body::addObject(App::DocumentObject *feature)
{
    if(!isAllowed(feature))
        throw Base::Exception("Body: object is not allowed");
    
    insertObject (feature, getNextSolidFeature (), /*after = */ false);
    // Move the Tip if we added a solid
    if (isSolidFeature(feature)) {
        Tip.setValue (feature);
    }
}


void Body::insertObject(App::DocumentObject* feature, App::DocumentObject* target, bool after)
{
    if (target) {
        if (target == BaseFeature.getValue()) {
            // Handle the insertion relative to the base feature in a special way
            if (after) {
                target = nullptr;
            } else {
                throw Base::Exception("Body: impossible to insert before the base object");
            }
        } else if (!hasObject (target)) {
            // Check if the target feature belongs to the body
            throw Base::Exception("Body: the feature we should insert relative to is not part of that body");
        }
    }

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

    Group.setValues (model);

    // Set the BaseFeature property
    if (Body::isSolidFeature(feature)) {
        // Set BaseFeature property to previous feature (this might be the Tip feature)
        App::DocumentObject* prevSolidFeature = getPrevSolidFeature(feature);
        // NULL is ok here, it just means we made the current one fiature the base solid
        static_cast<PartDesign::Feature*>(feature)->BaseFeature.setValue(prevSolidFeature);

        // Reroute the next solid feature's BaseFeature property to this feature
        App::DocumentObject* nextSolidFeature = getNextSolidFeature(feature);
        if (nextSolidFeature) {
            assert ( nextSolidFeature->isDerivedFrom ( PartDesign::Feature::getClassTypeId () ) );
            static_cast<PartDesign::Feature*>(nextSolidFeature)->BaseFeature.setValue(feature);
        }
    }

}


void Body::removeObject(App::DocumentObject* feature)
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
    model.erase(it);
    Group.setValues(model);
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
        if ( !tip->getTypeId().isDerivedFrom ( PartDesign::Feature::getClassTypeId() )
                && tip != BaseFeature.getValue () ) {
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

    if(connection.connected())
        connection.disconnect();

    Part::BodyBase::onSettingDocument();
}

void Body::onChanged (const App::Property* prop) {
    if ( prop == &BaseFeature ) {
        App::DocumentObject *baseFeature = BaseFeature.getValue();
        App::DocumentObject *nextSolid = getNextSolidFeature ( baseFeature );
        if ( nextSolid ) {
            assert ( nextSolid->isDerivedFrom ( PartDesign::Feature::getClassTypeId () ) );
            static_cast<PartDesign::Feature*>(nextSolid)->BaseFeature.setValue( baseFeature );
        }
    }

    Part::BodyBase::onChanged ( prop );
}

void Body::setupObject () {
    Part::BodyBase::setupObject ();
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
