/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include "UniqueNameManager.h"

void Base::UniqueNameManager::PiecewiseSparseIntegerSet::Add(unsigned int value)
{
    etype newSpan(value, 1);
    // Look for the smallest entry not less than newSpan.
    // Bear in mind that overlapping spans are neither less than nor greater than ech other.
    iterator above = Spans.lower_bound(newSpan);
    if (above != Spans.end() && above->first <= value) {
        // A span was found that includes value so there is nothing to do as it is already in the set.
        return;
    }

    // Set below to the next span below 'after', if any, otherwise to Spans.end().
    // Logically, we want Spans.begin()-1 so 'below' is always the entry before 'after',
    // but that is not an allowed reference so Spans.end() is used
    iterator below;
    if (above == Spans.begin()) {
        // No spans are less than newSpan
        // (possibly Spans is empty, in which case above == Spans.end() too)
        below = Spans.end();
    }
    else {
        // At least one span is less than newSpan,
        // and 'above' is the next span above that
        // (or above == Spans.end() if all spans are below newSpan)
        below = above;
        --below;
    }

    // Determine whether the span above (if any) and/or the span below (if any)
    // are adjacent to newSpan and if so, merge them appropriately and remove the
    // original span(s) that was/were merged, updating newSpan to be the new merged
    // span.
    if (above != Spans.end() && below != Spans.end()
        && above->first - below->first + 1 == below->second) {
        // below and above have a gap of exactly one between them, and this must be value
        // so we coalesce the two spans (and the gap) into one.
        newSpan = etype(below->first, below->second + above->second + 1);
        Spans.erase(above);
        above = Spans.erase(below);
    }
    else if (below != Spans.end() && value - below->first == below->second) {
        // value is adjacent to the end of below, so just expand below by one
        newSpan = etype(below->first, below->second + 1);
        above = Spans.erase(below);
    }
    else if (above != Spans.end() && above->first - value == 1) {
        // value is adjacent to the start of above, so juse expand above down by one
        newSpan = etype(above->first - 1, above->second + 1);
        above = Spans.erase(above);
    }
    // else  value is not adjacent to any existing span, so just make anew span for it
    Spans.insert(above, newSpan);
}
void Base::UniqueNameManager::PiecewiseSparseIntegerSet::Remove(unsigned int value)
{
    etype newSpan(value, 1);
    iterator at = Spans.lower_bound(newSpan);
    if (at == Spans.end() || at->first > value) {
        // The found span does not include value so there is nothing to do, as it is already not in
        // the set.
        return;
    }
    if (at->second == 1) {
        // value is the only in this span, just remove the span
        Spans.erase(at);
    }
    else if (at->first == value) {
        // value is the first in this span, trim the lower end
        etype replacement(at->first + 1, at->second - 1);
        Spans.insert(Spans.erase(at), replacement);
    }
    else if (value - at->first == at->second - 1) {
        // value is the last in this span, trim the upper end
        etype replacement(at->first, at->second - 1);
        Spans.insert(Spans.erase(at), replacement);
    }
    else {
        // value is in the moddle of the span, so we must split it.
        etype firstReplacement(at->first, value - at->first);
        etype secondReplacement(value + 1, at->second - ((value + 1) - at->first));
        // Because erase returns the iterator after the erased element, and insert returns the
        // iterator for the inserted item, we want to insert secondReplacement first.
        Spans.insert(Spans.insert(Spans.erase(at), secondReplacement), firstReplacement);
    }
}
bool Base::UniqueNameManager::PiecewiseSparseIntegerSet::Contains(unsigned int value) const
{
    iterator at = Spans.lower_bound(etype(value, 1));
    return at != Spans.end() && at->first <= value;
}

std::tuple<unsigned int, unsigned int> Base::UniqueNameManager::decomposeName(const std::string& name,
                                                              std::string& baseNameOut,
                                                              std::string& nameSuffixOut) const
{
    auto suffixStart = std::make_reverse_iterator(GetNameSuffixStartPosition(name));
    nameSuffixOut = name.substr(name.crend() - suffixStart);
    auto digitsStart = std::find_if_not(suffixStart, name.crend(), [](char c) {
        return std::isdigit(c);
    });
    baseNameOut = name.substr(0, name.crend() - digitsStart);
    unsigned int digitCount = digitsStart - suffixStart;
    if (digitCount == 0) {
        // No digits in name
        return std::tuple<unsigned int, unsigned int> {0, 0};
    }
    else {
        return std::tuple<unsigned int, unsigned int> {
            digitCount,
            std::stoul(name.substr(name.crend() - digitsStart, digitCount))};
    }
}
void Base::UniqueNameManager::addExactName(const std::string& name)
{
    std::string baseName;
    std::string nameSuffix;
    unsigned int digitCount;
    unsigned int digitsValue;
    std::tie(digitCount, digitsValue) = decomposeName(name, baseName, nameSuffix);
    baseName += nameSuffix;
    auto baseNameEntry = UniqueSeeds.find(baseName);
    if (baseNameEntry == UniqueSeeds.end()) {
        // First use of baseName
        baseNameEntry =
            UniqueSeeds.emplace(baseName, std::vector<PiecewiseSparseIntegerSet>()).first;
    }
    if (digitCount >= baseNameEntry->second.size()) {
        // First use of this digitCount
        baseNameEntry->second.resize(digitCount + 1);
    }
    PiecewiseSparseIntegerSet& baseNameAndDigitCountEntry = baseNameEntry->second[digitCount];
    // Name should not already be there
    assert(!baseNameAndDigitCountEntry.Contains(digitsValue));
    baseNameAndDigitCountEntry.Add(digitsValue);
}
std::string Base::UniqueNameManager::makeUniqueName(const std::string& modelName,
                                                    int minDigits) const
{
    std::string namePrefix;
    std::string nameSuffix;
    decomposeName(modelName, namePrefix, nameSuffix);
    std::string baseName = namePrefix + nameSuffix;
    auto baseNameEntry = UniqueSeeds.find(baseName);
    if (baseNameEntry == UniqueSeeds.end()) {
        // First use of baseName, just return it with no unique digits
        return baseName;
    }
    // We don't care about the digit count of the suggested name, we always use at least the most
    // digits ever used before.
    int digitCount = baseNameEntry->second.size() - 1;
    unsigned int digitsValue;
    if (digitCount < minDigits) {
        // Caller is asking for more digits than we have in any registered name.
        // We start the longer digit string at 000...0001 even though we might have shorter strings
        // with larger numeric values.
        digitCount = minDigits;
        digitsValue = 1;
    }
    else {
        digitsValue = baseNameEntry->second[digitCount].Next();
    }
    std::string digits = std::to_string(digitsValue);
    if (digitCount > digits.size()) {
        namePrefix += std::string(digitCount - digits.size(), '0');
    }
    return namePrefix + digits + nameSuffix;
}

void Base::UniqueNameManager::removeExactName(const std::string& name)
{
    std::string baseName;
    std::string nameSuffix;
    unsigned int digitCount;
    unsigned int digitsValue;
    std::tie(digitCount, digitsValue) = decomposeName(name, baseName, nameSuffix);
    baseName += nameSuffix;
    auto baseNameEntry = UniqueSeeds.find(baseName);
    if (baseNameEntry == UniqueSeeds.end()) {
        // name must not be registered, so nothing to do.
        return;
    }
    auto& digitValueSets = baseNameEntry->second;
    if (digitCount >= digitValueSets.size()) {
        // First use of this digitCount, name must not be registered, so nothing to do.
        return;
    }
    digitValueSets[digitCount].Remove(digitsValue);
    // an element of digitValueSets may now be newly empty and so may other elements below it
    // Prune off all such trailing empty entries.
    auto lastNonemptyEntry =
        std::find_if(digitValueSets.crbegin(), digitValueSets.crend(), [](auto& it) {
            return it.Any();
        });
    if (lastNonemptyEntry == digitValueSets.crend()) {
        // All entries are empty, so the entire baseName can be forgotten.
        UniqueSeeds.erase(baseName);
    }
    else {
        digitValueSets.resize(digitValueSets.crend() - lastNonemptyEntry);
    }
}

bool Base::UniqueNameManager::containsName(const std::string& name) const
{
    std::string baseName;
    std::string nameSuffix;
    unsigned int digitCount;
    unsigned int digitsValue;
    std::tie(digitCount, digitsValue) = decomposeName(name, baseName, nameSuffix);
    baseName += nameSuffix;
    auto baseNameEntry = UniqueSeeds.find(baseName);
    if (baseNameEntry == UniqueSeeds.end()) {
        // base name is not registered
        return false;
    }
    if (digitCount >= baseNameEntry->second.size()) {
        // First use of this digitCount, name must not be registered, so not in collection
        return false;
    }
    return baseNameEntry->second[digitCount].Contains(digitsValue);
}
