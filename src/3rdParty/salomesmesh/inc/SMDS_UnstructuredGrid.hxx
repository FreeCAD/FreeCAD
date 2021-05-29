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

// File:    SMDS_UnstructuredGrid.hxx
// Author:  prascle
// Created: September 16, 2009, 10:28 PM

#ifndef _SMDS_UNSTRUCTUREDGRID_HXX
#define _SMDS_UNSTRUCTUREDGRID_HXX

#include "SMESH_SMDS.hxx"

#include <vtkUnstructuredGrid.h>
#include <vtkCellLinks.h>

#include <vector>
#include <set>
#include <map>

//#define VTK_HAVE_POLYHEDRON
//#ifdef VTK_HAVE_POLYHEDRON
#define VTK_MAXTYPE VTK_POLYHEDRON
//#else
//  #define VTK_MAXTYPE VTK_QUADRATIC_PYRAMID
//#endif

#define NBMAXNEIGHBORS 100

// allow very huge polyhedrons in tests
#define NBMAXNODESINCELL 5000

class SMDS_Downward;
class SMDS_Mesh;
class SMDS_MeshCell;
class SMDS_MeshVolume;

class SMDS_EXPORT SMDS_CellLinks: public vtkCellLinks
{
public:
  void ResizeForPoint(vtkIdType vtkID);
  static SMDS_CellLinks* New();
protected:
  SMDS_CellLinks();
  ~SMDS_CellLinks();
};

class SMDS_EXPORT SMDS_UnstructuredGrid: public vtkUnstructuredGrid
{
public:
  void setSMDS_mesh(SMDS_Mesh *mesh);
  void compactGrid(std::vector<int>& idNodesOldToNew,
                   int               newNodeSize,
                   std::vector<int>& idCellsOldToNew,
                   int               newCellSize);
  virtual VTK_MTIME_TYPE GetMTime();
  // OUV_PORTING_VTK6: seems to be useless
  //virtual void Update();
  //virtual void UpdateInformation();
  virtual vtkPoints *GetPoints();

  //#ifdef VTK_HAVE_POLYHEDRON
  int InsertNextLinkedCell(int type, int npts, vtkIdType *pts);
  //#endif

  int CellIdToDownId(int vtkCellId);
  void setCellIdToDownId(int vtkCellId, int downId);
  void CleanDownwardConnectivity();
  void BuildDownwardConnectivity(bool withEdges);
  int GetNeighbors(int* neighborsVtkIds, int* downIds, unsigned char* downTypes, int vtkId, bool getSkin=false);
  int GetParentVolumes(int* volVtkIds, int vtkId);
  int GetParentVolumes(int* volVtkIds, int downId, unsigned char downType);
  void GetNodeIds(std::set<int>& nodeSet, int downId, unsigned char downType);
  void ModifyCellNodes(int vtkVolId, std::map<int, int> localClonedNodeIds);
  int getOrderedNodesOfFace(int vtkVolId, int& dim, std::vector<vtkIdType>& orderedNodes);
  void BuildLinks();
  SMDS_MeshCell* extrudeVolumeFromFace(int vtkVolId, int domain1, int domain2,
                                       std::set<int>&                      originalNodes,
                                       std::map<int, std::map<int, int> >& nodeDomains,
                                       std::map<int, std::map<long,int> >& nodeQuadDomains);
  vtkCellLinks* GetLinks()
  {
#ifdef VTK_CELL_ARRAY_V2
    return static_cast<vtkCellLinks*>(GetCellLinks());
#else
    return Links;
#endif
  }
  SMDS_Downward* getDownArray(unsigned char vtkType)
  {
    return _downArray[vtkType];
  }
  void AllocateDiameters( vtkIdType maxVtkID );
  void SetBallDiameter( vtkIdType vtkID, double diameter );
  double GetBallDiameter( vtkIdType vtkID ) const;

  static SMDS_UnstructuredGrid* New();
  SMDS_Mesh *_mesh;

protected:
  SMDS_UnstructuredGrid();
  ~SMDS_UnstructuredGrid();
  void copyNodes(vtkPoints *newPoints, std::vector<int>& idNodesOldToNew, int& alreadyCopied, int start, int end);
  void copyBloc(vtkUnsignedCharArray *newTypes, std::vector<int>& idCellsOldToNew, std::vector<int>& idNodesOldToNew,
                vtkCellArray* newConnectivity, vtkIdTypeArray* newLocations, vtkIdType* pointsCell, int& alreadyCopied,
                int start, int end);

  std::vector<int> _cellIdToDownId; //!< convert vtk Id to downward[vtkType] id, initialized with -1
  std::vector<unsigned char> _downTypes;
  std::vector<SMDS_Downward*> _downArray;
};

#endif  /* _SMDS_UNSTRUCTUREDGRID_HXX */

