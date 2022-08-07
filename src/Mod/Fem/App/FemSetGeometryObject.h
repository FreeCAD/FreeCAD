/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#ifndef Fem_FemSetGeometryObject_H
#define Fem_FemSetGeometryObject_H

#include <App/DocumentObject.h>
#include "FemSetObject.h"


namespace Fem
{

class FemExport FemSetGeometryObject : public FemSetObject
{
    PROPERTY_HEADER(Fem::FemSetGeometryObject);

public:
    /// Constructor
    FemSetGeometryObject();
    virtual ~FemSetGeometryObject();

    // returns the type name of the ViewProvider
    virtual const char* getViewProviderName() const {
        return "FemGui::ViewProviderSetGeometry";
    }
    virtual App::DocumentObjectExecReturn *execute() {
        return App::DocumentObject::StdReturn;
    }
    virtual short mustExecute() const;
    virtual PyObject *getPyObject();


};

} //namespace Fem


#endif // Fem_FemSetGeometryObject_H
