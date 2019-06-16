/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2017 Qingfeng Xia  <qingfeng.xia at oxford uni>         *
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
# include <map>

# include <Bnd_Box.hxx>
# include <BRep_Tool.hxx>
# include <BRepBndLib.hxx>
# include <BRepExtrema_DistShapeShape.hxx>
# include <TopoDS_Vertex.hxx>
# include <BRepBuilderAPI_MakeVertex.hxx>
# include <gp_Pnt.hxx>
# include <TopoDS_Face.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Shape.hxx>

# include <SMESH_Gen.hxx>
# include <SMESH_Mesh.hxx>
# include <SMDS_PolyhedralVolumeOfNodes.hxx>
# include <SMDS_VolumeTool.hxx>
# include <SMESHDS_Mesh.hxx>

# include <vtkDataSetReader.h>
# include <vtkDataSetWriter.h>
# include <vtkStructuredGrid.h>
# include <vtkImageData.h>
# include <vtkRectilinearGrid.h>
# include <vtkUnstructuredGrid.h>
# include <vtkXMLUnstructuredGridReader.h>
# include <vtkXMLUnstructuredGridWriter.h>
# include <vtkPointData.h>
# include <vtkCellData.h>
# include <vtkCellArray.h>
# include <vtkDataArray.h>
# include <vtkDoubleArray.h>
# include <vtkIdList.h>
# include <vtkCellTypes.h>
# include <vtkTriangle.h>
# include <vtkQuad.h>
# include <vtkQuadraticTriangle.h>
# include <vtkQuadraticQuad.h>
# include <vtkTetra.h>
# include <vtkPyramid.h>
# include <vtkWedge.h>
# include <vtkHexahedron.h>
# include <vtkQuadraticTetra.h>
# include <vtkQuadraticPyramid.h>
# include <vtkQuadraticWedge.h>
# include <vtkQuadraticHexahedron.h>
#endif

#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Base/Console.h>
#include <Base/Type.h>
#include <Base/Parameter.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>

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


void FemVTKTools::importVTKMesh(vtkSmartPointer<vtkDataSet> dataset, FemMesh* mesh, float scale)
{
    const vtkIdType nPoints = dataset->GetNumberOfPoints();
    const vtkIdType nCells = dataset->GetNumberOfCells();
    Base::Console().Log("%d nodes/points and %d cells/elements found!\n", nPoints, nCells);
    Base::Console().Log("Build SMESH mesh out of the vtk mesh data.\n", nPoints, nCells);

    //vtkSmartPointer<vtkCellArray> cells = dataset->GetCells();  // works only for vtkUnstructuredGrid
    vtkSmartPointer<vtkIdList> idlist= vtkSmartPointer<vtkIdList>::New();

    //Now fill the SMESH datastructure
    SMESH_Mesh* smesh = const_cast<SMESH_Mesh*>(mesh->getSMesh());
    SMESHDS_Mesh* meshds = smesh->GetMeshDS();
    meshds->ClearMesh();

    for(vtkIdType i=0; i<nPoints; i++)
    {
        double* p = dataset->GetPoint(i);
        meshds->AddNodeWithID(p[0]*scale, p[1]*scale, p[2]*scale, i+1);
    }

    for(vtkIdType iCell=0; iCell<nCells; iCell++)
    {
        idlist->Reset();
        idlist = dataset->GetCell(iCell)->GetPointIds();
        vtkIdType *ids = idlist->GetPointer(0);
        switch(dataset->GetCellType(iCell))
        {
            // 2D faces
            case VTK_TRIANGLE:  // tria3
                meshds->AddFaceWithID(ids[0]+1, ids[1]+1, ids[2]+1, iCell+1);
                break;
            case VTK_QUADRATIC_TRIANGLE:  // tria6
                meshds->AddFaceWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, iCell+1);
                break;
            case VTK_QUAD:  // quad4
                meshds->AddFaceWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, iCell+1);
                break;
            case VTK_QUADRATIC_QUAD:  // quad8
                meshds->AddFaceWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, iCell+1);
                break;

            // 3D volumes
            case VTK_TETRA:  // tetra4
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, iCell+1);
                break;
            case VTK_QUADRATIC_TETRA:  // tetra10
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, ids[8]+1, ids[9]+1, iCell+1);
                break;
            case VTK_HEXAHEDRON:  // hexa8
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, iCell+1);
                break;
            case VTK_QUADRATIC_HEXAHEDRON:  // hexa20
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, ids[8]+1, ids[9]+1,\
                                        ids[10]+1, ids[11]+1, ids[12]+1, ids[13]+1, ids[14]+1, ids[15]+1, ids[16]+1, ids[17]+1, ids[18]+1, ids[19]+1,\
                                        iCell+1);
                break;
            case VTK_WEDGE:  // penta6
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, iCell+1);
                break;
            case VTK_QUADRATIC_WEDGE:  // penta15
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, ids[8]+1, ids[9]+1,\
                                        ids[10]+1, ids[11]+1, ids[12]+1, ids[13]+1, ids[14]+1,\
                                        iCell+1);
                break;
            case VTK_PYRAMID:  // pyra5
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, iCell+1);
                break;
            case VTK_QUADRATIC_PYRAMID:  // pyra13
                meshds->AddVolumeWithID(ids[0]+1, ids[1]+1, ids[2]+1, ids[3]+1, ids[4]+1, ids[5]+1, ids[6]+1, ids[7]+1, ids[8]+1, ids[9]+1,\
                                        ids[10]+1, ids[11]+1, ids[12]+1,\
                                        iCell+1);
                break;

            // not handled cases
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
    Base::Console().Log("  Start: VTK mesh builder faces.\n");

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
        else
        {
            throw std::runtime_error("Face not yet supported by FreeCAD's VTK mesh builder\n");
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

    Base::Console().Log("  End: VTK mesh builder faces.\n");
}

void exportFemMeshCells(vtkSmartPointer<vtkUnstructuredGrid> grid, const SMDS_VolumeIteratorPtr& aVolIter)
{
    Base::Console().Log("  Start: VTK mesh builder volumes.\n");

    vtkSmartPointer<vtkCellArray> tetraArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> pyramidArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> wedgeArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> hexaArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadTetraArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadPyramidArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadWedgeArray = vtkSmartPointer<vtkCellArray>::New();
    vtkSmartPointer<vtkCellArray> quadHexaArray = vtkSmartPointer<vtkCellArray>::New();

    for (;aVolIter->more();)
    {
        const SMDS_MeshVolume* aVol = aVolIter->next();

        if (aVol->NbNodes() == 4) { // tetra4
            Base::Console().Log("    Volume tetra4\n");
            vtkSmartPointer<vtkTetra> cell = vtkSmartPointer<vtkTetra>::New();
            cell->GetPointIds()->SetId(0, aVol->GetNode(0)->GetID()-1);
            cell->GetPointIds()->SetId(1, aVol->GetNode(1)->GetID()-1);
            cell->GetPointIds()->SetId(2, aVol->GetNode(2)->GetID()-1);
            cell->GetPointIds()->SetId(3, aVol->GetNode(3)->GetID()-1);
            tetraArray->InsertNextCell(cell);
        }
        else if (aVol->NbNodes() == 5) { // pyra5
            Base::Console().Log("    Volume pyra5\n");
            vtkSmartPointer<vtkPyramid> cell = vtkSmartPointer<vtkPyramid>::New();
            cell->GetPointIds()->SetId(0, aVol->GetNode(0)->GetID()-1);
            cell->GetPointIds()->SetId(1, aVol->GetNode(1)->GetID()-1);
            cell->GetPointIds()->SetId(2, aVol->GetNode(2)->GetID()-1);
            cell->GetPointIds()->SetId(3, aVol->GetNode(3)->GetID()-1);
            cell->GetPointIds()->SetId(4, aVol->GetNode(4)->GetID()-1);
            pyramidArray->InsertNextCell(cell);
        }
        else if (aVol->NbNodes() == 6) { // penta6
            Base::Console().Log("    Volume penta6\n");
            vtkSmartPointer<vtkWedge> cell = vtkSmartPointer<vtkWedge>::New();
            cell->GetPointIds()->SetId(0, aVol->GetNode(0)->GetID()-1);
            cell->GetPointIds()->SetId(1, aVol->GetNode(1)->GetID()-1);
            cell->GetPointIds()->SetId(2, aVol->GetNode(2)->GetID()-1);
            cell->GetPointIds()->SetId(3, aVol->GetNode(3)->GetID()-1);
            cell->GetPointIds()->SetId(4, aVol->GetNode(4)->GetID()-1);
            cell->GetPointIds()->SetId(5, aVol->GetNode(5)->GetID()-1);
            wedgeArray->InsertNextCell(cell);
        }
        else if (aVol->NbNodes() == 8) { // hexa8
            Base::Console().Log("    Volume hexa8\n");
            vtkSmartPointer<vtkHexahedron> cell = vtkSmartPointer<vtkHexahedron>::New();
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
        else if (aVol->NbNodes() == 10) { // tetra10
            Base::Console().Log("    Volume tetra10\n");
            vtkSmartPointer<vtkQuadraticTetra> tetra = vtkSmartPointer<vtkQuadraticTetra>::New();
            for(int i=0; i<10; i++){
                tetra->GetPointIds()->SetId(i, aVol->GetNode(i)->GetID()-1);
            }
            quadTetraArray->InsertNextCell(tetra);
        }

        else if (aVol->NbNodes() == 13) { // pyra13
            Base::Console().Log("    Volume pyra13\n");
            vtkSmartPointer<vtkQuadraticPyramid> cell = vtkSmartPointer<vtkQuadraticPyramid>::New();
            for(int i=0; i<13; i++){
                cell->GetPointIds()->SetId(i, aVol->GetNode(i)->GetID()-1);
                // Base::Console().Log("node ids: %i\n", aVol->GetNode(i)->GetID()-1);
            }
            quadPyramidArray->InsertNextCell(cell);
        }
        else if (aVol->NbNodes() == 15) { // penta15
            Base::Console().Log("    Volume penta15\n");
            vtkSmartPointer<vtkQuadraticWedge> cell = vtkSmartPointer<vtkQuadraticWedge>::New();
            for(int i=0; i<15; i++){
                cell->GetPointIds()->SetId(i, aVol->GetNode(i)->GetID()-1);
            }
            quadWedgeArray->InsertNextCell(cell);
        }
        else if (aVol->NbNodes() == 20) { // hexa20
            Base::Console().Log("    Volume hexa20\n");
            vtkSmartPointer<vtkQuadraticHexahedron> cell = vtkSmartPointer<vtkQuadraticHexahedron>::New();
            for(int i=0; i<20; i++){
                cell->GetPointIds()->SetId(i, aVol->GetNode(i)->GetID()-1);
            }
            quadHexaArray->InsertNextCell(cell);
        }
        else {
            throw std::runtime_error("Volume not yet supported by FreeCAD's VTK mesh builder\n");
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

    if(quadPyramidArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUADRATIC_PYRAMID, quadPyramidArray);

    if(quadWedgeArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUADRATIC_WEDGE, quadWedgeArray);

    if(quadHexaArray->GetNumberOfCells()>0)
        grid->SetCells(VTK_QUADRATIC_HEXAHEDRON, quadHexaArray);

    Base::Console().Log("  End: VTK mesh builder volumes.\n");
}

void FemVTKTools::exportVTKMesh(const FemMesh* mesh, vtkSmartPointer<vtkUnstructuredGrid> grid, float scale)
{

    Base::Console().Log("Start: VTK mesh builder ======================\n");
    SMESH_Mesh* smesh = const_cast<SMESH_Mesh*>(mesh->getSMesh());
    SMESHDS_Mesh* meshDS = smesh->GetMeshDS();

    // nodes
    Base::Console().Log("  Start: VTK mesh builder nodes.\n");

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    SMDS_NodeIteratorPtr aNodeIter = meshDS->nodesIterator();

    while (aNodeIter->more()) {
        const SMDS_MeshNode* node = aNodeIter->next();  // why float, not double?
        double coords[3] = {double(node->X()*scale), double(node->Y()*scale), double(node->Z()*scale)};
        points->InsertPoint(node->GetID()-1, coords);
        // memory is allocated by VTK points size for max node id, not for point count
        // if the SMESH mesh has gaps in node numbering, points without any element assignment will be inserted in these point gaps too
        // this needs to be taken into account on node mapping when FreeCAD FEM results are exported to vtk
    }
    grid->SetPoints(points);
    // nodes debugging
    const SMDS_MeshInfo& info = meshDS->GetMeshInfo();
    Base::Console().Log("    Size of nodes in SMESH grid: %i.\n", info.NbNodes());
    const vtkIdType nNodes = grid->GetNumberOfPoints();
    Base::Console().Log("    Size of nodes in VTK grid: %i.\n", nNodes);
    Base::Console().Log("  End: VTK mesh builder nodes.\n");

    // faces
    SMDS_FaceIteratorPtr aFaceIter = meshDS->facesIterator();
    exportFemMeshFaces(grid, aFaceIter);

    // volumes
    SMDS_VolumeIteratorPtr aVolIter = meshDS->volumesIterator();
    exportFemMeshCells(grid, aVolIter);

    Base::Console().Log("End: VTK mesh builder ======================\n");
}

void FemVTKTools::writeVTKMesh(const char* filename, const FemMesh* mesh)
{

    Base::TimeInfo Start;
    Base::Console().Log("Start: write FemMesh from VTK unstructuredGrid ======================\n");
    Base::FileInfo f(filename);

    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    exportVTKMesh(mesh, grid);
    //vtkSmartPointer<vtkDataSet> dataset = vtkDataSet::SafeDownCast(grid);
    Base::Console().Log("Start: writing mesh data ======================\n");
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
        std::vector<App::DocumentObject*> fem = (static_cast<FemAnalysis*>(obj))->Group.getValues();
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
        App::DocumentObject* newobj = pcDoc->addObject(type.getName());
        static_cast<FemAnalysis*>(obj)->addObject(newobj);
        return newobj;
    }
    else
    {
        return pcDoc->addObject(type.getName()); // create in the acitive document
    }

}


App::DocumentObject* FemVTKTools::readResult(const char* filename, App::DocumentObject* res)
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
        Base::Console().Message("FemResultObject pointer is NULL, trying to get the active object\n");
        if(obj->getTypeId() == Base::Type::fromName("Fem::FemResultObjectPython"))
            result = obj;
        else
        {
            Base::Console().Message("the active object is not the correct type, do nothing\n");
            return NULL;
        }
    }

    App::DocumentObject* mesh = pcDoc->addObject("Fem::FemMeshObject", "ResultMesh");
    FemMesh* fmesh = new FemMesh(); // PropertyFemMesh instance is responsible to release FemMesh ??
    importVTKMesh(dataset, fmesh);
    static_cast<PropertyFemMesh*>(mesh->getPropertyByName("FemMesh"))->setValue(*fmesh);
    static_cast<App::PropertyLink*>(result->getPropertyByName("Mesh"))->setValue(mesh);
    // PropertyLink is the property type to store DocumentObject pointer

    //vtkSmartPointer<vtkPointData> pd = dataset->GetPointData();
    importFreeCADResult(dataset, result);

    pcDoc->recompute();
    Base::Console().Log("    %f: Done \n", Base::TimeInfo::diffTimeF(Start, Base::TimeInfo()));
    Base::Console().Log("End: read FemResult with FemMesh from VTK file ======================\n");

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
        res = pcDoc->getActiveObject(); //type checking is done by caller
    }
    if(!res) {
        Base::Console().Error("Result object pointer is invalid and it is not active object");
        return;
    }

    Base::TimeInfo Start;
    Base::Console().Log("Start: write FemResult to VTK unstructuredGrid dataset =======\n");
    Base::FileInfo f(filename);

    // mesh
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    App::DocumentObject* mesh = static_cast<App::PropertyLink*>(res->getPropertyByName("Mesh"))->getValue();
    const FemMesh& fmesh = static_cast<PropertyFemMesh*>(mesh->getPropertyByName("FemMesh"))->getValue();
    FemVTKTools::exportVTKMesh(&fmesh, grid);

    Base::Console().Log("    %f: vtk mesh builder finished\n",Base::TimeInfo::diffTimeF(Start, Base::TimeInfo()));

    // result
    FemVTKTools::exportFreeCADResult(res, grid);

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

    Base::Console().Log("    %f: writing result object to vtk finished\n",Base::TimeInfo::diffTimeF(Start, Base::TimeInfo()));
    Base::Console().Log("End: write FemResult to VTK unstructuredGrid dataset =======\n");
}


std::map<std::string, std::string> _getFreeCADMechResultVectorProperties() {
    // see src/Mod/Fem/femobjects/_FemResultMechanical
    // App::PropertyVectorList will be a list of vectors in vtk
    std::map<std::string, std::string> resFCVecProp;
    resFCVecProp["DisplacementVectors"] = "Displacement";
    // https://forum.freecadweb.org/viewtopic.php?f=18&t=33106&start=70#p296317
    resFCVecProp["PS1Vector"] = "Major Principal Stress";
    resFCVecProp["PS2Vector"] = "Intermediate Principal Stress";
    resFCVecProp["PS3Vector"] = "Minor Principal Stress";

    return resFCVecProp;
}

// see https://forum.freecadweb.org/viewtopic.php?f=18&t=33106&start=30#p277434 for further information regarding names etc...
// some scalar list are not needed on VTK file export but they are needed for internal VTK pipeline
// TODO some filter to only export the needed values to VTK file but have all in FreeCAD VTK pipline
std::map<std::string, std::string> _getFreeCADMechResultScalarProperties() {
    // see src/Mod/Fem/femobjects/_FemResultMechanical
    // App::PropertyFloatList will be a list of scalars in vtk
    std::map<std::string, std::string> resFCScalProp;
    resFCScalProp["DisplacementLengths"] = "Displacement Magnitude";  // can be plotted in Paraview as THE DISPLACEMENT MAGNITUDE
    resFCScalProp["MaxShear"] = "Tresca Stress";
    resFCScalProp["NodeStressXX"] = "Stress xx component";
    resFCScalProp["NodeStressYY"] = "Stress yy component";
    resFCScalProp["NodeStressZZ"] = "Stress zz component";
    resFCScalProp["NodeStressXY"] = "Stress xy component";
    resFCScalProp["NodeStressXZ"] = "Stress xz component";
    resFCScalProp["NodeStressYZ"] = "Stress yz component";
    resFCScalProp["NodeStrainXX"] = "Strain xx component";
    resFCScalProp["NodeStrainYY"] = "Strain yy component";
    resFCScalProp["NodeStrainZZ"] = "Strain zz component";
    resFCScalProp["NodeStrainXY"] = "Strain xy component";
    resFCScalProp["NodeStrainXZ"] = "Strain xz component";
    resFCScalProp["NodeStrainYZ"] = "Strain yz component";
    resFCScalProp["Peeq"] = "Equivalent Plastic Strain";
    // https://forum.freecadweb.org/viewtopic.php?f=18&t=33106&start=70#p296317
    // resFCScalProp["PrincipalMax"] = "Major Principal Stress";  // can be plotted in Paraview as THE MAJOR PRINCIPAL STRESS MAGNITUDE
    // resFCScalProp["PrincipalMed"] = "Intermediate Principal Stress";  // can be plotted in Paraview as THE INTERMEDIATE PRINCIPAL STRESS MAGNITUDE
    // resFCScalProp["PrincipalMin"] = "Minor Principal Stress";  // can be plotted in Paraview as THE MINOR PRINCIPAL STRESS MAGNITUDE
    resFCScalProp["StressValues"] = "von Mises Stress";
    resFCScalProp["Temperature"] = "Temperature";
    resFCScalProp["MohrCoulomb"] = "MohrCoulomb";
    resFCScalProp["ReinforcementRatio_x"] = "ReinforcementRatio_x";
    resFCScalProp["ReinforcementRatio_y"] = "ReinforcementRatio_y";
    resFCScalProp["ReinforcementRatio_z"] = "ReinforcementRatio_z";

    resFCScalProp["UserDefined"] = "UserDefinedMyName";  // this is empty or am I wrong ?!
    resFCScalProp["MassFlowRate"] = "Mass Flow Rate";
    resFCScalProp["NetworkPressure"] = "Network Pressure";

    return resFCScalProp;
}


void FemVTKTools::importFreeCADResult(vtkSmartPointer<vtkDataSet> dataset, App::DocumentObject* result) {
    Base::Console().Log("Start: import vtk result file data into a FreeCAD result object.\n");

    std::map<std::string, std::string> vectors = _getFreeCADMechResultVectorProperties();
    std::map<std::string, std::string> scalars = _getFreeCADMechResultScalarProperties();

    double ts = 0.0;  // t=0.0 for static simulation
    static_cast<App::PropertyFloat*>(result->getPropertyByName("Time"))->setValue(ts);

    vtkSmartPointer<vtkPointData> pd = dataset->GetPointData();
    if(pd->GetNumberOfArrays() == 0) {
        Base::Console().Error("No point data array is found in vtk data set, do nothing\n");
        // if pointData is empty, data may be in cellDate, cellData -> pointData interpolation is possible in VTK
        return;
    }

    // NodeNumbers
    const vtkIdType nPoints = dataset->GetNumberOfPoints();
    std::vector<long> nodeIds(nPoints);
    for(vtkIdType i=0; i<nPoints; ++i) {
        nodeIds[i] = i+1;
    }
    static_cast<App::PropertyIntegerList*>(result->getPropertyByName("NodeNumbers"))->setValues(nodeIds);
    Base::Console().Log("    NodeNumbers have been filled with values.\n");

    // vectors
    for (std::map<std::string, std::string>::iterator it = vectors.begin(); it != vectors.end(); ++it) {
        int dim = 3;  // Fixme: currently 3D only, here we could run into trouble, FreeCAD only supports dim 3D, I do not know about VTK
        vtkDataArray* vector_field = vtkDataArray::SafeDownCast(pd->GetArray(it->second.c_str()));
        if(vector_field && vector_field->GetNumberOfComponents() == dim) {
            App::PropertyVectorList* vector_list = static_cast<App::PropertyVectorList*>(result->getPropertyByName(it->first.c_str()));
            if(vector_list) {
                std::vector<Base::Vector3d> vec(nPoints);
                for(vtkIdType i=0; i<nPoints; ++i) {
                    double *p = vector_field->GetTuple(i); // both vtkFloatArray and vtkDoubleArray return double* for GetTuple(i)
                    vec[i] = (Base::Vector3d(p[0], p[1], p[2]));
                }
                // PropertyVectorList will not show up in PropertyEditor
                vector_list->setValues(vec);
                Base::Console().Log("    A PropertyVectorList has been filled with values: %s\n", it->first.c_str());
            }
            else {
                Base::Console().Error("static_cast<App::PropertyVectorList*>((result->getPropertyByName(\"%s\")) failed.\n", it->first.c_str());
                continue;
            }
        }
        else
            Base::Console().Message("    PropertyVectorList NOT found in vkt file data: %s\n", it->first.c_str());
    }

    // scalars
    for (std::map<std::string, std::string>::iterator it = scalars.begin(); it != scalars.end(); ++it) {
        vtkDataArray* vec = vtkDataArray::SafeDownCast(pd->GetArray(it->second.c_str()));
        if(nPoints && vec && vec->GetNumberOfComponents() == 1) {
            App::PropertyFloatList* field = static_cast<App::PropertyFloatList*>(result->getPropertyByName(it->first.c_str()));
            if (!field) {
                Base::Console().Error("static_cast<App::PropertyFloatList*>((result->getPropertyByName(\"%s\")) failed.\n", it->first.c_str());
                continue;
            }

            double vmin=1.0e100, vmean=0.0, vmax=-1.0e100;
            std::vector<double> values(nPoints, 0.0);
            for(vtkIdType i = 0; i < vec->GetNumberOfTuples(); i++) {
                double v = *(vec->GetTuple(i));
                values[i] = v;
                vmean += v;
                if(v > vmax) vmax = v;
                if(v < vmin) vmin = v;
            }
            field->setValues(values);
            Base::Console().Log("    A PropertyFloatList has been filled with vales: %s\n", it->first.c_str());
        }
        else
            Base::Console().Message("    PropertyFloatList NOT found in vkt file data %s\n", it->first.c_str());
    }

    // stats
    // stats are added by importVTKResults

    Base::Console().Log("End: import vtk result file data into a FreeCAD result object.\n");
}


void FemVTKTools::exportFreeCADResult(const App::DocumentObject* result, vtkSmartPointer<vtkDataSet> grid) {
    Base::Console().Log("Start: Create VTK result data from FreeCAD result data.\n");

    std::map<std::string, std::string> vectors = _getFreeCADMechResultVectorProperties();
    std::map<std::string, std::string> scalars = _getFreeCADMechResultScalarProperties();

    const Fem::FemResultObject* res = static_cast<const Fem::FemResultObject*>(result);
    const vtkIdType nPoints = grid->GetNumberOfPoints();

    // we need the corresponding mesh to get the correct id for the result data (when the freecad smesh mesh has gaps in the points
    // vtk has more points. Vtk does not support point gaps, thus the gaps are filled with points. Then the mapping must be correct)
    App::DocumentObject* meshObj = res->Mesh.getValue();
    if (!meshObj || !meshObj->isDerivedFrom(FemMeshObject::getClassTypeId())) {
        Base::Console().Error("Result object does not correctly link to mesh");
        return;
    }
    SMESH_Mesh* smesh = const_cast<SMESH_Mesh*>(static_cast<FemMeshObject*>(meshObj)->FemMesh.getValue().getSMesh());
    SMESHDS_Mesh* meshDS = smesh->GetMeshDS();

    // vectors
    for (std::map<std::string, std::string>::iterator it = vectors.begin(); it != vectors.end(); ++it) {
        const int dim=3;  //Fixme, detect dim, but FreeCAD PropertyVectorList ATM only has DIM of 3
        App::PropertyVectorList* field = nullptr;
        if (res->getPropertyByName(it->first.c_str()))
            field = static_cast<App::PropertyVectorList*>(res->getPropertyByName(it->first.c_str()));
        else
            Base::Console().Error("    PropertyVectorList not found: %s\n", it->first.c_str());
        if (field && field->getSize() > 0) {
            //if (nPoints != field->getSize())
            //    Base::Console().Error("Size of PropertyVectorList = %d, not equal to vtk mesh node count %d \n", field->getSize(), nPoints);
            const std::vector<Base::Vector3d>& vel = field->getValues();
            vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
            data->SetNumberOfComponents(dim);
            data->SetNumberOfTuples(nPoints);
            data->SetName(it->second.c_str());

            //we need to set values for the unused points.
            //TODO: ensure that the result bar does not include the used 0 if it is not part of the result (e.g. does the result bar show 0 as smallest value?)
            if (nPoints != field->getSize()) {
                double tuple[] = {0,0,0};
                for (vtkIdType i=0; i<nPoints; ++i) {
                    data->SetTuple(i, tuple);
                }
            }

            SMDS_NodeIteratorPtr aNodeIter = meshDS->nodesIterator();
            for (std::vector<Base::Vector3d>::const_iterator jt=vel.begin(); jt!=vel.end(); ++jt) {
                const SMDS_MeshNode* node = aNodeIter->next();
                double tuple[] = {jt->x, jt->y, jt->z};
                data->SetTuple(node->GetID()-1, tuple);
            }
            grid->GetPointData()->AddArray(data);
            Base::Console().Log("    The PropertyVectorList %s was exported to VTK vector list: %s\n", it->first.c_str(), it->second.c_str());
        }
        else
            Base::Console().Message("    PropertyVectorList NOT exported to vtk: %s size is: %i\n", it->first.c_str(), field->getSize());
    }

    // scalars
    for (std::map<std::string, std::string>::iterator it = scalars.begin(); it != scalars.end(); ++it) {
        App::PropertyFloatList* field = nullptr;
        if (res->getPropertyByName(it->first.c_str()))
            field = static_cast<App::PropertyFloatList*>(res->getPropertyByName(it->first.c_str()));
        else
            Base::Console().Error("PropertyFloatList %s not found \n", it->first.c_str());
        if (field && field->getSize() > 0) {
            //if (nPoints != field->getSize())
            //    Base::Console().Error("Size of PropertyFloatList = %d, not equal to vtk mesh node count %d \n", field->getSize(), nPoints);
            const std::vector<double>& vec = field->getValues();
            vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
            data->SetNumberOfValues(nPoints);
            data->SetName(it->second.c_str());

            //we need to set values for the unused points.
            //TODO: ensure that the result bar does not include the used 0 if it is not part of the result (e.g. does the result bar show 0 as smallest value?)
            if (nPoints != field->getSize()) {
                for (vtkIdType i=0; i<nPoints; ++i) {
                    data->SetValue(i, 0);
                }
             }

            SMDS_NodeIteratorPtr aNodeIter = meshDS->nodesIterator();
            for (size_t i=0; i<vec.size(); ++i) {
                const SMDS_MeshNode* node = aNodeIter->next();
                data->SetValue(node->GetID()-1, vec[i]);
            }

            grid->GetPointData()->AddArray(data);
            Base::Console().Log("    The PropertyFloatList %s was exported to VTK scalar list: %s\n", it->first.c_str(), it->second.c_str());
       }
        else
            Base::Console().Message("    PropertyFloatList NOT exported to vtk: %s size is: %i\n", it->first.c_str(), field->getSize());
    }

    Base::Console().Log("End: Create VTK result data from FreeCAD result data.\n");
}

} // namespace
