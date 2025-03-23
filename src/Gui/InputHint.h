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

#ifndef GUI_INPUTHINT_H
#define GUI_INPUTHINT_H

#include <Qt>

namespace Gui
{
struct InputHint
{
    enum class UserInput {
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

        // this is intentionally left as implicit conversion. For the intended use one UserInput is
        // de facto InputSequence of length one, therefore there is no need to make that explicit.
        explicit(false) InputSequence(UserInput key) : InputSequence({ key }) {}
        InputSequence(const std::initializer_list<UserInput> keys) : keys(keys) {}
    };

    const char* message {""};
    std::list<InputSequence> sequences {};
};

inline const char* stringifyUserInput(const InputHint::UserInput key)
{
    using enum InputHint::UserInput;

    // clang-format off
    switch (key) {
        // Keyboard Keys
        case KeySpace: return "Space";
        case KeyExclam: return "!";
        case KeyQuoteDbl: return "\"";
        case KeyNumberSign: return "-/+";
        case KeyDollar: return "$";
        case KeyPercent: return "%";
        case KeyAmpersand: return "&";
        case KeyApostrophe: return "*";
        case KeyParenLeft: return "(";
        case KeyParenRight: return ")";
        case KeyAsterisk: return "*";
        case KeyPlus: return "+";
        case KeyComma: return ",";
        case KeyMinus: return "-";
        case KeyPeriod: return ".";
        case KeySlash: return "/";
        case Key0: return "0";
        case Key1: return "1";
        case Key2: return "2";
        case Key3: return "3";
        case Key4: return "4";
        case Key5: return "5";
        case Key6: return "6";
        case Key7: return "7";
        case Key8: return "8";
        case Key9: return "9";
        case KeyColon: return ":";
        case KeySemicolon: return ";";
        case KeyLess: return "<";
        case KeyEqual: return "=";
        case KeyGreater: return ">";
        case KeyQuestion: return "?";
        case KeyAt: return "@";
        case KeyA: return "A";
        case KeyB: return "B";
        case KeyC: return "C";
        case KeyD: return "D";
        case KeyE: return "E";
        case KeyF: return "F";
        case KeyG: return "G";
        case KeyH: return "H";
        case KeyI: return "I";
        case KeyJ: return "J";
        case KeyK: return "K";
        case KeyL: return "L";
        case KeyM: return "M";
        case KeyN: return "N";
        case KeyO: return "O";
        case KeyP: return "P";
        case KeyQ: return "Q";
        case KeyR: return "R";
        case KeyS: return "S";
        case KeyT: return "T";
        case KeyU: return "U";
        case KeyV: return "V";
        case KeyW: return "W";
        case KeyX: return "X";
        case KeyY: return "Y";
        case KeyZ: return "Z";
        case KeyBracketLeft: return "[";
        case KeyBackslash: return "\\";
        case KeyBracketRight: return "]";
        case KeyUnderscore: return "_";
        case KeyQuoteLeft: return "\"";
        case KeyBraceLeft: return "{";
        case KeyBar: return "Bar";
        case KeyBraceRight: return "}";
        case KeyAsciiTilde: return "~";

        // misc keys
        case KeyEscape: return "Escape";
        case KeyTab: return "⭾";
        case KeyBacktab: return "Backtab";
        case KeyBackspace: return "⌫";
        case KeyReturn: return "Return";
        case KeyEnter: return "Enter";
        case KeyInsert: return "Insert";
        case KeyDelete: return "Delete";
        case KeyPause: return "Pause";
        case KeyPrintScr: return "Print";
        case KeySysReq: return "SysReq";
        case KeyClear: return "Clear";

        // cursor movement
        case KeyHome: return "Home";
        case KeyEnd: return "End";
        case KeyLeft: return "⇦";
        case KeyUp: return "⇧";
        case KeyRight: return "⇨";
        case KeyDown: return "⇩";
        case KeyPageUp: return "PgDown";
        case KeyPageDown: return "PgUp";

        // modifiers
        case KeyShift: return "Shift";
        case KeyControl: return "Control";
        case KeyMeta: return "Meta";
        case KeyAlt: return "Alt";
        case KeyCapsLock: return "Caps Lock";
        case KeyNumLock: return "Num Lock";
        case KeyScrollLock: return "Scroll Lock";

        // function
        case KeyF1: return "F1";
        case KeyF2: return "F2";
        case KeyF3: return "F3";
        case KeyF4: return "F4";
        case KeyF5: return "F5";
        case KeyF6: return "F6";
        case KeyF7: return "F7";
        case KeyF8: return "F8";
        case KeyF9: return "F9";
        case KeyF10: return "F10";
        case KeyF11: return "F11";
        case KeyF12: return "F12";
        case KeyF13: return "F13";
        case KeyF14: return "F14";
        case KeyF15: return "F15";
        case KeyF16: return "F16";
        case KeyF17: return "F17";
        case KeyF18: return "F18";
        case KeyF19: return "F19";
        case KeyF20: return "F20";
        case KeyF21: return "F21";
        case KeyF22: return "F22";
        case KeyF23: return "F23";
        case KeyF24: return "F24";
        case KeyF25: return "F25";
        case KeyF26: return "F26";
        case KeyF27: return "F27";
        case KeyF28: return "F28";
        case KeyF29: return "F29";
        case KeyF30: return "F30";
        case KeyF31: return "F31";
        case KeyF32: return "F32";
        case KeyF33: return "F33";
        case KeyF34: return "F34";
        case KeyF35: return "F35";

        default: return "???";
    }
    // clang-format on
}
}
#endif // GUI_INPUTHINT_H
