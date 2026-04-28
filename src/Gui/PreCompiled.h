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


#pragma once

#include <FCConfig.h>

// standard
#include <cstdio>
#include <cassert>
#include <fcntl.h>
#include <cctype>
#include <typeinfo>

#ifdef FC_OS_WIN32
# include <Windows.h>
# include <io.h>
# include <shellapi.h>
#endif

// streams
#include <iostream>
#include <iomanip>

// STL
#include <algorithm>
#include <atomic>
#include <bitset>
#include <limits>
#include <list>
#include <map>
#include <numbers>
#include <optional>
#include <queue>
#include <random>
#include <ranges>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <ranges>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Boost
#include <boost_graph_adjacency_list.hpp>
#include <fastsignals/signal.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/bind/bind.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/math/constants/constants.hpp>
#include <boost/program_options.hpp>
#include <boost/utility.hpp>

// Xerces
#include <xercesc/util/TranscodingException.hpp>
#include <xercesc/util/XMLString.hpp>

// Qt/OpenGL
#include <QOpenGLFramebufferObjectFormat>

// Keep this order to avoid compiler warnings
#include "QtAll.h"
#include "InventorAll.h"

#if defined(FC_OS_WIN32)
# include <windows.h>
#endif
