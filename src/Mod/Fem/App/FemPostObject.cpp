/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include "FemPostObject.h"
#include <Base/Console.h>
#include <App/DocumentObjectPy.h>
#include <vtkPointData.h>
#include <vtkCellData.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostObject, App::GeoFeature)


FemPostObject::FemPostObject()
{
    ADD_PROPERTY(ModificationTime,(0));
}

FemPostObject::~FemPostObject()
{
}

short FemPostObject::mustExecute(void) const
{
    return 1;
}

DocumentObjectExecReturn* FemPostObject::execute(void) {
    
    //analyse the data and print
    Base::Console().Message("\nPoly Data Analysis:\n");
    
    vtkPolyData*  poly = polyDataSource->GetOutput();
    vtkPointData* point = poly->GetPointData();
    Base::Console().Message("Point components: %i\n", point->GetNumberOfComponents());
    Base::Console().Message("Point arrays: %i\n", point->GetNumberOfArrays());
    Base::Console().Message("Point tuples: %i\n", point->GetNumberOfTuples());
    
    vtkCellData* cell = poly->GetCellData();
    Base::Console().Message("Cell components: %i\n", cell->GetNumberOfComponents());
    Base::Console().Message("Cell arrays: %i\n", cell->GetNumberOfArrays());
    Base::Console().Message("Point tuples: %i\n", cell->GetNumberOfTuples());

    
    if(polyDataSource && static_cast<unsigned long>(ModificationTime.getValue()) < polyDataSource->GetMTime())
        ModificationTime.setValue(static_cast<unsigned long>(polyDataSource->GetMTime()));
    
    return DocumentObject::StdReturn;
}


PyObject *FemPostObject::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

void FemPostObject::onChanged(const Property* prop)
{
    App::GeoFeature::onChanged(prop);

    // if the placement has changed apply the change to the grid data as well

}
