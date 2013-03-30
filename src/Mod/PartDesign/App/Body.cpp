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
#include "Body.h"
#include "BodyPy.h"

#include <Base/Console.h>


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Body, Part::BodyBase)

Body::Body()
{

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

const Part::TopoShape Body::getPreviousSolid(const PartDesign::Feature* f)
{
    std::vector<App::DocumentObject*> features = Model.getValues();
    std::vector<App::DocumentObject*>::const_iterator it = std::find(features.begin(), features.end(), f);
    if ((it == features.end()) || (it == features.begin()))
        // Wrong body or there is no previous feature
        return Part::TopoShape();
    // move to previous feature
    it--;
    // Skip sketches
    while (!(*it)->getTypeId().isDerivedFrom(PartDesign::Feature::getClassTypeId())) {
        if (it == features.begin())
            return Part::TopoShape();
        it--;
    }

    return static_cast<const PartDesign::Feature*>(*it)->Shape.getShape();
}

const bool Body::hasFeature(const App::DocumentObject* f)
{
    std::vector<App::DocumentObject*> features = Model.getValues();
    return std::find(features.begin(), features.end(), f) != features.end();
}

App::DocumentObjectExecReturn *Body::execute(void)
{
    std::vector<App::DocumentObject*> children = Model.getValues();
    //Base::Console().Error("Body exec children:\n");
    //for (std::vector<App::DocumentObject*>::const_iterator o = children.begin(); o != children.end(); o++)
    //    Base::Console().Error("%s\n", (*o)->getNameInDocument());

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
