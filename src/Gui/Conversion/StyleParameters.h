/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include <QBrush>
#include <QLinearGradient>
#include <QMarginsF>
#include <QRadialGradient>

#include <Base/Converter.h>
#include <Gui/Conversion/Qt.h>
#include <Gui/StyleParameters/Gradient.h>
#include <Gui/StyleParameters/Insets.h>
#include <Gui/StyleParameters/Value.h>


namespace Base
{

template<>
inline QMarginsF convertTo<QMarginsF, Gui::StyleParameters::Insets>(
    const Gui::StyleParameters::Insets& insets
)
{
    return {
        insets.left().value,
        insets.top().value,
        insets.right().value,
        insets.bottom().value,
    };
}
template<>
inline QLinearGradient convertTo<QLinearGradient, Gui::StyleParameters::LinearGradient>(
    const Gui::StyleParameters::LinearGradient& gradient
)
{
    QLinearGradient qGradient(gradient.x1(), gradient.y1(), gradient.x2(), gradient.y2());
    qGradient.setCoordinateMode(QGradient::ObjectMode);
    for (const auto& stop : gradient.colorStops()) {
        qGradient.setColorAt(stop.position.value, stop.color.asValue<QColor>());
    }
    return qGradient;
}
template<>
inline QRadialGradient convertTo<QRadialGradient, Gui::StyleParameters::RadialGradient>(
    const Gui::StyleParameters::RadialGradient& gradient
)
{
    QRadialGradient
        qGradient(gradient.cx(), gradient.cy(), gradient.radius(), gradient.fx(), gradient.fy());
    qGradient.setCoordinateMode(QGradient::ObjectMode);
    for (const auto& stop : gradient.colorStops()) {
        qGradient.setColorAt(stop.position.value, stop.color.asValue<QColor>());
    }
    return qGradient;
}
template<>
inline QBrush convertTo<QBrush, Gui::StyleParameters::Value>(const Gui::StyleParameters::Value& value)
{
    using namespace Gui::StyleParameters;

    if (const Color* color = value.tryGet<Color>()) {
        return color->asValue<QColor>();
    }

    const Tuple* tuple = value.tryGet<Tuple>();
    if (!tuple) {
        return Qt::NoBrush;
    }

    if (tuple->kind == TupleKind::LinearGradient) {
        if (const auto gradient = LinearGradient::tryFrom(value)) {
            return convertTo<QLinearGradient>(*gradient);
        }
    }

    if (tuple->kind == TupleKind::RadialGradient) {
        if (const auto gradient = RadialGradient::tryFrom(value)) {
            return convertTo<QRadialGradient>(*gradient);
        }
    }

    return Qt::NoBrush;
}

}  // namespace Base
