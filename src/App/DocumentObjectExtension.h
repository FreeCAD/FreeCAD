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
    PROPERTY_HEADER(App::DocumentObjectExtension );

public:

    DocumentObjectExtension ();
    virtual ~DocumentObjectExtension ();

    //override if execution is nesseccary
    virtual short extensionMustExecute(void) const;
    virtual App::DocumentObjectExecReturn *extensionExecute(void);
    
    
    /// get called after setting the document
    virtual void onExtendedSettingDocument();
    /// get called after a brand new object was created
    virtual void onExtendedSetupObject();
    /// get called when object is going to be removed from the document
    virtual void onExtendedUnsetupObject();
  
    virtual PyObject* getExtensionPyObject(void);
};

} //App

#endif // APP_DOCUMENTOBJECTEXTENSION_H
