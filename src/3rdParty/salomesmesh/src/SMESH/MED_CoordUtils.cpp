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
#include "MED_CoordUtils.hxx"
#include "MED_Utilities.hxx"
 
namespace MED
{

  enum ECoordName{eX, eY, eZ, eNone};

  template<ECoordName TCoordId>
  TFloat 
  GetCoord(const TCCoordSlice& theCoordSlice)
  {
    return theCoordSlice[TCoordId];
  }

  template<>
  TFloat 
  GetCoord<eNone>(const TCCoordSlice& theCoordSlice)
  {
    return 0.0;
  }
  
  TGetCoord
  aXYZGetCoord[3] = {
    &GetCoord<eX>, 
    &GetCoord<eY>, 
    &GetCoord<eZ>
  };
  
  TGetCoord
  aXYGetCoord[3] = {
    &GetCoord<eX>, 
    &GetCoord<eY>, 
    &GetCoord<eNone>
  };
  
  TGetCoord
  aYZGetCoord[3] = {
    &GetCoord<eNone>,
    &GetCoord<eX>, 
    &GetCoord<eY>
  };
  
  TGetCoord 
  aXZGetCoord[3] = {
    &GetCoord<eX>, 
    &GetCoord<eNone>,
    &GetCoord<eY>
  };
  
  
  TGetCoord 
  aXGetCoord[3] = {
    &GetCoord<eX>, 
    &GetCoord<eNone>,
    &GetCoord<eNone>
  };
  
  TGetCoord
  aYGetCoord[3] = {
    &GetCoord<eNone>,
    &GetCoord<eX>, 
    &GetCoord<eNone>
  };

  TGetCoord
  aZGetCoord[3] = {
    &GetCoord<eNone>,
    &GetCoord<eNone>,
    &GetCoord<eX>
  };

  
  //---------------------------------------------------------------
  TCoordHelper
  ::TCoordHelper(TGetCoord* theGetCoord):
    myGetCoord(theGetCoord)
  {}

  TFloat 
  TCoordHelper
  ::GetCoord(TCCoordSlice& theCoordSlice, 
             TInt theCoordId)
  {
    return (*myGetCoord[theCoordId])(theCoordSlice);
  }


  //---------------------------------------------------------------
  PCoordHelper
  GetCoordHelper(PNodeInfo theNodeInfo)
  {
    PCoordHelper aCoordHelper;
    {
      PMeshInfo aMeshInfo = theNodeInfo->GetMeshInfo();
      TInt aMeshDimension = aMeshInfo->GetDim();
      bool anIsDimPresent[3] = {false, false, false};
      for(int iDim = 0; iDim < aMeshDimension; iDim++){
        // PAL16857(SMESH not conform to the MED convention) ->
        // 1D - always along X
        // 2D - always in XOY plane
        anIsDimPresent[iDim] = iDim < aMeshDimension;
//      std::string aName = theNodeInfo->GetCoordName(iDim);
//         if ( aName.size() > 1 ) // PAL12148, aName has size 8 or 16
//           aName = aName.substr(0,1);
//      if(aName == "x" || aName == "X")
//        anIsDimPresent[eX] = true;
//      else if(aName == "y" || aName == "Y")
//        anIsDimPresent[eY] = true;
//      else if(aName == "z" || aName == "Z")
//        anIsDimPresent[eZ] = true;
      }

      switch(aMeshDimension){
      case 3:
        aCoordHelper.reset(new TCoordHelper(aXYZGetCoord));
        break;
      case 2:
        if(anIsDimPresent[eY] && anIsDimPresent[eZ])
          aCoordHelper.reset(new TCoordHelper(aYZGetCoord));
        else if(anIsDimPresent[eX] && anIsDimPresent[eZ])
          aCoordHelper.reset(new TCoordHelper(aXZGetCoord));
        else
          aCoordHelper.reset(new TCoordHelper(aXYGetCoord));
        break;
      case 1:
        if(anIsDimPresent[eY])
          aCoordHelper.reset(new TCoordHelper(aYGetCoord));
        else if(anIsDimPresent[eZ])
          aCoordHelper.reset(new TCoordHelper(aZGetCoord));
        else
          aCoordHelper.reset(new TCoordHelper(aXGetCoord));
        break;
      }
    }
    return aCoordHelper;
  }
}
