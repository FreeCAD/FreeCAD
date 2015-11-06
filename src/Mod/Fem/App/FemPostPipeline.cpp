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
#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <vtkDataSetReader.h>
#include <vtkGeometryFilter.h>
#include <vtkPointData.h>
#include <vtkStructuredGrid.h>
#include <vtkCellData.h>
#include <vtkUnstructuredGrid.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostPipeline, Fem::FemPostObject)
const char* FemPostPipeline::ModeEnums[]= {"Serial","Parallel",NULL};

FemPostPipeline::FemPostPipeline()
{
    ADD_PROPERTY_TYPE(Filter, (0), "Pipeline", App::Prop_None, "The filter used in in this pipeline");
    ADD_PROPERTY_TYPE(Function, (0), "Pipeline", App::Prop_Hidden, "The function provider which groups all pipeline functions");
    ADD_PROPERTY_TYPE(Mode,(long(0)), "Pipeline", App::Prop_None, "Selects the pipeline data transition mode. In serial every filter" 
                                                              "gets the output of the previous one as input, in parrallel every"
                                                              "filter gets the pipelien source as input.");     
    Mode.setEnums(ModeEnums);
    
    source = vtkUnstructuredGrid::New();
}

FemPostPipeline::~FemPostPipeline()
{
}

short FemPostPipeline::mustExecute(void) const
{
    return 1;
}

DocumentObjectExecReturn* FemPostPipeline::execute(void) {

//     Base::Console().Message("Pipeline analysis: \n");
//     Base::Console().Message("Data Type: %i\n", source->GetDataObjectType());
// 
//     if(source->GetDataObjectType() == VTK_STRUCTURED_GRID  ) {
//         vtkStructuredGrid*  poly = static_cast<vtkStructuredGrid*>(source.GetPointer());
//         vtkPointData* point = poly->GetPointData();
//         Base::Console().Message("Point components: %i\n", point->GetNumberOfComponents());
//         Base::Console().Message("Point arrays: %i\n", point->GetNumberOfArrays());
//         Base::Console().Message("Point tuples: %i\n", point->GetNumberOfTuples());
//         
//         vtkCellData* cell = poly->GetCellData();
//         Base::Console().Message("Cell components: %i\n", cell->GetNumberOfComponents());
//         Base::Console().Message("Cell arrays: %i\n", cell->GetNumberOfArrays());
//         Base::Console().Message("Point tuples: %i\n", cell->GetNumberOfTuples());
//     }
    
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


// PyObject *FemPostPipeline::getPyObject()
// {
//     if (PythonObject.is(Py::_None())){
//         // ref counter is set to 1
//         PythonObject = Py::Object(new DocumentObjectPy(this),true);
//     }
//     return Py::new_reference_to(PythonObject); 
// }

void FemPostPipeline::onChanged(const Property* prop)
{
    if(prop == &Filter || prop == &Mode) {
        
        //we check if all connections are right and add new ones if needed
        std::vector<App::DocumentObject*> objs = Filter.getValues();
        
        if(objs.empty())
            return;
        
        std::vector<App::DocumentObject*>::iterator it = objs.begin();
        FemPostFilter* filter = static_cast<FemPostFilter*>(*it);
        
        //the first one is always connected to the pipeline
        if(!filter->hasInputDataConnected() || filter->getConnectedInputData() != getSource())
            filter->connectInputData(getSource());
        
        //all the others need to be connected to the previous filter or the source, dependend on the mode
        ++it;
        for(; it != objs.end(); ++it) {
            FemPostFilter* nextFilter = static_cast<FemPostFilter*>(*it);
            
            if(Mode.getValue() == 0) {
                if(!nextFilter->hasInputAlgorithmConnected() || nextFilter->getConnectedInputAlgorithm() != filter->getOutputAlgorithm())
                    nextFilter->connectInputAlgorithm(filter->getOutputAlgorithm());
            }
            else {
                if(!nextFilter->hasInputDataConnected() || nextFilter->getConnectedInputData() != getSource())
                    nextFilter->connectInputData(getSource());
            }
            
            filter = nextFilter;
        };
    }
    
    App::GeoFeature::onChanged(prop);

}

FemPostObject* FemPostPipeline::getLastPostObject() {

    if(Filter.getValues().empty())
        return this;
    
    return static_cast<FemPostObject*>(Filter.getValues().back());
}
