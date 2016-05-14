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
//  File   : MED_Wrapper.cxx
//  Author : Alexey PETROV
//
#include "MED_Wrapper.hxx"
#include "MED_Utilities.hxx"
 
#include <boost/version.hpp>

#ifdef _DEBUG_
static int MYDEBUG = 0;
static int MYVALUEDEBUG = 0;
#else
// static int MYDEBUG = 0;
// static int MYVALUEDEBUG = 0;
#endif

namespace MED
{
  TLockProxy
  ::TLockProxy(TWrapper* theWrapper):
    myWrapper(theWrapper)
  {
#if BOOST_VERSION >= 103500
    myWrapper->myMutex.lock();
#else
    boost::detail::thread::lock_ops<TWrapper::TMutex>::lock(myWrapper->myMutex);
#endif
    INITMSG(MYDEBUG,"TLockProxy() - this -"<<this<<"; myWrapper = "<<myWrapper<<std::endl);
  }
  
  TLockProxy
  ::~TLockProxy()
  {
    INITMSG(MYDEBUG,"~TLockProxy() - this -"<<this<<"; myWrapper = "<<myWrapper<<std::endl);
#if BOOST_VERSION >= 103500
    myWrapper->myMutex.unlock();
#else
    boost::detail::thread::lock_ops<TWrapper::TMutex>::unlock(myWrapper->myMutex);
#endif
  }
  
  TWrapper*
  TLockProxy
  ::operator-> () const // never throws
  {
    return myWrapper;
  }

  //----------------------------------------------------------------------------
  TWrapper::~TWrapper()
  {
  }

  //----------------------------------------------------------------------------
  PMeshInfo
  TWrapper
  ::GetPMeshInfo(TInt theId,
                 TErr* theErr)
  {
    PMeshInfo anInfo = CrMeshInfo();
    GetMeshInfo(theId,*anInfo,theErr);
    return anInfo;
  }


  //----------------------------------------------------------------------------
  PFamilyInfo 
  TWrapper
  ::GetPFamilyInfo(const PMeshInfo& theMeshInfo, 
                   TInt theId,
                   TErr* theErr)
  {
    // must be reimplemented in connection with mesh type eSTRUCTURE
    //     if(theMeshInfo->GetType() != eNON_STRUCTURE)
    //       return PFamilyInfo();
    
    TInt aNbAttr = GetNbFamAttr(theId,*theMeshInfo);
    TInt aNbGroup = GetNbFamGroup(theId,*theMeshInfo);
    PFamilyInfo anInfo = CrFamilyInfo(theMeshInfo,aNbGroup,aNbAttr);
    GetFamilyInfo(theId,*anInfo,theErr);

#ifdef _DEBUG_
    std::string aName = anInfo->GetName();
    INITMSG(MYDEBUG,"GetPFamilyInfo - aFamilyName = '"<<aName<<
            "'; andId = "<<anInfo->GetId()<<
            "; aNbAttr = "<<aNbAttr<<
            "; aNbGroup = "<<aNbGroup<<"\n");
    for(TInt iGroup = 0; iGroup < aNbGroup; iGroup++){
      aName = anInfo->GetGroupName(iGroup);
      INITMSG(MYDEBUG,"aGroupName = '"<<aName<<"'\n");
    }
#endif
    
    return anInfo;
  }


  //----------------------------------------------------------------------------
  PNodeInfo
  TWrapper
  ::GetPNodeInfo(const PMeshInfo& theMeshInfo,
                 TErr* theErr)
  {
    TInt aNbElems = GetNbNodes(*theMeshInfo);
    if(aNbElems == 0){
      return PNodeInfo();
    }

    PNodeInfo anInfo = CrNodeInfo(theMeshInfo,aNbElems);
    GetNodeInfo(*anInfo,theErr);

#ifdef _DEBUG_
    TInt aDim = theMeshInfo->myDim;
    TInt aNbElem = anInfo->GetNbElem();
    INITMSG(MYDEBUG,"GetPNodeInfo: ");
    {
      INITMSG(MYDEBUG,"aCoords: "<<aNbElem<<": ");
      TNodeCoord& aCoord = anInfo->myCoord;
      for(TInt iElem = 0; iElem < aNbElem; iElem++){
        for(TInt iDim = 0, anId = iElem*aDim; iDim < aDim; iDim++, anId++){
          ADDMSG(MYVALUEDEBUG,aCoord[anId]<<",");
        }
        ADDMSG(MYVALUEDEBUG," ");
      }
      ADDMSG(MYDEBUG, std::endl);
      
      BEGMSG(MYVALUEDEBUG, "GetFamNum: ");
      for(TInt iElem = 0; iElem < aNbElem; iElem++){
        ADDMSG(MYVALUEDEBUG,anInfo->GetFamNum(iElem)<<", ");
      }
      ADDMSG(MYVALUEDEBUG, std::endl);
      
      if(anInfo->IsElemNum()){
        BEGMSG(MYVALUEDEBUG,"GetElemNum: ");
        for(TInt iElem = 0; iElem < aNbElem; iElem++){
          ADDMSG(MYVALUEDEBUG,anInfo->GetElemNum(iElem)<<", ");
        }
        ADDMSG(MYVALUEDEBUG, std::endl);
      }
    }
    ADDMSG(MYDEBUG, std::endl);
#endif
    
    return anInfo;
  }

  //----------------------------------------------------------------------------
  PPolygoneInfo
  TWrapper
  ::GetPPolygoneInfo(const PMeshInfo& theMeshInfo,
                     EEntiteMaillage theEntity, 
                     EGeometrieElement theGeom, 
                     EConnectivite theConnMode)
  {
    if(theMeshInfo->GetType() != eNON_STRUCTURE)
      return PPolygoneInfo();

    TInt aNbElem = GetNbPolygones(theMeshInfo,theEntity,theGeom,theConnMode);
    TInt aConnSize = GetPolygoneConnSize(theMeshInfo,theEntity,theGeom,theConnMode);
    PPolygoneInfo anInfo = CrPolygoneInfo(theMeshInfo,theEntity,theGeom,aNbElem,aConnSize,theConnMode);
    GetPolygoneInfo(anInfo);

#ifdef _DEBUG_
    INITMSG(MYDEBUG,"GetPPolygoneInfo"<<
            " - theGeom = "<<theGeom<<
            "; aNbElem = "<<aNbElem<<": ");
    for(TInt iElem = 1; iElem < aNbElem; iElem++){
      TCConnSlice aConnSlice = anInfo->GetConnSlice(iElem);
      TInt aConnDim = aConnSlice.size();
      for(TInt iConn = 0; iConn < aConnDim; iConn++){
        ADDMSG(MYVALUEDEBUG,aConnSlice[iConn]<<",");
      }
      ADDMSG(MYDEBUG," ");
    }
    ADDMSG(MYDEBUG, std::endl);
#endif

    return anInfo;
  }
  
  //----------------------------------------------------------------------------
  PPolyedreInfo
  TWrapper
  ::GetPPolyedreInfo(const PMeshInfo& theMeshInfo,
                     EEntiteMaillage theEntity, 
                     EGeometrieElement theGeom, 
                     EConnectivite theConnMode)
  {
    if(theMeshInfo->GetType() != eNON_STRUCTURE)
      return PPolyedreInfo();
    TInt aNbElem  = GetNbPolyedres(theMeshInfo,theEntity,theGeom,theConnMode);
    TInt aNbFaces, aConnSize;
    GetPolyedreConnSize(theMeshInfo,aNbFaces,aConnSize,theConnMode);
    PPolyedreInfo anInfo = CrPolyedreInfo(theMeshInfo,theEntity,theGeom,aNbElem,aNbFaces,aConnSize,theConnMode);
    GetPolyedreInfo(anInfo);

#ifdef _DEBUG_
    INITMSG(MYDEBUG,"GetPPolyedreInfo"<<
            " - theGeom = "<<theGeom<<
            "; aNbElem = "<<aNbElem<<": ");
    for(TInt iElem = 0; iElem < aNbElem; iElem++){
      TCConnSliceArr aConnSliceArr = anInfo->GetConnSliceArr(iElem);
      TInt aNbFaces = aConnSliceArr.size();
      ADDMSG(MYDEBUG,"{");
      for(TInt iFace = 0; iFace < aNbFaces; iFace++){
        TCConnSlice aConnSlice = aConnSliceArr[iFace];
        TInt aNbConn = aConnSlice.size();
        ADDMSG(MYDEBUG,"[");
        for(TInt iConn = 0; iConn < aNbConn; iConn++){
          ADDMSG(MYVALUEDEBUG,aConnSlice[iConn]<<",");
        }
        ADDMSG(MYDEBUG,"] ");
      }
      ADDMSG(MYDEBUG,"} ");
    }
    ADDMSG(MYDEBUG, std::endl);
#endif

    return anInfo;
  }
  
  //----------------------------------------------------------------------------
  PElemInfo 
  TWrapper
  ::GetPElemInfo(const PMeshInfo& theMeshInfo,
                 EEntiteMaillage theEntity, 
                 EGeometrieElement theGeom, 
                 EConnectivite theConnMode,
                 TErr* theErr)
  {
    EMaillage aType = theMeshInfo->GetType();
    if(aType == eNON_STRUCTURE){
      switch(theGeom){
      case ePOINT1:
        if(theEntity == eNOEUD)
          return GetPNodeInfo(theMeshInfo,theErr);
        return GetPCellInfo(theMeshInfo,theEntity,theGeom,theConnMode,theErr);
        break;
      case ePOLYGONE:
        return GetPPolygoneInfo(theMeshInfo,theEntity,theGeom,theConnMode);
        break;
      case ePOLYEDRE:
        return GetPPolyedreInfo(theMeshInfo,theEntity,theGeom,theConnMode);
        break;
      default:
        return GetPCellInfo(theMeshInfo,theEntity,theGeom,theConnMode,theErr);
      }
    } else {
      PGrilleInfo aGrille = GetPGrilleInfo(theMeshInfo);

      TInt nbElems;
      EBooleen theIsElemNum = eFAUX;
      // nodes
      switch(theGeom){
      case ePOINT1:
        nbElems = aGrille->GetNbNodes();
        theIsElemNum = eVRAI;
        break;
      case eSEG2:
      case eQUAD4:
      case eHEXA8:
        nbElems = aGrille->GetNbCells();
        break;
      default:
        nbElems = 0;
      }
      
      TIntVector aFamNum;
      TIntVector aElemNum;
      TStringVector aElemNames;
      
      PElemInfo aElemInfo;

      if(theGeom == ePOINT1){
        aElemInfo = CrElemInfo(theMeshInfo,
                               nbElems,
                               theIsElemNum);
        MED::TElemInfo &aTElemInfo = *aElemInfo;

        // must be reimplemente in connection with mesh type eSTRUCTURE
//      GetNumeration(aTElemInfo,
//                    nbElems,
//                    theEntity,
//                    theGeom,
//                    theErr);
        
        GetFamilies(aTElemInfo,
                    nbElems,
                    theEntity,
                    theGeom,
                    theErr);
        
        // must be reimplemente in connection with mesh type eSTRUCTURE
//      GetNames(aTElemInfo,
//               nbElems,
//               theEntity,
//               theGeom,
//               theErr);
      } else {
        aElemInfo = CrElemInfo(theMeshInfo,
                               nbElems,
                               aFamNum,
                               aElemNum,
                               aElemNames);
      }
      
      return aElemInfo;
    }
    return PElemInfo();
  }


  //----------------------------------------------------------------------------
  PCellInfo 
  TWrapper
  ::GetPCellInfo(const PMeshInfo& theMeshInfo,
                 EEntiteMaillage theEntity, 
                 EGeometrieElement theGeom, 
                 EConnectivite theConnMode,
                 TErr* theErr)
  {
    if(theMeshInfo->GetType() != eNON_STRUCTURE)
      return PCellInfo();
    TInt aNbElem = GetNbCells(theMeshInfo,theEntity,theGeom,theConnMode);
    PCellInfo anInfo = CrCellInfo(theMeshInfo,theEntity,theGeom,aNbElem,theConnMode);
    GetCellInfo(anInfo,theErr);

#ifdef _DEBUG_
    TInt aConnDim = anInfo->GetConnDim();
    INITMSG(MYDEBUG,"GetPCellInfo - theEntity = "<<theEntity<<"; theGeom = "<<theGeom<<"; aConnDim: "<<aConnDim<<"\n");
    BEGMSG(MYDEBUG,"GetPCellInfo - aNbElem: "<<aNbElem<<": ");
    for(TInt iElem = 0; iElem < aNbElem; iElem++){
      TCConnSlice aConnSlice = anInfo->GetConnSlice(iElem);
      for(TInt iConn = 0; iConn < aConnDim; iConn++){
        ADDMSG(MYVALUEDEBUG,aConnSlice[iConn]<<",");
      }
      ADDMSG(MYVALUEDEBUG," ");
    }
    ADDMSG(MYDEBUG, std::endl);

    BEGMSG(MYVALUEDEBUG,"GetPCellInfo - GetFamNum: ");
    for(TInt iElem = 0; iElem < aNbElem; iElem++){
      ADDMSG(MYVALUEDEBUG,anInfo->GetFamNum(iElem)<<", ");
    }
    ADDMSG(MYVALUEDEBUG, std::endl);

    if(anInfo->IsElemNum()){
      BEGMSG(MYVALUEDEBUG,"GetPCellInfo - GetElemNum: ");
      for(TInt iElem = 0; iElem < aNbElem; iElem++){
        ADDMSG(MYVALUEDEBUG,anInfo->GetElemNum(iElem)<<", ");
      }
      ADDMSG(MYVALUEDEBUG, std::endl);
    }
    ADDMSG(MYDEBUG, std::endl);
#endif
    
    return anInfo;
  }

  //----------------------------------------------------------------------------
  //! Read a MEDWrapped representation of MED Balls from the MED file
  PBallInfo
  TWrapper
  ::GetPBallInfo(const PMeshInfo& theMeshInfo)
  {
    TInt nbBalls = GetNbBalls(theMeshInfo);
    if ( nbBalls < 1 ) return PBallInfo();

    PBallInfo anInfo = CrBallInfo( theMeshInfo, nbBalls );
    GetBallInfo(anInfo);

    return anInfo;
  }
  //----------------------------------------------------------------------------
  PFieldInfo
  TWrapper
  ::GetPFieldInfo(const PMeshInfo& theMeshInfo, 
                  TInt theId,
                  TErr* theErr)
  {
    TInt aNbComp = GetNbComp(theId);
    PFieldInfo anInfo = CrFieldInfo(theMeshInfo,aNbComp);
    GetFieldInfo(theId,*anInfo,theErr);

#ifdef _DEBUG_
    INITMSG(MYDEBUG,
            "GetPFieldInfo "<<
            "- aName = '"<<anInfo->GetName()<<"'"<<
            "; aType = "<<anInfo->GetType()<<
            "; aNbComp = "<<aNbComp<<
            std::endl);
#endif
    
    return anInfo;
  }


  //----------------------------------------------------------------------------
  PTimeStampInfo
  TWrapper
  ::GetPTimeStampInfo(const PFieldInfo& theFieldInfo,
                      EEntiteMaillage theEntity,
                      const TGeom2Size& theGeom2Size,
                      TInt theId,
                      TErr* theErr)
  {
    PTimeStampInfo anInfo = CrTimeStampInfo(theFieldInfo,theEntity,theGeom2Size);
    GetTimeStampInfo(theId,*anInfo,theErr);

#ifdef _DEBUG_
    INITMSG(MYDEBUG,"GetPTimeStampInfo - anEntity = "<<anInfo->GetEntity()<<"\n");
    TGeom2NbGauss& aGeom2NbGauss = anInfo->myGeom2NbGauss;
    TGeom2NbGauss::const_iterator anIter = aGeom2NbGauss.begin();
    for(; anIter != aGeom2NbGauss.end(); anIter++){
      const EGeometrieElement& aGeom = anIter->first;
      INITMSG(MYDEBUG,"aGeom = "<<aGeom<<" - "<<aGeom2NbGauss[aGeom]<<";\n");
    }
#endif

    return anInfo;
  }


  //----------------------------------------------------------------------------
  PProfileInfo
  TWrapper
  ::GetPProfileInfo(TInt theId,
                    EModeProfil theMode,
                    TErr* theErr)
  {
    TProfileInfo::TInfo aPreInfo = GetProfilePreInfo(theId);
    PProfileInfo anInfo = CrProfileInfo(aPreInfo,theMode);
    GetProfileInfo(theId,*anInfo,theErr);

    return anInfo;
  }


  //----------------------------------------------------------------------------
  PTimeStampValueBase
  TWrapper
  ::CrTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     const TGeom2Profile& theGeom2Profile,
                     EModeSwitch theMode)
  {
    PFieldInfo aFieldInfo = theTimeStampInfo->GetFieldInfo();
    return CrTimeStampValue(theTimeStampInfo,
                            aFieldInfo->GetType(),
                            theGeom2Profile,
                            theMode);
  }

  //----------------------------------------------------------------------------
  PTimeStampValueBase
  TWrapper
  ::CrTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     const PTimeStampValueBase& theInfo)
  {
    PFieldInfo aFieldInfo = theTimeStampInfo->GetFieldInfo();
    return CrTimeStampValue(theTimeStampInfo,
                            theInfo,
                            aFieldInfo->GetType());
  }

  //----------------------------------------------------------------------------
  template<class TimeStampValueType>
  void
  Print(SharedPtr<TimeStampValueType> theTimeStampValue)
  {
    INITMSG(MYDEBUG,"Print - TimeStampValue\n");
    typename TimeStampValueType::TTGeom2Value& aGeom2Value = theTimeStampValue->myGeom2Value;
    typename TimeStampValueType::TTGeom2Value::const_iterator anIter = aGeom2Value.begin();
    for(; anIter != aGeom2Value.end(); anIter++){
      const EGeometrieElement& aGeom = anIter->first;
      const typename TimeStampValueType::TTMeshValue& aMeshValue = anIter->second;
      TInt aNbElem = aMeshValue.myNbElem;
      TInt aNbGauss = aMeshValue.myNbGauss;
      TInt aNbComp = aMeshValue.myNbComp;
      INITMSG(MYDEBUG,"aGeom = "<<aGeom<<" - "<<aNbElem<<": ");
      for(TInt iElem = 0; iElem < aNbElem; iElem++){
        typename TimeStampValueType::TTMeshValue::TCValueSliceArr aValueSliceArr = aMeshValue.GetGaussValueSliceArr(iElem);
        ADDMSG(MYVALUEDEBUG,"{");
        for(TInt iGauss = 0; iGauss < aNbGauss; iGauss++){
          const typename TimeStampValueType::TTMeshValue::TCValueSlice& aValueSlice = aValueSliceArr[iGauss];
          for(TInt iComp = 0; iComp < aNbComp; iComp++){
            ADDMSG(MYVALUEDEBUG,aValueSlice[iComp]<<" ");
          }
          ADDMSG(MYVALUEDEBUG,"| ");
        }
        ADDMSG(MYVALUEDEBUG,"} ");
      }
      ADDMSG(MYDEBUG,"\n");
    }
  }

  //----------------------------------------------------------------------------
  PTimeStampValueBase 
  TWrapper
  ::GetPTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                       const TMKey2Profile& theMKey2Profile,
                       const TKey2Gauss& theKey2Gauss,
                       TErr* theErr)
  {
    PFieldInfo aFieldInfo = theTimeStampInfo->GetFieldInfo();
    PTimeStampValueBase anInfo = CrTimeStampValue(theTimeStampInfo,
                                                  aFieldInfo->GetType());
    GetTimeStampValue(anInfo, 
                      theMKey2Profile, 
                      theKey2Gauss,
                      theErr);
#ifdef _DEBUG_
    if(aFieldInfo->GetType() == eFLOAT64)
      Print<TFloatTimeStampValue>(anInfo);
    else
      Print<TIntTimeStampValue>(anInfo);
#endif
    return anInfo;
  }

  //----------------------------------------------------------------------------
  void 
  TWrapper
  ::GetTimeStampVal(const PTimeStampVal& theVal,
                    const TMKey2Profile& theMKey2Profile,
                    const TKey2Gauss& theKey2Gauss,
                    TErr* theErr)
  {
    PTimeStampInfo aTimeStampInfo = theVal->GetTimeStampInfo();
    PFieldInfo aFieldInfo = aTimeStampInfo->GetFieldInfo();
    if(aFieldInfo->GetType() == eFLOAT64)
      GetTimeStampValue(theVal,
                        theMKey2Profile,
                        theKey2Gauss,
                        theErr);
    else{
      PTimeStampValueBase aVal = CrTimeStampValue(aTimeStampInfo,
                                                  theVal,
                                                  eINT);
      GetTimeStampValue(aVal,
                        theMKey2Profile,
                        theKey2Gauss,
                        theErr);
      CopyTimeStampValueBase(aVal, theVal);
    }
  }

  //----------------------------------------------------------------------------
  void
  TWrapper
  ::SetTimeStamp(const PTimeStampVal& theVal,
                 TErr* theErr)
  {
    PTimeStampInfo aTimeStampInfo = theVal->GetTimeStampInfo();
    PFieldInfo aFieldInfo = aTimeStampInfo->GetFieldInfo();
    if(aFieldInfo->GetType() == eFLOAT64)
      SetTimeStampValue(theVal, theErr);
    else{
      PTimeStampValueBase aVal = CrTimeStampValue(aTimeStampInfo,
                                                  eINT,
                                                  theVal->GetGeom2Profile(),
                                                  theVal->GetModeSwitch());
      CopyTimeStampValueBase(theVal, aVal);
      SetTimeStampValue(aVal, theErr);
    }
  }

  //----------------------------------------------------------------------------
  PTimeStampVal
  TWrapper
  ::CrTimeStampVal(const PTimeStampInfo& theTimeStampInfo,
                   const TGeom2Profile& theGeom2Profile,
                   EModeSwitch theMode)
  {
    return CrTimeStampValue(theTimeStampInfo,
                            eFLOAT64,
                            theGeom2Profile,
                            theMode);
  }

  //----------------------------------------------------------------------------
  PTimeStampVal
  TWrapper
  ::CrTimeStampVal(const PTimeStampInfo& theTimeStampInfo,
                   const PTimeStampVal& theInfo)
  {
    return CrTimeStampValue(theTimeStampInfo,
                            theInfo,
                            eFLOAT64);
  }

  //----------------------------------------------------------------------------
  PTimeStampVal 
  TWrapper
  ::GetPTimeStampVal(const PTimeStampInfo& theTimeStampInfo,
                     const TMKey2Profile& theMKey2Profile,
                     const TKey2Gauss& theKey2Gauss,
                     TErr* theErr)
  {
    PTimeStampVal anInfo = CrTimeStampVal(theTimeStampInfo);
    GetTimeStampVal(anInfo, 
                    theMKey2Profile, 
                    theKey2Gauss,
                    theErr);
    return anInfo;
  }

  //----------------------------------------------------------------------------
  PGrilleInfo
  TWrapper
  ::GetPGrilleInfo(const PMeshInfo& theMeshInfo)
  {
    if(theMeshInfo->GetType() != eSTRUCTURE)
      return PGrilleInfo();

    EGrilleType type;
    GetGrilleType(*theMeshInfo,type);
    PGrilleInfo anInfo;
    if(type == eGRILLE_STANDARD){
      const TInt nnoeuds = GetNbNodes(*theMeshInfo);
      anInfo = CrGrilleInfo(theMeshInfo,type,nnoeuds);
    }
    else {
      TIntVector aVec;
      aVec.resize(theMeshInfo->GetDim());
      for(int aAxe=0;aAxe<theMeshInfo->GetDim();aAxe++){
        ETable aATable;
        switch(aAxe){
        case 0:
          aATable = eCOOR_IND1;
          break;
        case 1:
          aATable = eCOOR_IND2;
          break;
        case 2:
          aATable = eCOOR_IND3;
          break;
        }
        aVec[aAxe] = GetNbNodes(*theMeshInfo,aATable);
      }
      anInfo = CrGrilleInfo(theMeshInfo,type,aVec);
    }

    GetGrilleInfo(anInfo);
    anInfo->SetGrilleType(type);

#ifdef _DEBUG_
    INITMSG(MYDEBUG,"GetPGrilleInfo: ");
    {
      TInt aNbElem = anInfo->GetNbNodes();
      BEGMSG(MYVALUEDEBUG,"GetFamNumNode: ");
      for(TInt iElem = 0; iElem < aNbElem; iElem++){
        ADDMSG(MYVALUEDEBUG,anInfo->GetFamNumNode(iElem)<<", ");
      }
      TInt aNbCells = anInfo->GetNbCells();
      BEGMSG(MYVALUEDEBUG,"GetFamNum: ");
      for(TInt iElem = 0; iElem < aNbCells; iElem++){
        ADDMSG(MYVALUEDEBUG,anInfo->GetFamNum(iElem)<<", ");
      }
      ADDMSG(MYVALUEDEBUG, std::endl);
      BEGMSG(MYVALUEDEBUG,"GetCoordName: ");
      for(TInt iElem = 0; iElem < theMeshInfo->GetDim(); iElem++){
        ADDMSG(MYVALUEDEBUG,anInfo->GetCoordName(iElem)<<", ");
      }
      ADDMSG(MYVALUEDEBUG, std::endl);
      BEGMSG(MYVALUEDEBUG,"GetCoordUnit: ");
      for(TInt iElem = 0; iElem < theMeshInfo->GetDim(); iElem++){
        ADDMSG(MYVALUEDEBUG,anInfo->GetCoordUnit(iElem)<<", ");
      }
      ADDMSG(MYVALUEDEBUG, std::endl);
      
    }
#endif
    
    return anInfo;
  }
  
  //----------------------------------------------------------------------------
  PGrilleInfo
  TWrapper
  ::GetPGrilleInfo(const PMeshInfo& theMeshInfo,
                   const PGrilleInfo& theInfo)
  {
    PGrilleInfo anInfo = CrGrilleInfo(theMeshInfo,theInfo);
    return anInfo;
  }  
}
