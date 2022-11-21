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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <Inventor/nodes/SoDepthBuffer.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/SbVec3f.h>
#endif  // #ifndef _PreComp_

#include <Gui/Application.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCBoundingBox.h>

#include "ViewProviderSketch.h"
#include "EditModeCoinManager.h"
#include "EditModeGridCoinManager.h"

using namespace SketcherGui;

//**************************** EditModeGridCoinManager class ******************************

EditModeGridCoinManager::EditModeGridCoinManager(   ViewProviderSketch &vp,
                                                    EditModeScenegraphNodes & editModeScenegraph):
    camCenterOnSketch(SbVec3f(0., 0., 0.)),
    camMaxDimension(200.),
    viewProvider(vp),
    editModeScenegraphNodes(editModeScenegraph)
{
}

EditModeGridCoinManager::~EditModeGridCoinManager()
{}

double EditModeGridCoinManager::getGridSize(const Gui::View3DInventorViewer* viewer)
{
    short pixelWidth = -1;
    short pixelHeight = -1;
    viewer->getViewportRegion().getViewportSizePixels().getValue(pixelWidth, pixelHeight);
    if (pixelWidth < 0 || pixelHeight < 0)
        return viewProvider.GridSize.getValue();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    int numberOfLines = std::max(pixelWidth, pixelHeight) / hGrp->GetInt("GridSizePixelThreshold", 15);
    ParameterGrp::handle hGrp2 = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Units");
    int units = hGrp2->GetInt("UserSchema", 0); //2 3 5 7 are imperial schemas. 2 3 inches, 5 7 feet
    double unitMultiplier = (units == 2 || units == 3) ? 25.4 : (units == 5 || units == 7) ? 304.8 : 1;

    int subdivisions = hGrp->GetInt("GridNumberSubdivision", 10);
    double newGridSize = unitMultiplier * pow(subdivisions, 1 + floor(log(camMaxDimension / unitMultiplier / numberOfLines) / log(subdivisions)));

    //cap the grid size
    newGridSize = std::max(newGridSize, 0.000001);
    newGridSize = std::min(newGridSize, 10000000.0);

    if (newGridSize != viewProvider.GridSize.getValue()) // protect from recursive calls
        viewProvider.GridSize.setValue(newGridSize); //grid size must be set for grid snap. But we need to block it from calling createGrid.

    return newGridSize;
}

SoSeparator* EditModeGridCoinManager::createGrid(bool cameraUpdate)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    Gui::MDIView* mdi = Gui::Application::Instance->editDocument()->getActiveView();
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(mdi)->getViewer();

    //First check if the user zoomed in or out by getting the camera dimension.
    float newCamMaxDimension = viewer->getMaxDimension();
    bool cameraDimensionsChanged = false;
    if (fabs(newCamMaxDimension - camMaxDimension) > 0) { //ie if user zoomed.
        camMaxDimension = newCamMaxDimension;
        cameraDimensionsChanged = true;
    }

    //Then we check if user moved by more than 10% of camera dimension (must be after updating camera dimension).
    SbVec3f newCamCenterOnSketch = viewer->getCenterPointOnFocalPlane();
    bool cameraCenterMoved = false;
    if ((camCenterOnSketch - newCamCenterOnSketch).length() > 0.1 * camMaxDimension) {
        camCenterOnSketch = newCamCenterOnSketch;
        cameraCenterMoved = true;
    }

    bool gridNeedUpdating = cameraDimensionsChanged || cameraCenterMoved;

    if (!gridNeedUpdating && cameraUpdate)
        return editModeScenegraphNodes.GridRoot;

    Gui::coinRemoveAllChildren(editModeScenegraphNodes.GridRoot);

    double step;
    if (viewProvider.GridAuto.getValue())
        step = getGridSize(viewer);
    else
        step = viewProvider.GridSize.getValue();

    int numberSubdivision = hGrp->GetInt("GridNumberSubdivision", 10);

    auto getColor = [](auto prefName) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
        SoBaseColor* lineColor = new SoBaseColor;
        float transparency;
        SbColor lineCol(0.7f, 0.7f, 0.7f);
        unsigned long lineColorL = hGrp->GetUnsigned(prefName, (unsigned long)(lineCol.getPackedValue()));
        lineCol.setPackedValue((uint32_t)lineColorL, transparency);
        lineColor->rgb.setValue(lineCol);
        return lineColor;

    };

    //First we create the subdivision lines
    createGridPart(step, numberSubdivision, true, (numberSubdivision == 1) ? true : false,
        hGrp->GetInt("GridLinePattern", 0x0f0f), getColor("GridLineColor"), hGrp->GetInt("GridLineWidth", 1));

    //Second we create the wider lines marking every nth lines
    if (numberSubdivision > 1) {
        createGridPart(step, numberSubdivision, false, true,
            hGrp->GetInt("GridDivLinePattern", 0xffff), getColor("GridDivLineColor"), hGrp->GetInt("GridDivLineWidth", 2));
    }

    return editModeScenegraphNodes.GridRoot;
}

SoSeparator* EditModeGridCoinManager::createGridPart(double Step, int numberSubdiv, bool subDivLines, bool divLines, int pattern, SoBaseColor* color, int lineWidth)
{
    SoGroup* parent = new Gui::SoSkipBoundingGroup();
    editModeScenegraphNodes.GridRoot->addChild(parent);
    SoVertexProperty* vts;

    parent->addChild(color);

    if (viewProvider.GridStyle.getValue() == 0) {
        SoDrawStyle* DefaultStyle = new SoDrawStyle;
        DefaultStyle->lineWidth = lineWidth;
        DefaultStyle->linePattern = pattern;
        parent->addChild(DefaultStyle);
    }
    else {
        SoMaterial* LightStyle = new SoMaterial;
        LightStyle->transparency = 0.7f;
        parent->addChild(LightStyle);
    }

    SoPickStyle* PickStyle = new SoPickStyle;
    PickStyle->style = SoPickStyle::UNPICKABLE;
    parent->addChild(PickStyle);

    SoLineSet* grid = new SoLineSet;
    vts = new SoVertexProperty;
    grid->vertexProperty = vts;

    float gridDimension = 1.5 * camMaxDimension;
    int vlines = static_cast<int>(gridDimension / Step);
    int nlines = 2 * vlines;
    if (nlines > 2000) {
        Gui::coinRemoveAllChildren(editModeScenegraphNodes.GridRoot);
        return editModeScenegraphNodes.GridRoot;
    }

    // set the grid indices
    grid->numVertices.setNum(nlines);
    int32_t* vertices = grid->numVertices.startEditing();
    for (int i = 0; i < nlines; i++)
        vertices[i] = 2;
    grid->numVertices.finishEditing();

    // set the grid coordinates
    vts->vertex.setNum(2 * nlines);
    SbVec3f* vertex_coords = vts->vertex.startEditing();

    float minX, minY, maxX, maxY, z;
    camCenterOnSketch.getValue(minX, minY, z);
    minX -= (gridDimension / 2);
    minY -= (gridDimension / 2);
    maxX = minX + gridDimension;
    maxY = minY + gridDimension;

    // vertical lines
    int i_offset_x = static_cast<int>(minX / Step);
    for (int i = 0; i < vlines; i++) {
        int iStep = (i + i_offset_x);
        if (((iStep % numberSubdiv == 0) && divLines) || ((iStep % numberSubdiv != 0) && subDivLines)) {
            vertex_coords[2 * i].setValue(iStep * Step, minY, 0);
            vertex_coords[2 * i + 1].setValue(iStep * Step, maxY, 0);
        }
        else {
            /*the number of vertices is defined before. To know the number of vertices ahead it would require
            to run the loop once before, which would double computation time.
            If vertices are not filled then there're visual bugs so there are here filled with dummy values.*/
            vertex_coords[2 * i].setValue(0, 0, 0);
            vertex_coords[2 * i + 1].setValue(0, 0, 0);
        }
    }

    // horizontal lines
    int i_offset_y = static_cast<int>(minY / Step) - vlines;
    for (int i = vlines; i < nlines; i++) {
        int iStep = (i + i_offset_y);
        if (((iStep % numberSubdiv == 0) && divLines) || ((iStep % numberSubdiv != 0) && subDivLines)) {
            vertex_coords[2 * i].setValue(minX, iStep * Step, 0);
            vertex_coords[2 * i + 1].setValue(maxX, iStep * Step, 0);
        }
        else {
            vertex_coords[2 * i].setValue(0, 0, 0);
            vertex_coords[2 * i + 1].setValue(0, 0, 0);
        }
    }
    vts->vertex.finishEditing();

    parent->addChild(vts);
    parent->addChild(grid);

    return editModeScenegraphNodes.GridRoot;
}
