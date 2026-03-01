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

#include <optional>
#include <string>
#include <FCGlobal.h>
#include <Base/Color.h>
#include <QBrush>
#include <QColor>
#include <QMarginsF>
#include <QPainter>
#include <QProxyStyle>
#include <QEvent>
#include <QPushButton>

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
        std::optional<InnerShadow> innerShadow;
    };

    /**
     * @brief Resolves style parameters with the given prefix into a BoxBackground.
     *
     * Reads the following parameters (with @p prefix prepended) from the global ParameterManager:
     *   - @c {prefix}Background      — solid colour or gradient brush
     *   - @c {prefix}BorderColor     — border fill colour (optional)
     *   - @c {prefix}BorderThickness — per-side border widths as QMarginsF (optional)
     *   - @c {prefix}BorderRadius    — per-corner radii as CornerRadii
     *
     * Parameters that are absent or carry an incompatible type are silently
     * ignored; their fields retain their BoxBackground defaults.
     *
     * If @p fallbackPrefix is non-empty, each property that is absent under
     * @p prefix is retried under @p fallbackPrefix before using the default.
     */
    static BoxBackground resolveBoxBackground(
        const std::string& prefix,
        const std::string& fallbackPrefix = {}
    );

protected:
    void drawPrimitive(
        PrimitiveElement element,
        const QStyleOption* option,
        QPainter* painter,
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
    static QColor toQColor(const Base::Color& color);
    static QBrush toBackgroundBrush(const StyleParameters::Value& value);
    static CornerRadii toCornerRadii(const StyleParameters::Corners& corners);
    static QMarginsF toMarginsF(const StyleParameters::Insets& insets);
    static InnerShadow toInnerShadow(const StyleParameters::InnerShadow& shadow);
};

}  // namespace Gui
