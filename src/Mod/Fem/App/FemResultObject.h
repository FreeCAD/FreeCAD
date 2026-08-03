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

#pragma once

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <Mod/Fem/FemGlobal.h>


namespace Fem
{
/// Father of all result data in a Fem Analysis
class FemExport FemResultObject: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemResultObject);

public:
    /// Constructor
    FemResultObject();
    ~FemResultObject() override;

    App::PropertyIntegerList NodeNumbers;
    /// Link to the corresponding mesh
    App::PropertyLink Mesh;
    /// Stats of analysis
    App::PropertyFloat Time;
    /// User defined results
    App::PropertyFloatList Stats;
    /// Displacement vectors of analysis

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "FemGui::ViewProviderResult";
    }
    App::DocumentObjectExecReturn* execute() override
    {
        return App::DocumentObject::StdReturn;
    }
    short mustExecute() const override;
    PyObject* getPyObject() override;
};

using FemResultObjectPython = App::FeaturePythonT<FemResultObject>;


}  // namespace Fem
