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

// File: SMDS_Downward.hxx
// Created: Jun 3, 2010
// Author: prascle

#ifndef SMDS_DOWNWARD_HXX_
#define SMDS_DOWNWARD_HXX_

#include "SMDS_UnstructuredGrid.hxx"

#include <vector>
#include <set>

typedef struct
{
  int nodeIds[8]; //!< max number of nodes in a face or edge: quad quad = 8
  int nbNodes;
  unsigned char vtkType;
} ElemByNodesType; // TODO resize for polyhedrons

typedef struct
{
  ElemByNodesType elems[6]; //!< max number of faces in a volume or edges in a face : hexahedron = 6
  int nbElems;
} ListElemByNodesType; // TODO resize for polyhedrons

class SMDS_EXPORT DownIdType
{
public:
  DownIdType(int a, unsigned char b) :
    cellId(a), cellType(b)
  {
  }
  int cellId;
  unsigned char cellType;
};

struct DownIdCompare
{
  bool operator ()(const DownIdType e1, const DownIdType e2) const
  {
    if (e1.cellId == e2.cellId)
      return (e1.cellType < e2.cellType);
    else
      return (e1.cellId < e2.cellId);
  }
};

class SMDS_EXPORT SMDS_Downward
{
  friend class SMDS_UnstructuredGrid;
  friend class SMDS_Down2D;
  friend class SMDS_Down3D;
public:
  virtual int getNumberOfDownCells(int cellId);
  virtual const int* getDownCells(int cellId);
  virtual const unsigned char* getDownTypes(int cellId);
  virtual int getNumberOfUpCells(int cellId) = 0;
  virtual const int* getUpCells(int cellId) = 0;
  virtual const unsigned char* getUpTypes(int cellId) = 0;
  virtual void getNodeIds(int cellId, std::set<int>& nodeSet) = 0;
  virtual int getNodes(int cellId, int* nodevec) {return 0; }
  virtual void getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes) {};
  int getVtkCellId(int cellId)
  {
    return _vtkCellIds[cellId];
  }
  int getMaxId()
  {
    return _maxId;
  }
  static int getCellDimension(unsigned char cellType);
protected:
  SMDS_Downward(SMDS_UnstructuredGrid *grid, int nbDownCells);
  virtual ~SMDS_Downward();
  int addCell(int vtkId = -1);
  virtual void initCell(int cellId);
  virtual void allocate(int nbElems) = 0;
  virtual void compactStorage() = 0;
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType); //!< Id's are downward connectivity id's
  virtual void addUpCell(int cellId, int upCellId, unsigned char aType); //!< Id's are downward connectivity id's
  virtual int getNodeSet(int cellId, int* nodeSet);

  SMDS_UnstructuredGrid* _grid;
  int _maxId;
  int _nbDownCells; //!< the same number for all cells of a derived class
  std::vector<int> _cellIds; //!< growing size: all the down cell id's, size = _maxId * _nbDownCells
  std::vector<int> _vtkCellIds; //!< growing size: size = _maxId, either vtkId or -1
  std::vector<unsigned char> _cellTypes; //!< fixed size: the same vector for all cells of a derived class

  static std::vector<int> _cellDimension; //!< conversion table: type --> dimension
};

class SMDS_EXPORT SMDS_Down1D: public SMDS_Downward
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual int getNumberOfUpCells(int cellId);
  virtual const int* getUpCells(int cellId);
  virtual const unsigned char* getUpTypes(int cellId);
  virtual void getNodeIds(int cellId, std::set<int>& nodeSet);
  virtual int getNodes(int cellId, int* nodevec) { return getNodeSet(cellId, nodevec); }
protected:
  SMDS_Down1D(SMDS_UnstructuredGrid *grid, int nbDownCells);
  ~SMDS_Down1D();
  virtual void initCell(int cellId);
  virtual void allocate(int nbElems);
  virtual void compactStorage();
  virtual void addUpCell(int cellId, int upCellId, unsigned char aType); //!< Id's are downward connectivity id's
  virtual int getNodeSet(int cellId, int* nodeSet);
  void setNodes(int cellId, int vtkId);
  void setNodes(int cellId, const int* nodeIds);
  int computeVtkCells(int cellId, std::vector<int>& vtkIds);
  int computeVtkCells(int* pts, std::vector<int>& vtkIds);
  int computeFaces(int cellId, int* vtkIds, int nbcells, int* downFaces, unsigned char* downTypes);
  int computeFaces(int* pts, int* vtkIds, int nbcells, int* downFaces, unsigned char* downTypes);

  std::vector<std::vector<int> > _upCellIdsVector; //!< the number of faces sharing an edge is not known
  std::vector<std::vector<unsigned char> > _upCellTypesVector; //!< the number of faces sharing an edge is not known
  std::vector<int> _upCellIds; //!< compacted storage after connectivity calculation
  std::vector<unsigned char> _upCellTypes; //!< compacted storage after connectivity calculation
  std::vector<int> _upCellIndex; //!< compacted storage after connectivity calculation
};

class SMDS_EXPORT SMDS_Down2D: public SMDS_Downward
{
  friend class SMDS_UnstructuredGrid;
  friend class SMDS_Down1D;
public:
  virtual int getNumberOfUpCells(int cellId);
  virtual const int* getUpCells(int cellId);
  virtual const unsigned char* getUpTypes(int cellId);
  virtual void getNodeIds(int cellId, std::set<int>& nodeSet);
protected:
  SMDS_Down2D(SMDS_UnstructuredGrid *grid, int nbDownCells);
  ~SMDS_Down2D();
  virtual void allocate(int nbElems);
  virtual void compactStorage();
  virtual void addUpCell(int cellId, int upCellId, unsigned char aType);
  virtual void computeEdgesWithNodes(int cellId, ListElemByNodesType& facesWithNodes) = 0;
  virtual int getNodeSet(int cellId, int* nodeSet);
  int computeVolumeIds(int cellId, int* ids);
  int computeVolumeIds(ElemByNodesType& faceByNodes, int* ids);
  int computeVolumeIdsFromNodesFace(int* nodes, int nbNodes, int* ids);
  void setTempNodes(int cellId, int vtkId);
  void setTempNodes(int cellId, ElemByNodesType& faceByNodes);
  bool isInFace(int cellId, int *pts, int npts);
  int FindEdgeByNodes(int cellId, ElemByNodesType& edgeByNodes);

  std::vector<int> _upCellIds; //!< 2 volumes max. per face
  std::vector<unsigned char> _upCellTypes; //!< 2 volume types per face
  std::vector<int> _tempNodes; //!< temporary storage of nodes, until downward connectivity completion
  int _nbNodes; //!< number of nodes in a face
};

class SMDS_EXPORT SMDS_Down3D: public SMDS_Downward
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual int getNumberOfUpCells(int cellId);
  virtual const int* getUpCells(int cellId);
  virtual const unsigned char* getUpTypes(int cellId);
  virtual void getNodeIds(int cellId, std::set<int>& nodeSet);
protected:
  SMDS_Down3D(SMDS_UnstructuredGrid *grid, int nbDownCells);
  ~SMDS_Down3D();
  virtual void allocate(int nbElems);
  virtual void compactStorage();
  virtual void computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes) = 0;
  int FindFaceByNodes(int cellId, ElemByNodesType& faceByNodes);
};

class SMDS_EXPORT SMDS_DownEdge: public SMDS_Down1D
{
  friend class SMDS_UnstructuredGrid;
public:
protected:
  SMDS_DownEdge(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownEdge();
};

class SMDS_EXPORT SMDS_DownQuadEdge: public SMDS_Down1D
{
  friend class SMDS_UnstructuredGrid;
public:
protected:
  SMDS_DownQuadEdge(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownQuadEdge();
};

class SMDS_EXPORT SMDS_DownTriangle: public SMDS_Down2D
{
  friend class SMDS_UnstructuredGrid;
public:
protected:
  SMDS_DownTriangle(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownTriangle();
  virtual void computeEdgesWithNodes(int cellId, ListElemByNodesType& edgesWithNodes);
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType); //!< Id's are downward connectivity id's
};

class SMDS_EXPORT SMDS_DownQuadTriangle: public SMDS_Down2D
{
  friend class SMDS_UnstructuredGrid;
public:
protected:
  SMDS_DownQuadTriangle(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownQuadTriangle();
  virtual void computeEdgesWithNodes(int cellId, ListElemByNodesType& edgesWithNodes);
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType); //!< Id's are downward connectivity id's
};

class SMDS_EXPORT SMDS_DownQuadrangle: public SMDS_Down2D
{
  friend class SMDS_UnstructuredGrid;
public:
protected:
  SMDS_DownQuadrangle(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownQuadrangle();
  virtual void computeEdgesWithNodes(int cellId, ListElemByNodesType& edgesWithNodes);
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType); //!< Id's are downward connectivity id's
};

class SMDS_EXPORT SMDS_DownQuadQuadrangle: public SMDS_Down2D
{
  friend class SMDS_UnstructuredGrid;
public:
protected:
  SMDS_DownQuadQuadrangle(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownQuadQuadrangle();
  virtual void computeEdgesWithNodes(int cellId, ListElemByNodesType& edgesWithNodes);
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType); //!< Id's are downward connectivity id's
};

//class SMDS_DownPolygon: public SMDS_Down2D
//{
//public:
//  SMDS_DownPolygon(SMDS_UnstructuredGrid *grid);
//  ~SMDS_DownPolygon();
//protected:
//};

//class SMDS_DownQuadPolygon: public SMDS_Down2D
//{
//public:
//  SMDS_DownQuadPolygon(SMDS_UnstructuredGrid *grid);
//  ~SMDS_DownQuadPolygon();
//protected:
//};

class SMDS_EXPORT SMDS_DownTetra: public SMDS_Down3D
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual void getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes);
protected:
  SMDS_DownTetra(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownTetra();
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType);
  virtual void computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes);
};

class SMDS_EXPORT SMDS_DownQuadTetra: public SMDS_Down3D
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual void getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes);
protected:
  SMDS_DownQuadTetra(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownQuadTetra();
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType);
  virtual void computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes);
};

class SMDS_EXPORT SMDS_DownPyramid: public SMDS_Down3D
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual void getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes);
protected:
  SMDS_DownPyramid(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownPyramid();
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType);
  virtual void computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes);
};

class SMDS_EXPORT SMDS_DownQuadPyramid: public SMDS_Down3D
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual void getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes);
protected:
  SMDS_DownQuadPyramid(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownQuadPyramid();
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType);
  virtual void computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes);
};

class SMDS_EXPORT SMDS_DownPenta: public SMDS_Down3D
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual void getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes);
protected:
  SMDS_DownPenta(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownPenta();
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType);
  virtual void computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes);
};

class SMDS_EXPORT SMDS_DownQuadPenta: public SMDS_Down3D
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual void getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes);
protected:
  SMDS_DownQuadPenta(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownQuadPenta();
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType);
  virtual void computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes);
};

class SMDS_EXPORT SMDS_DownHexa: public SMDS_Down3D
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual void getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes);
protected:
  SMDS_DownHexa(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownHexa();
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType);
  virtual void computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes);
};

class SMDS_EXPORT SMDS_DownQuadHexa: public SMDS_Down3D
{
  friend class SMDS_UnstructuredGrid;
public:
  virtual void getOrderedNodesOfFace(int cellId, std::vector<vtkIdType>& orderedNodes);
protected:
  SMDS_DownQuadHexa(SMDS_UnstructuredGrid *grid);
  ~SMDS_DownQuadHexa();
  virtual void addDownCell(int cellId, int lowCellId, unsigned char aType);
  virtual void computeFacesWithNodes(int cellId, ListElemByNodesType& facesWithNodes);
};

//class SMDS_DownPolyhedra: public SMDS_Down3D
//{
//public:
//  SMDS_DownPolyhedra(SMDS_UnstructuredGrid *grid);
//  ~SMDS_DownPolyhedra();
//protected:
//};

//class SMDS_DownQuadPolyhedra: public SMDS_Down3D
//{
//public:
//  SMDS_DownQuadPolyhedra(SMDS_UnstructuredGrid *grid);
//  ~SMDS_DownQuadPolyhedra();
//protected:
//};

#endif /* SMDS_DOWNWARD_HXX_ */
