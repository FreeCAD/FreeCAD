/***************************************************************************
 *   Copyright (c) 2024 Martin Rodriguez Reboredo <yakoyoku@gmail.com>     *
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

#include <App/FeaturePythonPyImp.h>

#include "Font.h"
#include "FontPy.h"


using namespace Text;


FC_LOG_LEVEL_INIT("Text", true, true)

PROPERTY_SOURCE(Text::Font, App::DocumentObject)


const char* Font::SourceEnums[] = {"Name", "Path", "Embedded", nullptr};


Font::Font()
{
    ADD_PROPERTY_TYPE(Name, (""), "Font", App::Prop_None, "Font face name");
    ADD_PROPERTY_TYPE(Source, (false), "Font", App::Prop_None, "Font comes from a family name, a file or it's embedded in document");
    Source.setEnums(SourceEnums);
    ADD_PROPERTY_TYPE(File, (""), "Font", App::Prop_None, "Font file path in filesystem");
    ADD_PROPERTY_TYPE(Included, (""), "Font", App::Prop_None, "Font file path in document");
}

Font::~Font() {}

App::DocumentObjectExecReturn* Font::execute() { return App::DocumentObject::execute(); }

short Font::mustExecute() const
{
    if (!isRestoring()) {
        if (Name.isTouched() || Source.isTouched()) {
            return true;
        }
    }
    return App::DocumentObject::mustExecute();
}

PyObject* Font::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        PythonObject = Py::Object(new FontPy(this), true);
    }
    return Py::new_reference_to(PythonObject);
}

void Font::onChanged(const App::Property* prop)
{
    if (prop == &Source) {
        Name.setReadOnly(Source.getValue() == 0);
        File.setReadOnly(Source.getValue() == 1);
        Included.setReadOnly(Source.getValue() == 2);
    }
    App::DocumentObject::onChanged(prop);
}

// Python Font feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Text::FontPython, Text::Font)
template<>
const char* Text::FontPython::getViewProviderName() const
{
    return "TextGui::ViewProviderFontPython";
}
template<>
PyObject* Text::FontPython::getPyObject()
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<FontPy>(this), true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class TextExport FeaturePythonT<Text::Font>;
}// namespace App
