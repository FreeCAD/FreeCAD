/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#include "ModelUuids.h"

#include "UUIDsPy.h"

#include "UUIDsPy.cpp"

using namespace Materials;

// returns a string which represents the object e.g. when printed in python
std::string UUIDsPy::representation() const
{
    return {"<ModelUUIDs object>"};
}

PyObject* UUIDsPy::PyMake(struct _typeobject*, PyObject*, PyObject*)  // Python wrapper
{
    // create a new instance of UUIDsPy and the Twin object
    return new UUIDsPy(new ModelUUIDs);
}

// constructor method
int UUIDsPy::PyInit(PyObject* /*args*/, PyObject* /*kwd*/)
{
    return 0;
}

Py::String UUIDsPy::getFather() const
{
    return Py::String(ModelUUIDs::ModelUUID_Legacy_Father.toStdString());
}

Py::String UUIDsPy::getMaterialStandard() const
{
    return Py::String(ModelUUIDs::ModelUUID_Legacy_MaterialStandard.toStdString());
}

Py::String UUIDsPy::getDensity() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_Density.toStdString());
}

Py::String UUIDsPy::getIsotropicLinearElastic() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic.toStdString());
}

Py::String UUIDsPy::getLinearElastic() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_LinearElastic.toStdString());
}

Py::String UUIDsPy::getOgdenYld2004p18() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_OgdenYld2004p18.toStdString());
}

Py::String UUIDsPy::getOrthotropicLinearElastic() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_OrthotropicLinearElastic.toStdString());
}

Py::String UUIDsPy::getFluid() const
{
    return Py::String(ModelUUIDs::ModelUUID_Fluid_Default.toStdString());
}

Py::String UUIDsPy::getThermal() const
{
    return Py::String(ModelUUIDs::ModelUUID_Thermal_Default.toStdString());
}

Py::String UUIDsPy::getElectromagnetic() const
{
    return Py::String(ModelUUIDs::ModelUUID_Electromagnetic_Default.toStdString());
}

Py::String UUIDsPy::getArchitectural() const
{
    return Py::String(ModelUUIDs::ModelUUID_Architectural_Default.toStdString());
}

Py::String UUIDsPy::getCosts() const
{
    return Py::String(ModelUUIDs::ModelUUID_Costs_Default.toStdString());
}

Py::String UUIDsPy::getBasicRendering() const
{
    return Py::String(ModelUUIDs::ModelUUID_Rendering_Basic.toStdString());
}

Py::String UUIDsPy::getTextureRendering() const
{
    return Py::String(ModelUUIDs::ModelUUID_Rendering_Texture.toStdString());
}

Py::String UUIDsPy::getAdvancedRendering() const
{
    Base::Console().Log(getModelUUIDsPtr()->ModelUUID_Rendering_Advanced.toStdString().c_str());
    return Py::String(getModelUUIDsPtr()->ModelUUID_Rendering_Advanced.toStdString());
}

Py::String UUIDsPy::getVectorRendering() const
{
    return Py::String(ModelUUIDs::ModelUUID_Rendering_Vector.toStdString());
}

PyObject* UUIDsPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int UUIDsPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
