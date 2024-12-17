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


#ifndef BASE_UNIQUENAMEMANAGER_H
#define BASE_UNIQUENAMEMANAGER_H

#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif
#include <vector>
#include <string>
#include <set>
#include <map>
#include <algorithm>
#include <tuple>

// ----------------------------------------------------------------------------

namespace Base
{
class BaseExport UniqueNameManager
{
protected:
    // This method returns the position of the start of the suffix (or name.cend() if no
    // suffix). It must return the same suffix lentgh (name.size() - returnValue) for both
    // unique names (one containing digits) and the corresponding base name (with no digits).
    virtual std::string::const_iterator GetNameSuffixStartPosition(const std::string& name) const
    {
        return name.cend();
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
            bool operator()(const etype& lhs, const etype& rhs) const
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

public:
    std::tuple<unsigned int, unsigned int> decomposeName(const std::string& name,
                                                         std::string& baseNameOut,
                                                         std::string& nameSuffixOut) const;

    UniqueNameManager()
    {}

    // Register a name in the collection. It is an error (detected only by assertions) to register a
    // name more than once. The effect if undetected is that the second registration will have no
    // effect
    void addExactName(const std::string& name);
    std::string makeUniqueName(const std::string& modelName, int minDigits = 0) const;

    // Remove a registered name so it can be generated again.
    // Nothing happens if you try to remove a non-registered name.
    void removeExactName(const std::string& name);

    bool containsName(const std::string& name) const;

    void clear()
    {
        UniqueSeeds.clear();
    }
};
}  // namespace Base

#endif  // BASE_UNIQUENAMEMANAGER_H
