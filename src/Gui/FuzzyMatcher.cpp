// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2025 tetektoza
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

#include "FuzzyMatcher.h"

#include <QtGlobal>

bool Gui::FuzzyMatcher::match(const QString& searchText, const QString& targetText, int& score)
{
    if (searchText.isEmpty()) {
        score = 0;
        return true;
    }

    const QString lowerSearch = searchText.toLower();
    const QString lowerTarget = targetText.toLower();

    if (lowerTarget.contains(lowerSearch)) {
        const int matchIndex = lowerTarget.indexOf(lowerSearch);
        const int coverage = (lowerSearch.length() * 100) / lowerTarget.length();
        score = 1000 - matchIndex + coverage;
        return true;
    }

    if (lowerSearch.length() < 3) {
        score = 0;
        return false;
    }

    int searchIndex = 0;
    int targetIndex = 0;
    int consecutiveMatches = 0;
    int maxConsecutive = 0;
    int firstMatchIndex = -1;
    int lastMatchIndex = -1;

    while (searchIndex < lowerSearch.length() && targetIndex < lowerTarget.length()) {
        if (lowerSearch[searchIndex] == lowerTarget[targetIndex]) {
            if (firstMatchIndex == -1) {
                firstMatchIndex = targetIndex;
            }
            lastMatchIndex = targetIndex;
            ++searchIndex;
            ++consecutiveMatches;
            maxConsecutive = qMax(maxConsecutive, consecutiveMatches);
        }
        else {
            consecutiveMatches = 0;
        }
        ++targetIndex;
    }

    if (searchIndex != lowerSearch.length()) {
        score = 0;
        return false;
    }

    const int matchSpan = lastMatchIndex - firstMatchIndex + 1;
    const int density = (lowerSearch.length() * 100) / matchSpan;
    if (density < 20) {
        score = 0;
        return false;
    }

    const int coverage = (lowerSearch.length() * 100) / lowerTarget.length();
    if (coverage < 15 && lowerTarget.length() > 20) {
        score = 0;
        return false;
    }

    const int densityScore = qMin(density, 100);
    const int consecutiveBonus = (maxConsecutive * 30) / lowerSearch.length();
    const int coverageScore = qMin(coverage * 2, 100);
    const int positionBonus = qMax(0, 50 - firstMatchIndex);

    score = densityScore + consecutiveBonus + coverageScore + positionBonus;
    if (score < 80) {
        score = 0;
        return false;
    }

    return true;
}
