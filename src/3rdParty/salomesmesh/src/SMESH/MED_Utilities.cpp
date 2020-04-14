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
#include "MED_Utilities.hxx"
#include "MED_Common.hxx"

using namespace std;

#ifdef _DEBUG_
static int MYDEBUG = 0;
#else
// static int MYDEBUG = 0;
#endif


int MED::PrefixPrinter::myCounter = 0;

MED::PrefixPrinter::PrefixPrinter(bool theIsActive):
  myIsActive(theIsActive)
{
  if(myIsActive)
    myCounter++;
  MSG(MYDEBUG,"MED::PrefixPrinter::PrefixPrinter(...)- "<<myCounter);
}

MED::PrefixPrinter::~PrefixPrinter()
{
  if(myIsActive){
    myCounter--;
    //Do not throw exceptions from inside destructors
    //if(myCounter < 0)
    //  EXCEPTION(runtime_error,"PrefixPrinter::~PrefixPrinter() - myCounter("<<myCounter<<") < 0");
  }
}

string MED::PrefixPrinter::GetPrefix()
{
  if(myCounter){
    if(myCounter < 0)
      EXCEPTION(runtime_error,"PrefixPrinter::~PrefixPrinter() - myCounter("<<myCounter<<") < 0");
    return string(myCounter*2,' ');
  }
  return "";
}


static MED::TEntity2GeomSet Entity2GeomSet;

bool InitEntity2GeomSet()
{
  using namespace MED;

  TGeomSet& aGeomARETESet = Entity2GeomSet[eARETE];
  aGeomARETESet.insert(eSEG2);
  aGeomARETESet.insert(eSEG3);

  TGeomSet& aGeomFACESet = Entity2GeomSet[eFACE];
  aGeomFACESet.insert(eTRIA3);
  aGeomFACESet.insert(eQUAD4);
  aGeomFACESet.insert(eTRIA6);
  aGeomFACESet.insert(eTRIA7);
  aGeomFACESet.insert(eQUAD8);
  aGeomFACESet.insert(eQUAD9);
  aGeomFACESet.insert(ePOLYGONE);
  aGeomFACESet.insert(ePOLYGON2);

  TGeomSet& aGeomMAILLESet = Entity2GeomSet[eMAILLE];
  aGeomMAILLESet.insert(ePOINT1);
  aGeomMAILLESet.insert(aGeomARETESet.begin(),aGeomARETESet.end());
  aGeomMAILLESet.insert(aGeomFACESet.begin(),aGeomFACESet.end());
  aGeomMAILLESet.insert(eTETRA4);
  aGeomMAILLESet.insert(ePYRA5);
  aGeomMAILLESet.insert(ePENTA6);
  aGeomMAILLESet.insert(eHEXA8);
  aGeomMAILLESet.insert(eOCTA12);
  aGeomMAILLESet.insert(eTETRA10);
  aGeomMAILLESet.insert(ePYRA13);
  aGeomMAILLESet.insert(ePENTA15);
  aGeomMAILLESet.insert(eHEXA20);
  aGeomMAILLESet.insert(eHEXA27);
  aGeomMAILLESet.insert(ePOLYEDRE);

  /* This combination allows reading nb of models of structure elements */
  Entity2GeomSet[eSTRUCT_ELEMENT].insert(eAllGeoType); 

  return true;
}

static bool anIsInited = InitEntity2GeomSet();

const MED::TEntity2GeomSet& MED::GetEntity2GeomSet()
{
  return Entity2GeomSet;
}


