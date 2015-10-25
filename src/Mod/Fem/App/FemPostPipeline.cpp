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

#include "FemPostPipeline.h"
#include <Base/Console.h>
#include <App/DocumentObjectPy.h>
#include <vtkDataSetReader.h>
#include <vtkGeometryFilter.h>
#include <vtkPointData.h>
#include <vtkStructuredGrid.h>
#include <vtkCellData.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostPipeline, Fem::FemPostObject)


FemPostPipeline::FemPostPipeline()
{
}

FemPostPipeline::~FemPostPipeline()
{
}

short FemPostPipeline::mustExecute(void) const
{

    return 1;
}

DocumentObjectExecReturn* FemPostPipeline::execute(void) {

    Base::Console().Message("Pipeline analysis: \n");
    Base::Console().Message("Data Type: %i\n", source->GetDataObjectType());

    if(source->GetDataObjectType() == VTK_STRUCTURED_GRID  ) {
        vtkStructuredGrid*  poly = static_cast<vtkStructuredGrid*>(source.GetPointer());
        vtkPointData* point = poly->GetPointData();
        Base::Console().Message("Point components: %i\n", point->GetNumberOfComponents());
        Base::Console().Message("Point arrays: %i\n", point->GetNumberOfArrays());
        Base::Console().Message("Point tuples: %i\n", point->GetNumberOfTuples());
        
        vtkCellData* cell = poly->GetCellData();
        Base::Console().Message("Cell components: %i\n", cell->GetNumberOfComponents());
        Base::Console().Message("Cell arrays: %i\n", cell->GetNumberOfArrays());
        Base::Console().Message("Point tuples: %i\n", cell->GetNumberOfTuples());
    }
    
    return Fem::FemPostObject::execute();
}


bool FemPostPipeline::canRead(Base::FileInfo File) {

    if (File.hasExtension("vtk") )
        return true;
    
    return false;
}


void FemPostPipeline::read(Base::FileInfo File) {
   
    // checking on the file
    if (!File.isReadable())
        throw Base::Exception("File to load not existing or not readable");
    
    if (File.hasExtension("vtk") ) {
        
        vtkSmartPointer<vtkDataSetReader> reader = vtkSmartPointer<vtkDataSetReader>::New();
        reader->SetFileName(File.filePath().c_str());
        reader->Update();
        source = reader->GetOutput();
        
    }
    else{
        throw Base::Exception("Unknown extension");
    }
    
    polyDataSource = vtkGeometryFilter::New();
    polyDataSource->SetInputData(source);
    polyDataSource->Update();
}


PyObject *FemPostPipeline::getPyObject()
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new DocumentObjectPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

void FemPostPipeline::onChanged(const Property* prop)
{
    App::GeoFeature::onChanged(prop);

    // if the placement has changed apply the change to the grid data as well

}
