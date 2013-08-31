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

#include <App/Plane.h>
#include <Base/Placement.h>

#include "Feature.h"
#include "Body.h"
#include "BodyPy.h"
#include "FeatureSketchBased.h"
#include "FeatureTransformed.h"
#include "DatumPoint.h"
#include "DatumLine.h"
#include "DatumPlane.h"

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Mod/Part/App/DatumFeature.h>


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Body, Part::BodyBase)

Body::Body()
{
    ADD_PROPERTY(IsActive,(0));
}

/*
// Note: The following code will catch Python Document::removeObject() modifications. If the object removed is
// a member of the Body::Model, then it will be automatically removed from the Model property which triggers the
// following two methods
// But since we require the Python user to call both Document::addObject() and Body::addFeature(), we should
// also require calling both Document::removeObject and Body::removeFeature() in order to be consistent
void Body::onBeforeChange(const App::Property *prop)
{
    // Remember the feature before the current Tip. If the Tip is already at the first feature, remember the next feature
    if (prop == &Model) {
        std::vector<App::DocumentObject*> features = Model.getValues();
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
    if (prop == &Model) {
        std::vector<App::DocumentObject*> features = Model.getValues();
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

    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        //return new App::DocumentObjectExecReturn("Linked object is not a PartDesign object");
        return Part::TopoShape();
    // get the shape of the tip
    return static_cast<Part::Feature*>(link)->Shape.getShape();
}

App::DocumentObject* Body::getPrevFeature(App::DocumentObject *start) const
{
    std::vector<App::DocumentObject*> features = Model.getValues();
    if (features.empty()) return NULL;
    App::DocumentObject* st = (start == NULL ? Tip.getValue() : start);
    if (st == NULL)
        return st; // Tip is NULL

    std::vector<App::DocumentObject*>::iterator it = std::find(features.begin(), features.end(), st);
    if (it == features.end()) return NULL; // Invalid start object

    it--;
    return *it;
}

App::DocumentObject* Body::getPrevSolidFeature(App::DocumentObject *start, const bool inclusive)
{
    std::vector<App::DocumentObject*> features = Model.getValues();
    if (features.empty()) return NULL;
    App::DocumentObject* st = (start == NULL ? Tip.getValue() : start);
    if (st == NULL)
        return st; // Tip is NULL

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

    std::vector<App::DocumentObject*>::iterator it;
    if (st == NULL)
        it = features.begin(); // Tip is NULL
    else
        it = std::find(features.begin(), features.end(), st);
    if (it == features.end()) return NULL; // Invalid start object

    // Skip sketches and datum features
    do {
        it++;
        if (it == features.end())
            return NULL;
    } while (!isSolidFeature(*it));

    return *it;
}

const bool Body::isMemberOfMultiTransform(const App::DocumentObject* f)
{
    if (f == NULL)
        return false;

    // This can be recognized because the Originals property is empty (it is contained
    // in the MultiTransform instead)
    return (f->getTypeId().isDerivedFrom(PartDesign::Transformed::getClassTypeId()) &&
            static_cast<const PartDesign::Transformed*>(f)->Originals.getValues().empty());
}

const bool Body::isSolidFeature(const App::DocumentObject* f)
{
    if (f == NULL)
        return false;

    if (f->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
        // Transformed Features inside a MultiTransform are not solid features
        return !isMemberOfMultiTransform(f);
    }
}

const bool Body::isAllowed(const App::DocumentObject* f)
{
    if (f == NULL)
        return false;

    // TODO: Should we introduce a PartDesign::FeaturePython class? This should then also return true for isSolidFeature()
    return (f->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId()) ||
            f->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())   ||
            f->getTypeId().isDerivedFrom(Part::Part2DObject::getClassTypeId()) ||
            f->getTypeId().isDerivedFrom(Part::FeaturePython::getClassTypeId()));
}

void Body::addFeature(App::DocumentObject *feature)
{
    // Set the BaseFeature property
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
            // Insert feature as before all other features in the body
            model.insert(model.begin(), feature);
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
    // This method must be called BEFORE the feature is removed from the Document!

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

    std::vector<App::DocumentObject*> model = Model.getValues();
    std::vector<App::DocumentObject*>::iterator it = std::find(model.begin(), model.end(), feature);

    // Adjust Tip feature if it is pointing to the deleted object
    App::DocumentObject* tipFeature = Tip.getValue();
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
            next++;
            if (next != model.end())
                Tip.setValue(*next);
            else
                Tip.setValue(NULL);
        }
    }

    // Erase feature from Model
    model.erase(it);
    Model.setValues(model);
}

App::DocumentObjectExecReturn *Body::execute(void)
{
    /*
    Base::Console().Error("Body '%s':\n", getNameInDocument());
    App::DocumentObject* tip = Tip.getValue();
    Base::Console().Error("   Tip: %s\n", (tip == NULL) ? "None" : tip->getNameInDocument());
    std::vector<App::DocumentObject*> model = Model.getValues();
    Base::Console().Error("   Model:\n");
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

    const Part::TopoShape& TipShape = getTipShape();

    if (TipShape._Shape.IsNull())
        //return new App::DocumentObjectExecReturn("empty shape");
        return App::DocumentObject::StdReturn;

    Shape.setValue(TipShape);
    
    return App::DocumentObject::StdReturn;
}

Base::BoundBox3d Body::getBoundBox()
{
    Base::BoundBox3d result;

    Part::Feature* tipSolid = static_cast<Part::Feature*>(getPrevSolidFeature());
    if (tipSolid != NULL) {
        if (tipSolid->Shape.getValue().IsNull())
            // This can happen when a new feature is added without having its Shape property set yet
            tipSolid = static_cast<Part::Feature*>(getPrevSolidFeature(NULL, false));

        if (tipSolid != NULL) {
            if (tipSolid->Shape.getValue().IsNull())
                tipSolid = NULL;
            else
                result = tipSolid->Shape.getShape().getBoundBox();
        }
    }
    if (tipSolid == NULL)
        result = App::Plane::getBoundBox();

    std::vector<App::DocumentObject*> model = Model.getValues();
    // TODO: In DatumLine and DatumPlane, recalculate the Base point to be as near as possible to the origin (0,0,0)
    for (std::vector<App::DocumentObject*>::const_iterator m = model.begin(); m != model.end(); m++) {
        if ((*m)->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())) {
            PartDesign::Point* point = static_cast<PartDesign::Point*>(*m);
            result.Add(point->getPoint());
        } else if ((*m)->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())) {
            PartDesign::Line* line = static_cast<PartDesign::Line*>(*m);
            result.Add(line->getBasePoint());
        } else if ((*m)->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())) {
            PartDesign::Plane* plane = static_cast<PartDesign::Plane*>(*m);
            result.Add(plane->getBasePoint());
        } else if ((*m)->getTypeId().isDerivedFrom(App::Plane::getClassTypeId())) {
            // Note: We only take into account the base planes here
            result.Add(Base::Vector3d(0,0,0));
        }
    }

    return result;
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
