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

#include "PreCompiled.h"

#include "SketcherTransformationExpressionHelper.h"

#include <Gui/Command.h>
#include <Mod/Sketcher/App/SketchObject.h>

using namespace SketcherGui;

void SketcherTransformationExpressionHelper::storeOriginalExpressions(
    Sketcher::SketchObject* sketchObject,
    const std::vector<int>& listOfGeoIds)
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

                App::ObjectIdentifier spath = sketchObject->Constraints.createPath(i);
                App::PropertyExpressionEngine::ExpressionInfo expr_info =
                    sketchObject->getExpression(spath);

                if (expr_info.expression) {
                    // map expression to geoid as a key
                    originalExpressions[geoId] =
                        std::shared_ptr<App::Expression>(expr_info.expression->copy());
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
    int secondNumberOfCopies)
{
    // apply stored expressions to new constraints, but bail out if we haven't stored
    // anything
    if (originalExpressions.empty() || !sketchObject) {
        return;
    }

    std::string sketchObj = Gui::Command::getObjectCmd(sketchObject);

    const std::vector<Sketcher::Constraint*>& vals = sketchObject->Constraints.getValues();

    for (size_t i = 0; i < vals.size(); i++) {
        const auto& cstr = vals[i];
        if (cstr->isDriving && cstr->isDimensional()) {

            bool expressionApplied = false;

            for (const auto& exprPair : originalExpressions) {
                if (expressionApplied) {
                    break;  // found a match, stop searching for this constraint
                }

                int originalGeoId = exprPair.first;

                // index of the original geometry before operation of the sketcher tool
                int originalIndex = indexOfGeoId(listOfGeoIds, originalGeoId);
                if (originalIndex >= 0) {
                    int firstCurveCreated = sketchObject->getHighestCurveIndex() + 1
                        - static_cast<int>(shapeGeometrySize);
                    int size = static_cast<int>(listOfGeoIds.size());

                    // check all copies of this geometry as we assign them the same expression
                    int numberOfCopiesToMake = numberOfCopies == 0 ? 1 : numberOfCopies;

                    for (int k = 0; k < secondNumberOfCopies && !expressionApplied; k++) {
                        for (int copy = 1; copy <= numberOfCopiesToMake && !expressionApplied;
                             copy++) {
                            int expectedNewGeoId = firstCurveCreated + originalIndex
                                + size * (copy - 1) + size * numberOfCopiesToMake * k;

                            // if this constraint references our copied geometry, apply the
                            // expression
                            if (cstr->First == expectedNewGeoId
                                || (cstr->Second == expectedNewGeoId
                                    && cstr->Type != Sketcher::Radius
                                    && cstr->Type != Sketcher::Diameter
                                    && cstr->Type != Sketcher::Weight)) {

                                Gui::Command::doCommand(Gui::Command::Doc,
                                                        "%s.setExpression('Constraints[%d]', '%s')",
                                                        sketchObj.c_str(),
                                                        static_cast<int>(i),
                                                        exprPair.second->toString().c_str());
                                expressionApplied = true;
                            }
                        }
                    }
                }
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
