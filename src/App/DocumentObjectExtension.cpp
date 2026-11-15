// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2016 Stefan Tröger
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

#include "DocumentObjectExtension.h"
#include "DocumentObjectExtensionPy.h"
#include "DocumentObject.h"
#include "ExtensionContainer.h"

using namespace App;

EXTENSION_PROPERTY_SOURCE(App::DocumentObjectExtension, App::Extension)

DocumentObjectExtension::DocumentObjectExtension()
{
    initExtensionType(App::DocumentObjectExtension::getExtensionClassTypeId());
}

DocumentObjectExtension::~DocumentObjectExtension() = default;

short int DocumentObjectExtension::extensionMustExecute()
{

    return 0;
}

App::DocumentObjectExecReturn* DocumentObjectExtension::extensionExecute()
{

    return App::DocumentObject::StdReturn;
}

void DocumentObjectExtension::onExtendedSettingDocument()
{}

void DocumentObjectExtension::onExtendedDocumentRestored()
{}

void DocumentObjectExtension::onExtendedSetupObject()
{}

void DocumentObjectExtension::onExtendedUnsetupObject()
{}

PyObject* DocumentObjectExtension::getExtensionPyObject()
{

    if (ExtensionPythonObject.is(Py::_None())) {
        // ref counter is set to 1
        ExtensionPythonObject = Py::Object(new DocumentObjectExtensionPy(this), true);
    }
    return Py::new_reference_to(ExtensionPythonObject);
}

const DocumentObject* DocumentObjectExtension::getExtendedObject() const
{

    assert(getExtendedContainer()->isDerivedFrom<DocumentObject>());
    return static_cast<const DocumentObject*>(getExtendedContainer());
}

DocumentObject* DocumentObjectExtension::getExtendedObject()
{

    assert(getExtendedContainer()->isDerivedFrom<DocumentObject>());
    return static_cast<DocumentObject*>(getExtendedContainer());
}

bool DocumentObjectExtension::extensionGetSubObject(DocumentObject*&,
                                                    const char*,
                                                    PyObject**,
                                                    Base::Matrix4D*,
                                                    bool,
                                                    int) const
{
    return false;
}

bool DocumentObjectExtension::extensionGetSubObjects(std::vector<std::string>&, int) const
{
    return false;
}

bool DocumentObjectExtension::extensionGetLinkedObject(DocumentObject*&,
                                                       bool,
                                                       Base::Matrix4D*,
                                                       bool,
                                                       int) const
{
    return false;
}
