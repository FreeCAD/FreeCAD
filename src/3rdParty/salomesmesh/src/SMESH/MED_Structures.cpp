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
//  File   : MED_Structure.cxx
//  Author : Eugeny NIKOLAEV
//
#include "MED_Structures.hxx"
#include "MED_Utilities.hxx"

#include <cstring>

using namespace MED;

namespace MED
{
  TInt
  GetNbNodes(EGeometrieElement typmai)
  {
    return typmai%100;
  }

  std::string 
  GetString(TInt theId, 
            TInt theStep, 
            const TString& theString)
  {
    const char* aPos = &theString[theId*theStep];
    TInt aSize = std::min(TInt(strlen(aPos)),theStep);
    return std::string(aPos,aSize);
  }

  void 
  SetString(TInt theId, 
            TInt theStep, 
            TString& theString, 
            const std::string& theValue)
  {
    TInt aSize = std::min(TInt(theValue.size()+1),theStep);
    char* aPos = &theString[theId*theStep];
    strncpy(aPos,theValue.c_str(),aSize);
  }

  void 
  SetString(TInt theId, 
            TInt theStep, 
            TString& theString, 
            const TString& theValue)
  {
    TInt aSize = std::min(TInt(theValue.size()+1),theStep);
    char* aPos = &theString[theId*theStep];
    const char* aValue = &theValue[0];
    strncpy(aPos,aValue,aSize);
  }

  TInt
  GetDimGaussCoord(EGeometrieElement theGeom)
  {
    return theGeom/100;
  }

  TInt
  GetNbRefCoord(EGeometrieElement theGeom)
  {
    return (theGeom%100);
  }

  //---------------------------------------------------------------
  PFloatTimeStampValue
  CastToFloatTimeStampValue(const PTimeStampValueBase& theTimeStampValue)
  {
    return theTimeStampValue;
  }

  PIntTimeStampValue
  CastToIntTimeStampValue(const PTimeStampValueBase& theTimeStampValue)
  {
    return theTimeStampValue;
  }
}

//---------------------------------------------------------------
TInt
TFamilyInfo
::GetAttrId(TInt theId) const 
{
  return myAttrId[theId];
}

TInt
TFamilyInfo
::GetAttrVal(TInt theId) const 
{
  return myAttrVal[theId];
}

void
TFamilyInfo
::SetAttrId(TInt theId,TInt theVal) 
{
  myAttrId[theId] = theVal;
}

void
TFamilyInfo
::SetAttrVal(TInt theId,TInt theVal) 
{
  myAttrVal[theId] = theVal;
}

//---------------------------------------------------------------
TInt
TElemInfo
::GetFamNum(TInt theId) const 
{
  return (*myFamNum)[theId];
}

void
TElemInfo
::SetFamNum(TInt theId, TInt theVal) 
{
  (*myFamNum)[theId] = theVal;
  myIsFamNum = eVRAI;
}

TInt
TElemInfo
::GetElemNum(TInt theId) const 
{
  return (*myElemNum)[theId];
}

void
TElemInfo
::SetElemNum(TInt theId, TInt theVal) 
{
  (*myElemNum)[theId] = theVal;
}

//---------------------------------------------------------------
TCCoordSlice 
TNodeInfo
::GetCoordSlice(TInt theId) const
{
  TInt aDim = myMeshInfo->GetSpaceDim();
  if(GetModeSwitch() == eFULL_INTERLACE)
    return TCCoordSlice(*myCoord, std::slice(theId*aDim, aDim, 1));
  else
    return TCCoordSlice(*myCoord, std::slice(theId, aDim, aDim));
}

TCoordSlice 
TNodeInfo
::GetCoordSlice(TInt theId)
{
  TInt aDim = myMeshInfo->GetSpaceDim();
  if(GetModeSwitch() == eFULL_INTERLACE)
    return TCoordSlice(*myCoord, std::slice(theId*aDim,aDim,1));
  else
    return TCoordSlice(*myCoord, std::slice(theId,aDim,aDim));
}

//---------------------------------------------------------------
TCConnSlice 
TCellInfo
::GetConnSlice(TInt theElemId) const
{
  if(GetModeSwitch() == eFULL_INTERLACE)
    return TCConnSlice(*myConn, std::slice(GetConnDim()*theElemId, GetNbNodes(myGeom), 1));
  else
    return TCConnSlice(*myConn, std::slice(theElemId, GetNbNodes(myGeom), GetConnDim()));
}

TConnSlice 
TCellInfo
::GetConnSlice(TInt theElemId)
{
  if(GetModeSwitch() == eFULL_INTERLACE)
    return TConnSlice(*myConn, std::slice(GetConnDim()*theElemId, GetNbNodes(myGeom), 1));
  else
    return TConnSlice(*myConn, std::slice(theElemId, GetNbNodes(myGeom), GetConnDim()));
}


//---------------------------------------------------------------
TInt
TPolygoneInfo
::GetNbConn(TInt theElemId) const 
{
  return (*myIndex)[theElemId + 1] - (*myIndex)[theElemId];
}

TCConnSlice 
TPolygoneInfo
::GetConnSlice(TInt theElemId) const
{
  return TCConnSlice(*myConn, std::slice((*myIndex)[theElemId] - 1, GetNbConn(theElemId), 1));
}

TConnSlice 
TPolygoneInfo
::GetConnSlice(TInt theElemId)
{
  return TConnSlice(*myConn, std::slice((*myIndex)[theElemId] - 1, GetNbConn(theElemId), 1));
}


//---------------------------------------------------------------
TInt 
TPolyedreInfo
::GetNbFaces(TInt theElemId) const 
{
  return (*myIndex)[theElemId+1] - (*myIndex)[theElemId];
}

TInt 
TPolyedreInfo
::GetNbNodes(TInt theElemId) const 
{
  TInt aNbNodes = 0;
  TInt aNbFaces = GetNbFaces(theElemId);
  TInt aStartFaceId = (*myIndex)[theElemId] - 1;
  for(TInt aFaceId = 0; aFaceId < aNbFaces; aFaceId++, aStartFaceId++){
    TInt aCurrentId = (*myFaces)[aStartFaceId];
    TInt aDiff = (*myFaces)[aStartFaceId + 1] - aCurrentId;
    aNbNodes += aDiff;
  }
  return aNbNodes;
}

TCConnSliceArr 
TPolyedreInfo
::GetConnSliceArr(TInt theElemId) const
{
  TInt aNbFaces = GetNbFaces(theElemId);
  TCConnSliceArr aConnSliceArr(aNbFaces);
  TInt aStartFaceId = (*myIndex)[theElemId] - 1;
  for(TInt aFaceId = 0; aFaceId < aNbFaces; aFaceId++, aStartFaceId++){
    TInt aCurrentId = (*myFaces)[aStartFaceId];
    TInt aDiff = (*myFaces)[aStartFaceId + 1] - aCurrentId;
    aConnSliceArr[aFaceId] =
      TCConnSlice(*myConn, std::slice(aCurrentId - 1, aDiff, 1));
  }
  return aConnSliceArr;
}

TConnSliceArr 
TPolyedreInfo
::GetConnSliceArr(TInt theElemId)
{
  TInt aNbFaces = GetNbFaces(theElemId);
  TConnSliceArr aConnSliceArr(aNbFaces);
  TInt aStartFaceId = (*myIndex)[theElemId] - 1;
  for(TInt aFaceId = 0; aFaceId < aNbFaces; aFaceId++, aStartFaceId++){
    TInt aCurrentId = (*myFaces)[aStartFaceId];
    TInt aDiff = (*myFaces)[aStartFaceId + 1] - aCurrentId;
    aConnSliceArr[aFaceId] =
      TConnSlice(*myConn, std::slice(aCurrentId - 1, aDiff, 1));
  }
  return aConnSliceArr;
}


//---------------------------------------------------------------
TMeshValueBase
::TMeshValueBase():
  myNbElem(0),
  myNbComp(0),
  myNbGauss(0),
  myStep(0)
{}

void
TMeshValueBase
::Allocate(TInt theNbElem,
           TInt theNbGauss,
           TInt theNbComp,
           EModeSwitch theMode)
{
  myModeSwitch = theMode;
  
  myNbElem = theNbElem;
  myNbGauss = theNbGauss;
  myNbComp = theNbComp;
  
  myStep = theNbComp*theNbGauss;
}

size_t
TMeshValueBase
::GetSize() const
{
  return myNbElem * myStep;
}
    
size_t
TMeshValueBase
::GetNbVal() const
{
  return myNbElem * myNbGauss;
}

size_t
TMeshValueBase
::GetNbGauss() const
{
  return myNbGauss;
}

size_t
TMeshValueBase
::GetStep() const
{
  return myStep;
}


//---------------------------------------------------------------
TInt
TProfileInfo
::GetElemNum(TInt theId) const 
{
  return (*myElemNum)[theId];
}

void
TProfileInfo
::SetElemNum(TInt theId,TInt theVal) 
{
  (*myElemNum)[theId] = theVal;
}

//---------------------------------------------------------------
bool
TGaussInfo::TLess
::operator()(const TKey& theLeft, const TKey& theRight) const
{
  EGeometrieElement aLGeom = boost::get<0>(theLeft);
  EGeometrieElement aRGeom = boost::get<0>(theRight);
  if(aLGeom != aRGeom)
    return aLGeom < aRGeom;

  const std::string& aLStr = boost::get<1>(theLeft);
  const std::string& aRStr = boost::get<1>(theRight);
  return aLStr < aRStr;
}

bool
TGaussInfo::TLess
::operator()(const TGaussInfo& theLeft, const TGaussInfo& theRight) const
{
  //clang: warning: reference cannot be bound to dereferenced null pointer in well-defined C++ code;
  //       pointer may be assumed to always convert to true [-Wundefined-bool-conversion]
  //if(!&theLeft)
  //  return true;

  //if(!&theRight)
  //  return false;

  if(theLeft.myGeom != theRight.myGeom)
    return theLeft.myGeom < theRight.myGeom;

  if(theLeft.myRefCoord != theRight.myRefCoord)
    return theLeft.myRefCoord < theRight.myRefCoord;

  return theLeft.myGaussCoord < theRight.myGaussCoord;
}

TCCoordSlice 
TGaussInfo
::GetRefCoordSlice(TInt theId) const
{
  if(GetModeSwitch() == eFULL_INTERLACE)
    return TCCoordSlice(myRefCoord,std::slice(theId*GetDim(),GetDim(),1));
  else
    return TCCoordSlice(myRefCoord,std::slice(theId,GetDim(),GetDim()));
}

TCoordSlice 
TGaussInfo
::GetRefCoordSlice(TInt theId)
{
  if(GetModeSwitch() == eFULL_INTERLACE)
    return TCoordSlice(myRefCoord,std::slice(theId*GetDim(),GetDim(),1));
  else
    return TCoordSlice(myRefCoord,std::slice(theId,GetDim(),GetDim()));
}

TCCoordSlice 
TGaussInfo
::GetGaussCoordSlice(TInt theId) const
{
  if(GetModeSwitch() == eFULL_INTERLACE)
    return TCCoordSlice(myGaussCoord,std::slice(theId*GetDim(),GetDim(),1));
  else
    return TCCoordSlice(myGaussCoord,std::slice(theId,GetDim(),GetDim()));
}

TCoordSlice 
TGaussInfo
::GetGaussCoordSlice(TInt theId)
{
  if(GetModeSwitch() == eFULL_INTERLACE)
    return TCoordSlice(myGaussCoord,std::slice(theId*GetDim(),GetNbGauss(),1));
  else
    return TCoordSlice(myGaussCoord,std::slice(theId,GetNbGauss(),GetDim()));
}


//---------------------------------------------------------------
TInt
TTimeStampInfo
::GetNbGauss(EGeometrieElement theGeom) const
{
  TGeom2NbGauss::const_iterator anIter = myGeom2NbGauss.find(theGeom);
  if(anIter == myGeom2NbGauss.end())
    return 1;//EXCEPTION(runtime_error,"TTimeStampInfo::GetNbGauss - myGeom2NbGauss.find(theGeom) fails");

  return anIter->second;
}


//---------------------------------------------------------------
// TGrilleInfo structure methods
//---------------------------------------------------------------
const EGrilleType&
TGrilleInfo
::GetGrilleType() const
{
  return myGrilleType;
}

EGrilleType
TGrilleInfo
::GetGrilleType()
{
  return myGrilleType;
}

void
TGrilleInfo
::SetGrilleType(EGrilleType theGrilleType)
{
  myGrilleType = theGrilleType;
}

const
TIndexes&
TGrilleInfo
::GetMapOfIndexes() const
{
  return myIndixes;
}

TIndexes&
TGrilleInfo
::GetMapOfIndexes()
{
  return myIndixes;
}

const
TFloatVector&
TGrilleInfo
::GetIndexes(TInt theAxisNumber) const
{
  TIndexes::const_iterator aIter=myIndixes.find(theAxisNumber);
  if(aIter==myIndixes.end())
    EXCEPTION(std::runtime_error, "const TGrilleInfo::GetIndexes - myIndixes.find(theAxisNumber); fails");
  return aIter->second;
}

TFloatVector&
TGrilleInfo
::GetIndexes(TInt theAxisNumber)
{
  TIndexes::iterator aIter=myIndixes.find(theAxisNumber);
  if(aIter==myIndixes.end())
    EXCEPTION(std::runtime_error, "TGrilleInfo::GetIndexes - myIndixes.find(theAxisNumber="<<theAxisNumber<<"); fails");
  return aIter->second;
}

TInt
TGrilleInfo
::GetNbIndexes(TInt theAxisNumber)
{
  const TFloatVector& aVector=GetIndexes(theAxisNumber);
  return aVector.size();
}

TInt
TGrilleInfo
::GetNbNodes()
{
  TInt nbNodes=0;
  TInt aDim = myMeshInfo->GetDim();
  for(int i=0;i<aDim;i++)
    if(nbNodes == 0)
      nbNodes = this->GetGrilleStructure()[i];
    else
      nbNodes = nbNodes*this->GetGrilleStructure()[i];
 
  return nbNodes;
}

TInt
TGrilleInfo
::GetNbCells()
{
  TInt nbCells=0;
  TInt aDim = myMeshInfo->GetDim();
  for(int i=0;i<aDim;i++)
    if(nbCells == 0)
      nbCells = this->GetGrilleStructure()[i]-1;
    else
      nbCells = nbCells*(this->GetGrilleStructure()[i]-1);
  return nbCells;
}

TInt
TGrilleInfo
::GetNbSubCells()
{
  TInt nb=0;
  TInt aDim = myMeshInfo->GetDim();
  switch (aDim) {
  case 3:
    nb =
      (myGrilleStructure[0]  ) * (myGrilleStructure[1]-1) * (myGrilleStructure[2]-1) +
      (myGrilleStructure[0]-1) * (myGrilleStructure[1]  ) * (myGrilleStructure[2]-1) +
      (myGrilleStructure[0]-1) * (myGrilleStructure[1]-1) * (myGrilleStructure[2]  );
    break;
  case 2:
    nb =
      (myGrilleStructure[0]  ) * (myGrilleStructure[1]-1) +
      (myGrilleStructure[0]-1) * (myGrilleStructure[1]  );
    break;
  }
  return nb;
}

EGeometrieElement
TGrilleInfo
::GetGeom()
{
  TInt aDim = myMeshInfo->GetDim();
  switch(aDim){
  case 1:
    return eSEG2;
  case 2:
    return eQUAD4;
  case 3:
    return eHEXA8;
  default:
    return eNONE;
  }
}

EGeometrieElement
TGrilleInfo
::GetSubGeom()
{
  TInt aDim = myMeshInfo->GetDim();
  switch(aDim){
  case 2:
    return eSEG2;
  case 3:
    return eQUAD4;
  }
  return eNONE;
}

EEntiteMaillage
TGrilleInfo
::GetEntity()
{
  return eMAILLE;
}

EEntiteMaillage
TGrilleInfo
::GetSubEntity()
{
  TInt aDim = myMeshInfo->GetDim();
  switch(aDim){
  case 2:
    return eARETE;
  case 3:
    return eFACE;
  }
  return EEntiteMaillage(-1);
}

const
TIntVector&
TGrilleInfo
::GetGrilleStructure() const
{
  return myGrilleStructure;
}

TIntVector
TGrilleInfo
::GetGrilleStructure()
{
  return myGrilleStructure;
}

void
TGrilleInfo
::SetGrilleStructure(TInt theAxis,TInt theNb)
{
  if(theAxis >= 0 && theAxis <=2 && theNb >= 0)
  myGrilleStructure[theAxis]=theNb;
}

const
TNodeCoord&
TGrilleInfo
::GetNodeCoord() const
{
  return myCoord;
}

TNodeCoord&
TGrilleInfo
::GetNodeCoord()
{
  return myCoord;
}

TNodeCoord
TGrilleInfo
::GetCoord(TInt theId)
{
  TNodeCoord aCoord;
  TInt aDim       = myMeshInfo->GetDim();
  TInt aNbNodes   = this->GetNbNodes();
  aCoord.resize(aDim);
  
  if(theId >= aNbNodes)
    EXCEPTION(std::runtime_error, "TGrilleInfo::GetCoord - theId out of range");

  if(myGrilleType == eGRILLE_STANDARD){
    switch(aDim){
    case 3:
      aCoord[2] = myCoord[aDim*theId+2];
      /* FALLTHRU */
    case 2:
      aCoord[1] = myCoord[aDim*theId+1];
      /* FALLTHRU */
    case 1:{
      aCoord[0] = myCoord[aDim*theId];
      break;
    }
    }
  } else {

    TFloatVector aVecX  = this->GetIndexes(0);
    TInt nbIndxX        = this->GetNbIndexes(0);
    
    switch(aDim){
    case 1:{
      aCoord[0] = aVecX[theId];
      break;
    }
    case 2:{
      TFloatVector aVecY = this->GetIndexes(1);
      TInt i,j,k;
      i = j = k = 0;
      i = theId % nbIndxX;
      j = theId / nbIndxX;
      if(myGrilleType == eGRILLE_CARTESIENNE){
        aCoord[0] = aVecX[i];
        aCoord[1] = aVecY[j];
      } else { // eGRILLE_POLAIRE (cylindrical)
        aCoord[0] = aVecX[i] * cos(aVecY[j]);
        aCoord[1] = aVecX[i] * sin(aVecY[j]);
      }
      break;
    }
    case 3:{
      TFloatVector aVecY = this->GetIndexes(1);
      TInt nbIndxY       = this->GetNbIndexes(1);
      TFloatVector aVecZ = this->GetIndexes(2);
      TInt i,j,k;
      i = j = k = 0;
      
      i = theId % nbIndxX;
      j = (theId / nbIndxX) % nbIndxY;
      k = theId / (nbIndxX*nbIndxY);

      if(myGrilleType == eGRILLE_CARTESIENNE){
        aCoord[0] = aVecX[i];
        aCoord[1] = aVecY[j];
        aCoord[2] = aVecZ[k];
      } else { // eGRILLE_POLAIRE (cylindrical)
        aCoord[0] = aVecX[i] * cos(aVecY[j]);
        aCoord[1] = aVecX[i] * sin(aVecY[j]);
        aCoord[2] = aVecZ[k];
      }
      
      break;
    }
    }
  }

  return aCoord;
}

TIntVector
TGrilleInfo
::GetConn(TInt theId, const bool isSub)
{
  TIntVector anIndexes;
  TInt aDim = myMeshInfo->GetDim();

  TInt idx;
  TInt iMin, jMin, kMin, iMax, jMax, kMax;
  TInt loc[3];

  loc[0] = loc[1] = loc[2] = 0;
  iMin = iMax = jMin = jMax = kMin = kMax = 0;

  switch(aDim) {
  case 3:
    {
      TInt nbX = this->GetGrilleStructure()[0];
      TInt nbY = this->GetGrilleStructure()[1];
      TInt nbZ = this->GetGrilleStructure()[2];
      TInt d01 = nbX*nbY, dX = 1, dY = 1, dZ = 1;
      if ( isSub )
      {
        if ( theId < nbX * (nbY-1) * (nbZ-1))
        { // face is normal to X axis
          dX = 0;
        }
        else if ( theId < nbX * (nbY-1) * (nbZ-1) + (nbX-1) * nbY * (nbZ-1))
        {  // face is normal to Y axis
          theId -= nbX * (nbY-1) * (nbZ-1);
          dY = 0;
        }
        else
        {
          theId -= nbX * (nbY-1) * (nbZ-1) + (nbX-1) * nbY * (nbZ-1);
          dZ = 0;
        }
      }
      //else
      {
        iMin = theId % (nbX - dX);
        jMin = (theId / (nbX - dX)) % (nbY - dY);
        kMin = theId / ((nbX - dX) * (nbY - dY));
        iMax = iMin+dX;
        jMax = jMin+dY;
        kMax = kMin+dZ;
      }
      for (loc[2]=kMin; loc[2]<=kMax; loc[2]++)
        for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
          for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
          {
            idx = loc[0] + loc[1]*nbX + loc[2]*d01;
            anIndexes.push_back(idx);
          }
      break;
    }
  case 2:
    {
      TInt nbX = this->GetGrilleStructure()[0];
      TInt nbY = this->GetGrilleStructure()[1];
      TInt dX = 1, dY = 1;
      if ( isSub )
      {
        if ( theId < nbX * (nbY-1))
        { // edge is normal to X axis
          dX = 0;
        }
        else
        {
          theId -= nbX * (nbY-1);
          dY = 0;
        }
      }
      iMin = theId % (nbX-dX);
      jMin = theId / (nbX-dX);
      iMax = iMin+dX;
      jMax = jMin+dY;
      for (loc[1]=jMin; loc[1]<=jMax; loc[1]++)
        for (loc[0]=iMin; loc[0]<=iMax; loc[0]++)
        {
          idx = loc[0] + loc[1]*nbX;
          anIndexes.push_back(idx);
        }
      break;
    }
  case 1:
    {
      iMin = theId;
      for (loc[0]=iMin; loc[0]<=iMin+1; loc[0]++)
      {
        idx = loc[0];
        anIndexes.push_back(idx);
      }
      break;
    }
  }

  return anIndexes;
}

TInt
TGrilleInfo
::GetFamNumNode(TInt theId) const 
{
  return myFamNumNode[theId];
}

void
TGrilleInfo
::SetFamNumNode(TInt theId,TInt theVal) 
{
  myFamNumNode[theId] = theVal;
}

TInt
TGrilleInfo
::GetFamNum(TInt theId) const 
{
  return myFamNum[theId];
}

void
TGrilleInfo
::SetFamNum(TInt theId,TInt theVal) 
{
  myFamNum[theId] = theVal;
}

TInt
TGrilleInfo
::GetFamSubNum(TInt theId) const 
{
  return myFamSubNum[theId];
}

void
TGrilleInfo
::SetFamSubNum(TInt theId,TInt theVal) 
{
  myFamSubNum[theId] = theVal;
}
