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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <BitmapFactory.h>
# include <QBuffer>
# include <QPainter>
#endif

#include "InputHint.h"
#include "InputHintWidget.h"

Gui::InputHintWidget::InputHintWidget(QWidget* parent) : QLabel(parent)
{}

void Gui::InputHintWidget::showHints(const std::list<InputHint>& hints)
{
    if (hints.empty()) {
        clearHints();
        return;
    }

    const auto getKeyImage = [this](InputHint::UserInput key) {
        const auto& factory = BitmapFactory();

        QPixmap image = [&] {
            QColor color = palette().text().color();

            if (auto iconPath = getCustomIconPath(key)) {
                return factory.pixmapFromSvg(*iconPath,
                                             QSize(24, 24),
                                             {{0xFFFFFF, color.rgb() & RGB_MASK}});
            }

            return generateKeyIcon(key, color);
        }();


        QBuffer buffer;
        image.save(&buffer, "png");

        return QStringLiteral("<img src=\"data:image/png;base64,%1\" width=%2 height=24 />")
            .arg(QLatin1String(buffer.data().toBase64()))
            .arg(image.width());
    };

    const auto getHintHTML = [&](const InputHint& hint) {
        QString message = QStringLiteral("<td valign=bottom>%1</td>").arg(tr(hint.message.c_str()));

        for (const auto& sequence : hint.sequences) {
            QList<QString> keyImages;

            for (const auto key : sequence.keys) {
                keyImages.append(getKeyImage(key));
            }

            message = message.arg(keyImages.join(QString {}));
        }

        return message;
    };

    QStringList messages;
    for (const auto& hint : hints) {
        messages.append(getHintHTML(hint));
    }

    QString html = QStringLiteral("<table style=\"line-height: 28px\" height=28>"
                                  "<tr>%1</tr>"
                                  "</table>");

    setText(html.arg(messages.join(QStringLiteral("<td width=10></td>"))));
}

void Gui::InputHintWidget::clearHints()
{
    setText({});
}

std::optional<const char*> Gui::InputHintWidget::getCustomIconPath(const InputHint::UserInput key)
{
    switch (key) {
        case InputHint::UserInput::MouseLeft:
            return ":/icons/user-input/mouse-left.svg";
        case InputHint::UserInput::MouseRight:
            return ":/icons/user-input/mouse-right.svg";
        case InputHint::UserInput::MouseMove:
            return ":/icons/user-input/mouse-move.svg";
        case InputHint::UserInput::MouseMiddle:
            return ":/icons/user-input/mouse-middle.svg";
        case InputHint::UserInput::MouseScroll:
            return ":/icons/user-input/mouse-scroll.svg";
        case InputHint::UserInput::MouseScrollDown:
            return ":/icons/user-input/mouse-scroll-down.svg";
        case InputHint::UserInput::MouseScrollUp:
            return ":/icons/user-input/mouse-scroll-up.svg";
        default:
            return std::nullopt;
    }
}

QPixmap Gui::InputHintWidget::generateKeyIcon(const InputHint::UserInput key, const QColor color)
{
    constexpr int margin = 3;
    constexpr int padding = 4;
    constexpr int radius = 2;
    constexpr int iconTotalHeight = 24;
    constexpr int iconSymbolHeight = iconTotalHeight - 2 * margin;

    const QFont font(QStringLiteral("sans"), 10, QFont::Bold);
    const QFontMetrics fm(font);
    const QString text = QString::fromUtf8(inputRepresentation(key));
    const QRect textBoundingRect = fm.tightBoundingRect(text);

    const int symbolWidth = std::max(textBoundingRect.width() + padding * 2, iconSymbolHeight);

    const QRect keyRect(margin, margin, symbolWidth, 18);

    QPixmap pixmap(symbolWidth + margin * 2, iconTotalHeight);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 2));
    painter.setFont(font);
    painter.drawRoundedRect(keyRect, radius, radius);
    painter.drawText(
        // adjust the rectangle so it is visually centered
        // this is important for characters that are below baseline
        keyRect.translated(0, -(textBoundingRect.y() + textBoundingRect.height()) / 2),
        Qt::AlignHCenter,
        text);

    return pixmap;
}

const char* Gui::InputHintWidget::inputRepresentation(const InputHint::UserInput key)
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
        case KeyTab: return "tab ⭾";
        case KeyBacktab: return "Backtab";
        case KeyBackspace: return "⌫";
        case KeyReturn: return "↵ Enter";
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
        case KeyLeft: return "←";
        case KeyUp: return "↑";
        case KeyRight: return "→";
        case KeyDown: return "↓";
        case KeyPageUp: return "PgDown";
        case KeyPageDown: return "PgUp";

        // modifiers
#ifdef FC_OS_MACOSX
        case KeyShift: return "⇧";
        case KeyControl: return "⌘";
        case KeyMeta: return "⌃";
        case KeyAlt: return "⌥";
#else
        case KeyShift: return "Shift";
        case KeyControl: return "Ctrl";
#ifdef FC_OS_WIN32
        case KeyMeta: return "⊞ Win";
#else
        case KeyMeta: return "❖ Meta";
#endif
        case KeyAlt: return "Alt";
#endif
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