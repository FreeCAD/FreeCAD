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
#ifndef MED_TStructures_HeaderFile
#define MED_TStructures_HeaderFile

#include "MED_Structures.hxx"

#ifdef WIN32
#pragma warning(disable:4250)
#endif

namespace MED
{
  //---------------------------------------------------------------
  //! To provide a common way to handle values of MEDWrapper types as native MED types
  template<class TValue, class TRepresentation>
  struct TValueHolder
  {
    TValue& myValue;
    TRepresentation myRepresentation;

    TValueHolder(TValue& theValue):
      myValue(theValue),
      myRepresentation(TRepresentation(theValue))
    {}

    ~TValueHolder()
    {
      myValue = TValue(myRepresentation);
    }

    TRepresentation*
    operator& ()
    {
      return &myRepresentation;
    }

    operator TRepresentation () const
    {
      return myRepresentation;
    }  

    const TValue&
    operator() () const
    {
      return myValue;
    }  
  };
  
  //! To customize TValueHolder common template definition for TVector
  template<class TVal, class TRepresentation>
  struct TValueHolder<TVector<TVal>, TRepresentation>
  {
    typedef TVector<TVal> TValue;
    TValue& myValue;
    TRepresentation* myRepresentation;

    TValueHolder(TValue& theValue):
      myValue(theValue)
    {
      if(theValue.empty())
        myRepresentation = (TRepresentation*)NULL;
      else
        myRepresentation = (TRepresentation*)&theValue[0];
    }

    TRepresentation*
    operator& ()
    {
      return myRepresentation;
    }
  };
  
  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTNameInfo: virtual TNameInfo
  {
    TTNameInfo(const std::string& theValue)
    {
      myName.resize(GetNOMLength<eVersion>()+1);
      SetName(theValue);
    }

    virtual
    std::string
    GetName() const 
    { 
      return GetString(0, GetNOMLength<eVersion>(), myName);
    }

    virtual
    void
    SetName(const std::string& theValue)
    {
      SetString(0, GetNOMLength<eVersion>(), myName, theValue);
    }

    virtual
    void
    SetName(const TString& theValue)
    {
      SetString(0, GetNOMLength<eVersion>(), myName, theValue);
    }
  };


  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTMeshInfo: 
    virtual TMeshInfo, 
    virtual TTNameInfo<eVersion>
  {
    typedef TTNameInfo<eVersion> TNameInfoBase;

    TTMeshInfo(const PMeshInfo& theInfo):
      TNameInfoBase(theInfo->GetName())
    {
      myDim = theInfo->GetDim();
      mySpaceDim = theInfo->GetSpaceDim();
      myType = theInfo->GetType();
      
      myDesc.resize(GetDESCLength<eVersion>()+1);
      SetDesc(theInfo->GetDesc());
    }

    TTMeshInfo(TInt theDim, TInt theSpaceDim,
               const std::string& theValue,
               EMaillage theType,
               const std::string& theDesc):
      TNameInfoBase(theValue)
    {
      myDim = theDim;
      mySpaceDim = theSpaceDim;
      myType = theType;
      
      myDesc.resize(GetDESCLength<eVersion>()+1);
      SetDesc(theDesc);
    }

    virtual 
    std::string
    GetDesc() const 
    { 
      return GetString(0, GetDESCLength<eVersion>(), myDesc);
    }

    virtual
    void
    SetDesc(const std::string& theValue)
    {
      SetString(0, GetDESCLength<eVersion>(), myDesc, theValue);
    }
  };


  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTFamilyInfo: 
    virtual TFamilyInfo, 
    virtual TTNameInfo<eVersion>
  {
    typedef TTNameInfo<eVersion> TNameInfoBase;

    TTFamilyInfo(const PMeshInfo& theMeshInfo, const PFamilyInfo& theInfo):
      TNameInfoBase(theInfo->GetName())
    {
      myMeshInfo = theMeshInfo;

      myId = theInfo->GetId();

      myNbGroup = theInfo->GetNbGroup();
      myGroupNames.resize(myNbGroup*GetLNOMLength<eVersion>()+1);
      if(myNbGroup){
        for(TInt anId = 0; anId < myNbGroup; anId++){
          SetGroupName(anId,theInfo->GetGroupName(anId));
        }
      }

      myNbAttr = theInfo->GetNbAttr();
      myAttrId.resize(myNbAttr);
      myAttrVal.resize(myNbAttr);
      myAttrDesc.resize(myNbAttr*GetDESCLength<eVersion>()+1);
      if(myNbAttr){
        for(TInt anId = 0; anId < myNbAttr; anId++){
          SetAttrDesc(anId,theInfo->GetAttrDesc(anId));
          myAttrVal[anId] = theInfo->GetAttrVal(anId);
          myAttrId[anId] = theInfo->GetAttrId(anId);
        }
      }
    }

    TTFamilyInfo(const PMeshInfo& theMeshInfo,
                 TInt theNbGroup, 
                 TInt theNbAttr,
                 TInt theId,
                 const std::string& theValue):
      TNameInfoBase(theValue)
    {
      myMeshInfo = theMeshInfo;

      myId = theId;

      myNbGroup = theNbGroup;
      myGroupNames.resize(theNbGroup*GetLNOMLength<eVersion>()+1);

      myNbAttr = theNbAttr;
      myAttrId.resize(theNbAttr);
      myAttrVal.resize(theNbAttr);
      myAttrDesc.resize(theNbAttr*GetDESCLength<eVersion>()+1);
    }

    TTFamilyInfo(const PMeshInfo& theMeshInfo,
                 const std::string& theValue,
                 TInt theId,
                 const TStringSet& theGroupNames, 
                 const TStringVector& theAttrDescs, 
                 const TIntVector& theAttrIds, 
                 const TIntVector& theAttrVals):
      TNameInfoBase(theValue)
    {
      myMeshInfo = theMeshInfo;

      myId = theId;

      myNbGroup = (TInt)theGroupNames.size();
      myGroupNames.resize(myNbGroup*GetLNOMLength<eVersion>()+1);
      if(myNbGroup){
        TStringSet::const_iterator anIter = theGroupNames.begin();
        for(TInt anId = 0; anIter != theGroupNames.end(); anIter++, anId++){
          const std::string& aVal = *anIter;
          SetGroupName(anId,aVal);
        }
      }

      myNbAttr = (TInt)theAttrDescs.size();
      myAttrId.resize(myNbAttr);
      myAttrVal.resize(myNbAttr);
      myAttrDesc.resize(myNbAttr*GetDESCLength<eVersion>()+1);
      if(myNbAttr){
        for(TInt anId = 0, anEnd = (TInt)theAttrDescs.size(); anId < anEnd; anId++){
          SetAttrDesc(anId,theAttrDescs[anId]);
          myAttrVal[anId] = theAttrVals[anId];
          myAttrId[anId] = theAttrIds[anId];
        }
      }
    }

    virtual
    std::string
    GetGroupName(TInt theId) const 
    { 
      return GetString(theId, GetLNOMLength<eVersion>(), myGroupNames);
    }

    virtual
    void
    SetGroupName(TInt theId, const std::string& theValue)
    {
      SetString(theId, GetLNOMLength<eVersion>(), myGroupNames, theValue);
    }

    virtual
    std::string
    GetAttrDesc(TInt theId) const 
    { 
      return GetString(theId, GetDESCLength<eVersion>(), myAttrDesc);
    }

    virtual
    void
    SetAttrDesc(TInt theId, const std::string& theValue)
    {
      SetString(theId, GetDESCLength<eVersion>(), myAttrDesc, theValue);
    }
  };


  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTElemInfo: virtual TElemInfo
  {
    TTElemInfo(const PMeshInfo& theMeshInfo, const PElemInfo& theInfo)
    {
      myMeshInfo = theMeshInfo;
      
      myNbElem = theInfo->GetNbElem();
      myFamNum.reset(new TElemNum(myNbElem));
      myIsFamNum = eFAUX; // is set to eVRAI in SetFamNum()

      myIsElemNum = theInfo->IsElemNum();
      if(theInfo->IsElemNum())
        myElemNum.reset(new TElemNum(myNbElem));
      else
        myElemNum.reset(new TElemNum());

      myIsElemNames = theInfo->IsElemNames();
      if(theInfo->IsElemNames())
        myElemNames.reset(new TString(myNbElem*GetPNOMLength<eVersion>() + 1));
      else
        myElemNames.reset(new TString());

      if(theInfo->GetNbElem()){
        for(TInt anId = 0; anId < myNbElem; anId++){
          SetFamNum(anId, theInfo->GetFamNum(anId));
        }
        if(theInfo->IsElemNum() == eVRAI){
          for(TInt anId = 0; anId < myNbElem; anId++){
            SetElemNum(anId, theInfo->GetElemNum(anId));
          }
        }
        if(theInfo->IsElemNames() == eVRAI){
          for(TInt anId = 0; anId < myNbElem; anId++){
            SetElemName(anId,theInfo->GetElemName(anId));
          }
        }
      }
    }

    TTElemInfo(const PMeshInfo& theMeshInfo, 
               TInt theNbElem,
               EBooleen theIsElemNum,
               EBooleen theIsElemNames)
    {
      myMeshInfo = theMeshInfo;

      myNbElem = theNbElem;
      myFamNum.reset(new TElemNum(theNbElem));
      myIsFamNum = eFAUX; // is set to eVRAI in SetFamNum()

      myIsElemNum = theIsElemNum;
      if(theIsElemNum)
        myElemNum.reset(new TElemNum(theNbElem));
      else
        myElemNum.reset(new TElemNum());

      myIsElemNames = theIsElemNames;
      if(theIsElemNames)
        myElemNames.reset(new TString(theNbElem*GetPNOMLength<eVersion>() + 1));
      else
        myElemNames.reset(new TString());
   }
    
    TTElemInfo(const PMeshInfo& theMeshInfo, 
               TInt theNbElem,
               const TIntVector& theFamilyNums,
               const TIntVector& theElemNums,
               const TStringVector& theElemNames)
    {
      myMeshInfo = theMeshInfo;
      
      myNbElem = theNbElem;
      myFamNum.reset(new TElemNum(theNbElem));
      myIsFamNum = eFAUX; // is set to eVRAI in SetFamNum()
      
      myIsElemNum = theElemNums.size()? eVRAI: eFAUX;
      if(myIsElemNum)
        myElemNum.reset(new TElemNum(theNbElem));
      else
        myElemNum.reset(new TElemNum());
      
      myIsElemNames = theElemNames.size()? eVRAI: eFAUX;
      if(myIsElemNames)
        myElemNames.reset(new TString(theNbElem*GetPNOMLength<eVersion>() + 1));
      else
        myElemNames.reset(new TString());
     
      if(theNbElem){

        if(theFamilyNums.size())
          *myFamNum = theFamilyNums;

        if(myIsElemNum)
          *myElemNum = theElemNums;

        if(myIsElemNames){
          for(TInt anId = 0; anId < theNbElem; anId++){
            const std::string& aVal = theElemNames[anId];
            SetElemName(anId,aVal);
          }
        }
      }
    }

    virtual
    std::string
    GetElemName(TInt theId) const 
    { 
      return GetString(theId,GetPNOMLength<eVersion>(), *myElemNames);
    }

    virtual
    void
    SetElemName(TInt theId, const std::string& theValue)
    {
      SetString(theId,GetPNOMLength<eVersion>(), *myElemNames, theValue);
    }
  };


  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTNodeInfo: 
    virtual TNodeInfo, 
    virtual TTElemInfo<eVersion>
  {
    typedef TTElemInfo<eVersion> TElemInfoBase;

    TTNodeInfo(const PMeshInfo& theMeshInfo, const PNodeInfo& theInfo):
      TNodeInfo(theInfo),
      TElemInfoBase(theMeshInfo, theInfo)
    {
      myModeSwitch = theInfo->GetModeSwitch();
      
      mySystem = theInfo->GetSystem();
      
      myCoord.reset(new TNodeCoord(*theInfo->myCoord));
      
      TInt aSpaceDim = theMeshInfo->GetSpaceDim();

      myCoordNames.resize(aSpaceDim*GetPNOMLength<eVersion>()+1);
      for(TInt anId = 0; anId < aSpaceDim; anId++)
        SetCoordName(anId,theInfo->GetCoordName(anId));
      
      myCoordUnits.resize(aSpaceDim*GetPNOMLength<eVersion>()+1);
      for(TInt anId = 0; anId < aSpaceDim; anId++)
        SetCoordUnit(anId,theInfo->GetCoordUnit(anId));
    }

    TTNodeInfo(const PMeshInfo& theMeshInfo, 
               TInt theNbElem,
               EModeSwitch theMode,
               ERepere theSystem, 
               EBooleen theIsElemNum,
               EBooleen theIsElemNames):
      TModeSwitchInfo(theMode),
      TElemInfoBase(theMeshInfo,
                    theNbElem,
                    theIsElemNum,
                    theIsElemNames)
    {
      mySystem = theSystem;

      myCoord.reset(new TNodeCoord(theNbElem * theMeshInfo->mySpaceDim));

      myCoordUnits.resize(theMeshInfo->mySpaceDim*GetPNOMLength<eVersion>()+1);

      myCoordNames.resize(theMeshInfo->mySpaceDim*GetPNOMLength<eVersion>()+1);
    }

    
    TTNodeInfo(const PMeshInfo& theMeshInfo, 
               const TFloatVector& theNodeCoords,
               EModeSwitch theMode,
               ERepere theSystem, 
               const TStringVector& theCoordNames,
               const TStringVector& theCoordUnits,
               const TIntVector& theFamilyNums,
               const TIntVector& theElemNums,
               const TStringVector& theElemNames):
      TModeSwitchInfo(theMode),
      TElemInfoBase(theMeshInfo,
                    (TInt)theNodeCoords.size()/theMeshInfo->GetDim(),
                    theFamilyNums,
                    theElemNums,
                    theElemNames)
    {
      mySystem = theSystem;

      myCoord.reset(new TNodeCoord(theNodeCoords));
      
      TInt aSpaceDim = theMeshInfo->GetSpaceDim();

      myCoordNames.resize(aSpaceDim*GetPNOMLength<eVersion>()+1);
      if(!theCoordNames.empty())
        for(TInt anId = 0; anId < aSpaceDim; anId++)
          SetCoordName(anId,theCoordNames[anId]);
      
      myCoordUnits.resize(aSpaceDim*GetPNOMLength<eVersion>() + 1);
      if(!theCoordUnits.empty())
        for(TInt anId = 0; anId < aSpaceDim; anId++)
          SetCoordUnit(anId, theCoordUnits[anId]);
    }

    virtual
    std::string
    GetCoordName(TInt theId) const 
    { 
      return GetString(theId,GetPNOMLength<eVersion>(),myCoordNames);
    }

    virtual
    void
    SetCoordName(TInt theId, const std::string& theValue)
    {
      SetString(theId,GetPNOMLength<eVersion>(),myCoordNames,theValue);
    }

    virtual
    std::string 
    GetCoordUnit(TInt theId) const 
    { 
      return GetString(theId,GetPNOMLength<eVersion>(),myCoordUnits);
    }

    virtual
    void
    SetCoordUnit(TInt theId, const std::string& theValue)
    {
      SetString(theId,GetPNOMLength<eVersion>(),myCoordUnits,theValue);
    }
  };

  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTPolygoneInfo: 
    virtual TPolygoneInfo, 
    virtual TTElemInfo<eVersion>
  {
    typedef TTElemInfo<eVersion> TElemInfoBase;

    TTPolygoneInfo(const PMeshInfo& theMeshInfo, const PPolygoneInfo& theInfo):
      TElemInfoBase(theMeshInfo,theInfo)
    {
      myEntity = theInfo->GetEntity();
      myGeom = theInfo->GetGeom();

      myIndex.reset(new TElemNum(*theInfo->myIndex));
      myConn.reset(new TElemNum(*theInfo->myConn));

      myConnMode = theInfo->GetConnMode();
    }

    TTPolygoneInfo(const PMeshInfo& theMeshInfo, 
                   EEntiteMaillage theEntity, 
                   EGeometrieElement theGeom,
                   TInt theNbElem,
                   TInt theConnSize,
                   EConnectivite theConnMode,
                   EBooleen theIsElemNum,
                   EBooleen theIsElemNames):
      TElemInfoBase(theMeshInfo,
                    theNbElem,
                    theIsElemNum,
                    theIsElemNames)
    {
      myEntity = theEntity;
      myGeom = theGeom;

      myIndex.reset(new TElemNum(theNbElem + 1));
      myConn.reset(new TElemNum(theConnSize));

      myConnMode = theConnMode;
    }
    
    TTPolygoneInfo(const PMeshInfo& theMeshInfo, 
                   EEntiteMaillage theEntity, 
                   EGeometrieElement theGeom,
                   const TIntVector& theIndexes,
                   const TIntVector& theConnectivities,
                   EConnectivite theConnMode,
                   const TIntVector& theFamilyNums,
                   const TIntVector& theElemNums,
                   const TStringVector& theElemNames):
      TElemInfoBase(theMeshInfo,
                    (TInt)theIndexes.size() - 1,
                    theFamilyNums,
                    theElemNums,
                    theElemNames)
    {
      myEntity = theEntity;
      myGeom = theGeom;

      myIndex.reset(new TElemNum(theIndexes));
      myConn.reset(new TElemNum(theConnectivities));

      myConnMode = theConnMode;
    }
  };
  
  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTPolyedreInfo: 
    virtual TPolyedreInfo, 
    virtual TTElemInfo<eVersion>
  {
    typedef TTElemInfo<eVersion> TElemInfoBase;

    TTPolyedreInfo(const PMeshInfo& theMeshInfo, const PPolyedreInfo& theInfo):
      TElemInfoBase(theMeshInfo,theInfo)
    {
      myEntity = theInfo->GetEntity();
      myGeom = theInfo->GetGeom();

      myIndex.reset(new TElemNum(*theInfo->myIndex));
      myFaces.reset(new TElemNum(*theInfo->myFaces));
      myConn.reset(new TElemNum(*theInfo->myConn));

      myConnMode = theInfo->GetConnMode();
    }

    TTPolyedreInfo(const PMeshInfo& theMeshInfo, 
                   EEntiteMaillage theEntity, 
                   EGeometrieElement theGeom,
                   TInt theNbElem,
                   TInt theNbFaces,
                   TInt theConnSize,
                   EConnectivite theConnMode,
                   EBooleen theIsElemNum,
                   EBooleen theIsElemNames):
      TElemInfoBase(theMeshInfo,
                    theNbElem,
                    theIsElemNum,
                    theIsElemNames)
    {
      myEntity = theEntity;
      myGeom = theGeom;

      myIndex.reset(new TElemNum(theNbElem + 1));
      myFaces.reset(new TElemNum(theNbFaces));
      myConn.reset(new TElemNum(theConnSize));

      myConnMode = theConnMode;
    }
    
    TTPolyedreInfo(const PMeshInfo& theMeshInfo, 
                   EEntiteMaillage theEntity, 
                   EGeometrieElement theGeom,
                   const TIntVector& theIndexes,
                   const TIntVector& theFaces,
                   const TIntVector& theConnectivities,
                   EConnectivite theConnMode,
                   const TIntVector& theFamilyNums,
                   const TIntVector& theElemNums,
                   const TStringVector& theElemNames):
      TElemInfoBase(theMeshInfo,
                    (TInt)theIndexes.size()-1,
                    theFamilyNums,
                    theElemNums,
                    theElemNames)
    {
      myEntity = theEntity;
      myGeom = theGeom;

      myIndex.reset(new TElemNum(theIndexes));
      myFaces.reset(new TElemNum(theFaces));
      myConn.reset(new TElemNum(theConnectivities));

      myConnMode = theConnMode;
    }
  };

  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTCellInfo: 
    virtual TCellInfo, 
    virtual TTElemInfo<eVersion>
  {
    typedef TTElemInfo<eVersion> TElemInfoBase;

    TTCellInfo(const PMeshInfo& theMeshInfo, const PCellInfo& theInfo):
      TElemInfoBase(theMeshInfo,theInfo)
    {
      myEntity = theInfo->GetEntity();
      myGeom = theInfo->GetGeom();
      myConnMode  = theInfo->GetConnMode();
      
      TInt aConnDim = GetNbNodes(myGeom);
      TInt aNbConn = GetNbConn<eVersion>(myGeom, myEntity, myMeshInfo->myDim);
      myConn.reset(new TElemNum(myNbElem * aNbConn));
      for(TInt anElemId = 0; anElemId < myNbElem; anElemId++){
        TConnSlice aConnSlice = GetConnSlice(anElemId);
        TCConnSlice aConnSlice2 = theInfo->GetConnSlice(anElemId);
        for(TInt anConnId = 0; anConnId < aConnDim; anConnId++){
          aConnSlice[anConnId] = aConnSlice2[anConnId];
        }
      }
    }

    TTCellInfo(const PMeshInfo& theMeshInfo, 
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom,
               TInt theNbElem,
               EConnectivite theConnMode,
               EBooleen theIsElemNum,
               EBooleen theIsElemNames,
               EModeSwitch theMode):
      TModeSwitchInfo(theMode),
      TElemInfoBase(theMeshInfo,
                    theNbElem,
                    theIsElemNum,
                    theIsElemNames)
    {
      myEntity = theEntity;
      myGeom = theGeom;

      myConnMode = theConnMode;
      TInt aNbConn = GetNbConn<eVersion>(theGeom, myEntity, theMeshInfo->myDim);
      myConn.reset(new TElemNum(theNbElem * aNbConn));
    }
    
    TTCellInfo(const PMeshInfo& theMeshInfo, 
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom,
               const TIntVector& theConnectivities,
               EConnectivite theConnMode,
               const TIntVector& theFamilyNums,
               const TIntVector& theElemNums,
               const TStringVector& theElemNames,
               EModeSwitch theMode):
      TModeSwitchInfo(theMode),
      TElemInfoBase(theMeshInfo,
                    (TInt)theConnectivities.size() / GetNbNodes(theGeom),
                    theFamilyNums,
                    theElemNums,
                    theElemNames)
    {
      myEntity = theEntity;
      myGeom = theGeom;

      myConnMode = theConnMode;
      TInt aConnDim = GetNbNodes(myGeom);
      TInt aNbConn = GetNbConn<eVersion>(myGeom, myEntity, myMeshInfo->myDim);
      myConn.reset(new TElemNum(myNbElem * aNbConn));
      for(TInt anElemId = 0; anElemId < myNbElem; anElemId++){
        TConnSlice aConnSlice = GetConnSlice(anElemId);
        for(TInt anConnId = 0; anConnId < aConnDim; anConnId++){
          aConnSlice[anConnId] = theConnectivities[anElemId*aConnDim + anConnId];
        }
      }
    }

    virtual 
    TInt
    GetConnDim() const 
    { 
      return GetNbConn<eVersion>(myGeom, myEntity, myMeshInfo->myDim);
    }

  };

  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTBallInfo: 
    virtual TBallInfo,
    virtual TTCellInfo<eVersion>
  {
    typedef TTCellInfo<eVersion> TCellInfoBase;

    TTBallInfo(const PMeshInfo& theMeshInfo, const PBallInfo& theInfo):
      TCellInfoBase::TElemInfoBase(theMeshInfo, theInfo),
      TCellInfoBase(theMeshInfo,theInfo)
    {
      myDiameters = theInfo->myDiameters;
    }

    TTBallInfo(const PMeshInfo& theMeshInfo,
               TInt             theNbElem,
               EBooleen         theIsElemNum ):
      TCellInfoBase::TElemInfoBase(theMeshInfo,
                                   theNbElem,
                                   theIsElemNum,
                                   /*theIsElemNames=*/eFAUX),
      TCellInfoBase(theMeshInfo,
                    eSTRUCT_ELEMENT,
                    eBALL,
                    theNbElem,
                    /*EConnectivite=*/eNOD,
                    theIsElemNum,
                    /*theIsElemNames=*/eFAUX,
                    eFULL_INTERLACE)
    {
      myDiameters.resize( theNbElem );
    }

    TTBallInfo(const PMeshInfo&  theMeshInfo, 
               const TIntVector& theNodes,
               TFloatVector&     theDiameters,
               const TIntVector& theFamilyNums,
               const TIntVector& theElemNums):
      TCellInfoBase::TElemInfoBase(theMeshInfo,
                                   (TInt)std::max(theNodes.size(),theDiameters.size() ),
                                   theFamilyNums,
                                   theElemNums,
                                   TStringVector()),
      TCellInfoBase(theMeshInfo,
                    eSTRUCT_ELEMENT,
                    eBALL,
                    theNodes,
                    /*EConnectivite=*/eNOD,
                    theFamilyNums,
                    theElemNums,
                    TStringVector(),
                    eFULL_INTERLACE)
    {
      myDiameters.swap( theDiameters );
    }
  };

  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTFieldInfo: 
    virtual TFieldInfo, 
    virtual TTNameInfo<eVersion>
  {
    typedef TTNameInfo<eVersion> TNameInfoBase;

    TTFieldInfo(const PMeshInfo& theMeshInfo, const PFieldInfo& theInfo):
      TNameInfoBase(theInfo->GetName())
    {
      myMeshInfo = theMeshInfo;

      myNbComp = theInfo->GetNbComp();
      myCompNames.resize(myNbComp*GetPNOMLength<eVersion>()+1);
      for(TInt anId = 0; anId < myNbComp; anId++){
        SetCompName(anId,theInfo->GetCompName(anId));
      }

      myUnitNames.resize(myNbComp*GetPNOMLength<eVersion>()+1);
      for(TInt anId = 0; anId < myNbComp; anId++){
        SetUnitName(anId,theInfo->GetUnitName(anId));
      }

      myType = theInfo->GetType();

      myIsLocal = theInfo->GetIsLocal();
      myNbRef = theInfo->GetNbRef();
    }

    TTFieldInfo(const PMeshInfo& theMeshInfo, 
                TInt theNbComp,
                ETypeChamp theType,
                const std::string& theValue,
                EBooleen theIsLocal,
                TInt theNbRef):
      TNameInfoBase(theValue)
    {
      myMeshInfo = theMeshInfo;

      myNbComp = theNbComp;
      myCompNames.resize(theNbComp*GetPNOMLength<eVersion>()+1);
      myUnitNames.resize(theNbComp*GetPNOMLength<eVersion>()+1);

      myType = theType;

      myIsLocal = theIsLocal;
      myNbRef = theNbRef;
    }
    
    virtual 
    std::string
    GetCompName(TInt theId) const 
    { 
      return GetString(theId,GetPNOMLength<eVersion>(),myCompNames);
    }

    virtual
    void
    SetCompName(TInt theId, const std::string& theValue)
    {
      SetString(theId,GetPNOMLength<eVersion>(),myCompNames,theValue);
    }

    virtual
    std::string 
    GetUnitName(TInt theId) const 
    { 
      return GetString(theId,GetPNOMLength<eVersion>(),myUnitNames);
    }

    virtual
    void
    SetUnitName(TInt theId, const std::string& theValue)
    {
      SetString(theId,GetPNOMLength<eVersion>(),myUnitNames,theValue);
    }
  };


  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTGaussInfo: 
    virtual TGaussInfo,
    virtual TTNameInfo<eVersion>
  {
    typedef TTNameInfo<eVersion> TNameInfoBase;

    TTGaussInfo(const TGaussInfo::TInfo& theInfo,
                EModeSwitch theMode):
      TModeSwitchInfo(theMode),
      TNameInfoBase(boost::get<1>(boost::get<0>(theInfo)))
    {
      const TGaussInfo::TKey& aKey = boost::get<0>(theInfo);

      myGeom = boost::get<0>(aKey);
      myRefCoord.resize(GetNbRef()*GetDim());

      TInt aNbGauss = boost::get<1>(theInfo);
      myGaussCoord.resize(aNbGauss*GetDim());
      myWeight.resize(aNbGauss);
    }
  };


  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTTimeStampInfo: virtual TTimeStampInfo
  {
    TTTimeStampInfo(const PFieldInfo& theFieldInfo, const PTimeStampInfo& theInfo)
    {
      myFieldInfo = theFieldInfo;

      myEntity = theInfo->GetEntity();
      myGeom2Size = theInfo->GetGeom2Size();

      myNumDt = theInfo->GetNumDt();
      myNumOrd = theInfo->GetNumOrd();
      myDt = theInfo->GetDt();

      myUnitDt.resize(GetPNOMLength<eVersion>()+1);
      SetUnitDt(theInfo->GetUnitDt());

      myGeom2NbGauss = theInfo->myGeom2NbGauss;
      myGeom2Gauss = theInfo->GetGeom2Gauss();
    }

    TTTimeStampInfo(const PFieldInfo& theFieldInfo, 
                    EEntiteMaillage theEntity,
                    const TGeom2Size& theGeom2Size,
                    const TGeom2NbGauss& theGeom2NbGauss,
                    TInt theNumDt,
                    TInt theNumOrd,
                    TFloat theDt,
                    const std::string& theUnitDt,
                    const TGeom2Gauss& theGeom2Gauss)
    {
      myFieldInfo = theFieldInfo;

      myEntity = theEntity;
      myGeom2Size = theGeom2Size;

      myNumDt = theNumDt;
      myNumOrd = theNumDt;
      myDt = theDt;

      myUnitDt.resize(GetPNOMLength<eVersion>()+1);
      SetUnitDt(theUnitDt);

      myGeom2NbGauss = theGeom2NbGauss;
      myGeom2Gauss = theGeom2Gauss;
    }

    virtual 
    std::string
    GetUnitDt() const
    { 
      return GetString(0,GetPNOMLength<eVersion>(),myUnitDt);
    }

    virtual
    void
    SetUnitDt(const std::string& theValue)
    {
      SetString(0,GetPNOMLength<eVersion>(),myUnitDt,theValue);
    }
  };


  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTProfileInfo: 
    virtual TProfileInfo,
    virtual TTNameInfo<eVersion>
  {
    typedef TTNameInfo<eVersion> TNameInfoBase;

    TTProfileInfo(const TProfileInfo::TInfo& theInfo,
                  EModeProfil theMode):
      TNameInfoBase(boost::get<0>(theInfo))
    {
      TInt aSize = boost::get<1>(theInfo);
      myElemNum.reset(new TElemNum(aSize));
      myMode = aSize > 0? theMode: eNO_PFLMOD;
    }
  };


  //---------------------------------------------------------------
  template<EVersion eVersion, class TMeshValueType>
  struct TTTimeStampValue: virtual TTimeStampValue<TMeshValueType>
  {
    TTTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     const PTimeStampValueBase& theInfo,
                     ETypeChamp theTypeChamp)
    {
      typedef TTimeStampValue<TMeshValueType> TCompatible;
      if(TCompatible* aCompatible = dynamic_cast<TCompatible*>(theInfo.get())){
        this->myTimeStampInfo = theTimeStampInfo;
        this->myTypeChamp = theTypeChamp;
        this->myGeom2Profile = aCompatible->GetGeom2Profile();
        this->myGeom2Value = aCompatible->myGeom2Value;
        this->myGeomSet = aCompatible->GetGeomSet();
      }else
        EXCEPTION(std::runtime_error,"TTTimeStampValue::TTTimeStampValue - use incompatible arguments!");
    }

    TTTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     ETypeChamp theTypeChamp,
                     const TGeom2Profile& theGeom2Profile,
                     EModeSwitch theMode):
      TModeSwitchInfo(theMode)
    {
      this->myTimeStampInfo = theTimeStampInfo;

      this->myTypeChamp = theTypeChamp;

      this->myGeom2Profile = theGeom2Profile;

      TInt aNbComp = theTimeStampInfo->myFieldInfo->myNbComp;

      const TGeom2Size& aGeom2Size = theTimeStampInfo->GetGeom2Size();
      TGeom2Size::const_iterator anIter = aGeom2Size.begin();
      for(; anIter != aGeom2Size.end(); anIter++){
        const EGeometrieElement& aGeom = anIter->first;
        TInt aNbElem = anIter->second;

        MED::PProfileInfo aProfileInfo;
        MED::TGeom2Profile::const_iterator anIter = theGeom2Profile.find(aGeom);
        if(anIter != theGeom2Profile.end())
          aProfileInfo = anIter->second;

        if(aProfileInfo && aProfileInfo->IsPresent())
          aNbElem = aProfileInfo->GetSize();

        TInt aNbGauss = theTimeStampInfo->GetNbGauss(aGeom);
        
        this->GetMeshValue(aGeom).Allocate(aNbElem,aNbGauss,aNbComp);
      }
    }

    virtual 
    size_t
    GetValueSize(EGeometrieElement theGeom) const
    {
      return this->GetMeshValue(theGeom).GetSize();
    }

    virtual 
    size_t
    GetNbVal(EGeometrieElement theGeom) const
    {
      return this->GetMeshValue(theGeom).GetNbVal();
    }

    virtual 
    size_t
    GetNbGauss(EGeometrieElement theGeom) const
    {
      return this->GetMeshValue(theGeom).GetNbGauss();
    }

    virtual 
    void
    AllocateValue(EGeometrieElement theGeom,
                  TInt theNbElem,
                  TInt theNbGauss,
                  TInt theNbComp,
                  EModeSwitch theMode = eFULL_INTERLACE)
    {
      this->GetMeshValue(theGeom).Allocate(theNbElem,theNbGauss,theNbComp,theMode);
    }
    
    virtual 
    unsigned char*
    GetValuePtr(EGeometrieElement theGeom)
    {
      return this->GetMeshValue(theGeom).GetValuePtr();
    }
  };

  //---------------------------------------------------------------
  template<EVersion eVersion>
  struct TTGrilleInfo:
    virtual TGrilleInfo
  {
    TTGrilleInfo(const PMeshInfo& theMeshInfo,
                 const PGrilleInfo& theInfo)
    {
      myMeshInfo        = theMeshInfo;

      myCoord           = theInfo->GetNodeCoord();
      
      myGrilleType      = theInfo->GetGrilleType();

      myCoordNames      = theInfo->myCoordNames;

      myCoordUnits      = theInfo->myCoordUnits;

      myIndixes         = theInfo->GetMapOfIndexes();

      myGrilleStructure = theInfo->GetGrilleStructure();

      myGrilleType      = theInfo->GetGrilleType();

      myFamNumNode.resize(theInfo->GetNbNodes());
      myFamNumNode      = theInfo->myFamNumNode;

      myFamNum      = theInfo->myFamNum;
    }

    TTGrilleInfo(const PMeshInfo& theMeshInfo,
                 const EGrilleType& type,
                 const TInt nnoeuds)
    {
      myMeshInfo        = theMeshInfo;
      TInt aSpaceDim = theMeshInfo->GetSpaceDim();
      if(type == eGRILLE_STANDARD){
        myCoord.resize(aSpaceDim*nnoeuds);
        myCoordNames.resize(aSpaceDim*GetPNOMLength<eVersion>()+1);
        myCoordUnits.resize(aSpaceDim*GetPNOMLength<eVersion>()+1);
      } else { //if(type == eGRILLE_CARTESIENNE){
        myCoordNames.resize(aSpaceDim*GetPNOMLength<eVersion>()+aSpaceDim);
        myCoordUnits.resize(aSpaceDim*GetPNOMLength<eVersion>()+aSpaceDim);
      }
      myGrilleStructure.resize(aSpaceDim);
      myFamNumNode.resize(nnoeuds);
    }

    TTGrilleInfo(const PMeshInfo& theMeshInfo,
                 const EGrilleType& type)
    {
      myMeshInfo        = theMeshInfo;
      TInt aSpaceDim = theMeshInfo->GetSpaceDim();
      if(type == eGRILLE_STANDARD){
        myCoordNames.resize(aSpaceDim*GetPNOMLength<eVersion>()+1);
        myCoordUnits.resize(aSpaceDim*GetPNOMLength<eVersion>()+1);
      } else {// if(type == eGRILLE_CARTESIENNE){
        myCoordNames.resize(aSpaceDim*GetPNOMLength<eVersion>()+aSpaceDim);
        myCoordUnits.resize(aSpaceDim*GetPNOMLength<eVersion>()+aSpaceDim);
      }
      myGrilleStructure.resize(aSpaceDim);
    }

    TTGrilleInfo(const PMeshInfo& theMeshInfo,
                 const EGrilleType& type,
                 const MED::TIntVector& nbNodeVec)
    {
      myMeshInfo        = theMeshInfo;

      TInt aSpaceDim = theMeshInfo->GetSpaceDim();
      if(type == eGRILLE_STANDARD){
        myCoordNames.resize(aSpaceDim*GetPNOMLength<eVersion>()+1);
        myCoordUnits.resize(aSpaceDim*GetPNOMLength<eVersion>()+1);
      } else {// if(type == eGRILLE_CARTESIENNE){
        myCoordNames.resize(aSpaceDim*GetPNOMLength<eVersion>()+aSpaceDim);
        myCoordUnits.resize(aSpaceDim*GetPNOMLength<eVersion>()+aSpaceDim);
      }

      if(type != eGRILLE_STANDARD)
        for(unsigned int aAxe=0;aAxe<nbNodeVec.size();aAxe++){
          myIndixes[aAxe].resize(nbNodeVec[aAxe]);
        }
      myGrilleStructure.resize(aSpaceDim);
    }

    virtual
    std::string
    GetCoordName(TInt theId) const 
    { 
      return GetString(theId,GetPNOMLength<eVersion>(),myCoordNames);
    }

    virtual
    void
    SetCoordName(TInt theId, const std::string& theValue)
    {
      SetString(theId,GetPNOMLength<eVersion>(),myCoordNames,theValue);
    }

    virtual
    std::string 
    GetCoordUnit(TInt theId) const 
    { 
      return GetString(theId,GetPNOMLength<eVersion>(),myCoordUnits);
    }

    virtual
    void
    SetCoordUnit(TInt theId, const std::string& theValue)
    {
      SetString(theId,GetPNOMLength<eVersion>(),myCoordUnits,theValue);
    }

  };
}

#endif
