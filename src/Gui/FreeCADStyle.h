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

#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <FCGlobal.h>
#include <Base/Color.h>
#include <QBrush>
#include <QColor>
#include <QMarginsF>
#include <QPainter>
#include <QProxyStyle>
#include <QEvent>
#include <QPushButton>
#include <QToolButton>

namespace Gui
{

namespace StyleParameters
{
class Corners;
class Insets;
class InnerShadow;
struct Value;
}  // namespace StyleParameters

class GuiExport FreeCADStyle: public QProxyStyle
{
    Q_OBJECT

public:
    FreeCADStyle()
        : QProxyStyle(QStringLiteral("Fusion"))
    {}

    /**
     * @brief Per-corner border radii in pixels.
     */
    struct CornerRadii
    {
        qreal topLeft = 0;
        qreal topRight = 0;
        qreal bottomRight = 0;
        qreal bottomLeft = 0;
    };

    /**
     * @brief Describes an inward shadow drawn on top of a box background.
     */
    struct InnerShadow
    {
        QColor color;
        qreal x = 0;
        qreal y = 0;
        qreal blur = 0;
    };

    /**
     * @brief Describes the visual appearance of a painted background box.
     *
     * All border fields must be set together (borderColor + borderThickness)
     * for a border to be drawn; partial specification is silently ignored.
     */
    struct BoxBackground
    {
        QBrush background;
        std::optional<QColor> borderColor;
        std::optional<QMarginsF> borderThickness;
        CornerRadii borderRadius;  // default: all zero (sharp corners)
        std::optional<QColor> overlay;
        std::optional<InnerShadow> innerShadow;
    };

    /**
     * @brief Resolves style parameters from an ordered list of prefixes into a BoxBackground.
     *
     * Reads the following parameters (with each prefix prepended) from the global ParameterManager:
     *   - @c {prefix}Background      — solid colour or gradient brush
     *   - @c {prefix}Overlay         — semi-transparent colour composited on top of background
     * (optional)
     *   - @c {prefix}BorderColor     — border fill colour (optional)
     *   - @c {prefix}BorderThickness — per-side border widths as QMarginsF (optional)
     *   - @c {prefix}BorderRadius    — per-corner radii as CornerRadii
     *   - @c {prefix}InnerShadow     — inward shadow (optional)
     *
     * For each property the prefixes are tried in order; the first match wins.
     * Parameters that are absent under all prefixes or carry an incompatible
     * type are silently ignored; their fields retain BoxBackground defaults.
     *
     * @code{.cpp}
     * resolveBoxBackground({"ButtonPressedPrimary", "ButtonPressed", "Button"})
     * @endcode
     */
    BoxBackground resolveBoxBackground(std::initializer_list<std::string_view> prefixes) const;

protected:
    void drawPrimitive(
        PrimitiveElement element,
        const QStyleOption* option,
        QPainter* painter,
        const QWidget* widget = nullptr
    ) const override;

    void drawControl(
        ControlElement element,
        const QStyleOption* option,
        QPainter* painter,
        const QWidget* widget = nullptr
    ) const override;

    QSize sizeFromContents(
        ContentsType type,
        const QStyleOption* option,
        const QSize& size,
        const QWidget* widget = nullptr
    ) const override;

    QRect subControlRect(
        ComplexControl complexControl,
        const QStyleOptionComplex* option,
        SubControl subControl,
        const QWidget* widget = nullptr
    ) const override;

    /**
     * @brief Paints a background box with optional rounded corners and border.
     *
     * If borderColor + borderThickness are both set:
     *   1. Fill the outer rounded rect (borderRadius) with borderColor.
     *   2. Fill the inner rounded rect (inset by borderThickness, inner radii
     *      shrunk by thickness) with background.
     * Otherwise just fill the outer rounded rect with background.
     *
     * The painter state (pen, brush, render hints) is saved and restored.
     */
    static void drawBoxBackground(QPainter* painter, const QRect& rect, const BoxBackground& rule);

    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    /**
     * @brief Resolves a single named parameter from the application's StyleParameterManager.
     *
     * Returns nullopt if the manager is unavailable or the parameter is not defined.
     */
    std::optional<StyleParameters::Value> resolve(std::string_view name) const;

    /**
     * @brief Tries each name in order and returns the first match.
     *
     * Useful for resolved-with-fallback patterns, e.g.:
     * @code{.cpp}
     * resolve({"ToolButtonSmallPadding", "ToolButtonPadding"})
     * @endcode
     */
    std::optional<StyleParameters::Value> resolve(std::initializer_list<std::string_view> names) const;

    /**
     * @brief Tries resolving each @p prefix concatenated with @p suffix, in order.
     *
     * Useful for the prefix-fallback pattern used in resolveBoxBackground:
     * @code{.cpp}
     * resolve({"ButtonHoverPrimary", "ButtonHover", "Button"}, "Background")
     * @endcode
     */
    std::optional<StyleParameters::Value> resolve(
        std::initializer_list<std::string_view> prefixes,
        std::string_view suffix
    ) const;


    static std::string controlSizeSuffix(const QWidget* widget);

    static QColor toQColor(const Base::Color& color);
    static QBrush toBackgroundBrush(const StyleParameters::Value& value);
    static CornerRadii toCornerRadii(const StyleParameters::Corners& corners);
    static QMarginsF toMarginsF(const StyleParameters::Insets& insets);
    static InnerShadow toInnerShadow(const StyleParameters::InnerShadow& shadow);
};

}  // namespace Gui
