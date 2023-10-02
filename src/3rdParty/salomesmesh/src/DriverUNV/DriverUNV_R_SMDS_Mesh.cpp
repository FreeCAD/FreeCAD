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

#include "DriverUNV_R_SMDS_Mesh.h"
#include "SMDS_Mesh.hxx"
#include "SMDS_MeshGroup.hxx"

#include "utilities.h"

#include "UNV164_Structure.hxx"
#include "UNV2411_Structure.hxx"
#include "UNV2412_Structure.hxx"
#include "UNV2417_Structure.hxx"
#include "UNV2420_Structure.hxx"
#include "UNV_Utilities.hxx"

#include <Basics_Utils.hxx>

using namespace std;


#ifdef _DEBUG_
static int MYDEBUG = 1;
#else
static int MYDEBUG = 0;
#endif

namespace
{
  /*!
   * \brief Move node coordinates to the global Cartesian CS
   */
  void transformNodes( UNV2411::TDataSet::const_iterator fromNode,
                       UNV2411::TDataSet::const_iterator endNode,
                       const UNV2420::TRecord &          csRecord )
  {
    const int csLabel = fromNode->exp_coord_sys_num;

    UNV2411::TDataSet::const_iterator nodeIt;

    // apply Transformation Matrix
    if ( !csRecord.isIdentityMatrix() )
    {
      for ( nodeIt = fromNode; nodeIt != endNode; ++nodeIt )
      {
        const UNV2411::TRecord& nodeRec = *nodeIt;
        if ( nodeRec.exp_coord_sys_num == csLabel )
          csRecord.ApplyMatrix( (double*) nodeRec.coord );
      }
    }

    // transform from Cylindrical CS
    if ( csRecord.coord_sys_type == UNV2420::Cylindrical )
    {
      for ( nodeIt = fromNode; nodeIt != endNode; ++nodeIt )
      {
        const UNV2411::TRecord& nodeRec = *nodeIt;
        if ( nodeRec.exp_coord_sys_num == csLabel )
          csRecord.FromCylindricalCS( (double*) nodeRec.coord );
      }
    }
    // transform from Spherical CS
    else if ( csRecord.coord_sys_type == UNV2420::Spherical )
    {
      for ( nodeIt = fromNode; nodeIt != endNode; ++nodeIt )
      {
        const UNV2411::TRecord& nodeRec = *nodeIt;
        if ( nodeRec.exp_coord_sys_num == csLabel )
          csRecord.FromSphericalCS( (double*) nodeRec.coord );
      }
    }
  }
}

DriverUNV_R_SMDS_Mesh::~DriverUNV_R_SMDS_Mesh()
{
  if (myGroup != 0) 
    delete myGroup;
}


Driver_Mesh::Status DriverUNV_R_SMDS_Mesh::Perform()
{
  // Kernel_Utils::Localizer loc;
  Status aResult = DRS_OK;
  // See http://forum.freecad.org/viewtopic.php?f=18&t=14404&p=135981#p136086 (wmayer)
  // Line endings in UNIX format cause problems on Windows, thus the file must be opened
  // in binary mode.
  std::ifstream in_stream(myFile.c_str(), std::ios::in | std::ios::binary);
  try
  {
    {
      // Read Units
      UNV164::TRecord aUnitsRecord;
      UNV164::Read( in_stream, aUnitsRecord );

      // Read Coordinate systems
      UNV2420::TDataSet aCoordSysDataSet;
      UNV2420::Read(in_stream, myMeshName, aCoordSysDataSet);

      // Read nodes
      using namespace UNV2411;
      TDataSet aDataSet2411;
      UNV2411::Read(in_stream,aDataSet2411);
      if(MYDEBUG) MESSAGE("Perform - aDataSet2411.size() = "<<aDataSet2411.size());

      // Move nodes in a global CS
      if ( !aCoordSysDataSet.empty() )
      {
        UNV2420::TDataSet::const_iterator csIter = aCoordSysDataSet.begin();
        for ( ; csIter != aCoordSysDataSet.end(); ++csIter )
        {
          // find any node in this CS
          TDataSet::const_iterator nodeIter = aDataSet2411.begin();
          for (; nodeIter != aDataSet2411.end(); nodeIter++)
            if ( nodeIter->exp_coord_sys_num == csIter->coord_sys_label )
            {
              transformNodes( nodeIter, aDataSet2411.end(), *csIter );
              break;
            }
        }
      }
      // Move nodes to SI unit system
      const double lenFactor = aUnitsRecord.factors[ UNV164::LENGTH_FACTOR ]; 
      if ( lenFactor != 1. )
      {
        TDataSet::iterator nodeIter = aDataSet2411.begin(), nodeEnd;
        for ( nodeEnd = aDataSet2411.end(); nodeIter != nodeEnd; nodeIter++)
        {
          UNV2411::TRecord& nodeRec = *nodeIter;
          nodeRec.coord[0] *= lenFactor;
          nodeRec.coord[1] *= lenFactor;
          nodeRec.coord[2] *= lenFactor;
        }
      }

      // Create nodes in the mesh
      TDataSet::const_iterator anIter = aDataSet2411.begin();
      for(; anIter != aDataSet2411.end(); anIter++)
      {
        const TRecord& aRec = *anIter;
        myMesh->AddNodeWithID(aRec.coord[0],aRec.coord[1],aRec.coord[2],aRec.label);
      }
    }
    {
      using namespace UNV2412;
      TDataSet aDataSet2412;
      UNV2412::Read(in_stream,aDataSet2412);
      TDataSet::const_iterator anIter = aDataSet2412.begin();
      if(MYDEBUG) MESSAGE("Perform - aDataSet2412.size() = "<<aDataSet2412.size());
      for(; anIter != aDataSet2412.end(); anIter++)
      {
        SMDS_MeshElement* anElement = NULL;
        const TRecord& aRec = *anIter;
        if(IsBeam(aRec.fe_descriptor_id)) {
          switch ( aRec.node_labels.size() ) {
          case 2: // edge with two nodes
            //MESSAGE("add edge " << aLabel << " " << aRec.node_labels[0] << " " << aRec.node_labels[1]);
            anElement = myMesh->AddEdgeWithID(aRec.node_labels[0],
                                              aRec.node_labels[1],
                                              aRec.label);
            break;
          case 3: // quadratic edge (with 3 nodes)
            //MESSAGE("add edge " << aRec.label << " " << aRec.node_labels[0] << " " << aRec.node_labels[1] << " " << aRec.node_labels[2]);
            anElement = myMesh->AddEdgeWithID(aRec.node_labels[0],
                                              aRec.node_labels[2],
                                              aRec.node_labels[1],
                                              aRec.label);
          }
        }
        else if(IsFace(aRec.fe_descriptor_id)) {
          //MESSAGE("add face " << aRec.label);
          switch(aRec.fe_descriptor_id){
          case 41: // Plane Stress Linear Triangle      
          case 51: // Plane Strain Linear Triangle      
          case 61: // Plate Linear Triangle             
          case 74: // Membrane Linear Triangle          
          case 81: // Axisymetric Solid Linear Triangle 
          case 91: // Thin Shell Linear Triangle        
            anElement = myMesh->AddFaceWithID(aRec.node_labels[0],
                                              aRec.node_labels[1],
                                              aRec.node_labels[2],
                                              aRec.label);
            break;

          case 42: //  Plane Stress Parabolic Triangle       
          case 52: //  Plane Strain Parabolic Triangle       
          case 62: //  Plate Parabolic Triangle              
          case 72: //  Membrane Parabolic Triangle           
          case 82: //  Axisymetric Solid Parabolic Triangle  
          case 92: //  Thin Shell Parabolic Triangle
            if ( aRec.node_labels.size() == 7 )
              anElement = myMesh->AddFaceWithID(aRec.node_labels[0],
                                                aRec.node_labels[2],
                                                aRec.node_labels[4],
                                                aRec.node_labels[1],
                                                aRec.node_labels[3],
                                                aRec.node_labels[5],
                                                aRec.node_labels[6],
                                                aRec.label);
            else
              anElement = myMesh->AddFaceWithID(aRec.node_labels[0],
                                                aRec.node_labels[2],
                                                aRec.node_labels[4],
                                                aRec.node_labels[1],
                                                aRec.node_labels[3],
                                                aRec.node_labels[5],
                                                aRec.label);
            break;

          case 44: // Plane Stress Linear Quadrilateral     
          case 54: // Plane Strain Linear Quadrilateral     
          case 64: // Plate Linear Quadrilateral            
          case 71: // Membrane Linear Quadrilateral         
          case 84: // Axisymetric Solid Linear Quadrilateral
          case 94: // Thin Shell Linear Quadrilateral       
            anElement = myMesh->AddFaceWithID(aRec.node_labels[0],
                                              aRec.node_labels[1],
                                              aRec.node_labels[2],
                                              aRec.node_labels[3],
                                              aRec.label);
            break;

          case 45: // Plane Stress Parabolic Quadrilateral      
          case 55: // Plane Strain Parabolic Quadrilateral      
          case 65: // Plate Parabolic Quadrilateral             
          case 75: // Membrane Parabolic Quadrilateral          
          case 85: // Axisymetric Solid Parabolic Quadrilateral 
          case 95: // Thin Shell Parabolic Quadrilateral        
            if ( aRec.node_labels.size() == 9 )
              anElement = myMesh->AddFaceWithID(aRec.node_labels[0],
                                                aRec.node_labels[2],
                                                aRec.node_labels[4],
                                                aRec.node_labels[6],
                                                aRec.node_labels[1],
                                                aRec.node_labels[3],
                                                aRec.node_labels[5],
                                                aRec.node_labels[7],
                                                aRec.node_labels[8],
                                                aRec.label);
            else
              anElement = myMesh->AddFaceWithID(aRec.node_labels[0],
                                                aRec.node_labels[2],
                                                aRec.node_labels[4],
                                                aRec.node_labels[6],
                                                aRec.node_labels[1],
                                                aRec.node_labels[3],
                                                aRec.node_labels[5],
                                                aRec.node_labels[7],
                                                aRec.label);
            break;
          }
        }
        else if(IsVolume(aRec.fe_descriptor_id)){
          //MESSAGE("add volume " << aRec.label);
          switch(aRec.fe_descriptor_id){

          case 111: // Solid Linear Tetrahedron - TET4
            anElement = myMesh->AddVolumeWithID(aRec.node_labels[0],
                                                aRec.node_labels[2],
                                                aRec.node_labels[1],
                                                aRec.node_labels[3],
                                                aRec.label);
            break;

          case 118: // Solid Quadratic Tetrahedron - TET10
            anElement = myMesh->AddVolumeWithID(aRec.node_labels[0],
                                                aRec.node_labels[4],
                                                aRec.node_labels[2],

                                                aRec.node_labels[9],

                                                aRec.node_labels[5],
                                                aRec.node_labels[3],
                                                aRec.node_labels[1],

                                                aRec.node_labels[6],
                                                aRec.node_labels[8],
                                                aRec.node_labels[7],
                                                aRec.label);
            break;
            
          case 112: // Solid Linear Prism - PRISM6
            anElement = myMesh->AddVolumeWithID(aRec.node_labels[0],
                                                aRec.node_labels[2],
                                                aRec.node_labels[1],
                                                aRec.node_labels[3],
                                                aRec.node_labels[5],
                                                aRec.node_labels[4],
                                                aRec.label);
            break;
            
          case 113: // Solid Quadratic Prism - PRISM15
            anElement = myMesh->AddVolumeWithID(aRec.node_labels[0],
                                                aRec.node_labels[4],
                                                aRec.node_labels[2],

                                                aRec.node_labels[9],
                                                aRec.node_labels[13],
                                                aRec.node_labels[11],

                                                aRec.node_labels[5],
                                                aRec.node_labels[3],
                                                aRec.node_labels[1],

                                                aRec.node_labels[14],
                                                aRec.node_labels[12],
                                                aRec.node_labels[10],

                                                aRec.node_labels[6],
                                                aRec.node_labels[8],
                                                aRec.node_labels[7],
                                                aRec.label);
            break;
            
          case 115: // Solid Linear Brick - HEX8
            anElement = myMesh->AddVolumeWithID(aRec.node_labels[0],
                                                aRec.node_labels[3],
                                                aRec.node_labels[2],
                                                aRec.node_labels[1],
                                                aRec.node_labels[4],
                                                aRec.node_labels[7],
                                                aRec.node_labels[6],
                                                aRec.node_labels[5],
                                                aRec.label);
            break;

          case 116: // Solid Quadratic Brick - HEX20
            anElement = myMesh->AddVolumeWithID(aRec.node_labels[0],
                                                aRec.node_labels[6],
                                                aRec.node_labels[4],
                                                aRec.node_labels[2],

                                                aRec.node_labels[12],
                                                aRec.node_labels[18],
                                                aRec.node_labels[16],
                                                aRec.node_labels[14],

                                                aRec.node_labels[7],
                                                aRec.node_labels[5],
                                                aRec.node_labels[3],
                                                aRec.node_labels[1],

                                                aRec.node_labels[19],
                                                aRec.node_labels[17],
                                                aRec.node_labels[15],
                                                aRec.node_labels[13],

                                                aRec.node_labels[8],
                                                aRec.node_labels[11],
                                                aRec.node_labels[10],
                                                aRec.node_labels[9],
                                                aRec.label);
            break;

          case 114: // pyramid of 13 nodes (quadratic) - PIRA13
            anElement = myMesh->AddVolumeWithID(aRec.node_labels[0],
                                                aRec.node_labels[6],
                                                aRec.node_labels[4],
                                                aRec.node_labels[2],
                                                aRec.node_labels[7],
                                                aRec.node_labels[5],
                                                aRec.node_labels[3],
                                                aRec.node_labels[1],

                                                aRec.node_labels[8],
                                                aRec.node_labels[11],
                                                aRec.node_labels[10],
                                                aRec.node_labels[9],
                                                aRec.node_labels[12],
                                                aRec.label);
            break;

          }
        }
        if(!anElement)
          MESSAGE("DriverUNV_R_SMDS_Mesh::Perform - can not add element with ID = "<<aRec.label<<" and type = "<<aRec.fe_descriptor_id);
      }
    }
    {
      using namespace UNV2417;      
      TDataSet aDataSet2417;
      UNV2417::Read(in_stream,aDataSet2417);
      if(MYDEBUG) MESSAGE("Perform - aDataSet2417.size() = "<<aDataSet2417.size());
      if  (aDataSet2417.size() > 0) {
        myGroup = new SMDS_MeshGroup(myMesh);
        TDataSet::const_iterator anIter = aDataSet2417.begin();
        for(; anIter != aDataSet2417.end(); anIter++){
          const TGroupId& aLabel = anIter->first;
          const TRecord& aRec = anIter->second;

          int aNodesNb = aRec.NodeList.size();
          int aElementsNb = aRec.ElementList.size();

          bool useSuffix = ((aNodesNb > 0) && (aElementsNb > 0));
          int i;
          if (aNodesNb > 0) {
            SMDS_MeshGroup* aNodesGroup = (SMDS_MeshGroup*) myGroup->AddSubGroup(SMDSAbs_Node);
            std::string aGrName = (useSuffix) ? aRec.GroupName + "_Nodes" : aRec.GroupName;
            int i = aGrName.find( "\r\n" );
            if (i > 0)
              aGrName.erase (i, 2);
            i = aGrName.find( "\r" );
            if (i > 0)
              aGrName.erase (i, 1);
            myGroupNames.insert(TGroupNamesMap::value_type(aNodesGroup, aGrName));
            myGroupId.insert(TGroupIdMap::value_type(aNodesGroup, aLabel));

            for (i = 0; i < aNodesNb; i++) {
              const SMDS_MeshNode* aNode = myMesh->FindNode(aRec.NodeList[i]);
              if (aNode)
                aNodesGroup->Add(aNode);
            }
          }
          if (aElementsNb > 0){
            SMDS_MeshGroup* aEdgesGroup = 0;
            SMDS_MeshGroup* aFacesGroup = 0;
            SMDS_MeshGroup* aVolumeGroup = 0;
            bool createdGroup = false;

            for (i = 0; i < aElementsNb; i++) {
              const SMDS_MeshElement* aElement = myMesh->FindElement(aRec.ElementList[i]);
              if (aElement) {
                switch (aElement->GetType()) {
                case SMDSAbs_Edge:
                  if (!aEdgesGroup) {
                    aEdgesGroup = (SMDS_MeshGroup*) myGroup->AddSubGroup(SMDSAbs_Edge);
                    if (!useSuffix && createdGroup) useSuffix = true;
                    std::string aEdgesGrName = (useSuffix) ? aRec.GroupName + "_Edges" : aRec.GroupName;
                    int i = aEdgesGrName.find( "\r\n" );
                    if (i > 0)
                      aEdgesGrName.erase (i, 2);
                    i = aEdgesGrName.find( "\r" );
                    if (i > 0)
                      aEdgesGrName.erase (i, 1);
                    myGroupNames.insert(TGroupNamesMap::value_type(aEdgesGroup, aEdgesGrName));
                    myGroupId.insert(TGroupIdMap::value_type(aEdgesGroup, aLabel));
                    createdGroup = true;
                  }
                  aEdgesGroup->Add(aElement);
                  break;
                case SMDSAbs_Face:
                  if (!aFacesGroup) {
                    aFacesGroup = (SMDS_MeshGroup*) myGroup->AddSubGroup(SMDSAbs_Face);
                    if (!useSuffix && createdGroup) useSuffix = true;
                    std::string aFacesGrName = (useSuffix) ? aRec.GroupName + "_Faces" : aRec.GroupName;
                    int i = aFacesGrName.find( "\r\n" );
                    if (i > 0)
                      aFacesGrName.erase (i, 2);
                    i = aFacesGrName.find( "\r" );
                    if (i > 0)
                      aFacesGrName.erase (i, 1);
                    myGroupNames.insert(TGroupNamesMap::value_type(aFacesGroup, aFacesGrName));
                    myGroupId.insert(TGroupIdMap::value_type(aFacesGroup, aLabel));
                    createdGroup = true;
                  }
                  aFacesGroup->Add(aElement);
                  break;
                case SMDSAbs_Volume:
                  if (!aVolumeGroup) {
                    aVolumeGroup = (SMDS_MeshGroup*) myGroup->AddSubGroup(SMDSAbs_Volume);
                    if (!useSuffix && createdGroup) useSuffix = true;
                    std::string aVolumeGrName = (useSuffix) ? aRec.GroupName + "_Volumes" : aRec.GroupName;
                    int i = aVolumeGrName.find( "\r\n" );
                    if (i > 0)
                      aVolumeGrName.erase (i, 2);
                    i = aVolumeGrName.find( "\r" );
                    if (i > 0)
                      aVolumeGrName.erase (i, 1);
                    myGroupNames.insert(TGroupNamesMap::value_type(aVolumeGroup, aVolumeGrName));
                    myGroupId.insert(TGroupIdMap::value_type(aVolumeGroup, aLabel));
                    createdGroup = true;
                  }
                  aVolumeGroup->Add(aElement);
                  break;
                }
              } 
            }
          }
        }
      }
    } 
  }
  catch(const std::exception& exc){
    INFOS("Follow exception was cought:\n\t"<<exc.what());
  }
  catch(...){
    INFOS("Unknown exception was cought !!!");
  }
  if (myMesh)
    myMesh->compactMesh();
  return aResult;
}
