/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#ifndef __PRECOMPILED__
#define __PRECOMPILED__

#include <FCConfig.h>


/// point at which warnings of overly long specifiers disabled (needet for VC6)
#ifdef _MSC_VER
#pragma warning(disable : 4251)
#pragma warning(disable : 4275)
#pragma warning(disable : 4503)
#pragma warning(disable : 4786)  // specifier longer then 255 chars
#endif


#ifdef _PreComp_

// standard
#include <cassert>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <iostream>
#include <list>
#include <map>
#include <sstream>
#include <vector>

// boost
#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/indexed.hpp>

// OpenCasCade =====================================================================================
// Base
#include <Mod/Part/App/OpenCascadeAll.h>

#endif  //_PreComp_

#endif
