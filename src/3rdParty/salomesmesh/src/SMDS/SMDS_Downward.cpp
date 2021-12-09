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

// File: SMDS_Downward.cxx
// Created: Jun 3, 2010
// Author: prascle

#include "SMDS_Downward.hxx"
#include "SMDS_Mesh.hxx"
#include "utilities.h"

#include <vtkCellType.h>
#include <vtkCellLinks.h>

#include <map>

using namespace std;

// ---------------------------------------------------------------------------

vector<int> SMDS_Downward::_cellDimension;

/*! get the dimension of a cell (1,2,3 for 1D, 2D 3D) given the vtk cell type
 *
 * @param cellType vtk cell type @see vtkCellType.h
 * @return 1,2 or 3
 */
int SMDS_Downward::getCellDimension(unsigned char cellType)
{
  if (_cellDimension.empty())
    {
      _cellDimension.resize(VTK_MAXTYPE + 1, 0);
      _cellDimension[VTK_LINE] = 1;
      _cellDimension[VTK_QUADRATIC_EDGE] = 1;
      _cellDimension[VTK_TRIANGLE] = 2;
      _cellDimension[VTK_QUADRATIC_TRIANGLE] = 2;
      _cellDimension[VTK_BIQUADRATIC_TRIANGLE] = 2;
      _cellDimension[VTK_QUAD] = 2;
      _cellDimension[VTK_QUADRATIC_QUAD] = 2;
      _cellDimension[VTK_BIQUADRATIC_QUAD] = 2;
      _cellDimension[VTK_TETRA] = 3;
      _cellDimension[VTK_QUADRATIC_TETRA] = 3;
      _cellDimension[VTK_HEXAHEDRON] = 3;
      _cellDimension[VTK_QUADRATIC_HEXAHEDRON] = 3;
      _cellDimension[VTK_TRIQUADRATIC_HEXAHEDRON] = 3;
      _cellDimension[VTK_WEDGE] = 3;
      _cellDimension[VTK_QUADRATIC_WEDGE] = 3;
      _cellDimension[VTK_PYRAMID] = 3;
      _cellDimension[VTK_QUADRATIC_PYRAMID] = 3;
      _cellDimension[VTK_HEXAGONAL_PRISM] = 3;
    }
  return _cellDimension[cellType];
}

// ---------------------------------------------------------------------------

/*! Generic constructor for all the downward connectivity structures (one per vtk cell type).
 *  The static structure for cell dimension is set only once.
 * @param grid unstructured grid associated to the mesh.
 * @param nbDownCells number of downward entities associated to this vtk type of cell.
 * @return
 */
SMDS_Downward::SMDS_Downward(SMDS_UnstructuredGrid *grid, int nbDownCells) :
  _grid(grid), _nbDownCells(nbDownCells)
{
  this->_maxId = 0;
  this->_cellIds.clear();
  this->_cellTypes.clear();
  if (_cellDimension.empty())
    getCellDimension( VTK_LINE );
}

SMDS_Downward::~SMDS_Downward()
{
}

/*! Give or create an entry for downward connectivity structure relative to a cell.
 * If the entry already exists, just return its id, otherwise, create it.
 * The internal storage memory is allocated if needed.
 * The SMDS_UnstructuredGrid::_cellIdToDownId vector is completed for vtkUnstructuredGrid cells.
 * @param vtkId for a vtkUnstructuredGrid cell  or -1 (default) for a created downward cell.
 * @return the rank in downward[vtkType] structure.
 */
int SMDS_Downward::addCell(int vtkId)
{
  int localId = -1;
  if (vtkId >= 0)
    localId = _grid->CellIdToDownId(vtkId);
  if (localId >= 0)
    return localId;

  localId = this->_maxId;
  this->_maxId++;
  this->allocate(_maxId);
  if (vtkId >= 0)
    {
      this->_vtkCellIds[localId] = vtkId;
      _grid->setCellIdToDownId(vtkId, localId);
    }
  this->initCell(localId);
  return localId;
}

/*! generic method do nothing. see derived methods
 *
 * @param cellId
 */
void SMDS_Downward::initCell(int cellId)
{
}

/*! Get the number of downward entities associated to a cell (always the same for a given vtk type of cell)
 *
 * @param cellId not used here.
 * @return
 */
int SMDS_Downward::getNumberOfDownCells(int cellId)
{
  return _nbDownCells;
}

/*! get a pointer on the downward entities id's associated to a cell.
 * @see SMDS_Downward::getNumberOfDownCells for the number of downward entities.
 * @see SMDS_Downward::getDownTypes for the vtk cell types associated to the downward entities.
 * @param cellId index of the cell in the downward structure relative to a given vtk cell type.
 * @return table of downward entities id's.
 */
const int* SMDS_Downward::getDownCells(int cellId)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  return &_cellIds[_nbDownCells * cellId];
}

/*! get a list of vtk cell types associated to downward entities of a given cell, in the same order
 * than the downward entities id's list (@see SMDS_Downward::getDownCells).
 *
 * @param cellId index of the cell in the downward structure relative to a vtk cell type.
 * @return table of downward entities types.
 */
const unsigned char* SMDS_Downward::getDownTypes(int cellId)
{
  return &_cellTypes[0];
}

/*! add a downward entity of dimension n-1 (cell or node) to a given cell.
 * Actual implementation is done in derived methods.
 * @param cellId index of the parent cell (dimension n) in the downward structure relative to a vtk cell type.
 * @param lowCellId index of the children cell to add (dimension n-1)
 * @param aType vtk cell type of the cell to add (needed to find the SMDS_Downward structure containing the cell to add).
 */
void SMDS_Downward::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  ASSERT(0); // must be re-implemented in derived class
}

/*! add a downward entity of dimension n+1 to a given cell.
 * Actual implementation is done in derived methods.
 * @param cellId index of the children cell (dimension n) in the downward structure relative to a vtk cell type.
 * @param upCellId index of the parent cell to add (dimension n+1)
 * @param aType vtk cell type of the cell to add (needed to find the SMDS_Downward structure containing the cell to add).
 */
void SMDS_Downward::addUpCell(int cellId, int upCellId, unsigned char aType)
{
  ASSERT(0); // must be re-implemented in derived class
}

int SMDS_Downward::getNodeSet(int cellId, int* nodeSet)
{
  return 0;
}

// ---------------------------------------------------------------------------

SMDS_Down1D::SMDS_Down1D(SMDS_UnstructuredGrid *grid, int nbDownCells) :
  SMDS_Downward(grid, nbDownCells)
{
  _upCellIdsVector.clear();
  _upCellTypesVector.clear();
  _upCellIds.clear();
  _upCellTypes.clear();
  _upCellIndex.clear();
}

SMDS_Down1D::~SMDS_Down1D()
{
}

/*! clear vectors used to reference 2D cells containing the edge
 *
 * @param cellId
 */
void SMDS_Down1D::initCell(int cellId)
{
  _upCellIdsVector[cellId].clear();
  _upCellTypesVector[cellId].clear();
}

/*! Resize the downward connectivity storage vector if needed.
 *
 * @param nbElems total number of elements of the same type required
 */
void SMDS_Down1D::allocate(int nbElems)
{
  if (nbElems >= _vtkCellIds.size())
    {
      _vtkCellIds.resize(nbElems + SMDS_Mesh::chunkSize, -1);
      _cellIds.resize(_nbDownCells * (nbElems + SMDS_Mesh::chunkSize), -1);
      _upCellIdsVector.resize(nbElems + SMDS_Mesh::chunkSize);
      _upCellTypesVector.resize(nbElems + SMDS_Mesh::chunkSize);
    }
}

void SMDS_Down1D::compactStorage()
{
  _cellIds.resize(_nbDownCells * _maxId);
  _vtkCellIds.resize(_maxId);

  int sizeUpCells = 0;
  for (int i = 0; i < _maxId; i++)
    sizeUpCells += _upCellIdsVector[i].size();
  _upCellIds.resize(sizeUpCells, -1);
  _upCellTypes.resize(sizeUpCells);
  _upCellIndex.resize(_maxId + 1, -1); // id and types of rank i correspond to [ _upCellIndex[i], _upCellIndex[i+1] [
  int current = 0;
  for (int i = 0; i < _maxId; i++)
    {
      _upCellIndex[i] = current;
      for (int j = 0; j < _upCellIdsVector[i].size(); j++)
        {
          _upCellIds[current] = _upCellIdsVector[i][j];
          _upCellTypes[current] = _upCellTypesVector[i][j];
          current++;
        }
    }
  _upCellIndex[_maxId] = current;

  _upCellIdsVector.clear();
  _upCellTypesVector.clear();
}

void SMDS_Down1D::addUpCell(int cellId, int upCellId, unsigned char aType)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  int nbFaces = _upCellIdsVector[cellId].size();
  for (int i = 0; i < nbFaces; i++)
    {
      if ((_upCellIdsVector[cellId][i] == upCellId) && (_upCellTypesVector[cellId][i] == aType))
        {
          return; // already done
        }
    }
  _upCellIdsVector[cellId].push_back(upCellId);
  _upCellTypesVector[cellId].push_back(aType);
}

int SMDS_Down1D::getNumberOfUpCells(int cellId)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  return _upCellIndex[cellId + 1] - _upCellIndex[cellId];
}

const int* SMDS_Down1D::getUpCells(int cellId)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  return &_upCellIds[_upCellIndex[cellId]];
}

const unsigned char* SMDS_Down1D::getUpTypes(int cellId)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  return &_upCellTypes[_upCellIndex[cellId]];
}

void SMDS_Down1D::getNodeIds(int cellId, std::set<int>& nodeSet)
{
  for (int i = 0; i < _nbDownCells; i++)
    nodeSet.insert(_cellIds[_nbDownCells * cellId + i]);
}

int SMDS_Down1D::getNodeSet(int cellId, int* nodeSet)
{
  for (int i = 0; i < _nbDownCells; i++)
    nodeSet[i] = _cellIds[_nbDownCells * cellId + i];
  return _nbDownCells;
}

void SMDS_Down1D::setNodes(int cellId, int vtkId)
{
  vtkIdType npts = 0;
  vtkIdTypePtr pts; // will refer to the point id's of the face
  _grid->GetCellPoints(vtkId, npts, pts);
  // MESSAGE(vtkId << " " << npts << "  " << _nbDownCells);
  //ASSERT(npts == _nbDownCells);
  for (int i = 0; i < npts; i++)
    {
      _cellIds[_nbDownCells * cellId + i] = pts[i];
    }
}

void SMDS_Down1D::setNodes(int cellId, const int* nodeIds)
{
  //ASSERT(nodeIds.size() == _nbDownCells);
  for (int i = 0; i < _nbDownCells; i++)
    {
      _cellIds[_nbDownCells * cellId + i] = nodeIds[i];
    }
}

/*! Build the list of vtkUnstructuredGrid cells containing the edge.
 * We keep in the list the cells that contains all the nodes, we keep only volumes and faces.
 * @param cellId id of the edge in the downward structure
 * @param vtkIds vector of vtk id's
 * @return number of vtk cells (size of vector)
 */
int SMDS_Down1D::computeVtkCells(int cellId, std::vector<int>& vtkIds)
{
  vtkIds.clear();

  // --- find all the cells the points belong to, and how many of the points belong to a given cell

  int *pts = &_cellIds[_nbDownCells * cellId];
  int ncells = this->computeVtkCells(pts, vtkIds);
  return ncells;
}

/*! Build the list of vtkUnstructuredGrid cells containing the edge.
 *
 * @param pts list of points id's defining an edge
 * @param vtkIds vector of vtk id's
 * @return number of vtk cells (size of vector)
 */
int SMDS_Down1D::computeVtkCells(int *pts, std::vector<int>& vtkIds)
{

  // --- find all the cells the points belong to, and how many of the points belong to a given cell

  int cellIds[1000];
  int cellCnt[1000];
  int cnt = 0;
  for (int i = 0; i < _nbDownCells; i++)
    {
      vtkIdType point = pts[i];
      int numCells = _grid->GetLinks()->GetNcells(point);
      vtkIdTypePtr cells = _grid->GetLinks()->GetCells(point);
      for (int j = 0; j < numCells; j++)
        {
          int vtkCellId = cells[j];
          bool found = false;
          for (int k = 0; k < cnt; k++)
            {
              if (cellIds[k] == vtkCellId)
                {
                  cellCnt[k] += 1;
                  found = true;
                  break;
                }
            }
          if (!found)
            {
              cellIds[cnt] = vtkCellId;
              cellCnt[cnt] = 1;
              // TODO ASSERT(cnt<1000);
              cnt++;
            }
        }
    }

  // --- find the face and volume cells: they contains all the points and are of type volume or face

  int ncells = 0;
  for (int i = 0; i < cnt; i++)
    {
      if (cellCnt[i] == _nbDownCells)
        {
          int vtkElemId = cellIds[i];
          int vtkType = _grid->GetCellType(vtkElemId);
          if (SMDS_Downward::getCellDimension(vtkType) > 1)
            {
              vtkIds.push_back(vtkElemId);
              ncells++;
            }
        }
    }

  return ncells;
}

/*! Build the list of downward faces from a list of vtk cells.
 *
 * @param cellId id of the edge in the downward structure
 * @param vtkIds vector of vtk id's
 * @param downFaces vector of face id's in downward structures
 * @param downTypes vector of face types
 * @return number of downward faces
 */
int SMDS_Down1D::computeFaces(int cellId, int* vtkIds, int nbcells, int* downFaces, unsigned char* downTypes)
{
  int *pts = &_cellIds[_nbDownCells * cellId];
  int nbFaces = this->computeFaces(pts, vtkIds, nbcells, downFaces, downTypes);
  return nbFaces;
}

/*! Build the list of downward faces from a list of vtk cells.
 *
 * @param pts list of points id's defining an edge
 * @param vtkIds vector of vtk id's
 * @param downFaces vector of face id's in downward structures
 * @param downTypes vector of face types
 * @return number of downward faces
 */
int SMDS_Down1D::computeFaces(int* pts, int* vtkIds, int nbcells, int* downFaces, unsigned char* downTypes)
{
  int cnt = 0;
  for (int i = 0; i < nbcells; i++)
    {
      int vtkId = vtkIds[i];
      int vtkType = _grid->GetCellType(vtkId);
      if (SMDS_Downward::getCellDimension(vtkType) == 2)
        {
          int faceId = _grid->CellIdToDownId(vtkId);
          downFaces[cnt] = faceId;
          downTypes[cnt] = vtkType;
          cnt++;
        }
      else if (SMDS_Downward::getCellDimension(vtkType) == 3)
        {
          int volId = _grid->CellIdToDownId(vtkId);
          SMDS_Downward * downvol = _grid->getDownArray(vtkType);
          //const int *downIds = downvol->getDownCells(volId);
          const unsigned char* downTypesVol = downvol->getDownTypes(volId);
          int nbFaces = downvol->getNumberOfDownCells(volId);
          const int* faceIds = downvol->getDownCells(volId);
          for (int n = 0; n < nbFaces; n++)
            {
              SMDS_Down2D *downFace = static_cast<SMDS_Down2D*> (_grid->getDownArray(downTypesVol[n]));
              bool isInFace = downFace->isInFace(faceIds[n], pts, _nbDownCells);
              if (isInFace)
                {
                  bool alreadySet = false;
                  for (int k = 0; k < cnt; k++)
                    if (faceIds[n] == downFaces[k])
                      {
                        alreadySet = true;
                        break;
                      }
                  if (!alreadySet)
                    {
                      downFaces[cnt] = faceIds[n];
                      downTypes[cnt] = downTypesVol[n];
                      cnt++;
                    }
                }
            }
        }
    }
  return cnt;
}

// ---------------------------------------------------------------------------

SMDS_Down2D::SMDS_Down2D(SMDS_UnstructuredGrid *grid, int nbDownCells) :
  SMDS_Downward(grid, nbDownCells)
{
  _upCellIds.clear();
  _upCellTypes.clear();
  _tempNodes.clear();
  _nbNodes = 0;
}

SMDS_Down2D::~SMDS_Down2D()
{
}

int SMDS_Down2D::getNumberOfUpCells(int cellId)
{
  int nbup = 0;
  if (_upCellIds[2 * cellId] >= 0)
    nbup++;
  if (_upCellIds[2 * cellId + 1] >= 0)
    nbup++;
  return nbup;
}

const int* SMDS_Down2D::getUpCells(int cellId)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  return &_upCellIds[2 * cellId];
}

const unsigned char* SMDS_Down2D::getUpTypes(int cellId)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  return &_upCellTypes[2 * cellId];
}

void SMDS_Down2D::getNodeIds(int cellId, std::set<int>& nodeSet)
{
  for (int i = 0; i < _nbDownCells; i++)
    {
      int downCellId = _cellIds[_nbDownCells * cellId + i];
      unsigned char cellType = _cellTypes[i];
      this->_grid->getDownArray(cellType)->getNodeIds(downCellId, nodeSet);
    }
}

/*! Find in vtkUnstructuredGrid the volumes containing a face already stored in vtkUnstructuredGrid.
 * Search the volumes containing a face, to store the info in SMDS_Down2D for later uses
 * with SMDS_Down2D::getUpCells and SMDS_Down2D::getUpTypes.
 * A face belongs to 0, 1 or 2 volumes, identified by their id in vtkUnstructuredGrid.
 * @param cellId the face cell id in vkUnstructuredGrid
 * @param ids a couple of vtkId, initialized at -1 (no parent volume)
 * @return number of volumes (0, 1 or 2)
 */
int SMDS_Down2D::computeVolumeIds(int cellId, int* ids)
{
  // --- find point id's of the face

  vtkIdType npts = 0;
  vtkIdTypePtr pts; // will refer to the point id's of the face
  _grid->GetCellPoints(cellId, npts, pts);
  vector<int> nodes;
  for (int i = 0; i < npts; i++)
    nodes.push_back(pts[i]);
  int nvol = this->computeVolumeIdsFromNodesFace(&nodes[0], npts, ids);
  return nvol;
}

/*! Find in vtkUnstructuredGrid the volumes containing a face described by it's nodes
 * Search the volumes containing a face, to store the info in SMDS_Down2D for later uses
 * with SMDS_Down2D::getUpCells and SMDS_Down2D::getUpTypes.
 * A face belongs to 0, 1 or 2 volumes, identified by their id in vtkUnstructuredGrid.
 * @param faceByNodes
 * @param ids a couple of vtkId, initialized at -1 (no parent volume)
 * @return number of volumes (0, 1 or 2)
 */
int SMDS_Down2D::computeVolumeIds(ElemByNodesType& faceByNodes, int* ids)
{
  int nvol = this->computeVolumeIdsFromNodesFace(&faceByNodes.nodeIds[0], faceByNodes.nbNodes, ids);
  return nvol;
}

/*! Find in vtkUnstructuredGrid the volumes containing a face described by it's nodes
 * Search the volumes containing a face, to store the info in SMDS_Down2D for later uses
 * with SMDS_Down2D::getUpCells and SMDS_Down2D::getUpTypes.
 * A face belongs to 0, 1 or 2 volumes, identified by their id in vtkUnstructuredGrid.
 * @param pts array of vtk node id's
 * @param npts number of nodes
 * @param ids
 * @return number of volumes (0, 1 or 2)
 */
int SMDS_Down2D::computeVolumeIdsFromNodesFace(int* pts, int npts, int* ids)
{

  // --- find all the cells the points belong to, and how many of the points belong to a given cell

  int cellIds[1000];
  int cellCnt[1000];
  int cnt = 0;
  for (int i = 0; i < npts; i++)
    {
      vtkIdType point = pts[i];
      int numCells = _grid->GetLinks()->GetNcells(point);
      //MESSAGE("cells pour " << i << " " << numCells);
      vtkIdTypePtr cells = _grid->GetLinks()->GetCells(point);
      for (int j = 0; j < numCells; j++)
        {
          int vtkCellId = cells[j];
          bool found = false;
          for (int k = 0; k < cnt; k++)
            {
              if (cellIds[k] == vtkCellId)
                {
                  cellCnt[k] += 1;
                  found = true;
                  break;
                }
            }
          if (!found)
            {
              cellIds[cnt] = vtkCellId;
              cellCnt[cnt] = 1;
              // TODO ASSERT(cnt<1000);
              cnt++;
            }
        }
    }

  // --- find the volume cells: they contains all the points and are of type volume

  int nvol = 0;
  for (int i = 0; i < cnt; i++)
    {
      //MESSAGE("cell " << cellIds[i] << " points " << cellCnt[i]);
      if (cellCnt[i] == npts)
        {
          int vtkElemId = cellIds[i];
          int vtkType = _grid->GetCellType(vtkElemId);
          if (SMDS_Downward::getCellDimension(vtkType) == 3)
            {
              ids[nvol] = vtkElemId; // store the volume id in given vector
              nvol++;
            }
        }
      if (nvol == 2)
        break;
    }

  return nvol;
}

void SMDS_Down2D::setTempNodes(int cellId, int vtkId)
{
  vtkIdType npts = 0;
  vtkIdTypePtr pts; // will refer to the point id's of the face
  _grid->GetCellPoints(vtkId, npts, pts);
  // MESSAGE(vtkId << " " << npts << "  " << _nbNodes);
  //ASSERT(npts == _nbNodes);
  for (int i = 0; i < npts; i++)
    {
      _tempNodes[_nbNodes * cellId + i] = pts[i];
    }
}

void SMDS_Down2D::setTempNodes(int cellId, ElemByNodesType& faceByNodes)
{
  for (int i = 0; i < faceByNodes.nbNodes; i++)
    _tempNodes[_nbNodes * cellId + i] = faceByNodes.nodeIds[i];
}

/*! Find if all the nodes belongs to the face.
 *
 * @param cellId the face cell Id
 * @param nodeSet set of node id's to be found in the face list of nodes
 * @return
 */
bool SMDS_Down2D::isInFace(int cellId, int *pts, int npts)
{
  int nbFound = 0;
  int *nodes = &_tempNodes[_nbNodes * cellId];
  for (int j = 0; j < npts; j++)
    {
      int point = pts[j];
      for (int i = 0; i < _nbNodes; i++)
        {
          if (nodes[i] == point)
            {
              nbFound++;
              break;
            }
        }
    }
  return (nbFound == npts);
}

/*! Resize the downward connectivity storage vector if needed.
 *
 * @param nbElems total number of elements of the same type required
 */
void SMDS_Down2D::allocate(int nbElems)
{
  if (nbElems >= _vtkCellIds.size())
    {
      _cellIds.resize(_nbDownCells * (nbElems + SMDS_Mesh::chunkSize), -1);
      _vtkCellIds.resize(nbElems + SMDS_Mesh::chunkSize, -1);
      _upCellIds.resize(2 * (nbElems + SMDS_Mesh::chunkSize), -1);
      _upCellTypes.resize(2 * (nbElems + SMDS_Mesh::chunkSize), -1);
      _tempNodes.resize(_nbNodes * (nbElems + SMDS_Mesh::chunkSize), -1);
    }
}

void SMDS_Down2D::compactStorage()
{
  _cellIds.resize(_nbDownCells * _maxId);
  _upCellIds.resize(2 * _maxId);
  _upCellTypes.resize(2 * _maxId);
  _vtkCellIds.resize(_maxId);
  _tempNodes.clear();
}

void SMDS_Down2D::addUpCell(int cellId, int upCellId, unsigned char aType)
{
  //ASSERT((cellId >=0)&& (cellId < _maxId));
  int *vols = &_upCellIds[2 * cellId];
  unsigned char *types = &_upCellTypes[2 * cellId];
  for (int i = 0; i < 2; i++)
    {
      if (vols[i] < 0)
        {
          vols[i] = upCellId; // use non affected volume
          types[i] = aType;
          return;
        }
      if ((vols[i] == upCellId) && (types[i] == aType)) // already done
        return;
    }
  ASSERT(0);
}

int SMDS_Down2D::getNodeSet(int cellId, int* nodeSet)
{
  for (int i = 0; i < _nbNodes; i++)
    nodeSet[i] = _tempNodes[_nbNodes * cellId + i];
  return _nbNodes;
}

int SMDS_Down2D::FindEdgeByNodes(int cellId, ElemByNodesType& edgeByNodes)
{
  int *edges = &_cellIds[_nbDownCells * cellId];
  for (int i = 0; i < _nbDownCells; i++)
    {
      if ((edges[i] >= 0) && (edgeByNodes.vtkType == _cellTypes[i]))
        {
          int nodeSet[3];
          int npts = this->_grid->getDownArray(edgeByNodes.vtkType)->getNodeSet(edges[i], nodeSet);
          bool found = false;
          for (int j = 0; j < npts; j++)
            {
              int point = edgeByNodes.nodeIds[j];
              found = false;
              for (int k = 0; k < npts; k++)
                {
                  if (nodeSet[k] == point)
                    {
                      found = true;
                      break;
                    }
                }
              if (!found)
                break;
            }
          if (found)
            return edges[i];
        }
    }
  return -1;
}

// ---------------------------------------------------------------------------

SMDS_Down3D::SMDS_Down3D(SMDS_UnstructuredGrid *grid, int nbDownCells) :
  SMDS_Downward(grid, nbDownCells)
{
}

SMDS_Down3D::~SMDS_Down3D()
{
}

void SMDS_Down3D::allocate(int nbElems)
{
  if (nbElems >= _vtkCellIds.size())
    {
      _cellIds.resize(_nbDownCells * (nbElems + SMDS_Mesh::chunkSize), -1);
      _vtkCellIds.resize(nbElems + SMDS_Mesh::chunkSize, -1);
    }
}

void SMDS_Down3D::compactStorage()
{
  // nothing to do, size was known before
}

int SMDS_Down3D::getNumberOfUpCells(int cellId)
{
  return 0;
}

const int* SMDS_Down3D::getUpCells(int cellId)
{
  return 0;
}

const unsigned char* SMDS_Down3D::getUpTypes(int cellId)
{
  return 0;
}

void SMDS_Down3D::getNodeIds(int cellId, std::set<int>& nodeSet)
{
  int vtkId = this->_vtkCellIds[cellId];
  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(vtkId, npts, nodes);
  for (int i = 0; i < npts; i++)
    nodeSet.insert(nodes[i]);
}

int SMDS_Down3D::FindFaceByNodes(int cellId, ElemByNodesType& faceByNodes)
{
  int *faces = &_cellIds[_nbDownCells * cellId];
  int npoints = 0;

  for (int i = 0; i < _nbDownCells; i++)
    {
      if ((faces[i] >= 0) && (faceByNodes.vtkType == _cellTypes[i]))
        {
          if (npoints == 0)
            npoints = faceByNodes.nbNodes;

          int nodeSet[10];
          int npts = this->_grid->getDownArray(faceByNodes.vtkType)->getNodeSet(faces[i], nodeSet);
          if (npts != npoints)
            continue; // skip this face
          bool found = false;
          for (int j = 0; j < npts; j++)
            {
              int point = faceByNodes.nodeIds[j];
              found = false;
              for (int k = 0; k < npts; k++)
                {
                  if (nodeSet[k] == point)
                    {
                      found = true;
                      break; // point j is in the 2 faces, skip remaining k values
                    }
                }
              if (!found)
                break; // point j is not in the 2 faces, skip the remaining tests
            }
          if (found)
            return faces[i];
        }
    }
  return -1;
}

// ---------------------------------------------------------------------------

SMDS_DownEdge::SMDS_DownEdge(SMDS_UnstructuredGrid *grid) :
  SMDS_Down1D(grid, 2)
{
  _cellTypes.push_back(VTK_VERTEX);
  _cellTypes.push_back(VTK_VERTEX);
}

SMDS_DownEdge::~SMDS_DownEdge()
{
}

// ---------------------------------------------------------------------------

SMDS_DownQuadEdge::SMDS_DownQuadEdge(SMDS_UnstructuredGrid *grid) :
  SMDS_Down1D(grid, 3)
{
  _cellTypes.push_back(VTK_VERTEX);
  _cellTypes.push_back(VTK_VERTEX);
  _cellTypes.push_back(VTK_VERTEX);
}

SMDS_DownQuadEdge::~SMDS_DownQuadEdge()
{
}

// ---------------------------------------------------------------------------

SMDS_DownTriangle::SMDS_DownTriangle(SMDS_UnstructuredGrid *grid) :
  SMDS_Down2D(grid, 3)
{
  _cellTypes.push_back(VTK_LINE);
  _cellTypes.push_back(VTK_LINE);
  _cellTypes.push_back(VTK_LINE);
  _nbNodes = 3;
}

SMDS_DownTriangle::~SMDS_DownTriangle()
{
}

void SMDS_DownTriangle::computeEdgesWithNodes(int cellId, ListElemByNodesType& edgesWithNodes)
{
  int *nodes = &_tempNodes[_nbNodes * cellId];
  edgesWithNodes.nbElems = 3;

  edgesWithNodes.elems[0].nodeIds[0] = nodes[0];
  edgesWithNodes.elems[0].nodeIds[1] = nodes[1];
  edgesWithNodes.elems[0].nbNodes = 2;
  edgesWithNodes.elems[0].vtkType = VTK_LINE;

  edgesWithNodes.elems[1].nodeIds[0] = nodes[1];
  edgesWithNodes.elems[1].nodeIds[1] = nodes[2];
  edgesWithNodes.elems[1].nbNodes = 2;
  edgesWithNodes.elems[1].vtkType = VTK_LINE;

  edgesWithNodes.elems[2].nodeIds[0] = nodes[2];
  edgesWithNodes.elems[2].nodeIds[1] = nodes[0];
  edgesWithNodes.elems[2].nbNodes = 2;
  edgesWithNodes.elems[2].vtkType = VTK_LINE;
}

void SMDS_DownTriangle::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0)&& (cellId < _maxId));
  //ASSERT(aType == VTK_LINE);
  int *faces = &_cellIds[_nbDownCells * cellId];
  for (int i = 0; i < _nbDownCells; i++)
    {
      if (faces[i] < 0)
        {
          faces[i] = lowCellId;
          return;
        }
      if (faces[i] == lowCellId)
        return;
    }
  ASSERT(0);
}

// ---------------------------------------------------------------------------

SMDS_DownQuadTriangle::SMDS_DownQuadTriangle(SMDS_UnstructuredGrid *grid) :
  SMDS_Down2D(grid, 3)
{
  _cellTypes.push_back(VTK_QUADRATIC_EDGE);
  _cellTypes.push_back(VTK_QUADRATIC_EDGE);
  _cellTypes.push_back(VTK_QUADRATIC_EDGE);
  _nbNodes = 6;
}

SMDS_DownQuadTriangle::~SMDS_DownQuadTriangle()
{
}

void SMDS_DownQuadTriangle::computeEdgesWithNodes(int cellId, ListElemByNodesType& edgesWithNodes)
{
  int *nodes = &_tempNodes[_nbNodes * cellId];
  edgesWithNodes.nbElems = 3;

  edgesWithNodes.elems[0].nodeIds[0] = nodes[0];
  edgesWithNodes.elems[0].nodeIds[1] = nodes[1];
  edgesWithNodes.elems[0].nodeIds[2] = nodes[3];
  edgesWithNodes.elems[0].nbNodes = 3;
  edgesWithNodes.elems[0].vtkType = VTK_QUADRATIC_EDGE;

  edgesWithNodes.elems[1].nodeIds[0] = nodes[1];
  edgesWithNodes.elems[1].nodeIds[1] = nodes[2];
  edgesWithNodes.elems[1].nodeIds[2] = nodes[4];
  edgesWithNodes.elems[1].nbNodes = 3;
  edgesWithNodes.elems[1].vtkType = VTK_QUADRATIC_EDGE;

  edgesWithNodes.elems[2].nodeIds[0] = nodes[2];
  edgesWithNodes.elems[2].nodeIds[1] = nodes[0];
  edgesWithNodes.elems[2].nodeIds[2] = nodes[5];
  edgesWithNodes.elems[2].nbNodes = 3;
  edgesWithNodes.elems[2].vtkType = VTK_QUADRATIC_EDGE;
}

void SMDS_DownQuadTriangle::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0)&& (cellId < _maxId));
  //ASSERT(aType == VTK_QUADRATIC_EDGE);
  int *edges = &_cellIds[_nbDownCells * cellId];
  for (int i = 0; i < _nbDownCells; i++)
    {
      if (edges[i] < 0)
        {
          edges[i] = lowCellId;
          return;
        }
      if (edges[i] == lowCellId)
        return;
    }
  ASSERT(0);
}

// ---------------------------------------------------------------------------

SMDS_DownQuadrangle::SMDS_DownQuadrangle(SMDS_UnstructuredGrid *grid) :
  SMDS_Down2D(grid, 4)
{
  _cellTypes.push_back(VTK_LINE);
  _cellTypes.push_back(VTK_LINE);
  _cellTypes.push_back(VTK_LINE);
  _cellTypes.push_back(VTK_LINE);
  _nbNodes = 4;
}

SMDS_DownQuadrangle::~SMDS_DownQuadrangle()
{
}

void SMDS_DownQuadrangle::computeEdgesWithNodes(int cellId, ListElemByNodesType& edgesWithNodes)
{
  int *nodes = &_tempNodes[_nbNodes * cellId];
  edgesWithNodes.nbElems = 4;

  edgesWithNodes.elems[0].nodeIds[0] = nodes[0];
  edgesWithNodes.elems[0].nodeIds[1] = nodes[1];
  edgesWithNodes.elems[0].nbNodes = 2;
  edgesWithNodes.elems[0].vtkType = VTK_LINE;

  edgesWithNodes.elems[1].nodeIds[0] = nodes[1];
  edgesWithNodes.elems[1].nodeIds[1] = nodes[2];
  edgesWithNodes.elems[1].nbNodes = 2;
  edgesWithNodes.elems[1].vtkType = VTK_LINE;

  edgesWithNodes.elems[2].nodeIds[0] = nodes[2];
  edgesWithNodes.elems[2].nodeIds[1] = nodes[3];
  edgesWithNodes.elems[2].nbNodes = 2;
  edgesWithNodes.elems[2].vtkType = VTK_LINE;

  edgesWithNodes.elems[3].nodeIds[0] = nodes[3];
  edgesWithNodes.elems[3].nodeIds[1] = nodes[0];
  edgesWithNodes.elems[3].nbNodes = 2;
  edgesWithNodes.elems[3].vtkType = VTK_LINE;
}

void SMDS_DownQuadrangle::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0)&& (cellId < _maxId));
  //ASSERT(aType == VTK_LINE);
  int *faces = &_cellIds[_nbDownCells * cellId];
  for (int i = 0; i < _nbDownCells; i++)
    {
      if (faces[i] < 0)
        {
          faces[i] = lowCellId;
          return;
        }
      if (faces[i] == lowCellId)
        return;
    }
  ASSERT(0);
}

// ---------------------------------------------------------------------------

SMDS_DownQuadQuadrangle::SMDS_DownQuadQuadrangle(SMDS_UnstructuredGrid *grid) :
  SMDS_Down2D(grid, 4)
{
  _cellTypes.push_back(VTK_QUADRATIC_EDGE);
  _cellTypes.push_back(VTK_QUADRATIC_EDGE);
  _cellTypes.push_back(VTK_QUADRATIC_EDGE);
  _cellTypes.push_back(VTK_QUADRATIC_EDGE);
  _nbNodes = 8;
}

SMDS_DownQuadQuadrangle::~SMDS_DownQuadQuadrangle()
{
}

void SMDS_DownQuadQuadrangle::computeEdgesWithNodes(int cellId, ListElemByNodesType& edgesWithNodes)
{
  int *nodes = &_tempNodes[_nbNodes * cellId];
  edgesWithNodes.nbElems = 4;

  edgesWithNodes.elems[0].nodeIds[0] = nodes[0];
  edgesWithNodes.elems[0].nodeIds[1] = nodes[1];
  edgesWithNodes.elems[0].nodeIds[2] = nodes[4];
  edgesWithNodes.elems[0].nbNodes = 3;
  edgesWithNodes.elems[0].vtkType = VTK_QUADRATIC_EDGE;

  edgesWithNodes.elems[1].nodeIds[0] = nodes[1];
  edgesWithNodes.elems[1].nodeIds[1] = nodes[2];
  edgesWithNodes.elems[1].nodeIds[2] = nodes[5];
  edgesWithNodes.elems[1].nbNodes = 3;
  edgesWithNodes.elems[1].vtkType = VTK_QUADRATIC_EDGE;

  edgesWithNodes.elems[2].nodeIds[0] = nodes[2];
  edgesWithNodes.elems[2].nodeIds[1] = nodes[3];
  edgesWithNodes.elems[2].nodeIds[2] = nodes[6];
  edgesWithNodes.elems[2].nbNodes = 3;
  edgesWithNodes.elems[2].vtkType = VTK_QUADRATIC_EDGE;

  edgesWithNodes.elems[3].nodeIds[0] = nodes[3];
  edgesWithNodes.elems[3].nodeIds[1] = nodes[0];
  edgesWithNodes.elems[3].nodeIds[2] = nodes[7];
  edgesWithNodes.elems[3].nbNodes = 3;
  edgesWithNodes.elems[3].vtkType = VTK_QUADRATIC_EDGE;
}

void SMDS_DownQuadQuadrangle::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0)&& (cellId < _maxId));
  //ASSERT(aType == VTK_QUADRATIC_EDGE);
  int *faces = &_cellIds[_nbDownCells * cellId];
  for (int i = 0; i < _nbDownCells; i++)
    {
      if (faces[i] < 0)
        {
          faces[i] = lowCellId;
          return;
        }
      if (faces[i] == lowCellId)
        return;
    }
  ASSERT(0);
}

// ---------------------------------------------------------------------------

SMDS_DownTetra::SMDS_DownTetra(SMDS_UnstructuredGrid *grid) :
  SMDS_Down3D(grid, 4)
{
  _cellTypes.push_back(VTK_TRIANGLE);
  _cellTypes.push_back(VTK_TRIANGLE);
  _cellTypes.push_back(VTK_TRIANGLE);
  _cellTypes.push_back(VTK_TRIANGLE);
}

SMDS_DownTetra::~SMDS_DownTetra()
{
}

void SMDS_DownTetra::getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes)
{
  set<int> setNodes;
  setNodes.clear();
  for (int i = 0; i < orderedNodes.size(); i++)
    setNodes.insert(orderedNodes[i]);
  //MESSAGE("cellId = " << cellId);

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(this->_vtkCellIds[cellId], npts, nodes);

  set<int> tofind;
  int ids[12] = { 0, 1, 2,  0, 3, 1,  2, 3, 0,   1, 3, 2 };
//int ids[12] = { 2, 1, 0,  1, 3, 0,  0, 3, 2,   2, 3, 1 };
  for (int k = 0; k < 4; k++)
    {
      tofind.clear();
      for (int i = 0; i < 3; i++)
        tofind.insert(nodes[ids[3 * k + i]]);
      if (setNodes == tofind)
        {
          for (int i = 0; i < 3; i++)
            orderedNodes[i] = nodes[ids[3 * k + i]];
          return;
        }
    }
  MESSAGE("=== Problem volume " << _vtkCellIds[cellId] << " " << _grid->_mesh->fromVtkToSmds(_vtkCellIds[cellId]));
  MESSAGE(orderedNodes[0] << " " << orderedNodes[1] << " " << orderedNodes[2]);
  MESSAGE(nodes[0] << " " << nodes[1] << " " << nodes[2] << " " << nodes[3]);
}

void SMDS_DownTetra::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0)&& (cellId < _maxId));
  //ASSERT(aType == VTK_TRIANGLE);
  int *faces = &_cellIds[_nbDownCells * cellId];
  for (int i = 0; i < _nbDownCells; i++)
    {
      if (faces[i] < 0)
        {
          faces[i] = lowCellId;
          return;
        }
      if (faces[i] == lowCellId)
        return;
    }
  ASSERT(0);
}

/*! Create a list of faces described by a vtk Type and  an ordered set of Node Id's
 * The linear tetrahedron is defined by four points.
 * @see vtkTetra.h in Filtering.
 * @param cellId volumeId in vtkUnstructuredGrid
 * @param facesWithNodes vector of face descriptors to be filled
 */
void SMDS_DownTetra::computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes)
{
  // --- find point id's of the volume

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(cellId, npts, nodes);

  // --- create all the ordered list of node id's for each face

  facesWithNodes.nbElems = 4;

  facesWithNodes.elems[0].nodeIds[0] = nodes[0];
  facesWithNodes.elems[0].nodeIds[1] = nodes[1];
  facesWithNodes.elems[0].nodeIds[2] = nodes[2];
  facesWithNodes.elems[0].nbNodes = 3;
  facesWithNodes.elems[0].vtkType = VTK_TRIANGLE;

  facesWithNodes.elems[1].nodeIds[0] = nodes[0];
  facesWithNodes.elems[1].nodeIds[1] = nodes[1];
  facesWithNodes.elems[1].nodeIds[2] = nodes[3];
  facesWithNodes.elems[1].nbNodes = 3;
  facesWithNodes.elems[1].vtkType = VTK_TRIANGLE;

  facesWithNodes.elems[2].nodeIds[0] = nodes[0];
  facesWithNodes.elems[2].nodeIds[1] = nodes[2];
  facesWithNodes.elems[2].nodeIds[2] = nodes[3];
  facesWithNodes.elems[2].nbNodes = 3;
  facesWithNodes.elems[2].vtkType = VTK_TRIANGLE;

  facesWithNodes.elems[3].nodeIds[0] = nodes[1];
  facesWithNodes.elems[3].nodeIds[1] = nodes[2];
  facesWithNodes.elems[3].nodeIds[2] = nodes[3];
  facesWithNodes.elems[3].nbNodes = 3;
  facesWithNodes.elems[3].vtkType = VTK_TRIANGLE;
}

// ---------------------------------------------------------------------------

SMDS_DownQuadTetra::SMDS_DownQuadTetra(SMDS_UnstructuredGrid *grid) :
  SMDS_Down3D(grid, 4)
{
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
}

SMDS_DownQuadTetra::~SMDS_DownQuadTetra()
{
}

void SMDS_DownQuadTetra::getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes)
{
  set<int> setNodes;
  setNodes.clear();
  for (int i = 0; i < orderedNodes.size(); i++)
    setNodes.insert(orderedNodes[i]);
  //MESSAGE("cellId = " << cellId);

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(this->_vtkCellIds[cellId], npts, nodes);

  set<int> tofind;
  int ids[24] = { 0, 1, 2, 4, 5, 6,  0, 3, 1, 7, 8, 4,  2, 3, 0, 9, 7, 6,  1, 3, 2, 8, 9, 5 };
//int ids[24] = { 2, 1, 0, 5, 4, 6,  1, 3, 0, 8, 7, 4,  0, 3, 2, 7, 9, 6,  2, 3, 1, 9, 8, 5 };
  for (int k = 0; k < 4; k++)
    {
      tofind.clear();
      for (int i = 0; i < 6; i++)
        tofind.insert(nodes[ids[6 * k + i]]);
      if (setNodes == tofind)
        {
          for (int i = 0; i < 6; i++)
            orderedNodes[i] = nodes[ids[6 * k + i]];
          return;
        }
    }
  MESSAGE("=== Problem volume " << _vtkCellIds[cellId] << " " << _grid->_mesh->fromVtkToSmds(_vtkCellIds[cellId]));
  MESSAGE(orderedNodes[0] << " " << orderedNodes[1] << " " << orderedNodes[2]);
  MESSAGE(nodes[0] << " " << nodes[1] << " " << nodes[2] << " " << nodes[3]);
}

void SMDS_DownQuadTetra::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0)&& (cellId < _maxId));
  //ASSERT(aType == VTK_QUADRATIC_TRIANGLE);
  int *faces = &_cellIds[_nbDownCells * cellId];
  for (int i = 0; i < _nbDownCells; i++)
    {
      if (faces[i] < 0)
        {
          faces[i] = lowCellId;
          return;
        }
      if (faces[i] == lowCellId)
        return;
    }
  ASSERT(0);
}

/*! Create a list of faces described by a vtk Type and  an ordered set of Node Id's
 * The ordering of the ten points defining the quadratic tetrahedron cell is point id's (0-3,4-9)
 * where id's 0-3 are the four tetrahedron vertices;
 * and point id's 4-9 are the mid-edge nodes between (0,1), (1,2), (2,0), (0,3), (1,3), and (2,3).
 * @see vtkQuadraticTetra.h in Filtering.
 * @param cellId volumeId in vtkUnstructuredGrid
 * @param facesWithNodes vector of face descriptors to be filled
 */
void SMDS_DownQuadTetra::computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes)
{
  // --- find point id's of the volume

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(cellId, npts, nodes);

  // --- create all the ordered list of node id's for each face

  facesWithNodes.nbElems = 4;

  facesWithNodes.elems[0].nodeIds[0] = nodes[0];
  facesWithNodes.elems[0].nodeIds[1] = nodes[1];
  facesWithNodes.elems[0].nodeIds[2] = nodes[2];
  facesWithNodes.elems[0].nodeIds[3] = nodes[4];
  facesWithNodes.elems[0].nodeIds[4] = nodes[5];
  facesWithNodes.elems[0].nodeIds[5] = nodes[6];
  facesWithNodes.elems[0].nbNodes = 6;
  facesWithNodes.elems[0].vtkType = VTK_QUADRATIC_TRIANGLE;

  facesWithNodes.elems[1].nodeIds[0] = nodes[0];
  facesWithNodes.elems[1].nodeIds[1] = nodes[1];
  facesWithNodes.elems[1].nodeIds[2] = nodes[3];
  facesWithNodes.elems[1].nodeIds[3] = nodes[4];
  facesWithNodes.elems[1].nodeIds[4] = nodes[8];
  facesWithNodes.elems[1].nodeIds[5] = nodes[7];
  facesWithNodes.elems[1].nbNodes = 6;
  facesWithNodes.elems[1].vtkType = VTK_QUADRATIC_TRIANGLE;

  facesWithNodes.elems[2].nodeIds[0] = nodes[0];
  facesWithNodes.elems[2].nodeIds[1] = nodes[2];
  facesWithNodes.elems[2].nodeIds[2] = nodes[3];
  facesWithNodes.elems[2].nodeIds[3] = nodes[6];
  facesWithNodes.elems[2].nodeIds[4] = nodes[9];
  facesWithNodes.elems[2].nodeIds[5] = nodes[7];
  facesWithNodes.elems[2].nbNodes = 6;
  facesWithNodes.elems[2].vtkType = VTK_QUADRATIC_TRIANGLE;

  facesWithNodes.elems[3].nodeIds[0] = nodes[1];
  facesWithNodes.elems[3].nodeIds[1] = nodes[2];
  facesWithNodes.elems[3].nodeIds[2] = nodes[3];
  facesWithNodes.elems[3].nodeIds[3] = nodes[5];
  facesWithNodes.elems[3].nodeIds[4] = nodes[9];
  facesWithNodes.elems[3].nodeIds[5] = nodes[8];
  facesWithNodes.elems[3].nbNodes = 6;
  facesWithNodes.elems[3].vtkType = VTK_QUADRATIC_TRIANGLE;
}

// ---------------------------------------------------------------------------

SMDS_DownPyramid::SMDS_DownPyramid(SMDS_UnstructuredGrid *grid) :
  SMDS_Down3D(grid, 5)
{
  _cellTypes.push_back(VTK_QUAD);
  _cellTypes.push_back(VTK_TRIANGLE);
  _cellTypes.push_back(VTK_TRIANGLE);
  _cellTypes.push_back(VTK_TRIANGLE);
  _cellTypes.push_back(VTK_TRIANGLE);
}

SMDS_DownPyramid::~SMDS_DownPyramid()
{
}

void SMDS_DownPyramid::getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes)
{
  set<int> setNodes;
  setNodes.clear();
  for (int i = 0; i < orderedNodes.size(); i++)
    setNodes.insert(orderedNodes[i]);
  //MESSAGE("cellId = " << cellId);

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(this->_vtkCellIds[cellId], npts, nodes);

  set<int> tofind;
  int ids[16] = { 0, 1, 2, 3,   0, 3, 4,   3, 2, 4,   2, 1, 4,   1, 0, 4 };

  // Quadrangular face
  tofind.clear();
  for (int i = 0; i < 4; i++)
    tofind.insert(nodes[ids[i]]);
  if (setNodes == tofind)
    {
      for (int i = 0; i < 4; i++)
        orderedNodes[i] = nodes[ids[i]];
      return;
    }
  // Triangular faces
  for (int k = 0; k < 4; k++)
    {
      tofind.clear();
      for (int i = 0; i < 3; i++)
        tofind.insert(nodes[ids[4 + 3 * k + i]]);
      if (setNodes == tofind)
        {
          for (int i = 0; i < 3; i++)
            orderedNodes[i] = nodes[ids[4 + 3 * k + i]];
          return;
        }
    }
  MESSAGE("=== Problem volume " << _vtkCellIds[cellId] << " " << _grid->_mesh->fromVtkToSmds(_vtkCellIds[cellId]));
  MESSAGE(orderedNodes[0] << " " << orderedNodes[1] << " " << orderedNodes[2]);
  MESSAGE(nodes[0] << " " << nodes[1] << " " << nodes[2] << " " << nodes[3]);
}

void SMDS_DownPyramid::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  int *faces = &_cellIds[_nbDownCells * cellId];
  if (aType == VTK_QUAD)
    {
      if (faces[0] < 0)
        {
          faces[0] = lowCellId;
          return;
        }
      if (faces[0] == lowCellId)
        return;
    }
  else
    {
      //ASSERT(aType == VTK_TRIANGLE);
      for (int i = 1; i < _nbDownCells; i++)
        {
          if (faces[i] < 0)
            {
              faces[i] = lowCellId;
              return;
            }
          if (faces[i] == lowCellId)
            return;
        }
    }
  ASSERT(0);
}

/*! Create a list of faces described by a vtk Type and  an ordered set of Node Id's
 * The pyramid is defined by the five points (0-4) where (0,1,2,3) is the base of the pyramid which,
 * using the right hand rule, forms a quadrilateral whose normal points in the direction of the
 * pyramid apex at vertex #4.
 * @see vtkPyramid.h in Filtering.
 * @param cellId volumeId in vtkUnstructuredGrid
 * @param facesWithNodes vector of face descriptors to be filled
 */
void SMDS_DownPyramid::computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes)
{
  // --- find point id's of the volume

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(cellId, npts, nodes);

  // --- create all the ordered list of node id's for each face

  facesWithNodes.nbElems = 5;

  facesWithNodes.elems[0].nodeIds[0] = nodes[0];
  facesWithNodes.elems[0].nodeIds[1] = nodes[1];
  facesWithNodes.elems[0].nodeIds[2] = nodes[2];
  facesWithNodes.elems[0].nodeIds[3] = nodes[3];
  facesWithNodes.elems[0].nbNodes = 4;
  facesWithNodes.elems[0].vtkType = VTK_QUAD;

  facesWithNodes.elems[1].nodeIds[0] = nodes[0];
  facesWithNodes.elems[1].nodeIds[1] = nodes[1];
  facesWithNodes.elems[1].nodeIds[2] = nodes[4];
  facesWithNodes.elems[1].nbNodes = 3;
  facesWithNodes.elems[1].vtkType = VTK_TRIANGLE;

  facesWithNodes.elems[2].nodeIds[0] = nodes[1];
  facesWithNodes.elems[2].nodeIds[1] = nodes[2];
  facesWithNodes.elems[2].nodeIds[2] = nodes[4];
  facesWithNodes.elems[2].nbNodes = 3;
  facesWithNodes.elems[2].vtkType = VTK_TRIANGLE;

  facesWithNodes.elems[3].nodeIds[0] = nodes[2];
  facesWithNodes.elems[3].nodeIds[1] = nodes[3];
  facesWithNodes.elems[3].nodeIds[2] = nodes[4];
  facesWithNodes.elems[3].nbNodes = 3;
  facesWithNodes.elems[3].vtkType = VTK_TRIANGLE;

  facesWithNodes.elems[4].nodeIds[0] = nodes[3];
  facesWithNodes.elems[4].nodeIds[1] = nodes[0];
  facesWithNodes.elems[4].nodeIds[2] = nodes[4];
  facesWithNodes.elems[4].nbNodes = 3;
  facesWithNodes.elems[4].vtkType = VTK_TRIANGLE;
}

// ---------------------------------------------------------------------------

SMDS_DownQuadPyramid::SMDS_DownQuadPyramid(SMDS_UnstructuredGrid *grid) :
  SMDS_Down3D(grid, 5)
{
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
}

SMDS_DownQuadPyramid::~SMDS_DownQuadPyramid()
{
}

void SMDS_DownQuadPyramid::getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes)
{
//   MESSAGE("SMDS_DownQuadPyramid::getOrderedNodesOfFace cellId = " << cellId);
  set<int> setNodes;
  setNodes.clear();
  for (int i = 0; i < orderedNodes.size(); i++)
    setNodes.insert(orderedNodes[i]);
  //MESSAGE("cellId = " << cellId);

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(this->_vtkCellIds[cellId], npts, nodes);

  set<int> tofind;
  int ids[32] = { 0, 1, 2, 3, 5, 6, 7, 8,
                  0, 3, 4, 8, 12, 9,   3, 2, 4, 7 , 11, 12,   2, 1, 4, 6, 10, 11,   1, 0, 4, 5, 9, 10 };

  // Quadrangular face
  tofind.clear();
  for (int i = 0; i < 8; i++)
    tofind.insert(nodes[ids[i]]);
  if (setNodes == tofind)
    {
      for (int i = 0; i < 8; i++)
        orderedNodes[i] = nodes[ids[i]];
      return;
    }
  // Triangular faces
  for (int k = 0; k < 4; k++)
    {
      tofind.clear();
      for (int i = 0; i < 6; i++)
        tofind.insert(nodes[ids[8 + 6 * k + i]]);
      if (setNodes == tofind)
        {
          for (int i = 0; i < 6; i++)
            orderedNodes[i] = nodes[ids[8 + 6 * k + i]];
          return;
        }
    }
  MESSAGE("=== Problem volume " << _vtkCellIds[cellId] << " " << _grid->_mesh->fromVtkToSmds(_vtkCellIds[cellId]));
  MESSAGE(orderedNodes[0] << " " << orderedNodes[1] << " " << orderedNodes[2] << " " << orderedNodes[3]);
  MESSAGE(nodes[0] << " " << nodes[1] << " " << nodes[2] << " " << nodes[3]);
}

void SMDS_DownQuadPyramid::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  int *faces = &_cellIds[_nbDownCells * cellId];
  if (aType == VTK_QUADRATIC_QUAD)
    {
      if (faces[0] < 0)
        {
          faces[0] = lowCellId;
          return;
        }
      if (faces[0] == lowCellId)
        return;
    }
  else
    {
      //ASSERT(aType == VTK_QUADRATIC_TRIANGLE);
      for (int i = 1; i < _nbDownCells; i++)
        {
          if (faces[i] < 0)
            {
              faces[i] = lowCellId;
              return;
            }
          if (faces[i] == lowCellId)
            return;
        }
    }
  ASSERT(0);
}

/*! Create a list of faces described by a vtk Type and  an ordered set of Node Id's
 * The ordering of the thirteen points defining the quadratic pyramid cell is point id's (0-4,5-12)
 * where point id's 0-4 are the five corner vertices of the pyramid; followed
 * by eight mid-edge nodes (5-12). Note that these mid-edge nodes lie on the edges defined by
 * 5(0,1), 6(1,2), 7(2,3), 8(3,0), 9(0,4), 10(1,4), 11(2,4), 12(3,4).
 * @see vtkQuadraticPyramid.h in Filtering.
 * @param cellId volumeId in vtkUnstructuredGrid
 * @param facesWithNodes vector of face descriptors to be filled
 */
void SMDS_DownQuadPyramid::computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes)
{
  // --- find point id's of the volume

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(cellId, npts, nodes);

  // --- create all the ordered list of node id's for each face

  facesWithNodes.nbElems = 5;

  facesWithNodes.elems[0].nodeIds[0] = nodes[0];
  facesWithNodes.elems[0].nodeIds[1] = nodes[1];
  facesWithNodes.elems[0].nodeIds[2] = nodes[2];
  facesWithNodes.elems[0].nodeIds[3] = nodes[3];
  facesWithNodes.elems[0].nodeIds[4] = nodes[5];
  facesWithNodes.elems[0].nodeIds[5] = nodes[6];
  facesWithNodes.elems[0].nodeIds[6] = nodes[7];
  facesWithNodes.elems[0].nodeIds[7] = nodes[8];
  facesWithNodes.elems[0].nbNodes = 8;
  facesWithNodes.elems[0].vtkType = VTK_QUADRATIC_QUAD;

  facesWithNodes.elems[1].nodeIds[0] = nodes[0];
  facesWithNodes.elems[1].nodeIds[1] = nodes[1];
  facesWithNodes.elems[1].nodeIds[2] = nodes[4];
  facesWithNodes.elems[1].nodeIds[3] = nodes[5];
  facesWithNodes.elems[1].nodeIds[4] = nodes[10];
  facesWithNodes.elems[1].nodeIds[5] = nodes[9];
  facesWithNodes.elems[1].nbNodes = 6;
  facesWithNodes.elems[1].vtkType = VTK_QUADRATIC_TRIANGLE;

  facesWithNodes.elems[2].nodeIds[0] = nodes[1];
  facesWithNodes.elems[2].nodeIds[1] = nodes[2];
  facesWithNodes.elems[2].nodeIds[2] = nodes[4];
  facesWithNodes.elems[2].nodeIds[3] = nodes[6];
  facesWithNodes.elems[2].nodeIds[4] = nodes[11];
  facesWithNodes.elems[2].nodeIds[5] = nodes[10];
  facesWithNodes.elems[2].nbNodes = 6;
  facesWithNodes.elems[2].vtkType = VTK_QUADRATIC_TRIANGLE;

  facesWithNodes.elems[3].nodeIds[0] = nodes[2];
  facesWithNodes.elems[3].nodeIds[1] = nodes[3];
  facesWithNodes.elems[3].nodeIds[2] = nodes[4];
  facesWithNodes.elems[3].nodeIds[3] = nodes[7];
  facesWithNodes.elems[3].nodeIds[4] = nodes[12];
  facesWithNodes.elems[3].nodeIds[5] = nodes[11];
  facesWithNodes.elems[3].nbNodes = 6;
  facesWithNodes.elems[3].vtkType = VTK_QUADRATIC_TRIANGLE;

  facesWithNodes.elems[4].nodeIds[0] = nodes[3];
  facesWithNodes.elems[4].nodeIds[1] = nodes[0];
  facesWithNodes.elems[4].nodeIds[2] = nodes[4];
  facesWithNodes.elems[4].nodeIds[3] = nodes[8];
  facesWithNodes.elems[4].nodeIds[4] = nodes[9];
  facesWithNodes.elems[4].nodeIds[5] = nodes[12];
  facesWithNodes.elems[4].nbNodes = 6;
  facesWithNodes.elems[4].vtkType = VTK_QUADRATIC_TRIANGLE;
}

// ---------------------------------------------------------------------------

SMDS_DownPenta::SMDS_DownPenta(SMDS_UnstructuredGrid *grid) :
  SMDS_Down3D(grid, 5)
{
  _cellTypes.push_back(VTK_QUAD);
  _cellTypes.push_back(VTK_QUAD);
  _cellTypes.push_back(VTK_QUAD);
  _cellTypes.push_back(VTK_TRIANGLE);
  _cellTypes.push_back(VTK_TRIANGLE);
}

SMDS_DownPenta::~SMDS_DownPenta()
{
}

void SMDS_DownPenta::getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes)
{
  set<int> setNodes;
  setNodes.clear();
  for (int i = 0; i < orderedNodes.size(); i++)
    setNodes.insert(orderedNodes[i]);
  //MESSAGE("cellId = " << cellId);

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(this->_vtkCellIds[cellId], npts, nodes);

  set<int> tofind;
//int ids[18] = { 0, 2, 1,  3, 4, 5,   0, 1, 4, 3,   1, 2, 5, 4,   2, 0, 3, 5 };
  int ids[18] = { 0, 1, 2,  3, 5, 4,   0, 3, 4, 1,   1, 4, 5, 2,   2, 5, 3, 0 };

  // Triangular faces
  for (int k = 0; k < 2; k++)
    {
      tofind.clear();
      for (int i = 0; i < 3; i++)
        tofind.insert(nodes[ids[3 * k + i]]);
      if (setNodes == tofind)
        {
          for (int i = 0; i < 3; i++)
            orderedNodes[i] = nodes[ids[3 * k + i]];
          return;
        }
    }
  // Quadrangular faces
  for (int k = 0; k < 3; k++)
    {
      tofind.clear();
      for (int i = 0; i < 4; i++)
        tofind.insert(nodes[ids[6 + 4 * k + i]]);
      if (setNodes == tofind)
        {
          for (int i = 0; i < 4; i++)
            orderedNodes[i] = nodes[ids[6 + 4 * k + i]];
          return;
        }
    }
  MESSAGE("=== Problem volume " << _vtkCellIds[cellId] << " " << _grid->_mesh->fromVtkToSmds(_vtkCellIds[cellId]));
  MESSAGE(orderedNodes[0] << " " << orderedNodes[1] << " " << orderedNodes[2]);
  MESSAGE(nodes[0] << " " << nodes[1] << " " << nodes[2] << " " << nodes[3]);
}

void SMDS_DownPenta::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  int *faces = &_cellIds[_nbDownCells * cellId];
  if (aType == VTK_QUAD)
    for (int i = 0; i < 3; i++)
      {
        if (faces[i] < 0)
          {
            faces[i] = lowCellId;
            return;
          }
        if (faces[i] == lowCellId)
          return;
      }
  else
    {
      //ASSERT(aType == VTK_TRIANGLE);
      for (int i = 3; i < _nbDownCells; i++)
        {
          if (faces[i] < 0)
            {
              faces[i] = lowCellId;
              return;
            }
          if (faces[i] == lowCellId)
            return;
        }
    }
  ASSERT(0);
}

/*! Create a list of faces described by a vtk Type and  an ordered set of Node Id's.
 * A wedge or pentahedron consists of two triangular and three quadrilateral faces
 * and is defined by the six points (0-5) where (0,1,2) is the base of the wedge which,
 * using the right hand rule, forms a triangle whose normal points outward
 * (away from the triangular face (3,4,5)).
 * @see vtkWedge.h in Filtering
 * @param cellId volumeId in vtkUnstructuredGrid
 * @param facesWithNodes vector of face descriptors to be filled
 */
void SMDS_DownPenta::computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes)
{
  // --- find point id's of the volume

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(cellId, npts, nodes);

  // --- create all the ordered list of node id's for each face

  facesWithNodes.nbElems = 5;

  facesWithNodes.elems[0].nodeIds[0] = nodes[0];
  facesWithNodes.elems[0].nodeIds[1] = nodes[2];
  facesWithNodes.elems[0].nodeIds[2] = nodes[5];
  facesWithNodes.elems[0].nodeIds[3] = nodes[3];
  facesWithNodes.elems[0].nbNodes = 4;
  facesWithNodes.elems[0].vtkType = VTK_QUAD;

  facesWithNodes.elems[1].nodeIds[0] = nodes[1];
  facesWithNodes.elems[1].nodeIds[1] = nodes[2];
  facesWithNodes.elems[1].nodeIds[2] = nodes[5];
  facesWithNodes.elems[1].nodeIds[3] = nodes[4];
  facesWithNodes.elems[1].nbNodes = 4;
  facesWithNodes.elems[1].vtkType = VTK_QUAD;

  facesWithNodes.elems[2].nodeIds[0] = nodes[0];
  facesWithNodes.elems[2].nodeIds[1] = nodes[1];
  facesWithNodes.elems[2].nodeIds[2] = nodes[4];
  facesWithNodes.elems[2].nodeIds[3] = nodes[3];
  facesWithNodes.elems[2].nbNodes = 4;
  facesWithNodes.elems[2].vtkType = VTK_QUAD;

  facesWithNodes.elems[3].nodeIds[0] = nodes[0];
  facesWithNodes.elems[3].nodeIds[1] = nodes[1];
  facesWithNodes.elems[3].nodeIds[2] = nodes[2];
  facesWithNodes.elems[3].nbNodes = 3;
  facesWithNodes.elems[3].vtkType = VTK_TRIANGLE;

  facesWithNodes.elems[4].nodeIds[0] = nodes[3];
  facesWithNodes.elems[4].nodeIds[1] = nodes[4];
  facesWithNodes.elems[4].nodeIds[2] = nodes[5];
  facesWithNodes.elems[4].nbNodes = 3;
  facesWithNodes.elems[4].vtkType = VTK_TRIANGLE;
}

// ---------------------------------------------------------------------------

SMDS_DownQuadPenta::SMDS_DownQuadPenta(SMDS_UnstructuredGrid *grid) :
  SMDS_Down3D(grid, 5)
{
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
  _cellTypes.push_back(VTK_QUADRATIC_TRIANGLE);
}

SMDS_DownQuadPenta::~SMDS_DownQuadPenta()
{
}

void SMDS_DownQuadPenta::getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes)
{
  set<int> setNodes;
  setNodes.clear();
  for (int i = 0; i < orderedNodes.size(); i++)
    setNodes.insert(orderedNodes[i]);
  //MESSAGE("cellId = " << cellId);

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(this->_vtkCellIds[cellId], npts, nodes);

  set<int> tofind;
//int ids[18] = { 0, 2, 1,  3, 4, 5,   0, 1, 4, 3,   1, 2, 5, 4,   2, 0, 3, 5 };
  int ids[36] = { 0, 1, 2, 6, 7, 8,  3, 5, 4, 11, 10, 9,
                  0, 3, 4, 1, 12, 9, 13, 6,   1, 4, 5, 2, 13, 10, 14, 7,   2, 5, 3, 0, 14, 11, 12, 8 };

  // Triangular faces
  for (int k = 0; k < 2; k++)
    {
      tofind.clear();
      for (int i = 0; i < 6; i++)
        tofind.insert(nodes[ids[6 * k + i]]);
      if (setNodes == tofind)
        {
          for (int i = 0; i < 6; i++)
            orderedNodes[i] = nodes[ids[6 * k + i]];
          return;
        }
    }
  // Quadrangular faces
  for (int k = 0; k < 3; k++)
    {
      tofind.clear();
      for (int i = 0; i < 8; i++)
        tofind.insert(nodes[ids[12 + 8 * k + i]]);
      if (setNodes == tofind)
        {
          for (int i = 0; i < 8; i++)
            orderedNodes[i] = nodes[ids[12 + 8 * k + i]];
          return;
        }
    }
  MESSAGE("=== Problem volume " << _vtkCellIds[cellId] << " " << _grid->_mesh->fromVtkToSmds(_vtkCellIds[cellId]));
  MESSAGE(orderedNodes[0] << " " << orderedNodes[1] << " " << orderedNodes[2]);
  MESSAGE(nodes[0] << " " << nodes[1] << " " << nodes[2] << " " << nodes[3]);
}

void SMDS_DownQuadPenta::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0) && (cellId < _maxId));
  int *faces = &_cellIds[_nbDownCells * cellId];
  if (aType == VTK_QUADRATIC_QUAD)
    for (int i = 0; i < 3; i++)
      {
        if (faces[i] < 0)
          {
            faces[i] = lowCellId;
            return;
          }
        if (faces[i] == lowCellId)
          return;
      }
  else
    {
      //ASSERT(aType == VTK_QUADRATIC_TRIANGLE);
      for (int i = 3; i < _nbDownCells; i++)
        {
          if (faces[i] < 0)
            {
              faces[i] = lowCellId;
              return;
            }
          if (faces[i] == lowCellId)
            return;
        }
    }
  ASSERT(0);
}

/*! Create a list of faces described by a vtk Type and  an ordered set of Node Id's
 * The quadratic wedge (or pentahedron) is defined by fifteen points.
 * The ordering of the fifteen points defining the cell is point id's (0-5,6-14)
 * where point id's 0-5 are the six corner vertices of the wedge, followed by
 * nine mid-edge nodes (6-14). Note that these mid-edge nodes lie on the edges defined by
 * (0,1), (1,2), (2,0), (3,4), (4,5), (5,3), (0,3), (1,4), (2,5).
 * @see vtkQuadraticWedge.h in Filtering
 * @param cellId volumeId in vtkUnstructuredGrid
 * @param facesWithNodes vector of face descriptors to be filled
 */
void SMDS_DownQuadPenta::computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes)
{
  // --- find point id's of the volume

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(cellId, npts, nodes);

  // --- create all the ordered list of node id's for each face

  facesWithNodes.nbElems = 5;

  facesWithNodes.elems[0].nodeIds[0] = nodes[0];
  facesWithNodes.elems[0].nodeIds[1] = nodes[2];
  facesWithNodes.elems[0].nodeIds[2] = nodes[5];
  facesWithNodes.elems[0].nodeIds[3] = nodes[3];
  facesWithNodes.elems[0].nodeIds[4] = nodes[8];
  facesWithNodes.elems[0].nodeIds[5] = nodes[14];
  facesWithNodes.elems[0].nodeIds[6] = nodes[11];
  facesWithNodes.elems[0].nodeIds[7] = nodes[12];
  facesWithNodes.elems[0].nbNodes = 8;
  facesWithNodes.elems[0].vtkType = VTK_QUADRATIC_QUAD;

  facesWithNodes.elems[1].nodeIds[0] = nodes[1];
  facesWithNodes.elems[1].nodeIds[1] = nodes[2];
  facesWithNodes.elems[1].nodeIds[2] = nodes[5];
  facesWithNodes.elems[1].nodeIds[3] = nodes[4];
  facesWithNodes.elems[1].nodeIds[4] = nodes[7];
  facesWithNodes.elems[1].nodeIds[5] = nodes[14];
  facesWithNodes.elems[1].nodeIds[6] = nodes[10];
  facesWithNodes.elems[1].nodeIds[7] = nodes[13];
  facesWithNodes.elems[1].nbNodes = 8;
  facesWithNodes.elems[1].vtkType = VTK_QUADRATIC_QUAD;

  facesWithNodes.elems[2].nodeIds[0] = nodes[0];
  facesWithNodes.elems[2].nodeIds[1] = nodes[1];
  facesWithNodes.elems[2].nodeIds[2] = nodes[4];
  facesWithNodes.elems[2].nodeIds[3] = nodes[3];
  facesWithNodes.elems[2].nodeIds[4] = nodes[6];
  facesWithNodes.elems[2].nodeIds[5] = nodes[13];
  facesWithNodes.elems[2].nodeIds[6] = nodes[9];
  facesWithNodes.elems[2].nodeIds[7] = nodes[12];
  facesWithNodes.elems[2].nbNodes = 8;
  facesWithNodes.elems[2].vtkType = VTK_QUADRATIC_QUAD;

  facesWithNodes.elems[3].nodeIds[0] = nodes[0];
  facesWithNodes.elems[3].nodeIds[1] = nodes[1];
  facesWithNodes.elems[3].nodeIds[2] = nodes[2];
  facesWithNodes.elems[3].nodeIds[3] = nodes[6];
  facesWithNodes.elems[3].nodeIds[4] = nodes[7];
  facesWithNodes.elems[3].nodeIds[5] = nodes[8];
  facesWithNodes.elems[3].nbNodes = 6;
  facesWithNodes.elems[3].vtkType = VTK_QUADRATIC_TRIANGLE;

  facesWithNodes.elems[4].nodeIds[0] = nodes[3];
  facesWithNodes.elems[4].nodeIds[1] = nodes[4];
  facesWithNodes.elems[4].nodeIds[2] = nodes[5];
  facesWithNodes.elems[4].nodeIds[3] = nodes[9];
  facesWithNodes.elems[4].nodeIds[4] = nodes[10];
  facesWithNodes.elems[4].nodeIds[5] = nodes[11];
  facesWithNodes.elems[4].nbNodes = 6;
  facesWithNodes.elems[4].vtkType = VTK_QUADRATIC_TRIANGLE;
}

// ---------------------------------------------------------------------------

SMDS_DownHexa::SMDS_DownHexa(SMDS_UnstructuredGrid *grid) :
  SMDS_Down3D(grid, 6)
{
  _cellTypes.push_back(VTK_QUAD);
  _cellTypes.push_back(VTK_QUAD);
  _cellTypes.push_back(VTK_QUAD);
  _cellTypes.push_back(VTK_QUAD);
  _cellTypes.push_back(VTK_QUAD);
  _cellTypes.push_back(VTK_QUAD);
}

SMDS_DownHexa::~SMDS_DownHexa()
{
}

void SMDS_DownHexa::getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes)
{
  set<int> setNodes;
  setNodes.clear();
  for (int i = 0; i < orderedNodes.size(); i++)
    setNodes.insert(orderedNodes[i]);
  //MESSAGE("cellId = " << cellId);

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(this->_vtkCellIds[cellId], npts, nodes);

  set<int> tofind;
//int ids[24] = { 0, 1, 2, 3,  7, 6, 5, 4,  4, 0, 3, 7,  5, 1, 0, 4,  6, 2, 1, 5,  7, 3, 2, 6};
  int ids[24] = { 3, 2, 1, 0,  4, 5, 6, 7,  7, 3, 0, 4,  4, 0, 1, 5,  5, 1, 2, 6,  6, 2, 3, 7};
  for (int k = 0; k < 6; k++) // loop on the 6 faces
    {
      tofind.clear();
      for (int i = 0; i < 4; i++)
        tofind.insert(nodes[ids[4 * k + i]]); // node ids of the face i
      if (setNodes == tofind)
        {
          for (int i = 0; i < 4; i++)
            orderedNodes[i] = nodes[ids[4 * k + i]];
          return;
        }
    }
  MESSAGE("=== Problem volume " << _vtkCellIds[cellId] << " " << _grid->_mesh->fromVtkToSmds(_vtkCellIds[cellId]));
  MESSAGE(orderedNodes[0] << " " << orderedNodes[1] << " " << orderedNodes[2] << " " << orderedNodes[3]);
  MESSAGE(nodes[0] << " " << nodes[1] << " " << nodes[2] << " " << nodes[3]);
  MESSAGE(nodes[4] << " " << nodes[5] << " " << nodes[6] << " " << nodes[7]);
}

void SMDS_DownHexa::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0)&& (cellId < _maxId));
  int *faces = &_cellIds[_nbDownCells * cellId];
  for (int i = 0; i < _nbDownCells; i++)
    {
      if (faces[i] < 0)
        {
          faces[i] = lowCellId;
          return;
        }
      if (faces[i] == lowCellId)
        return;
    }
  ASSERT(0);
  // MESSAGE("-------------------------------------> trop de faces ! " << cellId << " " << lowCellId);
}

/*! Create a list of faces described by a vtk Type and  an ordered set of Node Id's
 * The hexahedron is defined by the eight points (0-7), where (0,1,2,3) is the base
 * of the hexahedron which, using the right hand rule, forms a quadrilateral whose normal
 * points in the direction of the opposite face (4,5,6,7).
 * @see vtkHexahedron.h in Filtering
 * @param cellId volumeId in vtkUnstructuredGrid
 * @param facesWithNodes vector of face descriptors to be filled
 */
void SMDS_DownHexa::computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes)
{
  // --- find point id's of the volume

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(cellId, npts, nodes);

  // --- create all the ordered list of node id's for each face

  facesWithNodes.nbElems = 6;

  facesWithNodes.elems[0].nodeIds[0] = nodes[0];
  facesWithNodes.elems[0].nodeIds[1] = nodes[1];
  facesWithNodes.elems[0].nodeIds[2] = nodes[2];
  facesWithNodes.elems[0].nodeIds[3] = nodes[3];
  facesWithNodes.elems[0].nbNodes = 4;
  facesWithNodes.elems[0].vtkType = VTK_QUAD;

  facesWithNodes.elems[1].nodeIds[0] = nodes[4];
  facesWithNodes.elems[1].nodeIds[1] = nodes[5];
  facesWithNodes.elems[1].nodeIds[2] = nodes[6];
  facesWithNodes.elems[1].nodeIds[3] = nodes[7];
  facesWithNodes.elems[1].nbNodes = 4;
  facesWithNodes.elems[1].vtkType = VTK_QUAD;

  facesWithNodes.elems[2].nodeIds[0] = nodes[0];
  facesWithNodes.elems[2].nodeIds[1] = nodes[1];
  facesWithNodes.elems[2].nodeIds[2] = nodes[5];
  facesWithNodes.elems[2].nodeIds[3] = nodes[4];
  facesWithNodes.elems[2].nbNodes = 4;
  facesWithNodes.elems[2].vtkType = VTK_QUAD;

  facesWithNodes.elems[3].nodeIds[0] = nodes[1];
  facesWithNodes.elems[3].nodeIds[1] = nodes[2];
  facesWithNodes.elems[3].nodeIds[2] = nodes[6];
  facesWithNodes.elems[3].nodeIds[3] = nodes[5];
  facesWithNodes.elems[3].nbNodes = 4;
  facesWithNodes.elems[3].vtkType = VTK_QUAD;

  facesWithNodes.elems[4].nodeIds[0] = nodes[2];
  facesWithNodes.elems[4].nodeIds[1] = nodes[6];
  facesWithNodes.elems[4].nodeIds[2] = nodes[7];
  facesWithNodes.elems[4].nodeIds[3] = nodes[3];
  facesWithNodes.elems[4].nbNodes = 4;
  facesWithNodes.elems[4].vtkType = VTK_QUAD;

  facesWithNodes.elems[5].nodeIds[0] = nodes[3];
  facesWithNodes.elems[5].nodeIds[1] = nodes[7];
  facesWithNodes.elems[5].nodeIds[2] = nodes[4];
  facesWithNodes.elems[5].nodeIds[3] = nodes[0];
  facesWithNodes.elems[5].nbNodes = 4;
  facesWithNodes.elems[5].vtkType = VTK_QUAD;
}

// ---------------------------------------------------------------------------

SMDS_DownQuadHexa::SMDS_DownQuadHexa(SMDS_UnstructuredGrid *grid) :
  SMDS_Down3D(grid, 6)
{
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
  _cellTypes.push_back(VTK_QUADRATIC_QUAD);
}

SMDS_DownQuadHexa::~SMDS_DownQuadHexa()
{
}

void SMDS_DownQuadHexa::getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes)
{
  set<int> setNodes;
  setNodes.clear();
  for (int i = 0; i < orderedNodes.size(); i++)
    setNodes.insert(orderedNodes[i]);
  //MESSAGE("cellId = " << cellId);

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(this->_vtkCellIds[cellId], npts, nodes);

  set<int> tofind;
  //int ids[24] = { 3, 2, 1, 0,  4, 5, 6, 7,  7, 3, 0, 4,  4, 0, 1, 5,  5, 1, 2, 6,  6, 2, 3, 7};
  int ids[48] = { 3, 2, 1, 0,10, 9, 8,11,   4, 5, 6, 7,12,13,14,15,   7, 3, 0, 4,19,11,16,15,
                  4, 0, 1, 5,16, 8,17,12,   5, 1, 2, 6,17, 9,18,13,   6, 2, 3, 7,18,10,19,14};
  for (int k = 0; k < 6; k++)
    {
      tofind.clear();
      for (int i = 0; i < 8; i++)
        tofind.insert(nodes[ids[8 * k + i]]);
      if (setNodes == tofind)
        {
          for (int i = 0; i < 8; i++)
            orderedNodes[i] = nodes[ids[8 * k + i]];
          return;
        }
    }
  MESSAGE("=== Problem volume " << _vtkCellIds[cellId] << " " << _grid->_mesh->fromVtkToSmds(_vtkCellIds[cellId]));
  MESSAGE(orderedNodes[0] << " " << orderedNodes[1] << " " << orderedNodes[2] << " " << orderedNodes[3]);
  MESSAGE(nodes[0] << " " << nodes[1] << " " << nodes[2] << " " << nodes[3]);
}

void SMDS_DownQuadHexa::addDownCell(int cellId, int lowCellId, unsigned char aType)
{
  //ASSERT((cellId >=0)&& (cellId < _maxId));
  int *faces = &_cellIds[_nbDownCells * cellId];
  for (int i = 0; i < _nbDownCells; i++)
    {
      if (faces[i] < 0)
        {
          faces[i] = lowCellId;
          return;
        }
      if (faces[i] == lowCellId)
        return;
    }
  ASSERT(0);
}

/*! Create a list of faces described by a vtk Type and  an ordered set of Node Id's
 * The ordering of the twenty points defining the quadratic hexahedron cell is point id's (0-7,8-19)
 * where point id's 0-7 are the eight corner vertices of the cube, followed by twelve mid-edge nodes (8-19).
 * Note that these mid-edge nodes lie on the edges defined by
 * (0,1), (1,2), (2,3), (3,0), (4,5), (5,6), (6,7), (7,4), (0,4), (1,5), (2,6), (3,7).
 * @see vtkQuadraticHexahedron.h in Filtering
 * @param cellId volumeId in vtkUnstructuredGrid
 * @param facesWithNodes vector of face descriptors to be filled
 */
void SMDS_DownQuadHexa::computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes)
{
  // --- find point id's of the volume

  vtkIdType npts = 0;
  vtkIdTypePtr nodes; // will refer to the point id's of the volume
  _grid->GetCellPoints(cellId, npts, nodes);

  // --- create all the ordered list of node id's for each face

  facesWithNodes.nbElems = 6;

  facesWithNodes.elems[0].nodeIds[0] = nodes[0];
  facesWithNodes.elems[0].nodeIds[1] = nodes[1];
  facesWithNodes.elems[0].nodeIds[2] = nodes[2];
  facesWithNodes.elems[0].nodeIds[3] = nodes[3];
  facesWithNodes.elems[0].nodeIds[4] = nodes[8];
  facesWithNodes.elems[0].nodeIds[5] = nodes[9];
  facesWithNodes.elems[0].nodeIds[6] = nodes[10];
  facesWithNodes.elems[0].nodeIds[7] = nodes[11];
  facesWithNodes.elems[0].nbNodes = 8;
  facesWithNodes.elems[0].vtkType = VTK_QUADRATIC_QUAD;

  facesWithNodes.elems[1].nodeIds[0] = nodes[4];
  facesWithNodes.elems[1].nodeIds[1] = nodes[5];
  facesWithNodes.elems[1].nodeIds[2] = nodes[6];
  facesWithNodes.elems[1].nodeIds[3] = nodes[7];
  facesWithNodes.elems[1].nodeIds[4] = nodes[12];
  facesWithNodes.elems[1].nodeIds[5] = nodes[13];
  facesWithNodes.elems[1].nodeIds[6] = nodes[14];
  facesWithNodes.elems[1].nodeIds[7] = nodes[15];
  facesWithNodes.elems[1].nbNodes = 8;
  facesWithNodes.elems[1].vtkType = VTK_QUADRATIC_QUAD;

  facesWithNodes.elems[2].nodeIds[0] = nodes[0];
  facesWithNodes.elems[2].nodeIds[1] = nodes[1];
  facesWithNodes.elems[2].nodeIds[2] = nodes[5];
  facesWithNodes.elems[2].nodeIds[3] = nodes[4];
  facesWithNodes.elems[2].nodeIds[4] = nodes[8];
  facesWithNodes.elems[2].nodeIds[5] = nodes[17];
  facesWithNodes.elems[2].nodeIds[6] = nodes[12];
  facesWithNodes.elems[2].nodeIds[7] = nodes[16];
  facesWithNodes.elems[2].nbNodes = 8;
  facesWithNodes.elems[2].vtkType = VTK_QUADRATIC_QUAD;

  facesWithNodes.elems[3].nodeIds[0] = nodes[1];
  facesWithNodes.elems[3].nodeIds[1] = nodes[2];
  facesWithNodes.elems[3].nodeIds[2] = nodes[6];
  facesWithNodes.elems[3].nodeIds[3] = nodes[5];
  facesWithNodes.elems[3].nodeIds[4] = nodes[9];
  facesWithNodes.elems[3].nodeIds[5] = nodes[18];
  facesWithNodes.elems[3].nodeIds[6] = nodes[13];
  facesWithNodes.elems[3].nodeIds[7] = nodes[17];
  facesWithNodes.elems[3].nbNodes = 8;
  facesWithNodes.elems[3].vtkType = VTK_QUADRATIC_QUAD;

  facesWithNodes.elems[4].nodeIds[0] = nodes[2];
  facesWithNodes.elems[4].nodeIds[1] = nodes[6];
  facesWithNodes.elems[4].nodeIds[2] = nodes[7];
  facesWithNodes.elems[4].nodeIds[3] = nodes[3];
  facesWithNodes.elems[4].nodeIds[4] = nodes[18];
  facesWithNodes.elems[4].nodeIds[5] = nodes[14];
  facesWithNodes.elems[4].nodeIds[6] = nodes[19];
  facesWithNodes.elems[4].nodeIds[7] = nodes[10];
  facesWithNodes.elems[4].nbNodes = 8;
  facesWithNodes.elems[4].vtkType = VTK_QUADRATIC_QUAD;

  facesWithNodes.elems[5].nodeIds[0] = nodes[3];
  facesWithNodes.elems[5].nodeIds[1] = nodes[7];
  facesWithNodes.elems[5].nodeIds[2] = nodes[4];
  facesWithNodes.elems[5].nodeIds[3] = nodes[0];
  facesWithNodes.elems[5].nodeIds[4] = nodes[19];
  facesWithNodes.elems[5].nodeIds[5] = nodes[15];
  facesWithNodes.elems[5].nodeIds[6] = nodes[16];
  facesWithNodes.elems[5].nodeIds[7] = nodes[11];
  facesWithNodes.elems[5].nbNodes = 8;
  facesWithNodes.elems[5].vtkType = VTK_QUADRATIC_QUAD;
}

// ---------------------------------------------------------------------------

