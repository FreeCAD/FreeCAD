#ifndef EXPORT3DPDF_PRECOMPILED_H
#define EXPORT3DPDF_PRECOMPILED_H

#include <FCConfig.h>

// Importing of App classes
#ifdef FC_OS_WIN32
# define Export3DPDFAppExport __declspec(dllexport)
# define Export3DPDFGuiExport __declspec(dllexport)
#else // for Linux
# define Export3DPDFAppExport
# define Export3DPDFGuiExport
#endif

#ifdef _PreComp_

// Standard library
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>

// STL
#include <vector>
#include <map>
#include <string>
#include <list>
#include <set>
#include <algorithm>
#include <stack>
#include <queue>
#include <bitset>

#endif // _PreComp_

#endif // EXPORT3DPDF_PRECOMPILED_H 