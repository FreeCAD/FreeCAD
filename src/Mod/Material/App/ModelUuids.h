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

#ifndef MATERIAL_MODELUUIDS_H
#define MATERIAL_MODELUUIDS_H

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

    static const QString ModelUUID_Mechanical_Density;
    static const QString ModelUUID_Mechanical_IsotropicLinearElastic;
    static const QString ModelUUID_Mechanical_LinearElastic;
    static const QString ModelUUID_Mechanical_OgdenYld2004p18;
    static const QString ModelUUID_Mechanical_OrthotropicLinearElastic;

    static const QString ModelUUID_Fluid_Default;

    static const QString ModelUUID_Thermal_Default;

    static const QString ModelUUID_Electromagnetic_Default;

    static const QString ModelUUID_Architectural_Default;

    static const QString ModelUUID_Costs_Default;

    static const QString ModelUUID_Rendering_Basic;
    static const QString ModelUUID_Rendering_Texture;
    static const QString ModelUUID_Rendering_Advanced;
    static const QString ModelUUID_Rendering_Vector;

    static const QString ModelUUID_Test_Material;
};

}  // namespace Materials

#endif  // MATERIAL_MODELUUIDS_H
