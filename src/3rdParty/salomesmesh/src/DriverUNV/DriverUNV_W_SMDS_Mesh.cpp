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
#include <algorithm>

#include "DriverUNV_W_SMDS_Mesh.h"

#include "SMDS_Mesh.hxx"
#include "SMDS_QuadraticEdge.hxx"
#include "SMDS_QuadraticFaceOfNodes.hxx"
#include "SMDS_PolyhedralVolumeOfNodes.hxx"
#include "SMESHDS_GroupBase.hxx"

#include "utilities.h"

#include "UNV2411_Structure.hxx"
#include "UNV2412_Structure.hxx"
#include "UNV2417_Structure.hxx"
#include "UNV_Utilities.hxx"

using namespace std;
using namespace UNV;

namespace{
  typedef std::vector<size_t> TConnect;

  int GetConnect(const SMDS_ElemIteratorPtr& theNodesIter, 
		 TConnect& theConnect)
  {
    theConnect.clear();
    for(; theNodesIter->more();){
      const SMDS_MeshElement* anElem = theNodesIter->next();
      theConnect.push_back(anElem->GetID());
    }
    return theConnect.size();
  }
  
}

Driver_Mesh::Status DriverUNV_W_SMDS_Mesh::Perform()
{
  Status aResult = DRS_OK;
  std::ofstream out_stream(myFile.c_str());
  try{
    {
      using namespace UNV2411;
      TDataSet aDataSet2411;
      // Storing SMDS nodes to the UNV file
      //-----------------------------------
      MESSAGE("Perform - myMesh->NbNodes() = "<<myMesh->NbNodes());
      SMDS_NodeIteratorPtr aNodesIter = myMesh->nodesIterator();
      for(; aNodesIter->more();){
	const SMDS_MeshNode* aNode = aNodesIter->next();
	TRecord aRec;
	aRec.coord[0] = aNode->X();
	aRec.coord[1] = aNode->Y();
	aRec.coord[2] = aNode->Z();
	const TNodeLab& aLabel = aNode->GetID();
	aDataSet2411.insert(TDataSet::value_type(aLabel,aRec));
      }
      MESSAGE("Perform - aDataSet2411.size() = "<<aDataSet2411.size());
      UNV2411::Write(out_stream,aDataSet2411);
    }
    {
      using namespace UNV2412;
      TDataSet aDataSet2412;
      TConnect aConnect;

      // Storing SMDS Edges
      MESSAGE("Perform - myMesh->NbEdges() = "<<myMesh->NbEdges());
      if(myMesh->NbEdges()){
	SMDS_EdgeIteratorPtr anIter = myMesh->edgesIterator();
	for(; anIter->more();){
	  const SMDS_MeshEdge* anElem = anIter->next();
	  TElementLab aLabel = anElem->GetID();
	  int aNbNodes = anElem->NbNodes();
	  TRecord aRec;
	  aRec.node_labels.reserve(aNbNodes);
	  SMDS_ElemIteratorPtr aNodesIter;
          if( anElem->IsQuadratic() ) {
            aNodesIter = static_cast<const SMDS_QuadraticEdge* >
              ( anElem )->interlacedNodesElemIterator();
            aRec.fe_descriptor_id = 22;
          } else {
            aNodesIter = anElem->nodesIterator();
            aRec.fe_descriptor_id = 11;
          }
	  for(; aNodesIter->more();){
	    const SMDS_MeshElement* aNode = aNodesIter->next();
	    aRec.node_labels.push_back(aNode->GetID());
	  }
	  aDataSet2412.insert(TDataSet::value_type(aLabel,aRec));
	}
	MESSAGE("Perform - aDataSet2412.size() = "<<aDataSet2412.size());
      }

      MESSAGE("Perform - myMesh->NbFaces() = "<<myMesh->NbFaces());
      if(myMesh->NbFaces()){
	SMDS_FaceIteratorPtr anIter = myMesh->facesIterator();
	for(; anIter->more();){
	  const SMDS_MeshFace* anElem = anIter->next();
	  TElementLab aLabel = anElem->GetID();
	  int aNbNodes = anElem->NbNodes();
	  TRecord aRec;
	  aRec.node_labels.reserve(aNbNodes);
	  SMDS_ElemIteratorPtr aNodesIter;
          if( anElem->IsQuadratic() )
            aNodesIter = static_cast<const SMDS_QuadraticFaceOfNodes* >
              ( anElem )->interlacedNodesElemIterator();
          else
            aNodesIter = anElem->nodesIterator();
	  for(; aNodesIter->more();){
	    const SMDS_MeshElement* aNode = aNodesIter->next();
	    aRec.node_labels.push_back(aNode->GetID());
	  }
	  switch(aNbNodes){
	  case 3:
	    aRec.fe_descriptor_id = 41;
	    break;
	  case 4:
	    aRec.fe_descriptor_id = 44;
	    break;
	  case 6:
	    aRec.fe_descriptor_id = 42;
	    break;
	  case 8:
	    aRec.fe_descriptor_id = 45;
	    break;
	  default:
	    continue;
	  }
	  aDataSet2412.insert(TDataSet::value_type(aLabel,aRec));
	}
	MESSAGE("Perform - aDataSet2412.size() = "<<aDataSet2412.size());
      }

      MESSAGE("Perform - myMesh->NbVolumes() = "<<myMesh->NbVolumes());
      if(myMesh->NbVolumes()){
	SMDS_VolumeIteratorPtr anIter = myMesh->volumesIterator();
	for(; anIter->more();){
	  const SMDS_MeshVolume* anElem = anIter->next();
	  TElementLab aLabel = anElem->GetID();

	  int aNbNodes = anElem->NbNodes();
	  SMDS_ElemIteratorPtr aNodesIter = anElem->nodesIterator();
          if ( anElem->IsPoly() ) {
            if ( const SMDS_PolyhedralVolumeOfNodes* ph =
                 dynamic_cast<const SMDS_PolyhedralVolumeOfNodes*> (anElem))
            {
              aNbNodes = ph->NbUniqueNodes();
              aNodesIter = ph->uniqueNodesIterator();
            }
          }
	  aConnect.resize(aNbNodes);
	  GetConnect(aNodesIter,aConnect);

	  int anId = -1;
	  int* aConn = NULL;
	  switch(aNbNodes){
	  case 4: {
	    static int anIds[] = {0,2,1,3};
	    aConn = anIds;
	    anId = 111;
	    break;
	  }
	  case 6: {
	    static int anIds[] = {0,2,1,3,5,4};
	    aConn = anIds;
	    anId = 112;
	    break;
	  }
	  case 8: {
	    static int anIds[] = {0,3,2,1,4,7,6,5};
	    aConn = anIds;
	    anId = 115;
	    break;
	  }
	  case 10: {
	    static int anIds[] = {0,4,2,9,5,3, 1,6,8, 7};
	    aConn = anIds;
	    anId = 118;
	    break;
	  }
	  case 13: {
	    static int anIds[] = {0,6,4,2,7,5,3,1,8,11,10,9,12};
	    aConn = anIds;
	    anId = 114;
	    break;
	  }
	  case 15: {
	    static int anIds[] = {0,4,2,9,13,11,5,3,1,14,12,10,6,8,7};
	    aConn = anIds;
	    anId = 113;
	    break;
	  }
	  case 20: {
	    static int anIds[] = {0,6, 4,2, 12,18,16,14,7, 5, 3, 1, 19,17,15,13,8, 11,10,9};
	    aConn = anIds;
	    anId = 116;
	    break;
	  }
	  default:
	    continue;
	  }
	  if(aConn){
	    TRecord aRec;
	    aRec.fe_descriptor_id = anId;
	    aRec.node_labels.resize(aNbNodes);
	    for(int aNodeId = 0; aNodeId < aNbNodes; aNodeId++){
	      aRec.node_labels[aConn[aNodeId]] = aConnect[aNodeId];
	    }
	    aDataSet2412.insert(TDataSet::value_type(aLabel,aRec));
	  }
	}
	MESSAGE("Perform - aDataSet2412.size() = "<<aDataSet2412.size());
      }
      UNV2412::Write(out_stream,aDataSet2412);
    }
    {
      using namespace UNV2417;
      if (myGroups.size() > 0) {
	TDataSet aDataSet2417;
	TGroupList::const_iterator aIter = myGroups.begin();
	for (; aIter != myGroups.end(); aIter++) {
	  SMESHDS_GroupBase* aGroupDS = *aIter;
	  TRecord aRec;
	  aRec.GroupName = aGroupDS->GetStoreName();

	  int i;
	  SMDS_ElemIteratorPtr aIter = aGroupDS->GetElements();
	  if (aGroupDS->GetType() == SMDSAbs_Node) {
	    aRec.NodeList.resize(aGroupDS->Extent());
	    i = 0;
	    while (aIter->more()) {
	      const SMDS_MeshElement* aElem = aIter->next();
	      aRec.NodeList[i] = aElem->GetID(); 
	      i++;
	    }
	  } else {
	    aRec.ElementList.resize(aGroupDS->Extent());
	    i = 0;
	    while (aIter->more()) {
	      const SMDS_MeshElement* aElem = aIter->next();
	      aRec.ElementList[i] = aElem->GetID(); 
	      i++;
	    }
	  }
          // 0019936: EDF 794 SMESH : Export UNV : Node color and group id
	  //aDataSet2417.insert(TDataSet::value_type(aGroupDS->GetID(), aRec));
          aDataSet2417.insert(TDataSet::value_type(aGroupDS->GetID()+1, aRec));
	}
	UNV2417::Write(out_stream,aDataSet2417);
	myGroups.clear();
      }
    }
    /*    {
      using namespace UNV2417;
      TDataSet aDataSet2417;
      for ( TGroupsMap::iterator it = myGroupsMap.begin(); it != myGroupsMap.end(); it++ ) {
	SMESH_Group*       aGroup   = it->second;
	SMESHDS_GroupBase* aGroupDS = aGroup->GetGroupDS();
	if ( aGroupDS ) {
	  TRecord aRec;
	  aRec.GroupName = aGroup->GetName();
	  int i;
	  SMDS_ElemIteratorPtr aIter = aGroupDS->GetElements();
	  if (aGroupDS->GetType() == SMDSAbs_Node) {
	    aRec.NodeList.resize(aGroupDS->Extent());
	    i = 0;
	    while (aIter->more()) {
	      const SMDS_MeshElement* aElem = aIter->next();
	      aRec.NodeList[i] = aElem->GetID(); 
	      i++;
	    }
	  } else {
	    aRec.ElementList.resize(aGroupDS->Extent());
	    i = 0;
	    while (aIter->more()) {
	      const SMDS_MeshElement* aElem = aIter->next();
	      aRec.ElementList[i] = aElem->GetID(); 
	      i++;
	    }
	  }
	  aDataSet2417.insert(TDataSet::value_type(aGroupDS->GetID(), aRec));
	}
      }
      UNV2417::Write(out_stream,aDataSet2417);
      }*/

    out_stream.flush();
    out_stream.close();
    if (!check_file(myFile))
      EXCEPTION(runtime_error,"ERROR: Output file not good.");
  }
  catch(const std::exception& exc){
    INFOS("Follow exception was cought:\n\t"<<exc.what());
    throw;
  }
  catch(...){
    INFOS("Unknown exception was cought !!!");
    throw;
  }
  return aResult;
}
