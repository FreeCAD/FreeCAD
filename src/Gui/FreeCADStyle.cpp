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

#include "FreeCADStyle.h"

#include <algorithm>
#include <QGroupBox>
#include <QLayout>
#include <QLinearGradient>
#include <QPainterPath>
#include <QRadialGradient>

#include <Base/Color.h>
#include <Base/Exception.h>
#include <Base/ServiceProvider.h>

#include "StyleParameters/Corners.h"
#include "StyleParameters/Gradient.h"
#include "StyleParameters/Insets.h"
#include "StyleParameters/ParameterManager.h"

using namespace Gui;

namespace
{

// Arc start angles (in degrees) for each corner of a clockwise rounded rectangle.
constexpr qreal arcStartTopRight = 90;
constexpr qreal arcStartBottomRight = 0;
constexpr qreal arcStartBottomLeft = 270;
constexpr qreal arcStartTopLeft = 180;
constexpr qreal arcSweepClockwise = -90;

QPainterPath roundedRectPath(const QRectF& rect, const FreeCADStyle::CornerRadii& radii)
{
    const qreal topLeft = radii.topLeft;
    const qreal topRight = radii.topRight;
    const qreal bottomRight = radii.bottomRight;
    const qreal bottomLeft = radii.bottomLeft;

    QPainterPath path;
    path.moveTo(rect.left() + topLeft, rect.top());
    path.lineTo(rect.right() - topRight, rect.top());
    path.arcTo(
        rect.right() - (2 * topRight),
        rect.top(),
        2 * topRight,
        2 * topRight,
        arcStartTopRight,
        arcSweepClockwise
    );
    path.lineTo(rect.right(), rect.bottom() - bottomRight);
    path.arcTo(
        rect.right() - (2 * bottomRight),
        rect.bottom() - (2 * bottomRight),
        2 * bottomRight,
        2 * bottomRight,
        arcStartBottomRight,
        arcSweepClockwise
    );
    path.lineTo(rect.left() + bottomLeft, rect.bottom());
    path.arcTo(
        rect.left(),
        rect.bottom() - (2 * bottomLeft),
        2 * bottomLeft,
        2 * bottomLeft,
        arcStartBottomLeft,
        arcSweepClockwise
    );
    path.lineTo(rect.left(), rect.top() + topLeft);
    path.arcTo(rect.left(), rect.top(), 2 * topLeft, 2 * topLeft, arcStartTopLeft, arcSweepClockwise);
    path.closeSubpath();
    return path;
}

FreeCADStyle::CornerRadii innerRadii(const FreeCADStyle::CornerRadii& outer, const QMarginsF& thickness)
{
    auto shrink = [](qreal radius, qreal a, qreal b) -> qreal {
        return std::max(0.0, radius - std::max(a, b));
    };
    return {
        .topLeft = shrink(outer.topLeft, thickness.top(), thickness.left()),
        .topRight = shrink(outer.topRight, thickness.top(), thickness.right()),
        .bottomRight = shrink(outer.bottomRight, thickness.bottom(), thickness.right()),
        .bottomLeft = shrink(outer.bottomLeft, thickness.bottom(), thickness.left()),
    };
}

}  // namespace

void FreeCADStyle::drawBoxBackground(QPainter* painter, const QRect& rect, const BoxBackground& rule)
{
    if (rule.background.style() == Qt::NoBrush) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    // Clip to the outer rect so antialiased arc pixels cannot bleed outside.
    painter->setClipRect(rect, Qt::IntersectClip);

    const bool hasBorder = rule.borderColor.has_value() && rule.borderThickness.has_value();

    if (hasBorder) {
        const QMarginsF& thickness = *rule.borderThickness;

        // Snap each border side to the nearest integer pixel.
        const QRect innerRect = rect.adjusted(
            qRound(thickness.left()),
            qRound(thickness.top()),
            -qRound(thickness.right()),
            -qRound(thickness.bottom())
        );
        // Rounded margins used for inner corner radius computation.
        const QMarginsF snappedThickness(
            qRound(thickness.left()),
            qRound(thickness.top()),
            qRound(thickness.right()),
            qRound(thickness.bottom())
        );

        // 1. Fill outer rect with border colour.
        painter->fillPath(roundedRectPath(QRectF(rect), rule.borderRadius), QBrush(*rule.borderColor));

        // 2. Fill inner rect with background.
        painter->fillPath(
            roundedRectPath(QRectF(innerRect), innerRadii(rule.borderRadius, snappedThickness)),
            rule.background
        );
    }
    else {
        painter->fillPath(roundedRectPath(QRectF(rect), rule.borderRadius), rule.background);
    }

    painter->restore();
}

QColor FreeCADStyle::toQColor(const Base::Color& color)
{
    return QColor::fromRgbF(color.r, color.g, color.b, color.a);
}

FreeCADStyle::CornerRadii FreeCADStyle::toCornerRadii(const StyleParameters::Corners& corners)
{
    return {
        .topLeft = corners.topLeft().value,
        .topRight = corners.topRight().value,
        .bottomRight = corners.bottomRight().value,
        .bottomLeft = corners.bottomLeft().value,
    };
}

QMarginsF FreeCADStyle::toMarginsF(const StyleParameters::Insets& insets)
{
    return QMarginsF(insets.left().value, insets.top().value, insets.right().value, insets.bottom().value);
}

QBrush FreeCADStyle::toBackgroundBrush(const StyleParameters::Value& value)
{
    if (value.holds<Base::Color>()) {
        return QBrush(toQColor(value.get<Base::Color>()));
    }

    if (!value.holds<StyleParameters::Tuple>()) {
        return Qt::NoBrush;
    }

    const auto& tuple = value.get<StyleParameters::Tuple>();

    if (tuple.kind == StyleParameters::TupleKind::LinearGradient) {
        try {
            const StyleParameters::LinearGradient gradient(tuple);
            QLinearGradient qGradient(gradient.x1(), gradient.y1(), gradient.x2(), gradient.y2());
            qGradient.setCoordinateMode(QGradient::ObjectMode);
            for (const auto& stop : gradient.colorStops()) {
                qGradient.setColorAt(stop.position.value, toQColor(stop.color));
            }
            return QBrush(qGradient);
        }
        catch (const Base::Exception&) {
            return Qt::NoBrush;
        }
    }

    if (tuple.kind == StyleParameters::TupleKind::RadialGradient) {
        try {
            const StyleParameters::RadialGradient gradient(tuple);
            QRadialGradient qGradient(
                gradient.cx(),
                gradient.cy(),
                gradient.radius(),
                gradient.fx(),
                gradient.fy()
            );
            qGradient.setCoordinateMode(QGradient::ObjectMode);
            for (const auto& stop : gradient.colorStops()) {
                qGradient.setColorAt(stop.position.value, toQColor(stop.color));
            }
            return QBrush(qGradient);
        }
        catch (const Base::Exception&) {
            return Qt::NoBrush;
        }
    }

    return Qt::NoBrush;
}

FreeCADStyle::BoxBackground FreeCADStyle::resolveBoxBackground(const std::string& prefix)
{
    auto* manager = Base::provideService<StyleParameters::ParameterManager>();

    BoxBackground result;

    if (auto backgroundValue = manager->resolve(prefix + "Background")) {
        result.background = toBackgroundBrush(*backgroundValue);
    }

    if (auto borderColorValue = manager->resolve(prefix + "BorderColor")) {
        if (borderColorValue->holds<Base::Color>()) {
            result.borderColor = toQColor(borderColorValue->get<Base::Color>());
        }
    }

    if (auto borderThicknessValue = manager->resolve(prefix + "BorderThickness")) {
        try {
            result.borderThickness = toMarginsF(
                StyleParameters::BorderThickness(*borderThicknessValue)
            );
        }
        catch (const Base::Exception&) {
        }
    }

    if (auto borderRadiusValue = manager->resolve(prefix + "BorderRadius")) {
        try {
            result.borderRadius = toCornerRadii(StyleParameters::Corners(*borderRadiusValue));
        }
        catch (const Base::Exception&) {
        }
    }

    return result;
}

bool FreeCADStyle::eventFilter(QObject* obj, QEvent* event)
{
    // This is a hacky fix for https://github.com/FreeCAD/FreeCAD/issues/23607
    // Basically after widget is shown or polished we enforce it's minimum size to at least cover
    // the minimum size hint - something that QSS ignores if min-width is specified
    if (event->type() == QEvent::Polish || event->type() == QEvent::Show) {
        if (auto* btn = qobject_cast<QPushButton*>(obj)) {
            btn->setMinimumWidth(std::max(btn->minimumSizeHint().width(), btn->minimumWidth()));
        }
    }

    if (event->type() == QEvent::Polish) {
        if (auto* groupBox = qobject_cast<QGroupBox*>(obj)) {
            if (auto* layout = groupBox->layout()) {
                layout->setContentsMargins(0, 0, 0, 0);
            }
        }
    }

    return QObject::eventFilter(obj, event);
}
