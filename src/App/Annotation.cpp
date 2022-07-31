/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#include "PreCompiled.h"

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
    ADD_PROPERTY_TYPE(LabelText, (""), "Label",Prop_Output, "Text label of the annotation");
    ADD_PROPERTY_TYPE(BasePosition, (Base::Vector3d()), "Label", Prop_Output, "Base position");
    ADD_PROPERTY_TYPE(TextPosition, (Base::Vector3d()), "Label", Prop_Output, "Text position");
}

AnnotationLabel::~AnnotationLabel() = default;
