// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

#include <algorithm>

#include "DriverUNV_W_SMDS_Mesh.h"

#include "SMDS_Mesh.hxx"
#include "SMDS_QuadraticEdge.hxx"
#include "SMDS_QuadraticFaceOfNodes.hxx"
#include "SMDS_PolyhedralVolumeOfNodes.hxx"
#include "SMESHDS_GroupBase.hxx"

#include "utilities.h"

#include "UNV164_Structure.hxx"
#include "UNV2411_Structure.hxx"
#include "UNV2412_Structure.hxx"
#include "UNV2417_Structure.hxx"
#include "UNV2420_Structure.hxx"
#include "UNV_Utilities.hxx"

#include <Basics_Utils.hxx>

using namespace std;
using namespace UNV;

namespace{
  typedef std::vector<size_t> TConnect;

  inline int GetConnect(const SMDS_ElemIteratorPtr& theNodesIter,
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
  // Kernel_Utils::Localizer loc;
  Status aResult = DRS_OK;
  std::ofstream out_stream(myFile.c_str());
  try{

    UNV164::Write( out_stream ); // unit system
    UNV2420::Write( out_stream, myMeshName ); // Coordinate system

    {
      using namespace UNV2411;
      TDataSet aDataSet2411;
      // Storing SMDS nodes to the UNV file
      //-----------------------------------
      MESSAGE("Perform - myMesh->NbNodes() = "<<myMesh->NbNodes());
      SMDS_NodeIteratorPtr aNodesIter = myMesh->nodesIterator();
      TRecord aRec;
      while ( aNodesIter->more() )
      {
        const SMDS_MeshNode* aNode = aNodesIter->next();
        aRec.label    = aNode->GetID();
        aRec.coord[0] = aNode->X();
        aRec.coord[1] = aNode->Y();
        aRec.coord[2] = aNode->Z();
        aDataSet2411.push_back( aRec );
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
        while( anIter->more() )
        {
          const SMDS_MeshEdge* anElem = anIter->next();
          int aNbNodes = anElem->NbNodes();
          TRecord aRec;
          aRec.label = anElem->GetID();
          aRec.node_labels.reserve(aNbNodes);
          if( anElem->IsQuadratic() ) {
            aRec.fe_descriptor_id = 22;
          } else {
            aRec.fe_descriptor_id = 11;
          }
          SMDS_NodeIteratorPtr aNodesIter = anElem->nodesIteratorToUNV();
          while( aNodesIter->more())
          {
            const SMDS_MeshNode* aNode = aNodesIter->next();
            aRec.node_labels.push_back( aNode->GetID() );
          }
          aDataSet2412.push_back(aRec);
        }
        MESSAGE("Perform - aDataSet2412.size() = "<<aDataSet2412.size());
      }

      MESSAGE("Perform - myMesh->NbFaces() = "<<myMesh->NbFaces());
      if ( myMesh->NbFaces() )
      {
        SMDS_FaceIteratorPtr anIter = myMesh->facesIterator();
        while ( anIter->more())
        {
          const SMDS_MeshFace* anElem = anIter->next();
          if ( anElem->IsPoly() ) continue;
          int aNbNodes = anElem->NbNodes();
          TRecord aRec;
          aRec.label = anElem->GetID();
          aRec.node_labels.reserve(aNbNodes);
          SMDS_NodeIteratorPtr aNodesIter = anElem->nodesIteratorToUNV();
          while( aNodesIter->more() ) {
            const SMDS_MeshNode* aNode = aNodesIter->next();
            aRec.node_labels.push_back( aNode->GetID() );
          }
          switch ( aNbNodes ) {
          case 3: aRec.fe_descriptor_id = 41; break;
          case 4: aRec.fe_descriptor_id = 44; break;
          case 6: aRec.fe_descriptor_id = 42; break;
          case 7: aRec.fe_descriptor_id = 42; break;
          case 8: aRec.fe_descriptor_id = 45; break;
          case 9: aRec.fe_descriptor_id = 45; aRec.node_labels.resize( 8 ); break;
          default:
            continue;
          }
          aDataSet2412.push_back(aRec);
        }
        MESSAGE("Perform - aDataSet2412.size() = "<<aDataSet2412.size());
      }

      MESSAGE("Perform - myMesh->NbVolumes() = "<<myMesh->NbVolumes());
      if ( myMesh->NbVolumes() )
      {
        SMDS_VolumeIteratorPtr anIter = myMesh->volumesIterator();
        while ( anIter->more())
        {
          const SMDS_MeshVolume* anElem = anIter->next();
          if ( anElem->IsPoly() )
            continue;
          int aNbNodes = anElem->NbNodes();
          int anId = -1;
          switch(aNbNodes) {
          case 4:  anId = 111; break;
          case 6:  anId = 112; break;
          case 8:  anId = 115; break;
          case 10: anId = 118; break;
	  // Quadratic Pyramid are not properly supported into UNV file and
	  // shouldn't be part of the output only wedge are supported
	  // http://www.sdrl.uc.edu/sdrl/referenceinfo/universalfileformats/
          // file-format-storehouse/universal-dataset-number-2412
          // case 13: anId = 114; break;
          case 15: anId = 113; break;
          case 20:
          case 27: anId = 116; aNbNodes = 20; break;
          default:
            continue;
          }
          if(anId>0){
            TRecord aRec;
            aRec.label = anElem->GetID();
            aRec.fe_descriptor_id = anId;
            aRec.node_labels.reserve(aNbNodes);
            SMDS_NodeIteratorPtr aNodesIter = anElem->nodesIteratorToUNV();
            while ( aNodesIter->more() && aRec.node_labels.size() < aNbNodes )
            {
              const SMDS_MeshElement* aNode = aNodesIter->next();
              aRec.node_labels.push_back(aNode->GetID());
            }
            aDataSet2412.push_back(aRec);
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
