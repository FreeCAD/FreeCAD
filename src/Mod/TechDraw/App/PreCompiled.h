/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef TECHDRAW_PRECOMPILED_H
#define TECHDRAW_PRECOMPILED_H

#include <FCConfig.h>

// Exporting of App classes
#ifdef FC_OS_WIN32
# define TechDrawExport  __declspec(dllexport)
# define DrawingExport   __declspec(dllexport)
# define PartExport      __declspec(dllimport)
# define MeasureExport   __declspec(dllimport)
# define MeshExport      __declspec(dllimport)
# define SpreadsheetExport     __declspec(dllimport)
# define ImportExport    __declspec(dllimport)
#else // for Linux
# define TechDrawExport
# define DrawingExport
# define MeasureExport
# define PartExport
# define MeshExport
# define SpreadsheetExport
# define ImportExport
#endif

#ifdef _MSC_VER
# pragma warning( disable : 4275 )
#endif

#ifdef _PreComp_

// standard
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdio>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <boost/graph/boyer_myrvold_planar_test.hpp>
#include <boost/graph/is_kuratowski_subgraph.hpp>
#include <boost/regex.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <QDomDocument>
#include "QDomNodeModel.h"
#include <QFile>
#include <QLocale>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QtConcurrentRun>
#include <QXmlQuery>
#include <QXmlResultItems>

// OpenCasCade =====================================================================================
#include <Mod/Part/App/OpenCascadeAll.h>

#endif // _PreComp_
#endif
