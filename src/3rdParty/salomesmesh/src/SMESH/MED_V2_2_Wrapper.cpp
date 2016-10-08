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

#include "MED_V2_2_Wrapper.hxx"
#include "MED_Algorithm.hxx"
#include "MED_Utilities.hxx"

#include <med.h>
#include <med_err.h>

#ifdef _DEBUG_
static int MYDEBUG = 0;
// #else
// static int MYDEBUG = 0;
#endif

#define MED_VERSION_HEX    (MED_NUM_MAJEUR << 16 | MED_NUM_MINEUR << 8 | MED_NUM_RELEASE)


namespace MED
{
  template<>
  TInt
  GetDESCLength<eV2_2>()
  {
    return 200;
  }

  template<>
  TInt
  GetIDENTLength<eV2_2>()
  {
    return 8;
  }

  template<>
  TInt
  GetNOMLength<eV2_2>()
  {
    return 64;
  }

  template<>
  TInt
  GetLNOMLength<eV2_2>()
  {
    return 80;
  }

  template<>
  TInt
  GetPNOMLength<eV2_2>()
  {
    return 16;
  }

  template<>
  void
  GetVersionRelease<eV2_2>(TInt& majeur, TInt& mineur, TInt& release)
  {
    majeur=MED_MAJOR_NUM;
    mineur=MED_MINOR_NUM;
    release=MED_RELEASE_NUM;
  }

  template<>
  TInt
  GetNbConn<eV2_2>(EGeometrieElement typmai,
                   EEntiteMaillage typent,
                   TInt mdim)
  {
    return typmai%100;
  }

  namespace V2_2
  {

    //---------------------------------------------------------------
    class TFile{
      TFile();
      TFile(const TFile&);
      
    public:
      TFile(const std::string& theFileName): 
        myCount(0),
        myFid(0), 
        myFileName(theFileName)
      {}
      
      ~TFile()
      { 
        Close();
      }
      
      void
      Open(EModeAcces theMode, TErr* theErr = NULL)
      {
        if(myCount++ == 0){
          const char* aFileName = myFileName.c_str();
          myFid = MEDfileOpen(aFileName,med_access_mode(theMode));
        }
        if(theErr)
          *theErr = TErr(myFid);
        else if(myFid < 0)
          EXCEPTION(std::runtime_error,"TFile - MEDfileOpen('"<<myFileName<<"',"<<theMode<<")");
      }

      const TIdt& Id() const 
      { 
        if(myFid < 0)
          EXCEPTION(std::runtime_error,"TFile - GetFid() < 0");
        return myFid;
      }

      void Close()
      { 
        if(--myCount == 0)
          MEDfileClose(myFid);
      }

    protected:
      TInt myCount;
      TIdt myFid;
      std::string myFileName;
    };


    //---------------------------------------------------------------
    class TFileWrapper
    {
      PFile myFile;

    public:
      TFileWrapper(const PFile& theFile, EModeAcces theMode, TErr* theErr = NULL): 
        myFile(theFile)
      {
        myFile->Open(theMode,theErr);
      }
      
      ~TFileWrapper()
      {
        myFile->Close();
      }
    };


    //---------------------------------------------------------------
    TVWrapper::TVWrapper(const std::string& theFileName):
      myFile(new TFile(theFileName))
    {
      TErr aRet;
      myFile->Open( eLECTURE_ECRITURE, &aRet );
      // if(aRet < 0)
      //   myFile->Close();
      //   myFile->Open( eLECTURE_AJOUT, &aRet );
      // }
      if(aRet < 0) {
        myFile->Close();
        myFile->Open( eLECTURE, &aRet );
      }
      if(aRet < 0) {
        myFile->Close();
        myFile->Open( eCREATION, &aRet );
      }
    }


    //----------------------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbMeshes(TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return -1;
      
      return MEDnMesh(myFile->Id());
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetMeshInfo(TInt theMeshId, 
                  MED::TMeshInfo& theInfo,
                  TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      TValueHolder<TString, char> aMeshName(theInfo.myName);
      TValueHolder<TInt, med_int> aDim(theInfo.myDim);
      TValueHolder<TInt, med_int> aSpaceDim(theInfo.mySpaceDim);
      TValueHolder<EMaillage, med_mesh_type> aType(theInfo.myType);
      char dtunit[MED_SNAME_SIZE+1];
      med_sorting_type sorttype;
      med_int nstep;
      med_axis_type at;
      int naxis=MEDmeshnAxis(myFile->Id(),theMeshId);
      char *axisname=new char[naxis*MED_SNAME_SIZE+1];
      char *axisunit=new char[naxis*MED_SNAME_SIZE+1];
      TErr aRet = MEDmeshInfo(myFile->Id(),
                              theMeshId,
                              &aMeshName,
                              &aSpaceDim,
                              &aDim,
                              &aType,
                              &theInfo.myDesc[0],
                              dtunit,
                              &sorttype,
                              &nstep,
                              &at,
                              axisname,
                              axisunit);
      delete [] axisname;
      delete [] axisunit;
      if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetMeshInfo - MEDmeshInfo(...)");
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetMeshInfo(const MED::TMeshInfo& theInfo,
                  EModeAcces theMode,
                  TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      MED::TMeshInfo& anInfo = const_cast<MED::TMeshInfo&>(theInfo);
      
      TValueHolder<TString, char> aMeshName(anInfo.myName);
      TValueHolder<TInt, med_int> aDim(anInfo.myDim);
      TValueHolder<TInt, med_int> aSpaceDim(anInfo.mySpaceDim);
      TValueHolder<EMaillage, med_mesh_type> aType(anInfo.myType);
      TValueHolder<TString, char> aDesc(anInfo.myDesc);

      char *nam=new char[aSpaceDim*MED_SNAME_SIZE+1];
      std::fill(nam,nam+aSpaceDim*MED_SNAME_SIZE+1,'\0');
      char *unit=new char[aSpaceDim*MED_SNAME_SIZE+1];
      std::fill(unit,unit+aSpaceDim*MED_SNAME_SIZE+1,'\0');
      TErr aRet = MEDmeshCr(myFile->Id(),
                            &aMeshName,
                            aSpaceDim,
                            aDim,
                            aType,
                            &aDesc,
                            "",
                            MED_SORT_DTIT,
                            MED_CARTESIAN,
                            nam,
                            unit);
      delete [] nam;
      delete [] unit;
      
      //if(aRet == 0)
      //  aRet = MEDunvCr(myFile->Id(),&aMeshName);
      
      INITMSG(MYDEBUG,"TVWrapper::SetMeshInfo - MED_MODE_ACCES = "<<theMode<<"; aRet = "<<aRet<<std::endl);
      
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetMeshInfo - MEDmeshCr(...)");
    }
    
    
    //----------------------------------------------------------------------------
    void 
    TVWrapper
    ::SetMeshInfo(const MED::TMeshInfo& theInfo,
                  TErr* theErr)
    {
      TErr aRet;
      SetMeshInfo(theInfo,eLECTURE_ECRITURE,&aRet);
      
      if(aRet < 0)
        SetMeshInfo(theInfo,eLECTURE_AJOUT,&aRet);

      if(aRet < 0)
        SetMeshInfo(theInfo,eCREATION,&aRet);

      if(theErr)
        *theErr = aRet;
    }
    
    
    //----------------------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbFamilies(const MED::TMeshInfo& theInfo,
                    TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return -1;
      
      MED::TMeshInfo& anInfo = const_cast<MED::TMeshInfo&>(theInfo);
      TValueHolder<TString, char> aName(anInfo.myName);
      return MEDnFamily(myFile->Id(),&aName);
    }
    
    
    //----------------------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbFamAttr(TInt theFamId, 
                   const MED::TMeshInfo& theInfo,
                   TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return -1;
      
      MED::TMeshInfo& anInfo = const_cast<MED::TMeshInfo&>(theInfo);

      TValueHolder<TString, char> aName(anInfo.myName);

      return MEDnFamily23Attribute(myFile->Id(),&aName,theFamId);
    }
    
    
    //----------------------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbFamGroup(TInt theFamId, 
                    const MED::TMeshInfo& theInfo,
                    TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return -1;
      
      MED::TMeshInfo& anInfo = const_cast<MED::TMeshInfo&>(theInfo);

      TValueHolder<TString, char> aName(anInfo.myName);

      return MEDnFamilyGroup(myFile->Id(),&aName,theFamId);
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetFamilyInfo(TInt theFamId, 
                    MED::TFamilyInfo& theInfo,
                    TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      MED::TMeshInfo& aMeshInfo = *theInfo.myMeshInfo;
      
      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      TValueHolder<TString, char> aFamilyName(theInfo.myName);
      TValueHolder<TInt, med_int> aFamilyId(theInfo.myId);
      TValueHolder<TFamAttr, med_int> anAttrId(theInfo.myAttrId);
      TValueHolder<TFamAttr, med_int> anAttrVal(theInfo.myAttrVal);
      TValueHolder<TString, char> anAttrDesc(theInfo.myAttrDesc);
      TValueHolder<TString, char> aGroupNames(theInfo.myGroupNames);
      
      TErr aRet = MEDfamily23Info(myFile->Id(),
                                  &aMeshName,
                                  theFamId,
                                  &aFamilyName,
                                  &anAttrId,
                                  &anAttrVal,
                                  &anAttrDesc,
                                  &aFamilyId,
                                  &aGroupNames);

      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetFamilyInfo - MEDfamily23Info(...) - "<<
                  " aMeshInfo.myName = '"<<&aMeshName<<
                  "'; theFamId = "<<theFamId<<
                  "; theInfo.myNbGroup = "<<theInfo.myNbGroup<<
                  "; theInfo.myNbAttr = "<<theInfo.myNbAttr);
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetFamilyInfo(const MED::TFamilyInfo& theInfo,
                    EModeAcces theMode,
                    TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      MED::TFamilyInfo& anInfo = const_cast<MED::TFamilyInfo&>(theInfo);
      MED::TMeshInfo& aMeshInfo = *anInfo.myMeshInfo;
      
      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      TValueHolder<TString, char> aFamilyName(anInfo.myName);
      TValueHolder<TInt, med_int> aFamilyId(anInfo.myId);
      TValueHolder<TFamAttr, med_int> anAttrId(anInfo.myAttrId);
      TValueHolder<TFamAttr, med_int> anAttrVal(anInfo.myAttrVal);
      TValueHolder<TInt, med_int> aNbAttr(anInfo.myNbAttr);
      TValueHolder<TString, char> anAttrDesc(anInfo.myAttrDesc);
      TValueHolder<TInt, med_int> aNbGroup(anInfo.myNbGroup);
      TValueHolder<TString, char> aGroupNames(anInfo.myGroupNames);

      TErr aRet = MEDfamilyCr(myFile->Id(),
                              &aMeshName, 
                              &aFamilyName,
                              aFamilyId,
                              aNbGroup,
                              &aGroupNames);                 

      INITMSG(MYDEBUG,"TVWrapper::SetFamilyInfo - MED_MODE_ACCES = "<<theMode<<"; aRet = "<<aRet<<std::endl);
      
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetFamilyInfo - MEDfamilyCr(...)");
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetFamilyInfo(const MED::TFamilyInfo& theInfo,
                    TErr* theErr)
    {
      TErr aRet;
      SetFamilyInfo(theInfo,eLECTURE_ECRITURE,&aRet);
      
      if(aRet < 0)
        SetFamilyInfo(theInfo,eLECTURE_AJOUT,&aRet);

      if(theErr)
        *theErr = aRet;
    }
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetNames(TElemInfo&        theInfo,
               TInt              theNb,
               EEntiteMaillage   theEntity, 
               EGeometrieElement theGeom,
               TErr*             theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      if ( theGeom == eBALL )
        theGeom = GetBallGeom( theInfo.myMeshInfo );

      MED::TMeshInfo& aMeshInfo = *theInfo.myMeshInfo;

      TValueHolder<TString, char>                        aMeshName  (aMeshInfo.myName);
      TValueHolder<TString, char>                        anElemNames(theInfo.myElemNames);
      TValueHolder<EEntiteMaillage, med_entity_type>     anEntity   (theEntity);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom      (theGeom);
      
      TErr aRet = MEDmeshEntityNameRd(myFile->Id(),
                                      &aMeshName,
                                      MED_NO_DT,
                                      MED_NO_IT,
                                      anEntity,
                                      aGeom,
                                      &anElemNames);

      theInfo.myIsElemNames = aRet != 0? eFAUX : eVRAI ;

      if(theErr)
        *theErr = aRet;
    }

    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetNumeration(TElemInfo&        theInfo,
                    TInt              theNb,
                    EEntiteMaillage   theEntity, 
                    EGeometrieElement theGeom,
                    TErr*             theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      if ( theGeom == eBALL )
        theGeom = GetBallGeom( theInfo.myMeshInfo );

      MED::TMeshInfo& aMeshInfo = *theInfo.myMeshInfo;
      
      TValueHolder<TString, char>                        aMeshName(aMeshInfo.myName);
      TValueHolder<TElemNum, med_int>                    anElemNum(theInfo.myElemNum);
      TValueHolder<EEntiteMaillage, med_entity_type>     anEntity (theEntity);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom    (theGeom);
      
      TErr aRet = MEDmeshEntityNumberRd(myFile->Id(),
                                        &aMeshName,
                                        MED_NO_DT,
                                        MED_NO_IT,
                                        anEntity,
                                        aGeom,
                                        &anElemNum);

      theInfo.myIsElemNum = aRet != 0? eFAUX : eVRAI;

      if(theErr)
        *theErr = aRet;
    }

    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetFamilies(TElemInfo&        theInfo,
                  TInt              theNb,
                  EEntiteMaillage   theEntity, 
                  EGeometrieElement theGeom,
                  TErr*             theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;

      if ( theGeom == eBALL )
        theGeom = GetBallGeom( theInfo.myMeshInfo );
      
      MED::TMeshInfo& aMeshInfo = *theInfo.myMeshInfo;

      TValueHolder<TString, char>                        aMeshName(aMeshInfo.myName);
      TValueHolder<TElemNum, med_int>                    aFamNum  (theInfo.myFamNum);
      TValueHolder<EEntiteMaillage, med_entity_type>     anEntity (theEntity);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom    (theGeom);
      
      TErr aRet = MEDmeshEntityFamilyNumberRd(myFile->Id(),
                                              &aMeshName,
                                              MED_NO_DT,
                                              MED_NO_IT,
                                              anEntity,
                                              aGeom,
                                              &aFamNum);

      if(aRet < 0)
      {
//        if (aRet == MED_ERR_DOESNTEXIST) // --- only valid with MED3.x files
          {
            int aSize = (int)theInfo.myFamNum->size();
            theInfo.myFamNum->clear();
            theInfo.myFamNum->resize(aSize,0);
            aRet = 0;
          }
//        else
//          EXCEPTION(std::runtime_error,"GetGrilleInfo - MEDmeshEntityFamilyNumberRd(...) of CELLS");
      }
      if(theErr)
        *theErr = aRet;
    }


    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetNames(const TElemInfo& theInfo,
               EEntiteMaillage theEntity, 
               EGeometrieElement theGeom,
               TErr* theErr)
    { 
      SetNames(theInfo,eLECTURE_ECRITURE,theEntity,theGeom,theErr);
    }


    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetNames(const TElemInfo&  theInfo,
               EModeAcces        theMode,
               EEntiteMaillage   theEntity, 
               EGeometrieElement theGeom,
               TErr*             theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;

      if ( theGeom == eBALL )
        theGeom = GetBallGeom( theInfo.myMeshInfo );

      MED::TElemInfo& anInfo = const_cast<MED::TElemInfo&>(theInfo);
      MED::TMeshInfo& aMeshInfo = *anInfo.myMeshInfo;

      TErr aRet = 0;
      if(theInfo.myIsElemNames)
      {
        TValueHolder<TString, char>                        aMeshName  (aMeshInfo.myName);
        TValueHolder<TString, char>                        anElemNames(anInfo.myElemNames);
        TValueHolder<EEntiteMaillage, med_entity_type>     anEntity   (theEntity);
        TValueHolder<EGeometrieElement, med_geometry_type> aGeom      (theGeom);
      
        aRet  = MEDmeshEntityNameWr(myFile->Id(),
                                    &aMeshName,
                                    MED_NO_DT,
                                    MED_NO_IT,
                                    anEntity,
                                    aGeom, 
                                    (TInt)anInfo.myElemNames->size(),
                                    &anElemNames);
        if(theErr) 
          *theErr = aRet;
        else if(aRet < 0)
          EXCEPTION(std::runtime_error,"SetNames - MEDmeshEntityNameWr(...)");
      }
    }


    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetNumeration(const TElemInfo& theInfo,
                    EEntiteMaillage theEntity, 
                    EGeometrieElement theGeom,
                    TErr* theErr)
    { 
      SetNumeration(theInfo,eLECTURE_ECRITURE,theEntity,theGeom,theErr);
    }


    //----------------------------------------------------------------------------
    void 
    TVWrapper
    ::SetNumeration(const TElemInfo&  theInfo,
                    EModeAcces        theMode,
                    EEntiteMaillage   theEntity, 
                    EGeometrieElement theGeom,
                    TErr*             theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;

      if ( theGeom == eBALL )
        theGeom = GetBallGeom( theInfo.myMeshInfo );

      MED::TElemInfo& anInfo = const_cast<MED::TElemInfo&>(theInfo);
      MED::TMeshInfo& aMeshInfo = *anInfo.myMeshInfo;

      TErr aRet = 0;
      if(theInfo.myIsElemNum)
      {
        TValueHolder<TString, char>                        aMeshName(aMeshInfo.myName);
        TValueHolder<TElemNum, med_int>                    anElemNum(anInfo.myElemNum);
        TValueHolder<EEntiteMaillage, med_entity_type>     anEntity (theEntity);
        TValueHolder<EGeometrieElement, med_geometry_type> aGeom    (theGeom);
      
        aRet  = MEDmeshEntityNumberWr(myFile->Id(),
                                      &aMeshName,
                                      MED_NO_DT,
                                      MED_NO_IT,
                                      anEntity,
                                      aGeom,
                                      (TInt)anInfo.myElemNum->size(),
                                      &anElemNum);
        if(theErr) 
          *theErr = aRet;
        else if(aRet < 0)
          EXCEPTION(std::runtime_error,"SetNumeration - MEDmeshEntityNumberWr(...)");
      }
    }

    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetFamilies(const TElemInfo&  theInfo,
                  EEntiteMaillage   theEntity, 
                  EGeometrieElement theGeom,
                  TErr*             theErr)
    { 
      SetFamilies(theInfo,eLECTURE_ECRITURE,theEntity,theGeom,theErr);
    }

    //----------------------------------------------------------------------------
    void 
    TVWrapper
    ::SetFamilies(const TElemInfo&  theInfo,
                  EModeAcces        theMode,
                  EEntiteMaillage   theEntity, 
                  EGeometrieElement theGeom,
                  TErr*             theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;

      if ( theGeom == eBALL )
        theGeom = GetBallGeom( theInfo.myMeshInfo );

      MED::TElemInfo& anInfo = const_cast<MED::TElemInfo&>(theInfo);
      MED::TMeshInfo& aMeshInfo = *anInfo.myMeshInfo;

      TValueHolder<TString, char>                        aMeshName(aMeshInfo.myName);
      TValueHolder<TElemNum, med_int>                    aFamNum  (anInfo.myFamNum);
      TValueHolder<EEntiteMaillage, med_entity_type>     anEntity (theEntity);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom    (theGeom);
      
      TErr aRet = MEDmeshEntityFamilyNumberWr(myFile->Id(),
                                              &aMeshName,
                                              MED_NO_DT,
                                              MED_NO_IT,
                                              anEntity,
                                              aGeom,
                                              (TInt)anInfo.myFamNum->size(),
                                              &aFamNum);
      
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetFamilies - MEDmeshEntityFamilyNumberWr(...)");
    }
    
    //----------------------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbNodes(const MED::TMeshInfo& theMeshInfo,
                 ETable theTable,
                 TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return -1;
      
      MED::TMeshInfo& aMeshInfo = const_cast<MED::TMeshInfo&>(theMeshInfo);
      
      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      TValueHolder<ETable, med_data_type > aTable(theTable);
      med_bool chgt,trsf;
      return MEDmeshnEntity(myFile->Id(),
                            &aMeshName,
                            MED_NO_DT,
                            MED_NO_IT,
                            MED_NODE,
                            MED_NO_GEOTYPE,
                            aTable,
                            MED_NO_CMODE,
                            &chgt,
                            &trsf);
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetNodeInfo(MED::TNodeInfo& theInfo,
                  TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      MED::TMeshInfo& aMeshInfo = *theInfo.myMeshInfo;

      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      TValueHolder<TInt, med_int> aDim(aMeshInfo.myDim);
      TValueHolder<TNodeCoord, med_float> aCoord(theInfo.myCoord);
      TValueHolder<EModeSwitch, med_switch_mode> aModeSwitch(theInfo.myModeSwitch);
      TValueHolder<ERepere, med_axis_type> aSystem(theInfo.mySystem);
      TValueHolder<TString, char> aCoordNames(theInfo.myCoordNames);
      TValueHolder<TString, char> aCoordUnits(theInfo.myCoordUnits);
      TValueHolder<TString, char> anElemNames(theInfo.myElemNames);
      //TValueHolder<EBooleen, med_bool> anIsElemNames(theInfo.myIsElemNames);
      TValueHolder<TElemNum, med_int> anElemNum(theInfo.myElemNum);
      //TValueHolder<EBooleen, med_bool> anIsElemNum(theInfo.myIsElemNum);
      TValueHolder<TElemNum, med_int> aFamNum(theInfo.myFamNum);
      TValueHolder<TInt, med_int> aNbElem(theInfo.myNbElem);

      TErr aRet = MEDmeshNodeCoordinateRd(myFile->Id(),
                                          &aMeshName,
                                          MED_NO_DT,
                                          MED_NO_IT,
                                          aModeSwitch,
                                          &aCoord);

      TErr aRet2 =MEDmeshEntityFamilyNumberRd(myFile->Id(),
                                  &aMeshName,
                                  MED_NO_DT,
                                  MED_NO_IT,
                                  MED_NODE,
                                  MED_NO_GEOTYPE ,
                                  &aFamNum);
      if (aRet2  < 0)
      {
//        if (aRet2 == MED_ERR_DOESNTEXIST) // --- only valid with MED3.x files
          {
            int mySize = (int)theInfo.myFamNum->size();
            theInfo.myFamNum->clear();
            theInfo.myFamNum->resize(mySize,0);
          }
//        else
//          EXCEPTION(std::runtime_error,"GetNodeInfo - MEDmeshEntityFamilyNumberRd(...)");
      }
                                  
      if ( MEDmeshEntityNameRd(myFile->Id(),
                          &aMeshName,
                          MED_NO_DT,
                          MED_NO_IT,
                          MED_NODE,
                          MED_NO_GEOTYPE ,
                          &anElemNames) < 0) theInfo.myIsElemNames=eFAUX;
      
      if ( MEDmeshEntityNumberRd(myFile->Id(),
                            &aMeshName,
                            MED_NO_DT,
                            MED_NO_IT,
                            MED_NODE,
                            MED_NO_GEOTYPE ,
                            &anElemNum) < 0 ) theInfo.myIsElemNum=eFAUX;
      
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetNodeInfo - MEDmeshNodeCoordinateRd(...)");
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetNodeInfo(const MED::TNodeInfo& theInfo,
                  EModeAcces theMode,
                  TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      MED::TNodeInfo& anInfo = const_cast<MED::TNodeInfo&>(theInfo);
      MED::TMeshInfo& aMeshInfo = *anInfo.myMeshInfo;
      
      TValueHolder<TString, char>                aMeshName    (aMeshInfo.myName);
      TValueHolder<TNodeCoord, med_float>        aCoord       (anInfo.myCoord);
      TValueHolder<EModeSwitch, med_switch_mode> aModeSwitch  (anInfo.myModeSwitch);
      TValueHolder<ERepere, med_axis_type>       aSystem      (anInfo.mySystem);
      TValueHolder<TString, char>                aCoordNames  (anInfo.myCoordNames);
      TValueHolder<TString, char>                aCoordUnits  (anInfo.myCoordUnits);
      TValueHolder<TString, char>                anElemNames  (anInfo.myElemNames);
      TValueHolder<EBooleen, med_bool>           anIsElemNames(anInfo.myIsElemNames);
      TValueHolder<TElemNum, med_int>            anElemNum    (anInfo.myElemNum);
      TValueHolder<EBooleen, med_bool>           anIsElemNum  (anInfo.myIsElemNum);
      TValueHolder<TElemNum, med_int>            aFamNum      (anInfo.myFamNum);
      TValueHolder<TInt, med_int>                aNbElem      (anInfo.myNbElem);

      TErr aRet = MEDmeshNodeCoordinateWr(myFile->Id(),
                                          &aMeshName,
                                          MED_NO_DT,
                                          MED_NO_IT,
                                          MED_NO_DT,
                                          aModeSwitch,
                                          aNbElem,
                                          &aCoord);
                                          
      MEDmeshEntityFamilyNumberWr(myFile->Id(),
                                  &aMeshName,
                                  MED_NO_DT,
                                  MED_NO_IT,
                                  MED_NODE,
                                  MED_NO_GEOTYPE,
                                  aNbElem,
                                  &aFamNum);
      if(anIsElemNames)
        MEDmeshEntityNameWr(myFile->Id(),
                            &aMeshName,
                            MED_NO_DT,
                            MED_NO_IT,
                            MED_NODE,
                            MED_NO_GEOTYPE,
                            aNbElem,
                            &anElemNames);
      if(anIsElemNum)
        MEDmeshEntityNumberWr(myFile->Id(),
                              &aMeshName,
                              MED_NO_DT,
                              MED_NO_IT,
                              MED_NODE,
                              MED_NO_GEOTYPE,
                              aNbElem,
                              &anElemNum);
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetNodeInfo - MEDmeshNodeCoordinateWr(...)");
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetNodeInfo(const MED::TNodeInfo& theInfo,
                  TErr* theErr)
    {
      TErr aRet;
      SetNodeInfo(theInfo,eLECTURE_ECRITURE,&aRet);
      
      if(aRet < 0)
        SetNodeInfo(theInfo,eLECTURE_AJOUT,&aRet);

      if(theErr) 
        *theErr = aRet;
    }
    

    //-----------------------------------------------------------------
    void
    TVWrapper
    ::GetPolygoneInfo(MED::TPolygoneInfo& theInfo,
                      TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0)
        return;

      MED::TMeshInfo& aMeshInfo = *theInfo.myMeshInfo;

      TValueHolder<TString, char                       > aMeshName(aMeshInfo.myName);
      TValueHolder<TElemNum, med_int                   > anIndex  (theInfo.myIndex);
      TValueHolder<TElemNum, med_int                   > aConn    (theInfo.myConn);
      TValueHolder<EEntiteMaillage, med_entity_type    > anEntity (theInfo.myEntity);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom    (theInfo.myGeom);
      TValueHolder<EConnectivite, med_connectivity_mode> aConnMode(theInfo.myConnMode);
      TInt aNbElem = (TInt)theInfo.myElemNum->size();

      TErr aRet;
#if MED_VERSION_HEX < 0x030007
      aRet = -1;
#else
      aRet = MEDmeshPolygon2Rd(myFile->Id(), &aMeshName,
                               MED_NO_DT, MED_NO_IT,
                               anEntity, aGeom,
                               aConnMode, &anIndex, &aConn);
#endif

      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetPolygoneInfo - MEDmeshPolygonRd(...)");

      if(theInfo.myIsElemNames){
        GetNames(theInfo,aNbElem,theInfo.myEntity,theInfo.myGeom,&aRet);
        if(theErr) 
          *theErr = aRet;
      }

      if(theInfo.myIsElemNum){
        GetNumeration(theInfo,aNbElem,theInfo.myEntity,theInfo.myGeom,&aRet);
        if(theErr) 
          *theErr = aRet;
      }

      GetFamilies(theInfo,aNbElem,theInfo.myEntity,theInfo.myGeom,&aRet);
      if(theErr) 
        *theErr = aRet;
    }
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetPolygoneInfo(const MED::TPolygoneInfo& theInfo,
                      TErr* theErr)
    {
      SetPolygoneInfo(theInfo,eLECTURE_ECRITURE,theErr);
    }
    
    //----------------------------------------------------------------------------
    void 
    TVWrapper
    ::SetPolygoneInfo(const MED::TPolygoneInfo& theInfo,
                      EModeAcces theMode,
                      TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;

      MED::TPolygoneInfo& anInfo = const_cast<MED::TPolygoneInfo&>(theInfo);
      MED::TMeshInfo& aMeshInfo = *anInfo.myMeshInfo;

      TValueHolder<TString, char                       > aMeshName(aMeshInfo.myName);
      TValueHolder<TElemNum, med_int                   > anIndex  (anInfo.myIndex);
      TValueHolder<TElemNum, med_int                   > aConn    (anInfo.myConn);
      TValueHolder<EEntiteMaillage, med_entity_type    > anEntity (anInfo.myEntity);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom    (anInfo.myGeom);
      TValueHolder<EConnectivite, med_connectivity_mode> aConnMode(anInfo.myConnMode);

#if MED_VERSION_HEX < 0x030007
      TErr aRet = -1;
#else
      TErr aRet = MEDmeshPolygon2Wr(myFile->Id(), &aMeshName,
                                    MED_NO_DT, MED_NO_IT, MED_UNDEF_DT,
                                    anEntity, aGeom,
                                    aConnMode, anInfo.myNbElem + 1,
                                    &anIndex, &aConn);
#endif
      if(theErr)
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetPolygoneInfo - MEDmeshPolygonWr(...)");
      
      SetNames(anInfo,theInfo.myEntity,anInfo.myGeom,&aRet);
      if(theErr) 
        *theErr = aRet;
      
      SetNumeration(anInfo,theInfo.myEntity,anInfo.myGeom,&aRet);
      if(theErr) 
        *theErr = aRet;
      
      SetFamilies(anInfo,theInfo.myEntity,anInfo.myGeom,&aRet);
      if(theErr) 
        *theErr = aRet;
    }

    //----------------------------------------------------------------------------
    TInt 
    TVWrapper
    ::GetNbPolygones(const MED::TMeshInfo& theMeshInfo, 
                     EEntiteMaillage theEntity, 
                     EGeometrieElement theGeom, 
                     EConnectivite theConnMode,
                     TErr* theErr)
    {
      return GetNbCells(theMeshInfo,theEntity,theGeom,theConnMode,theErr);
    }
    
    //----------------------------------------------------------------------------
    TInt 
    TVWrapper
    ::GetPolygoneConnSize(const MED::TMeshInfo& theMeshInfo, 
                          EEntiteMaillage theEntity, 
                          EGeometrieElement theGeom, 
                          EConnectivite theConnMode,
                          TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0)
        return 0;

      MED::TMeshInfo& aMeshInfo = const_cast<MED::TMeshInfo&>(theMeshInfo);
      
      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      med_int aTaille = 0;
      med_bool chgt,trsf;
      aTaille=MEDmeshnEntity(myFile->Id(),
                             &aMeshName,
                             MED_NO_DT,
                             MED_NO_IT,
                             med_entity_type(theEntity),
                             med_geometry_type(theGeom),
                             MED_CONNECTIVITY,
                             med_connectivity_mode(theConnMode),
                             &chgt,
                             &trsf);

      
      if(aTaille < 0)
        EXCEPTION(std::runtime_error,"GetPolygoneInfo - MEDmeshnEntity(...)");

      return TInt(aTaille);
    }

    //-----------------------------------------------------------------
    void 
    TVWrapper
    ::GetPolyedreInfo(TPolyedreInfo& theInfo,
                      TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0)
        return;

      MED::TMeshInfo& aMeshInfo = *theInfo.myMeshInfo;

      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      TInt aNbElem = (TInt)theInfo.myElemNum->size();
      TValueHolder<TElemNum, med_int> anIndex(theInfo.myIndex);
      TValueHolder<TElemNum, med_int> aFaces(theInfo.myFaces);
      TValueHolder<TElemNum, med_int> aConn(theInfo.myConn);
      TValueHolder<EConnectivite, med_connectivity_mode> aConnMode(theInfo.myConnMode);

      TErr aRet;
      aRet = MEDmeshPolyhedronRd(myFile->Id(),
                                 &aMeshName,
                                 MED_NO_DT,
                                 MED_NO_IT,
                                 MED_CELL,
                                 aConnMode,
                                 &anIndex,
                                 &aFaces,
                                 &aConn);

      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetPolygoneInfo - MEDmeshPolyhedronRd(...)");

      if(theInfo.myIsElemNames){
        GetNames(theInfo,aNbElem,theInfo.myEntity,ePOLYEDRE,&aRet);
        if(theErr)
          *theErr = aRet;
      }

      if(theInfo.myIsElemNum){
        GetNumeration(theInfo,aNbElem,theInfo.myEntity,ePOLYEDRE,&aRet);
        if(theErr) 
          *theErr = aRet;
      }

      GetFamilies(theInfo,aNbElem,theInfo.myEntity,ePOLYEDRE,&aRet);
      if(theErr) 
        *theErr = aRet;
    }

    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetPolyedreInfo(const TPolyedreInfo& theInfo,
                      TErr* theErr)
    {
      SetPolyedreInfo(theInfo,eLECTURE_ECRITURE,theErr);
    }
    
    //----------------------------------------------------------------------------
    void 
    TVWrapper
    ::SetPolyedreInfo(const MED::TPolyedreInfo& theInfo,
                      EModeAcces theMode,
                      TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;

      MED::TPolyedreInfo& anInfo = const_cast<MED::TPolyedreInfo&>(theInfo);
      MED::TMeshInfo& aMeshInfo = *anInfo.myMeshInfo;

      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      TValueHolder<TElemNum, med_int> anIndex(anInfo.myIndex);
      TValueHolder<TElemNum, med_int> aFaces(anInfo.myFaces);
      TValueHolder<TElemNum, med_int> aConn(anInfo.myConn);
      TValueHolder<EConnectivite, med_connectivity_mode> aConnMode(anInfo.myConnMode);

      TErr aRet;
      aRet = MEDmeshPolyhedronWr(myFile->Id(),
                                 &aMeshName,
                                 MED_NO_DT,
                                 MED_NO_IT,
                                 MED_UNDEF_DT,
                                 MED_CELL,
                                 aConnMode,
                                 anInfo.myNbElem+1,
                                 &anIndex,
                                 (TInt)anInfo.myFaces->size(),
                                 &aFaces,
                                 &aConn);
      
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetPolyedreInfo - MEDmeshPolyhedronWr(...)");
      
      TValueHolder<EEntiteMaillage, med_entity_type> anEntity(anInfo.myEntity);

      if(theInfo.myIsElemNames){
        TValueHolder<TString, char> anElemNames(anInfo.myElemNames);
        aRet  = MEDmeshEntityNameWr(myFile->Id(),
                                    &aMeshName,
                                    MED_NO_DT,
                                    MED_NO_IT,
                                    anEntity,
                                    MED_POLYHEDRON,
                                    (TInt)anInfo.myElemNames->size(),
                                    &anElemNames);
        if(theErr) 
          *theErr = aRet;
        else if(aRet < 0)
          EXCEPTION(std::runtime_error,"SetPolyedreInfo - MEDmeshEntityNameWr(...)");
      }
      
      if(theInfo.myIsElemNum){
        TValueHolder<TElemNum, med_int> anElemNum(anInfo.myElemNum);
        aRet = MEDmeshEntityNumberWr(myFile->Id(),
                                     &aMeshName,
                                     MED_NO_DT,
                                     MED_NO_IT,
                                     anEntity,
                                     MED_POLYHEDRON,
                                     (TInt)anInfo.myElemNum->size(),
                                     &anElemNum);
        if(theErr) 
          *theErr = aRet;
        else if(aRet < 0)
          EXCEPTION(std::runtime_error,"SetPolyedreInfo - MEDmeshEntityNumberWr(...)");
      }
      
      
      TValueHolder<TElemNum, med_int> aFamNum(anInfo.myFamNum);
      aRet = MEDmeshEntityFamilyNumberWr(myFile->Id(),
                                         &aMeshName,
                                         MED_NO_DT,
                                         MED_NO_IT,
                                         anEntity,
                                         MED_POLYHEDRON,
                                         (TInt)anInfo.myFamNum->size(),
                                         &aFamNum);
      
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetPolyedreInfo - MEDmeshEntityFamilyNumberWr(...)");
    }

    //----------------------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbPolyedres(const MED::TMeshInfo& theMeshInfo, 
                     EEntiteMaillage theEntity, 
                     EGeometrieElement theGeom, 
                     EConnectivite theConnMode,
                     TErr* theErr)
    {
      return GetNbCells(theMeshInfo,theEntity,theGeom,theConnMode,theErr);
    }

    //----------------------------------------------------------------------------
    void
    TVWrapper    ::GetPolyedreConnSize(const TMeshInfo& theMeshInfo,
                          TInt& theNbFaces,
                          TInt& theConnSize,
                          EConnectivite theConnMode,
                          TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0) 
        EXCEPTION(std::runtime_error,"GetPolyedreConnSize - (...)");

      MED::TMeshInfo& aMeshInfo = const_cast<MED::TMeshInfo&>(theMeshInfo);
      
      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      TValueHolder<EConnectivite, med_connectivity_mode> aConnMode(theConnMode);
      //TValueHolder<TInt, med_int> aNbFaces(theNbFaces);
      //TValueHolder<TInt, med_int> aConnSize(theConnSize);

      med_bool chgt,trsf;
      theNbFaces = MEDmeshnEntity(myFile->Id(),
                                  &aMeshName,
                                  MED_NO_DT,
                                  MED_NO_IT,
                                  MED_CELL,
                                  MED_POLYHEDRON,
                                  MED_INDEX_NODE,
                                  aConnMode,
                                  &chgt,
                                  &trsf);

      theConnSize = MEDmeshnEntity(myFile->Id(),
                                  &aMeshName,
                                  MED_NO_DT,
                                  MED_NO_IT,
                                  MED_CELL,
                                  MED_POLYHEDRON,
                                  MED_CONNECTIVITY,
                                  aConnMode,
                                  &chgt,
                                  &trsf);

      if(theNbFaces < 0 || theConnSize<0)
        EXCEPTION(std::runtime_error,"GetPolygoneInfo - MEDmeshnEntity(...)");

    }
    
    //-----------------------------------------------------------------
    TEntityInfo
    TVWrapper
    ::GetEntityInfo(const MED::TMeshInfo& theMeshInfo,
                    EConnectivite theConnMode,
                    TErr* theErr)
    {
      TEntityInfo anInfo;
      
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return anInfo;

      if(theMeshInfo.GetType() == eNON_STRUCTURE) {
        TInt aNbElem = GetNbNodes(theMeshInfo);
        if(aNbElem > 0){
          anInfo[eNOEUD][ePOINT1] = aNbElem;
          const TEntity2GeomSet& anEntity2GeomSet = GetEntity2GeomSet();
          TEntity2GeomSet::const_iterator anIter = anEntity2GeomSet.begin();
          TEntity2GeomSet::const_iterator anIterEnd = anEntity2GeomSet.end();
          for(; anIter != anIterEnd; anIter++){
            const EEntiteMaillage& anEntity = anIter->first;
            const TGeomSet& aGeomSet = anIter->second;
            TGeomSet::const_iterator anIter2 = aGeomSet.begin();
            TGeomSet::const_iterator anIterEnd2 = aGeomSet.end();
            for(; anIter2 != anIterEnd2; anIter2++){
              const EGeometrieElement& aGeom = *anIter2;
              aNbElem = GetNbCells(theMeshInfo,anEntity,aGeom,theConnMode,theErr);
              if(aNbElem > 0) {
                if ( anEntity == eSTRUCT_ELEMENT ) {
                  const TInt nbStructTypes = aNbElem;
                  for ( TInt structType = 0; structType < nbStructTypes; ++structType ) {
                    // check type name to keep only "MED_BALL" structured element
                    TValueHolder<TString, char> aMeshName((TString&) theMeshInfo.myName );
                    char                        geotypename[ MED_NAME_SIZE + 1] = "";
                    med_geometry_type           geotype;
                    MEDmeshEntityInfo( myFile->Id(), &aMeshName, MED_NO_DT, MED_NO_IT,
                                       med_entity_type(anEntity), structType+1,
                                       geotypename, &geotype);
                    if ( strcmp( geotypename, MED_BALL_NAME ) == 0 ) {
                      aNbElem = GetNbCells( theMeshInfo, anEntity, EGeometrieElement(geotype),
                                            theConnMode, theErr);
                      if ( aNbElem > 0 )
                        anInfo[anEntity][EGeometrieElement(geotype)] = aNbElem;
                    }
                  }
                }
                else {
                  anInfo[anEntity][aGeom] = aNbElem;
                }
              }
            }
          }
        }
      } else { // eSTRUCTURE
        EGrilleType aGrilleType;
        TInt aNbNodes = 1;
        TInt aNbElem  = 1;
        TInt aNbSub   = 0;
        TInt aDim = theMeshInfo.GetDim();
        EGeometrieElement aGeom, aSubGeom;
        EEntiteMaillage aSubEntity = eMAILLE;

        GetGrilleType(theMeshInfo, aGrilleType);

        TIntVector aStruct(aDim);
        if(aGrilleType == eGRILLE_STANDARD)
        {
          GetGrilleStruct(theMeshInfo, aStruct, theErr);
        }
        else
        { // eGRILLE_CARTESIENNE and eGRILLE_POLAIRE
          ETable aTable[3] = { eCOOR_IND1, eCOOR_IND2, eCOOR_IND3 };
          for(med_int anAxis = 0; anAxis < aDim; anAxis++)
            aStruct[ anAxis ] = GetNbNodes(theMeshInfo, aTable[anAxis]);
        }
        for(med_int i = 0; i < aDim; i++){
          aNbNodes = aNbNodes * aStruct[i];
          aNbElem = aNbElem * (aStruct[i] - 1);
        }
        switch(aDim){
        case 1:
          aGeom = eSEG2;
          break;
        case 2:
          aGeom = eQUAD4;
          aSubGeom = eSEG2;
          aSubEntity = eARETE;
          aNbSub =
            (aStruct[0]  ) * (aStruct[1]-1) +
            (aStruct[0]-1) * (aStruct[1]  );
          break;
        case 3:
          aGeom = eHEXA8;
          aSubGeom = eQUAD4;
          aSubEntity = eFACE;
          aNbSub =
            (aStruct[0]  ) * (aStruct[1]-1) * (aStruct[2]-1) +
            (aStruct[0]-1) * (aStruct[1]  ) * (aStruct[2]-1) +
            (aStruct[0]-1) * (aStruct[1]-1) * (aStruct[2]  );
          break;
        }
        anInfo[eNOEUD][ePOINT1] = aNbNodes;
        anInfo[eMAILLE][aGeom] = aNbElem;
        if ( aDim > 1 )
          anInfo[aSubEntity][aSubGeom] = aNbSub;
      }
      return anInfo;
    }


    //-----------------------------------------------------------------
    TInt TVWrapper::GetNbCells(const MED::TMeshInfo& theMeshInfo,
                               EEntiteMaillage theEntity,
                               EGeometrieElement theGeom,
                               EConnectivite theConnMode,
                               TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0)
        return -1;

      MED::TMeshInfo& aMeshInfo = const_cast<MED::TMeshInfo&>(theMeshInfo);
      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      med_bool chgt,trsf;
      switch ( theGeom )
      {
      case MED::ePOLYGONE:
      case MED::ePOLYGON2:
      {
        return MEDmeshnEntity(myFile->Id(),&aMeshName,
                              MED_NO_DT,MED_NO_IT,
                              med_entity_type(theEntity),med_geometry_type(theGeom),
                              MED_INDEX_NODE,med_connectivity_mode(theConnMode),
                              &chgt,&trsf)-1;
      }
      case MED::ePOLYEDRE:
      {
        return MEDmeshnEntity(myFile->Id(),&aMeshName,
                              MED_NO_DT,MED_NO_IT,
                              med_entity_type(theEntity),MED_POLYHEDRON,
                              MED_INDEX_FACE,med_connectivity_mode(theConnMode),
                              &chgt,&trsf)-1;
      }
      case MED::eBALL:
      {
        return GetNbBalls( theMeshInfo );
      }
      default:
      {
        return MEDmeshnEntity(myFile->Id(),&aMeshName,
                              MED_NO_DT,MED_NO_IT,
                              med_entity_type(theEntity),med_geometry_type(theGeom),
                              MED_CONNECTIVITY,med_connectivity_mode(theConnMode),
                              &chgt,&trsf);
      }
      }
      return 0;
    }


    //----------------------------------------------------------------------------
    void TVWrapper::GetCellInfo(MED::TCellInfo& theInfo, TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0)
        return;

      MED::TMeshInfo& aMeshInfo = *theInfo.myMeshInfo;

      TValueHolder<TString, char>                        aMeshName    (aMeshInfo.myName);
      TValueHolder<TElemNum, med_int>                    aConn        (theInfo.myConn);
      TValueHolder<EModeSwitch, med_switch_mode>         aModeSwitch  (theInfo.myModeSwitch);
      TValueHolder<TString, char>                        anElemNames  (theInfo.myElemNames);
      TValueHolder<EBooleen, med_bool>                   anIsElemNames(theInfo.myIsElemNames);
      TValueHolder<TElemNum, med_int>                    anElemNum    (theInfo.myElemNum);
      TValueHolder<EBooleen, med_bool>                   anIsElemNum  (theInfo.myIsElemNum);
      TValueHolder<TElemNum, med_int>                    aFamNum      (theInfo.myFamNum);
      TValueHolder<EBooleen, med_bool>                   anIsFamNum   (theInfo.myIsFamNum);
      TValueHolder<EEntiteMaillage, med_entity_type>     anEntity     (theInfo.myEntity);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom        (theInfo.myGeom);
      TValueHolder<EConnectivite, med_connectivity_mode> aConnMode    (theInfo.myConnMode);

      TErr aRet;
      aRet = MEDmeshElementRd(myFile->Id(),
                              &aMeshName,
                              MED_NO_DT,
                              MED_NO_IT,
                              anEntity,
                              aGeom,
                              aConnMode,
                              aModeSwitch,
                              &aConn,
                              &anIsElemNames,
                              &anElemNames,
                              &anIsElemNum,
                              &anElemNum,
                              &anIsFamNum,
                              &aFamNum);

      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetCellInfo - MEDmeshElementRd(...)");
      
      if (anIsFamNum == MED_FALSE)
        {
          int mySize = (int) theInfo.myFamNum->size();
          theInfo.myFamNum->clear();
          theInfo.myFamNum->resize(mySize, 0);
        }
      
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetCellInfo(const MED::TCellInfo& theInfo,
                  EModeAcces theMode,
                  TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;

      MED::TCellInfo& anInfo    = const_cast<MED::TCellInfo&>(theInfo);
      MED::TMeshInfo& aMeshInfo = *anInfo.myMeshInfo;

      TValueHolder<TString, char>                        aMeshName    (aMeshInfo.myName);
      TValueHolder<TElemNum, med_int>                    aConn        (anInfo.myConn);
      TValueHolder<EModeSwitch, med_switch_mode>         aModeSwitch  (anInfo.myModeSwitch);
      TValueHolder<TString, char>                        anElemNames  (anInfo.myElemNames);
      TValueHolder<EBooleen, med_bool>                   anIsElemNames(anInfo.myIsElemNames);
      TValueHolder<TElemNum, med_int>                    anElemNum    (anInfo.myElemNum);
      TValueHolder<EBooleen, med_bool>                   anIsElemNum  (anInfo.myIsElemNum);
      TValueHolder<TElemNum, med_int>                    aFamNum      (anInfo.myFamNum);
      TValueHolder<EBooleen, med_bool>                   anIsFamNum   (anInfo.myIsFamNum);
      TValueHolder<EEntiteMaillage, med_entity_type>     anEntity     (anInfo.myEntity);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom        (anInfo.myGeom);
      TValueHolder<EConnectivite, med_connectivity_mode> aConnMode    (anInfo.myConnMode);
      TValueHolder<TInt, med_int>                        aNbElem      (anInfo.myNbElem);

      TErr aRet;
      aRet = MEDmeshElementConnectivityWr(myFile->Id(),
                                          &aMeshName,
                                          MED_NO_DT,
                                          MED_NO_IT,
                                          MED_UNDEF_DT,
                                          anEntity,
                                          aGeom,
                                          aConnMode,
                                          aModeSwitch,
                                          aNbElem,
                                          &aConn);

      MEDmeshEntityFamilyNumberWr(myFile->Id(),
                                  &aMeshName,
                                  MED_NO_DT,
                                  MED_NO_IT,
                                  anEntity,
                                  aGeom,
                                  aNbElem,
                                  &aFamNum);
      if(anIsElemNames)
        MEDmeshEntityNameWr(myFile->Id(),
                            &aMeshName,
                            MED_NO_DT,
                            MED_NO_IT,
                            anEntity,
                            aGeom,
                            aNbElem,
                            &anElemNames);
      if(anIsElemNum)
        MEDmeshEntityNumberWr(myFile->Id(),
                              &aMeshName,
                              MED_NO_DT,
                              MED_NO_IT,
                              anEntity,
                              aGeom,
                              aNbElem,
                              &anElemNum);
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetCellInfo - MEDmeshElementWr(...)");
    }
    

    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetCellInfo(const MED::TCellInfo& theInfo,
                  TErr* theErr)
    {
      SetCellInfo(theInfo,eLECTURE_ECRITURE,theErr);
    }

    //----------------------------------------------------------------------------
    //! Read geom type of MED_BALL structural element
    EGeometrieElement TVWrapper::GetBallGeom(const TMeshInfo& theMeshInfo)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE);

      // read med_geometry_type of "MED_BALL" element
      char geotypename[ MED_NAME_SIZE + 1] = MED_BALL_NAME;
      return EGeometrieElement( MEDstructElementGeotype( myFile->Id(), geotypename ) );
    }

    //----------------------------------------------------------------------------
    //! Read number of balls in the Mesh
    TInt TVWrapper::GetNbBalls(const TMeshInfo& theMeshInfo)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE);

      EGeometrieElement ballType = GetBallGeom( theMeshInfo );
      if ( ballType < 0 )
        return 0;

      return GetNbCells( theMeshInfo, eSTRUCT_ELEMENT, ballType, eNOD );
    }

    //----------------------------------------------------------------------------
    //! Read a MEDWrapped representation of MED_BALL from the MED file
    void TVWrapper::GetBallInfo(TBallInfo& theInfo, TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      // check geometry of MED_BALL
      if ( theInfo.myGeom == eBALL )
      {
        theInfo.myGeom = GetBallGeom( *theInfo.myMeshInfo );
        if ( theInfo.myGeom < 0 ) {
          if ( !theErr )
            EXCEPTION(std::runtime_error,"GetBallInfo - no balls in the mesh");
          *theErr = theInfo.myGeom;
          return;
        }
      }

      // read nodes ids
      GetCellInfo( theInfo );

      // read diameters
      TValueHolder<TString, char>                        aMeshName (theInfo.myMeshInfo->myName);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom     (theInfo.myGeom);
      TValueHolder<TFloatVector, void>                   aDiam     (theInfo.myDiameters);
      char varattname[ MED_NAME_SIZE + 1] = MED_BALL_DIAMETER;

      TErr aRet = MEDmeshStructElementVarAttRd( myFile->Id(), &aMeshName,
                                                MED_NO_DT, MED_NO_IT,
                                                aGeom,
                                                varattname,
                                                &aDiam);
      if ( theErr )
        *theErr = aRet;
      else if ( aRet < 0 )
        EXCEPTION(std::runtime_error,"GetBallInfo - pb at reading diameters");
    }


    //----------------------------------------------------------------------------
    //! Write a MEDWrapped representation of MED_BALL to the MED file
    void  TVWrapper::SetBallInfo(const TBallInfo& theInfo, EModeAcces theMode, TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);

      TErr ret;
      char ballsupportname[MED_NAME_SIZE+1]="BALL_SUPPORT_MESH";
      EGeometrieElement ballGeom = GetBallGeom( *theInfo.myMeshInfo );
      if ( ballGeom < 0 )
      {
        // no ball model in the file, create support mesh for it
        char dummyname [MED_NAME_SIZE*3+1]="";
        if (( ret = MEDsupportMeshCr( myFile->Id(),
                                      ballsupportname,
                                      theInfo.myMeshInfo->GetSpaceDim(),
                                      theInfo.myMeshInfo->GetDim(),
                                      "Support mesh for a ball model",
                                      MED_CARTESIAN,
                                      /*axisname=*/dummyname, /*unitname=*/dummyname)) < 0) {
          if ( !theErr )
            EXCEPTION(std::runtime_error,"SetBallInfo - MEDsupportMeshCr");
          *theErr = ret;
          return;
        }
        // write coordinates of 1 node
        med_float coord[3] = {0,0,0};
        if ((ret = MEDmeshNodeCoordinateWr(myFile->Id(),
                                           ballsupportname, MED_NO_DT, MED_NO_IT, 0.0,
                                           MED_FULL_INTERLACE, /*nnode=*/1, coord)) < 0) {
          if ( !theErr )
            EXCEPTION(std::runtime_error,"SetBallInfo - MEDmeshNodeCoordinateWr");
          *theErr = ret;
          return;
        }
        // ball model creation
        char geotypename[ MED_NAME_SIZE + 1] = MED_BALL_NAME;
        if (( ballGeom = (EGeometrieElement) MEDstructElementCr(myFile->Id(),
                                                                geotypename,
                                                                theInfo.myMeshInfo->GetSpaceDim(),
                                                                ballsupportname,
                                                                MED_NODE,MED_NONE)) < 0 ) {
          if ( !theErr )
            EXCEPTION(std::runtime_error,"SetBallInfo - MEDstructElementCr");
          *theErr = ret;
          return;
        }
        // create diameter attribute
        if (( ret = MEDstructElementVarAttCr(myFile->Id(),
                                             geotypename, MED_BALL_DIAMETER,
                                             MED_ATT_FLOAT64, /*ncomp=*/1)) < 0) {
          if ( !theErr )
            EXCEPTION(std::runtime_error,"SetBallInfo - MEDstructElementVarAttCr");
          *theErr = ret;
          return;
        }
      } // ballGeom < 0

      TBallInfo& aBallInfo = ((TBallInfo&) theInfo );
      aBallInfo.myGeom = ballGeom;

      // write node ids
      SetCellInfo(theInfo,theMode,theErr);
      if ( theErr && *theErr < 0 )
        return;

      // write diameter
      TValueHolder<TString, char>                        aMeshName (aBallInfo.myMeshInfo->myName);
      TValueHolder<EGeometrieElement, med_geometry_type> aGeom     (aBallInfo.myGeom);
      TValueHolder<TFloatVector, void>                   aDiam     (aBallInfo.myDiameters);
      ret = MEDmeshStructElementVarAttWr(myFile->Id(), &aMeshName,
                                         MED_NO_DT, MED_NO_IT,
                                         aGeom, MED_BALL_DIAMETER,
                                         theInfo.myNbElem, &aDiam);
      if ( theErr )
        *theErr = ret;
      else if ( ret < 0 )
        EXCEPTION(std::runtime_error,"SetBallInfo - MEDmeshStructElementVarAttWr");
    }

    //----------------------------------------------------------------------------
    //! Write a MEDWrapped representation of MED_BALL to the MED file
    void  TVWrapper::SetBallInfo(const TBallInfo& theInfo, TErr* theErr)
    {
      SetBallInfo( theInfo, eLECTURE_ECRITURE, theErr );
    }

    //-----------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbFields(TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return -1;
      
      return MEDnField(myFile->Id());
    }
    
    
    //----------------------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbComp(TInt theFieldId,
                TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return -1;
      
      return MEDfieldnComponent(myFile->Id(),theFieldId);
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetFieldInfo(TInt theFieldId, 
                   MED::TFieldInfo& theInfo,
                   TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      TString aFieldName(256); // Protect from memory problems with too long names
      TValueHolder<ETypeChamp, med_field_type> aType(theInfo.myType);
      TValueHolder<TString, char> aCompNames(theInfo.myCompNames);
      TValueHolder<TString, char> anUnitNames(theInfo.myUnitNames);
      MED::TMeshInfo& aMeshInfo = theInfo.myMeshInfo;
      
      TErr aRet;
      med_bool local;
      char dtunit[MED_SNAME_SIZE+1];
          char local_mesh_name[MED_NAME_SIZE+1]="";
      med_int nbofstp;
      theInfo.myNbComp = MEDfieldnComponent(myFile->Id(),theFieldId);
      aRet = MEDfieldInfo(myFile->Id(),
                          theFieldId,
                          &aFieldName[0],
                          local_mesh_name,
                          &local,
                          &aType,
                          &aCompNames,
                          &anUnitNames,
                          dtunit,
                          &nbofstp);

          if(strcmp(&aMeshInfo.myName[0],local_mesh_name) != 0 ) {
                  if(theErr)
                        *theErr = -1;
                  return;
          }

      theInfo.SetName(aFieldName);

      if(theErr)
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetFieldInfo - MEDfieldInfo(...)");
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetFieldInfo(const MED::TFieldInfo& theInfo,
                   EModeAcces theMode,
                   TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      MED::TFieldInfo& anInfo = const_cast<MED::TFieldInfo&>(theInfo);
      
      TValueHolder<TString, char> aFieldName(anInfo.myName);
      TValueHolder<ETypeChamp, med_field_type> aType(anInfo.myType);
      TValueHolder<TString, char> aCompNames(anInfo.myCompNames);
      TValueHolder<TString, char> anUnitNames(anInfo.myUnitNames);
      MED::TMeshInfo& aMeshInfo = anInfo.myMeshInfo;
      TErr aRet;
      char dtunit[MED_SNAME_SIZE+1];
      std::fill(dtunit,dtunit+MED_SNAME_SIZE+1,'\0');
      aRet = MEDfieldCr(myFile->Id(),
                        &aFieldName,
                        aType,
                        anInfo.myNbComp,
                        &aCompNames,
                        &anUnitNames,
                        dtunit,
                        &aMeshInfo.myName[0]);
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetFieldInfo - MEDfieldCr(...)");
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetFieldInfo(const MED::TFieldInfo& theInfo,
                   TErr* theErr)
    {
      TErr aRet;
      SetFieldInfo(theInfo,eLECTURE_ECRITURE,&aRet);
      
      if(aRet < 0)
        SetFieldInfo(theInfo,eLECTURE_AJOUT,&aRet);

      if(theErr) 
        *theErr = aRet;
    }
    
    
    //----------------------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbGauss(TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return -1;
      
      return MEDnLocalization(myFile->Id());
    }


    //----------------------------------------------------------------------------
    TGaussInfo::TInfo
    TVWrapper
    ::GetGaussPreInfo(TInt theId, 
                      TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0)
        return TGaussInfo::TInfo( TGaussInfo::TKey(ePOINT1,""),0 );
      
      med_int aNbGaussPoints = med_int();
      TVector<char> aName(GetNOMLength<eV2_2>()+1);
      med_geometry_type aGeom = MED_NONE;

      TErr aRet;
      med_int dim;
      char geointerpname[MED_NAME_SIZE+1]="";
      char ipointstructmeshname[MED_NAME_SIZE+1]="";
      med_int nsectionmeshcell;
      med_geometry_type sectiongeotype;
      aRet = MEDlocalizationInfo (myFile->Id(),
                                  theId,
                                  &aName[0],
                                  &aGeom,
                                  &dim,
                                  &aNbGaussPoints,
                                  geointerpname,
                                  ipointstructmeshname,
                                  &nsectionmeshcell,
                                  &sectiongeotype);
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetGaussPreInfo - MEDlocalizationInfo(...)");
      return TGaussInfo::TInfo(TGaussInfo::TKey(EGeometrieElement(aGeom),&aName[0]),
                               TInt(aNbGaussPoints));
    }


    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetGaussInfo(TInt theId, 
                   TGaussInfo& theInfo,
                   TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      TValueHolder<TNodeCoord, med_float> aRefCoord(theInfo.myRefCoord);
      TValueHolder<TNodeCoord, med_float> aGaussCoord(theInfo.myGaussCoord);
      TValueHolder<TWeight, med_float> aWeight(theInfo.myWeight);
      TValueHolder<EModeSwitch, med_switch_mode> aModeSwitch(theInfo.myModeSwitch);
      TValueHolder<TString, char> aGaussName(theInfo.myName);

      TErr aRet;
      aRet = MEDlocalizationRd(myFile->Id(),
                               &aGaussName,
                               aModeSwitch,
                               &aRefCoord,
                               &aGaussCoord,
                               &aWeight);

      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetGaussInfo - MEDlocalizationRd(...)");
    }


    //----------------------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbProfiles(TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return -1;
      
      return MEDnProfile(myFile->Id());
    }

    TProfileInfo::TInfo
    TVWrapper
    ::GetProfilePreInfo(TInt theId, 
                        TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0)
        return TProfileInfo::TInfo();
      
      med_int aSize = -1;
      TVector<char> aName(GetNOMLength<eV2_2>()+1);

      TErr aRet;
      aRet = MEDprofileInfo(myFile->Id(),
                            theId,
                            &aName[0],
                            &aSize);
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetProfilePreInfo - MEDprofileInfo(...)");
      
      return TProfileInfo::TInfo(&aName[0],aSize);
    }

    void
    TVWrapper
    ::GetProfileInfo(TInt theId, 
                     TProfileInfo& theInfo,
                     TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      TProfileInfo& anInfo = const_cast<TProfileInfo&>(theInfo);
      TValueHolder<TElemNum, med_int> anElemNum(anInfo.myElemNum);
      TValueHolder<TString, char>     aProfileName(anInfo.myName);

      TErr aRet;
      aRet = MEDprofileRd(myFile->Id(),
                          &aProfileName,
                          &anElemNum);
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetProfileInfo - MEDprofileRd(...)");
    }

    void
    TVWrapper
    ::SetProfileInfo(const TProfileInfo& theInfo,
                     EModeAcces          theMode,
                     TErr*               theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      TProfileInfo& anInfo = const_cast<TProfileInfo&>(theInfo);
      TValueHolder<TElemNum, med_int> anElemNum(anInfo.myElemNum);
      TValueHolder<TString, char>     aProfileName(anInfo.myName);

      TErr aRet;
      aRet = MEDprofileWr(myFile->Id(),      // descripteur du fichier.
                          &aProfileName,        // tableau de valeurs du profil.
                          theInfo.GetSize(), // taille du profil.
                          &anElemNum);    // nom profil.
      if(theErr)
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetProfileInfo - MEDprofileWr(...)");
    }

    void
    TVWrapper
    ::SetProfileInfo(const TProfileInfo& theInfo,
                     TErr*               theErr)
    {
      TErr aRet;
      SetProfileInfo(theInfo,eLECTURE_ECRITURE,&aRet);
      
      if(aRet < 0)
        SetProfileInfo(theInfo,eLECTURE_AJOUT,&aRet);

      if(aRet < 0)
        SetProfileInfo(theInfo,eCREATION,&aRet);

      if(theErr)
        *theErr = aRet;
    }

    //-----------------------------------------------------------------
    TInt
    TVWrapper
    ::GetNbTimeStamps(const MED::TFieldInfo& theInfo, 
                      const MED::TEntityInfo& theEntityInfo,
                      EEntiteMaillage& theEntity,
                      TGeom2Size& theGeom2Size,
                      TErr* theErr)
    {
      theEntity = EEntiteMaillage(-1);
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr){
        if(theEntityInfo.empty())
          *theErr = -1;
        if(*theErr < 0)
          return -1;
      }else if(theEntityInfo.empty()) 
        EXCEPTION(std::runtime_error,"GetNbTimeStamps - There is no any Entity on the Mesh");
      
      bool anIsPerformAdditionalCheck = GetNbMeshes() > 1;

      theGeom2Size.clear();
      TInt aNbTimeStamps = 0;
      TIdt anId = myFile->Id();

      MED::TFieldInfo& anInfo = const_cast<MED::TFieldInfo&>(theInfo);
      TValueHolder<TString, char> aFieldName(anInfo.myName);
      MED::TMeshInfo& aMeshInfo = anInfo.myMeshInfo;

      // workaround for IPAL13676
      MED::TEntityInfo localEntityInfo = theEntityInfo;
      TEntityInfo::iterator anLocalIter = localEntityInfo.find(eMAILLE);
      if(anLocalIter != localEntityInfo.end()){
        localEntityInfo[eNOEUD_ELEMENT] = anLocalIter->second;
      }
        
      TEntityInfo::const_iterator anIter = localEntityInfo.begin();
      for(; anIter != localEntityInfo.end(); anIter++){
        med_entity_type anEntity = med_entity_type(anIter->first);
        const TGeom2Size& aGeom2Size = anIter->second;
        TGeom2Size::const_iterator anGeomIter = aGeom2Size.begin();
        for(; anGeomIter != aGeom2Size.end(); anGeomIter++){
          med_geometry_type aGeom = med_geometry_type(anGeomIter->first);
          char aMeshName[MED_NAME_SIZE+1];
          med_bool islocal;
          med_field_type ft;
          char dtunit[MED_SNAME_SIZE+1];
          med_int myNbComp = MEDfieldnComponentByName(anId,&aFieldName);
          char *cname=new char[myNbComp*MED_SNAME_SIZE+1];
          char *unitname=new char[myNbComp*MED_SNAME_SIZE+1];
          med_int aNbStamps;
          MEDfieldInfoByName(anId,
                             &aFieldName,
                             aMeshName,
                             &islocal,
                             &ft,
                             cname,
                             unitname,
                             dtunit,
                             &aNbStamps);
          delete [] cname;
          delete [] unitname;
          med_int nval = 0;
          med_int aNumDt;
          med_int aNumOrd;
          med_float aDt;
          if (aNbStamps > 0)
            {
              MEDfieldComputingStepInfo(anId,
                                        &aFieldName,
                                        1,
                                        &aNumDt,
                                        &aNumOrd,
                                        &aDt);
              char profilename[MED_NAME_SIZE+1];
              char locname[MED_NAME_SIZE+1];
              med_int profilsize;
              med_int aNbGauss;

              // protection from crash (division by zero)
              // inside MEDfieldnValueWithProfile function
              // caused by the workaround for IPAL13676 (see above)
              if( anEntity == MED_NODE_ELEMENT && aGeom % 100 == 0 )
                continue;

              nval = MEDfieldnValueWithProfile(anId,
                                               &aFieldName,
                                               aNumDt,
                                               aNumOrd,
                                               anEntity,
                                               med_geometry_type(aGeom),
                                               1,
                                               MED_COMPACT_STMODE,
                                               profilename,
                                               &profilsize,
                                               locname,
                                               &aNbGauss);
            }
          bool anIsSatisfied =(nval > 0);
          if(anIsSatisfied){
            INITMSG(MYDEBUG,
                    "GetNbTimeStamps aNbTimeStamps = "<<aNbStamps<<
                    "; aGeom = "<<aGeom<<"; anEntity = "<<anEntity<<"\n");
            if(anIsPerformAdditionalCheck){
              anIsSatisfied = !strcmp(&aMeshName[0],&aMeshInfo.myName[0]);
              if(!anIsSatisfied){
                INITMSG(MYDEBUG,
                        "GetNbTimeStamps aMeshName = '"<<&aMeshName[0]<<"' != "<<
                        "; aMeshInfo.myName = '"<<&aMeshInfo.myName[0]<<"'\n");
              }
            }
          }
          if(anIsSatisfied){
            theGeom2Size[EGeometrieElement(aGeom)] = anGeomIter->second;
            theEntity = EEntiteMaillage(anEntity);
            aNbTimeStamps = aNbStamps;
          }
        }
        if(!theGeom2Size.empty()) 
          break;
      }
      return aNbTimeStamps;
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetTimeStampInfo(TInt theTimeStampId, 
                       MED::TTimeStampInfo& theInfo,
                       TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      const TGeom2Size& aGeom2Size = theInfo.myGeom2Size;
      
      if(theErr){
        if(aGeom2Size.empty())
          *theErr = -1;
        if(*theErr < 0)
          return;
      }else if(aGeom2Size.empty())
        EXCEPTION(std::runtime_error,"GetTimeStampInfo - There is no any cell");
      
      MED::TFieldInfo& aFieldInfo = *theInfo.myFieldInfo;
      MED::TMeshInfo& aMeshInfo = *aFieldInfo.myMeshInfo;
      
      TValueHolder<TString, char> aFieldName(aFieldInfo.myName);
      TValueHolder<EEntiteMaillage, med_entity_type> anEntity(theInfo.myEntity);
      TValueHolder<TInt, med_int> aNumDt(theInfo.myNumDt);
      TValueHolder<TInt, med_int> aNumOrd(theInfo.myNumOrd);
      TValueHolder<TString, char> anUnitDt(theInfo.myUnitDt);
      TValueHolder<TFloat, med_float> aDt(theInfo.myDt);
      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      TValueHolder<EBooleen, med_bool> anIsLocal(aFieldInfo.myIsLocal);
      TValueHolder<TInt, med_int> aNbRef(aFieldInfo.myNbRef);

      TGeom2NbGauss& aGeom2NbGauss = theInfo.myGeom2NbGauss;

      // just to get a time stamp unit (anUnitDt)
      med_field_type aFieldType;
      med_int aNbComp = MEDfieldnComponentByName(myFile->Id(), &aFieldName);
      char *aCompName = new char[aNbComp*MED_SNAME_SIZE+1];
      char *aCompUnit = new char[aNbComp*MED_SNAME_SIZE+1];
      med_int aNbStamps;
      MEDfieldInfoByName(myFile->Id(),
                         &aFieldName,
                         &aMeshName,
                         &anIsLocal,
                         &aFieldType,
                         aCompName,
                         aCompUnit,
                         &anUnitDt,
                         &aNbStamps);
      delete [] aCompName;
      delete [] aCompUnit;

      TGeom2Size::const_iterator anIter = aGeom2Size.begin();
      for(; anIter != aGeom2Size.end(); anIter++){
        const EGeometrieElement& aGeom = anIter->first;
        med_int aNbGauss = -1;

        TErr aRet;
        aRet = MEDfieldComputingStepInfo(myFile->Id(),
                                         &aFieldName,
                                         theTimeStampId,
                                         &aNumDt,  
                                         &aNumOrd,
                                         &aDt);
        char profilename[MED_NAME_SIZE+1];
        med_int profilsize;
        char locname[MED_NAME_SIZE+1];
        MEDfieldnValueWithProfile(myFile->Id(),
                                  &aFieldName,
                                  aNumDt,
                                  aNumOrd,
                                  anEntity,
                                  med_geometry_type(aGeom),
                                  1,
                                  MED_COMPACT_STMODE,
                                  profilename,
                                  &profilsize,
                                  locname,
                                  &aNbGauss);

        static TInt MAX_NB_GAUSS_POINTS = 32;
        if(aNbGauss <= 0 || aNbGauss > MAX_NB_GAUSS_POINTS)
          aNbGauss = 1;

        aGeom2NbGauss[aGeom] = aNbGauss;

        if(theErr) 
          *theErr = aRet;
        else if(aRet < 0)
          EXCEPTION(std::runtime_error,"GetTimeStampInfo - MEDfieldnValueWithProfile(...)");
      }      
    }
    

    //----------------------------------------------------------------------------
    void 
    TVWrapper
    ::GetTimeStampValue(const PTimeStampValueBase& theTimeStampValue,
                        const TMKey2Profile& theMKey2Profile,
                        const TKey2Gauss& theKey2Gauss,
                        TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      TIdt anId = myFile->Id();
      
      TValueHolder<EModeSwitch, med_switch_mode> aModeSwitch(theTimeStampValue->myModeSwitch);
      MED::TGeom2Profile& aGeom2Profile = theTimeStampValue->myGeom2Profile;

      MED::PTimeStampInfo aTimeStampInfo = theTimeStampValue->myTimeStampInfo;
      TValueHolder<EEntiteMaillage, med_entity_type> anEntity(aTimeStampInfo->myEntity);
      TValueHolder<TInt, med_int> aNumDt(aTimeStampInfo->myNumDt);
      TValueHolder<TInt, med_int> aNumOrd(aTimeStampInfo->myNumOrd);

      MED::PFieldInfo aFieldInfo = aTimeStampInfo->myFieldInfo;
      TValueHolder<TString, char> aFieldName(aFieldInfo->myName);
      TValueHolder<EBooleen, med_bool> anIsLocal(aFieldInfo->myIsLocal);

      MED::PMeshInfo aMeshInfo = aFieldInfo->myMeshInfo;
      TValueHolder<TString, char> aMeshName(aMeshInfo->myName);
      
      TGeom2Gauss& aGeom2Gauss = aTimeStampInfo->myGeom2Gauss;
      TVector<char> aGaussName(GetNOMLength<eV2_2>()+1);

      med_storage_mode aProfileMode = med_storage_mode(boost::get<0>(theMKey2Profile));
      MED::TKey2Profile aKey2Profile = boost::get<1>(theMKey2Profile);
      TVector<char> aProfileName(GetNOMLength<eV2_2>()+1);

      TGeom2Size& aGeom2Size = aTimeStampInfo->myGeom2Size;
      TGeom2Size::iterator anIter = aGeom2Size.begin();
      for(; anIter != aGeom2Size.end(); anIter++){
        EGeometrieElement aGeom = anIter->first;
        TInt aNbElem = anIter->second;
        med_int profilesize,aNbGauss;

        TInt aNbVal = MEDfieldnValueWithProfile(anId,
                                                &aFieldName,
                                                aNumDt,
                                                aNumOrd,
                                                anEntity,
                                                med_geometry_type(aGeom),
                                                1,
                                                aProfileMode,
                                                &aProfileName[0],
                                                &profilesize,
                                                &aGaussName[0],
                                                &aNbGauss);

        if(aNbVal <= 0){
          if(theErr){
            *theErr = -1;
            return;
          }
          EXCEPTION(std::runtime_error,"GetTimeStampValue - MEDfieldnValueWithProfile(...) - aNbVal == "<<aNbVal<<" <= 0");
        }
        
        TInt aNbComp = aFieldInfo->myNbComp;
        TInt aNbValue = aNbVal;// / aNbGauss; rules in MED changed
        theTimeStampValue->AllocateValue(aGeom,
                                         aNbValue,
                                         aNbGauss,
                                         aNbComp);
        TInt aValueSize = theTimeStampValue->GetValueSize(aGeom);

        INITMSG(MYDEBUG,
                "TVWrapper::GetTimeStampValue - aGeom = "<<aGeom<<
                "; aNbVal = "<<aNbVal<<
                "; aNbValue = "<<aNbValue<<
                "; aNbGauss = "<<aNbGauss<<
                "; aNbComp = "<<aNbComp<<
                std::endl);
        
        TErr aRet = MEDfieldValueWithProfileRd(anId,
                                               &aFieldName,
                                               aNumDt,
                                               aNumOrd,
                                               anEntity,
                                               med_geometry_type(aGeom),
                                               aProfileMode,
                                               &aProfileName[0],
                                               aModeSwitch,
                                               MED_ALL_CONSTITUENT,
                                               theTimeStampValue->GetValuePtr(aGeom));
        if(aRet < 0){
          if(theErr){
            *theErr = MED_FALSE;
            return;
          }
          EXCEPTION(std::runtime_error,"GetTimeStampValue - MEDfieldValueWithProfileRd(...)");
        }

        MED::PGaussInfo aGaussInfo;
        TGaussInfo::TKey aKey(aGeom,&aGaussName[0]);
        if(strcmp(&aGaussName[0],"") != 0){
          MED::TKey2Gauss::const_iterator anIter = theKey2Gauss.find(aKey);
          if(anIter != theKey2Gauss.end()){
            aGaussInfo = anIter->second;
            aGeom2Gauss[aGeom] = aGaussInfo;
          }
        }
        
        MED::PProfileInfo aProfileInfo;
        if(strcmp(&aProfileName[0],MED_NO_PROFILE) != 0){
          MED::TKey2Profile::const_iterator anIter = aKey2Profile.find(&aProfileName[0]);
          if(anIter != aKey2Profile.end()){
            aProfileInfo = anIter->second;
            aGeom2Profile[aGeom] = aProfileInfo;
          }
        }

        if(aGaussInfo && aNbGauss != aGaussInfo->GetNbGauss()){
          if(theErr){
            *theErr = MED_FALSE;
            return;
          }
          EXCEPTION(std::runtime_error,"GetTimeStampValue - aNbGauss != aGaussInfo->GetNbGauss()");
        }
        
        if(aProfileInfo && aProfileInfo->IsPresent()){
          TInt aNbSubElem = aProfileInfo->GetSize();
          TInt aProfileSize = aNbSubElem*aNbComp*aNbGauss;
          if(aProfileSize != aValueSize){
            if(theErr){
              *theErr = -1;
              return;
            }
            EXCEPTION(std::runtime_error,
                      "GetTimeStampValue - aProfileSize("<<aProfileSize<<
                      ") != aValueSize("<<aValueSize<<
                      "); aNbVal = "<<aNbVal<<
                      "; anEntity = "<<anEntity<<
                      "; aGeom = "<<aGeom<<
                      "; aNbElem = "<<aNbElem<<
                      "; aNbSubElem = "<<aNbSubElem<<
                      "; aNbComp = "<<aNbComp<<
                      "; aNbGauss = "<<aNbGauss<<
                      "");
          }
        }else{
          if((aProfileMode == MED_GLOBAL_STMODE) && (aNbElem != aNbValue)){
            if(theErr){
              *theErr = -1;
              return;
            }
            EXCEPTION(std::runtime_error,
                      "GetTimeStampValue - aNbElem("<<aNbElem<<
                      ") != aNbValue("<<aNbValue<<
                      "); aNbVal = "<<aNbVal<<
                      "; anEntity = "<<anEntity<<
                      "; aGeom = "<<aGeom<<
                      "; aNbElem = "<<aNbElem<<
                      "; aNbComp = "<<aNbComp<<
                      "; aNbGauss = "<<aNbGauss<<
                      "");
          }
        }
      }
    }
    
    
    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::SetTimeStampValue(const MED::PTimeStampValueBase& theTimeStampValue,
                        EModeAcces theMode,
                        TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
        return;
      
      TErr aRet;
      TIdt anId = myFile->Id();
      
      TValueHolder<EModeSwitch, med_switch_mode> aModeSwitch(theTimeStampValue->myModeSwitch);
      MED::TGeom2Profile& aGeom2Profile = theTimeStampValue->myGeom2Profile;

      MED::PTimeStampInfo aTimeStampInfo = theTimeStampValue->myTimeStampInfo;
      TValueHolder<EEntiteMaillage, med_entity_type> anEntity(aTimeStampInfo->myEntity);
      TValueHolder<TInt, med_int> aNumDt(aTimeStampInfo->myNumDt);
      TValueHolder<TInt, med_int> aNumOrd(aTimeStampInfo->myNumOrd);
      TValueHolder<TString, char> anUnitDt(aTimeStampInfo->myUnitDt);
      TValueHolder<TFloat, med_float> aDt(aTimeStampInfo->myDt);
      MED::TGeom2Gauss& aGeom2Gauss = aTimeStampInfo->myGeom2Gauss;

      MED::PFieldInfo aFieldInfo = aTimeStampInfo->myFieldInfo;
      TValueHolder<TString, char> aFieldName(aFieldInfo->myName);

      MED::PMeshInfo aMeshInfo = aFieldInfo->myMeshInfo;
      TValueHolder<TString, char> aMeshName(aMeshInfo->myName);
      
      const TGeomSet& aGeomSet = theTimeStampValue->myGeomSet;
      TGeomSet::const_iterator anIter = aGeomSet.begin();
      for(; anIter != aGeomSet.end(); anIter++){
        EGeometrieElement aGeom = *anIter;

        TVector<char> aGaussName(GetNOMLength<eV2_2>()+1);
        MED::TGeom2Gauss::const_iterator aGaussIter = aGeom2Gauss.find(aGeom);
        if(aGaussIter != aGeom2Gauss.end()){
          MED::PGaussInfo aGaussInfo = aGaussIter->second;
          strcpy(&aGaussName[0],&aGaussInfo->myName[0]);
        }

        TVector<char> aProfileName(GetNOMLength<eV2_2>()+1);
        med_storage_mode aProfileMode = med_storage_mode(eNO_PFLMOD);
        MED::TGeom2Profile::const_iterator aProfileIter = aGeom2Profile.find(aGeom);
        if(aProfileIter != aGeom2Profile.end()){
          MED::PProfileInfo aProfileInfo = aProfileIter->second;
          aProfileMode = med_storage_mode(aProfileInfo->myMode);
          strcpy(&aProfileName[0],&aProfileInfo->myName[0]);
        }

        med_int aNbVal = theTimeStampValue->GetNbVal(aGeom);

        aRet = MEDfieldValueWithProfileWr(anId,
                                          &aFieldName,
                                          aNumDt,
                                          aNumOrd,
                                          aDt,
                                          anEntity,
                                          med_geometry_type(aGeom),
                                          aProfileMode,
                                          &aProfileName[0],
                                          &aGaussName[0],
                                          aModeSwitch,
                                          MED_ALL_CONSTITUENT,
                                          aNbVal,
                                          theTimeStampValue->GetValuePtr(aGeom));
        if(aRet < 0){
          if(theErr){
            *theErr = MED_FALSE;
            break;
          }
          EXCEPTION(std::runtime_error,"SetTimeStampValue - MEDfieldValueWithProfileWr(...)");
        }
        
      }
      
      INITMSG(MYDEBUG,"TVWrapper::SetTimeStampValue - MED_MODE_ACCES = "<<theMode<<"; aRet = "<<aRet<<std::endl);
    }

    
    //----------------------------------------------------------------------------
    void 
    TVWrapper
    ::SetTimeStampValue(const PTimeStampValueBase& theTimeStampValue,
                        TErr* theErr)
    {
      TErr aRet;
      SetTimeStampValue(theTimeStampValue,eLECTURE_ECRITURE,&aRet);
      
      if(aRet < 0)
        SetTimeStampValue(theTimeStampValue,eLECTURE_AJOUT,&aRet);

      if(theErr) 
        *theErr = aRet;
    }
    
    //----------------------------------------------------------------------------
    void 
    TVWrapper
    ::SetGrilleInfo(const MED::TGrilleInfo& theInfo,
                    TErr* theErr)
    {
      SetGrilleInfo(theInfo,eLECTURE_ECRITURE,theErr);
    }

    //----------------------------------------------------------------------------
    void 
    TVWrapper
    ::SetGrilleInfo(const MED::TGrilleInfo& theInfo,
                    EModeAcces theMode,
                    TErr* theErr)
    {
      if(theInfo.myMeshInfo->myType != eSTRUCTURE)
        return;
      TFileWrapper aFileWrapper(myFile,theMode,theErr);
      
      if(theErr && *theErr < 0)
          return;

      MED::TGrilleInfo& anInfo = const_cast<MED::TGrilleInfo&>(theInfo);

      MED::TMeshInfo& aMeshInfo = *anInfo.myMeshInfo;
      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);

      TValueHolder<EGrilleType, med_grid_type > aGrilleType(anInfo.myGrilleType);

      TErr aRet = 0;
      aRet = MEDmeshGridTypeRd(myFile->Id(),
                               &aMeshName,
                               &aGrilleType);
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"SetGrilleInfo - MEDmeshGridTypeRd(...)");
      
      if(anInfo.myGrilleType == eGRILLE_STANDARD){
        TValueHolder<TNodeCoord, med_float> aCoord(anInfo.myCoord);
        TValueHolder<EModeSwitch, med_switch_mode> aModeSwitch(anInfo.myModeSwitch);
        TValueHolder<TString, char> aCoordNames(anInfo.myCoordNames);
        TValueHolder<TString, char> aCoordUnits(anInfo.myCoordUnits);
        med_int aNbNoeuds = med_int(anInfo.myCoord.size() / aMeshInfo.myDim);
        //med_axis_type aRepere = MED_CARTESIAN;

        aRet = MEDmeshNodeCoordinateWr(myFile->Id(),
                                       &aMeshName,
                                       MED_NO_DT,
                                       MED_NO_IT,
                                       MED_UNDEF_DT,
                                       aModeSwitch,
                                       aNbNoeuds,
                                       &aCoord);

        if(aRet < 0)
          EXCEPTION(std::runtime_error,"SetGrilleInfo - MEDmeshNodeCoordinateWr(...)");

        TValueHolder<TIntVector, med_int> aGrilleStructure(anInfo.myGrilleStructure);
        aRet = MEDmeshGridStructWr(myFile->Id(),
                                    &aMeshName,
                                   MED_NO_DT,
                                   MED_NO_IT,
                                   MED_UNDEF_DT,
                                   &aGrilleStructure);
        if(aRet < 0)
          EXCEPTION(std::runtime_error,"SetGrilleInfo - MEDmeshGridStructWr(...)");
        
      } else {
        for(med_int aAxis = 0; aAxis < aMeshInfo.myDim; aAxis++){
          aRet = MEDmeshGridIndexCoordinateWr(myFile->Id(),
                                              &aMeshName,
                                              MED_NO_DT,
                                              MED_NO_IT,
                                              MED_UNDEF_DT,
                                              aAxis+1,
                                              anInfo.GetIndexes(aAxis).size(),
                                              &anInfo.GetIndexes(aAxis)[0]);

          if(aRet < 0)
            EXCEPTION(std::runtime_error,"SetGrilleInfo - MEDmeshGridIndexCoordinateWr(...)");
        }
        
      }

      return;
    }

    //----------------------------------------------------------------------------
    void
    TVWrapper
    ::GetGrilleInfo(TGrilleInfo& theInfo,
                    TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0)
          return;
      
      MED::TMeshInfo& aMeshInfo = *theInfo.myMeshInfo;
      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      EMaillage aMaillageType = aMeshInfo.myType;
      
      GetGrilleType(aMeshInfo, theInfo.myGrilleType, theErr);
      EGrilleType aGrilleType = theInfo.myGrilleType;

      TErr aRet = 0;
      if(aMaillageType == eSTRUCTURE && aGrilleType == eGRILLE_STANDARD) {
        GetGrilleStruct(aMeshInfo, theInfo.myGrilleStructure, theErr);

        TValueHolder<TNodeCoord, med_float> aCoord(theInfo.myCoord);
        TValueHolder<EModeSwitch, med_switch_mode> aModeSwitch(theInfo.myModeSwitch);
        TValueHolder<TString, char> aCoordNames(theInfo.myCoordNames);
        TValueHolder<TString, char> aCoordUnits(theInfo.myCoordUnits);
        //med_axis_type aRepere;

        aRet = MEDmeshNodeCoordinateRd(myFile->Id(),
                                       &aMeshName,
                                       MED_NO_DT,
                                       MED_NO_IT,
                                       aModeSwitch,
                                       &aCoord);

        if(theErr) 
          *theErr = aRet;
        else if(aRet < 0)
          EXCEPTION(std::runtime_error,"GetGrilleInfo - MEDmeshNodeCoordinateRd(...)");

        //TInt aNbNodes = theInfo.GetNbNodes();//GetNbFamilies(aMeshInfo);
        TValueHolder<TElemNum, med_int> aFamNumNode(theInfo.myFamNumNode);
        
        aRet = MEDmeshEntityFamilyNumberRd(myFile->Id(),
                                           &aMeshName,
                                           MED_NO_DT,
                                           MED_NO_IT,
                                           MED_NODE,
                                           MED_NO_GEOTYPE,
                                           &aFamNumNode);

        if(aRet < 0)
        {
//            if (aRet == MED_ERR_DOESNTEXIST) // --- only valid with MED3.x files
              {
                int mySize = (int)theInfo.myFamNumNode.size();
                theInfo.myFamNumNode.clear();
                theInfo.myFamNumNode.resize(mySize,0);
                aRet = 0;
              }
//            else
//              EXCEPTION(std::runtime_error,"GetGrilleInfo - MEDmeshEntityFamilyNumberRd(...)");
        }
        if(theErr) 
          *theErr = aRet;

        //============================
      }

      if(aMaillageType == eSTRUCTURE && aGrilleType != eGRILLE_STANDARD){
        ETable aTable;
        for(med_int anAxis = 1; anAxis <= aMeshInfo.myDim; anAxis++){
          switch(anAxis){
          case 1 :
            aTable = eCOOR_IND1;
            break;
          case 2 :
            aTable = eCOOR_IND2;
            break;
          case 3 :
            aTable = eCOOR_IND3;
            break;
          default :
            aRet = -1;
          }
            
          if(theErr) 
            *theErr = aRet;
          else if(aRet < 0)
            EXCEPTION(std::runtime_error,"GetGrilleInfo - anAxis number out of range(...)");
          
          TInt aNbIndexes = GetNbNodes(aMeshInfo,aTable);
          if(aNbIndexes < 0)
            EXCEPTION(std::runtime_error,"GetGrilleInfo - Erreur a la lecture de la taille de l'indice");
            
          TValueHolder<TFloatVector, med_float> anIndexes(theInfo.GetIndexes(anAxis-1));
          //TValueHolder<ETable, med_data_type > table(aTable);
          //char aCompNames[MED_SNAME_SIZE+1];
          //char anUnitNames[MED_SNAME_SIZE+1];
          aRet=MEDmeshGridIndexCoordinateRd(myFile->Id(),&aMeshName,
                                            MED_NO_DT,MED_NO_IT,
                                            anAxis,
                                            &anIndexes);

          //theInfo.SetCoordName(anAxis-1, aCompNames);
          //theInfo.SetCoordUnit(anAxis-1, anUnitNames);
          theInfo.SetGrilleStructure(anAxis-1, aNbIndexes);

          if(theErr) 
            *theErr = aRet;
          else if(aRet < 0)
            EXCEPTION(std::runtime_error,"GetGrilleInfo - MEDindicesCoordLire(...)");
        }
      }

      EGeometrieElement aGeom = theInfo.GetGeom();
      EEntiteMaillage aEntity = theInfo.GetEntity();
      TInt aNbCells = theInfo.GetNbCells();
      
      theInfo.myFamNum.resize(aNbCells);
      TValueHolder<TElemNum, med_int> aFamNum(theInfo.myFamNum);
      
      aRet = MEDmeshEntityFamilyNumberRd(myFile->Id(),
                                         &aMeshName,MED_NO_DT,MED_NO_IT,med_entity_type(aEntity),
                                         med_geometry_type(aGeom),&aFamNum);

      if ( aMeshInfo.myDim == 3 )
      {
        aGeom = theInfo.GetSubGeom();
        aEntity = theInfo.GetSubEntity();
        aNbCells = theInfo.GetNbSubCells();
      
        theInfo.myFamSubNum.resize(aNbCells,0);
        TValueHolder<TElemNum, med_int> aFamNum(theInfo.myFamSubNum);
      
        aRet = MEDmeshEntityFamilyNumberRd(myFile->Id(),
                                    &aMeshName,MED_NO_DT,MED_NO_IT,
                                    med_entity_type(aEntity),
                                    med_geometry_type(aGeom),&aFamNum);
      }
      if(aRet < 0)
      {
//          if (aRet == MED_ERR_DOESNTEXIST) // --- only valid with MED3.x files
            {
              int mySize = (int)theInfo.myFamNumNode.size();
              theInfo.myFamNumNode.clear();
              theInfo.myFamNumNode.resize(mySize,0);
              aRet = 0;
            }
//          else
//            EXCEPTION(std::runtime_error,"GetGrilleInfo - MEDmeshEntityFamilyNumberRd(...)");
      }
      if(theErr) 
        *theErr = aRet;
    }

    void
    TVWrapper
    ::GetGrilleType(const MED::TMeshInfo& theMeshInfo,
                    EGrilleType& theGridType,
                    TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);

      if(theErr && *theErr < 0)
        EXCEPTION(std::runtime_error," GetGrilleType - aFileWrapper (...)");

      MED::TMeshInfo& aMeshInfo = const_cast<MED::TMeshInfo&>(theMeshInfo);
      
      if(aMeshInfo.myType == eSTRUCTURE){
        TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
        TValueHolder<EGrilleType, med_grid_type> aGridType(theGridType);
        TErr aRet = MEDmeshGridTypeRd(myFile->Id(),
                                      &aMeshName,
                                      &aGridType);

        if(aRet < 0)
          EXCEPTION(std::runtime_error,"GetGrilleInfo - MEDmeshGridTypeRd(...)");
      }
    }    
    
    void
    TVWrapper
    ::GetGrilleStruct(const MED::TMeshInfo& theMeshInfo,
                      TIntVector& theStruct,
                      TErr* theErr)
    {
      TFileWrapper aFileWrapper(myFile,eLECTURE,theErr);
      
      if(theErr && *theErr < 0)
          return;
      
      TErr aRet;
      MED::TMeshInfo& aMeshInfo = const_cast<MED::TMeshInfo&>(theMeshInfo);

      TValueHolder<TString, char> aMeshName(aMeshInfo.myName);
      TValueHolder<TIntVector, med_int> aGridStructure(theStruct);
        
      aRet = MEDmeshGridStructRd(myFile->Id(),
                                 &aMeshName,
                                 MED_NO_DT,
                                 MED_NO_IT,
                                 &aGridStructure);
      if(theErr) 
        *theErr = aRet;
      else if(aRet < 0)
        EXCEPTION(std::runtime_error,"GetGrilleInfo - MEDmeshGridStructRd(...)");
    }

  }
}
