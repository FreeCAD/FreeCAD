// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "Diagnostics.h"

#include <map>
#include <ranges>
#include <unordered_set>
#include <vector>

#include <Base/Console.h>

namespace Gui::StyleParameters
{

namespace
{

std::map<std::size_t, Diagnostics::Observer>& observers()
{
    static std::map<std::size_t, Diagnostics::Observer> instance;
    return instance;
}

std::unordered_set<std::string>& alreadyReported()
{
    static std::unordered_set<std::string> instance;
    return instance;
}

std::size_t nextObserverId()
{
    static std::size_t counter = 0;
    return ++counter;
}

/// Names of tokens currently being resolved, innermost last.
std::vector<std::string>& resolutionScopes()
{
    static std::vector<std::string> instance;
    return instance;
}

}  // namespace

Diagnostics::ResolutionScope::ResolutionScope(std::string tokenName)
{
    resolutionScopes().push_back(std::move(tokenName));
}

Diagnostics::ResolutionScope::~ResolutionScope()
{
    resolutionScopes().pop_back();
}

void Diagnostics::Subscription::reset()
{
    if (id_ != 0) {
        Diagnostics::remove(id_);
        id_ = 0;
    }
}

Diagnostics::Subscription Diagnostics::observe(Observer observer)
{
    const std::size_t id = nextObserverId();
    observers().emplace(id, std::move(observer));
    return Subscription(id);
}

void Diagnostics::remove(std::size_t id)
{
    observers().erase(id);
}

void Diagnostics::clear()
{
    alreadyReported().clear();
}

void Diagnostics::emit(const std::string& message)
{
    const auto& scopes = resolutionScopes();
    const std::string prefixed = scopes.empty() ? message : (scopes.back() + ": " + message);

    if (!alreadyReported().insert(prefixed).second) {
        return;
    }

    Base::Console().developerWarning("StyleParameters", "%s\n", prefixed);

    for (const auto& observer : observers() | std::views::values) {
        try {
            observer(prefixed);
        }
        catch (...) {
            // An observer must never break the totality of the code that reported.
        }
    }
}

}  // namespace Gui::StyleParameters
