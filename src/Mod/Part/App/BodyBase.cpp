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


#include "BodyBase.h"
#include "BodyBasePy.h"


namespace Part {


PROPERTY_SOURCE(Part::BodyBase, Part::Feature)

BodyBase::BodyBase()
{
    ADD_PROPERTY_TYPE (Origin, (0), 0, App::Prop_Hidden, "Origin linked to the body" );
    ADD_PROPERTY(Model       , (0) );
    ADD_PROPERTY(Tip         , (0) );
}

bool BodyBase::hasFeature(const App::DocumentObject* f) const
{
    const std::vector<App::DocumentObject*> &features = this->getFullModel();
    return std::find(features.begin(), features.end(), f) != features.end();
}

BodyBase* BodyBase::findBodyOf(const App::DocumentObject* f)
{
    App::Document* doc = f->getDocument();
    if (doc != NULL) {
        std::vector<App::DocumentObject*> bodies = doc->getObjectsOfType(BodyBase::getClassTypeId());
        for (std::vector<App::DocumentObject*>::const_iterator b = bodies.begin(); b != bodies.end(); b++) {
            BodyBase* body = static_cast<BodyBase*>(*b);
            if (body->hasFeature(f))
                return body;
        }
    }

    return NULL;
}

App::DocumentObjectExecReturn* BodyBase::execute(void)
{
    App::DocumentObject* tip = Tip.getValue();

    Part::TopoShape tipShape;
    if ( tip ) {
        if (   !tip->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())   ) {
            return new App::DocumentObjectExecReturn("Tip object is not a Part feature");
        }

        // get the shape of the tip
        tipShape = static_cast<Part::Feature *>(tip)->Shape.getShape();

        if ( tipShape._Shape.IsNull () ) {
            return new App::DocumentObjectExecReturn ( "Tip shape is empty" );
        }

        // We should hide here the transformation of the Tip Feature
        tipShape.transformShape (tipShape.getTransform(), true );

    } else {
        tipShape = Part::TopoShape();
    }

    Shape.setValue ( tipShape );

    return App::DocumentObject::StdReturn;
}

void BodyBase::removeModelFromDocument() {
    //delete all child objects if needed
    std::set<DocumentObject*> grp ( Model.getValues().begin (), Model.getValues().end() );
    for (auto obj : grp) {
        this->getDocument()->remObject(obj->getNameInDocument());
    }
}

App::Origin* BodyBase::getOrigin () const {
    App::DocumentObject *originObj = Origin.getValue ();

    if ( !originObj ) {
        std::stringstream err;
        err << "Can't find Origin for \"" << getNameInDocument () << "\"";
        throw Base::Exception ( err.str().c_str () );

    } else if (! originObj->isDerivedFrom ( App::Origin::getClassTypeId() ) ) {
        std::stringstream err;
        err << "Bad object \"" << originObj->getNameInDocument () << "\"(" << originObj->getTypeId().getName()
            << ") linked to the Origin of \"" << getNameInDocument () << "\"";
        throw Base::Exception ( err.str().c_str () );
    } else {
            return static_cast<App::Origin *> ( originObj );
    }
}


void BodyBase::setupObject () {
    // NOTE: the code shared with App::OriginGroup
    App::Document *doc = getDocument ();

    std::string objName = std::string ( getNameInDocument() ).append ( "Origin" );

    App::DocumentObject *originObj = doc->addObject ( "App::Origin", objName.c_str () );

    assert ( originObj && originObj->isDerivedFrom ( App::Origin::getClassTypeId () ) );
    Origin.setValue ( originObj );

}

void BodyBase::unsetupObject () {
    App::DocumentObject *origin = Origin.getValue ();

    if (origin && !origin->isDeleting ()) {
        origin->getDocument ()->remObject (origin->getNameInDocument());
    }

}


void BodyBase::addFeature(App::DocumentObject *feature)
{
    if (hasFeature(feature))
        throw Base::Exception("BodyBase: feature being added to the body is already in this body.");

    std::vector<App::DocumentObject*> model = Model.getValues();
    model.push_back(feature);
    Model.setValues(model);
}


void BodyBase::removeFeature(App::DocumentObject* feature)
{
    std::vector<App::DocumentObject*> model = Model.getValues();
    std::vector<App::DocumentObject*>::iterator it = std::find(model.begin(), model.end(), feature);
    if (it == model.end())
        throw Base::Exception("BodyBase: feature being removed doesn't belong to this body.");
    model.erase(it);
    Model.setValues(model);
}

short BodyBase::mustExecute() const
{
    if ( Tip.isTouched()) {
        return 1;
    }
    return Part::Feature::mustExecute();
}

PyObject *BodyBase::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new BodyBasePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

} /* Part */
