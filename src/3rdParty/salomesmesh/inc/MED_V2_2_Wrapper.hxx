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
#ifndef MED_V2_2_Wrapper_HeaderFile
#define MED_V2_2_Wrapper_HeaderFile

#ifdef WIN32
 #if defined MEDWRAPPER_V2_2_EXPORTS || defined MEDWrapper_V2_2_EXPORTS
  #if defined WIN32
   #define MED_V22_WRAPPER_EXPORT __declspec( dllexport )
  #else
   #define MED_V22_WRAPPER_EXPORT
  #endif
 #else
  #if defined WIN32
   #define MED_V22_WRAPPER_EXPORT __declspec( dllimport )
  #else
   #define MED_V22_WRAPPER_EXPORT
  #endif
 #endif
#else
 #define MED_V22_WRAPPER_EXPORT
#endif

#include "MED_Structures.hxx"
#include "MED_TWrapper.hxx"

namespace MED
{
  template<>
  TInt MED_V22_WRAPPER_EXPORT
  GetDESCLength<eV2_2>();
  
  template<>
  TInt MED_V22_WRAPPER_EXPORT
  GetIDENTLength<eV2_2>();
  
  template<>
  TInt MED_V22_WRAPPER_EXPORT
  GetNOMLength<eV2_2>();
  
  template<>
  TInt MED_V22_WRAPPER_EXPORT
  GetLNOMLength<eV2_2>();
  
  template<>
  TInt MED_V22_WRAPPER_EXPORT
  GetPNOMLength<eV2_2>();

  template<>
  void MED_V22_WRAPPER_EXPORT
  GetVersionRelease<eV2_2>(TInt& majeur, TInt& mineur, TInt& release);

  template<>
  TInt MED_V22_WRAPPER_EXPORT
  GetNbConn<eV2_2>(EGeometrieElement typmai,
                   EEntiteMaillage typent,
                   TInt mdim);

  namespace V2_2
  {
    //----------------------------------------------------------------------------
    class TFile;
    typedef boost::shared_ptr<TFile> PFile;
    
    typedef enum {eLECTURE, eLECTURE_ECRITURE, eLECTURE_AJOUT, eCREATION} EModeAcces; 

    //----------------------------------------------------------------------------
    class MED_V22_WRAPPER_EXPORT TVWrapper: public MED::TTWrapper<eV2_2>
    {
      TVWrapper();
      TVWrapper(const TVWrapper&);
      TVWrapper& operator=(const TVWrapper&);
      
    public:
      TVWrapper(const std::string& theFileName);

      //----------------------------------------------------------------------------
      virtual 
      TInt
      GetNbMeshes(TErr* theErr = NULL);
      
      virtual
      void
      GetMeshInfo(TInt theMeshId, MED::TMeshInfo&,
                               TErr* theErr = NULL);

      virtual
      void
      SetMeshInfo(const MED::TMeshInfo& theInfo,
                  TErr* theErr = NULL);
      
      void SetMeshInfo(const MED::TMeshInfo& theInfo,
                       EModeAcces theMode,
                       TErr* theErr = NULL);
      
      
      //----------------------------------------------------------------------------
      virtual
      TInt
      GetNbFamilies(const MED::TMeshInfo& theMeshInfo,
                    TErr* theErr = NULL);

      virtual 
      TInt
      GetNbFamAttr(TInt theFamId, 
                   const MED::TMeshInfo& theInfo,
                   TErr* theErr = NULL);
      
      virtual
      TInt
      GetNbFamGroup(TInt theFamId, 
                    const MED::TMeshInfo& theInfo,
                    TErr* theErr = NULL);
      
      virtual
      void
      GetFamilyInfo(TInt theFamId, 
                    MED::TFamilyInfo& theInfo,
                    TErr* theErr = NULL);
      
      virtual
      void
      SetFamilyInfo(const MED::TFamilyInfo& theInfo,
                    TErr* theErr = NULL);
      
      void 
      SetFamilyInfo(const MED::TFamilyInfo& theInfo,
                    EModeAcces theMode,
                    TErr* theErr = NULL);
      
      
      //----------------------------------------------------------------------------
      virtual
      void
      GetNames(TElemInfo& theInfo,
               TInt nb,
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom,
               TErr* theErr = NULL);

      virtual
      void
      GetNumeration(TElemInfo& theInfo,
                    TInt nb,
                    EEntiteMaillage theEntity, 
                    EGeometrieElement theGeom,
                    TErr* theErr = NULL);

      virtual
      void
      GetFamilies(TElemInfo& theInfo,
                  TInt nb,
                  EEntiteMaillage theEntity, 
                  EGeometrieElement theGeom,
                  TErr* theErr = NULL);

      virtual
      void
      SetNames(const TElemInfo& theInfo,
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom,
               TErr* theErr = NULL);

      void
      SetNames(const TElemInfo& theInfo,
               EModeAcces theMode,
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom,
               TErr* theErr = NULL);
      
      virtual
      void
      SetNumeration(const TElemInfo& theInfo,
                    EEntiteMaillage theEntity, 
                    EGeometrieElement theGeom,
                    TErr* theErr = NULL);
      void
      SetNumeration(const TElemInfo& theInfo,
                    EModeAcces theMode,
                    EEntiteMaillage theEntity, 
                    EGeometrieElement theGeom,
                    TErr* theErr = NULL);
      
      virtual
      void
      SetFamilies(const TElemInfo& theInfo,
                  EEntiteMaillage theEntity, 
                  EGeometrieElement theGeom,
                  TErr* theErr = NULL);
      void
      SetFamilies(const TElemInfo& theInfo,
                  EModeAcces theMode,
                  EEntiteMaillage theEntity, 
                  EGeometrieElement theGeom,
                  TErr* theErr = NULL);

      //----------------------------------------------------------------------------
      virtual
      TInt
      GetNbNodes(const MED::TMeshInfo& theMeshInfo,
                 TErr* theErr = NULL)
      {
        return GetNbNodes(theMeshInfo,eCOOR,theErr);
      }

      TInt
      GetNbNodes(const MED::TMeshInfo& theMeshInfo,
                 ETable theTable,
                 TErr* theErr = NULL);
      
      virtual 
      void
      GetNodeInfo(MED::TNodeInfo& theInfo,
                  TErr* theErr = NULL);
      
      virtual
      void
      SetNodeInfo(const MED::TNodeInfo& theInfo,
                  TErr* theErr = NULL);
      
      void
      SetNodeInfo(const MED::TNodeInfo& theInfo,
                  EModeAcces theMode,
                  TErr* theErr = NULL);
      
      //----------------------------------------------------------------------------
      virtual
      void
      GetPolygoneInfo(TPolygoneInfo& theInfo,
                      TErr* theErr = NULL);

      virtual 
      void
      SetPolygoneInfo(const TPolygoneInfo& theInfo,
                                   TErr* theErr = NULL);
      
      void
      SetPolygoneInfo(const MED::TPolygoneInfo& theInfo,
                      EModeAcces theMode,
                      TErr* theErr = NULL);

      virtual 
      TInt
      GetNbPolygones(const TMeshInfo& theMeshInfo,
                   EEntiteMaillage theEntity,
                   EGeometrieElement theGeom,
                   EConnectivite theConnMode = eNOD,
                   TErr* theErr = NULL);
      
      virtual 
      TInt
      GetPolygoneConnSize(const TMeshInfo& theMeshInfo,
                        EEntiteMaillage theEntity,
                        EGeometrieElement theGeom,
                        EConnectivite theConnMode = eNOD,
                        TErr* theErr = NULL);

      //----------------------------------------------------------------------------
      virtual 
      void
      GetPolyedreInfo(TPolyedreInfo& theInfo,
                      TErr* theErr = NULL);
      
      virtual
      void
      SetPolyedreInfo(const TPolyedreInfo& theInfo,
                      TErr* theErr = NULL);
      
      void
      SetPolyedreInfo(const MED::TPolyedreInfo& theInfo,
                      EModeAcces theMode,
                      TErr* theErr = NULL);
      
      virtual
      TInt
      GetNbPolyedres(const TMeshInfo& theMeshInfo,
                     EEntiteMaillage,
                     EGeometrieElement,
                     EConnectivite,
                     TErr* theErr = NULL);
      
      virtual 
      void
      GetPolyedreConnSize(const TMeshInfo& theMeshInfo,
                          TInt& theNbFaces,
                          TInt& theConnSize,
                          EConnectivite theConnMode = eNOD,
                          TErr* theErr = NULL);
      
      //----------------------------------------------------------------------------
      virtual
      TEntityInfo
      GetEntityInfo(const MED::TMeshInfo& theMeshInfo,
                    EConnectivite theConnMode = eNOD,
                    TErr* theErr = NULL);
      
      virtual 
      TInt
      GetNbCells(const MED::TMeshInfo& theMeshInfo, 
                 EEntiteMaillage, 
                 EGeometrieElement, 
                 EConnectivite theConnMode = eNOD,
                 TErr* theErr = NULL);
      
      virtual
      void
      GetCellInfo(MED::TCellInfo& theInfo,
                  TErr* theErr = NULL);

      virtual 
      void
      SetCellInfo(const MED::TCellInfo& theInfo,
                  TErr* theErr = NULL);
      
      void 
      SetCellInfo(const MED::TCellInfo& theInfo,
                  EModeAcces theMode,
                  TErr* theErr = NULL);
      
      //----------------------------------------------------------------------------
      //! Read geom type of MED_BALL structural element
      EGeometrieElement GetBallGeom(const TMeshInfo& theMeshInfo);
      
      //! Read number of balls in the Mesh
      virtual TInt GetNbBalls(const TMeshInfo& theMeshInfo);

      //! Read a MEDWrapped representation of MED_BALL from the MED file
      virtual void GetBallInfo(TBallInfo& theInfo, TErr* theErr = NULL);

      //! Write a MEDWrapped representation of MED_BALL to the MED file
      virtual void  SetBallInfo(const TBallInfo& theInfo, TErr* theErr);

      //! Write a MEDWrapped representation of MED_BALL to the MED file
      void  SetBallInfo(const TBallInfo& theInfo, EModeAcces theMode, TErr* theErr);

      //----------------------------------------------------------------------------
      virtual
      TInt
      GetNbFields(TErr* theErr = NULL);
      
      virtual
      TInt
      GetNbComp(TInt theFieldId,
                TErr* theErr = NULL);
      
      virtual 
      void
      GetFieldInfo(TInt theFieldId, 
                   MED::TFieldInfo& theInfo,
                   TErr* theErr = NULL);
      
      virtual
      void
      SetFieldInfo(const MED::TFieldInfo& theInfo,
                   TErr* theErr = NULL);
      
      void
      SetFieldInfo(const MED::TFieldInfo& theInfo,
                   EModeAcces theMode,
                   TErr* theErr = NULL);
      
      //----------------------------------------------------------------------------
      virtual 
      TInt
      GetNbGauss(TErr* theErr = NULL);

      virtual 
      TGaussInfo::TInfo
      GetGaussPreInfo(TInt theId, 
                      TErr* theErr = NULL);

      virtual 
      void
      GetGaussInfo(TInt theId, 
                   TGaussInfo& theInfo,
                   TErr* theErr = NULL);

      //----------------------------------------------------------------------------
      virtual 
      TInt
      GetNbProfiles(TErr* theErr = NULL);

      virtual 
      TProfileInfo::TInfo
      GetProfilePreInfo(TInt theId, 
                        TErr* theErr = NULL);
      
      virtual 
      void
      GetProfileInfo(TInt theId, 
                     TProfileInfo& theInfo,
                     TErr* theErr = NULL);

      virtual
      void
      SetProfileInfo(const TProfileInfo& theInfo,
                     TErr*               theErr = NULL);

      void
      SetProfileInfo(const TProfileInfo& theInfo,
                     EModeAcces          theMode,
                     TErr*               theErr = NULL);

      //----------------------------------------------------------------------------
      virtual 
      TInt
      GetNbTimeStamps(const MED::TFieldInfo& theInfo, 
                      const MED::TEntityInfo& theEntityInfo,
                      EEntiteMaillage& theEntity,
                      TGeom2Size& theGeom2Size,
                      TErr* theErr = NULL);
      
      virtual 
      void
      GetTimeStampInfo(TInt theTimeStampId, 
                       MED::TTimeStampInfo& theInfo,
                       TErr* theErr = NULL);
      
      virtual
      void
      GetTimeStampValue(const PTimeStampValueBase& theTimeStampValue,
                        const TMKey2Profile& theMKey2Profile,
                        const TKey2Gauss& theKey2Gauss,
                        TErr* theErr = NULL);
      
      virtual
      void
      SetTimeStampValue(const PTimeStampValueBase& theTimeStampValue,
                        TErr* theErr = NULL);
      
      void 
      SetTimeStampValue(const PTimeStampValueBase& theTimeStampValue,
                        EModeAcces theMode,
                        TErr* theErr = NULL);
      

      //----------------------------------------------------------------------------
      virtual
      void
      GetGrilleInfo(TGrilleInfo& theGrilleInfo,
                    TErr* theErr = NULL);
      
      virtual
      void
      SetGrilleInfo(const MED::TGrilleInfo& theGrilleInfo,
                    TErr* theErr = NULL);

      void
      SetGrilleInfo(const MED::TGrilleInfo& theGrilleInfo,
                    EModeAcces theMode,
                    TErr* theErr = NULL);

      virtual
      void
      GetGrilleType(const MED::TMeshInfo& theMeshInfo,
                    EGrilleType& type,
                    TErr* theErr = NULL);

      void
      GetGrilleStruct(const MED::TMeshInfo& theMeshInfo,
                      TIntVector& theStruct,
                      TErr* theErr = NULL);

    protected:
      PFile myFile;
    };
  }
}

#endif
