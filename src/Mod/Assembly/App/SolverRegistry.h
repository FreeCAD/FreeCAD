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

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace Assembly
{
class AssemblyObject;
}

namespace Assembly::Solver
{

class AssemblySolver;

/// Registry of available solver backends.
///
/// Built-in solvers (Ondsel, Chrono) self-register during module init.
/// Future Python-based solvers can register via AssemblyApp.registerSolver().
class SolverRegistry
{
public:
    using FactoryFn = std::function<std::shared_ptr<AssemblySolver>(AssemblyObject*)>;

    static SolverRegistry& instance();

    /// Register a solver backend by name.  First registered becomes the default.
    void registerSolver(const std::string& name, FactoryFn factory);

    /// Create a solver by name.  Returns nullptr if the name is unknown.
    std::shared_ptr<AssemblySolver> createSolver(const std::string& name, AssemblyObject* obj) const;

    /// Return the names of all registered solvers, in registration order.
    std::vector<std::string> getAvailableSolvers() const;

    /// Return the name of the default (first-registered) solver.
    std::string getDefaultSolverName() const;

private:
    SolverRegistry() = default;

    struct Entry
    {
        std::string name;
        FactoryFn factory;
    };
    std::vector<Entry> entries;
};

}  // namespace Assembly::Solver
