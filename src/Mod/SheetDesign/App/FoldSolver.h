// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
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


#ifndef SHEETDESIGN_FOLDSOLVER_H
#define SHEETDESIGN_FOLDSOLVER_H

#include <string>
#include <vector>

#include <Mod/SheetDesign/SheetDesignGlobal.h>

namespace SheetDesign
{

class SheetModel;

/// The result of the fold solver: the angle of each bend at each step from flat to folded.
struct FoldPath
{
    /// One entry for each step. An entry has the angle of each bend of the model, in radians.
    std::vector<std::vector<double>> states;
    /// For each step, the number of the score that was the driver.
    std::vector<int> drivers;
    /// The largest distance between the two sides of a closing bend, in all steps.
    double mismatch {0.0};
    /// The largest difference between an angle at the last step and the angle of the model.
    double endError {0.0};
    /// True if the part arrived at the angles of the model with all loops closed.
    bool complete {false};
    /// Text for each score: if it was a driver or if it followed.
    std::vector<std::string> notes;
};

/**
 * The motion of a part with coupled folds, from flat to folded.
 *
 * A loop of panels has fewer free angles than bends. The solver moves one score at a time (the
 * driver) and finds the angles of the other bends that keep each loop closed.
 *
 * The solver uses a thin pattern: each bend is a hinge on a line in flat space. The bends of
 * one score are put on one line, and a single score that almost meets two other scores is
 * moved to the point where they meet. This is the pattern as the designer draws it. The bend
 * lines of the model have layer offsets and bend allowances, and loops of such hinges cannot
 * move.
 */
class SheetDesignExport FoldSolver
{
public:
    /**
     * @param model   The folded model.
     * @param scores  For each score, the indexes of its bends.
     * @param order   For each score, its number in the fold sequence.
     */
    FoldSolver(
        const SheetModel& model,
        const std::vector<std::vector<int>>& scores,
        const std::vector<int>& order
    );
    ~FoldSolver();
    FoldSolver(const FoldSolver&) = delete;
    FoldSolver& operator=(const FoldSolver&) = delete;

    /// True if the model has a loop of panels.
    bool hasLoops() const;

    /// Fold the part score by score. steps: the number of steps for each driver.
    FoldPath solve(int steps = 12);

    /// The distance between the two sides of the closing bends for a set of angles.
    double mismatch(const std::vector<double>& angles) const;

private:
    struct Data;
    Data* d;
};

}  // namespace SheetDesign

#endif  // SHEETDESIGN_FOLDSOLVER_H
