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


void FemPostPipeline::load(FemResultObject* res) {

    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkUnstructuredGrid::New();
    source = grid;
    
    //first copy the mesh over
    //########################
    
    if(!res->Mesh.getValue() || !res->Mesh.getValue()->isDerivedFrom(Fem::FemMeshObject::getClassTypeId()))
        return;
    
    const FemMesh& mesh = static_cast<FemMeshObject*>(res->Mesh.getValue())->FemMesh.getValue();   
    SMESH_Mesh* smesh = const_cast<SMESH_Mesh*>(mesh.getSMesh());
    SMESHDS_Mesh* meshDS = smesh->GetMeshDS();
    const SMDS_MeshInfo& info = meshDS->GetMeshInfo();
    
    //start with the nodes
    vtkSmartPointer<vtkPoints> points = vtkPoints::New();
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
            vtkSmartPointer<vtkTriangle> tria = vtkTriangle::New(); 
            tria->GetPointIds()->SetId(0, aFace->GetNode(0)->GetID()-1);
            tria->GetPointIds()->SetId(1, aFace->GetNode(1)->GetID()-1);
            tria->GetPointIds()->SetId(2, aFace->GetNode(2)->GetID()-1);

            triangleArray->InsertNextCell(tria);
        }
        //quad
        else if(aFace->NbNodes() == 4) {
            vtkSmartPointer<vtkQuad> quad = vtkQuad::New(); 
            quad->GetPointIds()->SetId(0, aFace->GetNode(0)->GetID()-1);
            quad->GetPointIds()->SetId(1, aFace->GetNode(1)->GetID()-1);
            quad->GetPointIds()->SetId(2, aFace->GetNode(2)->GetID()-1);

            quadArray->InsertNextCell(quad);
        }
        else if (aFace->NbNodes() == 6) {
            vtkSmartPointer<vtkQuadraticTriangle> tria = vtkQuadraticTriangle::New(); 
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
    
    tetraArray->SetNumberOfCells(info.NbTetras());
    SMDS_VolumeIteratorPtr aVolIter = meshDS->volumesIterator();
    for (;aVolIter->more();) {
        const SMDS_MeshVolume* aVol = aVolIter->next();

        //tetrahedra
        if(aVol->NbNodes() == 4) {
            vtkSmartPointer<vtkTetra> tetra = vtkTetra::New();        
            tetra->GetPointIds()->SetId(0, aVol->GetNode(0)->GetID()-1);
            tetra->GetPointIds()->SetId(1, aVol->GetNode(1)->GetID()-1);
            tetra->GetPointIds()->SetId(2, aVol->GetNode(2)->GetID()-1);
            tetra->GetPointIds()->SetId(3, aVol->GetNode(3)->GetID()-1);

            tetraArray->InsertNextCell(tetra);
        }
        //quadratic tetrahedra
        else if( aVol->NbNodes() == 10) {
            
            vtkSmartPointer<vtkQuadraticTetra> tetra = vtkQuadraticTetra::New();        
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
        vtkSmartPointer<vtkDoubleArray> data = vtkDoubleArray::New();
        data->SetNumberOfValues(vec.size());
        data->SetName("Stress");
        
        for(size_t i=0; i<vec.size(); ++i)
            data->SetValue(i, vec[i]);
        
        grid->GetPointData()->AddArray(data);
    }
    
    if(!res->StressValues.getValues().empty()) {
        const std::vector<Base::Vector3d>& vec = res->DisplacementVectors.getValues();
        vtkSmartPointer<vtkDoubleArray> data = vtkDoubleArray::New();
        data->SetNumberOfComponents(3);
        data->SetName("Displacement");
        
        for(std::vector<Base::Vector3d>::const_iterator it=vec.begin(); it!=vec.end(); ++it) {
            double tuple[] = {it->x, it->y, it->z};
                    Base::Console().Message("disp mag; %f\n", it->Length());
            data->InsertNextTuple(tuple);
        }
        
        grid->GetPointData()->AddArray(data);
    }
    
      
    polyDataSource = vtkGeometryFilter::New();
    polyDataSource->SetInputData(source);
    polyDataSource->Update();
}
