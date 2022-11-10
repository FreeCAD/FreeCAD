/***************************************************************************
 *   Copyright (c) 2022 Peter McB                                          *
 *                                                                         *
 *   based on: FemSetNodesObject.h                                         *
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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


#ifndef Fem_FemSetElementNodesObject_H
#define Fem_FemSetElementNodesObject_H

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include "FemSetObject.h"

namespace Fem
{

class FemExport FemSetElementNodesObject : public FemSetObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemSetElementNodesObject);
//    PROPERTY_HEADER(Fem::FemSetElementNodesObject);

public:
    /// Constructor
    FemSetElementNodesObject();
    ~FemSetElementNodesObject() override;
//    virtual ~FemSetElementNodesObject();

    App::PropertyIntegerSet Elements;
//    App::PropertyFloatList FloatNodes;

    // returns the type name of the ViewProvider
//    virtual const char* getViewProviderName(void) const {
    const char* getViewProviderName() const override {
        return "FemGui::ViewProviderSetElementNodes";
    }
//    virtual App::DocumentObjectExecReturn *execute(void) {
    App::DocumentObjectExecReturn *execute() override {
        return App::DocumentObject::StdReturn;
    }
//    virtual short mustExecute(void) const;
//    virtual PyObject *getPyObject(void);
    short mustExecute() const override;
    PyObject *getPyObject() override;
    static std::string elementsName; //  = "ElementsSet"; 
    static std::string uniqueElementsName; // "ElementsSet" latest name


};

} //namespace Fem


#endif // Fem_FemSetElementNodesObject_H
