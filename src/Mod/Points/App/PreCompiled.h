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

#ifndef POINTS_PRECOMPILED_H
#define POINTS_PRECOMPILED_H

#include <FCConfig.h>

// point at which warnings of overly long specifiers disabled (needed for VC6)
#ifdef _MSC_VER
#pragma warning(disable : 4181)
#pragma warning(disable : 4305)
#pragma warning(disable : 4522)
#endif

#ifdef _PreComp_

// standard
#include <cstdio>

// STL
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <vector>

// boost
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/regex.hpp>

// Qt
#include <QtConcurrentMap>

#endif  //_PreComp_

#endif
