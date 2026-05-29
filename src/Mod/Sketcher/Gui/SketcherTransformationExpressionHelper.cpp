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

#include "SketcherTransformationExpressionHelper.h"

#include <Gui/Command.h>
#include <Mod/Sketcher/App/SketchObject.h>

using namespace SketcherGui;

void SketcherTransformationExpressionHelper::storeOriginalExpressions(
    Sketcher::SketchObject* sketchObject,
    const std::vector<int>& listOfGeoIds
)
{
    if (!sketchObject) {
        return;
    }

    const std::vector<Sketcher::Constraint*>& vals = sketchObject->Constraints.getValues();

    originalExpressions.clear();

    for (auto& geoId : listOfGeoIds) {
        for (size_t i = 0; i < vals.size(); i++) {
            const auto& cstr = vals[i];
            if (cstr->isDriving && cstr->isDimensional()
                && (cstr->First == geoId
                    || (cstr->Second == geoId && cstr->Type != Sketcher::Radius
                        && cstr->Type != Sketcher::Diameter && cstr->Type != Sketcher::Weight))) {

                App::ObjectIdentifier spath = sketchObject->Constraints.createPath(static_cast<int>(i)
                );
                App::PropertyExpressionEngine::ExpressionInfo expr_info
                    = sketchObject->getExpression(spath);

                if (expr_info.expression) {
                    // map expression to constraint index as a key, storing both expression and geoId
                    ConstraintExpressionInfo info;
                    info.expression = std::shared_ptr<App::Expression>(expr_info.expression->copy());
                    info.geoId = geoId;
                    originalExpressions[static_cast<int>(i)] = info;
                }
            }
        }
    }
}

void SketcherTransformationExpressionHelper::copyExpressionsToNewConstraints(
    Sketcher::SketchObject* sketchObject,
    const std::vector<int>& listOfGeoIds,
    size_t shapeGeometrySize,
    int numberOfCopies,
    int secondNumberOfCopies
)
{
    // apply stored expressions to new constraints, but bail out if we haven't stored anything
    if (originalExpressions.empty() || !sketchObject) {
        return;
    }

    std::string sketchObj = Gui::Command::getObjectCmd(sketchObject);
    const std::vector<Sketcher::Constraint*>& vals = sketchObject->Constraints.getValues();

    CopyCalculationParams params
        = calculateCopyParams(sketchObject, listOfGeoIds, shapeGeometrySize, numberOfCopies);
    for (size_t i = 0; i < vals.size(); i++) {
        const auto& cstr = vals[i];
        if (!cstr->isDriving || !cstr->isDimensional()) {
            continue;
        }

        // try to find and apply a matching expression for this constraint
        bool expressionApplied = false;
        for (const auto& exprPair : originalExpressions) {
            int origCstrIdx = exprPair.first;
            if (origCstrIdx < 0 || origCstrIdx >= static_cast<int>(vals.size())) {
                continue;
            }
            const auto& origCstr = vals[origCstrIdx];

            expressionApplied = tryApplyExpressionToConstraint(
                cstr,
                i,
                origCstr,
                listOfGeoIds,
                params,
                secondNumberOfCopies,
                exprPair.second.expression,
                sketchObj
            );

            if (expressionApplied) {
                break;
            }
        }
    }
}

void SketcherTransformationExpressionHelper::clear()
{
    originalExpressions.clear();
}

bool SketcherTransformationExpressionHelper::hasStoredExpressions() const
{
    return !originalExpressions.empty();
}

SketcherTransformationExpressionHelper::CopyCalculationParams SketcherTransformationExpressionHelper::calculateCopyParams(
    Sketcher::SketchObject* sketchObject,
    const std::vector<int>& listOfGeoIds,
    size_t shapeGeometrySize,
    int numberOfCopies
) const
{
    CopyCalculationParams params;
    params.firstCurveCreated = sketchObject->getHighestCurveIndex() + 1
        - static_cast<int>(shapeGeometrySize);
    params.size = static_cast<int>(listOfGeoIds.size());
    params.numberOfCopiesToMake = numberOfCopies == 0 ? 1 : numberOfCopies;
    return params;
}

bool SketcherTransformationExpressionHelper::tryApplyExpressionToConstraint(
    const Sketcher::Constraint* cstr,
    size_t constraintIndex,
    const Sketcher::Constraint* origCstr,
    const std::vector<int>& listOfGeoIds,
    const CopyCalculationParams& params,
    int secondNumberOfCopies,
    const std::shared_ptr<App::Expression>& expression,
    const std::string& sketchObj
) const
{
    // check all copies of this geometry as we assign them the same expression
    for (int k = 0; k < secondNumberOfCopies; k++) {
        int startCopy = (k == 0) ? 1 : 0;
        for (int copy = startCopy; copy <= params.numberOfCopiesToMake; copy++) {
            auto getExpectedGeoId = [&](int origGeoId) {
                if (origGeoId < 0) {
                    return origGeoId;  // negative IDs correspond to axes, etc.
                }
                int idx = indexOfGeoId(listOfGeoIds, origGeoId);
                if (idx >= 0) {
                    return params.firstCurveCreated + idx + params.size * (copy - 1)
                        + params.size * (params.numberOfCopiesToMake + 1) * k;
                }
                return origGeoId;  // geometry not in selection remains the same
            };

            bool match = (cstr->Type == origCstr->Type)
                && (cstr->First == getExpectedGeoId(origCstr->First))
                && (cstr->Second == getExpectedGeoId(origCstr->Second))
                && (cstr->Third == getExpectedGeoId(origCstr->Third));

            if (match) {
                Gui::Command::doCommand(
                    Gui::Command::Doc,
                    "%s.setExpression('Constraints[%d]', '%s')",
                    sketchObj.c_str(),
                    static_cast<int>(constraintIndex),
                    expression->toString().c_str()
                );
                return true;
            }
        }
    }
    return false;
}
