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

using Hint = Gui::InputHint::UserInput;

// clang-format off
static constexpr auto iconPath = std::to_array<std::tuple<Hint, std::string_view>>({
    {Hint::Mouse, ":/icons/user-input/mouse.svg"},
    {Hint::MouseLeft, ":/icons/user-input/mouse-left.svg"},
    {Hint::MouseRight, ":/icons/user-input/mouse-right.svg"},
    {Hint::MouseMove, ":/icons/user-input/mouse-move.svg"},
    {Hint::MouseMoveLeft, ":/icons/user-input/mouse-move-left.svg"},
    {Hint::MouseMoveMiddle, ":/icons/user-input/mouse-move-middle.svg"},
    {Hint::MouseMoveRight, ":/icons/user-input/mouse-move-right.svg"},
    {Hint::MouseDoubleLeft, ":/icons/user-input/mouse-double-left.svg"},
    {Hint::MouseMiddle, ":/icons/user-input/mouse-middle.svg"},
    {Hint::MouseScroll, ":/icons/user-input/mouse-scroll.svg"},
    {Hint::MouseScrollDown, ":/icons/user-input/mouse-scroll-down.svg"},
    {Hint::MouseScrollUp, ":/icons/user-input/mouse-scroll-up.svg"},
});

static auto userInputStr = std::to_array<std::tuple<Hint, QString>>({
    {Hint::KeyNumberSign, QStringLiteral("-/+")},
    {Hint::KeyTab, Gui::InputHintWidget::tr("tab ⭾")},
    {Hint::KeyBackspace, Gui::InputHintWidget::tr("⌫")},
    {Hint::KeyReturn, Gui::InputHintWidget::tr("↵ Enter")},
    {Hint::KeyLeft, QStringLiteral("←")},
    {Hint::KeyUp, QStringLiteral("↑")},
    {Hint::KeyRight, QStringLiteral("→")},
    {Hint::KeyDown, QStringLiteral("↓")},
#if defined (FC_OS_MACOSX)
    {Hint::KeyShift, QStringLiteral("⇧")},
    {Hint::KeyControl, QStringLiteral("⇧")},
    {Hint::KeyMeta, QStringLiteral("⌃")},
    {Hint::KeyAlt, QStringLiteral("⌥")},
#elif defined(FC_OS_WIN32)
    {Hint::KeyMeta, Gui::InputHintWidget::tr("⊞ Win")},
#else
    {Hint::KeyMeta, Gui::InputHintWidget::tr("❖ Meta")},
#endif
});

static constexpr std::array excludeHints {
    Hint::ModifierShift,
    Hint::ModifierCtrl,
    Hint::ModifierAlt,
    Hint::ModifierMeta,
    Hint::Mouse,
    Hint::MouseMove,
    Hint::MouseMoveLeft,
    Hint::MouseMoveMiddle,
    Hint::MouseMoveRight,
    Hint::MouseDoubleLeft,
    Hint::MouseLeft,
    Hint::MouseRight,
    Hint::MouseMiddle,
    Hint::MouseScroll,
    Hint::MouseScrollUp,
    Hint::MouseScrollDown,
};
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

    // Disallowed user input
    auto jt = std::find_if(excludeHints.begin(), excludeHints.end(), [key](const auto& hint) {
        return hint == key;
    });
    if (jt != excludeHints.end()) {
        return tr("???");
    }

    // Generic user input
    QKeySequence ks(static_cast<int>(key));
    return ks.toString(QKeySequence::NativeText);
}
