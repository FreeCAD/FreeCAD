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
#ifndef MED_Utilities_HeaderFile
#define MED_Utilities_HeaderFile

#include "MED_WrapperBase.hxx"

#include <iostream>     
#include <sstream>      
#include <string>
#include <string.h>
#include <stdexcept>


namespace MED
{
  class MEDWRAPPER_EXPORT PrefixPrinter
  {
    static int myCounter;
    bool myIsActive;
  public:
    PrefixPrinter(bool theIsActive = true);
    ~PrefixPrinter();

    static std::string GetPrefix();
  };
}

#ifdef _DEBUG_
  #define MSG(deb,msg) if(deb) std::cout<<MED::PrefixPrinter::GetPrefix()<<msg<<" ("<<__FILE__<<" ["<<__LINE__<<"])\n"
  #define BEGMSG(deb,msg) if(deb) std::cout<<MED::PrefixPrinter::GetPrefix()<<msg
  #define INITMSGA(deb,lev,msg) MED::PrefixPrinter aPrefixPrinter_##lev(deb); BEGMSG(deb,msg)
  #define INITMSG(deb,msg) INITMSGA(deb,,msg)
  #define ADDMSG(deb,msg) if(deb) std::cout<<msg
#else
  #define MSG(deb,msg)
  #define BEGMSG(deb,msg)
  #define INITMSGA(deb,lev,msg)
  #define INITMSG(deb,msg)
  #define ADDMSG(deb,msg)
#endif


#ifndef EXCEPTION

#define EXCEPTION(TYPE, MSG) {\
  std::ostringstream aStream;\
  aStream<<__FILE__<<"["<<__LINE__<<"]::"<<MSG;\
  throw TYPE(aStream.str().c_str());\
}

#endif

#endif
