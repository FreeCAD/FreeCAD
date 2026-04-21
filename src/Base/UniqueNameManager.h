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


#pragma once

#include <FCGlobal.h>
#include <vector>
#include <string>
#include <set>
#include <map>
#include <utility>  // Forward declares std::tuple
#include <stdexcept>
#include <algorithm>
#include <tuple>
#include "UnlimitedUnsigned.h"

// ----------------------------------------------------------------------------

namespace Base
{
/// @brief
/// This class acts as a multiset of strings with the capability of generating
///     other names that are not in the collection. Unique names are generated
///     by inserting digits into a base name. Normally these are appended to the
///     base name, but a derived class can override getNameSuffixStartPosition
///     to place the digits elsewhere in the base name.
// This class does not support copy-/move-construction
// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions)
class BaseExport UniqueNameManager
{
protected:
    // This method returns a reverse const iterator on name that references the position
    // of the start of the suffix (or name.crbegin() if no suffix).
    // It must return the same suffix length (name.crbegin() - returnValue) for both
    // unique names (one containing digits) and the corresponding base name (with no digits).
    virtual std::string::const_reverse_iterator getNameSuffixStartPosition(const std::string& name) const
    {
        return name.crbegin();
    }

private:
    template<typename IT>
    class PiecewiseSparseIntegerSet
    {
    public:
        PiecewiseSparseIntegerSet() = default;

    private:
        // Each pair being <lowest, count> represents the span of integers from lowest to
        // (lowest+count-1) inclusive
        using SpanType = std::pair<IT, IT>;
        // This span Comparer class is analogous to std::less and treats overlapping spans as being
        // neither greater nor less than each other
        class Comparer
        {
        public:
            bool operator()(SpanType lhs, SpanType rhs) const
            {
                // The equality case here is when lhs is below and directly adjacent to rhs.
                return lhs.second + lhs.first <= rhs.first;
            }
        };
        // spans is the set of spans. Adjacent spans are coalesced so there are always gaps between
        // the entries.
        std::set<SpanType, Comparer> spans;
        using SpanSetIterator = typename std::set<SpanType, Comparer>::iterator;

    public:
        void add(IT value)
        {
            SpanType newSpan(value, 1);
            // Look for the smallest entry not less than newSpan.
            // Bear in mind that overlapping spans are neither less than nor greater than ech other.
            auto above = spans.lower_bound(newSpan);
            if (above != spans.end() && above->first <= value) {
                // A span was found that includes value so there is nothing to do as it is already
                // in the set.
                return;
            }

            // Set below to the next span below 'after', if any, otherwise to spans.end().
            // Logically, we want spans.begin()-1 so 'below' is always the entry before 'after',
            // but that is not an allowed reference so spans.end() is used
            SpanSetIterator below;
            if (above == spans.begin()) {
                // No spans are less than newSpan
                // (possibly spans is empty, in which case above == spans.end() too)
                below = spans.end();
            }
            else {
                // At least one span is less than newSpan,
                // and 'above' is the next span above that
                // (or above == spans.end() if all spans are below newSpan)
                below = above;
                --below;
            }

            // Determine whether the span above (if any) and/or the span below (if any)
            // are adjacent to newSpan and if so, merge them appropriately and remove the
            // original span(s) that was/were merged, updating newSpan to be the new merged
            // span.
            if (above != spans.end() && below != spans.end()
                && above->first - below->first + 1 == below->second) {
                // below and above have a gap of exactly one between them, and this must be value
                // so we coalesce the two spans (and the gap) into one.
                newSpan = SpanType(below->first, below->second + above->second + 1);
                spans.erase(above);
                above = spans.erase(below);
            }
            else if (below != spans.end() && value - below->first == below->second) {
                // value is adjacent to the end of below, so just expand below by one
                newSpan = SpanType(below->first, below->second + 1);
                above = spans.erase(below);
            }
            else if (above != spans.end() && above->first - value == 1) {
                // value is adjacent to the start of above, so just expand above down by one
                newSpan = SpanType(above->first - 1, above->second + 1);
                above = spans.erase(above);
            }
            // else  value is not adjacent to any existing span, so just make anew span for it
            spans.insert(above, newSpan);
        }
        void remove(IT value)
        {
            SpanType newSpan(value, 1);
            auto at = spans.lower_bound(newSpan);
            if (at == spans.end() || at->first > value) {
                // The found span does not include value so there is nothing to do, as it is already
                // not in the set.
                return;
            }
            if (at->second == 1) {
                // value is the only in this span, just remove the span
                spans.erase(at);
            }
            else if (at->first == value) {
                // value is the first in this span, trim the lower end
                SpanType replacement(at->first + 1, at->second - 1);
                spans.insert(spans.erase(at), replacement);
            }
            else if (value - at->first == at->second - 1) {
                // value is the last in this span, trim the upper end
                SpanType replacement(at->first, at->second - 1);
                spans.insert(spans.erase(at), replacement);
            }
            else {
                // value is in the moddle of the span, so we must split it.
                SpanType firstReplacement(at->first, value - at->first);
                SpanType secondReplacement(value + 1, at->second - ((value + 1) - at->first));
                // Because erase returns the iterator after the erased element, and insert returns
                // the iterator for the inserted item, we want to insert secondReplacement first.
                spans.insert(spans.insert(spans.erase(at), secondReplacement), firstReplacement);
            }
        }
        bool contains(IT value) const
        {
            auto at = spans.lower_bound(SpanType(value, IT(1)));
            return at != spans.end() && at->first <= value;
        }
        bool empty() const
        {
            return spans.empty();
        }
        void clear()
        {
            spans.clear();
        }
        IT next() const
        {
            if (spans.empty()) {
                return IT(0);
            }
            auto last = spans.end();
            --last;
            return last->first + last->second;
        }
    };
    // Keyed as uniqueSeeds[baseName][digitCount][digitValue] iff that seed is taken.
    // We need the double-indexing so that Name01 and Name001 can both be indexed, although we only
    // ever allocate off the longest for each name i.e. uniqueSeeds[baseName].size()-1 digits.
    std::map<std::string, std::vector<PiecewiseSparseIntegerSet<UnlimitedUnsigned>>> uniqueSeeds;
    // Counts of inserted strings that have duplicates, i.e. more than one instance in the
    // collection. This does not contain entries for singleton names.
    std::map<std::string, unsigned int> duplicateCounts;

    /// @brief Break a uniquified name into its parts
    /// @param name The name to break up
    /// @return a tuple(basePrefix, nameSuffix, uniqueDigitCount, uniqueDigitsValue);
    /// The two latter values will be (0,0) if name is a base name without uniquifying digits.
    std::tuple<std::string, std::string, unsigned int, UnlimitedUnsigned> decomposeName(
        const std::string& name
    ) const;

public:
    UniqueNameManager() = default;
    virtual ~UniqueNameManager() = default;

    /// Check if two names are unique forms of the same base name
    bool haveSameBaseName(const std::string& first, const std::string& second) const;

    /// Register a name in the collection.
    /// This collection acts as a multiset, so multiple registrations of the same
    /// name are counted, and an equal number of removals is necessary to fully
    /// remove the name, allowing it to be returned by makeUniqueName
    void addExactName(const std::string& name);
    /// Using the given modelName as a model, generate a name that is not already
    /// in the collection of names. The model name may already contain uniquifying digits
    /// which will be stripped and replaced with other digits as needed.
    std::string makeUniqueName(const std::string& modelName, std::size_t minDigits = 0) const;
    /// Remove a registered name so it can be generated again. If the name was added
    /// several times this decrements the count, and the name is only fully removed when
    /// the count of removes equals or exceeds the count of adds.
    void removeExactName(const std::string& name);
    /// Test if the given name is already in the collection
    bool containsName(const std::string& name) const;
    /// @brief Empty (clear) out the contents from this collection
    void clear()
    {
        uniqueSeeds.clear();
        duplicateCounts.clear();
    }
};
}  // namespace Base
