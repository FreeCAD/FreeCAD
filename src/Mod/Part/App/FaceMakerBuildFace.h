// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 FreeCAD contributors                               *
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

#pragma once

#include "FaceMaker.h"

#include <gp_Pln.hxx>

#include <Mod/Part/PartGlobal.h>

namespace Part
{

/**
 * @brief The FaceMakerBuildFace class creates faces by splitting edges at
 * intersections using OCCT's BOPAlgo_BuilderFace. Unlike FaceMakerBullseye
 * which relies on pre-formed closed wires, this maker handles arbitrary
 * overlapping geometry by first splitting all edges at mutual intersections,
 * then finding all bounded planar face regions.
 */
class PartExport FaceMakerBuildFace: public FaceMakerPublic
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    std::string getUserFriendlyName() const override;
    std::string getBriefExplanation() const override;

    void setPlane(const gp_Pln& plane) override;

protected:
    void Build_Essence() override;

private:
    gp_Pln myPlane;
    bool planeSupplied = false;
};

}  // namespace Part
