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

    return matchLowercase(searchText.toLower(), targetText.toLower(), score);
}

bool Gui::FuzzyMatcher::matchLowercase(
    const QString& lowercaseSearchText,
    const QString& lowercaseTargetText,
    int& score
)
{
    if (lowercaseSearchText.isEmpty()) {
        score = 0;
        return true;
    }

    const int matchIndex = lowercaseTargetText.indexOf(lowercaseSearchText);
    if (matchIndex >= 0) {
        const int coverage = (lowercaseSearchText.length() * 100) / lowercaseTargetText.length();
        score = 1000 - matchIndex + coverage;
        return true;
    }

    if (lowercaseSearchText.length() < 3) {
        score = 0;
        return false;
    }

    int searchIndex = 0;
    int targetIndex = 0;
    int consecutiveMatches = 0;
    int maxConsecutive = 0;
    int firstMatchIndex = -1;
    int lastMatchIndex = -1;

    while (searchIndex < lowercaseSearchText.length() && targetIndex < lowercaseTargetText.length()) {
        if (lowercaseSearchText[searchIndex] == lowercaseTargetText[targetIndex]) {
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

    if (searchIndex != lowercaseSearchText.length()) {
        score = 0;
        return false;
    }

    const int matchSpan = lastMatchIndex - firstMatchIndex + 1;
    const int density = (lowercaseSearchText.length() * 100) / matchSpan;
    if (density < 20) {
        score = 0;
        return false;
    }

    const int coverage = (lowercaseSearchText.length() * 100) / lowercaseTargetText.length();
    if (coverage < 15 && lowercaseTargetText.length() > 20) {
        score = 0;
        return false;
    }

    const int densityScore = qMin(density, 100);
    const int consecutiveBonus = (maxConsecutive * 30) / lowercaseSearchText.length();
    const int coverageScore = qMin(coverage * 2, 100);
    const int positionBonus = qMax(0, 50 - firstMatchIndex);

    score = densityScore + consecutiveBonus + coverageScore + positionBonus;
    if (score < 80) {
        score = 0;
        return false;
    }

    return true;
}
