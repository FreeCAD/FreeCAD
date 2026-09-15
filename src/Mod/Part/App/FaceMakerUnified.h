// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD contributors
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include "FaceMakerBuildFace.h"

#include <Mod/Part/PartGlobal.h>

namespace Part
{

/**
 * @brief Face maker that extends FaceMakerBuildFace with overlap-aware
 * classification and a non-planar fallback.
 *
 * Adds on top of the BuildFace pipeline:
 * - Partially overlapping wires are fused (union outline) while fully
 *   nested wires still create holes via even-odd classification.
 * - When no plane can be found, falls back to BRepFill_Filling
 *   (N-sided BSpline patch) for each closed wire individually.
 */
class PartExport FaceMakerUnified: public FaceMakerBuildFace
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    std::string getUserFriendlyName() const override;
    std::string getBriefExplanation() const override;

protected:
    void Build_Essence() override;
};

}  // namespace Part
