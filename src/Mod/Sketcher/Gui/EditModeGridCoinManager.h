/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef SKETCHERGUI_EditModeGridCoinManager_H
#define SKETCHERGUI_EditModeGridCoinManager_H


#include <Base/Parameter.h>
#include <App/Application.h>

#include "EditModeCoinManagerParameters.h"

class SoSeparator;
class SoBaseColor;
class SbVec3f;
class View3DInventorViewer;

namespace SketcherGui {

class ViewProviderSketch;

/** @brief      Class for managing the Edit mode coin nodes of ViewProviderSketch relating to grid.
 *  @details
 *
 * EditModeGridCoinManager is a helper of EditModeCoinManager specialised in grid management.
 *
 * 1. Creation of Edit mode coin nodes to handle grid representation.
 *
 */
class SketcherGuiExport EditModeGridCoinManager
{

public:
    explicit EditModeGridCoinManager(       ViewProviderSketch &vp,
                                            EditModeScenegraphNodes & editModeScenegraph);
    ~EditModeGridCoinManager();

    double getGridSize(const Gui::View3DInventorViewer* viewer);
    SoSeparator* createGrid(bool cameraUpdate = false);
    SoSeparator* createGridPart(double Step, int numberSubdiv, bool divLines, bool subDivLines, int pattern, SoBaseColor* color, int lineWidth = 1);

    SbVec3f camCenterOnSketch;
    float camMaxDimension;

private:
    ViewProviderSketch & viewProvider;

    EditModeScenegraphNodes & editModeScenegraphNodes;
};


} // namespace SketcherGui


#endif // SKETCHERGUI_EditModeGridCoinManager_H

