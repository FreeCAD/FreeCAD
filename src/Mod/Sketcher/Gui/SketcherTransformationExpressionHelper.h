/***************************************************************************
 *   Copyright (c) 2025 FreeCAD Contributors                               *
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

#ifndef SKETCHERGUI_SketcherTransformationExpressionHelper_H
#define SKETCHERGUI_SketcherTransformationExpressionHelper_H

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
    void storeOriginalExpressions(Sketcher::SketchObject* sketchObject,
                                  const std::vector<int>& listOfGeoIds);

    /** @brief Apply stored expressions to new constraints after transformation
     *
     * @param sketchObject The sketch object containing the new constraints
     * @param listOfGeoIds Original list of geometry IDs that were transformed
     * @param shapeGeometrySize Number of new geometries created
     * @param numberOfCopies Number of copies made (0 means single operation on the original
     * geometry)
     * @param secondNumberOfCopies Number of rows for array operations
     */
    void copyExpressionsToNewConstraints(Sketcher::SketchObject* sketchObject,
                                         const std::vector<int>& listOfGeoIds,
                                         size_t shapeGeometrySize,
                                         int numberOfCopies,
                                         int secondNumberOfCopies);

    void clear();
    bool hasStoredExpressions() const;

private:
    // original geo id to expression mapping
    std::map<int, std::shared_ptr<App::Expression>> originalExpressions;
};

}  // namespace SketcherGui

#endif  // SKETCHERGUI_SketcherTransformationExpressionHelper_H
