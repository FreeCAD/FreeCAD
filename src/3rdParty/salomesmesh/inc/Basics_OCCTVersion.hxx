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

// File   : Basics_OCCTVersion.hxx
// Author : Julia DOROVSKIKH, Open CASCADE S.A.S (julia.dorovskikh@opencascade.com)

#ifndef BASICS_OCCTVERSION_HXX
#define BASICS_OCCTVERSION_HXX

#include <Standard_Version.hxx>

//
// NOTE: Since version 6.7.0 OCC_VERSION_DEVELOPMENT macro in the Standard_Version.hxx
// points to the development status of the OCCT version: for example "dev", "alpha",
// "beta", "rc1", etc.
// OCC_VERSION_MAJOR, OCC_VERSION_MINOR and OCC_VERSION_MAINTENANCE macros
// specify actual (final) version number; for development version it is a future
// target version number (i.e. version number is incremented immediately after
// releasing of the stable version).
//

#ifdef OCC_VERSION_SERVICEPACK
#  define OCC_VERSION_LARGE (OCC_VERSION_MAJOR << 24 | OCC_VERSION_MINOR << 16 | OCC_VERSION_MAINTENANCE << 8 | OCC_VERSION_SERVICEPACK)
#else
#  ifdef OCC_VERSION_DEVELOPMENT
#    define OCC_VERSION_LARGE ((OCC_VERSION_MAJOR << 24 | OCC_VERSION_MINOR << 16 | OCC_VERSION_MAINTENANCE << 8)-1)
#  else
#    define OCC_VERSION_LARGE (OCC_VERSION_MAJOR << 24 | OCC_VERSION_MINOR << 16 | OCC_VERSION_MAINTENANCE << 8)
#  endif
#endif

#endif // BASICS_OCCTVERSION_HXX
