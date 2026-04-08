// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Jacob Oursland <jacob.oursland[at]gmail.com>        *
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

#include "SolverRegistry.h"

#include <Base/Console.h>

#include <algorithm>

using namespace Assembly::Solver;

SolverRegistry& SolverRegistry::instance()
{
    static SolverRegistry registry;
    return registry;
}

void SolverRegistry::registerSolver(const std::string& name, FactoryFn factory)
{
    // Prevent duplicate registration
    auto it = std::ranges::find_if(entries, [&](const Entry& e) { return e.name == name; });
    if (it != entries.end()) {
        Base::Console().warning(
            "SolverRegistry: solver '%s' already registered, skipping\n",
            name.c_str()
        );
        return;
    }
    entries.push_back({name, std::move(factory)});
}

std::shared_ptr<AssemblySolver> SolverRegistry::createSolver(
    const std::string& name,
    AssemblyObject* obj
) const
{
    auto it = std::ranges::find_if(entries, [&](const Entry& e) { return e.name == name; });
    if (it != entries.end()) {
        return it->factory(obj);
    }
    return nullptr;
}

std::vector<std::string> SolverRegistry::getAvailableSolvers() const
{
    std::vector<std::string> names;
    names.reserve(entries.size());
    for (const auto& entry : entries) {
        names.push_back(entry.name);
    }
    return names;
}

std::string SolverRegistry::getDefaultSolverName() const
{
    if (entries.empty()) {
        return "Ondsel";
    }
    return entries.front().name;
}
