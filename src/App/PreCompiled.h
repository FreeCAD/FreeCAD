// SPDX-License-Identifier: LGPL-2.1-or-later

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
#include <cassert>
#include <csignal>
#include <cstdio>
#include <ctime>

#ifdef FC_OS_WIN32
#include <crtdbg.h>
#include <direct.h>
#include <windows.h>
#endif

#if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#endif

// Streams
#include <iostream>
#include <sstream>

// STL
#include <array>
#include <bitset>
#include <chrono>
#if defined(FC_OS_WIN32)
#include <codecvt>
#endif
#include <exception>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <random>
#include <set>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Boost
#include <boost_graph_adjacency_list.hpp>
#include <fastsignals/signal.h>

#include <boost/program_options.hpp>
#include <boost/regex.hpp>
#include <boost/utility.hpp>
#include <boost/bind/bind.hpp>
#include <boost/core/ignore_unused.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/scope_exit.hpp>

#include <fmt/format.h>

// Qt -- only QtCore
#include <QDir>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QString>
