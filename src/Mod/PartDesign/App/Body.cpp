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

#include <Base/Placement.h>

#include "Feature.h"
#include "DatumFeature.h"
#include "Body.h"
#include "BodyPy.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include "FeatureSketchBased.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Body, Part::BodyBase)

Body::Body()
{
    ADD_PROPERTY(IsActive,(0));
}

void Body::onChanged(const App::Property *prop)
{
    Base::Console().Error("Checking Body '%s' for sanity\n", getNameInDocument());
    App::DocumentObject* tip = Tip.getValue();
    Base::Console().Error("   Tip: %s\n", (tip == NULL) ? "None" : tip->getNameInDocument());
    std::vector<App::DocumentObject*> model = Model.getValues();
    Base::Console().Error("   Model:\n");
    for (std::vector<App::DocumentObject*>::const_iterator m = model.begin(); m != model.end(); m++) {
        Base::Console().Error("      %s", (*m)->getNameInDocument());
        if ((*m)->getTypeId().isDerivedFrom(PartDesign::SketchBased::getClassTypeId())) {
            App::DocumentObject* baseFeature = static_cast<PartDesign::SketchBased*>(*m)->BaseFeature.getValue();
            Base::Console().Error(", Base: %s\n", baseFeature == NULL ? "None" : baseFeature->getNameInDocument());
        } else {
            Base::Console().Error("\n");
        }
    }

    return Part::Feature::onChanged(prop);
}

short Body::mustExecute() const
{
    if (Tip.isTouched() )
        return 1;
    return 0;
}

const Part::TopoShape Body::getTipShape()
{
    // TODO right selection for Body
    App::DocumentObject* link = Tip.getValue();
    if (!link)
        return Part::TopoShape();
    //Base::Console().Error("Body tip: %s\n", link->getNameInDocument());
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        //return new App::DocumentObjectExecReturn("Linked object is not a PartDesign object");
        return Part::TopoShape();
    // get the shape of the tip
    return static_cast<Part::Feature*>(link)->Shape.getShape();
}
/*
const Part::TopoShape Body::getPreviousSolid(const PartDesign::Feature* f)
{
    std::vector<App::DocumentObject*> features = Model.getValues();
    std::vector<App::DocumentObject*>::const_iterator it = std::find(features.begin(), features.end(), f);
    if ((it == features.end()) || (it == features.begin()))
        // Wrong body or there is no previous feature
        return Part::TopoShape();
    // move to previous feature
    it--;
    // Skip sketches and datum features
    while ((*it)->getTypeId().isDerivedFrom(PartDesign::Datum::getClassTypeId()) ||
           (*it)->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId())) {
        if (it == features.begin())
            return Part::TopoShape();
        it--;
    }

    return static_cast<const PartDesign::Feature*>(*it)->Shape.getShape();
}
*/
App::DocumentObject* Body::getPrevSolidFeature(App::DocumentObject *start, const bool inclusive)
{
    std::vector<App::DocumentObject*> features = Model.getValues();
    if (features.empty()) return NULL;
    App::DocumentObject* st = (start == NULL ? Tip.getValue() : start);

    if (inclusive && isSolidFeature(st))
        return st;

    std::vector<App::DocumentObject*>::iterator it = std::find(features.begin(), features.end(), st);
    if (it == features.end()) return NULL; // Invalid start object

    // Skip sketches and datum features
    do {
        if (it == features.begin())
            return NULL;
        it--;
    } while (!isSolidFeature(*it));

    return *it;
}

App::DocumentObject* Body::getNextSolidFeature(App::DocumentObject *start, const bool inclusive)
{
    std::vector<App::DocumentObject*> features = Model.getValues();
    if (features.empty()) return NULL;
    App::DocumentObject* st = (start == NULL ? Tip.getValue() : start);

    if (inclusive && isSolidFeature(st))
            return st;

    std::vector<App::DocumentObject*>::iterator it = std::find(features.begin(), features.end(), st);
    if (it == features.end()) return NULL; // Invalid start object

    // Skip sketches and datum features
    do {
        it++;
        if (it == features.end())
            return NULL;
    } while (!isSolidFeature(*it));

    return *it;
}

const bool Body::hasFeature(const App::DocumentObject* f)
{
    std::vector<App::DocumentObject*> features = Model.getValues();
    return std::find(features.begin(), features.end(), f) != features.end();
}

const bool Body::isAfterTip(const App::DocumentObject *f) {
    std::vector<App::DocumentObject*> features = Model.getValues();
    std::vector<App::DocumentObject*>::const_iterator it = std::find(features.begin(), features.end(), f);
    std::vector<App::DocumentObject*>::const_iterator tip = std::find(features.begin(), features.end(), Tip.getValue());
    return (it > tip);
}

const bool Body::isSolidFeature(const App::DocumentObject* f)
{
    return (!f->getTypeId().isDerivedFrom(PartDesign::Datum::getClassTypeId()) &&
            !f->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId()));
}

Body* Body::findBodyOf(const App::DocumentObject* f)
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (doc != NULL) {
        std::vector<App::DocumentObject*> bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());
        for (std::vector<App::DocumentObject*>::const_iterator b = bodies.begin(); b != bodies.end(); b++) {
            PartDesign::Body* body = static_cast<PartDesign::Body*>(*b);
            if (body->hasFeature(f))
                return body;
        }
    }

    return NULL;
}

void Body::insertFeature(App::DocumentObject *feature)
{
    // Set the BaseFeature property
    // Note: This is not strictly necessary for Datum features
    if (feature->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
        App::DocumentObject* prevSolidFeature = getPrevSolidFeature(NULL, true);
        if (prevSolidFeature != NULL)
            // Set BaseFeature property to previous feature (this might be the Tip feature)
            static_cast<PartDesign::Feature*>(feature)->BaseFeature.setValue(prevSolidFeature);
    }

    if (Body::isSolidFeature(feature)) {
        // Reroute the next solid feature's BaseFeature property to this feature
        App::DocumentObject* nextSolidFeature = getNextSolidFeature(NULL, false);
        if (nextSolidFeature != NULL)
            static_cast<PartDesign::Feature*>(nextSolidFeature)->BaseFeature.setValue(feature);
    }

    // Insert the new feature after the current Tip feature
    App::DocumentObject* tipFeature = Tip.getValue();
    std::vector<App::DocumentObject*> model = Model.getValues();
    if (tipFeature == NULL) {
        if (model.empty())
            // First feature in the body
            model.push_back(feature);
        else
            throw Base::Exception("Body has features, but Tip is not valid");
    } else {
        // Insert after Tip
        std::vector<App::DocumentObject*>::iterator it = std::find(model.begin(), model.end(), tipFeature);
        if (it == model.end())
            throw Base::Exception("Body: Tip is not contained in model");

        it++;
        if (it == model.end())
            model.push_back(feature);
        else
            model.insert(it, feature);
    }
    Model.setValues(model);

    // Move the Tip
    Tip.setValue(feature);
}

void Body::removeFeature(App::DocumentObject* feature)
{
    if (isSolidFeature(feature)) {
        // This is a solid feature
        // If the next feature is solid, reroute its BaseFeature property to the previous solid feature
        App::DocumentObject* nextSolidFeature = getNextSolidFeature(feature, false);
        if (nextSolidFeature != NULL) {
            App::DocumentObject* prevSolidFeature = getPrevSolidFeature(feature, false);
            PartDesign::Feature* nextFeature = static_cast<PartDesign::Feature*>(nextSolidFeature);
            if ((prevSolidFeature == NULL) && (nextFeature->BaseFeature.getValue() != NULL))
                // sanity check
                throw Base::Exception((std::string("Body: Base feature of ") + nextFeature->getNameInDocument() + " was removed!").c_str());
            nextFeature->BaseFeature.setValue(prevSolidFeature);
        }
    }

    // Adjust Tip feature if it is pointing to the deleted object
    App::DocumentObject* tipFeature = Tip.getValue();
    std::vector<App::DocumentObject*> model = Model.getValues();
    std::vector<App::DocumentObject*>::iterator it = std::find(model.begin(), model.end(), feature);
    if (tipFeature == feature) {
        // Set the Tip to the previous feature if possible, otherwise to the next feature
        std::vector<App::DocumentObject*>::const_iterator prev = it, next = it;
        if (it != model.begin()) {
            prev--;
            next++;

            if (prev != model.end()) {
                Tip.setValue(*prev);
            } else {
                if (next != model.end())
                    Tip.setValue(*next);
                else
                    Tip.setValue(NULL);
            }
        } else {
            Tip.setValue(NULL);
        }
    }

    // Erase feature from Model
    model.erase(it);
    Model.setValues(model);
}

App::DocumentObjectExecReturn *Body::execute(void)
{
    const Part::TopoShape& TipShape = getTipShape();

    if (TipShape._Shape.IsNull())
        //return new App::DocumentObjectExecReturn("empty shape");
        return App::DocumentObject::StdReturn;

    Shape.setValue(TipShape);

    
    return App::DocumentObject::StdReturn;
}

PyObject *Body::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new BodyPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

}
