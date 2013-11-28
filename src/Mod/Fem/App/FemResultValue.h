/***************************************************************************
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


#ifndef Fem_FemResultValue_H
#define Fem_FemResultValue_H


#include <App/PropertyStandard.h>
#include <App/FeaturePython.h>

#include "FemResultObject.h"

namespace Fem
{
/// Father of all result data in a Fem Analysis
class AppFemExport FemResultValue : public Fem::FemResultObject
{
    PROPERTY_HEADER(Fem::FemResultValue);

public:
    /// Constructor
    FemResultValue(void);
    virtual ~FemResultValue();

    /// List of values
    App::PropertyFloatList Values;

    /// returns the type name of the ViewProvider
    //virtual const char* getViewProviderName(void) const {
    //    return "FemGui::ViewProviderFemSet";
    //}
    virtual App::DocumentObjectExecReturn *execute(void) {
        return App::DocumentObject::StdReturn;
    }
    virtual short mustExecute(void) const;
    virtual PyObject *getPyObject(void);


};

typedef App::FeaturePythonT<FemResultValue> FemResultValuePython;

} //namespace Fem


#endif // Fem_FemResultValue_H
