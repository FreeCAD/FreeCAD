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
// File      : SMDS_MemoryLimit.cxx
// Created   : Fri Sep 21 17:16:42 2007
// Author    : Edward AGAPOV (eap)
// Executable to find out a lower RAM limit (MB), i.e. at what size of freeRAM
// reported by sysinfo, no more memory can be allocated.
// This is not done inside a function of SALOME because allocated memory is not returned
// to the system. (PAL16631)
//
#ifndef WIN32
#if !(defined(__MACH__) && defined(__APPLE__))
#include <sys/sysinfo.h>
#endif
#endif

#ifdef _DEBUG_
#include <iostream>
#endif

int main (int argc, char ** argv)
{
#if (defined(__MACH__) && defined(__APPLE__))
	//do nothing
#else
  // To better understand what is going on here, consult bug [SALOME platform 0019911]
#ifndef WIN32
  struct sysinfo si;
  int err = sysinfo( &si );
  if ( err )
    return -1;
  unsigned long freeRamKb = ( si.freeram  * si.mem_unit ) / 1024;

  // totat RAM size in Gb, float is in order not to have 1 instead of 1.9
  float totalramGb = float( si.totalram * si.mem_unit ) / 1024 / 1024 / 1024;

  // nb Kbites to allocate at one step. Small nb leads to hung up
  const int stepKb = int( 5 * totalramGb );

  unsigned long nbSteps = freeRamKb / stepKb * 2;
  try {
    while ( nbSteps-- ) {
      new char[stepKb*1024];
      err = sysinfo( &si );
      if ( !err )
        freeRamKb = ( si.freeram  * si.mem_unit ) / 1024;
    }
  } catch (...) {}

// #ifdef _DEBUG_
//   std::cout << freeRamKb / 1024 << std::endl;
// #endif
  return freeRamKb / 1024;
#endif
#endif

  return -1;
}
