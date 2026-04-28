// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include "Extension.h"

namespace Base
{
class Matrix4D;
}
namespace App
{
class DocumentObject;
class DocumentObjectExecReturn;

/**
 * @brief Extension with special document object calls
 *
 */
class AppExport DocumentObjectExtension: public App::Extension
{

    // The cass does not have properties itself, but it is important to provide the property access
    // functions. see cpp file for details
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::DocumentObjectExtension);

public:
    DocumentObjectExtension();
    ~DocumentObjectExtension() override;

    App::DocumentObject* getExtendedObject();
    const App::DocumentObject* getExtendedObject() const;

    // override if execution is necessary
    virtual short extensionMustExecute();
    virtual App::DocumentObjectExecReturn* extensionExecute();


    /// get called after setting the document
    virtual void onExtendedSettingDocument();
    /// get called after a document has been fully restored
    virtual void onExtendedDocumentRestored();
    /// get called after a brand new object was created
    virtual void onExtendedSetupObject();
    /// get called when object is going to be removed from the document
    virtual void onExtendedUnsetupObject();

    PyObject* getExtensionPyObject() override;

    /// returns the type name of the ViewProviderExtension which is automatically attached
    /// to the viewprovider object when it is initiated
    virtual const char* getViewProviderExtensionName() const
    {
        return "";
    }

    /** Get the sub object by name
     * @sa DocumentObject::getSubObject()
     *
     * @return Return turn if handled, the sub object is returned in \c ret
     */
    virtual bool extensionGetSubObject(DocumentObject*& ret,
                                       const char* subname,
                                       PyObject** pyObj,
                                       Base::Matrix4D* mat,
                                       bool transform,
                                       int depth) const;

    /** Get name references of all sub objects
     * @sa DocumentObject::getSubObjects()
     *
     * @return Return turn if handled, the sub object is returned in \c ret
     */
    virtual bool extensionGetSubObjects(std::vector<std::string>& ret, int reason) const;

    /**
     * @brief Get the linked object of this extension.
     *
     * This method returns the linked object of this document object.  If there
     * is no linked object, it will return the container object of the
     * extension.
     *
     * @param[out] ret The linked object is returned in this parameter.
     *
     * @param[in] recurse If true, it will recursively resolve the link until it
     * reaches the final linked object.
     *
     * @param mat[in,out] If non-null, it is used as the current transformation
     * matrix on input. On output it is used as the accumulated transformation
     * up until the final linked object.
     *
     * @param[in] transform If false, then it will not accumulate the object's own
     * placement into @p mat, which lets you override the object's placement.
     *
     * @param[in] depth This parameter indicates the level on which we are
     * resolving the link.
     *
     * @return Returns true if the linked object is successfully retrieved and
     * returned in @p ret. If the linked object is not found or is invalid, it
     * returns false.
     */
    virtual bool extensionGetLinkedObject(DocumentObject*& ret,
                                          bool recursive,
                                          Base::Matrix4D* mat,
                                          bool transform,
                                          int depth) const;

    virtual int extensionSetElementVisible(const char*, bool)
    {
        return -1;
    }
    virtual int extensionIsElementVisible(const char*)
    {
        return -1;
    }
    virtual bool extensionHasChildElement() const
    {
        return false;
    }
};

}  // namespace App
