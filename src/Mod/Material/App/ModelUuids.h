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

#pragma once

#include <QString>

#include <Base/BaseClass.h>

#include <Mod/Material/MaterialGlobal.h>

namespace Materials
{

class MaterialsExport ModelUUIDs: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ModelUUIDs()
    {}
    ~ModelUUIDs() override = default;

    // UUIDs for predefined material models

    static const QString ModelUUID_Legacy_Father;
    static const QString ModelUUID_Legacy_MaterialStandard;

    static const QString ModelUUID_Machining_Machinability;

    static const QString ModelUUID_Mechanical_ArrudaBoyce;
    static const QString ModelUUID_Mechanical_Density;
    static const QString ModelUUID_Mechanical_Hardness;
    static const QString ModelUUID_Mechanical_IsotropicLinearElastic;
    static const QString ModelUUID_Mechanical_LinearElastic;
    static const QString ModelUUID_Mechanical_MooneyRivlin;
    static const QString ModelUUID_Mechanical_NeoHooke;
    static const QString ModelUUID_Mechanical_OgdenN1;
    static const QString ModelUUID_Mechanical_OgdenN2;
    static const QString ModelUUID_Mechanical_OgdenN3;
    static const QString ModelUUID_Mechanical_OgdenYld2004p18;
    static const QString ModelUUID_Mechanical_OrthotropicLinearElastic;
    static const QString ModelUUID_Mechanical_PolynomialN1;
    static const QString ModelUUID_Mechanical_PolynomialN2;
    static const QString ModelUUID_Mechanical_PolynomialN3;
    static const QString ModelUUID_Mechanical_ReducedPolynomialN1;
    static const QString ModelUUID_Mechanical_ReducedPolynomialN2;
    static const QString ModelUUID_Mechanical_ReducedPolynomialN3;
    static const QString ModelUUID_Mechanical_Yeoh;

    static const QString ModelUUID_Fluid_Default;

    static const QString ModelUUID_Thermal_Default;

    static const QString ModelUUID_Electromagnetic_Default;

    static const QString ModelUUID_Architectural_Default;
    static const QString ModelUUID_Rendering_Architectural;

    static const QString ModelUUID_Costs_Default;

    static const QString ModelUUID_Rendering_Basic;
    static const QString ModelUUID_Rendering_Texture;
    static const QString ModelUUID_Rendering_Advanced;
    static const QString ModelUUID_Rendering_Vector;

    static const QString ModelUUID_Render_Appleseed;
    static const QString ModelUUID_Render_Carpaint;
    static const QString ModelUUID_Render_Cycles;
    static const QString ModelUUID_Render_Diffuse;
    static const QString ModelUUID_Render_Disney;
    static const QString ModelUUID_Render_Emission;
    static const QString ModelUUID_Render_Glass;
    static const QString ModelUUID_Render_Luxcore;
    static const QString ModelUUID_Render_Luxrender;
    static const QString ModelUUID_Render_Mixed;
    static const QString ModelUUID_Render_Ospray;
    static const QString ModelUUID_Render_Pbrt;
    static const QString ModelUUID_Render_Povray;
    static const QString ModelUUID_Render_SubstancePBR;
    static const QString ModelUUID_Render_Texture;
    static const QString ModelUUID_RenderWB;

    static const QString ModelUUID_Test_Model;
};

}  // namespace Materials