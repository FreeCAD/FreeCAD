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

#ifndef MED_Wrapper_HeaderFile
#define MED_Wrapper_HeaderFile

#include "MED_WrapperBase.hxx"

#include "MED_Structures.hxx"
#include "MED_Algorithm.hxx"

#include <boost/thread/mutex.hpp>

namespace MED
{

  //----------------------------------------------------------------------------
  //! Define a base class that wraps the MED API
  struct MEDWRAPPER_EXPORT TWrapper
  {
    typedef boost::mutex TMutex;
    //! This is a syncronization primitive which allow to support thread safety for the MED access
    TMutex myMutex;

    virtual
    ~TWrapper();

    //----------------------------------------------------------------------------
    //! Gets version of the MED library used for the MED file
    virtual 
    EVersion
    GetVersion() = 0;
    
    //----------------------------------------------------------------------------
    //! Creates a MEDWrapper MED Mesh representation
    virtual 
    PMeshInfo 
    CrMeshInfo(TInt theDim = 0, TInt theSpaceDim = 0,
               const std::string& theValue = "",
               EMaillage theType = eNON_STRUCTURE,
               const std::string& theDesc = "") = 0;

    //! A copy-constructor for the MEDWrapper MED Mesh representation
    virtual 
    PMeshInfo 
    CrMeshInfo(const PMeshInfo& theInfo) = 0;

    //! Read number of MED Mesh entities in the defined MED file
    virtual 
    TInt 
    GetNbMeshes(TErr* theErr = NULL) = 0;
    
    //! Read a MEDWrapper MED Mesh representation by its number
    virtual 
    void 
    GetMeshInfo(TInt theMeshId, 
                TMeshInfo& theInfo,
                TErr* theErr = NULL) = 0;

    //! Write the MEDWrapper MED Mesh representation in the defined MED file
    virtual 
    void
    SetMeshInfo(const TMeshInfo& theInfo,
                TErr* theErr = NULL) = 0;
    
    //! Read a MEDWrapper MED Mesh representation by its number
    virtual 
    PMeshInfo
    GetPMeshInfo(TInt theId,
                 TErr* theErr = NULL);


    //----------------------------------------------------------------------------
    //! Read number of MED Family entities in the defined MED file
    virtual 
    TInt
    GetNbFamilies(const TMeshInfo& theMeshInfo,
                  TErr* theErr = NULL) = 0;

    //! Read number of attributes for defined MED Family
    virtual 
    TInt
    GetNbFamAttr(TInt theFamId, 
                 const TMeshInfo& theInfo,
                 TErr* theErr = NULL) = 0;
    
    //! Read number of MED Groups where MED Family with the number belong to
    virtual
    TInt
    GetNbFamGroup(TInt theFamId, 
                  const TMeshInfo& theInfo,
                  TErr* theErr = NULL) = 0;
    
    //! Read a MEDWrapper MED Family representation by its number
    virtual
    void
    GetFamilyInfo(TInt theFamId, 
                  TFamilyInfo& theInfo,
                  TErr* theErr = NULL) = 0;
    
    //! Write a MEDWrapper MED Family representation by its number
    virtual
    void
    SetFamilyInfo(const TFamilyInfo& theInfo,
                  TErr* theErr = NULL) = 0;
    
    //! Creates a MEDWrapper MED Family representation
    virtual 
    PFamilyInfo
    CrFamilyInfo(const PMeshInfo& theMeshInfo,
                 TInt theNbGroup = 0, 
                 TInt theNbAttr = 0,
                 TInt theId = 0,
                 const std::string& theValue = "") = 0;
    
    //! Creates a MEDWrapper MED Family representation
    virtual 
    PFamilyInfo
    CrFamilyInfo(const PMeshInfo& theMeshInfo,
                 const std::string& theValue,
                 TInt theId,
                 const TStringSet& theGroupNames, 
                 const TStringVector& theAttrDescs = TStringVector(), 
                 const TIntVector& theAttrIds = TIntVector(), 
                 const TIntVector& theAttrVals = TIntVector()) = 0;
    
    //! A copy-constructor for the MEDWrapper MED Family representation
    virtual
    PFamilyInfo
    CrFamilyInfo(const PMeshInfo& theMeshInfo,
                 const PFamilyInfo& theInfo) = 0;
    
    //! Write a MEDWrapper MED Family representation by its number
    PFamilyInfo
    GetPFamilyInfo(const PMeshInfo& theMeshInfo, 
                   TInt theId,
                   TErr* theErr = NULL);
    
    //----------------------------------------------------------------------------
    //! Read sequence of names for any descendant of TElemInfo
    virtual
    void
    GetNames(TElemInfo& theInfo,
             TInt theNb,
             EEntiteMaillage theEntity, 
             EGeometrieElement theGeom,
             TErr* theErr = NULL)
    {}

    //! Read sequence of numerous for any descendant of TElemInfo
    virtual
    void
    GetNumeration(TElemInfo& theInfo,
                  TInt theNb,
                  EEntiteMaillage theEntity, 
                  EGeometrieElement theGeom,
                  TErr* theErr = NULL)
    {}

    //! Read sequence MED Family indexes for any descendant of TElemInfo
    virtual
    void
    GetFamilies(TElemInfo& theInfo,
                TInt theNb,
                EEntiteMaillage theEntity, 
                EGeometrieElement theGeom,
                TErr* theErr = NULL)
    {}

    //! Write sequence of names for any descendant of TElemInfo
    virtual
    void
    SetNames(const TElemInfo& theInfo,
             EEntiteMaillage theEntity, 
             EGeometrieElement theGeom,
             TErr* theErr = NULL)
    {}

    //! Write sequence of numerous for any descendant of TElemInfo
    virtual
    void
    SetNumeration(const TElemInfo& theInfo,
                  EEntiteMaillage theEntity, 
                  EGeometrieElement theGeom,
                  TErr* theErr = NULL)
    {}

    //! Write sequence MED Family indexes for any descendant of TElemInfo
    virtual
    void
    SetFamilies(const TElemInfo& theInfo,
                EEntiteMaillage theEntity, 
                EGeometrieElement theGeom,
                TErr* theErr = NULL)
    {}

    //! Read a MEDWrapper MED Element representation from defined MED file
    PElemInfo
    GetPElemInfo(const PMeshInfo& theMeshInfo,
                 EEntiteMaillage theEntity = eNOEUD, 
                 EGeometrieElement theGeom = ePOINT1, 
                 EConnectivite theConnMode = eNOD,
                 TErr* theErr = NULL);

    //----------------------------------------------------------------------------
    //! Read number of nodes in defined MED Mesh
    virtual
    TInt
    GetNbNodes(const TMeshInfo& theMeshInfo,
               TErr* theErr = NULL) = 0;
    
    virtual
    TInt
    GetNbNodes(const TMeshInfo& theMeshInfo,
               ETable theTable,
               TErr* theErr = NULL)
    {
      return 0;
    }

    //! Read a MEDWrapper MED Nodes representation from defined MED file
    virtual
    void
    GetNodeInfo(TNodeInfo& theInfo,
                TErr* theErr = NULL) = 0;

    //! Write the MEDWrapper MED Nodes representation into defined MED file
    virtual 
    void
    SetNodeInfo(const TNodeInfo& theInfo,
                TErr* theErr = NULL) = 0;
    
    //! Creates a MEDWrapper MED Nodes representation
    virtual 
    PElemInfo
    CrElemInfo(const PMeshInfo& theMeshInfo, 
               TInt theNbElem,
               EBooleen theIsElemNum = eVRAI,
               EBooleen theIsElemNames = eVRAI)
    {
      return PElemInfo();
    }

    //! Creates a MEDWrapper MED Nodes representation
    virtual 
    PElemInfo
    CrElemInfo(const PMeshInfo& theMeshInfo, 
               TInt theNbElem,
               const TIntVector& theFamNum,
               const TIntVector& aElemNum,
               const TStringVector& aElemNames)
    {
      return PElemInfo();
    }

    //! Creates a MEDWrapper MED Nodes representation
    virtual
    PNodeInfo
    CrNodeInfo(const PMeshInfo& theMeshInfo, 
               TInt theNbElem,
               EModeSwitch theMode = eFULL_INTERLACE,
               ERepere theSystem = eCART, 
               EBooleen theIsElemNum = eVRAI,
               EBooleen theIsElemNames = eVRAI) = 0;

    //! Creates a MEDWrapper MED Nodes representation
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
               const TStringVector& theElemNames = TStringVector()) = 0;
    
    //! A copy-constructor for the MEDWrapper MED Nodes representation
    virtual
    PNodeInfo
    CrNodeInfo(const PMeshInfo& theMeshInfo,
               const PNodeInfo& theInfo) = 0;
    
    //! Read a MEDWrapper MED Nodes representation from defined MED file
    PNodeInfo
    GetPNodeInfo(const PMeshInfo& theMeshInfo,
                 TErr* theErr = NULL);
    
    //----------------------------------------------------------------------------
    //! Read a MEDWrapper MED Polygones representation from defined MED file
    /*! This feature is supported only for version of 2.2 and higher */
    virtual
    void
    GetPolygoneInfo(TPolygoneInfo& theInfo,
                    TErr* theErr = NULL) 
    {}

    //! Write a MEDWrapper MED Polygones representation from defined MED file
    /*! This feature is supported only for version of 2.2 and higher */
    virtual
    void
    SetPolygoneInfo(const TPolygoneInfo& theInfo,
                    TErr* theErr = NULL) 
    {}

    //! Read number of MED Polygones in defined MED Mesh
    /*! This feature is supported only for version of 2.2 and higher */
    virtual
    TInt
    GetNbPolygones(const TMeshInfo& theMeshInfo,
                   EEntiteMaillage theEntity,
                   EGeometrieElement theGeom,
                   EConnectivite theConnMode = eNOD,
                   TErr* theErr = NULL) 
    { 
      return 0;
    }

    //! Read connectivity infroamtion for the MED Polygones in defined MED Mesh
    /*! This feature is supported only for version of 2.2 and higher */
    virtual
    TInt
    GetPolygoneConnSize(const TMeshInfo& theMeshInfo,
                        EEntiteMaillage theEntity,
                        EGeometrieElement theGeom,
                        EConnectivite theConnMode = eNOD,
                        TErr* theErr = NULL)
    { 
      return 0;
    }
    
    //! Creates a MEDWrapper MED Polygones representation
    /*! This feature is supported only for version of 2.2 and higher */
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
      return PPolygoneInfo();
    }
    
    //! Creates a MEDWrapper MED Polygones representation
    /*! This feature is supported only for version of 2.2 and higher */
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
      return PPolygoneInfo();
    }

    //! A copy-constructor for the MEDWrapper MED Polygones representation
    virtual
    PPolygoneInfo
    CrPolygoneInfo(const PMeshInfo& theMeshInfo,
                   const PPolygoneInfo& theInfo)
    {
      return PPolygoneInfo();
    }
    
    //! Read a MEDWrapper MED Polygones representation from defined MED file
    /*! This feature is support only for version of 2.2 and higher */
    PPolygoneInfo
    GetPPolygoneInfo(const PMeshInfo& theMeshInfo,
                     EEntiteMaillage theEntity, 
                     EGeometrieElement theGeom, 
                     EConnectivite theConnMode = eNOD);
    
    //----------------------------------------------------------------------------
    //! Read a MEDWrapper MED Polyedres representation from defined MED file
    /*! This feature is support only for version of 2.2 and higher */
    virtual 
    void
    GetPolyedreInfo(TPolyedreInfo& theInfo,
                    TErr* theErr = NULL) 
    {}

    //! Write a MEDWrapper MED Polyedres representation from defined MED file
    /*! This feature is support only for version of 2.2 and higher */
    virtual
    void
    SetPolyedreInfo(const TPolyedreInfo& theInfo,
                    TErr* theErr = NULL)
    {}
    
    //! Read number of MED Polyedres in defined MED Mesh
    /*! This feature is support only for version of 2.2 and higher */
    virtual
    TInt
    GetNbPolyedres(const TMeshInfo& theMeshInfo,
                   EEntiteMaillage theEntity,
                   EGeometrieElement theGeom,
                   EConnectivite theConnMode = eNOD,
                   TErr* theErr = NULL)
    { 
      return 0;
    }

    //! Read connectivity infroamtion for the MED Polyedres in defined MED Mesh
    /*! This feature is support only for version of 2.2 and higher */
    virtual 
    void
    GetPolyedreConnSize(const TMeshInfo& theMeshInfo,
                        TInt& theNbFaces,
                        TInt& theConnSize,
                        EConnectivite theConnMode = eNOD,
                        TErr* theErr = NULL)
    {
      theNbFaces = theConnSize = 0;
    }
    
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
      return PPolyedreInfo();
    }
    
    //! Creates a MEDWrapper MED Polyedres representation
    /*! This feature is support only for version of 2.2 and higher */
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
      return PPolyedreInfo();
    }

    //! A copy-constructor for the MEDWrapper MED Polyedres representation
    virtual
    PPolyedreInfo
    CrPolyedreInfo(const PMeshInfo& theMeshInfo,
                   const PPolyedreInfo& theInfo)
    {
      return PPolyedreInfo();
    }
    
    //! Read a MEDWrapper MED Polyedres representation from defined MED file
    /*! This feature is support only for version of 2.2 and higher */
    PPolyedreInfo
    GetPPolyedreInfo(const PMeshInfo& theMeshInfo,
                     EEntiteMaillage theEntity, 
                     EGeometrieElement theGeom, 
                     EConnectivite theConnMode = eNOD);
    
    //----------------------------------------------------------------------------
    //! Get TEntityInfo which contains brief information about existing cells and their destribution among MED ENTITIES
    virtual
    TEntityInfo
    GetEntityInfo(const TMeshInfo& theMeshInfo,
                  EConnectivite theConnMode = eNOD,
                  TErr* theErr = NULL) = 0;
    
    //! Read number of cells for defined MED Mesh, ENTITY and geometrical type with define mode of connectivity
    virtual
    TInt
    GetNbCells(const TMeshInfo& theMeshInfo, 
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom, 
               EConnectivite theConnMode = eNOD,
               TErr* theErr = NULL) = 0;
    
    //! Read a MEDWrapper MED Cells representation from defined MED file
    virtual
    void
    GetCellInfo(TCellInfo& theInfo,
                TErr* theErr = NULL) = 0;
    
    //! Write the MEDWrapper MED Cells representation into defined MED file
    virtual 
    void
    SetCellInfo(const TCellInfo& theInfo,
                TErr* theErr = NULL) = 0;
    
    //! Creates a MEDWrapper MED Cells representation
    virtual
    PCellInfo
    CrCellInfo(const PMeshInfo& theMeshInfo, 
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom,
               TInt theNbElem,
               EConnectivite theConnMode = eNOD,
               EBooleen theIsElemNum = eVRAI,
               EBooleen theIsElemNames = eVRAI,
               EModeSwitch theMode = eFULL_INTERLACE) = 0;
    
    //! Creates a MEDWrapper MED Cells representation
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
               EModeSwitch theMode = eFULL_INTERLACE) = 0;
    
    //! A copy-constructor for the MEDWrapper MED Cells representation
    virtual
    PCellInfo
    CrCellInfo(const PMeshInfo& theMeshInfo,
               const PCellInfo& theInfo) = 0;
    
    //! Read a MEDWrapper MED Cells representation from defined MED file
    PCellInfo
    GetPCellInfo(const PMeshInfo& theMeshInfo,
                 EEntiteMaillage theEntity, 
                 EGeometrieElement theGeom, 
                 EConnectivite theConnMode = eNOD,
                 TErr* theErr = NULL);

    //----------------------------------------------------------------------------
    //! Read number of balls in the Mesh
    /*! This feature is supported since version 3.0 */
    virtual
    TInt
    GetNbBalls(const TMeshInfo& theMeshInfo)
    {
      return 0;
    }

    //! Read a MEDWrapped representation of MED_BALL from the MED file
    /*! This feature is supported since version 3.0 */
    virtual
    void
    GetBallInfo(TBallInfo& theInfo,
                TErr* theErr = NULL) 
    {}

    //! Write a MEDWrapped representation of MED_BALL to the MED file
    /*! This feature is supported since version 3.0 */
    virtual
    void
    SetBallInfo(const TBallInfo& theInfo,
                TErr* theErr = NULL) 
    {}

    //! Creates a MEDWrapper MED Balls representation
    /*! This feature is supported since version 3.0 */
    virtual 
    PBallInfo
    CrBallInfo(const PMeshInfo& theMeshInfo, 
               TInt             theNbBalls,
               EBooleen         theIsElemNum = eVRAI)
    {
      return PBallInfo();
    }

    //! Creates a MEDWrapper MED Balls representation
    /*! This feature is supported since version 3.0 */
    virtual
    PBallInfo 
    CrBallInfo(const PMeshInfo&  theMeshInfo, 
               const TIntVector& theNodes,
               TFloatVector&     theDiameters,
               const TIntVector& theFamilyNums = TIntVector(),
               const TIntVector& theElemNums = TIntVector())
    {
      return PBallInfo();
    }

    //! A copy-constructor for the MEDWrapped MED Balls representation
    virtual
    PBallInfo
    CrBallInfo(const PMeshInfo& theMeshInfo,
               const PBallInfo& theInfo)
    {
      return PBallInfo();
    }
    
    //! Read a MEDWrapped MED Balls representation from defined MED file
    /*! This feature is supported since version 3.0 */
    virtual
    PBallInfo
    GetPBallInfo(const PMeshInfo& theMeshInfo);

    //----------------------------------------------------------------------------
    //! Read number of MED FIELDS in defined MED Mesh
    virtual 
    TInt
    GetNbFields(TErr* theErr = NULL) = 0;
    
    //! Read number of components for the defined MED FIELD by its order number
    virtual
    TInt
    GetNbComp(TInt theFieldId,
              TErr* theErr = NULL) = 0;
    
    //! Read MEDWrapper MED FIELD representation by its order number
    virtual 
    void
    GetFieldInfo(TInt theFieldId, 
                 TFieldInfo& theInfo,
                 TErr* theErr = NULL) = 0;

    //! Write MEDWrapper MED FIELD representation into defined MED file
    virtual 
    void
    SetFieldInfo(const TFieldInfo& theInfo,
                 TErr* theErr = NULL) = 0;
    

    //! Creates a MEDWrapper MED FIELD representation
    virtual 
    PFieldInfo
    CrFieldInfo(const PMeshInfo& theMeshInfo, 
                TInt theNbComp = 0,
                ETypeChamp theType = eFLOAT64,
                const std::string& theValue = "",
                EBooleen theIsLocal = eVRAI,
                TInt theNbRef = 1) = 0;

    //! A copy-constructor for the MEDWrapper MED FIELD representation
    virtual 
    PFieldInfo
    CrFieldInfo(const PMeshInfo& theMeshInfo,
                const PFieldInfo& theInfo) = 0;
    
    //! Read a MEDWrapper MED FIELD representation from defined MED file
    PFieldInfo
    GetPFieldInfo(const PMeshInfo& theMeshInfo, 
                  TInt theId,
                  TErr* theErr = NULL);


    //----------------------------------------------------------------------------
    //! Read number of MED GAUSS in defined MED Mesh
    /*! This feature is support only for version of 2.2 and higher */
    virtual 
    TInt
    GetNbGauss(TErr* theErr = NULL)
    { 
      return TInt();
    }

    //! Read brief MED GAUSS information by its order number from defined MED Mesh
    /*! This feature is support only for version of 2.2 and higher */
    virtual 
    TGaussInfo::TInfo
    GetGaussPreInfo(TInt theId, 
                    TErr* theErr = NULL)
    {
      return TGaussInfo::TInfo( TGaussInfo::TKey(ePOINT1,""),0 );
    }
    
    //! Read a MEDWrapper MED GAUSS representation by its order number from defined MED file
    /*! This feature is support only for version of 2.2 and higher */
    virtual 
    void
    GetGaussInfo(TInt theId, 
                 TGaussInfo& theInfo,
                 TErr* theErr = NULL)
    {}

    //! Creates a MEDWrapper MED GAUSS representation
    /*! This feature is support only for version of 2.2 and higher */
    virtual
    PGaussInfo
    CrGaussInfo(const TGaussInfo::TInfo& theInfo,
                EModeSwitch theMode = eFULL_INTERLACE) = 0;


    //----------------------------------------------------------------------------
    //! Read number of MED TIMESTAMPS in defined MED Mesh
    /*!
      By the way some additional information can be obtained:
      - to what MED ENTITY the MED TIMESTAMP conntected to;
      - on what geometrical types the MED TIMESTAMP defined to.
    */
    virtual 
    TInt
    GetNbTimeStamps(const TFieldInfo& theInfo, 
                    const TEntityInfo& theEntityInfo,
                    EEntiteMaillage& theEntity,
                    TGeom2Size& theGeom2Size,
                    TErr* theErr = NULL) = 0;
    
    //! Read MEDWrapper MED TIMESTAMP representation by its order number
    virtual 
    void
    GetTimeStampInfo(TInt theTimeStampId, 
                     TTimeStampInfo& theInfo,
                     TErr* theErr = NULL) = 0;

    //! Creates a MEDWrapper MED TIMESTAMP representation
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
                    const TGeom2Gauss& theGeom2Gauss = TGeom2Gauss()) = 0;

    //! A copy-constructor for the MEDWrapper MED TIMESTAMP representation
    virtual 
    PTimeStampInfo 
    CrTimeStampInfo(const PFieldInfo& theFieldInfo,
                    const PTimeStampInfo& theInfo) = 0;
    
    //! Read MEDWrapper MED TIMESTAMP representation by its order number
    PTimeStampInfo
    GetPTimeStampInfo(const PFieldInfo& theFieldInfo,
                      EEntiteMaillage theEntity,
                      const TGeom2Size& theGeom2Size,
                      TInt theId,
                      TErr* theErr = NULL);
    

    //----------------------------------------------------------------------------
    //! Read number of MED PROFILES in defined MED Mesh
    virtual 
    TInt
    GetNbProfiles(TErr* theErr = NULL) = 0;

    //! Read brief MED PROFILE information by its order number from defined MED Mesh
    virtual 
    TProfileInfo::TInfo
    GetProfilePreInfo(TInt theId, 
                      TErr* theErr = NULL) = 0;

    //! Read a MEDWrapper MED PROFILE representation by its order number from defined MED file
    virtual 
    void
    GetProfileInfo(TInt theId, 
                   TProfileInfo& theInfo,
                   TErr* theErr = NULL) = 0;

    //! Creates a MEDWrapper MED PROFILE representation
    virtual
    PProfileInfo
    CrProfileInfo(const TProfileInfo::TInfo& theInfo,
                  EModeProfil theMode = eCOMPACT) = 0;

    //! Write a MEDWrapper MED PROFILE representation
    virtual
    void
    SetProfileInfo(const TProfileInfo& theInfo,
                   TErr* theErr = NULL) = 0;

    //! Read a MEDWrapper MED PROFILE representation by its order number from defined MED file
    PProfileInfo
    GetPProfileInfo(TInt theId,
                    EModeProfil theMode = eCOMPACT,
                    TErr* theErr = NULL);


    //----------------------------------------------------------------------------
    //! Read the values for MEDWrapper MED TIMESTAMP from defined MED file
    virtual 
    void
    GetTimeStampValue(const PTimeStampValueBase& theTimeStampValue,
                      const TMKey2Profile& theMKey2Profile,
                      const TKey2Gauss& theKey2Gauss,
                      TErr* theErr = NULL) = 0;
    
    //! Write the values for MEDWrapper MED TIMESTAMP to defined MED file
    virtual 
    void
    SetTimeStampValue(const PTimeStampValueBase& theTimeStampValue,
                      TErr* theErr = NULL) = 0;
    
    //! Creates the values for MEDWrapper MED TIMESTAMP representation
    virtual
    PTimeStampValueBase
    CrTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     ETypeChamp theTypeChamp,
                     const TGeom2Profile& theGeom2Profile = TGeom2Profile(),
                     EModeSwitch theMode = eFULL_INTERLACE) = 0;

    //! Creates the values for MEDWrapper MED TIMESTAMP representation
    virtual
    PTimeStampValueBase
    CrTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     const TGeom2Profile& theGeom2Profile = TGeom2Profile(),
                     EModeSwitch theMode = eFULL_INTERLACE);

    //! A copy-constructor for the values for MEDWrapper MED TIMESTAMP representation
    virtual 
    PTimeStampValueBase
    CrTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     const PTimeStampValueBase& theInfo,
                     ETypeChamp theTypeChamp) = 0;
    
    //! A copy-constructor for the values for MEDWrapper MED TIMESTAMP representation
    virtual 
    PTimeStampValueBase
    CrTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                     const PTimeStampValueBase& theInfo);
    
    //! Read the values for MEDWrapper MED TIMESTAMP from defined MED file
    PTimeStampValueBase
    GetPTimeStampValue(const PTimeStampInfo& theTimeStampInfo,
                       const TMKey2Profile& theMKey2Profile,
                       const TKey2Gauss& theKey2Gauss,
                       TErr* theErr = NULL);
    
    //----------------------------------------------------------------------------
    // Backward compatibility  declarations
    //! Read the values for MEDWrapper MED TIMESTAMP from defined MED file
    virtual 
    void
    GetTimeStampVal(const PTimeStampVal& theVal,
                    const TMKey2Profile& theMKey2Profile,
                    const TKey2Gauss& theKey2Gauss,
                    TErr* theErr = NULL);
    
    //! Write the values for MEDWrapper MED TIMESTAMP to defined MED file
    virtual 
    void
    SetTimeStamp(const PTimeStampVal& theVal,
                 TErr* theErr = NULL);
    
    //! Creates the values for MEDWrapper MED TIMESTAMP representation
    virtual
    PTimeStampVal
    CrTimeStampVal(const PTimeStampInfo& theTimeStampInfo,
                   const TGeom2Profile& theGeom2Profile = TGeom2Profile(),
                   EModeSwitch theMode = eFULL_INTERLACE);

    //! A copy-constructor for the values for MEDWrapper MED TIMESTAMP representation
    virtual 
    PTimeStampVal
    CrTimeStampVal(const PTimeStampInfo& theTimeStampInfo,
                   const PTimeStampVal& theInfo);
    
    //! Read the values for MEDWrapper MED TIMESTAMP from defined MED file
    PTimeStampVal
    GetPTimeStampVal(const PTimeStampInfo& theTimeStampInfo,
                     const TMKey2Profile& theMKey2Profile,
                     const TKey2Gauss& theKey2Gauss,
                     TErr* theErr = NULL);

    //----------------------------------------------------------------------------
    //! Read a MEDWrapper MED Grille representation from defined MED file
    /*! This feature is support only for version of 2.2 and higher */
    PGrilleInfo
    GetPGrilleInfo(const PMeshInfo& theMeshInfo);

    //! Read a MEDWrapper MED Grille representation from defined MED file
    /*! This feature is support only for version of 2.2 and higher */
    PGrilleInfo
    GetPGrilleInfo(const PMeshInfo& theMeshInfo,
                   const PGrilleInfo& theInfo);

    //! Read a MEDWrapper MED Grille representation from defined MED file
    /*! This feature is support only for version of 2.2 and higher */
    virtual
    void
    GetGrilleInfo(TGrilleInfo& theInfo,
                  TErr* theErr = NULL)
    {}

    //! Write the MEDWrapper MED Grille representation into defined MED file
    /*! This feature is support only for version of 2.2 and higher */
    virtual 
    void
    SetGrilleInfo(const TGrilleInfo& theInfo,
                  TErr* theErr = NULL)
    {}

    /*! This feature is support only for version of 2.2 and higher */
    virtual
    PGrilleInfo
    CrGrilleInfo(const PMeshInfo& theMeshInfo,
                 const PGrilleInfo& theGrilleInfo)
    {
      return PGrilleInfo();
    }

    /*! This feature is support only for version of 2.2 and higher */
    virtual
    PGrilleInfo
    CrGrilleInfo(const PMeshInfo& theMeshInfo)
    {
      return PGrilleInfo();
    }
    
    /*! This feature is support only for version of 2.2 and higher */
    virtual
    PGrilleInfo
    CrGrilleInfo(const PMeshInfo& theMeshInfo,
                 const EGrilleType& type)
    {
      return PGrilleInfo();
    }
    
    /*! This feature is support only for version of 2.2 and higher */
    virtual
    PGrilleInfo
    CrGrilleInfo(const PMeshInfo& theMeshInfo,
                 const EGrilleType& type,
                 const TInt& nbNodes)
    {
      return PGrilleInfo();
    }

    /*! This feature is support only for version of 2.2 and higher */
    virtual
    PGrilleInfo
    CrGrilleInfo(const PMeshInfo& theMeshInfo,
                 const EGrilleType& type,
                 const MED::TIntVector& nbNodeVec)
    {
      return PGrilleInfo();
    }

    /*! This feature is support only for version of 2.2 and higher */
    virtual
    void
    GetGrilleType(const TMeshInfo& theMeshInfo,
                  EGrilleType& type,
                  TErr* theErr = NULL)
    {
    }
    
  };


  //----------------------------------------------------------------------------
  //! This class provide thread-safety for MEDWrapper interaction
  class MEDWRAPPER_EXPORT TLockProxy
  {
    TLockProxy& operator=(const TLockProxy& );
    TWrapper* myWrapper;

  public:
    TLockProxy(TWrapper* theWrapper);

    ~TLockProxy();

    TWrapper * operator-> () const;
  };

#if 0
  //----------------------------------------------------------------------------
  //! To specialize the SharedPtr for TWrapper
  template<> 
  class MEDWRAPPER_EXPORT SharedPtr<TWrapper>: public boost::shared_ptr<TWrapper>
  {
  public:
    SharedPtr() {}

    template<class Y>
    explicit SharedPtr(Y * p): 
      boost::shared_ptr<TWrapper>(p) 
    {}

    template<class Y>
    SharedPtr(SharedPtr<Y> const & r):
      boost::shared_ptr<TWrapper>(boost::dynamic_pointer_cast<TWrapper,Y>(r))
    {}

    template<class Y>
    SharedPtr& 
    operator=(SharedPtr<Y> const & r)
    {
      SharedPtr<TWrapper>(r).swap(*this);
      return *this;
    }

    template<class Y> 
    SharedPtr& 
    operator()(Y * p) // Y must be complete
    {
      return operator=<Y>(SharedPtr<Y>(p));
    }

    template<class Y> 
    SharedPtr& 
    operator()(SharedPtr<Y> const & r) // Y must be complete
    {
      return operator=<Y>(SharedPtr<Y>(r));
    }

    TLockProxy operator-> () const // never throws
    {
      return TLockProxy(this->get());
    }
    
  protected:
    operator const TWrapper& () const;

    operator TWrapper& ();

    TWrapper& operator* () const;

    TWrapper * get() const // never throws
    {
      return boost::shared_ptr<TWrapper>::get();
    }
  };

  //----------------------------------------------------------------------------
  typedef SharedPtr<TWrapper> PWrapper;
#endif
}

#endif
