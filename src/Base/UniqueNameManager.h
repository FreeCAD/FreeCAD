/***************************************************************************
 *   Copyright (c) 2024 Kevin Martin <kpmartin@papertrail.ca>              *
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


#ifndef SRC_BASE_UNIQUENAMEMANAGER_H_
#define SRC_BASE_UNIQUENAMEMANAGER_H_

#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <utility>
#include <tuple>

// ----------------------------------------------------------------------------

namespace Base
{
/// @brief
/// This class acts as a multiset of strings with the capability of generating
///     other names that are not in the collection. Unique names are generated
///     by inserting digits into a base name. Normally these are appended to the
///     base name, but a derived class can override GetNameSuffixStartPosition
///     to place the digits elsewhere in the base name.
class BaseExport UniqueNameManager
{
protected:
    // This method returns a reverse const iterator on name that references the position
    // of the start of the suffix (or name.crbegin() if no suffix).
    // It must return the same suffix length (name.crbegin() - returnValue) for both
    // unique names (one containing digits) and the corresponding base name (with no digits).
    virtual std::string::const_reverse_iterator
    GetNameSuffixStartPosition(const std::string& name) const
    {
        return name.crbegin();
    }

private:
    class PiecewiseSparseIntegerSet
    {
    public:
        PiecewiseSparseIntegerSet()
        {}

    private:
        // Each pair being <lowest, count> represents the span of integers from lowest to
        // (lowest+count-1) inclusive
        using etype = std::pair<unsigned int, unsigned int>;
        // This span comparer class is analogous to std::less and treats overlapping spans as being
        // neither greater nor less than each other
        class comparer
        {
        public:
            bool operator()(etype lhs, etype rhs) const
            {
                // The equality case here is when lhs is below and directly adjacent to rhs.
                return lhs.second + lhs.first <= rhs.first;
            }
        };
        // Spans is the set of spans. Adjacent spans are coalesced so there are always gaps between
        // the entries.
        std::set<etype, comparer> Spans;
        using iterator = typename std::set<etype, comparer>::iterator;
        using const_iterator = typename std::set<etype, comparer>::const_iterator;

    public:
        void Add(unsigned int value);
        void Remove(unsigned int value);
        bool Contains(unsigned int value) const;
        bool Any() const
        {
            return Spans.size() != 0;
        }
        void Clear()
        {
            Spans.clear();
        }
        unsigned int Next() const
        {
            if (Spans.size() == 0) {
                return 0;
            }
            iterator last = Spans.end();
            --last;
            return last->first + last->second;
        }
    };
    // Keyed as UniqueSeeds[baseName][digitCount][digitValue] iff that seed is taken.
    // We need the double-indexing so that Name01 and Name001 can both be indexed, although we only
    // ever allocate off the longest for each name i.e. UniqueSeeds[baseName].size()-1 digits.
    std::map<std::string, std::vector<PiecewiseSparseIntegerSet>> UniqueSeeds;
    // Counts of inserted strings that have duplicates, i.e. more than one instance in the
    // collection. This does not contain entries for singleton names.
    std::map<std::string, unsigned int> DuplicateCounts;

public:
    /// @brief Break a uniquified name into its parts
    /// @param name The name to break up
    /// @return a tuple(basePrefix, nameSuffix, uniqueDigitCount, uniqueDigitsValue);
    /// The two latter values will be (0,0) if name is a base name without uniquifying digits.
    std::tuple<std::string, std::string, unsigned int, unsigned int>
    decomposeName(const std::string& name) const;

    UniqueNameManager()
    {}

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
    /// @brief EMpty (clear) out the contents from this collection
    void clear()
    {
        UniqueSeeds.clear();
        DuplicateCounts.clear();
    }
};
}  // namespace Base

#endif  // SRC_BASE_UNIQUENAMEMANAGER_H_
