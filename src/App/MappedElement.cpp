// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************************************
 *                                                                                                 *
 *   Copyright (c) 2022 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>                       *
 *   Copyright (c) 2023 FreeCAD Project Association                                                *
 *                                                                                                 *
 *   This file is part of FreeCAD.                                                                 *
 *                                                                                                 *
 *   FreeCAD is free software: you can redistribute it and/or modify it under the terms of the     *
 *   GNU Lesser General Public License as published by the Free Software Foundation, either        *
 *   version 2.1 of the License, or (at your option) any later version.                            *
 *                                                                                                 *
 *   FreeCAD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;          *
 *   without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     *
 *   See the GNU Lesser General Public License for more details.                                   *
 *                                                                                                 *
 *   You should have received a copy of the GNU Lesser General Public License along with           *
 *   FreeCAD. If not, see <https://www.gnu.org/licenses/>.                                         *
 *                                                                                                 *
 **************************************************************************************************/

#include <cstdlib>
#include <unordered_set>

#include "DocumentObject.h"
#include "MappedElement.h"

#include <algorithm>

using namespace Data;

// hextoa converts a single hex character to its integer value.
inline
int hextoa(char c)
{
    constexpr int HEX_OFFSET = 10;
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + HEX_OFFSET;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + HEX_OFFSET;
    }

    return -1; // invalid
}


// getHashLength returns the length of the #hexvalue without the leading '#'.
// If the string does not start with '#', it returns -1.
inline
int getHashLength(const MappedName& name)
{
    if (name[0] != '#') {
        return 0;
    }

    int len = name.size();
    int i = 1; // start after the '#'
    for (; i < len; ++i) {
        if (!std::isxdigit(name[i])) {
            break;
        }
    }

    return i - 1;
}

// getIntLength returns the length of the integer starting at the given start
// index.  If the string does not start with a digit or the start is past the
// end of the string, it returns 0.
inline
int getIntLength(const MappedName& name, int start)
{
    int len = name.size();

    int i = start;
    for (; i < len; ++i) {
        if (std::isdigit(name[i]) == 0) {
            break; // not a digit, stop counting
        }
    }

    return i - start; // return the length of the integer
}

// compareText compares the text part of two MappedNames starting at the given
// index.  It returns -1 if left is less than right, 1 if left is greater than
// right, and 0 if they are equal.
inline
int compareText(const MappedName& left, const MappedName& right, int start)
{
    int leftLength = left.size();
    int rightLength = right.size();
    int minLength = std::min(leftLength, rightLength);

    for (int i = start; i < minLength; ++i) {
        char ac = left[i];
        char bc = right[i];

        if (ac != bc) {
            return ac < bc ? -1 : 1; // left is less than right
        }
    }

    if (leftLength == rightLength) {
        return 0; // they are equal
    }

    // If we reach here, it means one is a prefix of the other.
    return leftLength < rightLength ? -1 : 1; // left is less than right
}

// compareHashed compares two MappedNames that could be hex values, or a
// combination of types.  It returns:
// - -1 if left is less than right,
// - 1 if left is greater than right,
// - 0 if they are equal.
// This function handles identifiers being compared against hex values.
inline
int compareHashed(const MappedName& left, const MappedName& right)
{
    int leftLength = getHashLength(left);
    int rightLength = getHashLength(right);

    if (leftLength < 1 || rightLength < 1 || // one of them is not a hex value
        leftLength != rightLength) {         // they are not the same length

        // Choose the shorter one as less favors identifiers first and numbers
        // to be smallest to largest.
        return leftLength < rightLength ? -1 : 1;
    }

    // They are the same length and both are hex values, compare the hex digits.
    int i = 1;
    for (; i <= leftLength; ++i) {
        int leftValue = hextoa(left[i]);
        int rightValue = hextoa(right[i]);

        if (leftValue != rightValue) {
            return leftValue < rightValue ? -1 : 1;
        }
    }

    // If we reach here, the hex values are equal, compare the text part.
    return compareText(left, right, i);
}

// compareNumbers compares parts of two MappedNames that are identifiers
// containing numbers.  It returns:
// - -1 if left is less than right,
// - 1 if left is greater than right,
// - 0 if they are equal.
inline
int compareNumbers(const MappedName& left, const MappedName& right, int start)
{
    int leftLength = getIntLength(left, start);
    int rightLength = getIntLength(right, start);

    if (leftLength != rightLength) {
        // The shorter number is less than the longer one.
        return leftLength < rightLength ? -1 : 1;
    }

    int i = start;
    for (; i < leftLength; ++i) {
        char lc = left[i];
        char rc = right[i];

        if (lc != rc) {
            return lc < rc ? -1 : 1;
        }
    }

    // If we reach here, the numbers are equal, compare the text part.
    return compareText(left, right, i);
}

// compareIdentifiers compares two MappedNames that are identifiers.
// It returns:
// - -1 if left is less than right,
// - 1 if left is greater than right,
// - 0 if they are equal.
inline
int compareIdentifiers(const MappedName& left, const MappedName& right)
{
    int leftLength = left.size();
    int rightLength = right.size();
    int minLength = std::min(leftLength, rightLength);

    int i = 0;
    for (i = 0; i < minLength; ++i) {
        char lc = left[i];
        char rc = right[i];
        bool ldigit = std::isdigit(lc);
        bool rdigit = std::isdigit(rc);

        if (!ldigit && !rdigit) {
            if (lc != rc) {
                return lc < rc ? -1 : 1; // left is less than right
            }

            continue; // equal, continue to next character
        }

        if (ldigit && rdigit) {
            // both identifiers are the same up to this point,
            return compareNumbers(left, right, i);
        }

        return ldigit ? -1 : 1; // left is less than right
    }

    // The identifier is the entire string, so we compare the lengths.
    if (leftLength == rightLength) {
        return 0; // they are equal
    }

    return leftLength < rightLength ? -1 : 1; // left is less than right
}

// Compare and sort the names.
//
// There are two forms of names that we compare:
// - Hex value:  #<hex><text> like: #123:8;whatever or #aff-otherstuff;
// - Identifier: <id><number><text> like: Edge123:8;whatever or Edge1234:8;whatever
//
// 1. Empty names are less than any other name.
// 2. Identifiers are ordered before hex values.
// 3. If both names are hex values, compare the hex values, smaller first.
// 3. If both names are identifiers, compare the identifiers, lexicographically
//    for the name and numerically for the number, smaller first.
// 6. If the identifiers or hex values are equal, compare the text part lexicographically.
// 7. If the text parts are equal but one is shorter, the shorter name is first.
bool ElementNameComparator::operator()(const MappedName& leftName,
                                       const MappedName& rightName) const
{
    int leftLength = leftName.size();
    int rightLength = rightName.size();

    if (std::min(leftLength, rightLength) == 0) {
        return leftLength < rightLength; // empty names are less than any other name
    }

    // If at least one of the names is a hex value, we compare them as such.
    if (leftName[0] == '#' || rightName[0] == '#') {
        int got = compareHashed(leftName, rightName);
        return got < 0; // if leftName is less than rightName, return true
    }

    int got = compareIdentifiers(leftName, rightName);

    return got < 0; // if leftName is less than rightName or 0, return true
}

HistoryItem::HistoryItem(App::DocumentObject* obj, const Data::MappedName& name)
    : obj(obj)
    , tag(0)
    , element(name)
{
    if (obj) {
        tag = obj->getID();
    }
}
