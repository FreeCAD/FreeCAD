/***************************************************************************
 *   Copyright (c) Stefan Tröger          (stefantroeger@gmx.net) 2016     *
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


#ifndef APP_DOCUMENTOBJECTEXTENSION_H
#define APP_DOCUMENTOBJECTEXTENSION_H

#include "Extension.h"
#include "DocumentObject.h"

namespace App {
    
/**
 * @brief Extension with special document object calls
 * 
 */
class AppExport DocumentObjectExtension : public App::Extension
{

    //The cass does not have properties itself, but it is important to provide the property access
    //functions. see cpp file for details
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::DocumentObjectExtension );

public:

    DocumentObjectExtension ();
    virtual ~DocumentObjectExtension ();

    App::DocumentObject*       getExtendedObject();
    const App::DocumentObject* getExtendedObject() const;
  
    //override if execution is necessary
    virtual short extensionMustExecute(void);
    virtual App::DocumentObjectExecReturn *extensionExecute(void);
    
    
    /// get called after setting the document
    virtual void onExtendedSettingDocument();
    /// get called after a document has been fully restored
    virtual void onExtendedDocumentRestored();
    /// get called after a brand new object was created
    virtual void onExtendedSetupObject();
    /// get called when object is going to be removed from the document
    virtual void onExtendedUnsetupObject();

    virtual PyObject* getExtensionPyObject(void) override;
    
    /// returns the type name of the ViewProviderExtension which is automatically attached 
    /// to the viewprovider object when it is initiated
    virtual const char* getViewProviderExtensionName(void) const {return "";}

    /** Get the sub object by name
     * @sa DocumentObject::getSubObject()
     *
     * @return Return turn if handled, the sub object is returned in \c ret
     */
    virtual bool extensionGetSubObject( DocumentObject *&ret, const char *subname, 
        PyObject **pyObj, Base::Matrix4D *mat, bool transform, int depth) const;

    /** Get name references of all sub objects
     * @sa DocumentObject::getSubObjects()
     *
     * @return Return turn if handled, the sub object is returned in \c ret
     */
    virtual bool extensionGetSubObjects(std::vector<std::string> &ret, int reason) const;

    /** Get the linked object
     *  @sa DocumentObject::getLinkedObject()
     *
     * @return Return turn if handled, the linked object is returned in \c ret
     */
    virtual bool extensionGetLinkedObject(DocumentObject *&ret, bool recursive,
            Base::Matrix4D *mat, bool transform, int depth) const;

    virtual int extensionSetElementVisible(const char *, bool) {return -1;}
    virtual int extensionIsElementVisible(const char *) {return -1;}
    virtual bool extensionHasChildElement() const {return false;}
};

} //App

#endif // APP_DOCUMENTOBJECTEXTENSION_H
