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
 * using enum InputHint::UserInput;
 *
 * // simple example of hint
 * InputHint selectPointHint {
 *     .message = QWidget::tr("%1 select point"),
 *     .sequences = { MouseLeft }
 * }
 *
 * // multiple sequences are also allowed in one hint
 * InputHint lockAxisHint {
 *     .message = QWidget::tr("%1, %2 or %3 lock X, Y or Z axis"),
 *     .sequences = { KeyX, KeyY, KeyZ }
 * }
 *
 * // hints can also use sequences consisting of multiple keys
 * InputHint exitProgramHint {
 *     .message = QWidget::tr("%1 exit program"),
 *     .sequences = { {KeyAlt, KeyF4} }
 * }
 * @endcode
 */
struct InputHint
{
    enum class UserInput
    {
        // Modifier
        ModifierShift = Qt::KeyboardModifier::ShiftModifier,
        ModifierCtrl = Qt::KeyboardModifier::ControlModifier,
        ModifierAlt = Qt::KeyboardModifier::AltModifier,
        ModifierMeta = Qt::KeyboardModifier::MetaModifier,

        // Keyboard Keys
        KeySpace = Qt::Key_Space,
        KeyExclam = Qt::Key_Exclam,
        KeyQuoteDbl = Qt::Key_QuoteDbl,
        KeyNumberSign = Qt::Key_NumberSign,
        KeyDollar = Qt::Key_Dollar,
        KeyPercent = Qt::Key_Percent,
        KeyAmpersand = Qt::Key_Ampersand,
        KeyApostrophe = Qt::Key_Apostrophe,
        KeyParenLeft = Qt::Key_ParenLeft,
        KeyParenRight = Qt::Key_ParenRight,
        KeyAsterisk = Qt::Key_Asterisk,
        KeyPlus = Qt::Key_Plus,
        KeyComma = Qt::Key_Comma,
        KeyMinus = Qt::Key_Minus,
        KeyPeriod = Qt::Key_Period,
        KeySlash = Qt::Key_Slash,
        Key0 = Qt::Key_0,
        Key1 = Qt::Key_1,
        Key2 = Qt::Key_2,
        Key3 = Qt::Key_3,
        Key4 = Qt::Key_4,
        Key5 = Qt::Key_5,
        Key6 = Qt::Key_6,
        Key7 = Qt::Key_7,
        Key8 = Qt::Key_8,
        Key9 = Qt::Key_9,
        KeyColon = Qt::Key_Colon,
        KeySemicolon = Qt::Key_Semicolon,
        KeyLess = Qt::Key_Less,
        KeyEqual = Qt::Key_Equal,
        KeyGreater = Qt::Key_Greater,
        KeyQuestion = Qt::Key_Question,
        KeyAt = Qt::Key_At,
        KeyA = Qt::Key_A,
        KeyB = Qt::Key_B,
        KeyC = Qt::Key_C,
        KeyD = Qt::Key_D,
        KeyE = Qt::Key_E,
        KeyF = Qt::Key_F,
        KeyG = Qt::Key_G,
        KeyH = Qt::Key_H,
        KeyI = Qt::Key_I,
        KeyJ = Qt::Key_J,
        KeyK = Qt::Key_K,
        KeyL = Qt::Key_L,
        KeyM = Qt::Key_M,
        KeyN = Qt::Key_N,
        KeyO = Qt::Key_O,
        KeyP = Qt::Key_P,
        KeyQ = Qt::Key_Q,
        KeyR = Qt::Key_R,
        KeyS = Qt::Key_S,
        KeyT = Qt::Key_T,
        KeyU = Qt::Key_U,
        KeyV = Qt::Key_V,
        KeyW = Qt::Key_W,
        KeyX = Qt::Key_X,
        KeyY = Qt::Key_Y,
        KeyZ = Qt::Key_Z,
        KeyBracketLeft = Qt::Key_BracketLeft,
        KeyBackslash = Qt::Key_Backslash,
        KeyBracketRight = Qt::Key_BracketRight,
        KeyAsciiCircum = Qt::Key_AsciiCircum,
        KeyUnderscore = Qt::Key_Underscore,
        KeyQuoteLeft = Qt::Key_QuoteLeft,
        KeyBraceLeft = Qt::Key_BraceLeft,
        KeyBar = Qt::Key_Bar,
        KeyBraceRight = Qt::Key_BraceRight,
        KeyAsciiTilde = Qt::Key_AsciiTilde,

        // misc keys
        KeyEscape = Qt::Key_Escape,
        KeyTab = Qt::Key_Tab,
        KeyBacktab = Qt::Key_Backtab,
        KeyBackspace = Qt::Key_Backspace,
        KeyReturn = Qt::Key_Return,
        KeyEnter = Qt::Key_Enter,
        KeyInsert = Qt::Key_Insert,
        KeyDelete = Qt::Key_Delete,
        KeyPause = Qt::Key_Pause,
        KeyPrintScr = Qt::Key_Print,
        KeySysReq = Qt::Key_SysReq,
        KeyClear = Qt::Key_Clear,

        // cursor movement
        KeyHome = Qt::Key_Home,
        KeyEnd = Qt::Key_End,
        KeyLeft = Qt::Key_Left,
        KeyUp = Qt::Key_Up,
        KeyRight = Qt::Key_Right,
        KeyDown = Qt::Key_Down,
        KeyPageUp = Qt::Key_PageUp,
        KeyPageDown = Qt::Key_PageDown,

        // modifiers
        KeyShift = Qt::Key_Shift,
        KeyControl = Qt::Key_Control,
        KeyMeta = Qt::Key_Meta,
        KeyAlt = Qt::Key_Alt,
        KeyCapsLock = Qt::Key_CapsLock,
        KeyNumLock = Qt::Key_NumLock,
        KeyScrollLock = Qt::Key_ScrollLock,

        // function keys
        KeyF1 = Qt::Key_F1,
        KeyF2 = Qt::Key_F2,
        KeyF3 = Qt::Key_F3,
        KeyF4 = Qt::Key_F4,
        KeyF5 = Qt::Key_F5,
        KeyF6 = Qt::Key_F6,
        KeyF7 = Qt::Key_F7,
        KeyF8 = Qt::Key_F8,
        KeyF9 = Qt::Key_F9,
        KeyF10 = Qt::Key_F10,
        KeyF11 = Qt::Key_F11,
        KeyF12 = Qt::Key_F12,
        KeyF13 = Qt::Key_F13,
        KeyF14 = Qt::Key_F14,
        KeyF15 = Qt::Key_F15,
        KeyF16 = Qt::Key_F16,
        KeyF17 = Qt::Key_F17,
        KeyF18 = Qt::Key_F18,
        KeyF19 = Qt::Key_F19,
        KeyF20 = Qt::Key_F20,
        KeyF21 = Qt::Key_F21,
        KeyF22 = Qt::Key_F22,
        KeyF23 = Qt::Key_F23,
        KeyF24 = Qt::Key_F24,
        KeyF25 = Qt::Key_F25,
        KeyF26 = Qt::Key_F26,
        KeyF27 = Qt::Key_F27,
        KeyF28 = Qt::Key_F28,
        KeyF29 = Qt::Key_F29,
        KeyF30 = Qt::Key_F30,
        KeyF31 = Qt::Key_F31,
        KeyF32 = Qt::Key_F32,
        KeyF33 = Qt::Key_F33,
        KeyF34 = Qt::Key_F34,
        KeyF35 = Qt::Key_F35,

        // numpad keys
        KeyNum0 = static_cast<int>(Qt::Key_0) | static_cast<int>(Qt::KeypadModifier),
        KeyNum1 = static_cast<int>(Qt::Key_1) | static_cast<int>(Qt::KeypadModifier),
        KeyNum2 = static_cast<int>(Qt::Key_2) | static_cast<int>(Qt::KeypadModifier),
        KeyNum3 = static_cast<int>(Qt::Key_3) | static_cast<int>(Qt::KeypadModifier),
        KeyNum4 = static_cast<int>(Qt::Key_4) | static_cast<int>(Qt::KeypadModifier),
        KeyNum5 = static_cast<int>(Qt::Key_5) | static_cast<int>(Qt::KeypadModifier),
        KeyNum6 = static_cast<int>(Qt::Key_6) | static_cast<int>(Qt::KeypadModifier),
        KeyNum7 = static_cast<int>(Qt::Key_7) | static_cast<int>(Qt::KeypadModifier),
        KeyNum8 = static_cast<int>(Qt::Key_8) | static_cast<int>(Qt::KeypadModifier),
        KeyNum9 = static_cast<int>(Qt::Key_9) | static_cast<int>(Qt::KeypadModifier),

        // Mouse Keys
        MouseMove = 1 << 16,
        MouseLeft = 2 << 16,
        MouseRight = 3 << 16,
        MouseMiddle = 4 << 16,
        MouseScroll = 5 << 16,
        MouseScrollUp = 6 << 16,
        MouseScrollDown = 7 << 16,
    };

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
