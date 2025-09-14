/***************************************************************************
 *   Copyright (c) Juergen Riegel 2007    <juergen.riegel@web.de>          *
 *   LGPL                                                                  *
 ***************************************************************************/

#ifndef __PRECOMPILED__

#define __PRECOMPILED__

#include <FCConfig.h>

// Importing of App classes
#ifdef FC_OS_WIN32
#define MeshExport __declspec(dllimport)
#define AppJtReaderExport __declspec(dllexport)
#else  // for Linux
#define MeshExport
#define AppJtReaderExport
#endif

#ifdef _PreComp_

// standard

#include <cassert>
#include <cstdio>

// STL
#include <algorithm>
#include <bitset>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <strstream>
#include <vector>

// sys
#include <sys/types.h>

#endif  //_PreComp_

#endif
