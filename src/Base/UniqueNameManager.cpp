// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Kevin Martin <kpmartin@papertrail.ca>              *
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
 ***************************************************************************/

#include "UniqueNameManager.h"

#include <algorithm>
#include <string>
#include <tuple>
#include <vector>


std::tuple<std::string_view, std::string_view, unsigned int, Base::UnlimitedUnsigned> Base::
    UniqueNameManager::decomposeName(std::string_view name) const
{
    auto suffixStart = getNameSuffixStartPosition(name);
    auto digitsStart = std::find_if_not(suffixStart, name.crend(), [](char c) {
        return std::isdigit(c);
    });
    unsigned int digitCount = digitsStart - suffixStart;
    return {
        name.substr(0, name.crend() - digitsStart),
        name.substr(name.crend() - suffixStart),
        digitCount,
        UnlimitedUnsigned::fromString(name.substr(name.crend() - digitsStart, digitCount))
    };
}
bool Base::UniqueNameManager::haveSameBaseName(std::string_view first, std::string_view second) const
{
    auto firstSuffixStart = getNameSuffixStartPosition(first);
    auto secondSuffixStart = getNameSuffixStartPosition(second);
    if (firstSuffixStart - first.crbegin() != secondSuffixStart - second.crbegin()) {
        // The suffixes are different lengths
        return false;
    }
    auto firstDigitsStart = std::find_if_not(firstSuffixStart, first.crend(), [](char c) {
        return std::isdigit(c);
    });
    auto secondDigitsStart = std::find_if_not(secondSuffixStart, second.crend(), [](char c) {
        return std::isdigit(c);
    });
    return std::equal(firstDigitsStart, first.crend(), secondDigitsStart, second.crend());
}

void Base::UniqueNameManager::addExactName(std::string_view name)
{
    const auto [namePrefix, nameSuffix, digitCount, digitsValue] = decomposeName(name);
    std::string baseName {namePrefix};
    baseName += nameSuffix;
    auto baseNameEntry = uniqueSeeds.find(baseName);
    if (baseNameEntry == uniqueSeeds.end()) {
        // First use of baseName
        baseNameEntry
            = uniqueSeeds
                  .emplace(baseName, std::vector<PiecewiseSparseIntegerSet<UnlimitedUnsigned>>())
                  .first;
    }
    if (digitCount >= baseNameEntry->second.size()) {
        // First use of this digitCount
        baseNameEntry->second.resize(digitCount + 1);
    }
    PiecewiseSparseIntegerSet<UnlimitedUnsigned>& baseNameAndDigitCountEntry
        = baseNameEntry->second[digitCount];

    if (baseNameAndDigitCountEntry.contains(digitsValue)) {
        // We already have at least one instance of the name.
        // Increment the duplicateCounts entry for that name,
        // making one if none is present with an initial count of 1
        // representing the singleton element we already have.
        duplicateCounts.try_emplace(std::string {name}, 1U).first->second++;
        return;
    }
    baseNameAndDigitCountEntry.add(digitsValue);
}
std::string Base::UniqueNameManager::makeUniqueName(std::string_view modelName, std::size_t minDigits) const
{
    auto [namePrefix, nameSuffix, digitCount, digitsValue] = decomposeName(modelName);
    std::string baseName {namePrefix};
    baseName += nameSuffix;
    auto baseNameEntry = uniqueSeeds.find(baseName);
    if (baseNameEntry == uniqueSeeds.end()) {
        // First use of baseName, just return it with no unique digits
        return baseName;
    }
    // We don't care about the digit count or value of the suggested name,
    // we always use at least the most digits ever used before.
    digitCount = baseNameEntry->second.size() - 1;
    if (digitCount < minDigits) {
        // Caller is asking for more digits than we have in any registered name.
        // We start the longer digit string at 000...0001 even though we might have shorter strings
        // with larger numeric values.
        digitCount = minDigits;
        digitsValue = UnlimitedUnsigned(1);
    }
    else {
        digitsValue = baseNameEntry->second[digitCount].next();
    }
    const std::string digits = digitsValue.toString();
    std::string digitsPadding;
    if (digitCount > digits.size()) {
        digitsPadding.resize(digitCount - digits.size(), '0');
    }
    return std::string {namePrefix}.append(digitsPadding).append(digits).append(nameSuffix);
}

void Base::UniqueNameManager::removeExactName(std::string_view name)
{
    auto duplicateCountFound = duplicateCounts.find(name);
    if (duplicateCountFound != duplicateCounts.end()) {
        // The name has duplicates. Decrement the duplicate count.
        if (--duplicateCountFound->second <= 1) {
            // After removal, there are no duplicates, only a single instance.
            // Remove the duplicateCounts entry.
            duplicateCounts.erase(duplicateCountFound);
        }
        return;
    }
    const auto [namePrefix, nameSuffix, digitCount, digitsValue] = decomposeName(name);
    std::string baseName {namePrefix};
    baseName += nameSuffix;
    auto baseNameEntry = uniqueSeeds.find(baseName);
    if (baseNameEntry == uniqueSeeds.end()) {
        // name must not be registered, so nothing to do.
        return;
    }
    auto& digitValueSets = baseNameEntry->second;
    if (digitCount >= digitValueSets.size()) {
        // First use of this digitCount, name must not be registered, so nothing to do.
        return;
    }
    digitValueSets[digitCount].remove(digitsValue);
    // an element of digitValueSets may now be newly empty and so may other elements below it
    // Prune off all such trailing empty entries.
    auto lastNonemptyEntry
        = std::find_if(digitValueSets.crbegin(), digitValueSets.crend(), [](auto& it) {
              return !it.empty();
          });
    if (lastNonemptyEntry == digitValueSets.crend()) {
        // All entries are empty, so the entire baseName can be forgotten.
        uniqueSeeds.erase(baseName);
    }
    else {
        digitValueSets.resize(digitValueSets.crend() - lastNonemptyEntry);
    }
}

bool Base::UniqueNameManager::containsName(std::string_view name) const
{
    if (duplicateCounts.find(name) != duplicateCounts.end()) {
        // There are at least two instances of the name
        return true;
    }
    const auto [namePrefix, nameSuffix, digitCount, digitsValue] = decomposeName(name);
    std::string baseName {namePrefix};
    baseName += nameSuffix;
    auto baseNameEntry = uniqueSeeds.find(baseName);
    if (baseNameEntry == uniqueSeeds.end()) {
        // base name is not registered
        return false;
    }
    if (digitCount >= baseNameEntry->second.size()) {
        // First use of this digitCount, name must not be registered, so not in collection
        return false;
    }
    return baseNameEntry->second[digitCount].contains(digitsValue);
}
