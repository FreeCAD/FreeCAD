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
#ifndef MED_Algorithm_HeaderFile
#define MED_Algorithm_HeaderFile

#include "MED_WrapperBase.hxx"
#include "MED_Structures.hxx"

#include <set>

namespace MED
{
  //---------------------------------------------------------------
  typedef std::map<EGeometrieElement,PElemInfo> TGeom2ElemInfo;
  typedef std::map<EEntiteMaillage,TGeom2ElemInfo> TEntity2TGeom2ElemInfo;

  //! Get set of TElemInfo by its geometrical type and corresponding MED ENTITY
  MEDWRAPPER_EXPORT
  TEntity2TGeom2ElemInfo
  GetEntity2TGeom2ElemInfo(const PWrapper& theWrapper, 
                           const PMeshInfo& theMeshInfo,
                           const MED::TEntityInfo& theEntityInfo);


  //---------------------------------------------------------------
  typedef std::set<PFamilyInfo> TFamilyInfoSet;

  //! Read set of MED FAMILIES for defined MED file
  MEDWRAPPER_EXPORT
  TFamilyInfoSet
  GetFamilyInfoSet(const PWrapper& theWrapper, 
                   const PMeshInfo& theMeshInfo);
  

  //---------------------------------------------------------------
  typedef boost::tuple<PFamilyInfo,TInt> TFamilyTSize;

  bool
  operator<(const TFamilyTSize& theLeft, const TFamilyTSize& theRight);
  typedef std::set<TFamilyTSize> TFamilyTSizeSet;


  //---------------------------------------------------------------
  typedef std::map<EEntiteMaillage,TFamilyTSizeSet> TEntity2FamilySet;
  
  //! Split set of MED FAMILIES by corresponding MED ENTITY
  MEDWRAPPER_EXPORT
  TEntity2FamilySet
  GetEntity2FamilySet(const PWrapper& theWrapper, 
                      const TEntity2TGeom2ElemInfo& theEntity2TGeom2ElemInfo,
                      const TFamilyInfoSet& theFamilyInfoSet);
  

  //---------------------------------------------------------------
  typedef std::map<std::string,TFamilyInfoSet> TGroupInfo;
  
  //! Split the input set of MED FAMILIES by corresponding MED GROUPS
  MEDWRAPPER_EXPORT
  TGroupInfo
  GetGroupInfo(const TFamilyInfoSet& theFamilyInfoSet);
  
  
  //---------------------------------------------------------------
  typedef std::set<PTimeStampInfo> TTimeStampInfoSet;
  typedef std::map<PFieldInfo,TTimeStampInfoSet> TFieldInfo2TimeStampInfoSet;

  //! Read set of MED TIMESTAMPS groupped by corresponding MED FIELDS
  MEDWRAPPER_EXPORT
  TFieldInfo2TimeStampInfoSet
  GetFieldInfo2TimeStampInfoSet(const PWrapper& theWrapper, 
                                const PMeshInfo& theMeshInfo,
                                const MED::TEntityInfo& theEntityInfo);
  

  //---------------------------------------------------------------
  typedef std::map<EEntiteMaillage,TFieldInfo2TimeStampInfoSet> TEntite2TFieldInfo2TimeStampInfoSet;

  //! Split the input set of MED TIMESTAMPS by corresponding MED FIELDS and MED ENTITIES
  MEDWRAPPER_EXPORT
  TEntite2TFieldInfo2TimeStampInfoSet
  GetEntite2TFieldInfo2TimeStampInfoSet(const TFieldInfo2TimeStampInfoSet& theFieldInfo2TimeStampInfoSet);


  //---------------------------------------------------------------
  typedef std::map<TGaussInfo::TKey,PGaussInfo,TGaussInfo::TLess> TKey2Gauss;

  //! Read set of MED GAUSS
  MEDWRAPPER_EXPORT
  TKey2Gauss
  GetKey2Gauss(const PWrapper& theWrapper, 
               TErr* theErr = NULL,
               EModeSwitch theMode = eFULL_INTERLACE);


  //---------------------------------------------------------------
  //! Get MED PROFILE by its name
  MEDWRAPPER_EXPORT
  PProfileInfo
  GetProfileInfo(const PWrapper& theWrapper, 
                 const std::string& theProfileName,
                 TErr* theErr = NULL,
                 EModeProfil theMode = eCOMPACT);


  //---------------------------------------------------------------
  typedef std::map<TProfileInfo::TKey,PProfileInfo> TKey2Profile;
  typedef boost::tuple<EModeProfil,TKey2Profile> TMKey2Profile;

  //! Read set of MED PROFILES
  MEDWRAPPER_EXPORT
  TMKey2Profile
  GetMKey2Profile(const PWrapper& theWrapper, 
                  TErr* theErr = NULL,
                  EModeProfil theMode = eCOMPACT);

  //---------------------------------------------------------------
  //! Get Entity for Grille by family id.
  MEDWRAPPER_EXPORT
  EEntiteMaillage
  GetEntityByFamilyId(PGrilleInfo& theInfo,
                      TInt theId);

  typedef std::map<TInt,TInt> TFamilyID2NbCells;
  
  //! Get Number of cells for theId family, for Grille
  MEDWRAPPER_EXPORT
  TFamilyID2NbCells
  GetFamilyID2NbCells(PGrilleInfo& theInfo);

  //! Convert eNOEUD_ELEMENT to eMAILLE
  MEDWRAPPER_EXPORT
  EEntiteMaillage
  ConvertEntity(const EEntiteMaillage& aEntity);

}

#endif
