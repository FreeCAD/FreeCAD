/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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

#ifndef _PreComp_
# include <cassert>
# include <algorithm>
#endif

#include "ViewProviderExtension.h"
#include "ViewProviderExtensionPy.h"
#include "ViewProviderExtensionPython.h"
#include "ViewProviderDocumentObject.h"

using namespace Gui;

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderExtension, App::Extension)

ViewProviderExtension::ViewProviderExtension()
{
    initExtensionType(Gui::ViewProviderExtension::getExtensionClassTypeId());
}

ViewProviderExtension::~ViewProviderExtension() = default;

const ViewProviderDocumentObject* ViewProviderExtension::getExtendedViewProvider() const{

    assert(getExtendedContainer()->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()));
    return static_cast<const ViewProviderDocumentObject*>(getExtendedContainer());
}

ViewProviderDocumentObject* ViewProviderExtension::getExtendedViewProvider() {

    assert(getExtendedContainer()->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()));
    return static_cast<ViewProviderDocumentObject*>(getExtendedContainer());
}

void ViewProviderExtension::extensionUpdateData(const App::Property*) {

}

PyObject* ViewProviderExtension::getExtensionPyObject() {

    if (ExtensionPythonObject.is(Py::_None())){
        // ref counter is set to 1
        auto ext = new ViewProviderExtensionPy(this);
        ExtensionPythonObject = Py::asObject(ext);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}

namespace Gui {
EXTENSION_PROPERTY_SOURCE_TEMPLATE(Gui::ViewProviderExtensionPython, Gui::ViewProviderExtension)

// explicit template instantiation
template class GuiExport ViewProviderExtensionPythonT<ViewProviderExtension>;
}
