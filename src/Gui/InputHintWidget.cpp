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

#include <algorithm>
#include <array>
#include <optional>
#include <ranges>
#include <tuple>
#include <utility>

#include <QBuffer>
#include <QPainter>

#include <FCConfig.h>

#include <BitmapFactory.h>

#include "InputHint.h"
#include "InputHintWidget.h"

namespace
{
constexpr int iconSize = 22;
constexpr int iconMargin = 2;

constexpr auto modifierKeys = std::to_array<std::pair<Qt::KeyboardModifier, Qt::Key>>({
    {Qt::ShiftModifier, Qt::Key_Shift},
    {Qt::ControlModifier, Qt::Key_Control},
    {Qt::AltModifier, Qt::Key_Alt},
    {Qt::MetaModifier, Qt::Key_Meta},
});

// Returns the second element of the first table entry whose first element equals key
template<typename Table, typename Key>
constexpr auto lookup(const Table& table, const Key& key)
    -> std::optional<std::tuple_element_t<1, std::ranges::range_value_t<Table>>>
{
    const auto it = std::ranges::find_if(table, [&key](const auto& entry) {
        return std::get<0>(entry) == key;
    });
    if (it != std::ranges::end(table)) {
        return std::get<1>(*it);
    }
    return std::nullopt;
}
}  // namespace

using namespace Gui;
using Input = InputHint::UserInput;

// clang-format off
static constexpr auto mouseIconPaths = std::to_array<std::tuple<Input, const char*>>({
    {MouseInput::Mouse, ":/icons/user-input/mouse.svg"},
    {MouseInput::MouseLeft, ":/icons/user-input/mouse-left.svg"},
    {MouseInput::MouseRight, ":/icons/user-input/mouse-right.svg"},
    {MouseInput::MouseMove, ":/icons/user-input/mouse-move.svg"},
    {MouseInput::MouseMoveLeft, ":/icons/user-input/mouse-move-left.svg"},
    {MouseInput::MouseMoveMiddle, ":/icons/user-input/mouse-move-middle.svg"},
    {MouseInput::MouseMoveRight, ":/icons/user-input/mouse-move-right.svg"},
    {MouseInput::MouseDoubleLeft, ":/icons/user-input/mouse-double-left.svg"},
    {MouseInput::MouseMiddle, ":/icons/user-input/mouse-middle.svg"},
    {MouseInput::MouseScroll, ":/icons/user-input/mouse-scroll.svg"},
    {MouseInput::MouseScrollDown, ":/icons/user-input/mouse-scroll-down.svg"},
    {MouseInput::MouseScrollUp, ":/icons/user-input/mouse-scroll-up.svg"},
});

// Symbols shown as they are, without translation
static constexpr auto keySymbols = std::to_array<std::tuple<Input, const char*>>({
    {Qt::Key_Space, "  ␣  "},
    {Qt::Key_NumberSign, "-/+"},
    {Qt::Key_Backspace, "⌫"},
    {Qt::Key_Return, "↵"},
    {Qt::Key_Left, "←"},
    {Qt::Key_Up, "↑"},
    {Qt::Key_Right, "→"},
    {Qt::Key_Down, "↓"},

    // modifiers
#if defined (FC_OS_MACOSX)
    {Qt::Key_Shift, "⇧"},
    {Qt::Key_Control, "⌘"},
    {Qt::Key_Meta, "⌃"},
    {Qt::Key_Alt, "⌥"},
#elif defined(FC_OS_WIN32)
    {Qt::Key_Meta, "⊞ Win"},
#else
    {Qt::Key_Meta, "❖ Meta"},
#endif
});

// Labels translated in the Gui::InputHintWidget context
static constexpr auto keyLabels = std::to_array<std::tuple<Input, const char*>>({
    // misc keys
    //: Keyboard key for Escape
    {Qt::Key_Escape, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Esc")},
    //: Keyboard key for Tab
    {Qt::Key_Tab, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Tab ⭾")},
    //: Keyboard key for Backtab
    {Qt::Key_Backtab, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Backtab")},
    //: Keyboard key for numpad Enter
    {Qt::Key_Enter, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Enter")},
    //: Keyboard key for Insert
    {Qt::Key_Insert, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Insert")},
    //: Keyboard key for Delete
    {Qt::Key_Delete, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Del")},
    //: Keyboard key for Pause
    {Qt::Key_Pause, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Pause")},
    //: Keyboard key for Print
    {Qt::Key_Print, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Print")},
    //: Keyboard key for SysReq
    {Qt::Key_SysReq, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "SysReq")},
    //: Keyboard key for Clear
    {Qt::Key_Clear, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Clear")},

    // cursor movement
    //: Keyboard key for Home
    {Qt::Key_Home, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Home")},
    //: Keyboard key for End
    {Qt::Key_End, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "End")},
    //: Keyboard key for Page Up
    {Qt::Key_PageUp, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "PgUp")},
    //: Keyboard key for Page Down
    {Qt::Key_PageDown, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "PgDown")},

    // modifiers
#if !defined(FC_OS_MACOSX)
    //: Keyboard key for Shift on Windows & Linux
    {Qt::Key_Shift, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "⇧ Shift")},
    //: Keyboard key for Control on Windows & Linux
    {Qt::Key_Control, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Ctrl")},
    //: Keyboard key for Alt on Windows & Linux
    {Qt::Key_Alt, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Alt")},
#endif
    //: Keyboard key for Caps Lock
    {Qt::Key_CapsLock, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Caps Lock")},
    //: Keyboard key for Num Lock
    {Qt::Key_NumLock, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Num Lock")},
    //: Keyboard key for Scroll Lock
    {Qt::Key_ScrollLock, QT_TRANSLATE_NOOP("Gui::InputHintWidget", "Scroll Lock")},
});
// clang-format on

Gui::InputHintWidget::InputHintWidget(QWidget* parent)
    : StatusBarLabel(parent)
{
    setMinimumHeight(iconSize + iconMargin * 2);
}

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
                return factory.pixmapFromSvg(
                    *iconPath,
                    QSize(iconSize, iconSize),
                    {{0xFFFFFF, color.rgb() & RGB_MASK}}
                );
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

    QString html = QStringLiteral(
                       "<table style=\"line-height: %1px\" height=%1>"
                       "<tr>%2</tr>"
                       "</table>"
    )
                       .arg(iconSize + iconMargin * 2);

    setText(html.arg(messages.join(QStringLiteral("<td width=10></td>"))));
}

void Gui::InputHintWidget::clearHints()
{
    setText({});
}

std::optional<const char*> Gui::InputHintWidget::getCustomIconPath(const InputHint::UserInput key)
{
    return lookup(mouseIconPaths, key);
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

    QPixmap pixmap = BitmapFactory().empty({symbolWidth + margin * 2, height});

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(color, 2));
    painter.setFont(font);
    painter.drawRoundedRect(keyRect, radius, radius);
    painter.drawText(
        // adjust the rectangle so it is visually centered
        // this is important for characters that are below baseline
        keyRect.translated(0, -(textBoundingRect.y() + textBoundingRect.height() + 1) / 2),
        Qt::AlignHCenter,
        text
    );

    return pixmap;
}

QString Gui::InputHintWidget::inputRepresentation(InputHint::UserInput key)
{
    // A modifier is shown the same way as its key, so only the key is listed in the tables
    if (auto modifier = std::get_if<Qt::KeyboardModifier>(&key)) {
        if (auto modifierKey = lookup(modifierKeys, *modifier)) {
            key = *modifierKey;
        }
    }

    // Custom string representation
    if (auto symbol = lookup(keySymbols, key)) {
        return QString::fromUtf8(*symbol);
    }
    if (auto label = lookup(keyLabels, key)) {
        return tr(*label);
    }

    if (auto qtkey = std::get_if<Qt::Key>(&key)) {
        // Generic key
        QKeySequence ks(static_cast<int>(*qtkey));
        return ks.toString(QKeySequence::NativeText);
    }

    // Mouse inputs have icons, supported modifiers were mapped to keys above
    return QStringLiteral("???");
}
