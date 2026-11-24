// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2008 Werner Mayer
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/



#include "Annotation.h"

using namespace App;

PROPERTY_SOURCE(App::Annotation, App::DocumentObject)


Annotation::Annotation()
{
    ADD_PROPERTY(LabelText, (""));
    ADD_PROPERTY(Position, (Base::Vector3d()));
}

Annotation::~Annotation() = default;

// --------------------------------------------------

PROPERTY_SOURCE(App::AnnotationLabel, App::DocumentObject)


AnnotationLabel::AnnotationLabel()
{
    ADD_PROPERTY_TYPE(LabelText, (""), "Label", Prop_Output, "Text label of the annotation");
    ADD_PROPERTY_TYPE(BasePosition, (Base::Vector3d()), "Label", Prop_Output, "Base position");
    ADD_PROPERTY_TYPE(TextPosition, (Base::Vector3d()), "Label", Prop_Output, "Text position");
}

AnnotationLabel::~AnnotationLabel() = default;
