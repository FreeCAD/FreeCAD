/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2009     *
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
# include <cstdlib>
# include <memory>
# include <cmath>

# include <Bnd_Box.hxx>
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <TopoDS_Vertex.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <gp_Pnt.hxx>
#endif

#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Base/Console.h>
#include <Base/Type.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

#include <SMESH_Gen.hxx>
#include <SMESH_Mesh.hxx>
#include <SMDS_PolyhedralVolumeOfNodes.hxx>
#include <SMDS_VolumeTool.hxx>

# include <TopoDS_Face.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Shape.hxx>

#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>
#include <vtkStructuredGrid.h>
#include <vtkImageData.h>
#include <vtkRectilinearGrid.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include <vtkPointData.h>
#include <vtkCellData.h>
#include <vtkCellArray.h>
#include <vtkDataArray.h>
#include <vtkDoubleArray.h>
#include <vtkIdList.h>
#include <vtkCellTypes.h>

#include <vtkTetra.h>
#include <vtkHexahedron.h>
#include <vtkWedge.h>
#include <vtkPyramid.h>
#include <vtkQuadraticTetra.h>
#include <vtkQuadraticHexahedron.h>
#include <vtkTriangle.h>
#include <vtkQuad.h>
#include <vtkQuadraticTriangle.h>
#include <vtkQuadraticQuad.h>

#include "FemVTKTools.h"
#include "FemMeshProperty.h"
#include "FemAnalysis.h"

namespace Fem
{

template<class TReader> vtkDataSet* readVTKFile(const char*fileName)
{
  vtkSmartPointer<TReader> reader =
    vtkSmartPointer<TReader>::New();
  reader->SetFileName(fileName);
  reader->Update();
  reader->GetOutput()->Register(reader);
  return vtkDataSet::SafeDownCast(reader->GetOutput());
}

template<class TWriter> void writeVTKFile(const char* filename, vtkSmartPointer<vtkUnstructuredGrid> dataset)
{
  vtkSmartPointer<TWriter> writer =
    vtkSmartPointer<TWriter>::New();
  writer->SetFileName(filename);
  writer->SetInputData(dataset);
  writer->Write();
}
  
void FemVTKTools::importVTKMesh(vtkSmartPointer<vtkDataSet> dataset, FemMesh* mesh)
{
    const vtkIdType nPoints = dataset->GetNumberOfPoints();
    const vtkIdType nCells = dataset->GetNumberOfCells();
    Base::Console().Log("%d nodes/points and %d cells/elements found!\n", nPoints, nCells);

    //vtkSmartPointer<vtkCellArray> cells = dataset->GetCells();  // works only for vtkUnstructuredGrid
    vtkSmartPointer<vtkIdList> idlist= vtkSmartPointer<vtkIdList>::New();

    //Now fill the SMESH datastructure
    SMESH_Mesh* smesh = const_cast<SMESH_Mesh*>(mesh->getSMesh());
    SMESHDS_Mesh* meshds = smesh->GetMeshDS();
    meshds->ClearMesh();

    for(vtkIdType i=0; i<nPoints; i++)
    {   
        double* p = dataset->GetPoint(i);
        meshds->AddNodeWithID(p[0], p[1], p[2], i+1);
    }

    for(vtkIdType iCell=0; iCell<nCells; iCell++)
    {
        idlist->Reset();
        idlist = dataset->GetCell(iCell)->GetPointIds();
        vtkIdType *ids = idlist->GetPointer(0);
        // 3D cells first
        switch(dataset->GetCellType(iCell))
        {
            case VTK_TETRA:
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, iCell+1);
                break;
            case VTK_HEXAHEDRON:
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, iCell+1);
                break;
            case VTK_QUADRATIC_TETRA:
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, ids[8]+1, ids[9]+1, iCell+1);
                break;
            case VTK_QUADRATIC_HEXAHEDRON:
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, ids[8]+1, ids[9]+1,\
                                        ids[10]+1, ids[11]+1, ids[12]+1, ids[13]+1, ids[14]+1, ids[15]+1, ids[16]+1, ids[17]+1, ids[18]+1, ids[19]+1,\
                                        iCell+1);
                break;
            case VTK_WEDGE:
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, iCell+1);
                break;
            case VTK_PYRAMID:
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, iCell+1);
                break;
            // 2D elements
            case VTK_TRIANGLE:
                meshds->AddFaceWithID(ids[0]+1, ids[1]+1, ids[2]+1, iCell+1);
                break;
            case VTK_QUADRATIC_TRIANGLE:
                meshds->AddFaceWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, iCell+1);
                break;
            case VTK_QUAD:
                meshds->AddFaceWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, iCell+1);
                break;
            case VTK_QUADRATIC_QUAD:
                meshds->AddFaceWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, iCell+1);
                break;
            default:
            {
                Base::Console().Error("Only common 2D and 3D Cells are supported in VTK mesh import\n");
                break;
            }
        }
    }
}

FemMesh* FemVTKTools::readVTKMesh(const char* filename, FemMesh* mesh)
{
    Base::TimeInfo Start;
    Base::Console().Log("Start: read FemMesh from VTK unstructuredGrid ======================\n");
    Base::FileInfo f(filename);
      
    if(f.hasExtension("vtu"))
    {
        vtkSmartPointer<vtkDataSet> dataset  = readVTKFile<vtkXMLUnstructuredGridReader>(filename);
        importVTKMesh(dataset, mesh);
    }
    else if(f.hasExtension("vtk"))
    {
        vtkSmartPointer<vtkDataSet> dataset = readVTKFile<vtkDataSetReader>(filename);
        importVTKMesh(dataset, mesh);
    }
    else
    {
        Base::Console().Error("file name extension is not supported\n");
        return NULL;
    }
    //Mesh should link to the part feature, in order to set up FemConstraint

    Base::Console().Log("    %f: Done \n",Base::TimeInfo::diffTimeF(Start,Base::TimeInfo()));
    return mesh;
}

void exportFemMeshFaces(vtkSmartPointer<vtkUnstructuredGrid> grid, const SMDS_FaceIteratorPtr& aFaceIter)
{

    vtkSmartPointer<vtkCellArray> triangleArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadTriangleArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadQuadArray = vtkSmartPointer<vtkCellArray>::New();

    for (;aFaceIter->more();)
    {
        const SMDS_MeshFace* aFace = aFaceIter->next();

        //triangle
        if(aFace->NbNodes() == 3)
        {
            vtkSmartPointer<vtkTriangle> tria = vtkSmartPointer<vtkTriangle>::New();
            tria->GetPointIds()->SetId(0, aFace->GetNode(0)->GetID()-1);
            tria->GetPointIds()->SetId(1, aFace->GetNode(1)->GetID()-1);
            tria->GetPointIds()->SetId(2, aFace->GetNode(2)->GetID()-1);

            triangleArray->InsertNextCell(tria);
        }
        //quad
        else if(aFace->NbNodes() == 4)
        {
            vtkSmartPointer<vtkQuad> quad = vtkSmartPointer<vtkQuad>::New();
            quad->GetPointIds()->SetId(0, aFace->GetNode(0)->GetID()-1);
            quad->GetPointIds()->SetId(1, aFace->GetNode(1)->GetID()-1);
            quad->GetPointIds()->SetId(2, aFace->GetNode(2)->GetID()-1);
            quad->GetPointIds()->SetId(3, aFace->GetNode(3)->GetID()-1);
            
            quadArray->InsertNextCell(quad);
        }
        //quadratic triangle
        else if (aFace->NbNodes() == 6)
        {
            vtkSmartPointer<vtkQuadraticTriangle> tria = vtkSmartPointer<vtkQuadraticTriangle>::New();
            tria->GetPointIds()->SetId(0, aFace->GetNode(0)->GetID()-1);
            tria->GetPointIds()->SetId(1, aFace->GetNode(1)->GetID()-1);
            tria->GetPointIds()->SetId(2, aFace->GetNode(2)->GetID()-1);
            tria->GetPointIds()->SetId(3, aFace->GetNode(3)->GetID()-1);
            tria->GetPointIds()->SetId(4, aFace->GetNode(4)->GetID()-1);
            tria->GetPointIds()->SetId(5, aFace->GetNode(5)->GetID()-1);
            quadTriangleArray->InsertNextCell(tria);
        }
        //quadratic quad
        else if(aFace->NbNodes() == 8)
        {
            vtkSmartPointer<vtkQuadraticQuad> quad = vtkSmartPointer<vtkQuadraticQuad>::New();
            quad->GetPointIds()->SetId(0, aFace->GetNode(0)->GetID()-1);
            quad->GetPointIds()->SetId(1, aFace->GetNode(1)->GetID()-1);
            quad->GetPointIds()->SetId(2, aFace->GetNode(2)->GetID()-1);
            quad->GetPointIds()->SetId(3, aFace->GetNode(3)->GetID()-1);
            quad->GetPointIds()->SetId(4, aFace->GetNode(4)->GetID()-1);
            quad->GetPointIds()->SetId(5, aFace->GetNode(5)->GetID()-1);
            quad->GetPointIds()->SetId(6, aFace->GetNode(6)->GetID()-1);
            quad->GetPointIds()->SetId(7, aFace->GetNode(7)->GetID()-1);
            
            quadQuadArray->InsertNextCell(quad);
        }
     }
     if(triangleArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_TRIANGLE, triangleArray);

     if(quadArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUAD, quadArray);

     if(quadTriangleArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUADRATIC_TRIANGLE, quadTriangleArray);
        
     if(quadQuadArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUADRATIC_QUAD, quadQuadArray);

}

void exportFemMeshCells(vtkSmartPointer<vtkUnstructuredGrid> grid, const SMDS_VolumeIteratorPtr& aVolIter)
{
    // add common CFD cells like hex, wedge, prism, in addition to tetra
    vtkSmartPointer<vtkCellArray> tetraArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> hexaArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> wedgeArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> pyramidArray = vtkSmartPointer<vtkCellArray>::New();
    // quadratic elemnts with 13 and 15 nodes are not added yet
    vtkSmartPointer<vtkCellArray> quadTetraArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadHexaArray = vtkSmartPointer<vtkCellArray>::New();
     
    for (;aVolIter->more();)
    {
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
        // common cell types for CFD
        if(aVol->NbNodes() == 5) {
            vtkSmartPointer<vtkPyramid> cell= vtkSmartPointer<vtkPyramid>::New();
            cell->GetPointIds()->SetId(0, aVol->GetNode(0)->GetID()-1);
            cell->GetPointIds()->SetId(1, aVol->GetNode(1)->GetID()-1);
            cell->GetPointIds()->SetId(2, aVol->GetNode(2)->GetID()-1);
            cell->GetPointIds()->SetId(3, aVol->GetNode(3)->GetID()-1);
            cell->GetPointIds()->SetId(4, aVol->GetNode(4)->GetID()-1);
        
            pyramidArray->InsertNextCell(cell);
        }
        if(aVol->NbNodes() == 6) {
            vtkSmartPointer<vtkWedge> cell = vtkSmartPointer<vtkWedge>::New();
            cell->GetPointIds()->SetId(0, aVol->GetNode(0)->GetID()-1);
            cell->GetPointIds()->SetId(1, aVol->GetNode(1)->GetID()-1);
            cell->GetPointIds()->SetId(2, aVol->GetNode(2)->GetID()-1);
            cell->GetPointIds()->SetId(3, aVol->GetNode(3)->GetID()-1);
            cell->GetPointIds()->SetId(4, aVol->GetNode(4)->GetID()-1);
            cell->GetPointIds()->SetId(5, aVol->GetNode(5)->GetID()-1);
        
            wedgeArray->InsertNextCell(cell);
        }
        if(aVol->NbNodes() == 8) {
            vtkSmartPointer<vtkHexahedron> cell= vtkSmartPointer<vtkHexahedron>::New();
            cell->GetPointIds()->SetId(0, aVol->GetNode(0)->GetID()-1);
            cell->GetPointIds()->SetId(1, aVol->GetNode(1)->GetID()-1);
            cell->GetPointIds()->SetId(2, aVol->GetNode(2)->GetID()-1);
            cell->GetPointIds()->SetId(3, aVol->GetNode(3)->GetID()-1);
            cell->GetPointIds()->SetId(4, aVol->GetNode(4)->GetID()-1);
            cell->GetPointIds()->SetId(5, aVol->GetNode(5)->GetID()-1);
            cell->GetPointIds()->SetId(6, aVol->GetNode(6)->GetID()-1);
            cell->GetPointIds()->SetId(7, aVol->GetNode(7)->GetID()-1);
        
            hexaArray->InsertNextCell(cell);
        }
        //quadratic tetrahedra
        else if( aVol->NbNodes() == 10) {

            vtkSmartPointer<vtkQuadraticTetra> tetra = vtkSmartPointer<vtkQuadraticTetra>::New();
            for(int i=0; i<10; i++){
                tetra->GetPointIds()->SetId(i, aVol->GetNode(i)->GetID()-1);
            }

            quadTetraArray->InsertNextCell(tetra);
        }
        if(aVol->NbNodes() == 20) { // not tested, no sure about vertex sequence
            vtkSmartPointer<vtkHexahedron> cell= vtkSmartPointer<vtkHexahedron>::New();
            for(int i=0; i<20; i++){
                cell->GetPointIds()->SetId(i, aVol->GetNode(i)->GetID()-1);
            }
            hexaArray->InsertNextCell(cell);
        }
    }

    if(tetraArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_TETRA, tetraArray);

    if(pyramidArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_PYRAMID, pyramidArray);

    if(wedgeArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_WEDGE, wedgeArray);

    if(hexaArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_HEXAHEDRON, hexaArray);
        
    if(quadTetraArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUADRATIC_TETRA, quadTetraArray);
        
    if(quadHexaArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUADRATIC_HEXAHEDRON, quadHexaArray);

}

void FemVTKTools::exportVTKMesh(const FemMesh* mesh, vtkSmartPointer<vtkUnstructuredGrid> grid)
{
        
    SMESH_Mesh* smesh = const_cast<SMESH_Mesh*>(mesh->getSMesh());
    SMESHDS_Mesh* meshDS = smesh->GetMeshDS();
    const SMDS_MeshInfo& info = meshDS->GetMeshInfo();
    
    //start with the nodes
    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    SMDS_NodeIteratorPtr aNodeIter = meshDS->nodesIterator();

    points->SetNumberOfPoints(info.NbNodes());
    for(; aNodeIter->more(); ) {
        const SMDS_MeshNode* node = aNodeIter->next();  // why float, not double?
        double coords[3] = {double(node->X()), double(node->Y()), double(node->Z())};
        points->SetPoint(node->GetID()-1, coords);
    }
    grid->SetPoints(points);
    //start with 2d elements
    SMDS_FaceIteratorPtr aFaceIter = meshDS->facesIterator();
    exportFemMeshFaces(grid, aFaceIter);
    //3D volume elements
    SMDS_VolumeIteratorPtr aVolIter = meshDS->volumesIterator();
    exportFemMeshCells(grid, aVolIter);

}

void FemVTKTools::writeVTKMesh(const char* filename, const FemMesh* mesh)
{
    
    Base::TimeInfo Start;
    Base::Console().Log("Start: write FemMesh from VTK unstructuredGrid ======================\n");
    Base::FileInfo f(filename);
      
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    exportVTKMesh(mesh, grid);
    //vtkSmartPointer<vtkDataSet> dataset = vtkDataSet::SafeDownCast(grid);
    if(f.hasExtension("vtu")){
        writeVTKFile<vtkXMLUnstructuredGridWriter>(filename, grid);
    }
    else if(f.hasExtension("vtk")){
        writeVTKFile<vtkDataSetWriter>(filename, grid);
    }
    else{
        Base::Console().Error("file name extension is not supported to write VTK\n");
    }
    
    Base::Console().Log("    %f: Done \n",Base::TimeInfo::diffTimeF(Start, Base::TimeInfo()));
}


App::DocumentObject* getObjectByType(const Base::Type type)
{
    App::Document* pcDoc = App::GetApplication().getActiveDocument();
    if(!pcDoc)
    {
        Base::Console().Message("No active document is found thus created\n");
        pcDoc = App::GetApplication().newDocument();
    }
    App::DocumentObject* obj = pcDoc->getActiveObject();

    if(obj->getTypeId() == type)
    {
        return obj;
    }
    if(obj->getTypeId() ==  FemAnalysis::getClassTypeId())
    {
        std::vector<App::DocumentObject*> fem = (static_cast<FemAnalysis*>(obj))->Member.getValues();
        for (std::vector<App::DocumentObject*>::iterator it = fem.begin(); it != fem.end(); ++it) {
            if ((*it)->getTypeId().isDerivedFrom(type))
                return static_cast<App::DocumentObject*>(*it); // return the first of that type
        }
    }
    return NULL;
}

App::DocumentObject* createObjectByType(const Base::Type type)
{
    App::Document* pcDoc = App::GetApplication().getActiveDocument();
    if(!pcDoc)
    {
        Base::Console().Message("No active document is found thus created\n");
        pcDoc = App::GetApplication().newDocument();
    }
    App::DocumentObject* obj = pcDoc->getActiveObject();

    if(obj->getTypeId() ==  FemAnalysis::getClassTypeId())
    {
        std::vector<App::DocumentObject*> fem = (static_cast<FemAnalysis*>(obj))->Member.getValues();
        App::DocumentObject* newobj = pcDoc->addObject(type.getName());
        fem.push_back(newobj); // FemAnalysis is not a DocumentGroup derived class but DocumentObject
        (static_cast<FemAnalysis*>(obj))->Member.setValues(fem);
        return newobj;
    }
    else
    {
        return pcDoc->addObject(type.getName()); // create in the acitive document
    }

}


App::DocumentObject* FemVTKTools::readFluidicResult(const char* filename, App::DocumentObject* res)
{
    Base::TimeInfo Start;
    Base::Console().Log("Start: read FemResult with FemMesh from VTK file ======================\n");
    Base::FileInfo f(filename);
      
    vtkSmartPointer<vtkDataSet> ds;
    if(f.hasExtension("vtu"))
    {
        ds = readVTKFile<vtkXMLUnstructuredGridReader>(filename);
    }
    else if(f.hasExtension("vtk"))
    {
        ds = readVTKFile<vtkDataSetReader>(filename);
    }
    else
    {
        Base::Console().Error("file name extension is not supported\n");
    }
    
    App::Document* pcDoc = App::GetApplication().getActiveDocument();
    if(!pcDoc)
    {
        Base::Console().Message("No active document is found thus created\n");
        pcDoc = App::GetApplication().newDocument();
    }
    App::DocumentObject* obj = pcDoc->getActiveObject();

    vtkSmartPointer<vtkDataSet> dataset = ds;
    App::DocumentObject* result = NULL;
    if(!res)
        result = res;
    else
    {
        Base::Console().Log("FemResultObject pointer is NULL, trying to get the active object\n");
        if (obj->getTypeId() == Base::Type::fromName("Fem::FemResultObjectPython"))
            result = obj;
        else
        {
            Base::Console().Log("the active object is not the correct type, do nothing\n");
            return NULL;  
        }
    }

    App::DocumentObject* mesh = pcDoc->addObject("Fem::FemMeshObject", "ResultMesh");
    FemMesh* fmesh = new FemMesh(); // PropertyFemMesh instance is responsible to relase FemMesh ??
    importVTKMesh(dataset, fmesh);
    static_cast<PropertyFemMesh*>(mesh->getPropertyByName("FemMesh"))->setValue(*fmesh);
    static_cast<App::PropertyLink*>(result->getPropertyByName("Mesh"))->setValue(mesh);
    // PropertyLink is the property type to store DocumentObject pointer
    
    importFluidicResult(dataset, result);
    pcDoc->recompute();
    
    Base::Console().Log("    %f: Done \n", Base::TimeInfo::diffTimeF(Start, Base::TimeInfo()));
    
    return result;
}


void FemVTKTools::writeResult(const char* filename, const App::DocumentObject* res) {
    if (!res) 
    {
        App::Document* pcDoc = App::GetApplication().getActiveDocument();
        if(!pcDoc)
        {
            Base::Console().Message("No active document is found thus do nothing and return\n");
            return;
        }
        res = pcDoc->getActiveObject(); //type checking
    }
    if(!res) {
        Base::Console().Error("Result object pointer is invalid and it is not active oject");
        return;
    }

    Base::TimeInfo Start;
    Base::Console().Log("Start: write FemResult or CfdResult to VTK unstructuredGrid dataset =======\n");
    Base::FileInfo f(filename);
      
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    App::DocumentObject* mesh = static_cast<App::PropertyLink*>(res->getPropertyByName("Mesh"))->getValue();
    const FemMesh& fmesh = static_cast<PropertyFemMesh*>(mesh->getPropertyByName("FemMesh"))->getValue();
    FemVTKTools::exportVTKMesh(&fmesh, grid);
    
    if(res->getPropertyByName("Velocity")){
        FemVTKTools::exportFluidicResult(res, grid);
    }
    else if(res->getPropertyByName("DisplacementVectors")){
        FemVTKTools::exportMechanicalResult(res, grid);
    }
    else{
        return;
    }
    
    //vtkSmartPointer<vtkDataSet> dataset = vtkDataSet::SafeDownCast(grid);
    if(f.hasExtension("vtu")){
        writeVTKFile<vtkXMLUnstructuredGridWriter>(filename, grid);
    }
    else if(f.hasExtension("vtk")){
        writeVTKFile<vtkDataSetWriter>(filename, grid);
    }
    else{
        Base::Console().Error("file name extension is not supported to write VTK\n");
    }
    
    Base::Console().Log("    %f: Done \n",Base::TimeInfo::diffTimeF(Start, Base::TimeInfo()));
}


void FemVTKTools::importFluidicResult(vtkSmartPointer<vtkDataSet> dataset, App::DocumentObject* res) {

    // velocity and pressure are essential, Temperature is optional, so are turbulence related variables
    std::map<const char*, const char*> vars;  // varable name defined in openfoam -> property defined in CfdResult.py
    vars["Pressure"] = "p";
    vars["Velocity"] = "U";
    vars["Temperature"] = "T";
    vars["TurbulenceThermalDiffusivity"] = "alphat";
    vars["TurbulenceViscosity"] = "nut";
    vars["TurbulenceEnergy"] = "k";
    vars["TurbulenceDissipationRate"] = "epsilon";
    vars["TurbulenceSpecificDissipation"] = "omega";
    
    const int max_var_index = 11;
    std::vector<double> stats(3*max_var_index, 0.0);

    std::map<const char*, int> varids; // must agree with definition in  _TaskPanelCfdResult.py
    varids["Ux"] = 0;
    varids["Uy"] = 1;
    varids["Uz"] = 2;
    varids["Umag"] = 3;
    varids["Pressure"] = 4;
    varids["Temperature"] = 5;
    varids["TurbulenceEnergy"] = 6;
    varids["TurbulenceViscosity"] = 7;
    varids["TurbulenceDissipationRate"] = 8;
    //varids["TurbulenceThermalDiffusivity"] = 9;
    //varids["TurbulenceSpecificDissipation"] = 10;
    
    double ts = 0.0;  // t=0.0 for static simulation
    static_cast<App::PropertyFloat*>(res->getPropertyByName("Time"))->setValue(ts);
    
    vtkSmartPointer<vtkPointData> pd = dataset->GetPointData();
    const vtkIdType nPoints = dataset->GetNumberOfPoints();
    if(pd->GetNumberOfArrays() == 0) {
        Base::Console().Error("No point data array is found in vtk data set, do nothin\n");
        // if pointData is empty, data may be in cellDate, cellData -> pointData interpolation is possible in VTK
        return;
    }
    
    std::vector<long> nodeIds(nPoints);
    vtkSmartPointer<vtkDataArray> vel = pd->GetArray(vars["Velocity"]);
    if(nPoints && vel && vel->GetNumberOfComponents() == 3) {
        std::vector<Base::Vector3d> vec(nPoints);
        double vmin=1.0e100, vmean=0.0, vmax=0.0;  // only velocity magnitude is calc in c++
        for(vtkIdType i=0; i<nPoints; ++i) {
            double *p = vel->GetTuple(i); // both vtkFloatArray and vtkDoubleArray return double* for GetTuple(i)
            double vmag = std::sqrt(p[0]*p[0] + p[1]*p[1] + p[2]*p[2]);
            vmean += vmag;
            if(vmag > vmax) vmax = vmag;
            if(vmag < vmin) vmin = vmag;
            vec[i] = (Base::Vector3d(p[0], p[1], p[2]));
            nodeIds[i] = i;
        }
        int index = varids["Umag"];
        stats[index*3] = vmin;
        stats[index*3 + 2] = vmax;
        stats[index*3 + 1] = vmean/nPoints;
        App::PropertyVectorList* velocity = static_cast<App::PropertyVectorList*>(res->getPropertyByName("Velocity"));
        if(velocity) {
            //PropertyVectorList will not show up in PropertyEditor
            velocity->setValues(vec);
            static_cast<App::PropertyIntegerList*>(res->getPropertyByName("NodeNumbers"))->setValues(nodeIds);
            Base::Console().Message("Velocity field has been loaded \n");
        }
        else
            Base::Console().Error("Velocity property is not found in Cfd Result object \n");
    }
    else {
        Base::Console().Error("Velocity field is not found in Cfd Result vtk file \n");
        return;
    }

    for(auto const& kv: vars){
        if (std::string(kv.first) == std::string("Velocity"))
            continue;
        vtkDataArray* vec = vtkDataArray::SafeDownCast(pd->GetArray(kv.second));
        if(nPoints && vec && vec->GetNumberOfComponents() == 1) {  
            App::PropertyFloatList* field = static_cast<App::PropertyFloatList*>(res->getPropertyByName(kv.first));
            if (!field) {
                Base::Console().Error("static_cast<App::PropertyFloatList*>((res->getPropertyByName(\"%s\")) failed \n", kv.first);
                continue;
            }

            double vmin=1.0e100, vmean=0.0, vmax=0.0;
            std::vector<double> values(nPoints, 0.0);
            for(vtkIdType i = 0; i < vec->GetNumberOfTuples(); i++)
            {
                double v = *(vec->GetTuple(i));
                values[i] = v;
                vmean += v;
                if(v > vmax) vmax = v;
                if(v < vmin) vmin = v;
            }
            field->setValues(values);

            int index = varids[kv.first];
            stats[index*3] = vmin;
            stats[index*3 + 2] = vmax;
            stats[index*3 + 1] = vmean/nPoints;
            
            Base::Console().Message("field  \"%s\" has been loaded \n", kv.first);
        }
    }
    static_cast<App::PropertyFloatList*>(res->getPropertyByName("Stats"))->setValues(stats);
}

/*
void FemVTKTools::importMechanicalResult(const vtkDataSet* grid, App::DocumentObject* res) {
    // to be implemented later by FemWorkbench developer
}
 * */

void FemVTKTools::exportFluidicResult(const App::DocumentObject* res, vtkSmartPointer<vtkDataSet> grid) {
    if(!res->getPropertyByName("Velocity")){
        Base::Console().Message("Warning: essential field like `velocity` is not found in CfdResult\n");
        return;
    }
    App::PropertyVectorList* velocity = static_cast<App::PropertyVectorList*>(res->getPropertyByName("Velocity"));
    const std::vector<Base::Vector3d>& vel = velocity->getValues();
    if(!vel.empty()) {
        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
        data->SetNumberOfComponents(3);
        data->SetName("Velocity");

        for(std::vector<Base::Vector3d>::const_iterator it=vel.begin(); it!=vel.end(); ++it) {
            double tuple[] = {it->x, it->y, it->z};
            data->InsertNextTuple(tuple);
        }

        grid->GetPointData()->AddArray(data);
    }
    else{
        Base::Console().Message("Warning: essential fields pressure and velocity is empty in CfdResult\n");
    }
    // Temperature is optional, so are other turbulence related variables
    std::vector<const char*> vars;  // varable names are defined in CfdResult.py
    vars.push_back("Pressure");
    vars.push_back("Temperature");
    vars.push_back("TurbulenceThermalDiffusivity");
    vars.push_back("TurbulenceViscosity");
    vars.push_back("TurbulenceEnergy");
    vars.push_back("TurbulenceDissipationRate");
    vars.push_back("TurbulenceSpecificDissipation");
    for(auto const& var: vars){
        App::PropertyFloatList* field;
        if (res->getPropertyByName(var))
            field = static_cast<App::PropertyFloatList*>(res->getPropertyByName(var));
        if(!field && !field->getValues().empty()) {
            const std::vector<double>& vec = field->getValues();
            vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
            data->SetNumberOfValues(vec.size());
            data->SetName(var);

            for(size_t i=0; i<vec.size(); ++i)
                data->SetValue(i, vec[i]);

            grid->GetPointData()->AddArray(data);
        }
    }
}


void FemVTKTools::exportMechanicalResult(const App::DocumentObject* obj, vtkSmartPointer<vtkDataSet> grid) {
    // code redundance can be avoided by property inspection, consider refactoring
    const FemResultObject* res = static_cast<const FemResultObject*>(obj);
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
        const std::vector<double>& vec = res->UserDefined.getValues();
        vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
        data->SetNumberOfValues(vec.size());
        data->SetName("User Defined Results");

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
}

} // namespace
