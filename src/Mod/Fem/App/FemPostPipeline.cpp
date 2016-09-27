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
#include "FemMesh.h"
#include "FemMeshObject.h"
#include <Base/Console.h>
#include <App/Document.h>
#include <SMESH_Mesh.hxx>
#include <App/DocumentObjectPy.h>
#include <vtkDataSetReader.h>
#include <vtkGeometryFilter.h>
#include <vtkPointData.h>
#include <vtkStructuredGrid.h>
#include <vtkCellData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkTetra.h>
#include <vtkQuadraticTetra.h>
#include <vtkTriangle.h>
#include <vtkQuadraticTriangle.h>
#include <vtkQuad.h>
#include <vtkImageData.h>
#include <vtkRectilinearGrid.h>
#include <vtkAppendFilter.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLPolyDataReader.h>
#include <vtkXMLStructuredGridReader.h>
#include <vtkXMLRectilinearGridReader.h>
#include <vtkXMLImageDataReader.h>

using namespace Fem;
using namespace App;

PROPERTY_SOURCE(Fem::FemPostPipeline, Fem::FemPostObject)
const char* FemPostPipeline::ModeEnums[]= {"Serial","Parallel",NULL};

FemPostPipeline::FemPostPipeline()
{
    ADD_PROPERTY_TYPE(Filter, (0), "Pipeline", App::Prop_None, "The filter used in in this pipeline");
    ADD_PROPERTY_TYPE(Functions, (0), "Pipeline", App::Prop_Hidden, "The function provider which groups all pipeline functions");
    ADD_PROPERTY_TYPE(Mode,(long(0)), "Pipeline", App::Prop_None, "Selects the pipeline data transition mode. In serial every filter"
                                                              "gets the output of the previous one as input, in parrallel every"
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
        File.hasExtension("vtp") ||
        File.hasExtension("vts") ||
        File.hasExtension("vtr") ||
        File.hasExtension("vtu") ||
        File.hasExtension("vti"))
        return true;

    return false;
}

void FemPostPipeline::read(Base::FileInfo File) {

    // checking on the file
    if (!File.isReadable())
        throw Base::Exception("File to load not existing or not readable");

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
        throw Base::Exception("Unknown extension");
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

            //all the others need to be connected to the previous filter or the source, dependend on the mode
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

            //all the others need to be connected to the previous filter or grab the data, dependend on mode
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

    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();

    //first copy the mesh over
    //########################

    if(!res->Mesh.getValue() || !res->Mesh.getValue()->isDerivedFrom(Fem::FemMeshObject::getClassTypeId()))
        return;

    const FemMesh& mesh = static_cast<FemMeshObject*>(res->Mesh.getValue())->FemMesh.getValue();
    SMESH_Mesh* smesh = const_cast<SMESH_Mesh*>(mesh.getSMesh());
    SMESHDS_Mesh* meshDS = smesh->GetMeshDS();
    const SMDS_MeshInfo& info = meshDS->GetMeshInfo();

    //start with the nodes
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    SMDS_NodeIteratorPtr aNodeIter = meshDS->nodesIterator();

    points->SetNumberOfPoints(info.NbNodes());
    for(; aNodeIter->more(); ) {
        const SMDS_MeshNode* node = aNodeIter->next();
        float coords[3] = {float(node->X()), float(node->Y()), float(node->Z())};
        points->SetPoint(node->GetID()-1, coords);
    }
    grid->SetPoints(points);

    //start with 2d elements
    vtkSmartPointer<vtkCellArray> triangleArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadTriangleArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadArray = vtkSmartPointer<vtkCellArray>::New();

    SMDS_FaceIteratorPtr aFaceIter = meshDS->facesIterator();
    for (;aFaceIter->more();) {
        const SMDS_MeshFace* aFace = aFaceIter->next();

        //triangle
        if(aFace->NbNodes() == 3) {
            vtkSmartPointer<vtkTriangle> tria = vtkSmartPointer<vtkTriangle>::New();
            tria->GetPointIds()->SetId(0, aFace->GetNode(0)->GetID()-1);
            tria->GetPointIds()->SetId(1, aFace->GetNode(1)->GetID()-1);
            tria->GetPointIds()->SetId(2, aFace->GetNode(2)->GetID()-1);

            triangleArray->InsertNextCell(tria);
        }
        //quad
        else if(aFace->NbNodes() == 4) {
            vtkSmartPointer<vtkQuad> quad = vtkSmartPointer<vtkQuad>::New();
            quad->GetPointIds()->SetId(0, aFace->GetNode(0)->GetID()-1);
            quad->GetPointIds()->SetId(1, aFace->GetNode(1)->GetID()-1);
            quad->GetPointIds()->SetId(2, aFace->GetNode(2)->GetID()-1);

            quadArray->InsertNextCell(quad);
        }
        else if (aFace->NbNodes() == 6) {
            vtkSmartPointer<vtkQuadraticTriangle> tria = vtkSmartPointer<vtkQuadraticTriangle>::New();
            tria->GetPointIds()->SetId(0, aFace->GetNode(0)->GetID()-1);
            tria->GetPointIds()->SetId(1, aFace->GetNode(1)->GetID()-1);
            tria->GetPointIds()->SetId(2, aFace->GetNode(2)->GetID()-1);
            tria->GetPointIds()->SetId(3, aFace->GetNode(3)->GetID()-1);
            tria->GetPointIds()->SetId(4, aFace->GetNode(4)->GetID()-1);
            tria->GetPointIds()->SetId(5, aFace->GetNode(5)->GetID()-1);
            quadTriangleArray->InsertNextCell(tria);
        }
     }
     if(triangleArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_TRIANGLE, triangleArray);

     if(quadArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUAD, quadArray);

     if(quadTriangleArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUADRATIC_TRIANGLE, quadTriangleArray);


    //now all volumes
    vtkSmartPointer<vtkCellArray> tetraArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadTetraArray = vtkSmartPointer<vtkCellArray>::New();

    SMDS_VolumeIteratorPtr aVolIter = meshDS->volumesIterator();
    for (;aVolIter->more();) {
        const SMDS_MeshVolume* aVol = aVolIter->next();

        //tetrahedra
        if(aVol->NbNodes() == 4) {
            vtkSmartPointer<vtkTetra> tetra = vtkSmartPointer<vtkTetra>::New();
            tetra->GetPointIds()->SetId(0, aVol->GetNode(0)->GetID()-1);
            tetra->GetPointIds()->SetId(1, aVol->GetNode(1)->GetID()-1);
            tetra->GetPointIds()->SetId(2, aVol->GetNode(2)->GetID()-1);
            tetra->GetPointIds()->SetId(3, aVol->GetNode(3)->GetID()-1);

            tetraArray->InsertNextCell(tetra);
        }
        //quadratic tetrahedra
        else if( aVol->NbNodes() == 10) {

            vtkSmartPointer<vtkQuadraticTetra> tetra = vtkSmartPointer<vtkQuadraticTetra>::New();
            tetra->GetPointIds()->SetId(0, aVol->GetNode(0)->GetID()-1);
            tetra->GetPointIds()->SetId(1, aVol->GetNode(1)->GetID()-1);
            tetra->GetPointIds()->SetId(2, aVol->GetNode(2)->GetID()-1);
            tetra->GetPointIds()->SetId(3, aVol->GetNode(3)->GetID()-1);
            tetra->GetPointIds()->SetId(4, aVol->GetNode(4)->GetID()-1);
            tetra->GetPointIds()->SetId(5, aVol->GetNode(5)->GetID()-1);
            tetra->GetPointIds()->SetId(6, aVol->GetNode(6)->GetID()-1);
            tetra->GetPointIds()->SetId(7, aVol->GetNode(7)->GetID()-1);
            tetra->GetPointIds()->SetId(8, aVol->GetNode(8)->GetID()-1);
            tetra->GetPointIds()->SetId(9, aVol->GetNode(9)->GetID()-1);

            quadTetraArray->InsertNextCell(tetra);
        }
    }
    if(tetraArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_TETRA, tetraArray);

    if(quadTetraArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUADRATIC_TETRA, quadTetraArray);


    //Now copy the point data over
    //############################

    if(!res->StressValues.getValues().empty()) {
        const std::vector<double>& vec = res->StressValues.getValues();
        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
        data->SetNumberOfValues(vec.size());
        data->SetName("Von Mises stress");

        for(size_t i=0; i<vec.size(); ++i)
            data->SetValue(i, vec[i]);

        grid->GetPointData()->AddArray(data);
    }

    if(!res->StressValues.getValues().empty()) {
        const std::vector<double>& vec = res->MaxShear.getValues();
        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
        data->SetNumberOfValues(vec.size());
        data->SetName("Max shear stress (Tresca)");

        for(size_t i=0; i<vec.size(); ++i)
            data->SetValue(i, vec[i]);

        grid->GetPointData()->AddArray(data);
    }

    if(!res->StressValues.getValues().empty()) {
        const std::vector<double>& vec = res->PrincipalMax.getValues();
        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
        data->SetNumberOfValues(vec.size());
        data->SetName("Maximum Principal stress");

        for(size_t i=0; i<vec.size(); ++i)
            data->SetValue(i, vec[i]);

        grid->GetPointData()->AddArray(data);
    }

    if(!res->StressValues.getValues().empty()) {
        const std::vector<double>& vec = res->PrincipalMin.getValues();
        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
        data->SetNumberOfValues(vec.size());
        data->SetName("Minimum Principal stress");

        for(size_t i=0; i<vec.size(); ++i)
            data->SetValue(i, vec[i]);

        grid->GetPointData()->AddArray(data);
    }

    if(!res->StressValues.getValues().empty()) {
        const std::vector<double>& vec = res->Temperature.getValues();
        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
        data->SetNumberOfValues(vec.size());
        data->SetName("Temperature");

        for(size_t i=0; i<vec.size(); ++i)
            data->SetValue(i, vec[i]);

        grid->GetPointData()->AddArray(data);
    }

    if(!res->StressValues.getValues().empty()) {
        const std::vector<Base::Vector3d>& vec = res->DisplacementVectors.getValues();
        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
        data->SetNumberOfComponents(3);
        data->SetName("Displacement");

        for(std::vector<Base::Vector3d>::const_iterator it=vec.begin(); it!=vec.end(); ++it) {
            double tuple[] = {it->x, it->y, it->z};
            data->InsertNextTuple(tuple);
        }

        grid->GetPointData()->AddArray(data);
    }

    Data.setValue(grid);
}
