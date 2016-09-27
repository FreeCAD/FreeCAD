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

//  SMESH DriverMED : driver to read and write 'med' files
//  File   : DriverMED_R_SMESHDS_Mesh.cxx
//  Module : SMESH

#include "DriverMED_R_SMESHDS_Mesh.h"

#include "DriverMED_Family.h"
#include "SMESHDS_Group.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Comment.hxx"

#include "MED_CoordUtils.hxx"
#include "MED_Factory.hxx"
#include "MED_Utilities.hxx"

#include <NCollection_Map.hxx>

#include "utilities.h"

//#include <stdlib.h>

#ifdef _DEBUG_
static int MYDEBUG = 1;
//#define _DEXCEPT_
#else
static int MYDEBUG = 0;
#endif

#define _EDF_NODE_IDS_

using namespace MED;
using namespace std;

typedef std::map<int, DriverMED_FamilyPtr> TID2FamilyMap;

namespace DriverMED
{
  bool buildMeshGrille(const MED::PWrapper&  theWrapper,
                       const MED::PMeshInfo& theMeshInfo,
                       SMESHDS_Mesh*         theMesh,
                       const TID2FamilyMap&  myFamilies);
  /*!
   * \brief Ensure aFamily has a required ID
   * \param aFamily - a family to check
   * \param anID - an ID aFamily should have
   * \param myFamilies - a map of the family ID to the Family
   * \retval bool  - true if successful
   */
  bool checkFamilyID(DriverMED_FamilyPtr & aFamily,
                     int                   anID,
                     const TID2FamilyMap&  myFamilies);


  const SMDS_MeshNode* FindNode(const SMDS_Mesh* theMesh, TInt theId)
  {
    const SMDS_MeshNode* aNode = theMesh->FindNode(theId);
    if(aNode) return aNode;
    EXCEPTION(runtime_error,"SMDS_Mesh::FindNode - cannot find a SMDS_MeshNode for ID = "<<theId);
  }

}

//================================================================================
/*!
 * \brief Stores a mesh name
 */
//================================================================================

void DriverMED_R_SMESHDS_Mesh::SetMeshName(string theMeshName)
{
  myMeshName = theMeshName;
}

//================================================================================
/*!
 * \brief Reads a med file
 */
//================================================================================

Driver_Mesh::Status DriverMED_R_SMESHDS_Mesh::Perform()
{
  using namespace DriverMED;

  Status aResult = DRS_FAIL;
  bool isDescConn = false; // Mantis issue 0020483
#ifndef _DEXCEPT_
  try {
#endif
    myFamilies.clear();
    if(MYDEBUG) MESSAGE("Perform - myFile : "<<myFile);
    PWrapper aMed = CrWrapper(myFile,false); // We are using the internal MED file version checker instead of an external reader

    aResult = DRS_EMPTY;
    TInt aNbMeshes = aMed->GetNbMeshes();
    for (int iMesh = 0; iMesh < aNbMeshes; iMesh++)
    {
      // Reading the MED mesh
      //---------------------
      PMeshInfo aMeshInfo = aMed->GetPMeshInfo(iMesh+1);

      string aMeshName;
      if (myMeshId != -1) aMeshName = SMESH_Comment( myMeshId );
      else                aMeshName = myMeshName;

      if(MYDEBUG) MESSAGE("Perform - aMeshName : "<<aMeshName<<"; "<<aMeshInfo->GetName());
      if ( aMeshName != aMeshInfo->GetName() ) continue;
      aResult = DRS_OK;

      // Reading MED families to the temporary structure
      //------------------------------------------------
      TErr anErr;
      TInt aNbFams = aMed->GetNbFamilies(aMeshInfo);
      if(MYDEBUG) MESSAGE("Read " << aNbFams << " families");
      for (TInt iFam = 0; iFam < aNbFams; iFam++)
      {
        PFamilyInfo aFamilyInfo = aMed->GetPFamilyInfo(aMeshInfo,iFam+1,&anErr);
        if(anErr >= 0){
          TInt aFamId = aFamilyInfo->GetId();
          if(MYDEBUG) MESSAGE("Family " << aFamId << " :");

          DriverMED_FamilyPtr aFamily (new DriverMED_Family);

          TInt aNbGrp = aFamilyInfo->GetNbGroup();
          if(MYDEBUG) MESSAGE("belong to " << aNbGrp << " groups");
          bool isAttrOk = false;
          if(aFamilyInfo->GetNbAttr() == aNbGrp)
            isAttrOk = true;
          for (TInt iGr = 0; iGr < aNbGrp; iGr++)
          {
            string aGroupName = aFamilyInfo->GetGroupName(iGr);
            if ( isAttrOk ) {
              TInt anAttrVal = aFamilyInfo->GetAttrVal(iGr);
              aFamily->SetGroupAttributVal(anAttrVal);
            }
            if(MYDEBUG) MESSAGE(aGroupName);
            aFamily->AddGroupName(aGroupName);
          }
          aFamily->SetId( aFamId );
          myFamilies[aFamId] = aFamily;
        }
      }

      if (aMeshInfo->GetType() == MED::eSTRUCTURE)
      {
        /*bool aRes = */DriverMED::buildMeshGrille(aMed,aMeshInfo,myMesh,myFamilies);
        continue;
      }

      // Reading MED nodes to the corresponding SMDS structure
      //------------------------------------------------------
      PNodeInfo aNodeInfo = aMed->GetPNodeInfo(aMeshInfo);
      if (!aNodeInfo) {
        aResult = addMessage("No nodes", /*isFatal=*/true );
        continue;
      }
      aMeshInfo->myDim=aMeshInfo->mySpaceDim;// ignore meshdim in MEDFile because it can be false
      PCoordHelper aCoordHelper = GetCoordHelper(aNodeInfo);

      EBooleen anIsNodeNum = aNodeInfo->IsElemNum();
      TInt aNbElems = aNodeInfo->GetNbElem();
      if(MYDEBUG) MESSAGE("Perform - aNodeInfo->GetNbElem() = "<<aNbElems<<"; anIsNodeNum = "<<anIsNodeNum);
      DriverMED_FamilyPtr aFamily;
      for ( TInt iElem = 0; iElem < aNbElems; iElem++ )
      {
        TCCoordSlice aCoordSlice = aNodeInfo->GetCoordSlice(iElem);
        double aCoords[3] = {0.0, 0.0, 0.0};
        for(TInt iDim = 0; iDim < 3; iDim++)
          aCoords[iDim] = aCoordHelper->GetCoord(aCoordSlice,iDim);
        const SMDS_MeshNode* aNode;
        if ( anIsNodeNum ) {
          aNode = myMesh->AddNodeWithID
            (aCoords[0],aCoords[1],aCoords[2],aNodeInfo->GetElemNum(iElem));
        }
        else {
          aNode = myMesh->AddNodeWithID
            (aCoords[0],aCoords[1],aCoords[2], iElem+1);
        }

        // Save reference to this node from its family
        TInt aFamNum = aNodeInfo->GetFamNum(iElem);
        if ( DriverMED::checkFamilyID ( aFamily, aFamNum, myFamilies ))
        {
          aFamily->AddElement(aNode);
          aFamily->SetType(SMDSAbs_Node);
        }
      }

      // Are there any MED cells in descending connectivity
      // Mantis issue 0020483
      //---------------------------------------------------
      NCollection_Map<EEntiteMaillage> aDescendingEntitiesMap;
      if (!isDescConn) {
        MED::TEntityInfo aEntityInfoDesc = aMed->GetEntityInfo(aMeshInfo, eDESC);
        MED::TEntityInfo::iterator anEntityIterDesc = aEntityInfoDesc.begin();
        //for (; anEntityIterDesc != aEntityInfoDesc.end() && !isDescConn; anEntityIterDesc++) {
        for (; anEntityIterDesc != aEntityInfoDesc.end(); anEntityIterDesc++) {
          const EEntiteMaillage& anEntity = anEntityIterDesc->first;
          aDescendingEntitiesMap.Add(anEntity);
          //if (anEntity != eNOEUD) isDescConn = true;
        }
      }

      // Reading pre information about all MED cells
      //--------------------------------------------
      typedef MED::TVector<int> TNodeIds;
      bool takeNumbers = true;  // initially we trust the numbers from file
      MED::TEntityInfo aEntityInfo = aMed->GetEntityInfo(aMeshInfo, eNOD);
      MED::TEntityInfo::iterator anEntityIter = aEntityInfo.begin();

      for (; anEntityIter != aEntityInfo.end(); anEntityIter++)
      {
        const EEntiteMaillage& anEntity = anEntityIter->first;
        aDescendingEntitiesMap.Remove(anEntity); // Mantis issue 0020483
        if (anEntity == eNOEUD) continue;

        // Reading MED cells to the corresponding SMDS structure
        //------------------------------------------------------
        const MED::TGeom2Size& aGeom2Size = anEntityIter->second;
        MED::TGeom2Size::const_iterator aGeom2SizeIter = aGeom2Size.begin();
        for ( ; aGeom2SizeIter != aGeom2Size.end(); aGeom2SizeIter++)
        {
          const EGeometrieElement& aGeom = aGeom2SizeIter->first;

          if ( anEntity == eSTRUCT_ELEMENT ) // MED_BALL (issue 0021459)
          {
            PBallInfo aBallInfo = aMed->GetPBallInfo(aMeshInfo);
            TInt      aNbBalls  = aBallInfo->GetNbElem();

            EBooleen anIsElemNum = takeNumbers ? aBallInfo->IsElemNum() : eFAUX;
            if ( anIsElemNum && aBallInfo->myElemNum->empty() )
              anIsElemNum = eFAUX;

            // get supporting nodes
            TNodeIds aNodeIds;
#ifdef _EDF_NODE_IDS_
            if(anIsNodeNum) {
              aNodeIds.resize( aNbBalls );
              for(TInt iBall = 0; iBall < aNbBalls && anIsNodeNum; iBall++)
              {
                aNodeIds[iBall] = aNodeInfo->GetElemNum( (*aBallInfo->myConn)[ iBall ]-1 );
                anIsNodeNum = myMesh->FindNode( aNodeIds[iBall] ) ? eVRAI : eFAUX;
              }
            }
#endif
            if ( !anIsNodeNum )
              aNodeIds.swap( *(aBallInfo->myConn ));

            // allocate array of diameters
            vtkIdType maxID = myMesh->MaxElementID() + aNbBalls;
            if ( anIsElemNum && !aBallInfo->myElemNum->empty() )
              maxID = *std::max_element( aBallInfo->myElemNum->begin(),
                                         aBallInfo->myElemNum->end() );
            myMesh->getGrid()->AllocateDiameters( maxID ); // performance optimization

            // create balls
            SMDS_MeshElement* anElement;
            DriverMED_FamilyPtr aFamily;
            for ( TInt iBall = 0; iBall < aNbBalls; iBall++)
            {
              anElement = 0;
              if ( anIsElemNum ) {
                if (!(anElement = myMesh->AddBallWithID( aNodeIds[iBall],
                                                         aBallInfo->myDiameters[iBall],
                                                         aBallInfo->GetElemNum(iBall))))
                  anIsElemNum = eFAUX;
              }
              if ( !anElement )
                myMesh->AddBall( myMesh->FindNode( aNodeIds[iBall]),
                                 aBallInfo->myDiameters[iBall] );

              // Save reference to this element from its family
              TInt aFamNum = aBallInfo->GetFamNum(iBall);
              if ( DriverMED::checkFamilyID ( aFamily, aFamNum, myFamilies ))
              {
                aFamily->AddElement(anElement);
                aFamily->SetType( SMDSAbs_Ball );
              }
            }

            if ( !anIsElemNum &&
                 ( takeNumbers && aBallInfo->IsElemNum() && !aBallInfo->myElemNum->empty() ))
              if ( aResult < DRS_WARN_RENUMBER )
                aResult = DRS_WARN_RENUMBER;

            continue;
          } // MED_BALL

          switch(aGeom) {
          // case ePOINT1: ## PAL16410
          //     break;
          case ePOLYGONE:
          case ePOLYGON2:
          {
            PPolygoneInfo aPolygoneInfo = aMed->GetPPolygoneInfo(aMeshInfo,anEntity,aGeom);
            EBooleen anIsElemNum = takeNumbers ? aPolygoneInfo->IsElemNum() : eFAUX;

            typedef SMDS_MeshFace* (SMESHDS_Mesh::* FAddPolyWithID)
              (const std::vector<int> & nodes_ids, const int ID);
            typedef SMDS_MeshFace* (SMESHDS_Mesh::* FAddPolygon)
              (const std::vector<const SMDS_MeshNode*> & nodes);

            FAddPolyWithID addPolyWithID = & SMESHDS_Mesh::AddPolygonalFaceWithID;
            FAddPolygon       addPolygon = & SMESHDS_Mesh::AddPolygonalFace;
            if ( aGeom == ePOLYGON2 ) {
              addPolyWithID = & SMESHDS_Mesh::AddQuadPolygonalFaceWithID;
              addPolygon    = & SMESHDS_Mesh::AddQuadPolygonalFace;
            }
            TNodeIds aNodeIds;
            vector<const SMDS_MeshNode*> aNodes;
            const TInt aNbElem = aPolygoneInfo->GetNbElem();
            for ( TInt iElem = 0; iElem < aNbElem; iElem++ )
            {
              MED::TCConnSlice aConnSlice = aPolygoneInfo->GetConnSlice(iElem);
              TInt aNbConn = aPolygoneInfo->GetNbConn(iElem);
              aNodeIds.resize( aNbConn );
#ifdef _EDF_NODE_IDS_
              if(anIsNodeNum)
                for(TInt iConn = 0; iConn < aNbConn; iConn++)
                  aNodeIds[iConn] = aNodeInfo->GetElemNum(aConnSlice[iConn] - 1);
              else
                for(TInt iConn = 0; iConn < aNbConn; iConn++)
                  aNodeIds[iConn] = aConnSlice[iConn];
#else
              for(TInt iConn = 0; iConn < aNbConn; iConn++)
                aNodeIds[iConn] = aConnSlice[iConn];
#endif
              bool isRenum = false;
              SMDS_MeshElement* anElement = NULL;
              TInt aFamNum = aPolygoneInfo->GetFamNum(iElem);
#ifndef _DEXCEPT_
              try {
#endif
                if ( anIsElemNum ) {
                  TInt anElemId = aPolygoneInfo->GetElemNum( iElem );
                  anElement = (myMesh->*addPolyWithID)( aNodeIds, anElemId );
                }
                if ( !anElement ) {
                  aNodes.resize( aNbConn );
                  for ( TInt iConn = 0; iConn < aNbConn; iConn++ )
                    aNodes[iConn] = FindNode( myMesh, aNodeIds[iConn] );
                  anElement = (myMesh->*addPolygon)( aNodes );
                  isRenum = anIsElemNum;
                }
#ifndef _DEXCEPT_
              } catch(const std::exception& exc) {
                aResult = addMessage( exc.what(), /*isFatal=*/true );
              } catch (...) {
                aResult = addMessage( "Unknown exception", /*isFatal=*/true );
              }
#endif
              if ( !anElement ) {
                aResult = DRS_WARN_SKIP_ELEM;
              }
              else {
                if ( isRenum ) {
                  anIsElemNum = eFAUX;
                  takeNumbers = false;
                  if(aResult < DRS_WARN_RENUMBER)
                    aResult = DRS_WARN_RENUMBER;
                }
                if ( DriverMED::checkFamilyID ( aFamily, aFamNum, myFamilies ))
                {
                  // Save reference to this element from its family
                  aFamily->AddElement(anElement);
                  aFamily->SetType(anElement->GetType());
                }
              }
            }
            break;
          }
          case ePOLYEDRE: {
            PPolyedreInfo aPolyedreInfo = aMed->GetPPolyedreInfo(aMeshInfo,anEntity,aGeom);
            EBooleen anIsElemNum = takeNumbers ? aPolyedreInfo->IsElemNum() : eFAUX;

            TInt aNbElem = aPolyedreInfo->GetNbElem();
            for(TInt iElem = 0; iElem < aNbElem; iElem++){
              MED::TCConnSliceArr aConnSliceArr = aPolyedreInfo->GetConnSliceArr(iElem);
              TInt aNbFaces = aConnSliceArr.size();
              typedef MED::TVector<int> TQuantities;
              TQuantities aQuantities(aNbFaces);
              TInt aNbNodes = aPolyedreInfo->GetNbNodes(iElem);
              TNodeIds aNodeIds(aNbNodes);
              for(TInt iFace = 0, iNode = 0; iFace < aNbFaces; iFace++){
                MED::TCConnSlice aConnSlice = aConnSliceArr[iFace];
                TInt aNbConn = aConnSlice.size();
                aQuantities[iFace] = aNbConn;
#ifdef _EDF_NODE_IDS_
                if(anIsNodeNum)
                  for(TInt iConn = 0; iConn < aNbConn; iConn++)
                  {
                    aNodeIds[iNode] = aNodeInfo->GetElemNum(aConnSlice[iConn] - 1);
                    iNode++;
                  }
                else
                  for(TInt iConn = 0; iConn < aNbConn; iConn++)
                  {
                    aNodeIds[iNode++] = aConnSlice[iConn];
                  }
#else
                for(TInt iConn = 0; iConn < aNbConn; iConn++)
                {
                  aNodeIds[iNode++] = aConnSlice[iConn];
                }
#endif          
              }

              bool isRenum = false;
              SMDS_MeshElement* anElement = NULL;
              TInt aFamNum = aPolyedreInfo->GetFamNum(iElem);
                
#ifndef _DEXCEPT_
              try{
#endif
                if(anIsElemNum){
                  TInt anElemId = aPolyedreInfo->GetElemNum(iElem);
                  anElement = myMesh->AddPolyhedralVolumeWithID(aNodeIds,aQuantities,anElemId);
                }
                if(!anElement){
                  vector<const SMDS_MeshNode*> aNodes(aNbNodes);
                  for(TInt iConn = 0; iConn < aNbNodes; iConn++)
                    aNodes[iConn] = FindNode(myMesh,aNodeIds[iConn]);
                  anElement = myMesh->AddPolyhedralVolume(aNodes,aQuantities);
                  isRenum = anIsElemNum;
                }
#ifndef _DEXCEPT_
              }catch(const std::exception& exc){
                aResult = DRS_FAIL;
              }catch(...){
                aResult = DRS_FAIL;
              }
#endif          
              if(!anElement){
                aResult = DRS_WARN_SKIP_ELEM;
              }else{
                if(isRenum){
                  anIsElemNum = eFAUX;
                  takeNumbers = false;
                  if (aResult < DRS_WARN_RENUMBER)
                    aResult = DRS_WARN_RENUMBER;
                }
                if ( DriverMED::checkFamilyID ( aFamily, aFamNum, myFamilies )) {
                  // Save reference to this element from its family
                  aFamily->AddElement(anElement);
                  aFamily->SetType(anElement->GetType());
                }
              }
            }
            break;
          }
          default: {
            PCellInfo aCellInfo = aMed->GetPCellInfo(aMeshInfo,anEntity,aGeom);
            EBooleen anIsElemNum = takeNumbers ? aCellInfo->IsElemNum() : eFAUX;
            TInt aNbElems = aCellInfo->GetNbElem();
            if(MYDEBUG) MESSAGE("Perform - anEntity = "<<anEntity<<"; anIsElemNum = "<<anIsElemNum);
            if(MYDEBUG) MESSAGE("Perform - aGeom = "<<aGeom<<"; aNbElems = "<<aNbElems);

            TInt aNbNodes = -1;
            switch(aGeom){
            case eSEG2:    aNbNodes = 2;  break;
            case eSEG3:    aNbNodes = 3;  break;
            case eTRIA3:   aNbNodes = 3;  break;
            case eTRIA6:   aNbNodes = 6;  break;
            case eTRIA7:   aNbNodes = 7;  break;
            case eQUAD4:   aNbNodes = 4;  break;
            case eQUAD8:   aNbNodes = 8;  break;
            case eQUAD9:   aNbNodes = 9;  break;
            case eTETRA4:  aNbNodes = 4;  break;
            case eTETRA10: aNbNodes = 10; break;
            case ePYRA5:   aNbNodes = 5;  break;
            case ePYRA13:  aNbNodes = 13; break;
            case ePENTA6:  aNbNodes = 6;  break;
            case ePENTA15: aNbNodes = 15; break;
            case eHEXA8:   aNbNodes = 8;  break;
            case eHEXA20:  aNbNodes = 20; break;
            case eHEXA27:  aNbNodes = 27; break;
            case eOCTA12:  aNbNodes = 12; break;
            case ePOINT1:  aNbNodes = 1;  break;
            default:;
            }
            vector<TInt> aNodeIds(aNbNodes);
            for ( int iElem = 0; iElem < aNbElems; iElem++ )
            {
              bool anIsValidConnect = false;
              TCConnSlice aConnSlice = aCellInfo->GetConnSlice(iElem);
#ifndef _DEXCEPT_
              try{
#endif
#ifdef _EDF_NODE_IDS_
                if(anIsNodeNum)
                  for(int iNode = 0; iNode < aNbNodes; iNode++)
                    aNodeIds[iNode] = aNodeInfo->GetElemNum(aConnSlice[iNode] - 1);
                else
                  for(int iNode = 0; iNode < aNbNodes; iNode++)
                    aNodeIds[iNode] = aConnSlice[iNode];
#else
                for(int iNode = 0; iNode < aNbNodes; iNode++)
                  aNodeIds[iNode] = aConnSlice[iNode];
#endif
                anIsValidConnect = true;
#ifndef _DEXCEPT_
              }catch(const std::exception& exc){
                INFOS("Following exception was caught:\n\t"<<exc.what());
                aResult = addMessage( exc.what(), /*isFatal=*/true );
              }catch(...){
                INFOS("Unknown exception was caught !!!");
                aResult = addMessage( "Unknown exception", /*isFatal=*/true );
              }
#endif          
              if(!anIsValidConnect)
                continue;

              bool isRenum = false;
              const SMDS_MeshElement* anElement = NULL;
              TInt aFamNum = aCellInfo->GetFamNum(iElem);
#ifndef _DEXCEPT_
              try{
#endif
                //MESSAGE("Try to create element # " << iElem << " with id = "
                //        << aCellInfo->GetElemNum(iElem));
                switch(aGeom) {
                case ePOINT1:
                  //anElement = FindNode(myMesh,aNodeIds[0]);
                  if(anIsElemNum)
                    anElement = myMesh->Add0DElementWithID
                      (aNodeIds[0], aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->Add0DElement(FindNode(myMesh,aNodeIds[0]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eSEG2:
                  if(anIsElemNum)
                    anElement = myMesh->AddEdgeWithID(aNodeIds[0],
                                                      aNodeIds[1],
                                                      aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddEdge(FindNode(myMesh,aNodeIds[0]),
                                                FindNode(myMesh,aNodeIds[1]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eSEG3:
                  if(anIsElemNum)
                    anElement = myMesh->AddEdgeWithID(aNodeIds[0],
                                                      aNodeIds[1],
                                                      aNodeIds[2],
                                                      aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddEdge(FindNode(myMesh,aNodeIds[0]),
                                                FindNode(myMesh,aNodeIds[1]),
                                                FindNode(myMesh,aNodeIds[2]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eTRIA3:
                  aNbNodes = 3;
                  if(anIsElemNum)
                    anElement = myMesh->AddFaceWithID(aNodeIds[0],
                                                      aNodeIds[1],
                                                      aNodeIds[2],
                                                      aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddFace(FindNode(myMesh,aNodeIds[0]),
                                                FindNode(myMesh,aNodeIds[1]),
                                                FindNode(myMesh,aNodeIds[2]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eTRIA6:
                  aNbNodes = 6;
                  if(anIsElemNum)
                    anElement = myMesh->AddFaceWithID(aNodeIds[0], aNodeIds[1],
                                                      aNodeIds[2], aNodeIds[3],
                                                      aNodeIds[4], aNodeIds[5],
                                                      aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddFace(FindNode(myMesh,aNodeIds[0]),
                                                FindNode(myMesh,aNodeIds[1]),
                                                FindNode(myMesh,aNodeIds[2]),
                                                FindNode(myMesh,aNodeIds[3]),
                                                FindNode(myMesh,aNodeIds[4]),
                                                FindNode(myMesh,aNodeIds[5]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eTRIA7:
                  aNbNodes = 7;
                  if(anIsElemNum)
                    anElement = myMesh->AddFaceWithID(aNodeIds[0], aNodeIds[1],
                                                      aNodeIds[2], aNodeIds[3],
                                                      aNodeIds[4], aNodeIds[5], aNodeIds[6],
                                                      aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddFace(FindNode(myMesh,aNodeIds[0]),
                                                FindNode(myMesh,aNodeIds[1]),
                                                FindNode(myMesh,aNodeIds[2]),
                                                FindNode(myMesh,aNodeIds[3]),
                                                FindNode(myMesh,aNodeIds[4]),
                                                FindNode(myMesh,aNodeIds[5]),
                                                FindNode(myMesh,aNodeIds[6]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eQUAD4:
                  aNbNodes = 4;
                  if(anIsElemNum)
                    anElement = myMesh->AddFaceWithID(aNodeIds[0], aNodeIds[1],
                                                      aNodeIds[2], aNodeIds[3],
                                                      aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddFace(FindNode(myMesh,aNodeIds[0]),
                                                FindNode(myMesh,aNodeIds[1]),
                                                FindNode(myMesh,aNodeIds[2]),
                                                FindNode(myMesh,aNodeIds[3]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eQUAD8:
                  aNbNodes = 8;
                  if(anIsElemNum)
                    anElement = myMesh->AddFaceWithID(aNodeIds[0], aNodeIds[1],
                                                      aNodeIds[2], aNodeIds[3],
                                                      aNodeIds[4], aNodeIds[5],
                                                      aNodeIds[6], aNodeIds[7],
                                                      aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddFace(FindNode(myMesh,aNodeIds[0]),
                                                FindNode(myMesh,aNodeIds[1]),
                                                FindNode(myMesh,aNodeIds[2]),
                                                FindNode(myMesh,aNodeIds[3]),
                                                FindNode(myMesh,aNodeIds[4]),
                                                FindNode(myMesh,aNodeIds[5]),
                                                FindNode(myMesh,aNodeIds[6]),
                                                FindNode(myMesh,aNodeIds[7]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eQUAD9:
                  aNbNodes = 9;
                  if(anIsElemNum)
                    anElement = myMesh->AddFaceWithID(aNodeIds[0], aNodeIds[1],
                                                      aNodeIds[2], aNodeIds[3],
                                                      aNodeIds[4], aNodeIds[5],
                                                      aNodeIds[6], aNodeIds[7], aNodeIds[8],
                                                      aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddFace(FindNode(myMesh,aNodeIds[0]),
                                                FindNode(myMesh,aNodeIds[1]),
                                                FindNode(myMesh,aNodeIds[2]),
                                                FindNode(myMesh,aNodeIds[3]),
                                                FindNode(myMesh,aNodeIds[4]),
                                                FindNode(myMesh,aNodeIds[5]),
                                                FindNode(myMesh,aNodeIds[6]),
                                                FindNode(myMesh,aNodeIds[7]),
                                                FindNode(myMesh,aNodeIds[8]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eTETRA4:
                  aNbNodes = 4;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0], aNodeIds[1],
                                                        aNodeIds[2], aNodeIds[3],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eTETRA10:
                  aNbNodes = 10;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0], aNodeIds[1],
                                                        aNodeIds[2], aNodeIds[3],
                                                        aNodeIds[4], aNodeIds[5],
                                                        aNodeIds[6], aNodeIds[7],
                                                        aNodeIds[8], aNodeIds[9],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]),
                                                  FindNode(myMesh,aNodeIds[4]),
                                                  FindNode(myMesh,aNodeIds[5]),
                                                  FindNode(myMesh,aNodeIds[6]),
                                                  FindNode(myMesh,aNodeIds[7]),
                                                  FindNode(myMesh,aNodeIds[8]),
                                                  FindNode(myMesh,aNodeIds[9]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case ePYRA5:
                  aNbNodes = 5;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0], aNodeIds[1],
                                                        aNodeIds[2], aNodeIds[3],
                                                        aNodeIds[4],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]),
                                                  FindNode(myMesh,aNodeIds[4]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case ePYRA13:
                  aNbNodes = 13;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0], aNodeIds[1],
                                                        aNodeIds[2], aNodeIds[3],
                                                        aNodeIds[4], aNodeIds[5],
                                                        aNodeIds[6], aNodeIds[7],
                                                        aNodeIds[8], aNodeIds[9],
                                                        aNodeIds[10], aNodeIds[11],
                                                        aNodeIds[12],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]),
                                                  FindNode(myMesh,aNodeIds[4]),
                                                  FindNode(myMesh,aNodeIds[5]),
                                                  FindNode(myMesh,aNodeIds[6]),
                                                  FindNode(myMesh,aNodeIds[7]),
                                                  FindNode(myMesh,aNodeIds[8]),
                                                  FindNode(myMesh,aNodeIds[9]),
                                                  FindNode(myMesh,aNodeIds[10]),
                                                  FindNode(myMesh,aNodeIds[11]),
                                                  FindNode(myMesh,aNodeIds[12]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case ePENTA6:
                  aNbNodes = 6;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0],
                                                        aNodeIds[1],
                                                        aNodeIds[2],
                                                        aNodeIds[3],
                                                        aNodeIds[4],
                                                        aNodeIds[5],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]),
                                                  FindNode(myMesh,aNodeIds[4]),
                                                  FindNode(myMesh,aNodeIds[5]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case ePENTA15:
                  aNbNodes = 15;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0], aNodeIds[1],
                                                        aNodeIds[2], aNodeIds[3],
                                                        aNodeIds[4], aNodeIds[5],
                                                        aNodeIds[6], aNodeIds[7],
                                                        aNodeIds[8], aNodeIds[9],
                                                        aNodeIds[10], aNodeIds[11],
                                                        aNodeIds[12], aNodeIds[13],
                                                        aNodeIds[14],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]),
                                                  FindNode(myMesh,aNodeIds[4]),
                                                  FindNode(myMesh,aNodeIds[5]),
                                                  FindNode(myMesh,aNodeIds[6]),
                                                  FindNode(myMesh,aNodeIds[7]),
                                                  FindNode(myMesh,aNodeIds[8]),
                                                  FindNode(myMesh,aNodeIds[9]),
                                                  FindNode(myMesh,aNodeIds[10]),
                                                  FindNode(myMesh,aNodeIds[11]),
                                                  FindNode(myMesh,aNodeIds[12]),
                                                  FindNode(myMesh,aNodeIds[13]),
                                                  FindNode(myMesh,aNodeIds[14]));
                    isRenum = anIsElemNum;
                  }
                  break;
                case eHEXA8:
                  aNbNodes = 8;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0],
                                                        aNodeIds[1],
                                                        aNodeIds[2],
                                                        aNodeIds[3],
                                                        aNodeIds[4],
                                                        aNodeIds[5],
                                                        aNodeIds[6],
                                                        aNodeIds[7],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]),
                                                  FindNode(myMesh,aNodeIds[4]),
                                                  FindNode(myMesh,aNodeIds[5]),
                                                  FindNode(myMesh,aNodeIds[6]),
                                                  FindNode(myMesh,aNodeIds[7]));
                    isRenum = anIsElemNum;
                  }
                  break;

                case eHEXA20:
                  aNbNodes = 20;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0], aNodeIds[1],
                                                        aNodeIds[2], aNodeIds[3],
                                                        aNodeIds[4], aNodeIds[5],
                                                        aNodeIds[6], aNodeIds[7],
                                                        aNodeIds[8], aNodeIds[9],
                                                        aNodeIds[10], aNodeIds[11],
                                                        aNodeIds[12], aNodeIds[13],
                                                        aNodeIds[14], aNodeIds[15],
                                                        aNodeIds[16], aNodeIds[17],
                                                        aNodeIds[18], aNodeIds[19],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]),
                                                  FindNode(myMesh,aNodeIds[4]),
                                                  FindNode(myMesh,aNodeIds[5]),
                                                  FindNode(myMesh,aNodeIds[6]),
                                                  FindNode(myMesh,aNodeIds[7]),
                                                  FindNode(myMesh,aNodeIds[8]),
                                                  FindNode(myMesh,aNodeIds[9]),
                                                  FindNode(myMesh,aNodeIds[10]),
                                                  FindNode(myMesh,aNodeIds[11]),
                                                  FindNode(myMesh,aNodeIds[12]),
                                                  FindNode(myMesh,aNodeIds[13]),
                                                  FindNode(myMesh,aNodeIds[14]),
                                                  FindNode(myMesh,aNodeIds[15]),
                                                  FindNode(myMesh,aNodeIds[16]),
                                                  FindNode(myMesh,aNodeIds[17]),
                                                  FindNode(myMesh,aNodeIds[18]),
                                                  FindNode(myMesh,aNodeIds[19]));
                    isRenum = anIsElemNum;
                  }
                  break;

                case eHEXA27:
                  aNbNodes = 27;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0], aNodeIds[1],
                                                        aNodeIds[2], aNodeIds[3],
                                                        aNodeIds[4], aNodeIds[5],
                                                        aNodeIds[6], aNodeIds[7],
                                                        aNodeIds[8], aNodeIds[9],
                                                        aNodeIds[10], aNodeIds[11],
                                                        aNodeIds[12], aNodeIds[13],
                                                        aNodeIds[14], aNodeIds[15],
                                                        aNodeIds[16], aNodeIds[17],
                                                        aNodeIds[18], aNodeIds[19],
                                                        aNodeIds[20], aNodeIds[21],
                                                        aNodeIds[22], aNodeIds[23],
                                                        aNodeIds[24], aNodeIds[25],
                                                        aNodeIds[26],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]),
                                                  FindNode(myMesh,aNodeIds[4]),
                                                  FindNode(myMesh,aNodeIds[5]),
                                                  FindNode(myMesh,aNodeIds[6]),
                                                  FindNode(myMesh,aNodeIds[7]),
                                                  FindNode(myMesh,aNodeIds[8]),
                                                  FindNode(myMesh,aNodeIds[9]),
                                                  FindNode(myMesh,aNodeIds[10]),
                                                  FindNode(myMesh,aNodeIds[11]),
                                                  FindNode(myMesh,aNodeIds[12]),
                                                  FindNode(myMesh,aNodeIds[13]),
                                                  FindNode(myMesh,aNodeIds[14]),
                                                  FindNode(myMesh,aNodeIds[15]),
                                                  FindNode(myMesh,aNodeIds[16]),
                                                  FindNode(myMesh,aNodeIds[17]),
                                                  FindNode(myMesh,aNodeIds[18]),
                                                  FindNode(myMesh,aNodeIds[19]),
                                                  FindNode(myMesh,aNodeIds[20]),
                                                  FindNode(myMesh,aNodeIds[21]),
                                                  FindNode(myMesh,aNodeIds[22]),
                                                  FindNode(myMesh,aNodeIds[23]),
                                                  FindNode(myMesh,aNodeIds[24]),
                                                  FindNode(myMesh,aNodeIds[25]),
                                                  FindNode(myMesh,aNodeIds[26]));
                    isRenum = anIsElemNum;
                  }
                  break;

                case eOCTA12:
                  aNbNodes = 12;
                  if(anIsElemNum)
                    anElement = myMesh->AddVolumeWithID(aNodeIds[0], aNodeIds[1],
                                                        aNodeIds[2], aNodeIds[3],
                                                        aNodeIds[4], aNodeIds[5],
                                                        aNodeIds[6], aNodeIds[7],
                                                        aNodeIds[8], aNodeIds[9],
                                                        aNodeIds[10], aNodeIds[11],
                                                        aCellInfo->GetElemNum(iElem));
                  if (!anElement) {
                    anElement = myMesh->AddVolume(FindNode(myMesh,aNodeIds[0]),
                                                  FindNode(myMesh,aNodeIds[1]),
                                                  FindNode(myMesh,aNodeIds[2]),
                                                  FindNode(myMesh,aNodeIds[3]),
                                                  FindNode(myMesh,aNodeIds[4]),
                                                  FindNode(myMesh,aNodeIds[5]),
                                                  FindNode(myMesh,aNodeIds[6]),
                                                  FindNode(myMesh,aNodeIds[7]),
                                                  FindNode(myMesh,aNodeIds[8]),
                                                  FindNode(myMesh,aNodeIds[9]),
                                                  FindNode(myMesh,aNodeIds[10]),
                                                  FindNode(myMesh,aNodeIds[11]));
                    isRenum = anIsElemNum;
                  }
                  break;

                default:;

                } // switch(aGeom)

#ifndef _DEXCEPT_
              } catch(const std::exception& exc) {
                INFOS("The following exception was caught:\n\t"<<exc.what());
                aResult = addMessage( exc.what(), /*isFatal=*/true );
              } catch(...) {
                INFOS("Unknown exception was caught !!!");
                aResult = addMessage( "Unknown exception", /*isFatal=*/true );
              }
#endif
              if (!anElement) {
                aResult = DRS_WARN_SKIP_ELEM;
              }
              else {
                if (isRenum) {
                  anIsElemNum = eFAUX;
                  takeNumbers = false;
                  if (aResult < DRS_WARN_RENUMBER)
                    aResult = DRS_WARN_RENUMBER;
                }
                if ( DriverMED::checkFamilyID ( aFamily, aFamNum, myFamilies )) {
                  // Save reference to this element from its family
                  aFamily->AddElement(anElement);
                  aFamily->SetType(anElement->GetType());
                }
              }
            } // loop on aNbElems
          }} // switch(aGeom)
        } // loop on aGeom2Size
      } // loop on aEntityInfo

      if (aDescendingEntitiesMap.Extent()) isDescConn = true; // Mantis issue 0020483

    } // for(int iMesh = 0; iMesh < aNbMeshes; iMesh++)
#ifndef _DEXCEPT_
  }
  catch(const std::exception& exc)
  {
    INFOS("The following exception was caught:\n\t"<<exc.what());
    aResult = addMessage( exc.what(), /*isFatal=*/true );
  }
  catch(...)
  {
    INFOS("Unknown exception was caught !!!");
    aResult = addMessage( "Unknown exception", /*isFatal=*/true );
  }
#endif
  if (myMesh)
    myMesh->compactMesh();

  // Mantis issue 0020483
  if (aResult == DRS_OK && isDescConn) {
    INFOS("There are some elements in descending connectivity in med file. They were not read !!!");
    aResult = DRS_WARN_DESCENDING;
  }

  if(MYDEBUG) MESSAGE("Perform - aResult status = "<<aResult);
  return aResult;
}

list<string> DriverMED_R_SMESHDS_Mesh::GetMeshNames(Status& theStatus)
{
  list<string> aMeshNames;

  try {
    if(MYDEBUG) MESSAGE("GetMeshNames - myFile : " << myFile);
    theStatus = DRS_OK;
    PWrapper aMed = CrWrapper(myFile);

    if (TInt aNbMeshes = aMed->GetNbMeshes()) {
      for (int iMesh = 0; iMesh < aNbMeshes; iMesh++) {
        // Reading the MED mesh
        //---------------------
        PMeshInfo aMeshInfo = aMed->GetPMeshInfo(iMesh+1);
        aMeshNames.push_back(aMeshInfo->GetName());
      }
    }
  } catch(const std::exception& exc) {
    INFOS("Following exception was caught:\n\t"<<exc.what());
    theStatus = DRS_FAIL;
  } catch(...) {
    INFOS("Unknown exception was caught !!!");
    theStatus = DRS_FAIL;
  }

  return aMeshNames;
}

list<TNameAndType> DriverMED_R_SMESHDS_Mesh::GetGroupNamesAndTypes()
{
  list<TNameAndType> aResult;
  set<TNameAndType> aResGroupNames;

  map<int, DriverMED_FamilyPtr>::iterator aFamsIter = myFamilies.begin();
  for (; aFamsIter != myFamilies.end(); aFamsIter++)
  {
    DriverMED_FamilyPtr aFamily = (*aFamsIter).second;
    const MED::TStringSet& aGroupNames = aFamily->GetGroupNames();
    set<string>::const_iterator aGrNamesIter = aGroupNames.begin();
    for (; aGrNamesIter != aGroupNames.end(); aGrNamesIter++)
    {
      const set< SMDSAbs_ElementType >& types = aFamily->GetTypes();
      set< SMDSAbs_ElementType >::const_iterator type = types.begin();
      for ( ; type != types.end(); ++type )
      {
        TNameAndType aNameAndType = make_pair( *aGrNamesIter, *type );
        if ( aResGroupNames.insert( aNameAndType ).second ) {
          aResult.push_back( aNameAndType );
        }
      }
    }
  }

  return aResult;
}

void DriverMED_R_SMESHDS_Mesh::GetGroup(SMESHDS_Group* theGroup)
{
  string aGroupName (theGroup->GetStoreName());
  if(MYDEBUG) MESSAGE("Get Group " << aGroupName);

  map<int, DriverMED_FamilyPtr>::iterator aFamsIter = myFamilies.begin();
  for (; aFamsIter != myFamilies.end(); aFamsIter++)
  {
    DriverMED_FamilyPtr aFamily = (*aFamsIter).second;
    if (aFamily->GetTypes().count( theGroup->GetType() ) && aFamily->MemberOf(aGroupName))
    {
      const ElementsSet&           anElements = aFamily->GetElements();
      ElementsSet::const_iterator anElemsIter = anElements.begin();
      for (; anElemsIter != anElements.end(); anElemsIter++)
      {
        const SMDS_MeshElement * element = *anElemsIter;
        if ( element->GetType() == theGroup->GetType() ) // Issue 0020576
          theGroup->SMDSGroup().Add(element);
      }
      int aGroupAttrVal = aFamily->GetGroupAttributVal();
      if( aGroupAttrVal != 0)
        theGroup->SetColorGroup(aGroupAttrVal);
//       if ( element ) -- Issue 0020576
//         theGroup->SetType( theGroup->SMDSGroup().GetType() );
    }
  }
}

void DriverMED_R_SMESHDS_Mesh::GetSubMesh (SMESHDS_SubMesh* theSubMesh,
                                           const int theId)
{
  char submeshGrpName[ 30 ];
  sprintf( submeshGrpName, "SubMesh %d", theId );
  string aName (submeshGrpName);
  map<int, DriverMED_FamilyPtr>::iterator aFamsIter = myFamilies.begin();
  for (; aFamsIter != myFamilies.end(); aFamsIter++)
  {
    DriverMED_FamilyPtr aFamily = (*aFamsIter).second;
    if (aFamily->MemberOf(aName))
    {
      const ElementsSet&           anElements = aFamily->GetElements();
      ElementsSet::const_iterator anElemsIter = anElements.begin();
      if (aFamily->GetType() == SMDSAbs_Node)
      {
        for (; anElemsIter != anElements.end(); anElemsIter++)
        {
          const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>(*anElemsIter);
          theSubMesh->AddNode(node);
        }
      }
      else
      {
        for (; anElemsIter != anElements.end(); anElemsIter++)
        {
          theSubMesh->AddElement(*anElemsIter);
        }
      }
    }
  }
}

void DriverMED_R_SMESHDS_Mesh::CreateAllSubMeshes ()
{
  map<int, DriverMED_FamilyPtr>::iterator aFamsIter = myFamilies.begin();
  for (; aFamsIter != myFamilies.end(); aFamsIter++)
  {
    DriverMED_FamilyPtr aFamily = (*aFamsIter).second;
    MED::TStringSet aGroupNames = aFamily->GetGroupNames();
    set<string>::iterator aGrNamesIter = aGroupNames.begin();
    for (; aGrNamesIter != aGroupNames.end(); aGrNamesIter++)
    {
      string aName = *aGrNamesIter;
      // Check, if this is a Group or SubMesh name
      if (aName.substr(0, 7) == string("SubMesh"))
      {
        int Id = atoi(string(aName).substr(7).c_str());
        const ElementsSet&           anElements = aFamily->GetElements();
        ElementsSet::const_iterator anElemsIter = anElements.begin();
        if (aFamily->GetType() == SMDSAbs_Node)
        {
          for (; anElemsIter != anElements.end(); anElemsIter++)
          {
            const SMDS_MeshNode* node = static_cast<const SMDS_MeshNode*>( *anElemsIter );
            // find out a shape type
            TopoDS_Shape aShape = myMesh->IndexToShape( Id );
            int aShapeType = ( aShape.IsNull() ? -1 : aShape.ShapeType() );
            switch ( aShapeType ) {
            case TopAbs_FACE:
              myMesh->SetNodeOnFace(node, Id); break;
            case TopAbs_EDGE:
              myMesh->SetNodeOnEdge(node, Id); break;
            case TopAbs_VERTEX:
              myMesh->SetNodeOnVertex(node, Id); break;
            default:
              myMesh->SetNodeInVolume(node, Id);
            }
          }
        }
        else
        {
          for (; anElemsIter != anElements.end(); anElemsIter++)
          {
            myMesh->SetMeshElementOnShape(*anElemsIter, Id);
          }
        }
      }
    }
  }
}
/*!
 * \brief Ensure aFamily to have required ID
 * \param aFamily - a family to check and update
 * \param anID - an ID aFamily should have
 * \retval bool  - true if successful
 */
bool DriverMED::checkFamilyID(DriverMED_FamilyPtr & aFamily,
                              int                   anID,
                              const TID2FamilyMap&  myFamilies)
{
  if ( !aFamily || aFamily->GetId() != anID ) {
    map<int, DriverMED_FamilyPtr>::const_iterator i_fam = myFamilies.find(anID);
    if ( i_fam == myFamilies.end() )
      return false;
    aFamily = i_fam->second;
  }
  return ( aFamily->GetId() == anID );
}

/*!
 * \brief Reading the structured mesh and convert to non structured
 *        (by filling of smesh structure for non structured mesh)
 * \param theWrapper  - PWrapper const pointer
 * \param theMeshInfo - PMeshInfo const pointer
 * \param myFamilies  - a map of the family ID to the Family
 * \return TRUE, if successfully. Else FALSE
 */
bool DriverMED::buildMeshGrille(const MED::PWrapper&  theWrapper,
                                const MED::PMeshInfo& theMeshInfo,
                                SMESHDS_Mesh*         myMesh,
                                const TID2FamilyMap&  myFamilies)
{
  bool res = true;

  MED::PGrilleInfo aGrilleInfo = theWrapper->GetPGrilleInfo(theMeshInfo);
  MED::TInt aNbNodes = aGrilleInfo->GetNbNodes();
  MED::TInt aNbCells = aGrilleInfo->GetNbCells();
  MED::TInt aMeshDim = theMeshInfo->GetDim();
  DriverMED_FamilyPtr aFamily;
  for(MED::TInt iNode=0;iNode < aNbNodes; iNode++){
    double aCoords[3] = {0.0, 0.0, 0.0};
    const SMDS_MeshNode* aNode;
    MED::TNodeCoord aMEDNodeCoord = aGrilleInfo->GetCoord(iNode);
    for(MED::TInt iDim=0;iDim<aMeshDim;iDim++)
      aCoords[(int)iDim] = aMEDNodeCoord[(int)iDim];
    aNode = myMesh->AddNodeWithID(aCoords[0],aCoords[1],aCoords[2],iNode+1);
    if (!aNode) {
      EXCEPTION(runtime_error,"buildMeshGrille Error. Node not created! "<<(int)iNode);
    }

    if((aGrilleInfo->myFamNumNode).size() > 0){
      TInt aFamNum = aGrilleInfo->GetFamNumNode(iNode);
      if ( DriverMED::checkFamilyID ( aFamily, aFamNum, myFamilies ))
        {
          aFamily->AddElement(aNode);
          aFamily->SetType(SMDSAbs_Node);
        }
    }
    
  }

  SMDS_MeshElement* anElement = NULL;
  MED::TIntVector aNodeIds;
  for(MED::TInt iCell=0;iCell < aNbCells; iCell++){
    aNodeIds = aGrilleInfo->GetConn(iCell);
    switch(aGrilleInfo->GetGeom()){
    case MED::eSEG2:
      if(aNodeIds.size() != 2){
        res = false;
        EXCEPTION(runtime_error,"buildMeshGrille Error. Incorrect size of ids 2!="<<aNodeIds.size());
      }
      anElement = myMesh->AddEdgeWithID(aNodeIds[0]+1,
                                        aNodeIds[1]+1,
                                        iCell+1);
      break;
    case MED::eQUAD4:
      if(aNodeIds.size() != 4){
        res = false;
        EXCEPTION(runtime_error,"buildMeshGrille Error. Incorrect size of ids 4!="<<aNodeIds.size());
      }
      anElement = myMesh->AddFaceWithID(aNodeIds[0]+1,
                                        aNodeIds[2]+1,
                                        aNodeIds[3]+1,
                                        aNodeIds[1]+1,
                                        iCell+1);
      break;
    case MED::eHEXA8:
      if(aNodeIds.size() != 8){
        res = false;
        EXCEPTION(runtime_error,"buildMeshGrille Error. Incorrect size of ids 8!="<<aNodeIds.size());
      }
      anElement = myMesh->AddVolumeWithID(aNodeIds[0]+1,
                                          aNodeIds[2]+1,
                                          aNodeIds[3]+1,
                                          aNodeIds[1]+1,
                                          aNodeIds[4]+1,
                                          aNodeIds[6]+1,
                                          aNodeIds[7]+1,
                                          aNodeIds[5]+1,
                                          iCell+1);
      break;
    default:
      break;
    }
    if (!anElement) {
      EXCEPTION(runtime_error,"buildMeshGrille Error. Element not created! "<<iCell);
    }
    if((aGrilleInfo->myFamNum).size() > 0){
      TInt aFamNum = aGrilleInfo->GetFamNum(iCell);
      if ( DriverMED::checkFamilyID ( aFamily, aFamNum, myFamilies )){
        aFamily->AddElement(anElement);
        aFamily->SetType(anElement->GetType());
      }
    }
  }

  return res;
}
