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

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <utility>

#include <fmt/format.h>

#include <FCGlobal.h>

namespace Gui::StyleParameters
{

/**
 * @brief Reports style parameter values that could not be produced as requested.
 *
 * Style parameter access is total: a request that cannot be satisfied yields a default
 * instead of failing. Every such substitution is reported here so that the mishap stays
 * visible to whoever authored the theme.
 */
class GuiExport Diagnostics
{
public:
    using Observer = std::function<void(const std::string&)>;

    /// Keeps an observer registered until destroyed.
    class Subscription
    {
    public:
        Subscription() = default;
        explicit Subscription(std::size_t id)
            : id_(id)
        {}
        ~Subscription()
        {
            reset();
        }

        Subscription(const Subscription&) = delete;
        Subscription& operator=(const Subscription&) = delete;

        Subscription(Subscription&& other) noexcept
            : id_(std::exchange(other.id_, 0))
        {}

        Subscription& operator=(Subscription&& other) noexcept
        {
            if (this != &other) {
                reset();
                id_ = std::exchange(other.id_, 0);
            }
            return *this;
        }

    private:
        void reset();

        std::size_t id_ = 0;
    };

    /**
     * @brief Registers an observer for the lifetime of the returned handle.
     *
     * The observer must not call observe() again, nor destroy a live Subscription, from
     * within its own callback — report() is not reentrant with respect to the observer list.
     */
    [[nodiscard]] static Subscription observe(Observer observer);

    /**
     * @brief Names the token currently being resolved, so reports made while this scope is
     *        active are prefixed with it.
     *
     * Without a diagnostic naming which token produced it, a mishap that degrades silently
     * (rather than throwing) reports only the element and its type, and identical defects in
     * two different tokens dedup into a single, anonymous message. Scopes nest — resolving one
     * token can require resolving another — and a report is prefixed with the innermost active
     * scope.
     */
    class GuiExport ResolutionScope
    {
    public:
        explicit ResolutionScope(std::string tokenName);
        ~ResolutionScope();

        ResolutionScope(const ResolutionScope&) = delete;
        ResolutionScope& operator=(const ResolutionScope&) = delete;
        ResolutionScope(ResolutionScope&&) = delete;
        ResolutionScope& operator=(ResolutionScope&&) = delete;
    };

    /// Reports a mishap. Messages already reported since the last clear() are dropped.
    template<typename... Args>
    static void report(fmt::format_string<Args...> format, Args&&... args)
    {
        emit(fmt::format(format, std::forward<Args>(args)...));
    }

    /// Forgets which messages have already been reported.
    static void clear();

private:
    static void emit(const std::string& message);
    static void remove(std::size_t id);
};

}  // namespace Gui::StyleParameters
