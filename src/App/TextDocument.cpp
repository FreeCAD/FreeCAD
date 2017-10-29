/***************************************************************************
 *   Copyright (c) 2017 Markus Hovorka <m.hovorka@live.de>                 *
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

#include <string>
#include <fstream>
#include <boost/signals2.hpp>

#include "DocumentObject.h"
#include "TextDocument.h"


using namespace App;

PROPERTY_SOURCE(App::TextDocument, App::DocumentObject)

TextDocument::TextDocument()
{
    ADD_PROPERTY_TYPE(
            Text, (""), 0, App::Prop_Hidden,
            "Content of the document.");
    ADD_PROPERTY_TYPE(
            ReadOnly, (false), 0, App::Prop_None,
            "Defines whether the content can be edited.");
}

void TextDocument::onChanged(const Property* prop)
{
    if (prop == &Text)
        textChanged();
    DocumentObject::onChanged(prop);
}

const char* TextDocument::getViewProviderName() const
{
    return "Gui::ViewProviderTextDocument";
}

boost::signals2::connection TextDocument::connect(const TextSlot &sub)
{
    return textChanged.connect(sub);
}
