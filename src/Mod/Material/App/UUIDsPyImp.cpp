// SPDX-License-Identifier: LGPL-2.1-or-later

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
    return Py::String(ModelUUIDs::ModelUUID_Legacy_Father);
}

Py::String UUIDsPy::getMaterialStandard() const
{
    return Py::String(ModelUUIDs::ModelUUID_Legacy_MaterialStandard);
}

Py::String UUIDsPy::getArrudaBoyce() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_ArrudaBoyce);
}

Py::String UUIDsPy::getDensity() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_Density);
}

Py::String UUIDsPy::getHardness() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_Hardness);
}

Py::String UUIDsPy::getIsotropicLinearElastic() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_IsotropicLinearElastic);
}

Py::String UUIDsPy::getLinearElastic() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_LinearElastic);
}

Py::String UUIDsPy::getMachinability() const
{
    return Py::String(ModelUUIDs::ModelUUID_Machining_Machinability);
}

Py::String UUIDsPy::getMooneyRivlin() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_MooneyRivlin);
}

Py::String UUIDsPy::getNeoHooke() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_NeoHooke);
}

Py::String UUIDsPy::getOgdenN1() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_OgdenN1);
}

Py::String UUIDsPy::getOgdenN2() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_OgdenN2);
}

Py::String UUIDsPy::getOgdenN3() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_OgdenN3);
}

Py::String UUIDsPy::getOgdenYld2004p18() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_OgdenYld2004p18);
}

Py::String UUIDsPy::getOrthotropicLinearElastic() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_OrthotropicLinearElastic);
}

Py::String UUIDsPy::getPolynomialN1() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_PolynomialN1);
}

Py::String UUIDsPy::getPolynomialN2() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_PolynomialN2);
}

Py::String UUIDsPy::getPolynomialN3() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_PolynomialN3);
}

Py::String UUIDsPy::getReducedPolynomialN1() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_ReducedPolynomialN1);
}

Py::String UUIDsPy::getReducedPolynomialN2() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_ReducedPolynomialN2);
}

Py::String UUIDsPy::getReducedPolynomialN3() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_ReducedPolynomialN3);
}

Py::String UUIDsPy::getYeoh() const
{
    return Py::String(ModelUUIDs::ModelUUID_Mechanical_Yeoh);
}

Py::String UUIDsPy::getFluid() const
{
    return Py::String(ModelUUIDs::ModelUUID_Fluid_Default);
}

Py::String UUIDsPy::getThermal() const
{
    return Py::String(ModelUUIDs::ModelUUID_Thermal_Default);
}

Py::String UUIDsPy::getElectromagnetic() const
{
    return Py::String(ModelUUIDs::ModelUUID_Electromagnetic_Default);
}

Py::String UUIDsPy::getArchitectural() const
{
    return Py::String(ModelUUIDs::ModelUUID_Architectural_Default);
}

Py::String UUIDsPy::getArchitecturalRendering() const
{
    return Py::String(ModelUUIDs::ModelUUID_Rendering_Architectural);
}

Py::String UUIDsPy::getCosts() const
{
    return Py::String(ModelUUIDs::ModelUUID_Costs_Default);
}

Py::String UUIDsPy::getBasicRendering() const
{
    return Py::String(ModelUUIDs::ModelUUID_Rendering_Basic);
}

Py::String UUIDsPy::getTextureRendering() const
{
    return Py::String(ModelUUIDs::ModelUUID_Rendering_Texture);
}

Py::String UUIDsPy::getAdvancedRendering() const
{
    return Py::String(getModelUUIDsPtr()->ModelUUID_Rendering_Advanced);
}

Py::String UUIDsPy::getVectorRendering() const
{
    return Py::String(ModelUUIDs::ModelUUID_Rendering_Vector);
}

Py::String UUIDsPy::getRenderAppleseed() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Appleseed);
}

Py::String UUIDsPy::getRenderCarpaint() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Carpaint);
}

Py::String UUIDsPy::getRenderCycles() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Cycles);
}

Py::String UUIDsPy::getRenderDiffuse() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Diffuse);
}

Py::String UUIDsPy::getRenderDisney() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Disney);
}

Py::String UUIDsPy::getRenderEmission() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Emission);
}

Py::String UUIDsPy::getRenderLuxcore() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Luxcore);
}

Py::String UUIDsPy::getRenderLuxrender() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Luxrender);
}

Py::String UUIDsPy::getRenderGlass() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Glass);
}

Py::String UUIDsPy::getRenderMixed() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Mixed);
}

Py::String UUIDsPy::getRenderOspray() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Ospray);
}

Py::String UUIDsPy::getRenderPbrt() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Pbrt);
}

Py::String UUIDsPy::getRenderPovray() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Povray);
}

Py::String UUIDsPy::getRenderSubstancePBR() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_SubstancePBR);
}

Py::String UUIDsPy::getRenderTexture() const
{
    return Py::String(ModelUUIDs::ModelUUID_Render_Texture);
}

Py::String UUIDsPy::getRenderWB() const
{
    return Py::String(ModelUUIDs::ModelUUID_RenderWB);
}

Py::String UUIDsPy::getTestModel() const
{
    return Py::String(ModelUUIDs::ModelUUID_Test_Model);
}

PyObject* UUIDsPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int UUIDsPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
