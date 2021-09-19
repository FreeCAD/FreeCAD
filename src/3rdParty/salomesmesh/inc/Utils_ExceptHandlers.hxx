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

//  KERNEL Utils : common utils for KERNEL
//  File   : Utils_ExceptHandlers.hxx
//  Author : Oksana Tchebanova
//  Module : KERNEL
//  $Header:
//
#ifndef Utils_ExceptHandlers_HeaderFile
#define Utils_ExceptHandlers_HeaderFile

#include "SALOME_Utils.hxx"

#include <stdexcept>

typedef void (*PVF)();

class UTILS_EXPORT Unexpect { //save / retrieve unexpected exceptions treatment
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

class UTILS_EXPORT Terminate {//save / retrieve terminate function
  
  PVF old;
  public :
#ifndef _MSC_VER
    Terminate( PVF f ) 
      { old = std::set_terminate(f); }
  ~Terminate() { std::set_terminate(old); }
#else
    Terminate( PVF f ) 
      { old = ::set_terminate(f); }
  ~Terminate() { ::set_terminate(old); }
#endif
};

#define UNEXPECT_CATCH(FuncName, ExceptionConstructor) \
inline void FuncName () {\
   throw ExceptionConstructor (); \
}
//Example of the usage 

// void DTC_NotFound () {
//   throw (SALOME_DataTypeCatalog::NotFound());
// }
// or the same :
//
// UNEXPECT_CATCH( DTC_NotFound , SALOME_DataTypeCatalog::NotFound)
// in the function body :
// ....
// Unexpect aCatch(DTC_NotFound) // redefinition of the unexpect exceptions handler
// ....


//Definitions :
UTILS_EXPORT extern void SalomeException();
UTILS_EXPORT extern void SALOME_SalomeException();

#endif
