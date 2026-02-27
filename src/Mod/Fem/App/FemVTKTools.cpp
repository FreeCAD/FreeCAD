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


#include <Python.h>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <map>
#include <memory>

#include <SMESHDS_Mesh.hxx>
#include <SMESH_Mesh.hxx>

#include <vtkCellArray.h>
#include <vtkDataArray.h>
#include <vtkDataSetReader.h>
#include <vtkDataSetWriter.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkHexahedron.h>
#include <vtkIdList.h>
#include <vtkLine.h>
#include <vtkMultiBlockDataSet.h>
#include <vtkPointData.h>
#include <vtkPyramid.h>
#include <vtkQuad.h>
#include <vtkQuadraticEdge.h>
#include <vtkQuadraticHexahedron.h>
#include <vtkQuadraticPyramid.h>
#include <vtkQuadraticQuad.h>
#include <vtkQuadraticTetra.h>
#include <vtkQuadraticTriangle.h>
#include <vtkQuadraticWedge.h>
#include <vtkStringArray.h>
#include <vtkTetra.h>
#include <vtkTriangle.h>
#include <vtkUnsignedCharArray.h>
#include <vtkUnstructuredGrid.h>
#include <vtkWedge.h>
#include <vtkXMLMultiBlockDataWriter.h>
#include <vtkXMLPUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridReader.h>
#include <vtkXMLUnstructuredGridWriter.h>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Base/Type.h>

#include "FemAnalysis.h"
#include "FemResultObject.h"
#include "FemVTKTools.h"


namespace Fem
{

template<class TReader>
vtkDataSet* readVTKFile(const char* fileName)
{
    vtkSmartPointer<TReader> reader = vtkSmartPointer<TReader>::New();
    reader->SetFileName(fileName);
    reader->Update();
    auto output = reader->GetOutput();
    if (output) {
        output->Register(reader);
    }
    return vtkDataSet::SafeDownCast(output);
}

template<class TWriter>
void writeVTKFile(const char* filename, vtkSmartPointer<vtkUnstructuredGrid> dataset)
{
    vtkSmartPointer<TWriter> writer = vtkSmartPointer<TWriter>::New();
    writer->SetFileName(filename);
    writer->SetInputData(dataset);
    writer->Write();
}

namespace
{

// Helper function to fill vtkCellArray from SMDS_Mesh using vtk cell order
template<typename T, typename E>
void fillVtkArray(vtkSmartPointer<vtkCellArray>& elemArray, std::vector<int>& types, const E* elem)
{
    vtkSmartPointer<T> cell = vtkSmartPointer<T>::New();
    const std::vector<int>& order = SMDS_MeshCell::toVtkOrder(elem->GetEntityType());
    if (!order.empty()) {
        for (int i = 0; i < elem->NbNodes(); ++i) {
            cell->GetPointIds()->SetId(i, elem->GetNode(order[i])->GetID() - 1);
        }
    }
    else {
        for (int i = 0; i < elem->NbNodes(); ++i) {
            cell->GetPointIds()->SetId(i, elem->GetNode(i)->GetID() - 1);
        }
    }
    elemArray->InsertNextCell(cell);
    types.push_back(SMDS_MeshCell::toVtkType(elem->GetEntityType()));
}

// Helper function to fill SMDS_Mesh elements ID from vtk cell
void fillMeshElementIds(vtkCell* cell, std::vector<int>& ids)
{
    VTKCellType cellType = static_cast<VTKCellType>(cell->GetCellType());
    const std::vector<int>& order = SMDS_MeshCell::fromVtkOrder(cellType);
    vtkIdType* vtkIds = cell->GetPointIds()->GetPointer(0);
    ids.clear();
    int nbPoints = cell->GetNumberOfPoints();
    ids.resize(nbPoints);
    if (!order.empty()) {
        for (int i = 0; i < nbPoints; ++i) {
            ids[i] = vtkIds[order[i]] + 1;
        }
    }
    else {
        for (int i = 0; i < nbPoints; ++i) {
            ids[i] = vtkIds[i] + 1;
        }
    }
}

}  // namespace


void FemVTKTools::importVTKMesh(vtkSmartPointer<vtkDataSet> dataset, FemMesh* mesh, float scale)
{
    const vtkIdType nPoints = dataset->GetNumberOfPoints();
    const vtkIdType nCells = dataset->GetNumberOfCells();
    Base::Console().log("%d nodes/points and %d cells/elements found!\n", nPoints, nCells);
    Base::Console().log("Build SMESH mesh out of the vtk mesh data.\n", nPoints, nCells);

    // Now fill the SMESH datastructure
    SMESH_Mesh* smesh = mesh->getSMesh();
    SMESHDS_Mesh* meshds = smesh->GetMeshDS();
    meshds->ClearMesh();

    for (vtkIdType i = 0; i < nPoints; i++) {
        double* p = dataset->GetPoint(i);
        meshds->AddNodeWithID(p[0] * scale, p[1] * scale, p[2] * scale, i + 1);
    }

    for (vtkIdType iCell = 0; iCell < nCells; iCell++) {
        vtkCell* cell = dataset->GetCell(iCell);
        std::vector<int> ids;
        fillMeshElementIds(cell, ids);
        switch (cell->GetCellType()) {
            // 1D edges
            case VTK_LINE:  // seg2
                meshds->AddEdgeWithID(ids[0], ids[1], iCell + 1);
                break;
            case VTK_QUADRATIC_EDGE:  // seg3
                meshds->AddEdgeWithID(ids[0], ids[1], ids[2], iCell + 1);
                break;
            // 2D faces
            case VTK_TRIANGLE:  // tria3
                meshds->AddFaceWithID(ids[0], ids[1], ids[2], iCell + 1);
                break;
            case VTK_QUADRATIC_TRIANGLE:  // tria6
                meshds->AddFaceWithID(ids[0], ids[1], ids[2], ids[3], ids[4], ids[5], iCell + 1);
                break;
            case VTK_QUAD:  // quad4
                meshds->AddFaceWithID(ids[0], ids[1], ids[2], ids[3], iCell + 1);
                break;
            case VTK_QUADRATIC_QUAD:  // quad8
                meshds->AddFaceWithID(
                    ids[0],
                    ids[1],
                    ids[2],
                    ids[3],
                    ids[4],
                    ids[5],
                    ids[6],
                    ids[7],
                    iCell + 1
                );
                break;
            // 3D volumes
            case VTK_TETRA:  // tetra4
                meshds->AddVolumeWithID(ids[0], ids[1], ids[2], ids[3], iCell + 1);
                break;
            case VTK_QUADRATIC_TETRA:  // tetra10
                meshds->AddVolumeWithID(
                    ids[0],
                    ids[1],
                    ids[2],
                    ids[3],
                    ids[4],
                    ids[5],
                    ids[6],
                    ids[7],
                    ids[8],
                    ids[9],
                    iCell + 1
                );
                break;
            case VTK_HEXAHEDRON:  // hexa8
                meshds->AddVolumeWithID(
                    ids[0],
                    ids[1],
                    ids[2],
                    ids[3],
                    ids[4],
                    ids[5],
                    ids[6],
                    ids[7],
                    iCell + 1
                );
                break;
            case VTK_QUADRATIC_HEXAHEDRON:  // hexa20
                meshds->AddVolumeWithID(
                    ids[0],
                    ids[1],
                    ids[2],
                    ids[3],
                    ids[4],
                    ids[5],
                    ids[6],
                    ids[7],
                    ids[8],
                    ids[9],
                    ids[10],
                    ids[11],
                    ids[12],
                    ids[13],
                    ids[14],
                    ids[15],
                    ids[16],
                    ids[17],
                    ids[18],
                    ids[19],
                    iCell + 1
                );
                break;
            case VTK_WEDGE:  // penta6
                meshds->AddVolumeWithID(ids[0], ids[1], ids[2], ids[3], ids[4], ids[5], iCell + 1);
                break;
            case VTK_QUADRATIC_WEDGE:  // penta15
                meshds->AddVolumeWithID(
                    ids[0],
                    ids[1],
                    ids[2],
                    ids[3],
                    ids[4],
                    ids[5],
                    ids[6],
                    ids[7],
                    ids[8],
                    ids[9],
                    ids[10],
                    ids[11],
                    ids[12],
                    ids[13],
                    ids[14],
                    iCell + 1
                );
                break;
            case VTK_PYRAMID:  // pyra5
                meshds->AddVolumeWithID(ids[0], ids[1], ids[2], ids[3], ids[4], iCell + 1);
                break;
            case VTK_QUADRATIC_PYRAMID:  // pyra13
                meshds->AddVolumeWithID(
                    ids[0],
                    ids[1],
                    ids[2],
                    ids[3],
                    ids[4],
                    ids[5],
                    ids[6],
                    ids[7],
                    ids[8],
                    ids[9],
                    ids[10],
                    ids[11],
                    ids[12],
                    iCell + 1
                );
                break;

            // not handled cases
            default: {
                Base::Console().error(
                    "Only common 1D, 2D and 3D Cells are supported in VTK mesh import\n"
                );
                break;
            }
        }
    }
}

FemMesh* FemVTKTools::readVTKMesh(const char* filename, FemMesh* mesh)
{
    Base::TimeElapsed Start;
    Base::Console().log("Start: read FemMesh from VTK unstructuredGrid ======================\n");
    Base::FileInfo f(filename);

    if (f.hasExtension("vtu")) {
        vtkSmartPointer<vtkDataSet> dataset = readVTKFile<vtkXMLUnstructuredGridReader>(filename);
        if (!dataset.Get()) {
            Base::Console().error("Failed to load file %s\n", filename);
            return nullptr;
        }
        importVTKMesh(dataset, mesh);
    }
    else if (f.hasExtension("pvtu")) {
        vtkSmartPointer<vtkDataSet> dataset = readVTKFile<vtkXMLPUnstructuredGridReader>(filename);
        if (!dataset.Get()) {
            Base::Console().error("Failed to load file %s\n", filename);
            return nullptr;
        }
        importVTKMesh(dataset, mesh);
    }
    else if (f.hasExtension("vtk")) {
        vtkSmartPointer<vtkDataSet> dataset = readVTKFile<vtkDataSetReader>(filename);
        if (!dataset.Get()) {
            Base::Console().error("Failed to load file %s\n", filename);
            return nullptr;
        }
        importVTKMesh(dataset, mesh);
    }
    else {
        Base::Console().error("file name extension is not supported\n");
        return nullptr;
    }
    // Mesh should link to the part feature, in order to set up FemConstraint

    Base::Console().log("    %f: Done \n", Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed()));
    return mesh;
}

void exportFemMeshEdges(
    vtkSmartPointer<vtkCellArray>& elemArray,
    std::vector<int>& types,
    const SMDS_EdgeIteratorPtr& aEdgeIter
)
{
    Base::Console().log("  Start: VTK mesh builder edges.\n");

    while (aEdgeIter->more()) {
        const SMDS_MeshEdge* aEdge = aEdgeIter->next();
        // edge
        if (aEdge->GetEntityType() == SMDSEntity_Edge) {
            fillVtkArray<vtkLine>(elemArray, types, aEdge);
        }
        // quadratic edge
        else if (aEdge->GetEntityType() == SMDSEntity_Quad_Edge) {
            fillVtkArray<vtkQuadraticEdge>(elemArray, types, aEdge);
        }
        else {
            throw Base::TypeError("Edge not yet supported by FreeCAD's VTK mesh builder\n");
        }
    }

    Base::Console().log("  End: VTK mesh builder edges.\n");
}

void exportFemMeshFaces(
    vtkSmartPointer<vtkCellArray>& elemArray,
    std::vector<int>& types,
    const SMDS_FaceIteratorPtr& aFaceIter
)
{
    Base::Console().log("  Start: VTK mesh builder faces.\n");

    while (aFaceIter->more()) {
        const SMDS_MeshFace* aFace = aFaceIter->next();
        // triangle
        if (aFace->GetEntityType() == SMDSEntity_Triangle) {
            fillVtkArray<vtkTriangle>(elemArray, types, aFace);
        }
        // quad
        else if (aFace->GetEntityType() == SMDSEntity_Quadrangle) {
            fillVtkArray<vtkQuad>(elemArray, types, aFace);
        }
        // quadratic triangle
        else if (aFace->GetEntityType() == SMDSEntity_Quad_Triangle) {
            fillVtkArray<vtkQuadraticTriangle>(elemArray, types, aFace);
        }
        // quadratic quad
        else if (aFace->GetEntityType() == SMDSEntity_Quad_Quadrangle) {
            fillVtkArray<vtkQuadraticQuad>(elemArray, types, aFace);
        }
        else {
            throw Base::TypeError("Face not yet supported by FreeCAD's VTK mesh builder\n");
        }
    }

    Base::Console().log("  End: VTK mesh builder faces.\n");
}

void exportFemMeshCells(
    vtkSmartPointer<vtkCellArray>& elemArray,
    std::vector<int>& types,
    const SMDS_VolumeIteratorPtr& aVolIter
)
{
    Base::Console().log("  Start: VTK mesh builder volumes.\n");

    while (aVolIter->more()) {
        const SMDS_MeshVolume* aVol = aVolIter->next();

        if (aVol->GetEntityType() == SMDSEntity_Tetra) {  // tetra4
            fillVtkArray<vtkTetra>(elemArray, types, aVol);
        }
        else if (aVol->GetEntityType() == SMDSEntity_Pyramid) {  // pyra5
            fillVtkArray<vtkPyramid>(elemArray, types, aVol);
        }
        else if (aVol->GetEntityType() == SMDSEntity_Penta) {  // penta6
            fillVtkArray<vtkWedge>(elemArray, types, aVol);
        }
        else if (aVol->GetEntityType() == SMDSEntity_Hexa) {  // hexa8
            fillVtkArray<vtkHexahedron>(elemArray, types, aVol);
        }
        else if (aVol->GetEntityType() == SMDSEntity_Quad_Tetra) {  // tetra10
            fillVtkArray<vtkQuadraticTetra>(elemArray, types, aVol);
        }
        else if (aVol->GetEntityType() == SMDSEntity_Quad_Pyramid) {  // pyra13
            fillVtkArray<vtkQuadraticPyramid>(elemArray, types, aVol);
        }
        else if (aVol->GetEntityType() == SMDSEntity_Quad_Penta) {  // penta15
            fillVtkArray<vtkQuadraticWedge>(elemArray, types, aVol);
        }
        else if (aVol->GetEntityType() == SMDSEntity_Quad_Hexa) {  // hexa20
            fillVtkArray<vtkQuadraticHexahedron>(elemArray, types, aVol);
        }
        else {
            throw Base::TypeError("Volume not yet supported by FreeCAD's VTK mesh builder\n");
        }
    }

    Base::Console().log("  End: VTK mesh builder volumes.\n");
}

void FemVTKTools::exportVTKMesh(
    const FemMesh* mesh,
    vtkSmartPointer<vtkUnstructuredGrid> grid,
    bool highest,
    float scale
)
{

    Base::Console().log("Start: VTK mesh builder ======================\n");
    const SMESH_Mesh* smesh = mesh->getSMesh();
    const SMESHDS_Mesh* meshDS = smesh->GetMeshDS();

    // nodes
    Base::Console().log("  Start: VTK mesh builder nodes.\n");

    vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
    SMDS_NodeIteratorPtr aNodeIter = meshDS->nodesIterator();

    while (aNodeIter->more()) {
        const SMDS_MeshNode* node = aNodeIter->next();  // why float, not double?
        double coords[3]
            = {double(node->X() * scale), double(node->Y() * scale), double(node->Z() * scale)};
        points->InsertPoint(node->GetID() - 1, coords);
        // memory is allocated by VTK points size for max node id, not for point count
        // if the SMESH mesh has gaps in node numbering, points without any element
        // assignment will be inserted in these point gaps too
        // this needs to be taken into account on node mapping when FreeCAD FEM results
        // are exported to vtk
    }
    grid->SetPoints(points);
    // nodes debugging
    const SMDS_MeshInfo& info = meshDS->GetMeshInfo();
    Base::Console().log("    Size of nodes in SMESH grid: %i.\n", info.NbNodes());
    const vtkIdType nNodes = grid->GetNumberOfPoints();
    Base::Console().log("    Size of nodes in VTK grid: %i.\n", nNodes);
    Base::Console().log("  End: VTK mesh builder nodes.\n");

    vtkSmartPointer<vtkCellArray> elemArray = vtkSmartPointer<vtkCellArray>::New();
    std::vector<int> types;

    if (highest) {
        // try volumes
        SMDS_VolumeIteratorPtr aVolIter = meshDS->volumesIterator();
        exportFemMeshCells(elemArray, types, aVolIter);
        // try faces
        if (elemArray->GetNumberOfCells() == 0) {
            SMDS_FaceIteratorPtr aFaceIter = meshDS->facesIterator();
            exportFemMeshFaces(elemArray, types, aFaceIter);
        }
        // try edges
        if (elemArray->GetNumberOfCells() == 0) {
            SMDS_EdgeIteratorPtr aEdgeIter = meshDS->edgesIterator();
            exportFemMeshEdges(elemArray, types, aEdgeIter);
        }
    }
    else {
        // export all elements
        // edges
        SMDS_EdgeIteratorPtr aEdgeIter = meshDS->edgesIterator();
        exportFemMeshEdges(elemArray, types, aEdgeIter);
        // faces
        SMDS_FaceIteratorPtr aFaceIter = meshDS->facesIterator();
        exportFemMeshFaces(elemArray, types, aFaceIter);
        // volumes
        SMDS_VolumeIteratorPtr aVolIter = meshDS->volumesIterator();
        exportFemMeshCells(elemArray, types, aVolIter);
    }

    if (elemArray->GetNumberOfCells() > 0) {
        grid->SetCells(types.data(), elemArray);
    }

    Base::Console().log("End: VTK mesh builder ======================\n");
}

void FemVTKTools::writeVTKMesh(const char* filename, const FemMesh* mesh, bool highest)
{

    Base::TimeElapsed Start;
    Base::Console().log("Start: write FemMesh from VTK unstructuredGrid ======================\n");
    Base::FileInfo f(filename);

    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    exportVTKMesh(mesh, grid, highest);
    Base::Console().log("Start: writing mesh data ======================\n");
    if (f.hasExtension("vtu")) {
        writeVTKFile<vtkXMLUnstructuredGridWriter>(filename, grid);
    }
    else if (f.hasExtension("vtk")) {
        writeVTKFile<vtkDataSetWriter>(filename, grid);
    }
    else {
        Base::Console().error("file name extension is not supported to write VTK\n");
    }

    Base::Console().log("    %f: Done \n", Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed()));
}


App::DocumentObject* getObjectByType(const Base::Type type)
{
    App::Document* pcDoc = App::GetApplication().getActiveDocument();
    if (!pcDoc) {
        Base::Console().message("No active document is found thus created\n");
        pcDoc = App::GetApplication().newDocument();
    }
    App::DocumentObject* obj = pcDoc->getActiveObject();

    if (obj->getTypeId() == type) {
        return obj;
    }
    if (obj->is<FemAnalysis>()) {
        std::vector<App::DocumentObject*> fem = (static_cast<FemAnalysis*>(obj))->Group.getValues();
        for (const auto& it : fem) {
            if (it->isDerivedFrom(type)) {
                return static_cast<App::DocumentObject*>(it);  // return the first of that type
            }
        }
    }
    return nullptr;
}


App::DocumentObject* createObjectByType(const Base::Type type)
{
    App::Document* pcDoc = App::GetApplication().getActiveDocument();
    if (!pcDoc) {
        Base::Console().message("No active document is found thus created\n");
        pcDoc = App::GetApplication().newDocument();
    }
    App::DocumentObject* obj = pcDoc->getActiveObject();

    if (obj->is<FemAnalysis>()) {
        App::DocumentObject* newobj = pcDoc->addObject(type.getName());
        static_cast<FemAnalysis*>(obj)->addObject(newobj);
        return newobj;
    }
    else {
        return pcDoc->addObject(type.getName());  // create in the active document
    }
}


App::DocumentObject* FemVTKTools::readResult(const char* filename, App::DocumentObject* res)
{
    Base::TimeElapsed Start;
    Base::Console().log("Start: read FemResult with FemMesh from VTK file ======================\n");
    Base::FileInfo f(filename);

    vtkSmartPointer<vtkDataSet> ds;
    if (f.hasExtension("vtu")) {
        ds = readVTKFile<vtkXMLUnstructuredGridReader>(filename);
    }
    else if (f.hasExtension("vtk")) {
        ds = readVTKFile<vtkDataSetReader>(filename);
    }
    else {
        Base::Console().error("file name extension is not supported\n");
    }

    App::Document* pcDoc = App::GetApplication().getActiveDocument();
    if (!pcDoc) {
        Base::Console().message("No active document is found thus created\n");
        pcDoc = App::GetApplication().newDocument();
    }
    App::DocumentObject* obj = pcDoc->getActiveObject();

    vtkSmartPointer<vtkDataSet> dataset = ds;
    App::DocumentObject* result = nullptr;

    if (res) {
        Base::Console().message("FemResultObject pointer is NULL, trying to get the active object\n");
        if (obj->getTypeId() == Base::Type::fromName("Fem::FemResultObjectPython")) {
            result = obj;
        }
        else {
            Base::Console().message("the active object is not the correct type, do nothing\n");
            return nullptr;
        }
    }

    auto* mesh = pcDoc->addObject<Fem::FemMeshObject>("ResultMesh");
    std::unique_ptr<FemMesh> fmesh(new FemMesh());
    importVTKMesh(dataset, fmesh.get());
    static_cast<PropertyFemMesh*>(mesh->getPropertyByName("FemMesh"))->setValuePtr(fmesh.release());

    if (result) {
        // PropertyLink is the property type to store DocumentObject pointer
        App::PropertyLink* link = dynamic_cast<App::PropertyLink*>(result->getPropertyByName("Mesh"));
        if (link) {
            link->setValue(mesh);
        }

        // vtkSmartPointer<vtkPointData> pd = dataset->GetPointData();
        importFreeCADResult(dataset, result);
    }

    pcDoc->recompute();
    Base::Console().log("    %f: Done \n", Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed()));
    Base::Console().log("End: read FemResult with FemMesh from VTK file ======================\n");

    return result;
}


void FemVTKTools::writeResult(const char* filename, const App::DocumentObject* res)
{
    if (!res) {
        App::Document* pcDoc = App::GetApplication().getActiveDocument();
        if (!pcDoc) {
            Base::Console().message("No active document is found thus do nothing and return\n");
            return;
        }
        res = pcDoc->getActiveObject();  // type checking is done by caller
    }
    if (!res) {
        Base::Console().error("Result object pointer is invalid and it is not active object");
        return;
    }

    Base::TimeElapsed Start;
    Base::Console().log("Start: write FemResult to VTK unstructuredGrid dataset =======\n");
    Base::FileInfo f(filename);

    // mesh
    vtkSmartPointer<vtkUnstructuredGrid> grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
    App::DocumentObject* mesh
        = static_cast<App::PropertyLink*>(res->getPropertyByName("Mesh"))->getValue();
    const FemMesh& fmesh
        = static_cast<PropertyFemMesh*>(mesh->getPropertyByName("FemMesh"))->getValue();
    FemVTKTools::exportVTKMesh(&fmesh, grid);

    Base::Console().log(
        "    %f: vtk mesh builder finished\n",
        Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed())
    );

    // result
    FemVTKTools::exportFreeCADResult(res, grid);

    if (f.hasExtension("vtu")) {
        writeVTKFile<vtkXMLUnstructuredGridWriter>(filename, grid);
    }
    else if (f.hasExtension("vtk")) {
        writeVTKFile<vtkDataSetWriter>(filename, grid);
    }
    else {
        Base::Console().error("file name extension is not supported to write VTK\n");
    }

    Base::Console().log(
        "    %f: writing result object to vtk finished\n",
        Base::TimeElapsed::diffTimeF(Start, Base::TimeElapsed())
    );
    Base::Console().log("End: write FemResult to VTK unstructuredGrid dataset =======\n");
}


std::map<std::string, std::string> _getFreeCADMechResultVectorProperties()
{
    // see src/Mod/Fem/femobjects/_FemResultMechanical
    // App::PropertyVectorList will be a list of vectors in vtk
    std::map<std::string, std::string> resFCVecProp;
    resFCVecProp["DisplacementVectors"] = "Displacement";
    // the following three are filled only if there is a reinforced mat object
    // https://forum.freecad.org/viewtopic.php?f=18&t=33106&start=70#p296317
    // https://forum.freecad.org/viewtopic.php?f=18&t=33106&p=416006#p412800
    resFCVecProp["PS1Vector"] = "Major Principal Stress Vector";
    resFCVecProp["PS2Vector"] = "Intermediate Principal Stress Vector";
    resFCVecProp["PS3Vector"] = "Minor Principal Stress Vector";
    resFCVecProp["HeatFlux"] = "Heat Flux";

    return resFCVecProp;
}

// see https://forum.freecad.org/viewtopic.php?f=18&t=33106&start=30#p277434 for further
// information regarding names etc...
// some scalar list are not needed on VTK file export but they are needed for internal VTK pipeline
// TODO some filter to only export the needed values to VTK file but have all
// in FreeCAD VTK pipeline
std::map<std::string, std::string> _getFreeCADMechResultScalarProperties()
{
    // see src/Mod/Fem/femobjects/result_mechanical.py
    // App::PropertyFloatList will be a list of scalars in vtk
    std::map<std::string, std::string> resFCScalProp;
    resFCScalProp["DisplacementLengths"] = "Displacement Magnitude";  // can be plotted in Paraview
                                                                      // as THE DISPLACEMENT MAGNITUDE
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
    resFCScalProp["CriticalStrainRatio"] = "Critical Strain Ratio";

    // the following three are filled in all cases
    // https://forum.freecad.org/viewtopic.php?f=18&t=33106&start=70#p296317
    // it might be these can be generated in paraview from stress tensor values as
    // THE MAJOR PRINCIPAL STRESS MAGNITUDE, THE INTERMEDIATE PRINCIPAL STRESS MAGNITUDE,
    // THE MINOR PRINCIPAL STRESS MAGNITUDE
    // but I do not know how (Bernd), for some help see paraview tutorial on FreeCAD wiki
    // thus TODO they might not be exported to external file format (first I need to know
    // how to generate them in paraview)
    // but there are needed anyway because the pipeline in FreeCAD needs the principal stress values
    // https://forum.freecad.org/viewtopic.php?f=18&t=33106&p=416006#p412800
    resFCScalProp["PrincipalMax"] = "Major Principal Stress";  // can be plotted in Paraview as THE
                                                               // MAJOR PRINCIPAL STRESS MAGNITUDE
    resFCScalProp["PrincipalMed"] = "Intermediate Principal Stress";  // can be plotted in Paraview
                                                                      // as THE INTERMEDIATE
                                                                      // PRINCIPAL STRESS MAGNITUDE
    resFCScalProp["PrincipalMin"] = "Minor Principal Stress";  // can be plotted in Paraview as THE
                                                               // MINOR PRINCIPAL STRESS MAGNITUDE
    resFCScalProp["vonMises"] = "von Mises Stress";
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


void FemVTKTools::importFreeCADResult(vtkSmartPointer<vtkDataSet> dataset, App::DocumentObject* result)
{
    Base::Console().log("Start: import vtk result file data into a FreeCAD result object.\n");

    std::map<std::string, std::string> vectors = _getFreeCADMechResultVectorProperties();
    std::map<std::string, std::string> scalars = _getFreeCADMechResultScalarProperties();

    double ts = 0.0;  // t=0.0 for static simulation
    static_cast<App::PropertyFloat*>(result->getPropertyByName("Time"))->setValue(ts);

    vtkSmartPointer<vtkPointData> pd = dataset->GetPointData();
    if (pd->GetNumberOfArrays() == 0) {
        Base::Console().error("No point data array is found in vtk data set, do nothing\n");
        // if pointData is empty, data may be in cellDate,
        // cellData -> pointData interpolation is possible in VTK
        return;
    }

    // NodeNumbers
    const vtkIdType nPoints = dataset->GetNumberOfPoints();
    std::vector<long> nodeIds(nPoints);
    for (vtkIdType i = 0; i < nPoints; ++i) {
        nodeIds[i] = i + 1;
    }
    static_cast<App::PropertyIntegerList*>(result->getPropertyByName("NodeNumbers"))->setValues(nodeIds);
    Base::Console().log("    NodeNumbers have been filled with values.\n");

    // vectors
    for (const auto& it : vectors) {
        int dim = 3;  // Fixme: currently 3D only, here we could run into trouble,
                      //        FreeCAD only supports dim 3D, I do not know about VTK
        vtkDataArray* vector_field = vtkDataArray::SafeDownCast(pd->GetArray(it.second.c_str()));
        if (vector_field && vector_field->GetNumberOfComponents() == dim) {
            App::PropertyVectorList* vector_list = static_cast<App::PropertyVectorList*>(
                result->getPropertyByName(it.first.c_str())
            );
            if (vector_list) {
                std::vector<Base::Vector3d> vec(nPoints);
                for (vtkIdType i = 0; i < nPoints; ++i) {
                    double* p = vector_field->GetTuple(i);  // both vtkFloatArray and vtkDoubleArray
                                                            // return double* for GetTuple(i)
                    vec[i] = (Base::Vector3d(p[0], p[1], p[2]));
                }
                // PropertyVectorList will not show up in PropertyEditor
                vector_list->setValues(vec);
                Base::Console().log(
                    "    A PropertyVectorList has been filled with values: %s\n",
                    it.first.c_str()
                );
            }
            else {
                Base::Console().error(
                    "static_cast<App::PropertyVectorList*>((result->"
                    "getPropertyByName(\"%s\")) failed.\n",
                    it.first.c_str()
                );
                continue;
            }
        }
        else {
            Base::Console().message(
                "    PropertyVectorList NOT found in vkt file data: %s\n",
                it.first.c_str()
            );
        }
    }

    // scalars
    for (const auto& scalar : scalars) {
        vtkDataArray* vec = vtkDataArray::SafeDownCast(pd->GetArray(scalar.second.c_str()));
        if (nPoints && vec && vec->GetNumberOfComponents() == 1) {
            App::PropertyFloatList* field = static_cast<App::PropertyFloatList*>(
                result->getPropertyByName(scalar.first.c_str())
            );
            if (!field) {
                Base::Console().error(
                    "static_cast<App::PropertyFloatList*>((result->"
                    "getPropertyByName(\"%s\")) failed.\n",
                    scalar.first.c_str()
                );
                continue;
            }

            double vmin = 1.0e100, vmax = -1.0e100;
            std::vector<double> values(nPoints, 0.0);
            for (vtkIdType i = 0; i < vec->GetNumberOfTuples(); i++) {
                double v = *(vec->GetTuple(i));
                values[i] = v;
                if (v > vmax) {
                    vmax = v;
                }
                if (v < vmin) {
                    vmin = v;
                }
            }
            field->setValues(values);
            Base::Console().log(
                "    A PropertyFloatList has been filled with vales: %s\n",
                scalar.first.c_str()
            );
        }
        else {
            Base::Console().message(
                "    PropertyFloatList NOT found in vkt file data %s\n",
                scalar.first.c_str()
            );
        }
    }

    // stats
    // stats are added by importVTKResults

    Base::Console().log("End: import vtk result file data into a FreeCAD result object.\n");
}


void FemVTKTools::exportFreeCADResult(const App::DocumentObject* result, vtkSmartPointer<vtkDataSet> grid)
{
    Base::Console().log("Start: Create VTK result data from FreeCAD result data.\n");

    std::map<std::string, std::string> vectors = _getFreeCADMechResultVectorProperties();
    std::map<std::string, std::string> scalars = _getFreeCADMechResultScalarProperties();

    const Fem::FemResultObject* res = static_cast<const Fem::FemResultObject*>(result);
    const vtkIdType nPoints = grid->GetNumberOfPoints();

    // we need the corresponding mesh to get the correct id for the result data
    // (when the freecad smesh mesh has gaps in the points
    // vtk has more points. Vtk does not support point gaps, thus the gaps are
    // filled with points. Then the mapping must be correct)
    App::DocumentObject* meshObj = res->Mesh.getValue();
    if (!meshObj || !meshObj->isDerivedFrom<FemMeshObject>()) {
        Base::Console().error("Result object does not correctly link to mesh");
        return;
    }
    const SMESH_Mesh* smesh = static_cast<FemMeshObject*>(meshObj)->FemMesh.getValue().getSMesh();
    const SMESHDS_Mesh* meshDS = smesh->GetMeshDS();

    // all result object meshes are in mm therefore for e.g. length outputs like
    // displacement we must divide by 1000
    double factor = 1.0;

    // vectors
    for (const auto& it : vectors) {
        const int dim = 3;  // Fixme, detect dim, but FreeCAD PropertyVectorList ATM only has DIM of 3
        App::PropertyVectorList* field = nullptr;
        if (res->getPropertyByName(it.first.c_str())) {
            field = static_cast<App::PropertyVectorList*>(res->getPropertyByName(it.first.c_str()));
        }
        else {
            Base::Console().error("    PropertyVectorList not found: %s\n", it.first.c_str());
        }

        if (field && field->getSize() > 0) {
            const std::vector<Base::Vector3d>& vel = field->getValues();
            vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
            data->SetNumberOfComponents(dim);
            data->SetNumberOfTuples(nPoints);
            data->SetName(it.second.c_str());

            // we need to set values for the unused points.
            // TODO: ensure that the result bar does not include the used 0 if it is not
            // part of the result (e.g. does the result bar show 0 as smallest value?)
            if (nPoints != field->getSize()) {
                double tuple[] = {0, 0, 0};
                for (vtkIdType i = 0; i < nPoints; ++i) {
                    data->SetTuple(i, tuple);
                }
            }

            if (it.first.compare("DisplacementVectors") == 0) {
                factor = 0.001;  // to get meter
            }
            else {
                factor = 1.0;
            }

            SMDS_NodeIteratorPtr aNodeIter = meshDS->nodesIterator();
            for (const auto& jt : vel) {
                const SMDS_MeshNode* node = aNodeIter->next();
                double tuple[] = {jt.x * factor, jt.y * factor, jt.z * factor};
                data->SetTuple(node->GetID() - 1, tuple);
            }
            grid->GetPointData()->AddArray(data);
            Base::Console().log(
                "    The PropertyVectorList %s was exported to VTK vector list: %s\n",
                it.first.c_str(),
                it.second.c_str()
            );
        }
        else if (field) {
            Base::Console().log(
                "    PropertyVectorList NOT exported to vtk: %s size is: %i\n",
                it.first.c_str(),
                field->getSize()
            );
        }
    }

    // scalars
    for (const auto& scalar : scalars) {
        App::PropertyFloatList* field = nullptr;
        if (res->getPropertyByName(scalar.first.c_str())) {
            field = static_cast<App::PropertyFloatList*>(res->getPropertyByName(scalar.first.c_str()));
        }
        else {
            Base::Console().error("PropertyFloatList %s not found \n", scalar.first.c_str());
        }

        if (field && field->getSize() > 0) {
            const std::vector<double>& vec = field->getValues();
            vtkSmartPointer<vtkDoubleArray> data = vtkSmartPointer<vtkDoubleArray>::New();
            data->SetNumberOfValues(nPoints);
            data->SetName(scalar.second.c_str());

            // we need to set values for the unused points.
            // TODO: ensure that the result bar does not include the used 0 if it is not part
            // of the result (e.g. does the result bar show 0 as smallest value?)
            if (nPoints != field->getSize()) {
                for (vtkIdType i = 0; i < nPoints; ++i) {
                    data->SetValue(i, 0);
                }
            }

            if ((scalar.first.compare("MaxShear") == 0) || (scalar.first.compare("NodeStressXX") == 0)
                || (scalar.first.compare("NodeStressXY") == 0)
                || (scalar.first.compare("NodeStressXZ") == 0)
                || (scalar.first.compare("NodeStressYY") == 0)
                || (scalar.first.compare("NodeStressYZ") == 0)
                || (scalar.first.compare("NodeStressZZ") == 0)
                || (scalar.first.compare("PrincipalMax") == 0)
                || (scalar.first.compare("PrincipalMed") == 0)
                || (scalar.first.compare("PrincipalMin") == 0)
                || (scalar.first.compare("vonMises") == 0)
                || (scalar.first.compare("NetworkPressure") == 0)) {
                factor = 1e6;  // to get Pascal
            }
            else if (scalar.first.compare("DisplacementLengths") == 0) {
                factor = 0.001;  // to get meter
            }
            else {
                factor = 1.0;
            }

            SMDS_NodeIteratorPtr aNodeIter = meshDS->nodesIterator();
            for (double i : vec) {
                const SMDS_MeshNode* node = aNodeIter->next();
                // for the MassFlowRate the last vec entries can be a nullptr, thus check this
                if (node) {
                    data->SetValue(node->GetID() - 1, i * factor);
                }
            }

            grid->GetPointData()->AddArray(data);
            Base::Console().log(
                "    The PropertyFloatList %s was exported to VTK scalar list: %s\n",
                scalar.first.c_str(),
                scalar.second.c_str()
            );
        }
        else if (field) {
            Base::Console().log(
                "    PropertyFloatList NOT exported to vtk: %s size is: %i\n",
                scalar.first.c_str(),
                field->getSize()
            );
        }
    }

    Base::Console().log("End: Create VTK result data from FreeCAD result data.\n");
}


namespace FRDReader
{

enum class ElementType
{
    Edge = 11,
    QuadEdge = 12,
    Triangle = 7,
    QuadTriangle = 8,
    Quadrangle = 9,
    QuadQuadrangle = 10,
    Tetra = 3,
    QuadTetra = 6,
    Hexa = 1,
    QuadHexa = 4,
    Penta = 2,
    QuadPenta = 5
};

enum class AnalysisType
{
    Static = 0,
    TimeStep = 1,
    Frequency = 2,
    LoadStep = 3,
    UserNamed = 4
};

std::map<AnalysisType, std::string> mapAnalysisTypeToStr = {
    {AnalysisType::Static, "Static"},
    {AnalysisType::TimeStep, "TimeStep"},
    {AnalysisType::Frequency, "Frequency"},
    {AnalysisType::LoadStep, "LoadStep"},
    {AnalysisType::UserNamed, "User"}
};

// value format indicator
enum class Indicator
{
    Short = 0,
    Long = 1,
    // BinaryFloat = 2, not used
    // BinaryDouble = 3 not used
};

// number of nodes per CalculiX element type: {type, nodes}
std::map<ElementType, unsigned int> mapCcxTypeNodes = {
    {ElementType::Edge, 2},
    {ElementType::QuadEdge, 3},
    {ElementType::Triangle, 3},
    {ElementType::QuadTriangle, 6},
    {ElementType::Quadrangle, 4},
    {ElementType::QuadQuadrangle, 8},
    {ElementType::Tetra, 4},
    {ElementType::QuadTetra, 10},
    {ElementType::Hexa, 8},
    {ElementType::QuadHexa, 20},
    {ElementType::Penta, 6},
    {ElementType::QuadPenta, 15},
};

// map CalculiX nodes order to Vtk order
std::map<int, std::vector<int>> mapCcxToVtk = {
    {VTK_LINE, {0, 1}},
    {VTK_QUADRATIC_EDGE, {0, 1, 2}},
    {VTK_TRIANGLE, {0, 1, 2}},
    {VTK_QUADRATIC_TRIANGLE, {0, 1, 2, 3, 4, 5}},
    {VTK_QUAD, {0, 1, 2, 3}},
    {VTK_QUADRATIC_QUAD, {0, 1, 2, 3, 4, 5, 6, 7}},
    {VTK_TETRA, {0, 1, 2, 3}},
    {VTK_QUADRATIC_TETRA, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9}},
    {VTK_HEXAHEDRON, {0, 1, 2, 3, 4, 5, 6, 7}},
    {VTK_QUADRATIC_HEXAHEDRON,
     {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 16, 17, 18, 19, 12, 13, 14, 15}},
    {VTK_WEDGE, {0, 1, 2, 3, 4, 5}},
    {VTK_QUADRATIC_WEDGE, {0, 1, 2, 3, 4, 5, 6, 7, 8, 12, 13, 14, 9, 10, 11}}
};

// give position of first non-blank character of string_view
size_t getFirstNotBlankPos(const std::string_view& view)
{
    size_t pos = view.find_first_not_of(" ");
    if (pos == std::string_view::npos) {
        pos = 0;
    }

    return pos;
}

// get n-digits value from string_view
// not used until libc++ std::from_chars supports double values
// template<typename T>
// void valueFromLine(const std::string_view::const_iterator& it, int digits, T& value)
//{
//    std::string_view sub(it, digits);
//    auto pos = getFirstNotBlankPos(sub);
//    std::from_chars(sub.data() + pos, sub.data() + digits, value, 10);
//}
template<typename T>
void valueFromLine(const std::string_view::iterator& it, int digits, T& value)
{
    std::string_view sub(&*it, digits);
    value = std::strtol(sub.data(), nullptr, 10);
}
template<>
void valueFromLine<double>(const std::string_view::iterator& it, int digits, double& value)
{
    std::string_view sub(&*it, digits);
    value = std::strtof(sub.data(), nullptr);
}

// add cell from sorted nodes
template<typename T>
void addCell(vtkSmartPointer<vtkCellArray>& cellArray, const std::vector<int>& topoElem)
{
    vtkSmartPointer<T> cell = vtkSmartPointer<T>::New();
    cell->GetPointIds()->SetNumberOfIds(topoElem.size());
    int type = cell->GetCellType();
    for (size_t i = 0; i < topoElem.size(); ++i) {
        cell->GetPointIds()->SetId(i, topoElem[mapCcxToVtk[type][i]]);
    }
    cellArray->InsertNextCell(cell);
}

// fill cell array
void fillCell(
    vtkSmartPointer<vtkCellArray>& cellArray,
    std::vector<int>& topoElem,
    std::vector<int>& vtkType,
    ElementType elemType
)
{
    switch (elemType) {
        case ElementType::Hexa:
            addCell<vtkHexahedron>(cellArray, topoElem);
            vtkType.emplace_back(VTK_HEXAHEDRON);
            break;
        case ElementType::Penta:
            addCell<vtkWedge>(cellArray, topoElem);
            vtkType.emplace_back(VTK_WEDGE);
            break;
        case ElementType::Tetra:
            addCell<vtkTetra>(cellArray, topoElem);
            vtkType.emplace_back(VTK_TETRA);
            break;
        case ElementType::QuadHexa:
            addCell<vtkQuadraticHexahedron>(cellArray, topoElem);
            vtkType.emplace_back(VTK_QUADRATIC_HEXAHEDRON);
            break;
        case ElementType::QuadPenta:
            addCell<vtkQuadraticWedge>(cellArray, topoElem);
            vtkType.emplace_back(VTK_QUADRATIC_WEDGE);
            break;
        case ElementType::QuadTetra:
            addCell<vtkQuadraticTetra>(cellArray, topoElem);
            vtkType.emplace_back(VTK_QUADRATIC_TETRA);
            break;
        case ElementType::Triangle:
            addCell<vtkTriangle>(cellArray, topoElem);
            vtkType.emplace_back(VTK_TRIANGLE);
            break;
        case ElementType::QuadTriangle:
            addCell<vtkQuadraticTriangle>(cellArray, topoElem);
            vtkType.emplace_back(VTK_QUADRATIC_TRIANGLE);
            break;
        case ElementType::Quadrangle:
            addCell<vtkQuad>(cellArray, topoElem);
            vtkType.emplace_back(VTK_QUAD);
            break;
        case ElementType::QuadQuadrangle:
            addCell<vtkQuadraticQuad>(cellArray, topoElem);
            vtkType.emplace_back(VTK_QUADRATIC_QUAD);
            break;
        case ElementType::Edge:
            addCell<vtkLine>(cellArray, topoElem);
            vtkType.emplace_back(VTK_LINE);
            break;
        case ElementType::QuadEdge:
            addCell<vtkQuadraticEdge>(cellArray, topoElem);
            vtkType.emplace_back(VTK_QUADRATIC_EDGE);
            break;
    }
}

struct FRDResultInfo
{
    double value;
    long numNodes;
    AnalysisType analysisType;
    int step;
    Indicator indicator;

    bool operator==(const FRDResultInfo& other) const
    {
        return (this->step == other.step) && (this->analysisType == other.analysisType);
    }
    bool operator<(const FRDResultInfo& other) const
    {
        if (this->step < other.step) {
            return true;
        }
        else if (this->step > other.step) {
            return false;
        }
        else if (static_cast<int>(this->analysisType) < static_cast<int>(other.analysisType)) {
            return true;
        }
        else {
            return false;
        }
    }
};

// get number of digits from format indicator
int getDigits(Indicator indicator)
{
    int digits = 0;
    switch (indicator) {
        case Indicator::Short:
            digits = 5;
            break;
        case Indicator::Long:
            digits = 10;
            break;
    }

    return digits;
}

// get position of scalar entities in line result vector
std::vector<size_t> identifyScalarEntities(const std::vector<std::vector<int>> entities)
{
    std::vector<size_t> pos;
    for (auto it = entities.begin(); it != entities.end(); ++it) {
        // check type == 1 or component < 1
        if ((*it)[0] == 1 || (*it)[1] < 1) {
            pos.emplace_back(it - entities.begin());
        }
    }
    return pos;
}

// read nodes and fill vtkPoints object
std::map<int, int> readNodes(
    std::ifstream& ifstr,
    const std::string& lines,
    vtkSmartPointer<vtkPoints>& points
)
{
    std::string keyCode = "    2C";
    std::string keyCodeCoord = " -1";
    long numNodes {0};
    int indicator {0};
    int node {0};
    long nodeID = 0;

    // frd file might have nodes that are not numbered starting from zero.
    // Use the map to identify them
    std::map<int, int> mapNodes;

    std::string_view view {lines};
    std::string_view sub = view.substr(keyCode.length() + 18);

    valueFromLine(sub.begin(), 12, numNodes);

    sub = sub.substr(12 + 37);
    valueFromLine(sub.begin(), 1, indicator);
    int digits = getDigits(static_cast<Indicator>(indicator));

    points->SetNumberOfPoints(numNodes);

    std::string line;
    while (nodeID < numNodes && std::getline(ifstr, line)) {
        std::vector<double> coords;
        std::string_view view {line};
        if (view.rfind(keyCodeCoord, 0) == 0) {
            std::string_view v(line.data() + keyCodeCoord.length(), digits);
            valueFromLine(v.begin(), digits, node);

            std::string_view vi = view.substr(keyCodeCoord.length() + digits);
            double value;
            for (auto it = vi.begin(); it != vi.end(); it += 12) {
                valueFromLine(it, 12, value);
                coords.emplace_back(value);
            }
        }

        points->SetPoint(nodeID, coords.data());
        mapNodes[node] = nodeID++;
    }

    return mapNodes;
}

// fill elements and fill cell array
std::vector<int> readElements(
    std::ifstream& ifstr,
    const std::string& lines,
    const std::map<int, int>& mapNodes,
    vtkSmartPointer<vtkCellArray>& cellArray
)
{
    std::string line;
    std::string keyCode = "    3C";
    std::string keyCodeType = " -1";
    std::string keyCodeNodes = " -2";
    long numElem;
    int indicator;
    int elem;
    long elemID = 0;
    // element info: {type, group, material}
    std::vector<int> info(3);
    std::map<int, int> mapElem;
    std::vector<int> topoElem;
    std::vector<int> vtkType;

    std::string_view view {lines};

    std::string_view sub = view.substr(keyCode.length() + 18);
    valueFromLine(sub.begin(), 12, numElem);

    sub = sub.substr(12 + 37);
    valueFromLine(sub.begin(), 1, indicator);
    int digits = getDigits(static_cast<Indicator>(indicator));
    while (elemID < numElem && std::getline(ifstr, line)) {
        std::string_view view {line};
        if (view.rfind(keyCodeType, 0) == 0) {
            std::string_view v(line.data() + keyCodeType.length());
            valueFromLine(v.begin(), digits, elem);
            v = v.substr(digits);
            std::string_view::iterator it1;
            std::vector<int>::iterator it2;
            for (it1 = v.begin(), it2 = info.begin(); it1 != v.end() && it2 != info.end();
                 it1 += 5, ++it2) {
                valueFromLine(it1, 5, *it2);
            }
        }
        if (view.rfind(keyCodeNodes, 0) == 0) {
            std::string_view vi = view.substr(keyCodeNodes.length());
            int node;
            for (auto it = vi.begin(); it != vi.end(); it += digits) {
                valueFromLine(it, digits, node);
                topoElem.emplace_back(mapNodes.at(node));
            }

            // add cell to cellArray
            if (topoElem.size() == mapCcxTypeNodes[static_cast<ElementType>(info[0])]) {
                fillCell(cellArray, topoElem, vtkType, static_cast<ElementType>(info[0]));
                topoElem.clear();
                mapElem[elem] = elemID++;
            }
        }
    }
    return vtkType;
}

// read parameter header (not used)
void readParameter([[maybe_unused]] std::ifstream& ifstr, [[maybe_unused]] const std::string& line)
{}

// read first header from nodal result block
void readResultInfo([[maybe_unused]] std::ifstream& ifstr, const std::string& lines, FRDResultInfo& info)
{
    std::string keyCode = "  100C";

    std::string_view view {lines};
    std::string_view sub = view.substr(keyCode.length() + 6);
    valueFromLine(sub.begin(), 12, info.value);

    sub = sub.substr(12);
    valueFromLine(sub.begin(), 12, info.numNodes);

    sub = sub.substr(12 + 20);
    int anType;
    valueFromLine(sub.begin(), 2, anType);
    info.analysisType = static_cast<AnalysisType>(anType);

    sub = sub.substr(2);
    valueFromLine(sub.begin(), 5, info.step);

    sub = sub.substr(5 + 10);
    int ind;
    valueFromLine(sub.begin(), 2, ind);
    info.indicator = static_cast<Indicator>(ind);
}

// read result from nodal result block and add result array to grid
void readResults(
    std::ifstream& ifstr,
    [[maybe_unused]] const std::string& lines,
    const std::map<int, int>& mapNodes,
    const FRDResultInfo& info,
    vtkSmartPointer<vtkUnstructuredGrid>& grid
)
{
    int digits = getDigits(info.indicator);

    // get dataset info, start with " -4"
    std::string line;
    std::string keyDataSet = " -4";
    unsigned int numComps;
    std::getline(ifstr, line);
    std::string_view view = line;
    std::string_view sub = view.substr(keyDataSet.length() + 2);
    std::string dataSetName {sub.substr(0, 8)};
    // remove trailing spaces
    dataSetName.erase(dataSetName.find_last_not_of(" ") + 1);
    sub = sub.substr(8);
    valueFromLine(sub.begin(), 5, numComps);

    // get entity info
    std::string keyEntity = " -5";
    std::vector<std::string> entityNames;
    // type: 1: scalar; 2: vector; 4: matrix; 12: vector (3 amp - 3 phase); 14: tensor (6 amp - 6
    // phase) {type, row, col, exist}
    std::vector<std::vector<int>> entityTypes;
    unsigned int countComp = 0;
    while (countComp < numComps && std::getline(ifstr, line)) {
        std::string_view view {line};
        if (view.rfind(keyEntity, 0) == 0) {
            sub = view.substr(keyEntity.length() + 2);
            std::string en {sub.substr(0, 8)};
            // remove trailing spaces
            en.erase(en.find_last_not_of(" ") + 1);
            std::vector<int> et = {0, 0, 0, 0};
            // fill entityType, ignore MENU: "    1"
            sub = sub.substr(8 + 5, 4 * 5);
            std::string_view::iterator it1;
            std::vector<int>::iterator it2;
            for (it1 = sub.begin(), it2 = et.begin(); it1 != sub.end() && it2 != et.end();
                 (it1 += 5), ++it2) {
                valueFromLine(it1, digits, *it2);
            }

            if (et[3] == 0) {
                // ignore predefined entity
                entityNames.emplace_back(en);
                entityTypes.emplace_back(et);
            }
            ++countComp;
        }
    }

    // used components
    numComps = entityNames.size();

    // enter in node values block
    std::string code1 = " -1";
    std::string code2 = " -2";
    int node {-1};
    double value {0.0};
    std::vector<double> vecValues;
    std::vector<double> scaValues;
    std::vector<int> nodes;
    int countNodes = 0;
    size_t countScaPos {0};
    // result block could have both vector/matrix and scalar components
    // save each scalars entity in his own array
    auto scalarPos = identifyScalarEntities(entityTypes);
    // array for vector entities (if needed)
    vtkSmartPointer<vtkDoubleArray> vecArray = vtkSmartPointer<vtkDoubleArray>::New();
    // arrays for scalar entities (if needed)
    std::vector<vtkSmartPointer<vtkDoubleArray>> scaArrays;
    for (size_t i = 0; i < scalarPos.size(); ++i) {
        scaArrays.emplace_back(vtkSmartPointer<vtkDoubleArray>::New());
    }

    vecArray->SetNumberOfComponents(numComps - scalarPos.size());
    vecArray->SetNumberOfTuples(mapNodes.size());
    vecArray->SetName(dataSetName.c_str());
    // set all values to zero
    for (int i = 0; i < vecArray->GetNumberOfComponents(); ++i) {
        vecArray->FillComponent(i, 0.0);
    }
    //    vecArray->Fill(0.0);
    for (size_t i = 0; i < scaArrays.size(); ++i) {
        scaArrays[i]->SetNumberOfComponents(1);
        scaArrays[i]->SetNumberOfTuples(mapNodes.size());
        std::string name = entityNames[scalarPos[i]];
        scaArrays[i]->SetName(name.c_str());
        //        scaArrays[i]->Fill(0.0);
        for (int j = 0; j < scaArrays[i]->GetNumberOfComponents(); ++j) {
            scaArrays[i]->FillComponent(j, 0.0);
        }
    }

    while (countNodes < info.numNodes && std::getline(ifstr, line)) {
        std::string_view view {line};
        if (view.rfind(code1, 0) == 0) {
            sub = view.substr(code1.length());
            valueFromLine(sub.begin(), digits, node);
            // clear values vector for each node result block
            vecValues.clear();
            scaValues.clear();
            countScaPos = 0;
            try {
                // result nodes could not exist in .frd file due to element expansion
                // so mapNodes.at() could throw an exception
                nodes.emplace_back(mapNodes.at(node));
                sub = sub.substr(digits);
                for (auto it = sub.begin(); it != sub.end(); it += 12, ++countScaPos) {
                    valueFromLine(it, 12, value);
                    // search if value is scalar or vector/matrix component
                    auto pos = std::ranges::find(scalarPos, countScaPos);
                    if (pos == scalarPos.end()) {
                        vecValues.emplace_back(value);
                    }
                    else {
                        scaValues.emplace_back(value);
                    }
                }
            }
            catch (const std::out_of_range&) {
                Base::Console().warning("Invalid node: %d\n", node);
            }
            ++countNodes;
        }
        else if (view.rfind(code2, 0) == 0) {
            sub = view.substr(code2.length() + digits);
            for (auto it = sub.begin(); it != sub.end(); it += 12) {
                valueFromLine(it, 12, value);
                // search if value is scalar or vector/matrix component
                auto pos = std::ranges::find(scalarPos, countScaPos);
                if (pos == scalarPos.end()) {
                    vecValues.emplace_back(value);
                }
                else {
                    scaValues.emplace_back(value);
                }
            }
        }
        if ((vecValues.size() + scaValues.size()) == numComps) {
            if (!vecValues.empty()) {
                if (node == -1) {
                    throw Base::FileException("File to load not readable");
                }
                vecArray->SetTuple(mapNodes.at(node), vecValues.data());
            }
            if (!scaValues.empty()) {
                if (node == -1) {
                    throw Base::FileException("File to load not readable");
                }
                std::vector<vtkSmartPointer<vtkDoubleArray>>::iterator it1;
                std::vector<double>::iterator it2;
                for (it1 = scaArrays.begin(), it2 = scaValues.begin();
                     it1 != scaArrays.end() && it2 != scaValues.end();
                     ++it1, ++it2) {
                    (*it1)->SetTuple1(mapNodes.at(node), *it2);
                }
            }
        }
    }

    // add vecArray only if not all scalars
    if (numComps != scalarPos.size()) {
        grid->GetPointData()->AddArray(vecArray);
    }
    for (auto& s : scaArrays) {
        grid->GetPointData()->AddArray(s);
    }
}
vtkSmartPointer<vtkStringArray> createTimeInfo(const std::string& type)
{
    auto timeInfo = vtkSmartPointer<vtkStringArray>::New();
    timeInfo->SetName("TimeInfo");
    timeInfo->InsertNextValue(type);
    // set unit to empty string
    timeInfo->InsertNextValue("");

    return timeInfo;
}

vtkSmartPointer<vtkFloatArray> createTimeValue(const double& value)
{
    auto stepValue = vtkSmartPointer<vtkFloatArray>::New();
    stepValue->SetName("TimeValue");
    stepValue->InsertNextValue(value);

    return stepValue;
}

vtkSmartPointer<vtkMultiBlockDataSet> readFRD(std::ifstream& ifstr)
{
    auto points = vtkSmartPointer<vtkPoints>::New();
    auto cells = vtkSmartPointer<vtkCellArray>::New();
    auto multiBlock = vtkSmartPointer<vtkMultiBlockDataSet>::New();
    vtkSmartPointer<vtkUnstructuredGrid> grid;
    vtkSmartPointer<vtkMultiBlockDataSet> block;
    std::map<FRDResultInfo, vtkSmartPointer<vtkUnstructuredGrid>> grids;
    std::map<AnalysisType, vtkSmartPointer<vtkMultiBlockDataSet>> blocks;
    std::string line;
    std::map<int, int> mapNodes;
    std::vector<int> cellTypes;

    while (std::getline(ifstr, line)) {
        std::string keyCode = "    2C";
        std::string_view view = line;

        if (view.rfind(keyCode, 0) == 0) {
            // read nodes block
            mapNodes = readNodes(ifstr, line, points);
        }
        keyCode = "    3C";
        if (view.rfind(keyCode, 0) == 0) {
            // read elements block
            cellTypes = readElements(ifstr, line, mapNodes, cells);
        }
        keyCode = "    1P";
        if (view.rfind(keyCode, 0) == 0) {
            // read parameter
            readParameter(ifstr, line);
        }
        keyCode = "  100C";
        if (view.rfind(keyCode, 0) == 0) {
            // read result info block
            FRDResultInfo info;
            readResultInfo(ifstr, line, info);
            auto it = grids.find(info);
            if (it == grids.end()) {
                // create TimeInfo metadata
                auto timeInfo = createTimeInfo(mapAnalysisTypeToStr[info.analysisType]);
                // search analysis type block and create it if necessary
                auto it2 = blocks.find(info.analysisType);
                if (it2 == blocks.end()) {
                    block = vtkSmartPointer<vtkMultiBlockDataSet>::New();
                    block->GetFieldData()->AddArray(timeInfo);
                    blocks[info.analysisType] = block;
                }
                else {
                    block = it2->second;
                }
                // create unstructured grid
                grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
                grid->SetPoints(points);
                grid->SetCells(cellTypes.data(), cells);

                // create TimeValue metadata
                auto stepValue = createTimeValue(info.value);

                grid->GetFieldData()->AddArray(stepValue);
                grid->GetFieldData()->AddArray(timeInfo);

                grids[info] = grid;
                unsigned int nb = block->GetNumberOfBlocks();
                block->SetBlock(nb, grid);
            }
            else {
                grid = (*it).second;
            }
            // read result entries and node results
            readResults(ifstr, line, mapNodes, info, grid);
        }
    }
    int i = 0;

    for (const auto& b : blocks) {
        multiBlock->SetBlock(i, b.second);
        ++i;
    }

    // save points and elements even without results
    if (grids.empty()) {
        block = vtkSmartPointer<vtkMultiBlockDataSet>::New();
        grid = vtkSmartPointer<vtkUnstructuredGrid>::New();
        grid->SetPoints(points);
        grid->SetCells(cellTypes.data(), cells);
        auto timeInfo = createTimeInfo("");
        auto stepValue = createTimeValue(0);
        grid->GetFieldData()->AddArray(stepValue);
        grid->GetFieldData()->AddArray(timeInfo);

        block->SetBlock(0, grid);
        block->GetFieldData()->AddArray(timeInfo);
        multiBlock->SetBlock(0, block);
    }

    return multiBlock;
}

}  // namespace FRDReader

void FemVTKTools::frdToVTK(const char* filename, bool binary)
{
    Base::FileInfo fi(filename);

    if (!fi.isReadable()) {
        throw Base::FileException("File to load not existing or not readable", filename);
    }

    std::ifstream ifstr(filename, std::ios::in | std::ios::binary);

    vtkSmartPointer<vtkMultiBlockDataSet> multiBlock = FRDReader::readFRD(ifstr);

    std::string dir = fi.dirPath();

    for (unsigned int i = 0; i < multiBlock->GetNumberOfBlocks(); ++i) {
        vtkDataObject* block = multiBlock->GetBlock(i);
        // get TimeInfo
        vtkSmartPointer<vtkStringArray> info = vtkStringArray::SafeDownCast(
            block->GetFieldData()->GetAbstractArray(0)
        );
        std::string type = info->GetValue(0).c_str();

        auto writer = vtkSmartPointer<vtkXMLMultiBlockDataWriter>::New();
        writer->SetDataMode(
            binary ? vtkXMLMultiBlockDataWriter::Binary : vtkXMLMultiBlockDataWriter::Ascii
        );

        std::string blockFile = dir + "/" + fi.fileNamePure() + type + "."
            + writer->GetDefaultFileExtension();
        writer->SetFileName(blockFile.c_str());
        writer->SetInputData(block);
        writer->Update();
    }
}

}  // namespace Fem
