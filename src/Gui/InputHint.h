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

    enum Key {
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

    struct KeySequence
    {
        std::list<Key> keys;

        KeySequence(Key key) : KeySequence({ key }) {}
        KeySequence(std::initializer_list<Key> keys) : keys(keys) {}
    };

    const char* message;
    std::list<KeySequence> sequences;
};

const inline char* inputKeyToString(InputHint::Key key)
{
    // clang-format off
    switch (key) {
        // Keyboard Keys
        case InputHint::KeySpace: return "Space";
        case InputHint::KeyExclam: return "!";
        case InputHint::KeyQuoteDbl: return "\"";
        case InputHint::KeyNumberSign: return "-/+";
        case InputHint::KeyDollar: return "$";
        case InputHint::KeyPercent: return "%";
        case InputHint::KeyAmpersand: return "&";
        case InputHint::KeyApostrophe: return "*";
        case InputHint::KeyParenLeft: return "(";
        case InputHint::KeyParenRight: return ")";
        case InputHint::KeyAsterisk: return "*";
        case InputHint::KeyPlus: return "+";
        case InputHint::KeyComma: return ",";
        case InputHint::KeyMinus: return "-";
        case InputHint::KeyPeriod: return ".";
        case InputHint::KeySlash: return "/";
        case InputHint::Key0: return "0";
        case InputHint::Key1: return "1";
        case InputHint::Key2: return "2";
        case InputHint::Key3: return "3";
        case InputHint::Key4: return "4";
        case InputHint::Key5: return "5";
        case InputHint::Key6: return "6";
        case InputHint::Key7: return "7";
        case InputHint::Key8: return "8";
        case InputHint::Key9: return "9";
        case InputHint::KeyColon: return ":";
        case InputHint::KeySemicolon: return ";";
        case InputHint::KeyLess: return "<";
        case InputHint::KeyEqual: return "=";
        case InputHint::KeyGreater: return ">";
        case InputHint::KeyQuestion: return "?";
        case InputHint::KeyAt: return "@";
        case InputHint::KeyA: return "A";
        case InputHint::KeyB: return "B";
        case InputHint::KeyC: return "C";
        case InputHint::KeyD: return "D";
        case InputHint::KeyE: return "E";
        case InputHint::KeyF: return "F";
        case InputHint::KeyG: return "G";
        case InputHint::KeyH: return "H";
        case InputHint::KeyI: return "I";
        case InputHint::KeyJ: return "J";
        case InputHint::KeyK: return "K";
        case InputHint::KeyL: return "L";
        case InputHint::KeyM: return "M";
        case InputHint::KeyN: return "N";
        case InputHint::KeyO: return "O";
        case InputHint::KeyP: return "P";
        case InputHint::KeyQ: return "Q";
        case InputHint::KeyR: return "R";
        case InputHint::KeyS: return "S";
        case InputHint::KeyT: return "T";
        case InputHint::KeyU: return "U";
        case InputHint::KeyV: return "V";
        case InputHint::KeyW: return "W";
        case InputHint::KeyX: return "X";
        case InputHint::KeyY: return "Y";
        case InputHint::KeyZ: return "Z";
        case InputHint::KeyBracketLeft: return "[";
        case InputHint::KeyBackslash: return "\\";
        case InputHint::KeyBracketRight: return "]";
        case InputHint::KeyUnderscore: return "_";
        case InputHint::KeyQuoteLeft: return "\"";
        case InputHint::KeyBraceLeft: return "{";
        case InputHint::KeyBar: return "Bar";
        case InputHint::KeyBraceRight: return "}";
        case InputHint::KeyAsciiTilde: return "~";

        // misc keys
        case InputHint::KeyEscape: return "Escape";
        case InputHint::KeyTab: return "⭾";
        case InputHint::KeyBacktab: return "Backtab";
        case InputHint::KeyBackspace: return "⌫";
        case InputHint::KeyReturn: return "Return";
        case InputHint::KeyEnter: return "Enter";
        case InputHint::KeyInsert: return "Insert";
        case InputHint::KeyDelete: return "Delete";
        case InputHint::KeyPause: return "Pause";
        case InputHint::KeyPrintScr: return "Print";
        case InputHint::KeySysReq: return "SysReq";
        case InputHint::KeyClear: return "Clear";

        // cursor movement
        case InputHint::KeyHome: return "Home";
        case InputHint::KeyEnd: return "End";
        case InputHint::KeyLeft: return "⇦";
        case InputHint::KeyUp: return "⇧";
        case InputHint::KeyRight: return "⇨";
        case InputHint::KeyDown: return "⇩";
        case InputHint::KeyPageUp: return "PgDown";
        case InputHint::KeyPageDown: return "PgUp";

        // modifiers
        case InputHint::KeyShift: return "Shift";
        case InputHint::KeyControl: return "Control";
        case InputHint::KeyMeta: return "Meta";
        case InputHint::KeyAlt: return "Alt";
        case InputHint::KeyCapsLock: return "Caps Lock";
        case InputHint::KeyNumLock: return "Num Lock";
        case InputHint::KeyScrollLock: return "Scroll Lock";

        // function keys
        case InputHint::KeyF1: return "F1";
        case InputHint::KeyF2: return "F2";
        case InputHint::KeyF3: return "F3";
        case InputHint::KeyF4: return "F4";
        case InputHint::KeyF5: return "F5";
        case InputHint::KeyF6: return "F6";
        case InputHint::KeyF7: return "F7";
        case InputHint::KeyF8: return "F8";
        case InputHint::KeyF9: return "F9";
        case InputHint::KeyF10: return "F10";
        case InputHint::KeyF11: return "F11";
        case InputHint::KeyF12: return "F12";
        case InputHint::KeyF13: return "F13";
        case InputHint::KeyF14: return "F14";
        case InputHint::KeyF15: return "F15";
        case InputHint::KeyF16: return "F16";
        case InputHint::KeyF17: return "F17";
        case InputHint::KeyF18: return "F18";
        case InputHint::KeyF19: return "F19";
        case InputHint::KeyF20: return "F20";
        case InputHint::KeyF21: return "F21";
        case InputHint::KeyF22: return "F22";
        case InputHint::KeyF23: return "F23";
        case InputHint::KeyF24: return "F24";
        case InputHint::KeyF25: return "F25";
        case InputHint::KeyF26: return "F26";
        case InputHint::KeyF27: return "F27";
        case InputHint::KeyF28: return "F28";
        case InputHint::KeyF29: return "F29";
        case InputHint::KeyF30: return "F30";
        case InputHint::KeyF31: return "F31";
        case InputHint::KeyF32: return "F32";
        case InputHint::KeyF33: return "F33";
        case InputHint::KeyF34: return "F34";
        case InputHint::KeyF35: return "F35";
    }
    // clang-format on
}
}
#endif // GUI_INPUTHINT_H
