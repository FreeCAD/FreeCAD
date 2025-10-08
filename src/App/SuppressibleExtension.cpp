/***************************************************************************
 *   Copyright (c) 2024 Florian Foinant-Willig <ffw@2f2v.fr>               *
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


#include <Base/Tools.h>

#include "Extension.h"
#include "SuppressibleExtension.h"
#include "SuppressibleExtensionPy.h"


namespace App
{

EXTENSION_PROPERTY_SOURCE(App::SuppressibleExtension, App::DocumentObjectExtension)


EXTENSION_PROPERTY_SOURCE_TEMPLATE(App::SuppressibleExtensionPython, App::SuppressibleExtension)

// explicit template instantiation
template class AppExport ExtensionPythonT<SuppressibleExtensionPythonT<SuppressibleExtension>>;


SuppressibleExtension::SuppressibleExtension()
{
    initExtensionType(SuppressibleExtension::getExtensionClassTypeId());
    EXTENSION_ADD_PROPERTY_TYPE(Suppressed,
                                (false),
                                "Base",
                                PropertyType(Prop_None),
                                "Is object suppressed");
}

SuppressibleExtension::~SuppressibleExtension() = default;

PyObject* SuppressibleExtension::getExtensionPyObject()
{

    if (ExtensionPythonObject.is(Py::_None())) {
        // ref counter is set to 1
        auto ext = new SuppressibleExtensionPy(this);
        ExtensionPythonObject = Py::Object(ext, true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}

}  // namespace App
