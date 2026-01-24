// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2002 Jürgen Riegel <juergen.riegel@web.de>                             *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef POINTSGUI_PRECOMPILED_H
#define POINTSGUI_PRECOMPILED_H

#include <FCConfig.h>


// STL
#include <algorithm>
#include <limits>
#include <memory>

// boost
#include <boost/math/special_functions/fpclassify.hpp>

// Qt
#include <QDialog>
#include <QInputDialog>
#include <QMessageBox>

// Inventor
#include <Inventor/SbVec2f.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoIndexedPointSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoPointSet.h>

#endif  // POINTSGUI_PRECOMPILED_H
