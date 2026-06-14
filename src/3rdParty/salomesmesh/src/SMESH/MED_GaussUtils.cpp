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
#include "MED_GaussUtils.hxx"
#include "MED_Utilities.hxx"
 
#ifdef _DEBUG_
static int MYDEBUG = 0;
static int MYVALUEDEBUG = 0;
#else
// static int MYDEBUG = 0;
// static int MYVALUEDEBUG = 0;
#endif

//#define _DEBUG_REF_COORDS_

namespace MED
{
  //---------------------------------------------------------------
  TGaussCoord
  ::TGaussCoord():
    TModeSwitchInfo(eFULL_INTERLACE),
    myNbElem(0),
    myNbGauss(0),
    myDim(0),
    myGaussStep(0)
  {
  }

  void
  TGaussCoord
  ::Init(TInt theNbElem,
         TInt theNbGauss,
         TInt theDim,
         EModeSwitch theMode)
  {
    myModeSwitch = theMode;

    myNbElem = theNbElem;
    myNbGauss = theNbGauss;
    myDim = theDim;

    myGaussStep = myNbGauss*myDim;

    myGaussCoord.resize(theNbElem*myGaussStep);
  }


  TInt
  TGaussCoord
  ::GetNbElem() const
  { 
    return myNbElem; 
  }
  
  TInt
  TGaussCoord
  ::GetNbGauss() const
  { 
    return myNbGauss; 
  }
  
  TInt
  TGaussCoord
  ::GetDim() const
  { 
    return myDim; 
  }
  
  unsigned char*
  TGaussCoord
  ::GetValuePtr()
  {
    return (unsigned char*)&(myGaussCoord[0]);
  }


  TCCoordSliceArr 
  TGaussCoord
  ::GetCoordSliceArr(TInt theElemId) const
  {
    TCCoordSliceArr aCoordSliceArr(myNbGauss);
    if(GetModeSwitch() == eFULL_INTERLACE){
      TInt anId = theElemId*myGaussStep;
      for(TInt anGaussId = 0; anGaussId < myNbGauss; anGaussId++){
        aCoordSliceArr[anGaussId] =
          TCCoordSlice(myGaussCoord,std::slice(anId,myDim,1));
        anId += myDim;
      }
    }
    else{
      for(TInt anGaussId = 0; anGaussId < myNbGauss; anGaussId++){
        aCoordSliceArr[anGaussId] =
          TCCoordSlice(myGaussCoord,std::slice(theElemId,myDim,myGaussStep));
      }
    }
    return aCoordSliceArr;
  }


  TCoordSliceArr 
  TGaussCoord
  ::GetCoordSliceArr(TInt theElemId)
  {
    TCoordSliceArr aCoordSliceArr(myNbGauss);
    if(GetModeSwitch() == eFULL_INTERLACE){
      TInt anId = theElemId*myGaussStep;
      for(TInt anGaussId = 0; anGaussId < myNbGauss; anGaussId++){
        aCoordSliceArr[anGaussId] =
          TCoordSlice(myGaussCoord,std::slice(anId,myDim,1));
        anId += myDim;
      }
    }
    else{
      for(TInt anGaussId = 0; anGaussId < myNbGauss; anGaussId++){
        aCoordSliceArr[anGaussId] =
          TCoordSlice(myGaussCoord,std::slice(theElemId,myDim,myGaussStep));
      }
    }
    return aCoordSliceArr;
  }


  //---------------------------------------------------------------
  inline
  bool 
  IsEqual(TFloat theLeft, TFloat theRight)
  {
    static TFloat EPS = 1.0E-3;
    if(fabs(theLeft) + fabs(theRight) > EPS)
      return fabs(theLeft-theRight)/(fabs(theLeft)+fabs(theRight)) < EPS;
    return true;
  }


  //---------------------------------------------------------------
  class TShapeFun::TFun
  {
    TFloatVector myFun;
    TInt myNbRef;

  public:

    void
    Init(TInt theNbGauss,
         TInt theNbRef)
    {
      myFun.resize(theNbGauss*theNbRef);
      myNbRef = theNbRef;
    }

    TCFloatVecSlice 
    GetFunSlice(TInt theGaussId) const
    {
      return TCFloatVecSlice(myFun,std::slice(theGaussId*myNbRef,myNbRef,1));
    }

    TFloatVecSlice
    GetFunSlice(TInt theGaussId)
    {
      return TFloatVecSlice(myFun,std::slice(theGaussId*myNbRef,myNbRef,1));
    }
  };

  //---------------------------------------------------------------

  TShapeFun::TShapeFun(TInt theDim, TInt theNbRef):
    myRefCoord(theNbRef*theDim),
    myDim(theDim),
    myNbRef(theNbRef)
  {}

  TCCoordSlice 
  TShapeFun::GetCoord(TInt theRefId) const
  {
    return TCCoordSlice(myRefCoord,std::slice(theRefId*myDim,myDim,1));
  }

  TCoordSlice
  TShapeFun::GetCoord(TInt theRefId)
  {
    return TCoordSlice(myRefCoord,std::slice(theRefId*myDim,myDim,1));
  }

  void 
  TShapeFun::GetFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    TInt aNbRef = theRef.size();
    TInt aNbGauss = theGauss.size();
    theFun.Init(aNbGauss,aNbRef);
  }

  bool 
  TShapeFun::IsSatisfy(const TCCoordSliceArr& theRefCoord) const
  {
    TInt aNbRef = theRefCoord.size();
    TInt aNbRef2 = GetNbRef();
    INITMSG(MYDEBUG,"TShapeFun::IsSatisfy "<<
            "- aNbRef("<<aNbRef<<")"<<
            "; aNbRef2("<<aNbRef2<<")\n");
    bool anIsSatisfy = (aNbRef == aNbRef2);
    if(anIsSatisfy){
      for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
        const TCCoordSlice& aCoord2 = theRefCoord[aRefId];
        TCCoordSlice aCoord = GetCoord(aRefId);
        TInt aDim = aCoord.size();
        bool anIsEqual = false;
        for(TInt anId = 0; anId < aDim; anId++){
          anIsEqual = IsEqual(aCoord[anId],aCoord2[anId]);
          if(!anIsEqual){
            anIsSatisfy = false;
            break;
          }
        }
        if(!anIsEqual){
#ifdef _DEBUG_
          TCCoordSlice aCoord = GetCoord(aRefId);
          INITMSG(MYDEBUG,aRefId + 1<<": aCoord = {");
          TInt aDim = aCoord.size();
          for(TInt anId = 0; anId < aDim; anId++)
            ADDMSG(MYDEBUG,"\t"<<aCoord[anId]);
          const TCCoordSlice& aCoord2 = theRefCoord[aRefId];
          ADDMSG(MYDEBUG,"}\t!=\taCoord2 = {");
          for(TInt anId = 0; anId < aDim; anId++)
            ADDMSG(MYDEBUG,"\t"<<aCoord2[anId]);
          ADDMSG(MYDEBUG,"}\n");
#endif
#ifndef _DEBUG_
          BEGMSG(MYDEBUG,"anIsSatisfy = "<<anIsSatisfy<<"\n");
          return anIsSatisfy;
#endif
        }
      }
    }

    BEGMSG(MYDEBUG,"anIsSatisfy = "<<anIsSatisfy<<"\n");
    return anIsSatisfy;
  }

  bool
  TShapeFun::Eval(const TCellInfo&       theCellInfo,
                  const TNodeInfo&       theNodeInfo,
                  const TElemNum&        theElemNum,
                  const TCCoordSliceArr& theRef,
                  const TCCoordSliceArr& theGauss,
                  TGaussCoord&           theGaussCoord,
                  EModeSwitch            theMode)
  {
    INITMSG(MYDEBUG,"TShapeFun::Eval"<<std::endl);

    if(IsSatisfy(theRef)){
      const PMeshInfo& aMeshInfo = theCellInfo.GetMeshInfo();
      TInt aDim = aMeshInfo->GetDim();
      TInt aNbGauss = theGauss.size();

      bool anIsSubMesh = !theElemNum.empty();
      TInt aNbElem;
      if(anIsSubMesh)
        aNbElem = theElemNum.size();
      else
        aNbElem = theCellInfo.GetNbElem();

      theGaussCoord.Init(aNbElem,aNbGauss,aDim,theMode);

      TFun aFun;
      InitFun(theRef,theGauss,aFun);
      TInt aConnDim = theCellInfo.GetConnDim();

      INITMSG(MYDEBUG,"aDim = "<<aDim<<
              "; aNbGauss = "<<aNbGauss<<
              "; aNbElem = "<<aNbElem<<
              "; aNbNodes = "<<theNodeInfo.GetNbElem()<<
              std::endl);

      for(TInt anElemId = 0; anElemId < aNbElem; anElemId++){
        TInt aCellId = anIsSubMesh? theElemNum[anElemId]-1: anElemId;
        TCConnSlice aConnSlice = theCellInfo.GetConnSlice(aCellId);
        TCoordSliceArr aCoordSliceArr = theGaussCoord.GetCoordSliceArr(anElemId);

        for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
          TCoordSlice& aGaussCoordSlice = aCoordSliceArr[aGaussId];
          TCFloatVecSlice aFunSlice = aFun.GetFunSlice(aGaussId);

          for(TInt aConnId = 0; aConnId < aConnDim; aConnId++){
            TInt aNodeId = aConnSlice[aConnId] - 1;      
            TCCoordSlice aNodeCoordSlice = theNodeInfo.GetCoordSlice(aNodeId);

            for(TInt aDimId = 0; aDimId < aDim; aDimId++){
              aGaussCoordSlice[aDimId] += aNodeCoordSlice[aDimId]*aFunSlice[aConnId];
            }
          }
        }
      }

#ifdef _DEBUG_
      {
        INITMSG(MYVALUEDEBUG,"theGauss: ");
        for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
          TCCoordSlice aCoordSlice = theGauss[aGaussId];
          ADDMSG(MYVALUEDEBUG,"{");
          for(TInt aDimId = 0; aDimId < aDim; aDimId++){
            ADDMSG(MYVALUEDEBUG,aCoordSlice[aDimId]<<" ");
          }
          ADDMSG(MYVALUEDEBUG,"} ");
        }
        ADDMSG(MYVALUEDEBUG,std::endl);
      }
      for(TInt anElemId = 0; anElemId < aNbElem; anElemId++){
        TCCoordSliceArr aCoordSliceArr = theGaussCoord.GetCoordSliceArr(anElemId);
        INITMSG(MYVALUEDEBUG,"");
        for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
          TCCoordSlice aCoordSlice = aCoordSliceArr[aGaussId];
          ADDMSG(MYVALUEDEBUG,"{");
          for(TInt aDimId = 0; aDimId < aDim; aDimId++){
            ADDMSG(MYVALUEDEBUG,aCoordSlice[aDimId]<<" ");
          }
          ADDMSG(MYVALUEDEBUG,"} ");
        }
        ADDMSG(MYVALUEDEBUG,std::endl);
      }
#endif
      return true;
    }

    return false;
  }


  //---------------------------------------------------------------
  TSeg2a::TSeg2a():TShapeFun(1,2)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0; break;
      case  1: aCoord[0] =  1.0; break;
      }
    }
  }

  void
  TSeg2a::InitFun(const TCCoordSliceArr& theRef,
                  const TCCoordSliceArr& theGauss,
                  TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*(1.0 - aCoord[0]);
      aSlice[1] = 0.5*(1.0 + aCoord[0]);
    }
  }


  //---------------------------------------------------------------
  TSeg3a::TSeg3a():TShapeFun(1,3)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0; break;
      case  1: aCoord[0] =  1.0; break;
      case  2: aCoord[0] =  0.0; break;
      }
    }
  }

  void
  TSeg3a::InitFun(const TCCoordSliceArr& theRef,
                  const TCCoordSliceArr& theGauss,
                  TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*(1.0 - aCoord[0])*aCoord[0];
      aSlice[1] = 0.5*(1.0 + aCoord[0])*aCoord[0];
      aSlice[2] = (1.0 + aCoord[0])*(1.0 - aCoord[0]);
    }
  }



  //---------------------------------------------------------------
  TTria3a::TTria3a():
    TShapeFun(2,3)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] =  1.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] = -1.0; break;
      }
    }
  }

  void
  TTria3a::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*(1.0 + aCoord[1]);
      aSlice[1] = -0.5*(aCoord[0] + aCoord[1]);
      aSlice[2] = 0.5*(1.0 + aCoord[0]);
    }
  }



  //---------------------------------------------------------------
  TTria6a::TTria6a():TShapeFun(2,6)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] =  1.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] = -1.0; break;

      case  3: aCoord[0] = -1.0;  aCoord[1] =  1.0; break;
      case  4: aCoord[0] =  0.0;  aCoord[1] = -1.0; break;
      case  5: aCoord[0] =  0.0;  aCoord[1] =  0.0; break;
      }
    }
  }

  void
  TTria6a::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*(1.0 + aCoord[1])*aCoord[1];
      aSlice[1] = 0.5*(aCoord[0] + aCoord[1])*(aCoord[0] + aCoord[1] + 1);
      aSlice[2] = 0.5*(1.0 + aCoord[0])*aCoord[0];

      aSlice[3] = -1.0*(1.0 + aCoord[1])*(aCoord[0] + aCoord[1]);
      aSlice[4] = -1.0*(1.0 + aCoord[0])*(aCoord[0] + aCoord[1]);
      aSlice[5] = (1.0 + aCoord[1])*(1.0 + aCoord[1]);
    }
  }



  //---------------------------------------------------------------
  TTria3b::TTria3b():
    TShapeFun(2,3)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] =  0.0;  aCoord[1] =  0.0; break;
      case  1: aCoord[0] =  1.0;  aCoord[1] =  0.0; break;
      case  2: aCoord[0] =  0.0;  aCoord[1] =  1.0; break;
      }
    }
  }

  void
  TTria3b::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 1.0 - aCoord[0] - aCoord[1];
      aSlice[1] = aCoord[0];
      aSlice[2] = aCoord[1];
    }
  }



  //---------------------------------------------------------------
  TTria6b::TTria6b():
    TShapeFun(2,6)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] =  0.0;  aCoord[1] =  0.0; break;
      case  1: aCoord[0] =  1.0;  aCoord[1] =  0.0; break;
      case  2: aCoord[0] =  0.0;  aCoord[1] =  1.0; break;

      case  3: aCoord[0] =  0.5;  aCoord[1] =  0.0; break;
      case  4: aCoord[0] =  0.5;  aCoord[1] =  0.5; break;
      case  5: aCoord[0] =  0.0;  aCoord[1] =  0.5; break;
      }
    }
  }

  void
  TTria6b::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = (1.0 - aCoord[0] - aCoord[1])*(1.0 - 2.0*aCoord[0] - 2.0*aCoord[1]);
      aSlice[1] = aCoord[0]*(2.0*aCoord[0] - 1.0);
      aSlice[2] = aCoord[1]*(2.0*aCoord[1] - 1.0);

      aSlice[3] = 4.0*aCoord[0]*(1.0 - aCoord[0] - aCoord[1]);
      aSlice[4] = 4.0*aCoord[0]*aCoord[1];
      aSlice[5] = 4.0*aCoord[1]*(1.0 - aCoord[0] - aCoord[1]);
    }
  }



  //---------------------------------------------------------------
  TQuad4a::TQuad4a():
    TShapeFun(2,4)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] =  1.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] = -1.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  1.0; break;
      }
    }
  }

  void
  TQuad4a::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.25*(1.0 + aCoord[1])*(1.0 - aCoord[0]);
      aSlice[1] = 0.25*(1.0 - aCoord[1])*(1.0 - aCoord[0]);
      aSlice[2] = 0.25*(1.0 - aCoord[1])*(1.0 + aCoord[0]);
      aSlice[3] = 0.25*(1.0 + aCoord[0])*(1.0 + aCoord[1]);
    }
  }



  //---------------------------------------------------------------
  TQuad8a::TQuad8a():
    TShapeFun(2,8)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] =  1.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] = -1.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  1.0; break;

      case  4: aCoord[0] = -1.0;  aCoord[1] =  0.0; break;
      case  5: aCoord[0] =  0.0;  aCoord[1] = -1.0; break;
      case  6: aCoord[0] =  1.0;  aCoord[1] =  0.0; break;
      case  7: aCoord[0] =  0.0;  aCoord[1] =  1.0; break;
      }
    }
  }

  void
  TQuad8a::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.25*(1.0 + aCoord[1])*(1.0 - aCoord[0])*(aCoord[1] - aCoord[0] - 1.0);
      aSlice[1] = 0.25*(1.0 - aCoord[1])*(1.0 - aCoord[0])*(-aCoord[1] - aCoord[0] - 1.0);
      aSlice[2] = 0.25*(1.0 - aCoord[1])*(1.0 + aCoord[0])*(-aCoord[1] + aCoord[0] - 1.0);
      aSlice[3] = 0.25*(1.0 + aCoord[1])*(1.0 + aCoord[0])*(aCoord[1] + aCoord[0] - 1.0);

      aSlice[4] = 0.5*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[1]);
      aSlice[5] = 0.5*(1.0 - aCoord[1])*(1.0 - aCoord[0])*(1.0 + aCoord[0]);
      aSlice[6] = 0.5*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[1]);
      aSlice[7] = 0.5*(1.0 + aCoord[1])*(1.0 - aCoord[0])*(1.0 + aCoord[0]);
    }
  }



  //---------------------------------------------------------------
  TQuad9a::TQuad9a():
    TShapeFun(2,9)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] =  1.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] = -1.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  1.0; break;

      case  4: aCoord[0] = -1.0;  aCoord[1] =  0.0; break;
      case  5: aCoord[0] =  0.0;  aCoord[1] = -1.0; break;
      case  6: aCoord[0] =  1.0;  aCoord[1] =  0.0; break;
      case  7: aCoord[0] =  0.0;  aCoord[1] =  1.0; break;

      case  8: aCoord[0] =  0.0;  aCoord[1] =  0.0; break;
      }
    }
  }

  void
  TQuad9a::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.25*aCoord[0]*aCoord[1]*(aCoord[0] + 1.0)*(aCoord[1] - 1.0);
      aSlice[1] = 0.25*aCoord[0]*aCoord[1]*(aCoord[0] + 1.0)*(aCoord[1] + 1.0);
      aSlice[2] = 0.25*aCoord[0]*aCoord[1]*(aCoord[0] - 1.0)*(aCoord[1] + 1.0);
      aSlice[3] = 0.25*aCoord[0]*aCoord[1]*(aCoord[0] - 1.0)*(aCoord[1] - 1.0);

      aSlice[4] = 0.5*aCoord[0]*(aCoord[0] + 1.0)*(1.0 - aCoord[1]*aCoord[1]);
      aSlice[5] = 0.5*(1.0 - aCoord[0]*aCoord[0])*aCoord[1]*(aCoord[1] + 1.0);
      aSlice[6] = 0.5*aCoord[0]*(aCoord[0] - 1.0)*(1.0 - aCoord[1]*aCoord[1]);
      aSlice[7] = 0.5*(1.0 - aCoord[0]*aCoord[0])*aCoord[1]*(aCoord[1] - 1.0);

      aSlice[8] = (1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1]*aCoord[1]);
    }
  }



  //---------------------------------------------------------------
  TQuad4b::TQuad4b():
    TShapeFun(2,4)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] = -1.0; break;
      case  1: aCoord[0] =  1.0;  aCoord[1] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] =  1.0; break;
      case  3: aCoord[0] = -1.0;  aCoord[1] =  1.0; break;
      }
    }
  }

  void
  TQuad4b::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.25*(1.0 - aCoord[0])*(1.0 - aCoord[1]);
      aSlice[1] = 0.25*(1.0 + aCoord[0])*(1.0 - aCoord[1]);
      aSlice[2] = 0.25*(1.0 + aCoord[0])*(1.0 + aCoord[1]);
      aSlice[3] = 0.25*(1.0 - aCoord[0])*(1.0 + aCoord[1]);
    }
  }



  //---------------------------------------------------------------
  TQuad8b::TQuad8b():
    TShapeFun(2,8)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] = -1.0; break;
      case  1: aCoord[0] =  1.0;  aCoord[1] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] =  1.0; break;
      case  3: aCoord[0] = -1.0;  aCoord[1] =  1.0; break;

      case  4: aCoord[0] =  0.0;  aCoord[1] = -1.0; break;
      case  5: aCoord[0] =  1.0;  aCoord[1] =  0.0; break;
      case  6: aCoord[0] =  0.0;  aCoord[1] =  1.0; break;
      case  7: aCoord[0] = -1.0;  aCoord[1] =  0.0; break;
      }
    }
  }

  void
  TQuad8b::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.25*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(-1.0 - aCoord[0] - aCoord[1]);
      aSlice[1] = 0.25*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(-1.0 + aCoord[0] - aCoord[1]);
      aSlice[2] = 0.25*(1.0 + aCoord[0])*(1.0 + aCoord[1])*(-1.0 + aCoord[0] + aCoord[1]);
      aSlice[3] = 0.25*(1.0 - aCoord[0])*(1.0 + aCoord[1])*(-1.0 - aCoord[0] + aCoord[1]);

      aSlice[4] = 0.5*(1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1]);
      aSlice[5] = 0.5*(1.0 - aCoord[1]*aCoord[1])*(1.0 + aCoord[0]);
      aSlice[6] = 0.5*(1.0 - aCoord[0]*aCoord[0])*(1.0 + aCoord[1]);
      aSlice[7] = 0.5*(1.0 - aCoord[1]*aCoord[1])*(1.0 - aCoord[0]);

      //aSlice[4] = 0.5*(1.0 - aCoord[0])*(1.0 - aCoord[0])*(1.0 - aCoord[1]);
      //aSlice[5] = 0.5*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[1]);
      //aSlice[6] = 0.5*(1.0 - aCoord[0])*(1.0 - aCoord[0])*(1.0 + aCoord[1]);
      //aSlice[7] = 0.5*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[1]);
    }
  }



  //---------------------------------------------------------------
  TQuad9b::TQuad9b():
    TShapeFun(2,9)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] = -1.0; break;
      case  1: aCoord[0] =  1.0;  aCoord[1] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] =  1.0; break;
      case  3: aCoord[0] = -1.0;  aCoord[1] =  1.0; break;

      case  4: aCoord[0] =  0.0;  aCoord[1] = -1.0; break;
      case  5: aCoord[0] =  1.0;  aCoord[1] =  0.0; break;
      case  6: aCoord[0] =  0.0;  aCoord[1] =  1.0; break;
      case  7: aCoord[0] = -1.0;  aCoord[1] =  0.0; break;

      case  8: aCoord[0] =  0.0;  aCoord[1] =  0.0; break;
      }
    }
  }

  void
  TQuad9b::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.25*aCoord[0]*aCoord[1]*(aCoord[0] - 1.0)*(aCoord[1] - 1.0);
      aSlice[1] = 0.25*aCoord[0]*aCoord[1]*(aCoord[0] + 1.0)*(aCoord[1] - 1.0);
      aSlice[2] = 0.25*aCoord[0]*aCoord[1]*(aCoord[0] + 1.0)*(aCoord[1] + 1.0);
      aSlice[3] = 0.25*aCoord[0]*aCoord[1]*(aCoord[0] - 1.0)*(aCoord[1] + 1.0);

      aSlice[4] = 0.5*(1.0 - aCoord[0]*aCoord[0])*aCoord[1]*(aCoord[1] - 1.0);
      aSlice[5] = 0.5*aCoord[0]*(aCoord[0] + 1.0)*(1.0 - aCoord[1]*aCoord[1]);
      aSlice[6] = 0.5*(1.0 - aCoord[0]*aCoord[0])*aCoord[1]*(aCoord[1] + 1.0);
      aSlice[7] = 0.5*aCoord[0]*(aCoord[0] - 1.0)*(1.0 - aCoord[1]*aCoord[1]);

      aSlice[8] = (1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1]*aCoord[1]);
    }
  }



  //---------------------------------------------------------------
  TTetra4a::TTetra4a():
    TShapeFun(3,4)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  1: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case  2: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      }
    }
  }

  void
  TTetra4a::InitFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = aCoord[1];
      aSlice[1] = aCoord[2];
      aSlice[2] = 1.0 - aCoord[0] - aCoord[1] - aCoord[2];
      aSlice[3] = aCoord[0];
    }
  }



  //---------------------------------------------------------------
  TTetra10a::TTetra10a():
    TShapeFun(3,10)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  1: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case  2: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;

      case  4: aCoord[0] =  0.0;  aCoord[1] =  0.5;  aCoord[2] =  0.5; break;
      case  5: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case  6: aCoord[0] =  0.0;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;

      case  7: aCoord[0] =  0.5;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      case  8: aCoord[0] =  0.5;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case  9: aCoord[0] =  0.5;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      }
    }
  }

  void
  TTetra10a::InitFun(const TCCoordSliceArr& theRef,
                     const TCCoordSliceArr& theGauss,
                     TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = aCoord[1]*(2.0*aCoord[1] - 1.0);
      aSlice[1] = aCoord[2]*(2.0*aCoord[2] - 1.0);
      aSlice[2] = (1.0 - aCoord[0] - aCoord[1] - aCoord[2])*(1.0 - 2.0*aCoord[0] - 2.0*aCoord[1] - 2.0*aCoord[2]);
      aSlice[3] = aCoord[0]*(2.0*aCoord[0] - 1.0);

      aSlice[4] = 4.0*aCoord[1]*aCoord[2];
      aSlice[5] = 4.0*aCoord[2]*(1.0 - aCoord[0] - aCoord[1] - aCoord[2]);
      aSlice[6] = 4.0*aCoord[1]*(1.0 - aCoord[0] - aCoord[1] - aCoord[2]);

      aSlice[7] = 4.0*aCoord[0]*aCoord[1];
      aSlice[8] = 4.0*aCoord[0]*aCoord[2];
      aSlice[9] = 4.0*aCoord[0]*(1.0 - aCoord[0] - aCoord[1] - aCoord[2]);
    }
  }



  //---------------------------------------------------------------


  TTetra4b::TTetra4b():
    TShapeFun(3,4)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  2: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case  1: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      }
    }
  }

  void
  TTetra4b::InitFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = aCoord[1];
      aSlice[2] = aCoord[2];
      aSlice[1] = 1.0 - aCoord[0] - aCoord[1] - aCoord[2];
      aSlice[3] = aCoord[0];
    }
  }



  //---------------------------------------------------------------
  TTetra10b::TTetra10b():
    TShapeFun(3,10)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  2: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case  1: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;

      case  6: aCoord[0] =  0.0;  aCoord[1] =  0.5;  aCoord[2] =  0.5; break;
      case  5: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case  4: aCoord[0] =  0.0;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;

      case  7: aCoord[0] =  0.5;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      case  9: aCoord[0] =  0.5;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case  8: aCoord[0] =  0.5;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      }
    }
  }

  void
  TTetra10b::InitFun(const TCCoordSliceArr& theRef,
                     const TCCoordSliceArr& theGauss,
                     TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = aCoord[1]*(2.0*aCoord[1] - 1.0);
      aSlice[2] = aCoord[2]*(2.0*aCoord[2] - 1.0);
      aSlice[1] = (1.0 - aCoord[0] - aCoord[1] - aCoord[2])*(1.0 - 2.0*aCoord[0] - 2.0*aCoord[1] - 2.0*aCoord[2]);
      aSlice[3] = aCoord[0]*(2.0*aCoord[0] - 1.0);

      aSlice[6] = 4.0*aCoord[1]*aCoord[2];
      aSlice[5] = 4.0*aCoord[2]*(1.0 - aCoord[0] - aCoord[1] - aCoord[2]);
      aSlice[4] = 4.0*aCoord[1]*(1.0 - aCoord[0] - aCoord[1] - aCoord[2]);

      aSlice[7] = 4.0*aCoord[0]*aCoord[1];
      aSlice[9] = 4.0*aCoord[0]*aCoord[2];
      aSlice[8] = 4.0*aCoord[0]*(1.0 - aCoord[0] - aCoord[1] - aCoord[2]);
    }
  }



  //---------------------------------------------------------------
  THexa8a::THexa8a():
    TShapeFun(3,8)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case  1: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case  3: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case  4: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case  5: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case  6: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;
      case  7: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;
      }
    }
  }

  void
  THexa8a::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.125*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2]);
      aSlice[1] = 0.125*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2]);
      aSlice[2] = 0.125*(1.0 + aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2]);
      aSlice[3] = 0.125*(1.0 - aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2]);

      aSlice[4] = 0.125*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2]);
      aSlice[5] = 0.125*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2]);
      aSlice[6] = 0.125*(1.0 + aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2]);
      aSlice[7] = 0.125*(1.0 - aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2]);
    }
  }


  //---------------------------------------------------------------
  THexa20a::THexa20a(TInt theDim, TInt theNbRef):
    TShapeFun(theDim,theNbRef)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case  1: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case  3: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case  4: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case  5: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case  6: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;
      case  7: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;

      case  8: aCoord[0] =  0.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case  9: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] = -1.0; break;
      case 10: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case 11: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] = -1.0; break;
      case 12: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] =  0.0; break;
      case 13: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] =  0.0; break;
      case 14: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case 15: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case 16: aCoord[0] =  0.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case 17: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case 18: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;
      case 19: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      }
    }
  }

  void
  THexa20a::InitFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.125*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2])*
        (-2.0 - aCoord[0] - aCoord[1] - aCoord[2]);
      aSlice[1] = 0.125*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2])*
        (-2.0 + aCoord[0] - aCoord[1] - aCoord[2]);
      aSlice[2] = 0.125*(1.0 + aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2])*
        (-2.0 + aCoord[0] + aCoord[1] - aCoord[2]);
      aSlice[3] = 0.125*(1.0 - aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2])*
        (-2.0 - aCoord[0] + aCoord[1] - aCoord[2]);
      aSlice[4] = 0.125*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2])*
        (-2.0 - aCoord[0] - aCoord[1] + aCoord[2]);
      aSlice[5] = 0.125*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2])*
        (-2.0 + aCoord[0] - aCoord[1] + aCoord[2]);
      aSlice[6] = 0.125*(1.0 + aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2])*
        (-2.0 + aCoord[0] + aCoord[1] + aCoord[2]);
      aSlice[7] = 0.125*(1.0 - aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2])*
        (-2.0 - aCoord[0] + aCoord[1] + aCoord[2]);

      aSlice[8] = 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2]);
      aSlice[9] = 0.25*(1.0 - aCoord[1]*aCoord[1])*(1.0 + aCoord[0])*(1.0 - aCoord[2]);
      aSlice[10] = 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2]);
      aSlice[11] = 0.25*(1.0 - aCoord[1]*aCoord[1])*(1.0 - aCoord[0])*(1.0 - aCoord[2]);
      aSlice[12] = 0.25*(1.0 - aCoord[2]*aCoord[2])*(1.0 - aCoord[0])*(1.0 - aCoord[1]);
      aSlice[13] = 0.25*(1.0 - aCoord[2]*aCoord[2])*(1.0 + aCoord[0])*(1.0 - aCoord[1]);
      aSlice[14] = 0.25*(1.0 - aCoord[2]*aCoord[2])*(1.0 + aCoord[0])*(1.0 + aCoord[1]);
      aSlice[15] = 0.25*(1.0 - aCoord[2]*aCoord[2])*(1.0 - aCoord[0])*(1.0 + aCoord[1]);
      aSlice[16] = 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2]);
      aSlice[17] = 0.25*(1.0 - aCoord[1]*aCoord[1])*(1.0 + aCoord[0])*(1.0 + aCoord[2]);
      aSlice[18] = 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2]);
      aSlice[19] = 0.25*(1.0 - aCoord[1]*aCoord[1])*(1.0 - aCoord[0])*(1.0 + aCoord[2]);
    }
  }



  //---------------------------------------------------------------
  THexa27a::THexa27a():
    THexa20a(3,27)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case 20: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] = -1.0; break;
      case 21: aCoord[0] =  0.0;  aCoord[1] = -1.0;  aCoord[2] =  0.0; break;
      case 22: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case 23: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case 24: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case 25: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case 26: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      }
    }
  }

  void
  THexa27a::InitFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.125*aCoord[0]*(aCoord[0] - 1.0)*aCoord[1]*(aCoord[1] - 1.0)*aCoord[2]*(aCoord[2] - 1.0);
      aSlice[1] = 0.125*aCoord[0]*(aCoord[0] + 1.0)*aCoord[1]*(aCoord[1] - 1.0)*aCoord[2]*(aCoord[2] - 1.0);
      aSlice[2] = 0.125*aCoord[0]*(aCoord[0] + 1.0)*aCoord[1]*(aCoord[1] + 1.0)*aCoord[2]*(aCoord[2] - 1.0);
      aSlice[3] = 0.125*aCoord[0]*(aCoord[0] - 1.0)*aCoord[1]*(aCoord[1] + 1.0)*aCoord[2]*(aCoord[2] - 1.0);
      aSlice[4] = 0.125*aCoord[0]*(aCoord[0] + 1.0)*aCoord[1]*(aCoord[1] - 1.0)*aCoord[2]*(aCoord[2] + 1.0);
      aSlice[5] = 0.125*aCoord[0]*(aCoord[0] + 1.0)*aCoord[1]*(aCoord[1] - 1.0)*aCoord[2]*(aCoord[2] + 1.0);
      aSlice[6] = 0.125*aCoord[0]*(aCoord[0] + 1.0)*aCoord[1]*(aCoord[1] + 1.0)*aCoord[2]*(aCoord[2] + 1.0);
      aSlice[7] = 0.125*aCoord[0]*(aCoord[0] - 1.0)*aCoord[1]*(aCoord[1] + 1.0)*aCoord[2]*(aCoord[2] + 1.0);

      aSlice[8] = 0.25*(1.0 - aCoord[0]*aCoord[0])*aCoord[1]*(aCoord[1] - 1.0)*aCoord[2]*(aCoord[2] - 1.0);
      aSlice[9] = 0.25*aCoord[0]*(aCoord[0] + 1.0)*(1.0 - aCoord[1]*aCoord[1])*aCoord[2]*(aCoord[2] - 1.0);
      aSlice[10]= 0.25*aCoord[0]*(aCoord[0] + 1.0)*(1.0 - aCoord[1]*aCoord[1])*aCoord[2]*(aCoord[2] - 1.0);
      aSlice[11]= 0.25*aCoord[0]*(aCoord[0] - 1.0)*(1.0 - aCoord[1]*aCoord[1])*aCoord[2]*(aCoord[2] - 1.0);
      aSlice[12]= 0.25*aCoord[0]*(aCoord[0] - 1.0)*aCoord[1]*(aCoord[1] - 1.0)*(1.0 - aCoord[2]*aCoord[2]);
      aSlice[13]= 0.25*aCoord[0]*(aCoord[0] + 1.0)*aCoord[1]*(aCoord[1] - 1.0)*(1.0 - aCoord[2]*aCoord[2]);
      aSlice[14]= 0.25*aCoord[0]*(aCoord[0] + 1.0)*aCoord[1]*(aCoord[1] + 1.0)*(1.0 - aCoord[2]*aCoord[2]);
      aSlice[15]= 0.25*aCoord[0]*(aCoord[0] - 1.0)*aCoord[1]*(aCoord[1] + 1.0)*(1.0 - aCoord[2]*aCoord[2]);
      aSlice[16]= 0.25*(1.0 - aCoord[0]*aCoord[0])*aCoord[1]*(aCoord[1] - 1.0)*aCoord[2]*(aCoord[2] + 1.0);
      aSlice[17]= 0.25*aCoord[0]*(aCoord[0] + 1.0)*(1.0 - aCoord[1]*aCoord[1])*aCoord[2]*(aCoord[2] + 1.0);
      aSlice[18]= 0.25*(1.0 - aCoord[0]*aCoord[0])*aCoord[1]*(aCoord[1] + 1.0)*aCoord[2]*(aCoord[2] + 1.0);
      aSlice[19]= 0.25*aCoord[0]*(aCoord[0] - 1.0)*(1.0 - aCoord[1]*aCoord[1])*aCoord[2]*(aCoord[2] + 1.0);
      aSlice[20]= 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1]*aCoord[1])*aCoord[2]*(aCoord[2] - 1.0);
      aSlice[21]= 0.25*(1.0 - aCoord[0]*aCoord[0])*aCoord[1]*(aCoord[1] - 1.0)*(1.0 - aCoord[2]*aCoord[2]);
      aSlice[22]= 0.25*aCoord[0]*(aCoord[0] + 1.0)*(1.0 - aCoord[1]*aCoord[1])*(1.0 - aCoord[2]*aCoord[2]);
      aSlice[23]= 0.25*(1.0 - aCoord[0]*aCoord[0])*aCoord[1]*(aCoord[1] + 1.0)*(1.0 - aCoord[2]*aCoord[2]);
      aSlice[24]= 0.25*aCoord[0]*(aCoord[0] - 1.0)*(1.0 - aCoord[1]*aCoord[1])*(1.0 - aCoord[2]*aCoord[2]);
      aSlice[25]= 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1]*aCoord[1])*aCoord[2]*(aCoord[2] + 1.0);
      aSlice[26]= 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1]*aCoord[1])*(1.0 - aCoord[1]*aCoord[1]);
    }
  }



  //---------------------------------------------------------------
  THexa8b::THexa8b():
    TShapeFun(3,8)
  {
    TInt aNbRef = GetNbRef();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case  4: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case  7: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case  6: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;
      case  5: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;
      }
    }
  }

  void
  THexa8b::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.125*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2]);
      aSlice[3] = 0.125*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2]);
      aSlice[2] = 0.125*(1.0 + aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2]);
      aSlice[1] = 0.125*(1.0 - aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2]);

      aSlice[4] = 0.125*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2]);
      aSlice[7] = 0.125*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2]);
      aSlice[6] = 0.125*(1.0 + aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2]);
      aSlice[5] = 0.125*(1.0 - aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2]);
    }
  }



  //---------------------------------------------------------------
  THexa20b::THexa20b(TInt theDim, TInt theNbRef):
    TShapeFun(theDim,theNbRef)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case  2: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case  4: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case  7: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case  6: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;
      case  5: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;

      case 11: aCoord[0] =  0.0;  aCoord[1] = -1.0;  aCoord[2] = -1.0; break;
      case 10: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] = -1.0; break;
      case  9: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] = -1.0; break;
      case  8: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] = -1.0; break;
      case 16: aCoord[0] = -1.0;  aCoord[1] = -1.0;  aCoord[2] =  0.0; break;
      case 19: aCoord[0] =  1.0;  aCoord[1] = -1.0;  aCoord[2] =  0.0; break;
      case 18: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case 17: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case 15: aCoord[0] =  0.0;  aCoord[1] = -1.0;  aCoord[2] =  1.0; break;
      case 14: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case 13: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  1.0; break;
      case 12: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      }
    }
  }

  void
  THexa20b::InitFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.125*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2])*
        (-2.0 - aCoord[0] - aCoord[1] - aCoord[2]);
      aSlice[3] = 0.125*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2])*
        (-2.0 + aCoord[0] - aCoord[1] - aCoord[2]);
      aSlice[2] = 0.125*(1.0 + aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2])*
        (-2.0 + aCoord[0] + aCoord[1] - aCoord[2]);
      aSlice[1] = 0.125*(1.0 - aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2])*
        (-2.0 - aCoord[0] + aCoord[1] - aCoord[2]);
      aSlice[4] = 0.125*(1.0 - aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2])*
        (-2.0 - aCoord[0] - aCoord[1] + aCoord[2]);
      aSlice[7] = 0.125*(1.0 + aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2])*
        (-2.0 + aCoord[0] - aCoord[1] + aCoord[2]);
      aSlice[6] = 0.125*(1.0 + aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2])*
        (-2.0 + aCoord[0] + aCoord[1] + aCoord[2]);
      aSlice[5] = 0.125*(1.0 - aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2])*
        (-2.0 - aCoord[0] + aCoord[1] + aCoord[2]);

      aSlice[11] = 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1])*(1.0 - aCoord[2]);
      aSlice[10] = 0.25*(1.0 - aCoord[1]*aCoord[1])*(1.0 + aCoord[0])*(1.0 - aCoord[2]);
      aSlice[9] = 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 + aCoord[1])*(1.0 - aCoord[2]);
      aSlice[8] = 0.25*(1.0 - aCoord[1]*aCoord[1])*(1.0 - aCoord[0])*(1.0 - aCoord[2]);
      aSlice[16] = 0.25*(1.0 - aCoord[2]*aCoord[2])*(1.0 - aCoord[0])*(1.0 - aCoord[1]);
      aSlice[19] = 0.25*(1.0 - aCoord[2]*aCoord[2])*(1.0 + aCoord[0])*(1.0 - aCoord[1]);
      aSlice[18] = 0.25*(1.0 - aCoord[2]*aCoord[2])*(1.0 + aCoord[0])*(1.0 + aCoord[1]);
      aSlice[17] = 0.25*(1.0 - aCoord[2]*aCoord[2])*(1.0 - aCoord[0])*(1.0 + aCoord[1]);
      aSlice[15] = 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 - aCoord[1])*(1.0 + aCoord[2]);
      aSlice[14] = 0.25*(1.0 - aCoord[1]*aCoord[1])*(1.0 + aCoord[0])*(1.0 + aCoord[2]);
      aSlice[13] = 0.25*(1.0 - aCoord[0]*aCoord[0])*(1.0 + aCoord[1])*(1.0 + aCoord[2]);
      aSlice[12] = 0.25*(1.0 - aCoord[1]*aCoord[1])*(1.0 - aCoord[0])*(1.0 + aCoord[2]);
    }
  }



  //---------------------------------------------------------------
  TPenta6a::TPenta6a():
    TShapeFun(3,6)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] = -0.0;  aCoord[2] =  1.0; break;
      case  2: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  4: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case  5: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      }
    }
  }

  void
  TPenta6a::InitFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*aCoord[1]*(1.0 - aCoord[0]);
      aSlice[1] = 0.5*aCoord[2]*(1.0 - aCoord[0]);
      aSlice[2] = 0.5*(1.0 - aCoord[1] - aCoord[2])*(1.0 - aCoord[0]);

      aSlice[3] = 0.5*aCoord[1]*(aCoord[0] + 1.0);
      aSlice[4] = 0.5*aCoord[2]*(aCoord[0] + 1.0);
      aSlice[5] = 0.5*(1.0 - aCoord[1] - aCoord[2])*(1.0 + aCoord[0]);
    }
  }



  //---------------------------------------------------------------
  TPenta6b::TPenta6b():
    TShapeFun(3,6)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  2: aCoord[0] = -1.0;  aCoord[1] = -0.0;  aCoord[2] =  1.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  5: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case  4: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      }
    }
  }

  void
  TPenta6b::InitFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*aCoord[1]*(1.0 - aCoord[0]);
      aSlice[2] = 0.5*aCoord[2]*(1.0 - aCoord[0]);
      aSlice[1] = 0.5*(1.0 - aCoord[1] - aCoord[2])*(1.0 - aCoord[0]);

      aSlice[3] = 0.5*aCoord[1]*(aCoord[0] + 1.0);
      aSlice[5] = 0.5*aCoord[2]*(aCoord[0] + 1.0);
      aSlice[4] = 0.5*(1.0 - aCoord[1] - aCoord[2])*(1.0 + aCoord[0]);
    }
  }



  //---------------------------------------------------------------
  TPenta15a::TPenta15a():
    TShapeFun(3,15)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] = -0.0;  aCoord[2] =  1.0; break;
      case  2: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  4: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case  5: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;

      case  6: aCoord[0] = -1.0;  aCoord[1] =  0.5;  aCoord[2] =  0.5; break;
      case  7: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case  8: aCoord[0] = -1.0;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      case  9: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case 10: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case 11: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case 12: aCoord[0] =  1.0;  aCoord[1] =  0.5;  aCoord[2] =  0.5; break;
      case 13: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case 14: aCoord[0] =  1.0;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      }
    }
  }

  void
  TPenta15a::InitFun(const TCCoordSliceArr& theRef,
                     const TCCoordSliceArr& theGauss,
                     TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*aCoord[1]*(1.0 - aCoord[0])*(2.0*aCoord[1] - 2.0 - aCoord[0]);
      aSlice[1] = 0.5*aCoord[2]*(1.0 - aCoord[0])*(2.0*aCoord[2] - 2.0 - aCoord[0]);
      aSlice[2] = 0.5*(aCoord[0] - 1.0)*(1.0 - aCoord[1] - aCoord[2])*(aCoord[0] + 2.0*aCoord[1] + 2.0*aCoord[2]);

      aSlice[3] = 0.5*aCoord[1]*(1.0 + aCoord[0])*(2.0*aCoord[1] - 2.0 + aCoord[0]);
      aSlice[4] = 0.5*aCoord[2]*(1.0 + aCoord[0])*(2.0*aCoord[2] - 2.0 + aCoord[0]);
      aSlice[5] = 0.5*(-aCoord[0] - 1.0)*(1.0 - aCoord[1] - aCoord[2])*(-aCoord[0] + 2.0*aCoord[1] + 2.0*aCoord[2]);

      aSlice[6] = 2.0*aCoord[1]*aCoord[2]*(1.0 - aCoord[0]);
      aSlice[7] = 2.0*aCoord[2]*(1.0 - aCoord[1] - aCoord[2])*(1.0 - aCoord[0]);
      aSlice[8] = 2.0*aCoord[1]*(1.0 - aCoord[1] - aCoord[2])*(1.0 - aCoord[0]);

      aSlice[9] = aCoord[1]*(1.0 - aCoord[0]*aCoord[0]);
      aSlice[10] = aCoord[2]*(1.0 - aCoord[0]*aCoord[0]);
      aSlice[11] = (1.0 - aCoord[1] - aCoord[2])*(1.0 - aCoord[0]*aCoord[0]);

      aSlice[12] = 2.0*aCoord[1]*aCoord[2]*(1.0 + aCoord[0]);
      aSlice[13] = 2.0*aCoord[2]*(1.0 - aCoord[1] - aCoord[2])*(1.0 + aCoord[0]);
      aSlice[14] = 2.0*aCoord[1]*(1.0 - aCoord[1] - aCoord[2])*(1.0 + aCoord[0]);
    }
  }



  //---------------------------------------------------------------
  TPenta15b::TPenta15b():
    TShapeFun(3,15)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] = -1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  2: aCoord[0] = -1.0;  aCoord[1] = -0.0;  aCoord[2] =  1.0; break;
      case  1: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  1.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  5: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case  4: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;

      case  8: aCoord[0] = -1.0;  aCoord[1] =  0.5;  aCoord[2] =  0.5; break;
      case  7: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case  6: aCoord[0] = -1.0;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      case 12: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case 14: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      case 13: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case 11: aCoord[0] =  1.0;  aCoord[1] =  0.5;  aCoord[2] =  0.5; break;
      case 10: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case  9: aCoord[0] =  1.0;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      }
    }
  }

  void
  TPenta15b::InitFun(const TCCoordSliceArr& theRef,
                     const TCCoordSliceArr& theGauss,
                     TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*aCoord[1]*(1.0 - aCoord[0])*(2.0*aCoord[1] - 2.0 - aCoord[0]);
      aSlice[2] = 0.5*aCoord[2]*(1.0 - aCoord[0])*(2.0*aCoord[2] - 2.0 - aCoord[0]);
      aSlice[1] = 0.5*(aCoord[0] - 1.0)*(1.0 - aCoord[1] - aCoord[2])*(aCoord[0] + 2.0*aCoord[1] + 2.0*aCoord[2]);

      aSlice[3] = 0.5*aCoord[1]*(1.0 + aCoord[0])*(2.0*aCoord[1] - 2.0 + aCoord[0]);
      aSlice[5] = 0.5*aCoord[2]*(1.0 + aCoord[0])*(2.0*aCoord[2] - 2.0 + aCoord[0]);
      aSlice[4] = 0.5*(-aCoord[0] - 1.0)*(1.0 - aCoord[1] - aCoord[2])*(-aCoord[0] + 2.0*aCoord[1] + 2.0*aCoord[2]);

      aSlice[8] = 2.0*aCoord[1]*aCoord[2]*(1.0 - aCoord[0]);
      aSlice[7] = 2.0*aCoord[2]*(1.0 - aCoord[1] - aCoord[2])*(1.0 - aCoord[0]);
      aSlice[6] = 2.0*aCoord[1]*(1.0 - aCoord[1] - aCoord[2])*(1.0 - aCoord[0]);

      aSlice[12] = aCoord[1]*(1.0 - aCoord[0]*aCoord[0]);
      aSlice[14] = aCoord[2]*(1.0 - aCoord[0]*aCoord[0]);
      aSlice[13] = (1.0 - aCoord[1] - aCoord[2])*(1.0 - aCoord[0]*aCoord[0]);

      aSlice[11] = 2.0*aCoord[1]*aCoord[2]*(1.0 + aCoord[0]);
      aSlice[10] = 2.0*aCoord[2]*(1.0 - aCoord[1] - aCoord[2])*(1.0 + aCoord[0]);
      aSlice[9]  = 2.0*aCoord[1]*(1.0 - aCoord[1] - aCoord[2])*(1.0 + aCoord[0]);
    }
  }



  //---------------------------------------------------------------
  TPyra5a::TPyra5a():
    TShapeFun(3,5)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  1: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  2: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  0.0;  aCoord[1] = -1.0;  aCoord[2] =  0.0; break;
      case  4: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      }
    }
  }

  void
  TPyra5a::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);
      // APO & RNV:
      // Fix for 0019920: EDF 788 VISU: Bad localisation of gauss points for pyramids
      // Seems shape function for ePYRA5 elements is:
      // w0 = 0.25*(-X + Y - 1.0)*(-X - Y - 1.0)*(1.0 - Z);
      // w1 = 0.25*(-X - Y - 1.0)*(+X - Y - 1.0)*(1.0 - Z);
      // w2 = 0.25*(+X + Y - 1.0)*(+X - Y - 1.0)*(1.0 - Z);
      // w3 = 0.25*(+X + Y - 1.0)*(-X + Y - 1.0)*(1.0 - Z);
      // w4 = +Z;
      aSlice[0] = 0.25*(-aCoord[0] + aCoord[1] - 1.0)*(-aCoord[0] - aCoord[1] - 1.0)*(1.0 - aCoord[2]);
      aSlice[1] = 0.25*(-aCoord[0] - aCoord[1] - 1.0)*(+aCoord[0] - aCoord[1] - 1.0)*(1.0 - aCoord[2]);
      aSlice[2] = 0.25*(+aCoord[0] + aCoord[1] - 1.0)*(+aCoord[0] - aCoord[1] - 1.0)*(1.0 - aCoord[2]);
      aSlice[3] = 0.25*(+aCoord[0] + aCoord[1] - 1.0)*(-aCoord[0] + aCoord[1] - 1.0)*(1.0 - aCoord[2]);
      aSlice[4] = aCoord[2];
    }
  }



  //---------------------------------------------------------------
  TPyra5b::TPyra5b():
    TShapeFun(3,5)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){        
      case  0: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  2: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  1: aCoord[0] =  0.0;  aCoord[1] = -1.0;  aCoord[2] =  0.0; break;
      case  4: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;
      }
    }
  }

  void
  TPyra5b::InitFun(const TCCoordSliceArr& theRef,
                   const TCCoordSliceArr& theGauss,
                   TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);
    
    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);
      // APO & RNV:
      // Fix for 0019920: EDF 788 VISU: Bad localisation of gauss points for pyramids
      // Seems shape function for ePYRA5 elements is:
      // w0 = 0.25*(-X + Y - 1.0)*(-X - Y - 1.0)*(1.0 - Z);
      // w1 = 0.25*(-X - Y - 1.0)*(+X - Y - 1.0)*(1.0 - Z);
      // w2 = 0.25*(+X + Y - 1.0)*(+X - Y - 1.0)*(1.0 - Z);
      // w3 = 0.25*(+X + Y - 1.0)*(-X + Y - 1.0)*(1.0 - Z);
      // w4 = +Z;
      aSlice[0] = 0.25*(-aCoord[0] + aCoord[1] - 1.0)*(-aCoord[0] - aCoord[1] - 1.0)*(1.0 - aCoord[2]);
      aSlice[3] = 0.25*(-aCoord[0] - aCoord[1] - 1.0)*(+aCoord[0] - aCoord[1] - 1.0)*(1.0 - aCoord[2]);
      aSlice[2] = 0.25*(+aCoord[0] + aCoord[1] - 1.0)*(+aCoord[0] - aCoord[1] - 1.0)*(1.0 - aCoord[2]);
      aSlice[1] = 0.25*(+aCoord[0] + aCoord[1] - 1.0)*(-aCoord[0] + aCoord[1] - 1.0)*(1.0 - aCoord[2]);
      aSlice[4] = aCoord[2];
    }
  }
  


  //---------------------------------------------------------------
  TPyra13a::TPyra13a():
    TShapeFun(3,13)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  1: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  2: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  0.0;  aCoord[1] = -1.0;  aCoord[2] =  0.0; break;
      case  4: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;

      case  5: aCoord[0] =  0.5;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      case  6: aCoord[0] = -0.5;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      case  7: aCoord[0] = -0.5;  aCoord[1] = -0.5;  aCoord[2] =  0.0; break;
      case  8: aCoord[0] =  0.5;  aCoord[1] = -0.5;  aCoord[2] =  0.0; break;
      case  9: aCoord[0] =  0.5;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case 10: aCoord[0] =  0.0;  aCoord[1] =  0.5;  aCoord[2] =  0.5; break;
      case 11: aCoord[0] = -0.5;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case 12: aCoord[0] =  0.0;  aCoord[1] = -0.5;  aCoord[2] =  0.5; break;
      }
    }
  }

  void
  TPyra13a::InitFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*
        (aCoord[0] - 0.5)/(1.0 - aCoord[2]);
      aSlice[1] = 0.5*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(+aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*
        (aCoord[1] - 0.5)/(1.0 - aCoord[2]);
      aSlice[2] = 0.5*(+aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(+aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*
        (-aCoord[0] - 0.5)/(1.0 - aCoord[2]);
      aSlice[3] = 0.5*(+aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*
        (-aCoord[1] - 0.5)/(1.0 - aCoord[2]);

      aSlice[4] = 2.0*aCoord[2]*(aCoord[2] - 0.5);

      aSlice[5] = 0.5*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*
        (aCoord[0] - aCoord[1] + aCoord[2] - 1.0)/(1.0 - aCoord[2]);
      aSlice[6] = 0.5*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*
        (aCoord[0] + aCoord[1] + aCoord[2] - 1.0)/(1.0 - aCoord[2]);
      aSlice[7] = 0.5*(aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*
        (-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)/(1.0 - aCoord[2]);
      aSlice[8] = 0.5*(aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*
        (-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)/(1.0 - aCoord[2]);

      aSlice[9] = 0.5*aCoord[2]*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)/
        (1.0 - aCoord[2]);
      aSlice[10] = 0.5*aCoord[2]*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(aCoord[0] - aCoord[1] + aCoord[2] - 1.0)/
        (1.0 - aCoord[2]);
      aSlice[11] = 0.5*aCoord[2]*(aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(aCoord[0] + aCoord[1] + aCoord[2] - 1.0)/
        (1.0 - aCoord[2]);
      aSlice[12] = 0.5*aCoord[2]*(aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)/
        (1.0 - aCoord[2]);
    }
  }



  //---------------------------------------------------------------
  TPyra13b::TPyra13b():
    TShapeFun(3,13)
  {
    TInt aNbRef = myRefCoord.size();
    for(TInt aRefId = 0; aRefId < aNbRef; aRefId++){
      TCoordSlice aCoord = GetCoord(aRefId);
      switch(aRefId){
      case  0: aCoord[0] =  1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  3: aCoord[0] =  0.0;  aCoord[1] =  1.0;  aCoord[2] =  0.0; break;
      case  2: aCoord[0] = -1.0;  aCoord[1] =  0.0;  aCoord[2] =  0.0; break;
      case  1: aCoord[0] =  0.0;  aCoord[1] = -1.0;  aCoord[2] =  0.0; break;
      case  4: aCoord[0] =  0.0;  aCoord[1] =  0.0;  aCoord[2] =  1.0; break;

      case  8: aCoord[0] =  0.5;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      case  7: aCoord[0] = -0.5;  aCoord[1] =  0.5;  aCoord[2] =  0.0; break;
      case  6: aCoord[0] = -0.5;  aCoord[1] = -0.5;  aCoord[2] =  0.0; break;
      case  5: aCoord[0] =  0.5;  aCoord[1] = -0.5;  aCoord[2] =  0.0; break;
      case  9: aCoord[0] =  0.5;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case 12: aCoord[0] =  0.0;  aCoord[1] =  0.5;  aCoord[2] =  0.5; break;
      case 11: aCoord[0] = -0.5;  aCoord[1] =  0.0;  aCoord[2] =  0.5; break;
      case 10: aCoord[0] =  0.0;  aCoord[1] = -0.5;  aCoord[2] =  0.5; break;
      }
    }
  }

  void
  TPyra13b::InitFun(const TCCoordSliceArr& theRef,
                    const TCCoordSliceArr& theGauss,
                    TFun& theFun) const
  {
    GetFun(theRef,theGauss,theFun);

    TInt aNbGauss = theGauss.size();
    for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
      const TCCoordSlice& aCoord = theGauss[aGaussId];
      TFloatVecSlice aSlice = theFun.GetFunSlice(aGaussId);

      aSlice[0] = 0.5*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*
        (aCoord[0] - 0.5)/(1.0 - aCoord[2]);
      aSlice[3] = 0.5*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(+aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*
        (aCoord[1] - 0.5)/(1.0 - aCoord[2]);
      aSlice[2] = 0.5*(+aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(+aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*
        (-aCoord[0] - 0.5)/(1.0 - aCoord[2]);
      aSlice[1] = 0.5*(+aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*
        (-aCoord[1] - 0.5)/(1.0 - aCoord[2]);

      aSlice[4] = 2.0*aCoord[2]*(aCoord[2] - 0.5);

      aSlice[8] = 0.5*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*
        (aCoord[0] - aCoord[1] + aCoord[2] - 1.0)/(1.0 - aCoord[2]);
      aSlice[7] = 0.5*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*
        (aCoord[0] + aCoord[1] + aCoord[2] - 1.0)/(1.0 - aCoord[2]);
      aSlice[6] = 0.5*(aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*
        (-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)/(1.0 - aCoord[2]);
      aSlice[5] = 0.5*(aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*
        (-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)/(1.0 - aCoord[2]);

      aSlice[9] = 0.5*aCoord[2]*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)/
        (1.0 - aCoord[2]);
      aSlice[12] = 0.5*aCoord[2]*(-aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(aCoord[0] - aCoord[1] + aCoord[2] - 1.0)/
        (1.0 - aCoord[2]);
      aSlice[11] = 0.5*aCoord[2]*(aCoord[0] - aCoord[1] + aCoord[2] - 1.0)*(aCoord[0] + aCoord[1] + aCoord[2] - 1.0)/
        (1.0 - aCoord[2]);
      aSlice[10] = 0.5*aCoord[2]*(aCoord[0] + aCoord[1] + aCoord[2] - 1.0)*(-aCoord[0] + aCoord[1] + aCoord[2] - 1.0)/
        (1.0 - aCoord[2]);
    }
  }



  //---------------------------------------------------------------
  bool
  GetGaussCoord3D(const TGaussInfo& theGaussInfo, 
                  const TCellInfo& theCellInfo,
                  const TNodeInfo& theNodeInfo,
                  TGaussCoord& theGaussCoord,
                  const TElemNum& theElemNum,
                  EModeSwitch theMode)
  {
    INITMSG(MYDEBUG,"GetGaussCoord3D\n");

    if(theGaussInfo.myGeom == theCellInfo.myGeom){
      EGeometrieElement aGeom = theGaussInfo.myGeom;

      TInt aNbRef = theGaussInfo.GetNbRef();
      TCCoordSliceArr aRefSlice(aNbRef);
      for(TInt anId = 0; anId < aNbRef; anId++)
        aRefSlice[anId] = theGaussInfo.GetRefCoordSlice(anId);

      TInt aNbGauss = theGaussInfo.GetNbGauss();
      TCCoordSliceArr aGaussSlice(aNbGauss);
      for(TInt anId = 0; anId < aNbGauss; anId++)
        aGaussSlice[anId] = theGaussInfo.GetGaussCoordSlice(anId);

      switch(aGeom){
      case eSEG2: {
        INITMSG(MYDEBUG,"eSEG2"<<std::endl);

        if(TSeg2a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eSEG3: {
        INITMSG(MYDEBUG,"eSEG3"<<std::endl);

        if(TSeg3a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eTRIA3: {
        INITMSG(MYDEBUG,"eTRIA3"<<std::endl);

        if(TTria3a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TTria3b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eTRIA6: {
        INITMSG(MYDEBUG,"eTRIA6"<<std::endl);

        if(TTria6a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TTria6b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eQUAD4: {
        INITMSG(MYDEBUG,"eQUAD4"<<std::endl);

        if(TQuad4a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TQuad4b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eQUAD8: {
        INITMSG(MYDEBUG,"eQUAD8"<<std::endl);

        if(TQuad8a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TQuad8b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eQUAD9: {
        INITMSG(MYDEBUG,"eQUAD9"<<std::endl);

        if(TQuad9a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TQuad9b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eTETRA4: {
        INITMSG(MYDEBUG,"eTETRA4"<<std::endl);

        if(TTetra4a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TTetra4b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case ePYRA5: {
        INITMSG(MYDEBUG,"ePYRA5"<<std::endl);

        if(TPyra5a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TPyra5b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case ePENTA6: {
        INITMSG(MYDEBUG,"ePENTA6"<<std::endl);

        if(TPenta6a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TPenta6b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eHEXA8: {
        INITMSG(MYDEBUG,"eHEXA8"<<std::endl);

        if(THexa8a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(THexa8b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eTETRA10: {
        INITMSG(MYDEBUG,"eTETRA10"<<std::endl);

        if(TTetra10a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TTetra10b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case ePYRA13: {
        INITMSG(MYDEBUG,"ePYRA13"<<std::endl);

        if(TPyra13a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TPyra13b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case ePENTA15: {
        INITMSG(MYDEBUG,"ePENTA15"<<std::endl);

        if(TPenta15a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(TPenta15b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      case eHEXA20: {
        INITMSG(MYDEBUG,"eHEXA20"<<std::endl);

        if(THexa20a().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        if(THexa20b().Eval(theCellInfo,theNodeInfo,theElemNum,aRefSlice,aGaussSlice,theGaussCoord,theMode))
          return true;

        break;
      }
      default: 
        INITMSG(MYDEBUG,"eNONE"<<std::endl);
        return false;
      }
    }

    return false;
  }

  //---------------------------------------------------------------
  bool
  GetBaryCenter(const TCellInfo& theCellInfo,
                const TNodeInfo& theNodeInfo,
                TGaussCoord& theGaussCoord,
                const TElemNum& theElemNum,
                EModeSwitch theMode)
  {
    INITMSG(MYDEBUG,"GetBaryCenter\n");
    const PMeshInfo& aMeshInfo = theCellInfo.GetMeshInfo();
    TInt aDim = aMeshInfo->GetDim();
    static TInt aNbGauss = 1;

    bool anIsSubMesh = !theElemNum.empty();
    TInt aNbElem;
    if(anIsSubMesh)
      aNbElem = theElemNum.size();
    else
      aNbElem = theCellInfo.GetNbElem();

    theGaussCoord.Init(aNbElem,aNbGauss,aDim,theMode);

    TInt aConnDim = theCellInfo.GetConnDim();

    INITMSGA(MYDEBUG,0,
             "- aDim = "<<aDim<<
             "; aNbGauss = "<<aNbGauss<<
             "; aNbElem = "<<aNbElem<<
             "; aNbNodes = "<<theNodeInfo.GetNbElem()<<
             std::endl);

    for(TInt anElemId = 0; anElemId < aNbElem; anElemId++){
      TInt aCellId = anIsSubMesh? theElemNum[anElemId]-1: anElemId;
      TCConnSlice aConnSlice = theCellInfo.GetConnSlice(aCellId);
      TCoordSliceArr aCoordSliceArr = theGaussCoord.GetCoordSliceArr(anElemId);

      for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
        TCoordSlice& aGaussCoordSlice = aCoordSliceArr[aGaussId];

        for(TInt aConnId = 0; aConnId < aConnDim; aConnId++){
          TInt aNodeId = aConnSlice[aConnId] - 1;      
          TCCoordSlice aNodeCoordSlice = theNodeInfo.GetCoordSlice(aNodeId);

          for(TInt aDimId = 0; aDimId < aDim; aDimId++){
            aGaussCoordSlice[aDimId] += aNodeCoordSlice[aDimId];
          }
        }

        for(TInt aDimId = 0; aDimId < aDim; aDimId++){
          aGaussCoordSlice[aDimId] /= aConnDim;
        }
      }
    }

#ifdef _DEBUG_
    for(TInt anElemId = 0; anElemId < aNbElem; anElemId++){
      TCoordSliceArr aCoordSliceArr = theGaussCoord.GetCoordSliceArr(anElemId);
      INITMSG(MYVALUEDEBUG,"");
      for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
        TCoordSlice& aCoordSlice = aCoordSliceArr[aGaussId];
        ADDMSG(MYVALUEDEBUG,"{");
        for(TInt aDimId = 0; aDimId < aDim; aDimId++){
          ADDMSG(MYVALUEDEBUG,aCoordSlice[aDimId]<<" ");
        }
        ADDMSG(MYVALUEDEBUG,"} ");
      }
      ADDMSG(MYVALUEDEBUG,std::endl);
    }
#endif

    return true;
  }


  //---------------------------------------------------------------
  bool
  GetBaryCenter(const TPolygoneInfo& thePolygoneInfo,
                const TNodeInfo& theNodeInfo,
                TGaussCoord& theGaussCoord,
                const TElemNum& theElemNum,
                EModeSwitch theMode)
  {
    INITMSG(MYDEBUG,"GetBaryCenter\n");
    const PMeshInfo& aMeshInfo = thePolygoneInfo.GetMeshInfo();
    TInt aDim = aMeshInfo->GetDim();
    static TInt aNbGauss = 1;

    bool anIsSubMesh = !theElemNum.empty();
    TInt aNbElem;
    if(anIsSubMesh)
      aNbElem = theElemNum.size();
    else
      aNbElem = thePolygoneInfo.GetNbElem();

    theGaussCoord.Init(aNbElem,aNbGauss,aDim,theMode);

    INITMSGA(MYDEBUG,0,
             "- aDim = "<<aDim<<
             "; aNbGauss = "<<aNbGauss<<
             "; aNbElem = "<<aNbElem<<
             "; aNbNodes = "<<theNodeInfo.GetNbElem()<<
             std::endl);

    for(TInt anElemId = 0; anElemId < aNbElem; anElemId++){
      TInt aCellId = anIsSubMesh? theElemNum[anElemId]-1: anElemId;

      TCoordSliceArr aCoordSliceArr = theGaussCoord.GetCoordSliceArr(anElemId);
      TCConnSlice aConnSlice = thePolygoneInfo.GetConnSlice(aCellId);
      TInt aNbConn = thePolygoneInfo.GetNbConn(aCellId);
      TInt aNbNodes = thePolygoneInfo.GetNbConn(aCellId);

      for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
        TCoordSlice& aGaussCoordSlice = aCoordSliceArr[aGaussId];

        for(TInt aConnId = 0; aConnId < aNbConn; aConnId++){
          TInt aNodeId = aConnSlice[aConnId] - 1;      
          TCCoordSlice aNodeCoordSlice = theNodeInfo.GetCoordSlice(aNodeId);

          for(TInt aDimId = 0; aDimId < aDim; aDimId++){
            aGaussCoordSlice[aDimId] += aNodeCoordSlice[aDimId];
          }
        }

        for(TInt aDimId = 0; aDimId < aDim; aDimId++){
          aGaussCoordSlice[aDimId] /= aNbNodes;
        }
      }
    }

    return true;
  }


  //---------------------------------------------------------------
  bool
  GetBaryCenter(const TPolyedreInfo& thePolyedreInfo,
                const TNodeInfo& theNodeInfo,
                TGaussCoord& theGaussCoord,
                const TElemNum& theElemNum,
                EModeSwitch theMode)
  {
    INITMSG(MYDEBUG,"GetBaryCenter\n");
    const PMeshInfo& aMeshInfo = thePolyedreInfo.GetMeshInfo();
    TInt aDim = aMeshInfo->GetDim();
    static TInt aNbGauss = 1;

    bool anIsSubMesh = !theElemNum.empty();
    TInt aNbElem;
    if(anIsSubMesh)
      aNbElem = theElemNum.size();
    else
      aNbElem = thePolyedreInfo.GetNbElem();

    theGaussCoord.Init(aNbElem,aNbGauss,aDim,theMode);

    INITMSGA(MYDEBUG,0,
             "- aDim = "<<aDim<<
             "; aNbGauss = "<<aNbGauss<<
             "; aNbElem = "<<aNbElem<<
             "; aNbNodes = "<<theNodeInfo.GetNbElem()<<
             std::endl);

    for(TInt anElemId = 0; anElemId < aNbElem; anElemId++){
      TInt aCellId = anIsSubMesh? theElemNum[anElemId]-1: anElemId;

      TCoordSliceArr aCoordSliceArr = theGaussCoord.GetCoordSliceArr(anElemId);
      TCConnSliceArr aConnSliceArr = thePolyedreInfo.GetConnSliceArr(aCellId);
      TInt aNbFaces = aConnSliceArr.size();

      TInt aNbNodes = thePolyedreInfo.GetNbNodes(aCellId);

      for(TInt aGaussId = 0; aGaussId < aNbGauss; aGaussId++){
        TCoordSlice& aGaussCoordSlice = aCoordSliceArr[aGaussId];

        for(TInt aFaceId = 0; aFaceId < aNbFaces; aFaceId++){
          TCConnSlice aConnSlice = aConnSliceArr[aFaceId];
          TInt aNbConn = aConnSlice.size();
          for(TInt aConnId = 0; aConnId < aNbConn; aConnId++){
            TInt aNodeId = aConnSlice[aConnId] - 1;      
            TCCoordSlice aNodeCoordSlice = theNodeInfo.GetCoordSlice(aNodeId);

            for(TInt aDimId = 0; aDimId < aDim; aDimId++){
              aGaussCoordSlice[aDimId] += aNodeCoordSlice[aDimId];
            }
          }
        }
        for(TInt aDimId = 0; aDimId < aDim; aDimId++){
          aGaussCoordSlice[aDimId] /= aNbNodes;
        }
      }
    }

    return true;
  }
}
