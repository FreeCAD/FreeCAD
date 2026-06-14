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
//  File   : DriverMED_Family.cxx
//  Author : Julia DOROVSKIKH
//  Module : SMESH
//
#include "DriverMED_Family.h"
#include "MED_Factory.hxx"

#include <sstream>      

using namespace std;

//=============================================================================
/*!
 *  Default constructor
 */
//=============================================================================
DriverMED_Family
::DriverMED_Family():
  myGroupAttributVal(0)
{}


//=============================================================================
const ElementsSet& 
DriverMED_Family
::GetElements () const 
{ 
  return myElements; 
}

int 
DriverMED_Family
::GetId () const 
{ 
  return myId; 
}

void 
DriverMED_Family
::SetId (const int theId) 
{ 
  myId = theId; 
}

void
DriverMED_Family
::AddElement(const SMDS_MeshElement* theElement)
{
  myElements.insert( myElements.end(), theElement );
}

void
DriverMED_Family
::AddGroupName(std::string theGroupName)
{
  myGroupNames.insert(theGroupName); 
}

void
DriverMED_Family
::SetType(const SMDSAbs_ElementType theType) 
{ 
  myTypes.insert( myType = theType );
}

SMDSAbs_ElementType
DriverMED_Family
::GetType()
{
  return myType; 
}

const std::set< SMDSAbs_ElementType >&
DriverMED_Family
::GetTypes() const
{
  return myTypes;
}

bool
DriverMED_Family
::MemberOf(std::string theGroupName) const
{ 
  return myGroupNames.find(theGroupName) != myGroupNames.end(); 
}

const MED::TStringSet& 
DriverMED_Family
::GetGroupNames () const 
{ 
  return myGroupNames; 
}


int 
DriverMED_Family
::GetGroupAttributVal() const 
{
  return myGroupAttributVal;
} 

void
DriverMED_Family
::SetGroupAttributVal( int theValue) 
{
  myGroupAttributVal = theValue;
}

bool
DriverMED_Family
::IsEmpty () const 
{ 
  return myElements.empty(); 
}

//=============================================================================
/*!
 *  Split each group from list <aGroups> on some parts (families)
 *  on the basis of the elements membership in other groups from this list.
 *  Resulting families have no common elements.
 */
//=============================================================================
DriverMED_FamilyPtrList 
DriverMED_Family
::MakeFamilies(SMESHDS_SubMeshIteratorPtr      theSubMeshes,
               const SMESHDS_GroupBasePtrList& theGroups,
               const bool doGroupOfNodes,
               const bool doGroupOfEdges,
               const bool doGroupOfFaces,
               const bool doGroupOfVolumes,
               const bool doGroupOf0DElems,
               const bool doGroupOfBalls)
{
  DriverMED_FamilyPtrList aFamilies;

  string anAllNodesGroupName   = "Group_Of_All_Nodes";
  string anAllEdgesGroupName   = "Group_Of_All_Edges";
  string anAllFacesGroupName   = "Group_Of_All_Faces";
  string anAllVolumesGroupName = "Group_Of_All_Volumes";
  string anAll0DElemsGroupName = "Group_Of_All_0DElems";
  string anAllBallsGroupName   = "Group_Of_All_Balls";

  // Reserve 6 ids for families of free elements
  // (1 - nodes, -1 - edges, -2 - faces, -3 - volumes, -4 - 0D, -5 - balls).
  // 'Free' means here not belonging to any group.
  int aNodeFamId = FIRST_NODE_FAMILY;
  int aElemFamId = FIRST_ELEM_FAMILY;

  // Process sub-meshes
  while ( theSubMeshes->more() )
  {
    SMESHDS_SubMesh* aSubMesh = const_cast< SMESHDS_SubMesh* >( theSubMeshes->next() );
    const int anId = aSubMesh->GetID();
    if ( aSubMesh->IsComplexSubmesh() )
      continue; // submesh containing other submeshs
    DriverMED_FamilyPtrList aSMFams = SplitByType(aSubMesh,anId);
    DriverMED_FamilyPtrList::iterator aSMFamsIter = aSMFams.begin();
    for (; aSMFamsIter != aSMFams.end(); aSMFamsIter++)
    {
      DriverMED_FamilyPtr aFam2 = (*aSMFamsIter);
      DriverMED_FamilyPtrList::iterator aFamsIter = aFamilies.begin();
      while (aFamsIter != aFamilies.end())
      {
        DriverMED_FamilyPtr aFam1 = *aFamsIter;
        DriverMED_FamilyPtrList::iterator aCurrIter = aFamsIter++;
        if (aFam1->myType == aFam2->myType)
        {
          DriverMED_FamilyPtr aCommon (new DriverMED_Family);
          aFam1->Split(aFam2, aCommon);
          if (!aCommon->IsEmpty())
          {
            aFamilies.push_back(aCommon);
          }
          if (aFam1->IsEmpty())
          {
            aFamilies.erase(aCurrIter);
          }
          if (aFam2->IsEmpty()) 
            break;
        }
      }
      // The rest elements of family
      if (!aFam2->IsEmpty())
      {
        aFamilies.push_back(aFam2);
      }
    }
  }

  // Process groups
  SMESHDS_GroupBasePtrList::const_iterator aGroupsIter = theGroups.begin();
  for (; aGroupsIter != theGroups.end(); aGroupsIter++)
  {
    DriverMED_FamilyPtr aFam2 (new DriverMED_Family);
    aFam2->Init(*aGroupsIter);

    DriverMED_FamilyPtrList::iterator aFamsIter = aFamilies.begin();
    while (aFamsIter != aFamilies.end())
    {
      DriverMED_FamilyPtr aFam1 = *aFamsIter;
      DriverMED_FamilyPtrList::iterator aCurrIter = aFamsIter++;
      if (aFam1->myType == aFam2->myType)
      {
        DriverMED_FamilyPtr aCommon (new DriverMED_Family);
        aFam1->Split(aFam2, aCommon);
        if (!aCommon->IsEmpty())
        {
          aCommon->SetGroupAttributVal(0);
          aFamilies.push_back(aCommon);
        }
        if (aFam1->IsEmpty())
        {
          aFamilies.erase(aCurrIter);
        }
        if (aFam2->IsEmpty()) 
          break;
      }
    }
    // The rest elements of group
    if (!aFam2->IsEmpty())
    {
      aFamilies.push_back(aFam2);
    }
  }

  DriverMED_FamilyPtrList::iterator aFamsIter = aFamilies.begin();
  for (; aFamsIter != aFamilies.end(); aFamsIter++)
  {
    DriverMED_FamilyPtr aFam = *aFamsIter;
    if (aFam->myType == SMDSAbs_Node) {
      aFam->SetId(aNodeFamId++);
      if (doGroupOfNodes) aFam->myGroupNames.insert(anAllNodesGroupName);
    }
    else {
      aFam->SetId(aElemFamId--);
      if (aFam->myType == SMDSAbs_Edge) {
        if (doGroupOfEdges) aFam->myGroupNames.insert(anAllEdgesGroupName);
      }
      else if (aFam->myType == SMDSAbs_Face) {
        if (doGroupOfFaces) aFam->myGroupNames.insert(anAllFacesGroupName);
      }
      else if (aFam->myType == SMDSAbs_Volume) {
        if (doGroupOfVolumes) aFam->myGroupNames.insert(anAllVolumesGroupName);
      }
      else if (aFam->myType == SMDSAbs_0DElement) {
        if (doGroupOfVolumes) aFam->myGroupNames.insert(anAll0DElemsGroupName);
      }
      else if (aFam->myType == SMDSAbs_Ball) {
        if (doGroupOfVolumes) aFam->myGroupNames.insert(anAllBallsGroupName);
      }
    }
  }

  // Create families for elements, not belonging to any group
  if (doGroupOfNodes)
  {
    DriverMED_FamilyPtr aFreeNodesFam (new DriverMED_Family);
    aFreeNodesFam->SetId(REST_NODES_FAMILY);
    aFreeNodesFam->myType = SMDSAbs_Node;
    aFreeNodesFam->myGroupNames.insert(anAllNodesGroupName);
    aFamilies.push_back(aFreeNodesFam);
  }

  if (doGroupOfEdges)
  {
    DriverMED_FamilyPtr aFreeEdgesFam (new DriverMED_Family);
    aFreeEdgesFam->SetId(REST_EDGES_FAMILY);
    aFreeEdgesFam->myType = SMDSAbs_Edge;
    aFreeEdgesFam->myGroupNames.insert(anAllEdgesGroupName);
    aFamilies.push_back(aFreeEdgesFam);
  }

  if (doGroupOfFaces)
  {
    DriverMED_FamilyPtr aFreeFacesFam (new DriverMED_Family);
    aFreeFacesFam->SetId(REST_FACES_FAMILY);
    aFreeFacesFam->myType = SMDSAbs_Face;
    aFreeFacesFam->myGroupNames.insert(anAllFacesGroupName);
    aFamilies.push_back(aFreeFacesFam);
  }

  if (doGroupOfVolumes)
  {
    DriverMED_FamilyPtr aFreeVolumesFam (new DriverMED_Family);
    aFreeVolumesFam->SetId(REST_VOLUMES_FAMILY);
    aFreeVolumesFam->myType = SMDSAbs_Volume;
    aFreeVolumesFam->myGroupNames.insert(anAllVolumesGroupName);
    aFamilies.push_back(aFreeVolumesFam);
  }

  if (doGroupOf0DElems)
  {
    DriverMED_FamilyPtr aFree0DFam (new DriverMED_Family);
    aFree0DFam->SetId(REST_0DELEM_FAMILY);
    aFree0DFam->myType = SMDSAbs_0DElement;
    aFree0DFam->myGroupNames.insert(anAll0DElemsGroupName);
    aFamilies.push_back(aFree0DFam);
  }

  if (doGroupOfBalls)
  {
    DriverMED_FamilyPtr aFreeBallsFam (new DriverMED_Family);
    aFreeBallsFam->SetId(REST_BALL_FAMILY);
    aFreeBallsFam->myType = SMDSAbs_Ball;
    aFreeBallsFam->myGroupNames.insert(anAllBallsGroupName);
    aFamilies.push_back(aFreeBallsFam);
  }

  DriverMED_FamilyPtr aNullFam (new DriverMED_Family);
  aNullFam->SetId(0);
  aNullFam->myType = SMDSAbs_All;
  aFamilies.push_back(aNullFam);

  return aFamilies;
}

//=============================================================================
/*!
 *  Create TFamilyInfo for this family
 */
//=============================================================================
MED::PFamilyInfo 
DriverMED_Family::GetFamilyInfo(const MED::PWrapper& theWrapper, 
                                const MED::PMeshInfo& theMeshInfo) const
{
  ostringstream aStr;
  aStr << "FAM_" << myId;
  set<string>::const_iterator aGrIter = myGroupNames.begin();
  for(; aGrIter != myGroupNames.end(); aGrIter++){
    aStr << "_" << *aGrIter;
  }
  string aValue = aStr.str();
  // PAL19785,0019867 - med forbids whitespace to be the last char in the name
  int maxSize;
  //if ( theWrapper->GetVersion() == MED::eV2_1 )
  //  maxSize = MED::GetNOMLength<MED::eV2_1>();
  //else
//    maxSize = MED::GetNOMLength<MED::eV2_2>();
  maxSize = 2048;
  int lastCharPos = min( maxSize, (int) aValue.size() ) - 1;
  while ( isspace( aValue[ lastCharPos ] ))
    aValue.resize( lastCharPos-- );

  MED::PFamilyInfo anInfo;
  if(myId == 0 || myGroupAttributVal == 0){
    anInfo = theWrapper->CrFamilyInfo(theMeshInfo,
                                      aValue,
                                      myId,
                                      myGroupNames);
  }else{
    MED::TStringVector anAttrDescs (1, "");  // 1 attribute with empty description,
    MED::TIntVector anAttrIds (1, myId);        // Id=0,
    MED::TIntVector anAttrVals (1, myGroupAttributVal);
    anInfo = theWrapper->CrFamilyInfo(theMeshInfo,
                                      aValue,
                                      myId,
                                      myGroupNames,
                                      anAttrDescs,
                                      anAttrIds,
                                      anAttrVals);
  }

//  cout << endl;
//  cout << "Groups: ";
//  set<string>::iterator aGrIter = myGroupNames.begin();
//  for (; aGrIter != myGroupNames.end(); aGrIter++)
//  {
//    cout << " " << *aGrIter;
//  }
//  cout << endl;
//
//  cout << "Elements: ";
//  set<const SMDS_MeshElement *>::iterator anIter = myElements.begin();
//  for (; anIter != myElements.end(); anIter++)
//  {
//    cout << " " << (*anIter)->GetID();
//  }
//  cout << endl;

  return anInfo;
}

//=============================================================================
/*!
 *  Initialize the tool by SMESHDS_GroupBase
 */
//=============================================================================
void DriverMED_Family::Init (SMESHDS_GroupBase* theGroup)
{
  // Elements
  myElements.clear();
  SMDS_ElemIteratorPtr elemIt = theGroup->GetElements();
  while (elemIt->more())
  {
    myElements.insert( myElements.end(), elemIt->next() );
  }

  // Type
  myType = theGroup->GetType();

  // Groups list
  myGroupNames.clear();
  myGroupNames.insert(string(theGroup->GetStoreName()));

  Quantity_Color aColor = theGroup->GetColor();
  double aRed   = aColor.Red();
  double aGreen = aColor.Green();
  double aBlue  = aColor.Blue();
  int aR = int( aRed*255   );
  int aG = int( aGreen*255 );
  int aB = int( aBlue*255  );
//   cout << "aRed = " << aR << endl;
//   cout << "aGreen = " << aG << endl;
//   cout << "aBlue = " << aB << endl;
  myGroupAttributVal = (int)(aR*1000000 + aG*1000 + aB);
  //cout << "myGroupAttributVal = " << myGroupAttributVal << endl;
}

//=============================================================================
/*!
 *  Split <theSubMesh> on some parts (families)
 *  on the basis of the elements type.
 */
//=============================================================================
DriverMED_FamilyPtrList 
DriverMED_Family
::SplitByType (SMESHDS_SubMesh* theSubMesh,
               const int        theId)
{
  DriverMED_FamilyPtrList aFamilies;
  DriverMED_FamilyPtr aNodesFamily   (new DriverMED_Family);
  DriverMED_FamilyPtr anEdgesFamily  (new DriverMED_Family);
  DriverMED_FamilyPtr aFacesFamily   (new DriverMED_Family);
  DriverMED_FamilyPtr aVolumesFamily (new DriverMED_Family);
  // DriverMED_FamilyPtr a0DElemsFamily (new DriverMED_Family);
  // DriverMED_FamilyPtr aBallsFamily   (new DriverMED_Family);

  char submeshGrpName[ 30 ];
  sprintf( submeshGrpName, "SubMesh %d", theId );

  SMDS_NodeIteratorPtr aNodesIter = theSubMesh->GetNodes();
  while (aNodesIter->more())
  {
    const SMDS_MeshNode* aNode = aNodesIter->next();
    aNodesFamily->AddElement(aNode);
  }

  SMDS_ElemIteratorPtr anElemsIter = theSubMesh->GetElements();
  while (anElemsIter->more())
  {
    const SMDS_MeshElement* anElem = anElemsIter->next();
    switch (anElem->GetType())
    {
    case SMDSAbs_Edge:
      anEdgesFamily->AddElement(anElem);
      break;
    case SMDSAbs_Face:
      aFacesFamily->AddElement(anElem);
      break;
    case SMDSAbs_Volume:
      aVolumesFamily->AddElement(anElem);
      break;
    default:
      break;
    }
  }

  if (!aNodesFamily->IsEmpty()) {
    aNodesFamily->SetType(SMDSAbs_Node);
    aNodesFamily->AddGroupName(submeshGrpName);
    aFamilies.push_back(aNodesFamily);
  }
  if (!anEdgesFamily->IsEmpty()) {
    anEdgesFamily->SetType(SMDSAbs_Edge);
    anEdgesFamily->AddGroupName(submeshGrpName);
    aFamilies.push_back(anEdgesFamily);
  }
  if (!aFacesFamily->IsEmpty()) {
    aFacesFamily->SetType(SMDSAbs_Face);
    aFacesFamily->AddGroupName(submeshGrpName);
    aFamilies.push_back(aFacesFamily);
  }
  if (!aVolumesFamily->IsEmpty()) {
    aVolumesFamily->SetType(SMDSAbs_Volume);
    aVolumesFamily->AddGroupName(submeshGrpName);
    aFamilies.push_back(aVolumesFamily);
  }

  return aFamilies;
}

//=============================================================================
/*!
 *  Remove from <myElements> elements, common with <by>,
 *  Remove from <by> elements, common with <myElements>,
 *  Create family <common> from common elements, with combined groups list.
 */
//=============================================================================
void DriverMED_Family::Split (DriverMED_FamilyPtr by,
                              DriverMED_FamilyPtr common)
{
  // Elements
  ElementsSet::iterator anIter = by->myElements.begin(), elemInMe;
  while ( anIter != by->myElements.end())
  {
    elemInMe = myElements.find(*anIter);
    if (elemInMe != myElements.end())
    {
      common->myElements.insert(*anIter);
      myElements.erase(elemInMe);
      by->myElements.erase(anIter++);
    }
    else
      anIter++;
  }

  if (!common->IsEmpty())
  {
    // Groups list
    common->myGroupNames = myGroupNames;
    common->myGroupNames.insert( by->myGroupNames.begin(), by->myGroupNames.end() );

    // Type
    common->myType = myType;
  }
}
