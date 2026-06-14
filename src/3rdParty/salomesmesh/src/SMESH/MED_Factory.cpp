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

#include "MED_Factory.hxx"
#include "MED_Utilities.hxx"
#include "MED_V2_2_Wrapper.hxx"

#include <stdio.h>
#include <sstream>

#include <med.h>
extern "C"
{
#ifndef WIN32
  #include <unistd.h>
#endif
}

#ifdef _DEBUG_
static int MYDEBUG = 0;
#else
static int MYDEBUG = 0;
#endif

namespace MED
{
  
  EVersion GetVersionId(const std::string& theFileName,
                        bool theDoPreCheckInSeparateProcess)
  {
    INITMSG(MYDEBUG,"GetVersionId - theFileName = '"<<theFileName<<"'"<<std::endl);
    EVersion aVersion = eVUnknown;    

#ifndef WIN32
    if (access(theFileName.c_str(),F_OK))
      return aVersion;
    if(theDoPreCheckInSeparateProcess){
      // First check, is it possible to deal with the file
      std::ostringstream aStr;
      // File name is in quotes for the case of space(s) inside it (PAL13009)
      aStr<<"bash -c \""<<getenv("SMESH_ROOT_DIR")<<"/bin/salome/mprint_version \'"<<theFileName<<"\'\"";
      if(!MYDEBUG)
        aStr<<" 2>&1 > /dev/null";

      std::string aCommand = aStr.str();
      int aStatus = system(aCommand.c_str());

      BEGMSG(MYDEBUG,"aCommand = '"<<aCommand<<"'; aStatus = "<<aStatus<<std::endl);
      if(aStatus != 0)
        return aVersion;
    }
#endif
    // check compatibility of hdf and med versions
    med_bool hdfok, medok;
    MEDfileCompatibility(theFileName.c_str(), &hdfok, &medok);
    if ((!hdfok) /*|| (!medok)*/) // med-2.1 is KO since med-3.0.0
      return aVersion;

    // Next, try to open the file trough the MED API
    const char* aFileName = theFileName.c_str();
    med_idt aFid = MEDfileOpen(aFileName,MED_ACC_RDONLY);

    MSG(MYDEBUG,"GetVersionId - theFileName = '"<<theFileName<<"'; aFid = "<<aFid<<std::endl);
    if(aFid >= 0){
      med_int aMajor, aMinor, aRelease;
      med_err aRet = MEDfileNumVersionRd(aFid,&aMajor,&aMinor,&aRelease);
      INITMSG(MYDEBUG,"GetVersionId - theFileName = '"<<theFileName<<"'; aRet = "<<aRet<<std::endl);
      if(aRet >= 0){
        if(aMajor == 2 && aMinor == 1)
          aVersion = eV2_1;
        else
          aVersion = eV2_2;
      }
      else {
        // VSR: simulate med 2.3.6 behavior, med file version is assumed to 2.1
        aVersion = eV2_1;
      }
    }
    MEDfileClose(aFid);

    BEGMSG(MYDEBUG,"GetVersionId - theFileName = '"<<theFileName<<"'; aVersion = "<<aVersion<<std::endl);
    return aVersion;
  }

  bool getMEDVersion( const std::string& fname, int& major, int& minor, int& release )
  {
    med_idt f = MEDfileOpen(fname.c_str(), MED_ACC_RDONLY );
    if( f<0 )
      return false;

    med_int aMajor, aMinor, aRelease;
    med_err aRet = MEDfileNumVersionRd( f, &aMajor, &aMinor, &aRelease );
    major = aMajor;
    minor = aMinor;
    release = aRelease;
    MEDfileClose( f );
    if( aRet<0 ) {
      // VSR: simulate med 2.3.6 behavior, med file version is assumed to 2.1
      major = 2; minor = release = -1;
      //return false;
    }
    return true;
  }

  PWrapper CrWrapper(const std::string& theFileName,
                     bool theDoPreCheckInSeparateProcess)
  {
    PWrapper aWrapper;
    EVersion aVersion = GetVersionId(theFileName,theDoPreCheckInSeparateProcess);
    switch(aVersion){
    case eV2_2:
      aWrapper.reset(new MED::V2_2::TVWrapper(theFileName));
      break;
    case eV2_1:
      EXCEPTION(std::runtime_error,"Cannot open file '"<<theFileName<<"'. Med version 2.1 is not supported any more.");
      //aWrapper.reset(new MED::V2_1::TVWrapper(theFileName));
      break;
    default:
      EXCEPTION(std::runtime_error,"MED::CrWrapper - theFileName = '"<<theFileName<<"'");
    }
    return aWrapper;
  }

  PWrapper CrWrapper(const std::string& theFileName, EVersion theId)
  {
    EVersion aVersion = GetVersionId(theFileName);

    if(aVersion != theId)
      remove(theFileName.c_str());
    
    PWrapper aWrapper;
    switch(theId){
    case eV2_2:
      aWrapper.reset(new MED::V2_2::TVWrapper(theFileName));
      break;
    case eV2_1:
      EXCEPTION(std::runtime_error,"Cannot open file '"<<theFileName<<"'. Med version 2.1 is not supported any more.");
      //aWrapper.reset(new MED::V2_1::TVWrapper(theFileName));
      break;
    default:
      aWrapper.reset(new MED::V2_2::TVWrapper(theFileName));
    }
    return aWrapper;
  }

}
