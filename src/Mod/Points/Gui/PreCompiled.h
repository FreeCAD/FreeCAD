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

#ifndef POINTSGUI_PRECOMPILED_H
#define POINTSGUI_PRECOMPILED_H

#include <FCConfig.h>

#ifdef FC_OS_WIN32
# ifndef NOMINMAX
#  define NOMINMAX
# endif
#endif

#ifdef _PreComp_

// STL
# include <algorithm>
# include <limits>
# include <memory>

#ifdef FC_OS_WIN32
# include <windows.h>
#endif

// boost
# include <boost/math/special_functions/fpclassify.hpp>

// Qt
# include <QDialog>
# include <QInputDialog>

// Inventor
# include <Inventor/SbVec2f.h>
# include <Inventor/errors/SoDebugError.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoIndexedPointSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoNormal.h>
# include <Inventor/nodes/SoPointSet.h>

#endif  //_PreComp_

#endif // POINTSGUI_PRECOMPILED_H
