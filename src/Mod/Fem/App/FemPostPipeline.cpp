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
# include <SMESH_Mesh.hxx>
# include <vtkDataSetReader.h>
# include <vtkGeometryFilter.h>
# include <vtkStructuredGrid.h>
# include <vtkUnstructuredGrid.h>
# include <vtkImageData.h>
# include <vtkRectilinearGrid.h>
# include <vtkAppendFilter.h>
# include <vtkXMLUnstructuredGridReader.h>
# include <vtkXMLPolyDataReader.h>
# include <vtkXMLStructuredGridReader.h>
# include <vtkXMLRectilinearGridReader.h>
# include <vtkXMLImageDataReader.h>
#endif

#include "FemPostPipeline.h"
#include "FemMesh.h"
#include "FemMeshObject.h"
#include "FemVTKTools.h"

#include <Base/Console.h>
#include <App/Document.h>

#include <App/DocumentObjectPy.h>
#include <Mod/Fem/App/FemPostPipelinePy.h>


using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostPipeline, Fem::FemPostObject)
const char* FemPostPipeline::ModeEnums[]= {"Serial","Parallel",NULL};

FemPostPipeline::FemPostPipeline()
{
    ADD_PROPERTY_TYPE(Filter, (0), "Pipeline", App::Prop_None, "The filter used in in this pipeline");
    ADD_PROPERTY_TYPE(Functions, (0), "Pipeline", App::Prop_Hidden, "The function provider which groups all pipeline functions");
    ADD_PROPERTY_TYPE(Mode,(long(0)), "Pipeline", App::Prop_None, "Selects the pipeline data transition mode. In serial every filter"
                                                              "gets the output of the previous one as input, in parallel every"
                                                              "filter gets the pipelien source as input.");
    Mode.setEnums(ModeEnums);
}

FemPostPipeline::~FemPostPipeline()
{
}

short FemPostPipeline::mustExecute(void) const
{
    if(Mode.isTouched())
        return 1;

    return FemPostFilter::mustExecute();
}

DocumentObjectExecReturn* FemPostPipeline::execute(void) {

    //if we are the toplevel pipeline our data object is not created by filters, we are the main source!
    if(!Input.getValue())
        return StdReturn;

    //now if we are a filter than our data object is created by the filter we hold

    //if we are in serial mode we just copy over the data of the last filter,
    //but if we are in parallel we need to combine all filter results
    if(Mode.getValue() == 0) {

        //serial
        Data.setValue(getLastPostObject()->Data.getValue());
    }
    else {

        //parallel. go through all filters and append the result
        const std::vector<App::DocumentObject*>& filters = Filter.getValues();
        std::vector<App::DocumentObject*>::const_iterator it = filters.begin();

        vtkSmartPointer<vtkAppendFilter> append = vtkSmartPointer<vtkAppendFilter>::New();
        for(;it != filters.end(); ++it) {

            append->AddInputDataObject(static_cast<FemPostObject*>(*it)->Data.getValue());
        }

        append->Update();
        Data.setValue(append->GetOutputDataObject(0));
    }


    return Fem::FemPostObject::execute();
}


bool FemPostPipeline::canRead(Base::FileInfo File) {

    if (File.hasExtension("vtk") ||
        // from FemResult only unstructural mesh is supported in femvtktoools.cpp
        File.hasExtension("vtp") ||
        File.hasExtension("vts") ||
        File.hasExtension("vtr") ||
        File.hasExtension("vti") ||
        File.hasExtension("vtu"))
        return true;

    return false;
}

void FemPostPipeline::read(Base::FileInfo File) {

    // checking on the file
    if (!File.isReadable())
        throw Base::FileException("File to load not existing or not readable", File);

    if (File.hasExtension("vtu"))
        readXMLFile<vtkXMLUnstructuredGridReader>(File.filePath());
    else if (File.hasExtension("vtp"))
        readXMLFile<vtkXMLPolyDataReader>(File.filePath());
    else if (File.hasExtension("vts"))
        readXMLFile<vtkXMLStructuredGridReader>(File.filePath());
    else if (File.hasExtension("vtr"))
        readXMLFile<vtkXMLRectilinearGridReader>(File.filePath());
    else if (File.hasExtension("vti"))
        readXMLFile<vtkXMLImageDataReader>(File.filePath());
    else if (File.hasExtension("vtk"))
        readXMLFile<vtkDataSetReader>(File.filePath());
    else
        throw Base::FileException("Unknown extension");
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

        //If we have a Input we need to ensure our filters are connected correctly
        if(Input.getValue()) {

            //the first filter is always connected to the input
            if(filter->Input.getValue() != Input.getValue())
                filter->Input.setValue(Input.getValue());

            //all the others need to be connected to the previous filter or the source, dependent on the mode
            ++it;
            for(; it != objs.end(); ++it) {
                FemPostFilter* nextFilter = static_cast<FemPostFilter*>(*it);

                if(Mode.getValue() == 0) { //serial mode
                    if( nextFilter->Input.getValue() != filter)
                        nextFilter->Input.setValue(filter);
                }
                else { //Parallel mode
                    if( nextFilter->Input.getValue() != Input.getValue())
                        nextFilter->Input.setValue(Input.getValue());
                }

                filter = nextFilter;
            };
        }
        //if we have no input the filters are responsible of grabbing the pipeline data themself
        else {
            //the first filter must always grab the data
            if(filter->Input.getValue() != NULL)
                filter->Input.setValue(NULL);

            //all the others need to be connected to the previous filter or grab the data, dependent on mode
            ++it;
            for(; it != objs.end(); ++it) {
                FemPostFilter* nextFilter = static_cast<FemPostFilter*>(*it);

                if(Mode.getValue() == 0) { //serial mode
                    if( nextFilter->Input.getValue() != filter)
                        nextFilter->Input.setValue(filter);
                }
                else { //Parallel mode
                    if( nextFilter->Input.getValue() != NULL)
                        nextFilter->Input.setValue(NULL);
                }

                filter = nextFilter;
            };
        }
    }

    App::GeoFeature::onChanged(prop);

}

FemPostObject* FemPostPipeline::getLastPostObject() {

    if(Filter.getValues().empty())
        return this;

    return static_cast<FemPostObject*>(Filter.getValues().back());
}

bool FemPostPipeline::holdsPostObject(FemPostObject* obj) {

    std::vector<App::DocumentObject*>::const_iterator it = Filter.getValues().begin();
    for(; it != Filter.getValues().end(); ++it) {

        if(*it == obj)
            return true;
    }
    return false;
}

void FemPostPipeline::load(FemResultObject* res) {
    if(!res->Mesh.getValue() || !res->Mesh.getValue()->isDerivedFrom(Fem::FemMeshObject::getClassTypeId())) {
        Base::Console().Warning("Mesh of result object is empty or not derived from Fem::FemMeshObject\n");
        return;
    }

    //first copy the mesh over
    //########################
    const FemMesh& mesh = static_cast<FemMeshObject*>(res->Mesh.getValue())->FemMesh.getValue();
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    FemVTKTools::exportVTKMesh(&mesh, grid);

    //Now copy the point data over
    //############################
    FemVTKTools::exportFreeCADResult(res, grid);

    Data.setValue(grid);
}

PyObject* FemPostPipeline::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FemPostPipelinePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
