/***************************************************************************
 *   Copyright (c) Juergen Riegel 2007    <juergen.riegel@web.de>          *
 *   LGPL                                                                  *
 ***************************************************************************/


#ifndef __PRECOMPILED__

#define __PRECOMPILED__



#include <FCConfig.h>



// Importing of App classes
#ifdef FC_OS_WIN32
# define MeshExport        __declspec(dllimport)
# define AppJtReaderExport __declspec(dllexport)
#else // for Linux
# define MeshExport
# define AppJtReaderExport
#endif

#ifdef _PreComp_



/// here get the warnings of to long specifieres disabled (needed for VC6)

#ifdef _MSC_VER
# pragma warning( disable : 4251 )
# pragma warning( disable : 4503 )
# pragma warning( disable : 4275 )
# pragma warning( disable : 4786 )  // specifier longer then 255 chars
#endif


// standard

#include <stdio.h>
#include <assert.h>


// STL
#include <algorithm>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <vector>
#include <bitset>
// Xerces
#include <xercesc/util/XercesDefs.hpp>




#endif //_PreComp_

#endif



