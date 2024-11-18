/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifdef _MSC_VER
#pragma warning(disable : 4181)
#pragma warning(disable : 4267)
#pragma warning(disable : 4275)
#pragma warning(disable : 4305)
#pragma warning(disable : 4522)
#endif

// pcl headers include <boost/bind.hpp> instead of <boost/bind/bind.hpp>
#ifndef BOOST_BIND_GLOBAL_PLACEHOLDERS
#define BOOST_BIND_GLOBAL_PLACEHOLDERS
#endif

#ifdef _PreComp_

// standard
#include <map>

// boost
#include <boost/math/special_functions/fpclassify.hpp>

// OpenCasCade
#include <Geom_BSplineSurface.hxx>
#include <Precision.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <math_Gauss.hxx>
#include <math_Householder.hxx>

// Qt
#include <QFuture>
#include <QFutureWatcher>
#include <QtConcurrentMap>

#endif  // _PreComp_
#endif
