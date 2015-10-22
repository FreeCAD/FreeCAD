//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//  SMESH SMESH : implementaion of SMESH idl descriptions
//  File   : SMESH_Mesh.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//  $Header: /home/server/cvs/SMESH/SMESH_SRC/src/SMESH/SMESH_Mesh.hxx,v 1.18.2.3 2008/11/27 12:25:15 abd Exp $
//
#ifndef _SMESH_MESH_HXX_
#define _SMESH_MESH_HXX_

#include "SMESH_SMESH.hxx"

#include "SMESH_Hypothesis.hxx"

#include "SMESHDS_Mesh.hxx"
#include "SMESHDS_Command.hxx"
#include "SMDSAbs_ElementType.hxx"

#include "SMESH_Exception.hxx"

#include <TopoDS_Shape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>

#include <list>
#include <map>

class SMESH_Gen;
class SMESHDS_Document;
class SMESH_Group;
class TopTools_ListOfShape;
class SMESH_subMesh;
class SMESH_HypoFilter;
class TopoDS_Solid;

class SMESH_EXPORT SMESH_Mesh
{
public:
  SMESH_Mesh(int               theLocalId, 
	     int               theStudyId, 
	     SMESH_Gen*        theGen,
	     bool              theIsEmbeddedMode,
	     SMESHDS_Document* theDocument);
  
  virtual ~SMESH_Mesh();
  
  /*!
   * \brief Set geometry to be meshed
   */
  void ShapeToMesh(const TopoDS_Shape & aShape);
  /*!
   * \brief Return geometry to be meshed. (It may be a PseudoShape()!)
   */
  TopoDS_Shape GetShapeToMesh() const;
  /*!
   * \brief Return true if there is a geometry to be meshed, not PseudoShape()
   */
  bool HasShapeToMesh() const { return _isShapeToMesh; }
   /*!
   * \brief Return diagonal size of bounding box of shape to mesh.
   */
  double GetShapeDiagonalSize() const;
  /*!
   * \brief Return diagonal size of bounding box of a shape.
   */
  static double GetShapeDiagonalSize(const TopoDS_Shape & aShape);
  /*!
   * \brief Return a solid which is returned by GetShapeToMesh() if
   *        a real geometry to be meshed was not set
   */
  static const TopoDS_Solid& PseudoShape();

  /*!
   * \brief Remove all nodes and elements
   */
  void Clear();

   /*!
   * \brief Remove all nodes and elements of indicated shape
   */
  void ClearSubMesh(const int theShapeId);

  int UNVToMesh(const char* theFileName);
  /*!
   * consult DriverMED_R_SMESHDS_Mesh::ReadStatus for returned value
   */
  int MEDToMesh(const char* theFileName, const char* theMeshName);
  
  int STLToMesh(const char* theFileName);

  int DATToMesh(const char* theFileName);

  SMESH_Hypothesis::Hypothesis_Status
  AddHypothesis(const TopoDS_Shape & aSubShape, int anHypId)
    throw(SMESH_Exception);
  
  SMESH_Hypothesis::Hypothesis_Status
  RemoveHypothesis(const TopoDS_Shape & aSubShape, int anHypId)
    throw(SMESH_Exception);
  
  const std::list <const SMESHDS_Hypothesis * >&
  GetHypothesisList(const TopoDS_Shape & aSubShape) const
    throw(SMESH_Exception);

  const SMESH_Hypothesis * GetHypothesis(const TopoDS_Shape &    aSubShape,
                                         const SMESH_HypoFilter& aFilter,
                                         const bool              andAncestors,
                                         TopoDS_Shape*           assignedTo=0) const;
  
  int GetHypotheses(const TopoDS_Shape &                     aSubShape,
                    const SMESH_HypoFilter&                  aFilter,
                    std::list <const SMESHDS_Hypothesis * >& aHypList,
                    const bool                               andAncestors) const;

  const std::list<SMESHDS_Command*> & GetLog() throw(SMESH_Exception);
  
  void ClearLog() throw(SMESH_Exception);
  
  int GetId()                { return _id; }
  
  SMESHDS_Mesh * GetMeshDS() { return _myMeshDS; }
  
  SMESH_Gen *GetGen()        { return _gen; }
  
  SMESH_subMesh *GetSubMesh(const TopoDS_Shape & aSubShape)
    throw(SMESH_Exception);
  
  SMESH_subMesh *GetSubMeshContaining(const TopoDS_Shape & aSubShape) const
    throw(SMESH_Exception);
  
  SMESH_subMesh *GetSubMeshContaining(const int aShapeID) const
    throw(SMESH_Exception);
  /*!
   * \brief Return submeshes of groups containing the given subshape
   */
  std::list<SMESH_subMesh*> GetGroupSubMeshesContaining(const TopoDS_Shape & shape) const
    throw(SMESH_Exception);
  /*!
   * \brief Say all submeshes that theChangedHyp has been modified
   */
  void NotifySubMeshesHypothesisModification(const SMESH_Hypothesis* theChangedHyp);

  const std::list < SMESH_subMesh * >&
  GetSubMeshUsingHypothesis(SMESHDS_Hypothesis * anHyp) throw(SMESH_Exception);
  /*!
   * \brief Return True if anHyp is used to mesh aSubShape
   */
  bool IsUsedHypothesis(SMESHDS_Hypothesis *  anHyp,
			const SMESH_subMesh * aSubMesh);
  /*!
   * \brief check if a hypothesis alowing notconform mesh is present
   */
  bool IsNotConformAllowed() const;
  
  bool IsMainShape(const TopoDS_Shape& theShape) const;
  /*!
   * \brief Return list of ancestors of theSubShape in the order
   *        that lower dimention shapes come first
   */
  const TopTools_ListOfShape& GetAncestors(const TopoDS_Shape& theSubShape) const;

  void SetAutoColor(bool theAutoColor) throw(SMESH_Exception);

  bool GetAutoColor() throw(SMESH_Exception);

  /*!
   * \brief Return data map of descendant to ancestor shapes
   */
  typedef TopTools_IndexedDataMapOfShapeListOfShape TAncestorMap;
  const TAncestorMap& GetAncestorMap() const { return _mapAncestors; }
  /*!
   * \brief Check group names for duplications.
   *  Consider maximum group name length stored in MED file
   */
  bool HasDuplicatedGroupNamesMED();

  void ExportMED(const char *file, 
		 const char* theMeshName = NULL, 
		 bool theAutoGroups = true, 
		 int theVersion = 0) 
    throw(SMESH_Exception);

  void ExportDAT(const char *file) throw(SMESH_Exception);
  void ExportUNV(const char *file) throw(SMESH_Exception);
  void ExportSTL(const char *file, const bool isascii) throw(SMESH_Exception);
  
  int NbNodes() throw(SMESH_Exception);
  
  int NbEdges(SMDSAbs_ElementOrder order = ORDER_ANY) throw(SMESH_Exception);
  
  int NbFaces(SMDSAbs_ElementOrder order = ORDER_ANY) throw(SMESH_Exception);
  
  int NbTriangles(SMDSAbs_ElementOrder order = ORDER_ANY) throw(SMESH_Exception);
  
  int NbQuadrangles(SMDSAbs_ElementOrder order = ORDER_ANY) throw(SMESH_Exception);

  int NbPolygons() throw(SMESH_Exception);
  
  int NbVolumes(SMDSAbs_ElementOrder order = ORDER_ANY) throw(SMESH_Exception);
  
  int NbTetras(SMDSAbs_ElementOrder order = ORDER_ANY) throw(SMESH_Exception);
  
  int NbHexas(SMDSAbs_ElementOrder order = ORDER_ANY) throw(SMESH_Exception);
  
  int NbPyramids(SMDSAbs_ElementOrder order = ORDER_ANY) throw(SMESH_Exception);

  int NbPrisms(SMDSAbs_ElementOrder order = ORDER_ANY) throw(SMESH_Exception);
  
  int NbPolyhedrons() throw(SMESH_Exception);
  
  int NbSubMesh() throw(SMESH_Exception);
  
  int NbGroup() const { return _mapGroup.size(); }
  
  SMESH_Group* AddGroup (const SMDSAbs_ElementType theType,
			 const char*               theName,
			 int&                      theId,
                         const TopoDS_Shape&       theShape=TopoDS_Shape());
  
  typedef boost::shared_ptr< SMDS_Iterator<SMESH_Group*> > GroupIteratorPtr;
  GroupIteratorPtr GetGroups() const;
  
  std::list<int> GetGroupIds() const;
  
  SMESH_Group* GetGroup (const int theGroupID);

  void RemoveGroup (const int theGroupID);

  SMESH_Group* ConvertToStandalone ( int theGroupID );

  SMDSAbs_ElementType GetElementType( const int id, const bool iselem );

  //
  
  ostream& Dump(ostream & save);
  
private:
  
protected:
  int                        _id;           // id given by creator (unique within the creator instance)
  int                        _studyId;
  int                        _idDoc;        // id given by SMESHDS_Document
  int                        _groupId;      // id generator for group objects
  int                        _nbSubShapes;  // initial nb of subshapes in the shape to mesh
  bool                       _isShapeToMesh;// set to true when a shape is given (only once)
  std::list <SMESH_subMesh*> _subMeshesUsingHypothesisList;
  SMESHDS_Document *         _myDocument;
  SMESHDS_Mesh *             _myMeshDS;
  std::map <int, SMESH_subMesh*> _mapSubMesh;
  std::map <int, SMESH_Group*>   _mapGroup;
  SMESH_Gen *                _gen;

  bool                       _isAutoColor;

  double                     _shapeDiagonal; //!< diagonal size of bounding box of shape to mesh

  TopTools_IndexedDataMapOfShapeListOfShape _mapAncestors;

protected:
  SMESH_Mesh() {};
  SMESH_Mesh(const SMESH_Mesh&) {};
};

#endif
