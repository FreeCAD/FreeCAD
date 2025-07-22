//  Copyright (C) 2007-2008  CEA/DEN, EDF R&D, OPEN CASCADE
//
//  Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
//  CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
//  See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
#include "UNV2411_Structure.hxx"
#include "UNV2412_Structure.hxx"
#include "UNV_Utilities.hxx"

#include "DriverUNV_R_SMDS_Mesh.h"
#include "DriverUNV_W_SMDS_Mesh.h"

using namespace std;

#ifdef DEBUG
static int MYDEBUG = 1;
#else
static int MYDEBUG = 0;
#endif


void ReadMed(const char* theFileName){
  std::ifstream in_stream(theFileName);

  UNV2411::TDataSet aDataSet2411;
  UNV2411::Read(in_stream,aDataSet2411);

  in_stream.seekg(0);
  UNV2412::TDataSet aDataSet2412;
  UNV2412::Read(in_stream,aDataSet2412);

  string aFileName(theFileName);
  aFileName += "-";
  std::ofstream out_stream(aFileName.c_str());

  UNV2411::Write(out_stream,aDataSet2411);
  UNV2412::Write(out_stream,aDataSet2412);
}


int main(int argc, char** argv){ 
  DriverUNV_R_SMDS_Mesh aR;
  DriverUNV_W_SMDS_Mesh aW;
  try{
    if(argc > 1){
      ReadMed(argv[1]);
    }
    return 0;
  }catch(std::exception& exc){
    MESSAGE("Follow exception was accured :\n"<<exc.what());
  }catch(...){
    MESSAGE("Unknown exception was accured !!!");
  } 
  return 1;
}
