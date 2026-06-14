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
#ifndef MED_TWrapper_HeaderFile
#define MED_TWrapper_HeaderFile

#include "MED_TStructures.hxx"
#include "MED_Wrapper.hxx"

namespace MED
{

  template<EVersion eVersion>
  class TTWrapper: public TWrapper
  {
  public:
    //----------------------------------------------------------------------------
    //! Gets version of the MED library used for the MED file
    virtual 
    EVersion
    GetVersion()
    {
      return eVersion;
    }
    
    //----------------------------------------------------------------------------
    virtual 
    PMeshInfo
    CrMeshInfo(TInt theDim = 0, TInt theSpaceDim = 0,
               const std::string& theValue = "",
               EMaillage theType = eNON_STRUCTURE,
               const std::string& theDesc = "")
    {
      return PMeshInfo(new TTMeshInfo<eVersion>
                       (theDim,
                        theSpaceDim,
                        theValue,
                        theType,
                        theDesc));
    }

    virtual 
    PMeshInfo
    CrMeshInfo(const PMeshInfo& theInfo)
    {
      return PMeshInfo(new TTMeshInfo<eVersion>(theInfo));
    }
    

    //----------------------------------------------------------------------------
    virtual
    PFamilyInfo
    CrFamilyInfo(const PMeshInfo& theMeshInfo,
                 TInt theNbGroup = 0, 
                 TInt theNbAttr = 0,
                 TInt theId = 0,
                 const std::string& theValue = "")
    {
      return PFamilyInfo(new TTFamilyInfo<eVersion>
                         (theMeshInfo,
                          theNbGroup,
                          theNbAttr,
                          theId,
                          theValue));
    }

    virtual
    PFamilyInfo
    CrFamilyInfo(const PMeshInfo& theMeshInfo,
                 const std::string& theValue,
                 TInt theId,
                 const MED::TStringSet& theGroupNames, 
                 const MED::TStringVector& theAttrDescs = MED::TStringVector(), 
                 const MED::TIntVector& theAttrIds = MED::TIntVector(), 
                 const MED::TIntVector& theAttrVals = MED::TIntVector())
    {
      return PFamilyInfo(new TTFamilyInfo<eVersion>
                         (theMeshInfo,
                          theValue,
                          theId,
                          theGroupNames,
                          theAttrDescs,
                          theAttrIds,
                          theAttrVals));
    }

    virtual
    PFamilyInfo
    CrFamilyInfo(const PMeshInfo& theMeshInfo,
                 const PFamilyInfo& theInfo)
    {
      return PFamilyInfo(new TTFamilyInfo<eVersion>
                         (theMeshInfo,
                          theInfo));
    }

    //----------------------------------------------------------------------------
    virtual
    PElemInfo
    CrElemInfo(const PMeshInfo& theMeshInfo, 
               TInt theNbElem,
               EBooleen theIsElemNum = eVRAI,
               EBooleen theIsElemNames = eVRAI)
    {
      return PElemInfo(new TTElemInfo<eVersion>
                       (theMeshInfo,
                        theNbElem,
                        theIsElemNum,
                        theIsElemNames));
    }

    virtual
    PElemInfo
    CrElemInfo(const PMeshInfo& theMeshInfo, 
               TInt theNbElem,
               const TIntVector& theFamNum,
               const TIntVector& aElemNum,
               const TStringVector& aElemNames)
    {
      return PElemInfo(new TTElemInfo<eVersion>
                       (theMeshInfo,
                        theNbElem,
                        theFamNum,
                        aElemNum,
                        aElemNames));
    }

    //----------------------------------------------------------------------------
    virtual
    PNodeInfo
    CrNodeInfo(const PMeshInfo& theMeshInfo, 
               TInt theNbElem,
               EModeSwitch theMode = eFULL_INTERLACE,
               ERepere theSystem = eCART, 
               EBooleen theIsElemNum = eVRAI,
               EBooleen theIsElemNames = eVRAI)
    {
      return PNodeInfo(new TTNodeInfo<eVersion>
                       (theMeshInfo,
                        theNbElem,
                        theMode,
                        theSystem,
                        theIsElemNum,
                        theIsElemNames));
    }

    virtual 
    PNodeInfo
    CrNodeInfo(const PMeshInfo& theMeshInfo, 
               const TFloatVector& theNodeCoords,
               EModeSwitch theMode = eFULL_INTERLACE,
               ERepere theSystem = eCART, 
               const TStringVector& theCoordNames = TStringVector(),
               const TStringVector& theCoordUnits = TStringVector(),
               const TIntVector& theFamilyNums = TIntVector(),
               const TIntVector& theElemNums = TIntVector(),
               const TStringVector& theElemNames = TStringVector())
    {
      return PNodeInfo(new TTNodeInfo<eVersion>
                       (theMeshInfo,
                        theNodeCoords,
                        theMode,
                        theSystem,
                        theCoordNames,
                        theCoordUnits,
                        theFamilyNums,
                        theElemNums,
                        theElemNames));
    }

    virtual 
    PNodeInfo
    CrNodeInfo(const PMeshInfo& theMeshInfo,
               const PNodeInfo& theInfo)
    {
      return PNodeInfo(new TTNodeInfo<eVersion>
                       (theMeshInfo,
                        theInfo));
    }
    
    //----------------------------------------------------------------------------
    virtual
    PPolygoneInfo
    CrPolygoneInfo(const PMeshInfo& theMeshInfo, 
                   EEntiteMaillage theEntity, 
                   EGeometrieElement theGeom,
                   TInt theNbElem,
                   TInt theConnSize,
                   EConnectivite theConnMode = eNOD,
                   EBooleen theIsElemNum = eVRAI,
                   EBooleen theIsElemNames = eVRAI)
    {
      return PPolygoneInfo(new TTPolygoneInfo<eVersion>
                           (theMeshInfo,
                            theEntity,
                            theGeom,
                            theNbElem,
                            theConnSize,
                            theConnMode,
                            theIsElemNum,
                            theIsElemNames));
    }

    virtual
    PPolygoneInfo
    CrPolygoneInfo(const PMeshInfo& theMeshInfo, 
                   EEntiteMaillage theEntity, 
                   EGeometrieElement theGeom,
                   const TIntVector& theIndexes,
                   const TIntVector& theConnectivities,
                   EConnectivite theConnMode = eNOD,
                   const TIntVector& theFamilyNums = TIntVector(),
                   const TIntVector& theElemNums = TIntVector(),
                   const TStringVector& theElemNames = TStringVector())
    {
      return PPolygoneInfo(new TTPolygoneInfo<eVersion>
                           (theMeshInfo,
                            theEntity,
                            theGeom,
                            theIndexes,
                            theConnectivities,
                            theConnMode,
                            theFamilyNums,
                            theElemNums,
                            theElemNames));
    }

    virtual
    PPolygoneInfo
    CrPolygoneInfo(const PMeshInfo& theMeshInfo,
                   const PPolygoneInfo& theInfo)
    {
      return PPolygoneInfo(new TTPolygoneInfo<eVersion>
                           (theMeshInfo,
                            theInfo));
    }
    
    //----------------------------------------------------------------------------
    virtual
    PPolyedreInfo
    CrPolyedreInfo(const PMeshInfo& theMeshInfo, 
                   EEntiteMaillage theEntity, 
                   EGeometrieElement theGeom,
                   TInt theNbElem,
                   TInt theNbFaces,
                   TInt theConnSize,
                   EConnectivite theConnMode = eNOD,
                   EBooleen theIsElemNum = eVRAI,
                   EBooleen theIsElemNames = eVRAI)
    {
      return PPolyedreInfo(new TTPolyedreInfo<eVersion>
                           (theMeshInfo,
                            theEntity,
                            theGeom,
                            theNbElem,
                            theNbFaces,
                            theConnSize,
                            theConnMode,
                            theIsElemNum,
                            theIsElemNames));
    }

    virtual
    PPolyedreInfo
    CrPolyedreInfo(const PMeshInfo& theMeshInfo, 
                   EEntiteMaillage theEntity, 
                   EGeometrieElement theGeom,
                   const TIntVector& theIndexes,
                   const TIntVector& theFaces,
                   const TIntVector& theConnectivities,
                   EConnectivite theConnMode = eNOD,
                   const TIntVector& theFamilyNums = TIntVector(),
                   const TIntVector& theElemNums = TIntVector(),
                   const TStringVector& theElemNames = TStringVector())
    {
      return PPolyedreInfo(new TTPolyedreInfo<eVersion>
                           (theMeshInfo,
                            theEntity,
                            theGeom,
                            theIndexes,
                            theFaces,
                            theConnectivities,
                            theConnMode,
                            theFamilyNums,
                            theElemNums,
                            theElemNames));
    }

    virtual
    PPolyedreInfo
    CrPolyedreInfo(const PMeshInfo& theMeshInfo,
                   const PPolyedreInfo& theInfo)
    {
      return PPolyedreInfo(new TTPolyedreInfo<eVersion>
                           (theMeshInfo,
                            theInfo));
    }

    //----------------------------------------------------------------------------
    virtual
    PCellInfo
    CrCellInfo(const PMeshInfo& theMeshInfo, 
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom,
               TInt theNbElem,
               EConnectivite theConnMode = eNOD,
               EBooleen theIsElemNum = eVRAI,
               EBooleen theIsElemNames = eVRAI,
               EModeSwitch theMode = eFULL_INTERLACE)
    {
      return PCellInfo(new TTCellInfo<eVersion>
                       (theMeshInfo,
                        theEntity,
                        theGeom,
                        theNbElem,
                        theConnMode,
                        theIsElemNum,
                        theIsElemNames,
                        theMode));
    }

    virtual
    PCellInfo
    CrCellInfo(const PMeshInfo& theMeshInfo, 
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom,
               const TIntVector& theConnectivities,
               EConnectivite theConnMode = eNOD,
               const TIntVector& theFamilyNums = TIntVector(),
               const TIntVector& theElemNums = TIntVector(),
               const TStringVector& theElemNames = TStringVector(),
               EModeSwitch theMode = eFULL_INTERLACE)
    {
      return PCellInfo(new TTCellInfo<eVersion>
                       (theMeshInfo,
                        theEntity,
                        theGeom,
                        theConnectivities,
                        theConnMode,
                        theFamilyNums,
                        theElemNums,
                        theElemNames,
                        theMode));
    }

    virtual
    PCellInfo
    CrCellInfo(const PMeshInfo& theMeshInfo,
               const PCellInfo& theInfo)
    {
      return PCellInfo(new TTCellInfo<eVersion>
                       (theMeshInfo,
                        theInfo));
    }
    
    //----------------------------------------------------------------------------
    //! Creates a MEDWrapper MED Balls representation
    virtual PBallInfo CrBallInfo(const PMeshInfo& theMeshInfo, 
                                 TInt             theNbBalls,
                                 EBooleen         theIsElemNum = eVRAI)
    {
      return PBallInfo( new TTBallInfo<eVersion>( theMeshInfo, theNbBalls, theIsElemNum ));
    }

    //! Creates a MEDWrapper MED Balls representation
    virtual PBallInfo CrBallInfo(const PMeshInfo&  theMeshInfo, 
                                 const TIntVector& theNodes,
                                 TFloatVector&     theDiameters,
                                 const TIntVector& theFamilyNums = TIntVector(),
                                 const TIntVector& theElemNums = TIntVector())
    {
      return PBallInfo( new TTBallInfo<eVersion>( theMeshInfo, theNodes, theDiameters,
                                                  theFamilyNums, theElemNums));
    }

    //! A copy-constructor for the MEDWrapper MED Balls representation
    virtual PBallInfo CrBallInfo(const PMeshInfo& theMeshInfo,
                                 const PBallInfo& theInfo)
    {
      return PBallInfo( new TTBallInfo<eVersion>( theMeshInfo, theInfo ));
    }

    //----------------------------------------------------------------------------
    virtual
    PFieldInfo
    CrFieldInfo(const PMeshInfo& theMeshInfo, 
                TInt theNbComp = 0,
                ETypeChamp theType = eFLOAT64,
                const std::string& theValue = "",
                EBooleen theIsLocal = eVRAI,
                TInt theNbRef = 1)
    {
      return PFieldInfo(new TTFieldInfo<eVersion>
                        (theMeshInfo,
                         theNbComp,
                         theType,
                         theValue,
                         theIsLocal,
                         theNbRef));
    }

    virtual
    PFieldInfo
    CrFieldInfo(const PMeshInfo& theMeshInfo,
                const PFieldInfo& theInfo)
    {
      return PFieldInfo(new TTFieldInfo<eVersion>
                        (theMeshInfo,
                         theInfo));
    }
    

    //----------------------------------------------------------------------------
    virtual
    PTimeStampInfo
    CrTimeStampInfo(const PFieldInfo& theFieldInfo, 
                    EEntiteMaillage theEntity,
                    const TGeom2Size& theGeom2Size,
                    const TGeom2NbGauss& theGeom2NbGauss = TGeom2NbGauss(),
                    TInt theNumDt = 0,
                    TInt theNumOrd = 0,
                    TFloat theDt = 0,
                    const std::string& theUnitDt = "",
                    const TGeom2Gauss& theGeom2Gauss = TGeom2Gauss())
    {
      return PTimeStampInfo(new TTTimeStampInfo<eVersion>
                            (theFieldInfo,
                             theEntity,
                             theGeom2Size,
                             theGeom2NbGauss,
                             theNumDt,
                             theNumOrd,
                             theDt,
                             theUnitDt,
                             theGeom2Gauss));
    }

    virtual
    PTimeStampInfo
    CrTimeStampInfo(const PFieldInfo& theFieldInfo,
                    const PTimeStampInfo& theInfo)
    {
      return PTimeStampInfo(new TTTimeStampInfo<eVersion>
                            (theFieldInfo,
                             theInfo));
    }


    //----------------------------------------------------------------------------
    virtual
    PGaussInfo
    CrGaussInfo(const TGaussInfo::TInfo& theInfo,
                EModeSwitch theMode = eFULL_INTERLACE)
    {
      return PGaussInfo(new TTGaussInfo<eVersion>
                        (theInfo,
                         theMode));
    }
    

    //----------------------------------------------------------------------------
    virtual
    PProfileInfo
    CrProfileInfo(const TProfileInfo::TInfo& theInfo,
                  EModeProfil theMode = eCOMPACT)
    {
      return PProfileInfo(new TTProfileInfo<eVersion>
                           (theInfo,
                            theMode));
    }
    

    //----------------------------------------------------------------------------
    virtual
    PTimeStampValueBase
    CrTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     ETypeChamp theTypeChamp,
                     const TGeom2Profile& theGeom2Profile = TGeom2Profile(),
                     EModeSwitch theMode = eFULL_INTERLACE)
    {
      if(theTypeChamp == eFLOAT64)
        return PTimeStampValueBase(new TTTimeStampValue<eVersion, TFloatMeshValue>
                                   (theTimeStampInfo,
                                    theTypeChamp,
                                    theGeom2Profile,
                                    theMode));
      return PTimeStampValueBase(new TTTimeStampValue<eVersion, TIntMeshValue>
                                 (theTimeStampInfo,
                                  theTypeChamp,
                                  theGeom2Profile,
                                  theMode));
    }

    virtual
    PTimeStampValueBase
    CrTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     const PTimeStampValueBase& theInfo,
                     ETypeChamp theTypeChamp)
    {
      if(theTypeChamp == eFLOAT64)
        return PTimeStampValueBase(new TTTimeStampValue<eVersion, TFloatMeshValue>
                                   (theTimeStampInfo,
                                    theInfo,
                                    theTypeChamp));
      return PTimeStampValueBase(new TTTimeStampValue<eVersion, TIntMeshValue>
                                 (theTimeStampInfo,
                                  theInfo,
                                  theTypeChamp));
    }
    
    //----------------------------------------------------------------------------
    virtual
    PGrilleInfo
    CrGrilleInfo(const PMeshInfo& theMeshInfo,
                 const PGrilleInfo& theInfo)
    {
      return PGrilleInfo(new TTGrilleInfo<eVersion>
                            (theMeshInfo,
                             theInfo));
    }
    virtual
    PGrilleInfo
    CrGrilleInfo(const PMeshInfo& theMeshInfo,
                 const EGrilleType& type)
    {
      return PGrilleInfo(new TTGrilleInfo<eVersion>
                            (theMeshInfo,
                             type));
    }

    virtual
    PGrilleInfo
    CrGrilleInfo(const PMeshInfo& theMeshInfo,
                 const EGrilleType& type,
                 const TInt& nbNodes)
    {
      return PGrilleInfo(new TTGrilleInfo<eVersion>
                            (theMeshInfo,
                             type,
                             nbNodes));
    }

    virtual
    PGrilleInfo
    CrGrilleInfo(const PMeshInfo& theMeshInfo,
                 const EGrilleType& type,
                 const MED::TIntVector& nbNodeVec)
    {
      return PGrilleInfo(new TTGrilleInfo<eVersion>
                            (theMeshInfo,
                             type,
                             nbNodeVec));
    }
    //----------------------------------------------------------------------------
  };

}


#endif
