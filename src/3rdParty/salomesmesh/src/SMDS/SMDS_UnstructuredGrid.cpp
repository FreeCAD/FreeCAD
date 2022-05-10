// Copyright (C) 2010-2015  CEA/DEN, EDF R&D, OPEN CASCADE
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

#include "SMDS_UnstructuredGrid.hxx"
#include "SMDS_Mesh.hxx"
#include "SMDS_MeshInfo.hxx"
#include "SMDS_Downward.hxx"
#include "SMDS_MeshVolume.hxx"

#include "utilities.h"
#include "chrono.hxx"

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellLinks.h>
#include <vtkDoubleArray.h>
#include <vtkIdTypeArray.h>
#include <vtkUnsignedCharArray.h>

#include <list>
#include <climits>

using namespace std;

SMDS_CellLinks* SMDS_CellLinks::New()
{
  MESSAGE("SMDS_CellLinks::New");
  return new SMDS_CellLinks();
}

void SMDS_CellLinks::ResizeForPoint(vtkIdType vtkID)
{
  if ( vtkID > this->MaxId )
  {
    this->MaxId = vtkID;
    if ( vtkID >= this->Size )
      vtkCellLinks::Resize( vtkID+SMDS_Mesh::chunkSize );
  }
}

SMDS_CellLinks::SMDS_CellLinks() :
  vtkCellLinks()
{
}

SMDS_CellLinks::~SMDS_CellLinks()
{
}

SMDS_UnstructuredGrid* SMDS_UnstructuredGrid::New()
{
  MESSAGE("SMDS_UnstructuredGrid::New");
  return new SMDS_UnstructuredGrid();
}

SMDS_UnstructuredGrid::SMDS_UnstructuredGrid() :
  vtkUnstructuredGrid()
{
  _cellIdToDownId.clear();
  _downTypes.clear();
  _downArray.clear();
  _mesh = 0;
}

SMDS_UnstructuredGrid::~SMDS_UnstructuredGrid()
{
}

VTK_MTIME_TYPE SMDS_UnstructuredGrid::GetMTime()
{
  VTK_MTIME_TYPE mtime = vtkUnstructuredGrid::GetMTime();
  MESSAGE("vtkUnstructuredGrid::GetMTime: " << mtime);
  return mtime;
}
// OUV_PORTING_VTK6: seems to be useless
/*
void SMDS_UnstructuredGrid::Update()
{
  MESSAGE("SMDS_UnstructuredGrid::Update");
  return vtkUnstructuredGrid::Update();
}

void SMDS_UnstructuredGrid::UpdateInformation()
{
  MESSAGE("SMDS_UnstructuredGrid::UpdateInformation");
  return vtkUnstructuredGrid::UpdateInformation();
}
*/
vtkPoints* SMDS_UnstructuredGrid::GetPoints()
{
  // TODO erreur incomprehensible de la macro vtk GetPoints apparue avec la version paraview de fin aout 2010
  //MESSAGE("*********************** SMDS_UnstructuredGrid::GetPoints " << this->Points << " " << vtkUnstructuredGrid::GetPoints());
  return this->Points;
}

//#ifdef VTK_HAVE_POLYHEDRON
int SMDS_UnstructuredGrid::InsertNextLinkedCell(int type, int npts, vtkIdType *pts)
{
  if (type != VTK_POLYHEDRON)
    return vtkUnstructuredGrid::InsertNextLinkedCell(type, npts, pts);

  // --- type = VTK_POLYHEDRON
  //MESSAGE("InsertNextLinkedCell VTK_POLYHEDRON");
  int cellid = this->InsertNextCell(type, npts, pts);

  set<vtkIdType> setOfNodes;
  setOfNodes.clear();
  int nbfaces = npts;
  int i = 0;
  for (int nf = 0; nf < nbfaces; nf++)
    {
      int nbnodes = pts[i];
      i++;
      for (int k = 0; k < nbnodes; k++)
        {
          //MESSAGE(" cell " << cellid << " face " << nf << " node " << pts[i]);
          setOfNodes.insert(pts[i]);
          i++;
        }
    }

  set<vtkIdType>::iterator it = setOfNodes.begin();
  for (; it != setOfNodes.end(); ++it)
    {
      //MESSAGE("reverse link for node " << *it << " cell " << cellid);
      this->GetLinks()->ResizeCellList(*it, 1);
      this->GetLinks()->AddCellReference(cellid, *it);
    }

  return cellid;
}
//#endif

void SMDS_UnstructuredGrid::setSMDS_mesh(SMDS_Mesh *mesh)
{
  _mesh = mesh;
}

void SMDS_UnstructuredGrid::compactGrid(std::vector<int>& idNodesOldToNew, int newNodeSize,
                                        std::vector<int>& idCellsOldToNew, int newCellSize)
{
  MESSAGE("------------------------- SMDS_UnstructuredGrid::compactGrid " << newNodeSize << " " << newCellSize);//CHRONO(1);
  int alreadyCopied = 0;

  // --- if newNodeSize, create a new compacted vtkPoints

  vtkPoints *newPoints = vtkPoints::New();
  newPoints->SetDataType(VTK_DOUBLE);
  newPoints->SetNumberOfPoints(newNodeSize);
  if (newNodeSize)
    {
      MESSAGE("-------------- compactGrid, newNodeSize " << newNodeSize);
      // rnv: to fix bug "21125: EDF 1233 SMESH: Degradation of precision in a test case for quadratic conversion"
      // using double type for storing coordinates of nodes instead float.
      int oldNodeSize = idNodesOldToNew.size();

      int i = 0;
      while ( i < oldNodeSize )
      {
        // skip a hole if any
        while ( i < oldNodeSize && idNodesOldToNew[i] < 0 )
          ++i;
        int startBloc = i;
        // look for a block end
        while ( i < oldNodeSize && idNodesOldToNew[i] >= 0 )
          ++i;
        int endBloc = i;
        copyNodes(newPoints, idNodesOldToNew, alreadyCopied, startBloc, endBloc);
      }
      newPoints->Squeeze();
    }

  // --- create new compacted Connectivity, Locations and Types

  int oldCellSize = this->Types->GetNumberOfTuples();

  vtkCellArray *newConnectivity = vtkCellArray::New();
  newConnectivity->Initialize();
  int oldCellDataSize = this->Connectivity->GetData()->GetSize();
  newConnectivity->Allocate(oldCellDataSize);
  MESSAGE("oldCellSize="<< oldCellSize << " oldCellDataSize=" << oldCellDataSize);

  vtkUnsignedCharArray *newTypes = vtkUnsignedCharArray::New();
  newTypes->Initialize();
  newTypes->SetNumberOfValues(newCellSize);

  vtkIdTypeArray *newLocations = vtkIdTypeArray::New();
  newLocations->Initialize();
  newLocations->SetNumberOfValues(newCellSize);

  // TODO some polyhedron may be huge (only in some tests)
  vtkIdType tmpid[NBMAXNODESINCELL];
  vtkIdType *pointsCell = &tmpid[0]; // --- points id to fill a new cell

  alreadyCopied = 0;
  int i = 0;
  while ( i < oldCellSize )
  {
    // skip a hole if any
    while ( i < oldCellSize && this->Types->GetValue(i) == VTK_EMPTY_CELL )
      ++i;
    int startBloc = i;
    // look for a block end
    while ( i < oldCellSize && this->Types->GetValue(i) != VTK_EMPTY_CELL )
      ++i;
    int endBloc = i;
    if ( endBloc > startBloc )
      copyBloc(newTypes,
               idCellsOldToNew, idNodesOldToNew,
               newConnectivity, newLocations,
               pointsCell, alreadyCopied,
               startBloc, endBloc);
  }
  newConnectivity->Squeeze();

  if (1/*newNodeSize*/)
    {
      MESSAGE("------- newNodeSize, setPoints");
      this->SetPoints(newPoints);
      MESSAGE("NumberOfPoints: " << this->GetNumberOfPoints());
    }

  if (vtkDoubleArray* diameters =
      vtkDoubleArray::SafeDownCast( vtkDataSet::CellData->GetScalars() )) // Balls
  {
    for (int oldCellID = 0; oldCellID < oldCellSize; oldCellID++)
    {
      if (this->Types->GetValue(oldCellID) == VTK_EMPTY_CELL)
        continue;
      int newCellId = idCellsOldToNew[ oldCellID ];
      if (newTypes->GetValue(newCellId) == VTK_POLY_VERTEX)
        diameters->SetValue( newCellId, diameters->GetValue( oldCellID ));
    }
  }

  if (this->FaceLocations)
    {
      vtkIdTypeArray *newFaceLocations = vtkIdTypeArray::New();
      newFaceLocations->Initialize();
      newFaceLocations->Allocate(newTypes->GetSize());
      vtkIdTypeArray *newFaces = vtkIdTypeArray::New();
      newFaces->Initialize();
      newFaces->Allocate(this->Faces->GetSize());
      for (int i = 0; i < oldCellSize; i++)
        {
          if (this->Types->GetValue(i) == VTK_EMPTY_CELL)
            continue;
          int newCellId = idCellsOldToNew[i];
          if (newTypes->GetValue(newCellId) == VTK_POLYHEDRON)
            {
               newFaceLocations->InsertNextValue(newFaces->GetMaxId()+1);
               int oldFaceLoc = this->FaceLocations->GetValue(i);
               int nCellFaces = this->Faces->GetValue(oldFaceLoc++);
               newFaces->InsertNextValue(nCellFaces);
               for (int n=0; n<nCellFaces; n++)
                 {
                   int nptsInFace = this->Faces->GetValue(oldFaceLoc++);
                   newFaces->InsertNextValue(nptsInFace);
                   for (int k=0; k<nptsInFace; k++)
                     {
                       int oldpt = this->Faces->GetValue(oldFaceLoc++);
                       newFaces->InsertNextValue(idNodesOldToNew[oldpt]);
                     }
                 }
            }
          else
            {
               newFaceLocations->InsertNextValue(-1);
            }
        }
      newFaceLocations->Squeeze();
      newFaces->Squeeze();
      this->SetCells(newTypes, newLocations, newConnectivity, newFaceLocations, newFaces);
      newFaceLocations->Delete();
      newFaces->Delete();
    }
  else
  {
    this->SetCells(newTypes, newLocations, newConnectivity, FaceLocations, Faces);
  }

  newPoints->Delete();
  newTypes->Delete();
  newLocations->Delete();
  newConnectivity->Delete();
  this->BuildLinks();
}

void SMDS_UnstructuredGrid::copyNodes(vtkPoints *newPoints, std::vector<int>& idNodesOldToNew, int& alreadyCopied,
                                      int start, int end)
{
  MESSAGE("copyNodes " << alreadyCopied << " " << start << " " << end << " size: " << end - start << " total: " << alreadyCopied + end - start);
  void *target = newPoints->GetVoidPointer(3 * alreadyCopied);
  void *source = this->Points->GetVoidPointer(3 * start);
  int nbPoints = end - start;
  if (nbPoints > 0)
    {
      memcpy(target, source, 3 * sizeof(double) * nbPoints);
      for (int j = start; j < end; j++)
        idNodesOldToNew[j] = alreadyCopied++; // old vtkId --> new vtkId
    }
}

void SMDS_UnstructuredGrid::copyBloc(vtkUnsignedCharArray *newTypes,
                                     std::vector<int>&     idCellsOldToNew,
                                     std::vector<int>&     idNodesOldToNew,
                                     vtkCellArray*         newConnectivity,
                                     vtkIdTypeArray*       newLocations,
                                     vtkIdType*            pointsCell,
                                     int&                  alreadyCopied,
                                     int                   start,
                                     int                   end)
{
  //MESSAGE("copyBloc " << alreadyCopied << " " << start << " " << end << " size: " << end - start << " total: " << alreadyCopied + end - start);
  for (int j = start; j < end; j++)
    {
      newTypes->SetValue(alreadyCopied, this->Types->GetValue(j));
      idCellsOldToNew[j] = alreadyCopied; // old vtkId --> new vtkId
      // The difference is mainly the internal representation of vtkCellArray between vtk 7.x and vtk 9.x
      // In the old version a single array of the form (n1,id1,id2,...,idn1, n2,id1,id2,...,idn2, ...) is used
      // whereas in the new version there are two arrays for offset and connectivity of the forms
      // (n1,n2,n3) and (id1,id2,...,idn1,id1,id2,...,idn2, ...)
      // The Locations array in vtk 7.x kept the positions of the n's of the above array: (0, idn1 + 1, idn2 + 2).
      // In vtk 9.x this array doesn't exist any more but its values can be determined with idni + i
#ifdef VTK_CELL_ARRAY_V2
      vtkIdType oldLoc = ((vtkIdTypeArray *)(this->Connectivity->GetOffsetsArray()))->GetValue( j ) + j;
#else
      vtkIdType oldLoc = this->Locations->GetValue(j);
#endif
      vtkIdType nbpts;
      vtkIdTypePtr oldPtsCell = 0;
      this->Connectivity->GetCell(oldLoc, nbpts, oldPtsCell);
      assert(nbpts < NBMAXNODESINCELL);
      //MESSAGE(j << " " << alreadyCopied << " " << (int)this->Types->GetValue(j) << " " << oldLoc << " " << nbpts );
      for (int l = 0; l < nbpts; l++)
        {
          int oldval = oldPtsCell[l];
          pointsCell[l] = idNodesOldToNew[oldval];
          //MESSAGE("   " << oldval << " " << pointsCell[l]);
        }
      /*int newcnt = */newConnectivity->InsertNextCell(nbpts, pointsCell);
      int newLoc = newConnectivity->GetInsertLocation(nbpts);
      //MESSAGE(newcnt << " " << newLoc);
      newLocations->SetValue(alreadyCopied, newLoc);
      alreadyCopied++;
    }
}

int SMDS_UnstructuredGrid::CellIdToDownId(int vtkCellId)
{
  if((vtkCellId < 0) || (vtkCellId >= _cellIdToDownId.size()))
    {
      //MESSAGE("SMDS_UnstructuredGrid::CellIdToDownId structure not up to date: vtkCellId="
      //    << vtkCellId << " max="<< _cellIdToDownId.size());
      return -1;
    }
  return _cellIdToDownId[vtkCellId];
}

void SMDS_UnstructuredGrid::setCellIdToDownId(int vtkCellId, int downId)
{
  // ASSERT((vtkCellId >= 0) && (vtkCellId < _cellIdToDownId.size()));
  _cellIdToDownId[vtkCellId] = downId;
}

void SMDS_UnstructuredGrid::CleanDownwardConnectivity()
{
  for (int i = 0; i < _downArray.size(); i++)
    {
      if (_downArray[i])
        delete _downArray[i];
      _downArray[i] = 0;
    }
  _cellIdToDownId.clear();
}

/*! Build downward connectivity: to do only when needed because heavy memory load.
 *  Downward connectivity is no more valid if vtkUnstructuredGrid is modified.
 *
 */
void SMDS_UnstructuredGrid::BuildDownwardConnectivity(bool withEdges)
{
  MESSAGE("SMDS_UnstructuredGrid::BuildDownwardConnectivity");CHRONO(2);
  // TODO calcul partiel sans edges

  // --- erase previous data if any

  this->CleanDownwardConnectivity();

  // --- create SMDS_Downward structures (in _downArray vector[vtkCellType])

  _downArray.resize(VTK_MAXTYPE + 1, 0);

  _downArray[VTK_LINE]                    = new SMDS_DownEdge(this);
  _downArray[VTK_QUADRATIC_EDGE]          = new SMDS_DownQuadEdge(this);
  _downArray[VTK_TRIANGLE]                = new SMDS_DownTriangle(this);
  _downArray[VTK_QUADRATIC_TRIANGLE]      = new SMDS_DownQuadTriangle(this);
  _downArray[VTK_BIQUADRATIC_TRIANGLE]    = new SMDS_DownQuadTriangle(this);
  _downArray[VTK_QUAD]                    = new SMDS_DownQuadrangle(this);
  _downArray[VTK_QUADRATIC_QUAD]          = new SMDS_DownQuadQuadrangle(this);
  _downArray[VTK_BIQUADRATIC_QUAD]        = new SMDS_DownQuadQuadrangle(this);
  _downArray[VTK_TETRA]                   = new SMDS_DownTetra(this);
  _downArray[VTK_QUADRATIC_TETRA]         = new SMDS_DownQuadTetra(this);
  _downArray[VTK_PYRAMID]                 = new SMDS_DownPyramid(this);
  _downArray[VTK_QUADRATIC_PYRAMID]       = new SMDS_DownQuadPyramid(this);
  _downArray[VTK_WEDGE]                   = new SMDS_DownPenta(this);
  _downArray[VTK_QUADRATIC_WEDGE]         = new SMDS_DownQuadPenta(this);
  _downArray[VTK_HEXAHEDRON]              = new SMDS_DownHexa(this);
  _downArray[VTK_QUADRATIC_HEXAHEDRON]    = new SMDS_DownQuadHexa(this);
  _downArray[VTK_TRIQUADRATIC_HEXAHEDRON] = new SMDS_DownQuadHexa(this);
  _downArray[VTK_HEXAGONAL_PRISM]         = new SMDS_DownPenta(this);

  // --- get detailed info of number of cells of each type, allocate SMDS_downward structures

  const SMDS_MeshInfo &meshInfo = _mesh->GetMeshInfo();

  int nbLinTetra  = meshInfo.NbTetras  (ORDER_LINEAR);
  int nbQuadTetra = meshInfo.NbTetras  (ORDER_QUADRATIC);
  int nbLinPyra   = meshInfo.NbPyramids(ORDER_LINEAR);
  int nbQuadPyra  = meshInfo.NbPyramids(ORDER_QUADRATIC);
  int nbLinPrism  = meshInfo.NbPrisms  (ORDER_LINEAR);
  int nbQuadPrism = meshInfo.NbPrisms  (ORDER_QUADRATIC);
  int nbLinHexa   = meshInfo.NbHexas   (ORDER_LINEAR);
  int nbQuadHexa  = meshInfo.NbHexas   (ORDER_QUADRATIC);
  int nbHexPrism  = meshInfo.NbHexPrisms();

  int nbLineGuess     = int((4.0 / 3.0) * nbLinTetra + 2 * nbLinPrism + 2.5 * nbLinPyra + 3 * nbLinHexa);
  int nbQuadEdgeGuess = int((4.0 / 3.0) * nbQuadTetra + 2 * nbQuadPrism + 2.5 * nbQuadPyra + 3 * nbQuadHexa);
  int nbLinTriaGuess  = 2 * nbLinTetra + nbLinPrism + 2 * nbLinPyra;
  int nbQuadTriaGuess = 2 * nbQuadTetra + nbQuadPrism + 2 * nbQuadPyra;
  int nbLinQuadGuess  = int((2.0 / 3.0) * nbLinPrism + (1.0 / 2.0) * nbLinPyra + 3 * nbLinHexa);
  int nbQuadQuadGuess = int((2.0 / 3.0) * nbQuadPrism + (1.0 / 2.0) * nbQuadPyra + 3 * nbQuadHexa);

  int GuessSize[VTK_MAXTYPE];
  GuessSize[VTK_LINE]                    = nbLineGuess;
  GuessSize[VTK_QUADRATIC_EDGE]          = nbQuadEdgeGuess;
  GuessSize[VTK_TRIANGLE]                = nbLinTriaGuess;
  GuessSize[VTK_QUADRATIC_TRIANGLE]      = nbQuadTriaGuess;
  GuessSize[VTK_BIQUADRATIC_TRIANGLE]    = nbQuadTriaGuess;
  GuessSize[VTK_QUAD]                    = nbLinQuadGuess;
  GuessSize[VTK_QUADRATIC_QUAD]          = nbQuadQuadGuess;
  GuessSize[VTK_BIQUADRATIC_QUAD]        = nbQuadQuadGuess;
  GuessSize[VTK_TETRA]                   = nbLinTetra;
  GuessSize[VTK_QUADRATIC_TETRA]         = nbQuadTetra;
  GuessSize[VTK_PYRAMID]                 = nbLinPyra;
  GuessSize[VTK_QUADRATIC_PYRAMID]       = nbQuadPyra;
  GuessSize[VTK_WEDGE]                   = nbLinPrism;
  GuessSize[VTK_QUADRATIC_WEDGE]         = nbQuadPrism;
  GuessSize[VTK_HEXAHEDRON]              = nbLinHexa;
  GuessSize[VTK_QUADRATIC_HEXAHEDRON]    = nbQuadHexa;
  GuessSize[VTK_TRIQUADRATIC_HEXAHEDRON] = nbQuadHexa;
  GuessSize[VTK_HEXAGONAL_PRISM]         = nbHexPrism;

  _downArray[VTK_LINE]                   ->allocate(nbLineGuess);
  _downArray[VTK_QUADRATIC_EDGE]         ->allocate(nbQuadEdgeGuess);
  _downArray[VTK_TRIANGLE]               ->allocate(nbLinTriaGuess);
  _downArray[VTK_QUADRATIC_TRIANGLE]     ->allocate(nbQuadTriaGuess);
  _downArray[VTK_BIQUADRATIC_TRIANGLE]   ->allocate(nbQuadTriaGuess);
  _downArray[VTK_QUAD]                   ->allocate(nbLinQuadGuess);
  _downArray[VTK_QUADRATIC_QUAD]         ->allocate(nbQuadQuadGuess);
  _downArray[VTK_BIQUADRATIC_QUAD]       ->allocate(nbQuadQuadGuess);
  _downArray[VTK_TETRA]                  ->allocate(nbLinTetra);
  _downArray[VTK_QUADRATIC_TETRA]        ->allocate(nbQuadTetra);
  _downArray[VTK_PYRAMID]                ->allocate(nbLinPyra);
  _downArray[VTK_QUADRATIC_PYRAMID]      ->allocate(nbQuadPyra);
  _downArray[VTK_WEDGE]                  ->allocate(nbLinPrism);
  _downArray[VTK_QUADRATIC_WEDGE]        ->allocate(nbQuadPrism);
  _downArray[VTK_HEXAHEDRON]             ->allocate(nbLinHexa);
  _downArray[VTK_QUADRATIC_HEXAHEDRON]   ->allocate(nbQuadHexa);
  _downArray[VTK_TRIQUADRATIC_HEXAHEDRON]->allocate(nbQuadHexa);
  _downArray[VTK_HEXAGONAL_PRISM]        ->allocate(nbHexPrism);

  // --- iteration on vtkUnstructuredGrid cells, only faces
  //     for each vtk face:
  //       create a downward face entry with its downward id.
  //       compute vtk volumes, create downward volumes entry.
  //       mark face in downward volumes
  //       mark volumes in downward face

  MESSAGE("--- iteration on vtkUnstructuredGrid cells, only faces");CHRONO(20);
  int cellSize = this->Types->GetNumberOfTuples();
  _cellIdToDownId.resize(cellSize, -1);

  for (int i = 0; i < cellSize; i++)
    {
      int vtkFaceType = this->GetCellType(i);
      if (SMDS_Downward::getCellDimension(vtkFaceType) == 2)
        {
          int vtkFaceId = i;
          //ASSERT(_downArray[vtkFaceType]);
          int connFaceId = _downArray[vtkFaceType]->addCell(vtkFaceId);
          SMDS_Down2D* downFace = static_cast<SMDS_Down2D*> (_downArray[vtkFaceType]);
          downFace->setTempNodes(connFaceId, vtkFaceId);
          int vols[2] = { -1, -1 };
          int nbVolumes = downFace->computeVolumeIds(vtkFaceId, vols);
          //MESSAGE("nbVolumes="<< nbVolumes);
          for (int ivol = 0; ivol < nbVolumes; ivol++)
            {
              int vtkVolId = vols[ivol];
              int vtkVolType = this->GetCellType(vtkVolId);
              //ASSERT(_downArray[vtkVolType]);
              int connVolId = _downArray[vtkVolType]->addCell(vtkVolId);
              _downArray[vtkVolType]->addDownCell(connVolId, connFaceId, vtkFaceType);
              _downArray[vtkFaceType]->addUpCell(connFaceId, connVolId, vtkVolType);
              // MESSAGE("Face " << vtkFaceId << " belongs to volume " << vtkVolId);
            }
        }
    }

  // --- iteration on vtkUnstructuredGrid cells, only volumes
  //     for each vtk volume:
  //       create downward volumes entry if not already done
  //       build a temporary list of faces described with their nodes
  //       for each face
  //         compute the vtk volumes containing this face
  //         check if the face is already listed in the volumes (comparison of ordered list of nodes)
  //         if not, create a downward face entry (resizing of structure required)
  //         (the downward faces store a temporary list of nodes to ease the comparison)
  //         create downward volumes entry if not already done
  //         mark volumes in downward face
  //         mark face in downward volumes

  CHRONOSTOP(20);
  MESSAGE("--- iteration on vtkUnstructuredGrid cells, only volumes");CHRONO(21);

  for (int i = 0; i < cellSize; i++)
    {
      int vtkType = this->GetCellType(i);
      if (SMDS_Downward::getCellDimension(vtkType) == 3)
        {
          //CHRONO(31);
          int vtkVolId = i;
          // MESSAGE("vtk volume " << vtkVolId);
          //ASSERT(_downArray[vtkType]);
          /*int connVolId = */_downArray[vtkType]->addCell(vtkVolId);

          // --- find all the faces of the volume, describe the faces by their nodes

          SMDS_Down3D* downVol = static_cast<SMDS_Down3D*> (_downArray[vtkType]);
          ListElemByNodesType facesWithNodes;
          downVol->computeFacesWithNodes(vtkVolId, facesWithNodes);
          // MESSAGE("vtk volume " << vtkVolId << " contains " << facesWithNodes.nbElems << " faces");
          //CHRONOSTOP(31);
          for (int iface = 0; iface < facesWithNodes.nbElems; iface++)
            {
              // --- find the volumes containing the face

              //CHRONO(32);
              int vtkFaceType = facesWithNodes.elems[iface].vtkType;
              SMDS_Down2D* downFace = static_cast<SMDS_Down2D*> (_downArray[vtkFaceType]);
              int vols[2] = { -1, -1 };
              int *nodes = &facesWithNodes.elems[iface].nodeIds[0];
              int lg = facesWithNodes.elems[iface].nbNodes;
              int nbVolumes = downFace->computeVolumeIdsFromNodesFace(nodes, lg, vols);
              // MESSAGE("vtk volume " << vtkVolId << " face " << iface << " belongs to " << nbVolumes << " volumes");

              // --- check if face is registered in the volumes
              //CHRONOSTOP(32);

              //CHRONO(33);
              int connFaceId = -1;
              for (int ivol = 0; ivol < nbVolumes; ivol++)
                {
                  int vtkVolId2 = vols[ivol];
                  int vtkVolType = this->GetCellType(vtkVolId2);
                  //ASSERT(_downArray[vtkVolType]);
                  int connVolId2 = _downArray[vtkVolType]->addCell(vtkVolId2);
                  SMDS_Down3D* downVol2 = static_cast<SMDS_Down3D*> (_downArray[vtkVolType]);
                  connFaceId = downVol2->FindFaceByNodes(connVolId2, facesWithNodes.elems[iface]);
                  if (connFaceId >= 0)
                    break; // --- face already created
                }//CHRONOSTOP(33);

              // --- if face is not registered in the volumes, create face

              //CHRONO(34);
              if (connFaceId < 0)
                {
                  connFaceId = _downArray[vtkFaceType]->addCell();
                  downFace->setTempNodes(connFaceId, facesWithNodes.elems[iface]);
                }//CHRONOSTOP(34);

              // --- mark volumes in downward face and mark face in downward volumes

              //CHRONO(35);
              for (int ivol = 0; ivol < nbVolumes; ivol++)
                {
                  int vtkVolId2 = vols[ivol];
                  int vtkVolType = this->GetCellType(vtkVolId2);
                  //ASSERT(_downArray[vtkVolType]);
                  int connVolId2 = _downArray[vtkVolType]->addCell(vtkVolId2);
                  _downArray[vtkVolType]->addDownCell(connVolId2, connFaceId, vtkFaceType);
                  _downArray[vtkFaceType]->addUpCell(connFaceId, connVolId2, vtkVolType);
                  // MESSAGE(" From volume " << vtkVolId << " face " << connFaceId << " belongs to volume " << vtkVolId2);
                }//CHRONOSTOP(35);
            }
        }
    }

  // --- iteration on vtkUnstructuredGrid cells, only edges
  //     for each vtk edge:
  //       create downward edge entry
  //       store the nodes id's in downward edge (redundant with vtkUnstructuredGrid)
  //       find downward faces
  //       (from vtk faces or volumes, get downward faces, they have a temporary list of nodes)
  //       mark edge in downward faces
  //       mark faces in downward edge

  CHRONOSTOP(21);
  MESSAGE("--- iteration on vtkUnstructuredGrid cells, only edges");CHRONO(22);

  for (int i = 0; i < cellSize; i++)
    {
      int vtkEdgeType = this->GetCellType(i);
      if (SMDS_Downward::getCellDimension(vtkEdgeType) == 1)
        {
          int vtkEdgeId = i;
          //ASSERT(_downArray[vtkEdgeType]);
          int connEdgeId = _downArray[vtkEdgeType]->addCell(vtkEdgeId);
          SMDS_Down1D* downEdge = static_cast<SMDS_Down1D*> (_downArray[vtkEdgeType]);
          downEdge->setNodes(connEdgeId, vtkEdgeId);
          vector<int> vtkIds;
          int nbVtkCells = downEdge->computeVtkCells(connEdgeId, vtkIds);
          int downFaces[1000];
          unsigned char downTypes[1000];
          int nbDownFaces = downEdge->computeFaces(connEdgeId, &vtkIds[0], nbVtkCells, downFaces, downTypes);
          for (int n = 0; n < nbDownFaces; n++)
            {
              _downArray[downTypes[n]]->addDownCell(downFaces[n], connEdgeId, vtkEdgeType);
              _downArray[vtkEdgeType]->addUpCell(connEdgeId, downFaces[n], downTypes[n]);
            }
        }
    }

  // --- iteration on downward faces (they are all listed now)
  //     for each downward face:
  //       build a temporary list of edges with their ordered list of nodes
  //       for each edge:
  //         find all the vtk cells containing this edge
  //         then identify all the downward faces containing the edge, from the vtk cells
  //         check if the edge is already listed in the faces (comparison of ordered list of nodes)
  //         if not, create a downward edge entry with the node id's
  //         mark edge in downward faces
  //         mark downward faces in edge (size of list unknown, to be allocated)

  CHRONOSTOP(22);CHRONO(23);

  for (int vtkFaceType = 0; vtkFaceType < VTK_QUADRATIC_PYRAMID; vtkFaceType++)
    {
      if (SMDS_Downward::getCellDimension(vtkFaceType) != 2)
        continue;

      // --- find all the edges of the face, describe the edges by their nodes

      SMDS_Down2D* downFace = static_cast<SMDS_Down2D*> (_downArray[vtkFaceType]);
      int maxId = downFace->getMaxId();
      for (int faceId = 0; faceId < maxId; faceId++)
        {
          //CHRONO(40);
          ListElemByNodesType edgesWithNodes;
          downFace->computeEdgesWithNodes(faceId, edgesWithNodes);
          // MESSAGE("downward face type " << vtkFaceType << " num " << faceId << " contains " << edgesWithNodes.nbElems << " edges");

          //CHRONOSTOP(40);
          for (int iedge = 0; iedge < edgesWithNodes.nbElems; iedge++)
            {

              // --- check if the edge is already registered by exploration of the faces

              //CHRONO(41);
              vector<int> vtkIds;
              unsigned char vtkEdgeType = edgesWithNodes.elems[iedge].vtkType;
              int *pts = &edgesWithNodes.elems[iedge].nodeIds[0];
              SMDS_Down1D* downEdge = static_cast<SMDS_Down1D*> (_downArray[vtkEdgeType]);
              int nbVtkCells = downEdge->computeVtkCells(pts, vtkIds);
              //CHRONOSTOP(41);CHRONO(42);
              int downFaces[1000];
              unsigned char downTypes[1000];
              int nbDownFaces = downEdge->computeFaces(pts, &vtkIds[0], nbVtkCells, downFaces, downTypes);
              //CHRONOSTOP(42);

              //CHRONO(43);
              int connEdgeId = -1;
              for (int idf = 0; idf < nbDownFaces; idf++)
                {
                  int faceId2 = downFaces[idf];
                  int faceType = downTypes[idf];
                  //ASSERT(_downArray[faceType]);
                  SMDS_Down2D* downFace2 = static_cast<SMDS_Down2D*> (_downArray[faceType]);
                  connEdgeId = downFace2->FindEdgeByNodes(faceId2, edgesWithNodes.elems[iedge]);
                  if (connEdgeId >= 0)
                    break; // --- edge already created
                }//CHRONOSTOP(43);

              // --- if edge is not registered in the faces, create edge

              if (connEdgeId < 0)
                {
                  //CHRONO(44);
                  connEdgeId = _downArray[vtkEdgeType]->addCell();
                  downEdge->setNodes(connEdgeId, edgesWithNodes.elems[iedge].nodeIds);
                  //CHRONOSTOP(44);
                }

              // --- mark faces in downward edge and mark edge in downward faces

              //CHRONO(45);
              for (int idf = 0; idf < nbDownFaces; idf++)
                {
                  int faceId2 = downFaces[idf];
                  int faceType = downTypes[idf];
                  //ASSERT(_downArray[faceType]);
                  //SMDS_Down2D* downFace2 = static_cast<SMDS_Down2D*> (_downArray[faceType]);
                  _downArray[vtkEdgeType]->addUpCell(connEdgeId, faceId2, faceType);
                  _downArray[faceType]->addDownCell(faceId2, connEdgeId, vtkEdgeType);
                  // MESSAGE(" From face t:" << vtkFaceType << " " << faceId <<
                  //  " edge " << connEdgeId << " belongs to face t:" << faceType << " " << faceId2);
                }//CHRONOSTOP(45);
            }
        }
    }

  CHRONOSTOP(23);CHRONO(24);

  // compact downward connectivity structure: adjust downward arrays size, replace vector<vector int>> by a single vector<int>
  // 3D first then 2D and last 1D to release memory before edge upCells reorganization, (temporary memory use)

  for (int vtkType = VTK_QUADRATIC_PYRAMID; vtkType >= 0; vtkType--)
    {
      if (SMDS_Downward *down = _downArray[vtkType])
        {
          down->compactStorage();
        }
    }

  // --- Statistics

  for (int vtkType = 0; vtkType <= VTK_QUADRATIC_PYRAMID; vtkType++)
    {
      if (SMDS_Downward *down = _downArray[vtkType])
        {
          if (down->getMaxId())
            {
              MESSAGE("Cells of Type " << vtkType << " : number of entities, est: "
                  << GuessSize[vtkType] << " real: " << down->getMaxId());
            }
        }
    }CHRONOSTOP(24);CHRONOSTOP(2);
  counters::stats();
}

/*! Get the neighbors of a cell.
 * Only the neighbors having the dimension of the cell are taken into account
 * (neighbors of a volume are the volumes sharing a face with this volume,
 *  neighbors of a face are the faces sharing an edge with this face...).
 * @param neighborsVtkIds vector of neighbors vtk id's to fill (reserve enough space).
 * @param downIds downward id's of cells of dimension n-1, to fill (reserve enough space).
 * @param downTypes vtk types of cells of dimension n-1, to fill (reserve enough space).
 * @param vtkId the vtk id of the cell
 * @return number of neighbors
 */
int SMDS_UnstructuredGrid::GetNeighbors(int* neighborsVtkIds, int* downIds, unsigned char* downTypes, int vtkId, bool getSkin)
{
  int vtkType = this->GetCellType(vtkId);
  int cellDim = SMDS_Downward::getCellDimension(vtkType);
  if (cellDim <2)
    return 0; // TODO voisins des edges = edges connectees
  int cellId = this->_cellIdToDownId[vtkId];

  int nbCells = _downArray[vtkType]->getNumberOfDownCells(cellId);
  const int *downCells = _downArray[vtkType]->getDownCells(cellId);
  const unsigned char* downTyp = _downArray[vtkType]->getDownTypes(cellId);

  // --- iteration on faces of the 3D cell (or edges on the 2D cell).

  int nb = 0;
  for (int i = 0; i < nbCells; i++)
    {
      int downId = downCells[i];
      int cellType = downTyp[i];
      int nbUp = _downArray[cellType]->getNumberOfUpCells(downId);
      const int *upCells = _downArray[cellType]->getUpCells(downId);
      const unsigned char* upTypes = _downArray[cellType]->getUpTypes(downId);

      // ---for a volume, max 2 upCells, one is this cell, the other is a neighbor
      //    for a face, number of neighbors (connected faces) not known

      for (int j = 0; j < nbUp; j++)
        {
          if ((upCells[j] == cellId) && (upTypes[j] == vtkType))
            continue;
          int vtkNeighbor = _downArray[upTypes[j]]->getVtkCellId(upCells[j]);
          neighborsVtkIds[nb] = vtkNeighbor;
          downIds[nb] = downId;
          downTypes[nb] = cellType;
          nb++;
          if (nb >= NBMAXNEIGHBORS)
            {
              INFOS("SMDS_UnstructuredGrid::GetNeighbors problem: NBMAXNEIGHBORS=" <<NBMAXNEIGHBORS << " not enough");
              return nb;
            }
        }
      if (getSkin)
        {
          if (cellDim == 3 && nbUp == 1) // this face is on the skin of the volume
            {
              neighborsVtkIds[nb] = _downArray[cellType]->getVtkCellId(downId); // OK if skin present
              downIds[nb] = downId;
              downTypes[nb] = cellType;
              nb++;
              if (nb >= NBMAXNEIGHBORS)
                {
                  INFOS("SMDS_UnstructuredGrid::GetNeighbors problem: NBMAXNEIGHBORS=" <<NBMAXNEIGHBORS << " not enough");
                  return nb;
                }
            }
        }
    }
  return nb;
}

/*! get the volumes containing a face or an edge of the grid
 * The edge or face belongs to the vtkUnstructuredGrid
 * @param volVtkIds vector of parent volume ids to fill (reserve enough space!)
 * @param vtkId vtk id of the face or edge
 */
int SMDS_UnstructuredGrid::GetParentVolumes(int* volVtkIds, int vtkId)
{
  int vtkType = this->GetCellType(vtkId);
  int dim = SMDS_Downward::getCellDimension(vtkType);
  int nbFaces = 0;
  unsigned char cellTypes[1000];
  int downCellId[1000];
  if (dim == 1)
    {
      int downId = this->CellIdToDownId(vtkId);
      if (downId < 0)
        {
          MESSAGE("Downward structure not up to date: new edge not taken into account");
          return 0;
        }
      nbFaces = _downArray[vtkType]->getNumberOfUpCells(downId);
      const int *upCells = _downArray[vtkType]->getUpCells(downId);
      const unsigned char* upTypes = _downArray[vtkType]->getUpTypes(downId);
      for (int i=0; i< nbFaces; i++)
        {
          cellTypes[i] = upTypes[i];
          downCellId[i] = upCells[i];
        }
    }
  else if (dim == 2)
    {
      nbFaces = 1;
      cellTypes[0] = this->GetCellType(vtkId);
      int downId = this->CellIdToDownId(vtkId);
      if (downId < 0)
        {
          MESSAGE("Downward structure not up to date: new face not taken into account");
          return 0;
        }
      downCellId[0] = downId;
    }

  int nbvol =0;
  for (int i=0; i<nbFaces; i++)
    {
      int vtkTypeFace = cellTypes[i];
      int downId = downCellId[i];
      int nv = _downArray[vtkTypeFace]->getNumberOfUpCells(downId);
      const int *upCells = _downArray[vtkTypeFace]->getUpCells(downId);
      const unsigned char* upTypes = _downArray[vtkTypeFace]->getUpTypes(downId);
       for (int j=0; j<nv; j++)
        {
          int vtkVolId = _downArray[upTypes[j]]->getVtkCellId(upCells[j]);
          if (vtkVolId >= 0)
            volVtkIds[nbvol++] = vtkVolId;
        }
    }
  return nbvol;
}

/*! get the volumes containing a face or an edge of the downward structure
 * The edge or face does not necessary belong to the vtkUnstructuredGrid
 * @param volVtkIds vector of parent volume ids to fill (reserve enough space!)
 * @param downId id in the downward structure
 * @param downType type of cell
 */
int SMDS_UnstructuredGrid::GetParentVolumes(int* volVtkIds, int downId, unsigned char downType)
{
  int vtkType = downType;
  int dim = SMDS_Downward::getCellDimension(vtkType);
  int nbFaces = 0;
  unsigned char cellTypes[1000];
  int downCellId[1000];
  if (dim == 1)
    {
      nbFaces = _downArray[vtkType]->getNumberOfUpCells(downId);
      const int *upCells = _downArray[vtkType]->getUpCells(downId);
      const unsigned char* upTypes = _downArray[vtkType]->getUpTypes(downId);
      for (int i=0; i< nbFaces; i++)
        {
          cellTypes[i] = upTypes[i];
          downCellId[i] = upCells[i];
        }
    }
  else if (dim == 2)
    {
      nbFaces = 1;
      cellTypes[0] = vtkType;
      downCellId[0] = downId;
    }

  int nbvol =0;
  for (int i=0; i<nbFaces; i++)
    {
      int vtkTypeFace = cellTypes[i];
      int downId = downCellId[i];
      int nv = _downArray[vtkTypeFace]->getNumberOfUpCells(downId);
      const int *upCells = _downArray[vtkTypeFace]->getUpCells(downId);
      const unsigned char* upTypes = _downArray[vtkTypeFace]->getUpTypes(downId);
       for (int j=0; j<nv; j++)
        {
          int vtkVolId = _downArray[upTypes[j]]->getVtkCellId(upCells[j]);
          if (vtkVolId >= 0)
            volVtkIds[nbvol++] = vtkVolId;
        }
    }
  return nbvol;
}

/*! get the node id's of a cell.
 * The cell is defined by it's downward connectivity id and type.
 * @param nodeSet set of of vtk node id's to fill.
 * @param downId downward connectivity id of the cell.
 * @param downType type of cell.
 */
void SMDS_UnstructuredGrid::GetNodeIds(std::set<int>& nodeSet, int downId, unsigned char downType)
{
  _downArray[downType]->getNodeIds(downId, nodeSet);
}

/*! change some nodes in cell without modifying type or internal connectivity.
 * Nodes inverse connectivity is maintained up to date.
 * @param vtkVolId vtk id of the cell
 * @param localClonedNodeIds map old node id to new node id.
 */
void SMDS_UnstructuredGrid::ModifyCellNodes(int vtkVolId, std::map<int, int> localClonedNodeIds)
{
#ifdef VTK_CELL_ARRAY_V2
  vtkNew<vtkIdList> cellPoints;
  this->GetCellPoints(vtkVolId, cellPoints.GetPointer());
  for (vtkIdType i = 0; i < cellPoints->GetNumberOfIds(); i++)
    {
      if (localClonedNodeIds.count(cellPoints->GetId(i)))
        {
          vtkIdType oldpt = cellPoints->GetId(i);
          cellPoints->SetId(i, localClonedNodeIds[oldpt]);
          //MESSAGE(oldpt << " --> " << pts[i]);
          //this->RemoveReferenceToCell(oldpt, vtkVolId);
          //this->AddReferenceToCell(pts[i], vtkVolId);
        }
    }
#else
  vtkIdType npts = 0;
  vtkIdType *pts; // will refer to the point id's of the face
  this->GetCellPoints(vtkVolId, npts, pts);
  for (int i = 0; i < npts; i++)
    {
      if (localClonedNodeIds.count(pts[i]))
        {
          vtkIdType oldpt = pts[i];
          pts[i] = localClonedNodeIds[oldpt];
          //MESSAGE(oldpt << " --> " << pts[i]);
          //this->RemoveReferenceToCell(oldpt, vtkVolId);
          //this->AddReferenceToCell(pts[i], vtkVolId);
        }
    }
#endif
}

/*! reorder the nodes of a face
 * @param vtkVolId vtk id of a volume containing the face, to get an orientation for the face.
 * @param orderedNodes list of nodes to reorder (in out)
 * @return size of the list
 */
int SMDS_UnstructuredGrid::getOrderedNodesOfFace(int vtkVolId, int& dim, std::vector<vtkIdType>& orderedNodes)
{
  int vtkType = this->GetCellType(vtkVolId);
  dim = SMDS_Downward::getCellDimension(vtkType);
  if (dim == 3)
    {
      SMDS_Down3D *downvol = static_cast<SMDS_Down3D*> (_downArray[vtkType]);
      int downVolId = this->_cellIdToDownId[vtkVolId];
      downvol->getOrderedNodesOfFace(downVolId, orderedNodes);
    }
  // else nothing to do;
  return orderedNodes.size();
}

void SMDS_UnstructuredGrid::BuildLinks()
{
  // Remove the old links if they are already built
  if (this->Links)
    {
    this->Links->UnRegister(this);
    }

#ifdef VTK_CELL_ARRAY_V2
  this->Links = SMDS_CellLinks::New();
  GetLinks()->Allocate(this->GetNumberOfPoints());
  GetLinks()->Register(this);
//FIXME: vtk9
  GetLinks()->BuildLinks(this);
  GetLinks()->Delete();
#else
  this->Links = SMDS_CellLinks::New();
  this->Links->Allocate(this->GetNumberOfPoints());
  this->Links->Register(this);
  this->Links->BuildLinks(this, this->Connectivity);
  this->Links->Delete();
#endif
}

/*! Create a volume (prism or hexahedron) by duplication of a face.
 * Designed for use in creation of flat elements separating volume domains.
 * A face separating two domains is shared by two volume cells.
 * All the nodes are already created (for the two faces).
 * Each original Node is associated to corresponding nodes in the domains.
 * Some nodes may be duplicated for more than two domains, when domain separations intersect.
 * In that case, even some of the nodes to use for the original face may be changed.
 * @param vtkVolId: vtk id of a volume containing the face, to get an orientation for the face.
 * @param domain1: domain of the original face
 * @param domain2: domain of the duplicated face
 * @param originalNodes: the vtk node ids of the original face
 * @param nodeDomains: map(original id --> map(domain --> duplicated node id))
 * @return ok if success.
 */
SMDS_MeshCell* SMDS_UnstructuredGrid::extrudeVolumeFromFace(int vtkVolId,
                                                  int domain1,
                                                  int domain2,
                                                  std::set<int>& originalNodes,
                                                  std::map<int, std::map<int, int> >& nodeDomains,
                                                  std::map<int, std::map<long, int> >& nodeQuadDomains)
{
  //MESSAGE("extrudeVolumeFromFace " << vtkVolId);
  vector<vtkIdType> orderedOriginals;
  orderedOriginals.clear();
  set<int>::const_iterator it = originalNodes.begin();
  for (; it != originalNodes.end(); ++it)
    orderedOriginals.push_back(*it);

  int dim = 0;
  int nbNodes = this->getOrderedNodesOfFace(vtkVolId, dim, orderedOriginals);
  vector<vtkIdType> orderedNodes;

  bool isQuadratic = false;
  switch (orderedOriginals.size())
  {
    case 3:
      if (dim == 2)
        isQuadratic = true;
      break;
    case 6:
    case 8:
      isQuadratic = true;
      break;
    default:
      isQuadratic = false;
      break;
  }

  if (isQuadratic)
    {
      long dom1 = domain1;
      long dom2 = domain2;
      long dom1_2; // for nodeQuadDomains
      if (domain1 < domain2)
        dom1_2 = dom1 + INT_MAX * dom2;
      else
        dom1_2 = dom2 + INT_MAX * dom1;
      //cerr << "dom1=" << dom1 << " dom2=" << dom2 << " dom1_2=" << dom1_2 << endl;
      int ima = orderedOriginals.size();
      int mid = orderedOriginals.size() / 2;
      //cerr << "ima=" << ima << " mid=" << mid << endl;
      for (int i = 0; i < mid; i++)
        orderedNodes.push_back(nodeDomains[orderedOriginals[i]][domain1]);
      for (int i = 0; i < mid; i++)
        orderedNodes.push_back(nodeDomains[orderedOriginals[i]][domain2]);
      for (int i = mid; i < ima; i++)
        orderedNodes.push_back(nodeDomains[orderedOriginals[i]][domain1]);
      for (int i = mid; i < ima; i++)
        orderedNodes.push_back(nodeDomains[orderedOriginals[i]][domain2]);
      for (int i = 0; i < mid; i++)
        {
          int oldId = orderedOriginals[i];
          int newId;
          if (nodeQuadDomains.count(oldId) && nodeQuadDomains[oldId].count(dom1_2))
            newId = nodeQuadDomains[oldId][dom1_2];
          else
            {
              double *coords = this->GetPoint(oldId);
              SMDS_MeshNode *newNode = _mesh->AddNode(coords[0], coords[1], coords[2]);
              newId = newNode->getVtkId();
              if (! nodeQuadDomains.count(oldId))
                {
                  std::map<long, int> emptyMap;
                  nodeQuadDomains[oldId] = emptyMap;
                }
              nodeQuadDomains[oldId][dom1_2] = newId;
            }
          orderedNodes.push_back(newId);
        }
    }
  else
    {
      for (int i = 0; i < nbNodes; i++)
        orderedNodes.push_back(nodeDomains[orderedOriginals[i]][domain1]);
      if (dim == 3)
        for (int i = 0; i < nbNodes; i++)
          orderedNodes.push_back(nodeDomains[orderedOriginals[i]][domain2]);
      else
        for (int i = nbNodes-1; i >= 0; i--)
          orderedNodes.push_back(nodeDomains[orderedOriginals[i]][domain2]);

    }

  if (dim == 3)
    {
      SMDS_MeshVolume *vol = _mesh->AddVolumeFromVtkIds(orderedNodes);
      return vol;
    }
  else if (dim == 2)
    {
      SMDS_MeshFace *face = _mesh->AddFaceFromVtkIds(orderedNodes);
      return face;
    }

  // TODO update sub-shape list of elements and nodes
  return 0;
}

//================================================================================
/*!
 * \brief Allocates data array for ball diameters
 *  \param MaxVtkID - max ID of a ball element
 */
//================================================================================

void SMDS_UnstructuredGrid::AllocateDiameters( vtkIdType MaxVtkID )
{
  SetBallDiameter( MaxVtkID, 0 );
}

//================================================================================
/*!
 * \brief Sets diameter of a ball element
 *  \param vtkID - vtk id of the ball element
 *  \param diameter - diameter of the ball element
 */
//================================================================================

void SMDS_UnstructuredGrid::SetBallDiameter( vtkIdType vtkID, double diameter )
{
  vtkDoubleArray* array = vtkDoubleArray::SafeDownCast( vtkDataSet::CellData->GetScalars() );
  if ( !array )
  {
    array = vtkDoubleArray::New();
    array->SetNumberOfComponents(1);
    vtkDataSet::CellData->SetScalars( array );
  }
  array->InsertValue( vtkID, diameter );
}

//================================================================================
/*!
 * \brief Returns diameter of a ball element
 *  \param vtkID - vtk id of the ball element
 */
//================================================================================

double SMDS_UnstructuredGrid::GetBallDiameter( vtkIdType vtkID ) const
{
  if ( vtkDataSet::CellData )
    return vtkDoubleArray::SafeDownCast( vtkDataSet::CellData->GetScalars() )->GetValue( vtkID );
  return 0;
}

