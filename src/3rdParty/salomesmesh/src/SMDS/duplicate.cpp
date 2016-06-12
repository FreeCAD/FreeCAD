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

//  SALOME Utils : general SALOME's definitions and tools
//  File   : duplicate.cxx
//  Author : Antoine YESSAYAN, EDF
//  Module : SALOME
//  $Header$
//
/*!
 *      This function can be changed by strdup() if strdup() is ANSI.
 *      It is strongly (and only) used in the Registry environment
 *      (RegistryService, RegistryConnexion, Identity, ...)
 */
extern "C"
{
#include <stdlib.h>
#include <string.h>
}
#include  "utilities.h"
#include "OpUtil.hxx"

const char* duplicate( const char *const str )
{
        ASSERT(str!=NULL) ;
        const size_t length = strlen( str ) ;
        ASSERT(length>0) ;
        char *new_str = new char[ 1+length ] ;
        ASSERT(new_str) ;
        strcpy( new_str , str ) ;
        return new_str ;
}
