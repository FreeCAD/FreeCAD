/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <App/DocumentObjectPy.h>
#include <Base/Placement.h>

#include "TrajectoryObject.h"


using namespace Robot;
using namespace App;

PROPERTY_SOURCE(Robot::TrajectoryObject, App::GeoFeature)


TrajectoryObject::TrajectoryObject()
{

    ADD_PROPERTY_TYPE(Base,
                      (Base::Placement()),
                      "Trajectory",
                      Prop_None,
                      "Base frame of the trajectory");
    ADD_PROPERTY_TYPE(Trajectory,
                      (Robot::Trajectory()),
                      "Trajectory",
                      Prop_None,
                      "Trajectory object");
}

short TrajectoryObject::mustExecute() const
{
    return 0;
}

PyObject* TrajectoryObject::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

void TrajectoryObject::onChanged(const Property* prop)
{
    App::GeoFeature::onChanged(prop);
}
