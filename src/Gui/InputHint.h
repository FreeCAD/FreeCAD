// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                     *
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

#include "FCGlobal.h"

#include <Qt>
#include <QString>

#include <list>
#include <variant>
#include <vector>

namespace Gui
{

/**
 * @brief Representation of one input hint.
 *
 * Each input is essentially a short message defining action with reference to inputs that will
 * trigger that action.
 *
 * For example,
 * @code{c++}
 * // simple example of hint
 * InputHint selectPointHint {
 *     .message = QWidget::tr("%1 select point"),
 *     .sequences = { {MouseInput::MouseLeft} }
 * }
 *
 * // multiple sequences are also allowed in one hint
 * InputHint lockAxisHint {
 *     .message = QWidget::tr("%1, %2 or %3 lock X, Y or Z axis"),
 *     .sequences = { {Qt::Key_X}, {Qt::Key_Y}, {Qt::Key_Z} }
 * }
 *
 * // hints can also use sequences consisting of multiple keys
 * InputHint exitProgramHint {
 *     .message = QWidget::tr("%1 exit program"),
 *     .sequences = { {Qt::AltModifier, Qt::Key_F4} }
 * }
 * @endcode
 */

enum class MouseInput
{
    // Mouse Keys
    MouseMove = 1 << 16,
    MouseLeft = 2 << 16,
    MouseRight = 3 << 16,
    MouseMiddle = 4 << 16,
    MouseScroll = 5 << 16,
    MouseScrollUp = 6 << 16,
    MouseScrollDown = 7 << 16,
    Mouse = 8 << 16,
    MouseMoveLeft = 9 << 16,
    MouseMoveMiddle = 10 << 16,
    MouseMoveRight = 11 << 16,
    MouseDoubleLeft = 12 << 16,
};

struct InputHint
{
    using UserInput = std::variant<Qt::KeyboardModifier, Qt::Key, MouseInput>;
    struct InputSequence
    {
        std::list<UserInput> keys {};

        // This is intentionally left as an implicit conversion. For the intended use one UserInput
        // is de facto InputSequence of length one. Therefore, there is no need to make that explicit.
        explicit(false) InputSequence(UserInput key)
            : InputSequence({key})
        {}

        explicit InputSequence(std::list<UserInput> keys)
            : keys(std::move(keys))
        {}

        InputSequence(const std::initializer_list<UserInput> keys)
            : keys(keys)
        {}

        friend bool operator==(const InputSequence& lhs, const InputSequence& rhs)
        {
            return lhs.keys == rhs.keys;
        }

        friend bool operator!=(const InputSequence& lhs, const InputSequence& rhs)
        {
            return !(lhs == rhs);
        }
    };

    /**
     * @brief Message associated with the hint.
     *
     * The message should confirm to rules stated in the documentation of the InputHint class.
     * Message can contain placeholders like %1, %2 etc. that will then be replaced with graphical
     * key representation from sequences field.
     *
     * @see https://freecad.github.io/DevelopersHandbook/designguide/input-hints.html
     */
    QString message;

    /**
     * @brief List of sequences to be substituted.
     */
    std::list<InputSequence> sequences;

    friend bool operator==(const InputHint& lhs, const InputHint& rhs)
    {
        return lhs.message == rhs.message && lhs.sequences == rhs.sequences;
    }

    friend bool operator!=(const InputHint& lhs, const InputHint& rhs)
    {
        return !(lhs == rhs);
    }
};

template<typename T>
struct StateHints
{
    T state;
    std::list<InputHint> hints;
};

template<typename T>
using HintTable = std::vector<StateHints<T>>;

template<typename T>
static std::list<InputHint> lookupHints(
    T state,
    HintTable<T> table,
    const std::list<InputHint>& fallback = {}
)
{
    const auto stateMatches = [&state](const StateHints<T>& entry) {
        return entry.state == state;
    };

    if (auto it = std::ranges::find_if(table, stateMatches); it != table.end()) {
        return it->hints;
    }

    return fallback;
}

}  // namespace Gui
