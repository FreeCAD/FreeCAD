/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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


#ifndef APP_PRECOMPILED_H
#define APP_PRECOMPILED_H

#include <FCConfig.h>

// here get the warnings of too long specifiers disabled
#ifdef _MSC_VER
#pragma warning( disable : 4251 )
#pragma warning( disable : 4273 )
#pragma warning( disable : 4275 )
#pragma warning( disable : 4482 )  // nonstandard extension used: enum 'App::ObjectStatus' used in qualified name
#pragma warning( disable : 4503 )
#pragma warning( disable : 4786 )  // specifier longer then 255 chars
#endif


#ifdef FC_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#endif

#ifdef _PreComp_

// standard
#include <cstdio>
#include <cassert>
#include <ctime>
#include <csignal>

#ifdef FC_OS_WIN32
#include <direct.h>
#include <windows.h>
#include <crtdbg.h>
#endif


// Streams
#include <iostream>
#include <sstream>

// STL 
#include <string>
#include <list>
#include <map>
#include <vector>
#include <set>
#include <stack>
#include <sstream>
#include <queue>
#include <bitset>
#include <exception>
#include <random>
#include <unordered_set>
#include <unordered_map>

// Boost
#include <boost/signals2.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>

#include <boost/tuple/tuple.hpp>
#include <boost/utility.hpp>
#include <boost/graph/adjacency_list.hpp>

#include <boost/program_options.hpp>
//namespace po = boost::program_options;

#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/exception.hpp>

#endif //_PreComp_

#endif // APP_PRECOMPILED_H
