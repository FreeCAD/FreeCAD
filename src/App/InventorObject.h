/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef APP_INVENTOROBJECT_H
#define APP_INVENTOROBJECT_H

#include "GeoFeature.h"
#include "PropertyStandard.h"


namespace App
{

class AppExport InventorObject : public GeoFeature
{
    PROPERTY_HEADER(App::InventorObject);

public:
    /// Constructor
    InventorObject();
    ~InventorObject() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "Gui::ViewProviderInventorObject";
    }
    DocumentObjectExecReturn *execute() override {
        return DocumentObject::StdReturn;
    }
    short mustExecute() const override;
    PyObject *getPyObject() override;

    PropertyString Buffer;
    PropertyString FileName;
};

} //namespace App


#endif // APP_INVENTOROBJECT_H
