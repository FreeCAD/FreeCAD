#ifndef EXPORT3DPDFGUI_PRECOMPILED_H
#define EXPORT3DPDFGUI_PRECOMPILED_H

#include <FCConfig.h>

// Importing of Gui classes
#ifdef FC_OS_WIN32
# define Export3DPDFGuiExport __declspec(dllexport)
#else // for Linux
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
#include <cmath>

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

// Qt Toolkit
#include <Gui/QtAll.h>

// Inventor includes OpenGL
#include <Gui/InventorAll.h>

#endif // _PreComp_

#endif // EXPORT3DPDFGUI_PRECOMPILED_H 