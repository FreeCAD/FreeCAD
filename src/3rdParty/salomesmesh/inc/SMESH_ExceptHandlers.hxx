//  KERNEL Utils : common utils for KERNEL
//  Copyright (C) 2003  CEA
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
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//
//
//
//  File   : SMESH_ExceptHandlers.hxx
//  Author : Oksana Tchebanova
//  Module : KERNEL
//  $Header:


#ifndef SMESH_ExceptHandlers_HeaderFile
#define SMESH_ExceptHandlers_HeaderFile

#include <stdexcept>

#if defined SMESH_EXPORTS
#if defined WIN32
#define SMESH_EXPORT __declspec( dllexport )
#else
#define SMESH_EXPORT
#endif
#else
#if defined WNT
#define SMESH_EXPORT __declspec( dllimport )
#else
#define SMESH_EXPORT
#endif
#endif

typedef void (*PVF)();

class SMESH_EXPORT Unexpect { //save / retrieve unexpected exceptions treatment
  PVF old;
  public :
#ifndef _MSC_VER
  // std::set_unexpected has been removed in C++17
    Unexpect( PVF f ) 
      { /*old = std::set_unexpected(f);*/old = f; }
  ~Unexpect() { /*std::set_unexpected(old);*/ }
#else
    Unexpect( PVF f ) 
	  { old = ::set_unexpected(f); }
  ~Unexpect() { ::set_unexpected(old); }
#endif
};

class SMESH_EXPORT Terminate {//save / retrieve terminate function
  
  PVF old;
  public :
#ifndef _MSC_VER
    Terminate( PVF f ) 
      { old = std::set_terminate(f); }
  ~Terminate() { std::set_terminate(old); }
#else
    Terminate( PVF f ) 
	  { old = std::set_terminate(f); }
  ~Terminate() { std::set_terminate(old); }
#endif
};

#define UNEXPECT_CATCH(FuncName, ExceptionConstructor) \
inline void FuncName () {\
   throw ExceptionConstructor (); \
}
//Example of the usage 

// void DTC_NotFound () {
//   throw (SMESH_DataTypeCatalog::NotFound());
// }
// or the same :
//
// UNEXPECT_CATCH( DTC_NotFound , SMESH_DataTypeCatalog::NotFound)
// in the function body :
// ....
// Unexpect aCatch(DTC_NotFound) // redefinition of the unexpect exceptions handler
// ....


//Definitions :
SMESH_EXPORT void SmeshException ();
#endif
