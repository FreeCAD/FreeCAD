// Copyright (C) 2007-2016  CEA/DEN, EDF R&D, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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

//  SMESH DriverMED : tool to split groups on families
//  File   : DriverMED_Family.hxx
//  Author : Julia DOROVSKIKH
//  Module : SMESH
//
#ifndef _INCLUDE_DRIVERMED_FAMILY
#define _INCLUDE_DRIVERMED_FAMILY

#include "SMESH_DriverMED.hxx"
#include "DriverMED.hxx"

#include "SMDS_Mesh.hxx"
#include "SMESHDS_GroupBase.hxx"
#include "SMESHDS_SubMesh.hxx"
#include "MED_Common.hxx"

#include <boost/shared_ptr.hpp>
#include <set>

#define REST_NODES_FAMILY 1
#define FIRST_NODE_FAMILY 2

#define REST_EDGES_FAMILY   -1
#define REST_FACES_FAMILY   -2
#define REST_VOLUMES_FAMILY -3
#define REST_0DELEM_FAMILY  -4
#define REST_BALL_FAMILY    -5
#define FIRST_ELEM_FAMILY   -6

typedef std::list<DriverMED_FamilyPtr               > DriverMED_FamilyPtrList;
typedef std::map<int,SMESHDS_SubMesh*               > SMESHDS_SubMeshPtrMap;
typedef std::list<SMESHDS_GroupBase*                > SMESHDS_GroupBasePtrList;
typedef std::set<const SMDS_MeshElement*,TIDCompare > ElementsSet;

class MESHDRIVERMED_EXPORT DriverMED_Family
{
 public:

  DriverMED_Family();

  //! Methods for groups storing to MED
  /*!
    Split each group from list <theGroups> and each sub-mesh from list <theSubMeshes>
    on some parts (families) on the basis of the elements membership in other groups
    from <theGroups> and other sub-meshes from <theSubMeshes>.
    Resulting families have no common elements.
  */
  static 
  DriverMED_FamilyPtrList
  MakeFamilies (SMESHDS_SubMeshIteratorPtr      theSubMeshes,
                const SMESHDS_GroupBasePtrList& theGroups,
                const bool doGroupOfNodes,
                const bool doGroupOfEdges,
                const bool doGroupOfFaces,
                const bool doGroupOfVolumes,
                const bool doGroupOf0DElems,
                const bool doGroupOfBalls);

  //! Create TFamilyInfo for this family
  MED::PFamilyInfo 
  GetFamilyInfo (const MED::PWrapper& theWrapper, 
                 const MED::PMeshInfo& theMeshInfo) const;

  //! Returns elements of this family
  const ElementsSet& GetElements () const;

  //! Returns a family ID
  int GetId () const;

  //! Sets a family ID
  void SetId (const int theId);

 public:

  // Methods for groups reading from MED

  void AddElement(const SMDS_MeshElement* theElement);

  const MED::TStringSet& GetGroupNames() const;
  void AddGroupName(std::string theGroupName);

  void SetType(const SMDSAbs_ElementType theType);
  SMDSAbs_ElementType GetType();
  const std::set< SMDSAbs_ElementType >& GetTypes() const;

  bool MemberOf(std::string theGroupName) const;

  int GetGroupAttributVal() const;
  void SetGroupAttributVal( int theValue);

 private:
  //! Initialize the tool by SMESHDS_GroupBase
  void Init (SMESHDS_GroupBase* group);

  //! Split <theSubMesh> on some parts (families) on the basis of the elements type.
  static
  DriverMED_FamilyPtrList 
  SplitByType(SMESHDS_SubMesh* theSubMesh,
              const int        theId);


  /*! Remove from <Elements> elements, common with <by>,
    Remove from <by> elements, common with <Elements>,
    Create family <common> from common elements, with combined groups list.
  */
  void Split (DriverMED_FamilyPtr by,
              DriverMED_FamilyPtr common);

  //! Check, if this family has empty list of elements
  bool IsEmpty () const;


 private:
  int                           myId;
  SMDSAbs_ElementType           myType;
  ElementsSet                   myElements;
  MED::TStringSet               myGroupNames;
  int                           myGroupAttributVal;
  std::set<SMDSAbs_ElementType> myTypes; // Issue 0020576
};

#endif
