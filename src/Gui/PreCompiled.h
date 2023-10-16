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


#ifndef GUI_PRECOMPILED_H
#define GUI_PRECOMPILED_H

#include <FCConfig.h>

#ifdef FC_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif

// point at which warnings of overly long specifiers disabled (needed for VC6)
#ifdef _MSC_VER
#pragma warning( disable : 4251 )
#pragma warning( disable : 4273 )
#pragma warning( disable : 4275 )
#pragma warning( disable : 4503 )
#pragma warning( disable : 4786 )  // specifier longer then 255 chars
#endif

#ifdef _PreComp_

// standard
#include <cstdio>
#include <cassert>
#include <fcntl.h>
#include <cctype>
#include <typeinfo>
#include <cfloat>
#include <climits>

#ifdef FC_OS_WIN32
#include <Windows.h>
#include <io.h>
#include <shellapi.h>
#endif

// streams
#include <iostream>
#include <iomanip>

// STL
#include <atomic>
#include <vector>
#include <map>
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <stack>
#include <queue>
#include <sstream>
#include <bitset>
#include <unordered_set>
#include <unordered_map>
#include <tuple>

// Boost
#include <boost_graph_adjacency_list.hpp>
#include <boost_signals2.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/bind/bind.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/filesystem/exception.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/program_options.hpp>
#include <boost/utility.hpp>

// Xerces
#include <xercesc/util/TranscodingException.hpp>
#include <xercesc/util/XMLString.hpp>

// Keep this order to avoid compiler warnings
#include "QtAll.h"
#include "InventorAll.h"

#elif defined(FC_OS_WIN32)
#include <windows.h>
#endif  //_PreComp_

#endif // GUI_PRECOMPILED_H
