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
#include <string_view>
#include <tuple>

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
}  // namespace

using namespace Gui;
using Input = InputHint::UserInput;

// clang-format off
static constexpr auto iconPath = std::to_array<std::tuple<Input, std::string_view>>({
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

static auto userInputStr = std::to_array<std::tuple<Input, QString>>({
    {Qt::Key_NumberSign, QStringLiteral("-/+")},
    {Qt::Key_Tab, Gui::InputHintWidget::tr("tab ⭾")},
    {Qt::Key_Backspace, Gui::InputHintWidget::tr("⌫")},
    {Qt::Key_Return, Gui::InputHintWidget::tr("↵ Enter")},
    {Qt::Key_Left, QStringLiteral("←")},
    {Qt::Key_Up, QStringLiteral("↑")},
    {Qt::Key_Right, QStringLiteral("→")},
    {Qt::Key_Down, QStringLiteral("↓")},
#if defined (FC_OS_MACOSX)
    {Qt::Key_Shift, QStringLiteral("⇧")},
    {Qt::Key_Control, QStringLiteral("⇧")},
    {Qt::Key_Meta, QStringLiteral("⌃")},
    {Qt::Key_Alt, QStringLiteral("⌥")},
#else
    {Qt::Key_Shift, Gui::InputHintWidget::tr("Shift")},
    {Qt::Key_Control, Gui::InputHintWidget::tr("Ctrl")},
    {Qt::Key_Alt, Gui::InputHintWidget::tr("Alt")},
#if defined(FC_OS_WIN32)
    {Qt::Key_Meta, Gui::InputHintWidget::tr("⊞ Win")},
#else
    {Qt::Key_Meta, Gui::InputHintWidget::tr("❖ Meta")},
#endif
#endif
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
    auto it = std::find_if(iconPath.begin(), iconPath.end(), [key](const auto& input) {
        return std::get<0>(input) == key;
    });
    if (it != iconPath.end()) {
        return std::get<1>(*it).data();
    }
    return std::nullopt;
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

QString Gui::InputHintWidget::inputRepresentation(const InputHint::UserInput key)
{
    // Custom string representation
    auto it = std::find_if(userInputStr.begin(), userInputStr.end(), [key](const auto& input) {
        return std::get<0>(input) == key;
    });
    if (it != userInputStr.end()) {
        return std::get<1>(*it);
    }

    if (auto qtkey = std::get_if<Qt::Key>(&key)) {
        // Generic user input
        QKeySequence ks(static_cast<int>(*qtkey));
        return ks.toString(QKeySequence::NativeText);
    }

    if (auto qtkey = std::get_if<Qt::KeyboardModifier>(&key)) {
        // Generic user input
        QKeySequence ks(static_cast<int>(*qtkey));
        return ks.toString(QKeySequence::NativeText);
    }

    return tr("???");
}
