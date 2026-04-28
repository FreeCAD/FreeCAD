// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 The FreeCAD Project Association AISBL               *
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

#include <map>
#include <vector>
#include <memory>

#include <App/Expression.h>
#include <App/PropertyExpressionEngine.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "Utils.h"

namespace SketcherGui
{

/** @brief Helper class for preserving expressions during sketch transformations
 *
 * This class provides functionality to preserve expressions when transforming
 * sketch geometries (translate, rotate, scale, etc.). It stores expressions
 * from original constraints and applies them to new constraints after transformation.
 */
class SketcherGuiExport SketcherTransformationExpressionHelper
{
public:
    /** @brief Store expressions from constraints affecting the given geometry list
     *
     * @param sketchObject The sketch object containing the constraints
     * @param listOfGeoIds List of geometry IDs that are being transformed
     */
    void storeOriginalExpressions(
        Sketcher::SketchObject* sketchObject,
        const std::vector<int>& listOfGeoIds
    );

    /** @brief Apply stored expressions to new constraints after transformation
     *
     * @param sketchObject The sketch object containing the new constraints
     * @param listOfGeoIds Original list of geometry IDs that were transformed
     * @param shapeGeometrySize Number of new geometries created
     * @param numberOfCopies Number of copies made (0 means single operation on the original
     * geometry)
     * @param secondNumberOfCopies Number of rows for array operations
     */
    void copyExpressionsToNewConstraints(
        Sketcher::SketchObject* sketchObject,
        const std::vector<int>& listOfGeoIds,
        size_t shapeGeometrySize,
        int numberOfCopies,
        int secondNumberOfCopies
    );

    void clear();
    bool hasStoredExpressions() const;

private:
    struct CopyCalculationParams
    {
        int firstCurveCreated;
        int size;
        int numberOfCopiesToMake;
    };

    struct ConstraintExpressionInfo
    {
        std::shared_ptr<App::Expression> expression;
        int geoId;  // the geoId from listOfGeoIds that this constraint references
    };

    /// calculate parameters needed for copy operations
    CopyCalculationParams calculateCopyParams(
        Sketcher::SketchObject* sketchObject,
        const std::vector<int>& listOfGeoIds,
        size_t shapeGeometrySize,
        int numberOfCopies
    ) const;

    /// try to apply an expression to a constraint if it matches copied geometry
    bool tryApplyExpressionToConstraint(
        const Sketcher::Constraint* cstr,
        size_t constraintIndex,
        int originalIndex,
        const CopyCalculationParams& params,
        int secondNumberOfCopies,
        const std::shared_ptr<App::Expression>& expression,
        const std::string& sketchObj
    ) const;

    /// check if a constraint references the specified geometry ID
    bool constraintReferencesGeometry(const Sketcher::Constraint* cstr, int geoId) const;

    // original constraint index to expression and geoId mapping
    std::map<int, ConstraintExpressionInfo> originalExpressions;
};

}  // namespace SketcherGui
