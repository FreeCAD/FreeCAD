// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#include <App/Application.h>
#include <App/Document.h>
#include <App/FeaturePythonPyImp.h>
#include <App/PropertyPythonObject.h>
#include <Base/Console.h>
#include <Base/Tools.h>

#include "Groups.h"
#include "BomGroupPy.h"
#include "JointGroupPy.h"
#include "SimulationGroupPy.h"
#include "SnapshotGroupPy.h"
#include "ViewGroupPy.h"

using namespace Assembly;


PROPERTY_SOURCE(Assembly::BomGroup, App::DocumentObjectGroup)
PROPERTY_SOURCE(Assembly::JointGroup, App::DocumentObjectGroup)
PROPERTY_SOURCE(Assembly::SimulationGroup, App::DocumentObjectGroup)
PROPERTY_SOURCE(Assembly::SnapshotGroup, App::DocumentObjectGroup)
PROPERTY_SOURCE(Assembly::ViewGroup, App::DocumentObjectGroup)


PyObject* BomGroup::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new BomGroupPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}


PyObject* JointGroup::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new JointGroupPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

std::vector<App::DocumentObject*> JointGroup::getJoints()
{
    std::vector<App::DocumentObject*> joints = {};

    Base::PyGILStateLocker lock;
    for (auto joint : getObjects()) {
        if (!joint) {
            continue;
        }

        auto* prop = dynamic_cast<App::PropertyBool*>(joint->getPropertyByName("Suppressed"));
        if (!prop || prop->getValue()) {
            // Filter grounded joints and deactivated joints.
            continue;
        }

        auto proxy = dynamic_cast<App::PropertyPythonObject*>(joint->getPropertyByName("Proxy"));
        if (proxy) {
            if (proxy->getValue().hasAttr("setJointConnectors")) {
                joints.push_back(joint);
            }
        }
    }

    return joints;
}


PyObject* SimulationGroup::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new SimulationGroupPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

PyObject* SnapshotGroup::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new SnapshotGroupPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}


PyObject* ViewGroup::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new ViewGroupPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
