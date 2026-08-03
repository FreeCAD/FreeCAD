/***************************************************************************
 *   Copyright (c) 2023 Peter McB                                          *
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 ***************************************************************************/


#pragma once

#include "FemSetObject.h"
#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>

namespace Fem
{

class FemExport FemSetElementNodesObject: public FemSetObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemSetElementNodesObject);

public:
    /// Constructor
    FemSetElementNodesObject();
    ~FemSetElementNodesObject() override;

    App::PropertyIntegerSet Elements;

    // returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderSetElementNodes";
    }
    App::DocumentObjectExecReturn* execute() override
    {
        return App::DocumentObject::StdReturn;
    }
    short mustExecute() const override;
    PyObject* getPyObject() override;
    static std::string getElementName()
    {
        return "ElementsSet";
    }
};

}  // namespace Fem
