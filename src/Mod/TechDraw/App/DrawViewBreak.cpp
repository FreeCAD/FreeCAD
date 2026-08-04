// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
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

#include "DrawViewBreak.h"

#include "DrawViewPart.h"

using namespace TechDraw;

PROPERTY_SOURCE_WITH_EXTENSIONS(TechDraw::DrawViewBreak, App::DocumentObject)

const char* DrawViewBreak::BreakTypeEnums[] = {
    QT_TRANSLATE_NOOP("DrawViewBreak", "None"),
    QT_TRANSLATE_NOOP("DrawViewBreak", "ZigZag"),
    QT_TRANSLATE_NOOP("DrawViewBreak", "Simple"),
    QT_TRANSLATE_NOOP("DrawViewBreak", "Sinusoid"),
    nullptr
};

DrawViewBreak::DrawViewBreak()
{
    static constexpr auto group = "Break";

    App::SuppressibleExtension::initExtension(this);

    ADD_PROPERTY_TYPE(StartPoint, (Base::Vector3d()), group, App::Prop_None,
                      "First 3D model point defining the break.");
    ADD_PROPERTY_TYPE(EndPoint, (Base::Vector3d()), group, App::Prop_None,
                      "Second 3D model point defining the break.");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(1.0, 0.0, 0.0)), group, App::Prop_None,
                      "Direction along which the broken view is compressed.");
    ADD_PROPERTY_TYPE(Gap, (10.0), group, App::Prop_None,
                      "Final gap size in millimetres.");
    BreakType.setEnums(BreakTypeEnums);
    ADD_PROPERTY_TYPE(BreakType,
                      (static_cast<long>(TechDraw::BreakType::ZIGZAG)),
                      group,
                      App::Prop_None,
                      "Break-line style.");
}

void DrawViewBreak::onChanged(const App::Property* property)
{
    App::DocumentObject::onChanged(property);

    if (isRestoring()
        || (property != &StartPoint && property != &EndPoint && property != &Direction
            && property != &Gap && property != &BreakType && property != &Suppressed)) {
        return;
    }

    for (auto* parent : getInList()) {
        if (parent && parent->isDerivedFrom<DrawViewPart>()) {
            parent->touch();
        }
    }
}
