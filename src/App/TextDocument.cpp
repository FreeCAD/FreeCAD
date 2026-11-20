// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2017 Markus Hovorka
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



#include "TextDocument.h"
#include "DocumentObject.h"


using namespace App;

PROPERTY_SOURCE(App::TextDocument, App::DocumentObject)

TextDocument::TextDocument()
{
    ADD_PROPERTY_TYPE(Text, (""), 0, App::Prop_Hidden, "Content of the document.");
}

void TextDocument::onChanged(const Property* prop)
{
    if (prop == &Text) {
        textChanged();
    }
    else if (prop == &Label) {
        labelChanged();
    }
    DocumentObject::onChanged(prop);
}

const char* TextDocument::getViewProviderName() const
{
    return "Gui::ViewProviderTextDocument";
}

fastsignals::advanced_connection TextDocument::connectText(const TextSlot& sub)
{
    return textChanged.connect(sub, fastsignals::advanced_tag());
}

fastsignals::connection TextDocument::connectLabel(const TextSlot& sub)
{
    return labelChanged.connect(sub);
}
