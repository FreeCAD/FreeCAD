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
# include <QBuffer>
# include <QPainter>
#endif

#include <BitmapFactory.h>

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

    constexpr int iconSize = 22;
    constexpr int iconMargin = 2;

    const auto getKeyImage = [this](InputHint::UserInput key) {
        const auto& factory = BitmapFactory();

        QPixmap image = [&] {
            QColor color = palette().text().color();

            if (auto iconPath = getCustomIconPath(key)) {
                return factory.pixmapFromSvg(*iconPath,
                                             QSize(iconSize, iconSize),
                                             {{0xFFFFFF, color.rgb() & RGB_MASK}});
            }

            return generateKeyIcon(key, color, iconSize);
        }();


        QBuffer buffer;
        image.save(&buffer, "png");

        return QStringLiteral("<img src=\"data:image/png;base64,%1\" height=%2 />")
            .arg(QString::fromLatin1(buffer.data().toBase64()))
            .arg(iconSize);
    };

    const auto getHintHTML = [&](const InputHint& hint) {
        QString message = QStringLiteral("<td valign=bottom>%1</td>").arg(hint.message);

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

    QString html = QStringLiteral("<table style=\"line-height: %1px\" height=%1>"
                                  "<tr>%2</tr>"
                                  "</table>")
                       .arg(iconSize + iconMargin * 2);

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

QPixmap Gui::InputHintWidget::generateKeyIcon(const InputHint::UserInput key, const QColor color, int height)
{
    constexpr int margin = 3;
    constexpr int padding = 4;
    constexpr int radius = 2;
    const int iconSymbolHeight = height - 2 * margin;

    const QFont font(QStringLiteral("sans"), 10, QFont::Bold);
    const QFontMetrics fm(font);
    const QString text = inputRepresentation(key);
    const QRect textBoundingRect = fm.tightBoundingRect(text);

    const int symbolWidth = std::max(textBoundingRect.width() + padding * 2, iconSymbolHeight);

    const QRect keyRect(margin, margin, symbolWidth, iconSymbolHeight);

    QPixmap pixmap = BitmapFactory().empty({ symbolWidth + margin * 2, height });

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

QString Gui::InputHintWidget::inputRepresentation(const InputHint::UserInput key)
{
    using enum InputHint::UserInput;

    // clang-format off
    switch (key) {
        // Keyboard Keys
        case KeySpace: return QStringLiteral("  ␣  ");
        case KeyExclam: return QStringLiteral("!");
        case KeyQuoteDbl: return QStringLiteral("\"");
        case KeyNumberSign: return QStringLiteral("-/+");
        case KeyDollar: return QStringLiteral("$");
        case KeyPercent: return QStringLiteral("%");
        case KeyAmpersand: return QStringLiteral("&");
        case KeyApostrophe: return QStringLiteral("\'");
        case KeyParenLeft: return QStringLiteral("(");
        case KeyParenRight: return QStringLiteral(")");
        case KeyAsterisk: return QStringLiteral("*");
        case KeyPlus: return QStringLiteral("+");
        case KeyComma: return QStringLiteral(",");
        case KeyMinus: return QStringLiteral("-");
        case KeyPeriod: return QStringLiteral(".");
        case KeySlash: return QStringLiteral("/");
        case Key0: return QStringLiteral("0");
        case Key1: return QStringLiteral("1");
        case Key2: return QStringLiteral("2");
        case Key3: return QStringLiteral("3");
        case Key4: return QStringLiteral("4");
        case Key5: return QStringLiteral("5");
        case Key6: return QStringLiteral("6");
        case Key7: return QStringLiteral("7");
        case Key8: return QStringLiteral("8");
        case Key9: return QStringLiteral("9");
        case KeyColon: return QStringLiteral(":");
        case KeySemicolon: return QStringLiteral(";");
        case KeyLess: return QStringLiteral("<");
        case KeyEqual: return QStringLiteral("=");
        case KeyGreater: return QStringLiteral(">");
        case KeyQuestion: return QStringLiteral("?");
        case KeyAt: return QStringLiteral("@");
        case KeyA: return QStringLiteral("A");
        case KeyB: return QStringLiteral("B");
        case KeyC: return QStringLiteral("C");
        case KeyD: return QStringLiteral("D");
        case KeyE: return QStringLiteral("E");
        case KeyF: return QStringLiteral("F");
        case KeyG: return QStringLiteral("G");
        case KeyH: return QStringLiteral("H");
        case KeyI: return QStringLiteral("I");
        case KeyJ: return QStringLiteral("J");
        case KeyK: return QStringLiteral("K");
        case KeyL: return QStringLiteral("L");
        case KeyM: return QStringLiteral("M");
        case KeyN: return QStringLiteral("N");
        case KeyO: return QStringLiteral("O");
        case KeyP: return QStringLiteral("P");
        case KeyQ: return QStringLiteral("Q");
        case KeyR: return QStringLiteral("R");
        case KeyS: return QStringLiteral("S");
        case KeyT: return QStringLiteral("T");
        case KeyU: return QStringLiteral("U");
        case KeyV: return QStringLiteral("V");
        case KeyW: return QStringLiteral("W");
        case KeyX: return QStringLiteral("X");
        case KeyY: return QStringLiteral("Y");
        case KeyZ: return QStringLiteral("Z");
        case KeyBracketLeft: return QStringLiteral("[");
        case KeyBackslash: return QStringLiteral("\\");
        case KeyBracketRight: return QStringLiteral("]");
        case KeyUnderscore: return QStringLiteral("_");
        case KeyQuoteLeft: return QStringLiteral("\"");
        case KeyBraceLeft: return QStringLiteral("{");
        case KeyBar: return QStringLiteral("|");
        case KeyBraceRight: return QStringLiteral("}");
        case KeyAsciiTilde: return QStringLiteral("~");

        // misc keys
        /*: Keyboard key for Escape */
        case KeyEscape: return tr("Esc");
        /*: Keyboard key for Tab */
        case KeyTab: return tr("Tab ⭾");
        /*: Keyboard key for Backtab */
        case KeyBacktab: return tr("Backtab");
        case KeyBackspace: return QStringLiteral("⌫");
        case KeyReturn: return QStringLiteral("↵");
        /*: Keyboard key for numpad Enter */
        case KeyEnter: return tr("Enter");
        /*: Keyboard key for Insert */
        case KeyInsert: return tr("Insert");
        /*: Keyboard key for Delete */
        case KeyDelete: return tr("Del");
        /*: Keyboard key for Pause */
        case KeyPause: return tr("Pause");
        /*: Keyboard key for Print */
        case KeyPrintScr: return tr("Print");
        /*: Keyboard key for SysReq */
        case KeySysReq: return tr("SysReq");
        /*: Keyboard key for Clear */
        case KeyClear: return tr("Clear");

        // cursor movement
        /*: Keyboard key for Home */
        case KeyHome: return tr("Home");
        /*: Keyboard key for End */
        case KeyEnd: return tr("End");
        case KeyLeft: return QStringLiteral("←");
        case KeyUp: return QStringLiteral("↑");
        case KeyRight: return QStringLiteral("→");
        case KeyDown: return QStringLiteral("↓");
        /*: Keyboard key for Page Down */
        case KeyPageUp: return tr("PgDown");
        /*: Keyboard key for Page Up */
        case KeyPageDown: return tr("PgUp");

        // modifiers
#ifdef FC_OS_MACOSX
        case KeyShift: return QStringLiteral("⇧");
        case KeyControl: return QStringLiteral("⌘");
        case KeyMeta: return QStringLiteral("⌃");
        case KeyAlt: return QStringLiteral("⌥");
#else
        /*: Keyboard key for Shift on Windows & Linux */
        case KeyShift: return tr("⇧ Shift");
        /*: Keyboard key for Control on Windows & Linux */
        case KeyControl: return tr("Ctrl");
#ifdef FC_OS_WIN32
        case KeyMeta: return QStringLiteral("⊞ Win");
#else
        case KeyMeta: return QStringLiteral("❖ Meta");
#endif
        /*: Keyboard key for Alt on Windows & Linux */
        case KeyAlt: return tr("Alt");
#endif
        /*: Keyboard key for Caps Lock */
        case KeyCapsLock: return tr("Caps Lock");
        /*: Keyboard key for Num Lock */
        case KeyNumLock: return tr("Num Lock");
        /*: Keyboard key for Scroll Lock */
        case KeyScrollLock: return tr("Scroll Lock");

        // function
        case KeyF1: return QStringLiteral("F1");
        case KeyF2: return QStringLiteral("F2");
        case KeyF3: return QStringLiteral("F3");
        case KeyF4: return QStringLiteral("F4");
        case KeyF5: return QStringLiteral("F5");
        case KeyF6: return QStringLiteral("F6");
        case KeyF7: return QStringLiteral("F7");
        case KeyF8: return QStringLiteral("F8");
        case KeyF9: return QStringLiteral("F9");
        case KeyF10: return QStringLiteral("F10");
        case KeyF11: return QStringLiteral("F11");
        case KeyF12: return QStringLiteral("F12");
        case KeyF13: return QStringLiteral("F13");
        case KeyF14: return QStringLiteral("F14");
        case KeyF15: return QStringLiteral("F15");
        case KeyF16: return QStringLiteral("F16");
        case KeyF17: return QStringLiteral("F17");
        case KeyF18: return QStringLiteral("F18");
        case KeyF19: return QStringLiteral("F19");
        case KeyF20: return QStringLiteral("F20");
        case KeyF21: return QStringLiteral("F21");
        case KeyF22: return QStringLiteral("F22");
        case KeyF23: return QStringLiteral("F23");
        case KeyF24: return QStringLiteral("F24");
        case KeyF25: return QStringLiteral("F25");
        case KeyF26: return QStringLiteral("F26");
        case KeyF27: return QStringLiteral("F27");
        case KeyF28: return QStringLiteral("F28");
        case KeyF29: return QStringLiteral("F29");
        case KeyF30: return QStringLiteral("F30");
        case KeyF31: return QStringLiteral("F31");
        case KeyF32: return QStringLiteral("F32");
        case KeyF33: return QStringLiteral("F33");
        case KeyF34: return QStringLiteral("F34");
        case KeyF35: return QStringLiteral("F35");

        // numpad
        /*: Keyboard key for numpad 0 */
        case KeyNum0: return tr("Num0");
        /*: Keyboard key for numpad 1 */
        case KeyNum1: return tr("Num1");
        /*: Keyboard key for numpad 2 */
        case KeyNum2: return tr("Num2");
        /*: Keyboard key for numpad 3 */
        case KeyNum3: return tr("Num3");
        /*: Keyboard key for numpad 4 */
        case KeyNum4: return tr("Num4");
        /*: Keyboard key for numpad 5 */
        case KeyNum5: return tr("Num5");
        /*: Keyboard key for numpad 6 */
        case KeyNum6: return tr("Num6");
        /*: Keyboard key for numpad 7 */
        case KeyNum7: return tr("Num7");
        /*: Keyboard key for numpad 8 */
        case KeyNum8: return tr("Num8");
        /*: Keyboard key for numpad 9 */
        case KeyNum9: return tr("Num9");


        default: return QStringLiteral("???");
    }
    // clang-format on
}
