/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef APP_DOCUMENTOBJECTGROUP_H
#define APP_DOCUMENTOBJECTGROUP_H

#include "FeaturePython.h"
#include "DocumentObject.h"
#include "PropertyLinks.h"
#include "GroupExtension.h"
#include <vector>


namespace App
{

class AppExport DocumentObjectGroup : public DocumentObject, public GroupExtension {

    PROPERTY_HEADER_WITH_EXTENSIONS(App::DocumentObjectGroup);

public:
    /// Constructor
    DocumentObjectGroup(void);
    virtual ~DocumentObjectGroup();

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const override{
        return "Gui::ViewProviderDocumentObjectGroup";
    }

    virtual PyObject *getPyObject(void) override;
};

typedef App::FeaturePythonT<DocumentObjectGroup> DocumentObjectGroupPython;


} //namespace App


#endif // APP_DOCUMENTOBJECTGROUP_H
