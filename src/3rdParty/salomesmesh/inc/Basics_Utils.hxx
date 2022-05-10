// Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
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

//  SALOME Utils : general SALOME's definitions and tools
//  File   : Basics_DirUtils.hxx
//  Author  : Alexander A. BORODIN
//  Module : SALOME
//
#ifndef _Basics_UTILS_HXX_
#define _Basics_UTILS_HXX_

#include "SALOME_Basics.hxx"
#include <string>
#include <iostream>

#ifndef WIN32
#include <sys/time.h>
#else
// avoid name collision with std::byte in C++17
#define NOCRYPT
#define NOGDI NOGDI
#include <winsock2.h>
#include <windows.h>
#pragma comment(lib,"winmm.lib")
#endif


namespace Kernel_Utils
{
  BASICS_EXPORT std::string GetHostname();

  class BASICS_EXPORT Localizer
  {
  public:
    Localizer();
    ~Localizer();
  private:
    std::string myCurLocale;
  };
  
  //! GUID type
  enum GUIDtype {
    DefUserID = 1,  //!< Default user attribute ID
    ObjectdID       //!< Global usage object identifier ID
  };

  //! Get predefined GUID
  BASICS_EXPORT std::string GetGUID( GUIDtype );
#ifndef WIN32
  BASICS_EXPORT void print_traceback();
#else
#if (_MSC_VER >= 1400) // Visual Studio 2005
  BASICS_EXPORT int setenv(const char*, const char*, int);
#endif
#endif
}


//
// =============================================================
// Helper macro for time analysis
// =============================================================
//
#ifndef WIN32
#define START_TIMING(name) static long name##tcount=0;static long name##cumul;long name##tt0; timeval name##tv; gettimeofday(&name##tv,0); \
                           name##tt0=name##tv.tv_usec+name##tv.tv_sec*1000000; \
                           if(name##tcount==0)std::cerr<<__FILE__<<":"<<__LINE__<<":"<<#name<<std::endl;

#define END_TIMING(name,NUMBER) name##tcount=name##tcount+1;gettimeofday(&name##tv,0); \
                                name##cumul=name##cumul+name##tv.tv_usec+name##tv.tv_sec*1000000 -name##tt0; \
                                if(name##tcount==NUMBER){ \
                                  std::cerr <<__FILE__<<":"<<__LINE__<<":"<<#name<<" temps CPU(mus): "<< name##cumul<<std::endl; \
                                  name##tcount=0;name##cumul=0;}
#else

#define START_TIMING(name) static long name##tcount=0;static DWORD name##cumul;DWORD  name##tv;DWORD  name##tt0 = timeGetTime(); \
                           if(name##tcount==0)std::cerr<<__FILE__<<":"<<__LINE__<<":"<<#name<<std::endl;

#define END_TIMING(name,NUMBER) name##tcount=name##tcount+1; name##tv = timeGetTime(); \
                                name##cumul=name##cumul+name##tv - name##tt0; \
                                if(name##tcount==NUMBER){ \
                                  std::cerr <<__FILE__<<":"<<__LINE__<<":"<<#name<<" temps CPU(mus): "<< name##cumul<<std::endl; \
                                  name##tcount=0;name##cumul=0;}
#endif



//
// =============================================================
// Macro and template functions for type conversions.
// =============================================================
//
#include <string>
#include <sstream>
#include <stdlib.h>

template < class T >
std::string ToString(const T &arg)
{
  std::stringstream out;
  out << arg;
  return(out.str());
}

template < class T >
double ToDouble(const T &arg) {
  std::stringstream out;
  out << arg;
  double value = atof(out.str().c_str());
  return value;
}

//
// =============================================================
// Simple Logger macros (no dependency with SALOME)
// =============================================================
//
#if defined(_DEBUG_) || defined(_DEBUG)
#define STDLOG(msg) {std::cerr<<std::flush<<__FILE__<<" ["<<__LINE__<<"] : "<<msg<<std::endl<<std::flush;}
#else
#define STDLOG(msg)
#endif

#ifdef LOG
#undef LOG
#endif
#define LOG STDLOG


#endif //_Basics_UTILS_HXX_
