// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <map>
#include <set>
#include <vector>

#include "Constraint.h"

namespace Sketcher
{

/// Groups form a forest: a member has one parent, and a child group is represented by its handle.
class GroupHierarchy
{
public:
    explicit GroupHierarchy(const std::vector<Constraint*>& constraints, bool activeOnly = true)
    {
        for (const auto* c : constraints) {
            if ((activeOnly && !c->isActive) || (c->Type != Group && c->Type != Text)) {
                continue;
            }
            const int handle = c->getGeoId(0);
            if (handle < 0 || !children.emplace(handle, std::vector<int>()).second) {
                valid = false;
                continue;
            }
            for (int i = 1; c->hasElement(i); ++i) {
                const int member = c->getGeoId(i);
                // Older constraint records pad the first three slots with GeoUndef.
                if (member == GeoEnum::GeoUndef) {
                    continue;
                }
                if (member < 0 || !parents.emplace(member, handle).second) {
                    valid = false;
                }
                children[handle].push_back(member);
            }
        }
        std::set<int> complete;
        for (const auto& [handle, members] : children) {
            std::set<int> visited;
            int current = handle;
            while (parents.contains(current) && !complete.contains(current)) {
                if (!visited.insert(current).second) {
                    valid = false;
                    break;
                }
                current = parents.at(current);
            }
            complete.insert(visited.begin(), visited.end());
        }
    }

    std::set<int> descendants(int handle) const
    {
        std::set<int> result;
        std::vector<int> pending {handle};
        while (!pending.empty()) {
            const int current = pending.back();
            pending.pop_back();
            if (auto it = children.find(current); it != children.end()) {
                for (int member : it->second) {
                    if (member != handle && result.insert(member).second) {
                        pending.push_back(member);
                    }
                }
            }
        }
        return result;
    }

    bool valid = true;
    std::map<int, int> parents;
    std::map<int, std::vector<int>> children;
};

}  // namespace Sketcher
