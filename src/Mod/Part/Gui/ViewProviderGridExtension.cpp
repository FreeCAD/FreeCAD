/***************************************************************************
 *   Copyright (c) 2023 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
# include <cfloat>

# include <Inventor/nodes/SoDepthBuffer.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/SbVec3f.h>

# include <QApplication>
#endif

#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Base/Parameter.h>
#include <App/Application.h>
#include <Mod/Part/App/PropertyTopoShape.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MDIView.h>

#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Gui/SoFCBoundingBox.h>

#include "ViewProviderGridExtension.h"


using namespace PartGui;
using namespace std;

EXTENSION_PROPERTY_SOURCE(PartGui::ViewProviderGridExtension, Gui::ViewProviderExtension)

App::PropertyQuantityConstraint::Constraints ViewProviderGridExtension::GridSizeRange = { 0.001,DBL_MAX,1.0 };

namespace PartGui {

class GridExtensionP {
public:
    explicit GridExtensionP(ViewProviderGridExtension *);
    ~GridExtensionP();

    void drawGrid(bool cameraUpdate = false);

    void setEnabled(bool enable);
    bool getEnabled();

    SoSeparator * getGridRoot();

    void getClosestGridPoint(double &x, double &y) const;
    double getGridSize() const;

    void setGridOrientation(Base::Vector3d origin, Base::Rotation rotation);

    // Configurable parameters (to be configured by specific VP)
    int GridSizePixelThreshold = 15;
    int GridNumberSubdivision = 10;
    int GridLinePattern = 0x0f0f;
    int GridDivLinePattern = 0xffff;
    int GridLineWidth = 1;
    int GridDivLineWidth = 2;
    unsigned int GridLineColor;
    unsigned int GridDivLineColor;

private:
    void computeGridSize(const Gui::View3DInventorViewer* viewer);
    void createGrid(bool cameraUpdate = false);
    void createGridPart(int numberSubdiv, bool divLines, bool subDivLines, int pattern, SoBaseColor* color, int lineWidth = 1);

    bool checkCameraZoomChange(const Gui::View3DInventorViewer* viewer);
    bool checkCameraTranslationChange(const Gui::View3DInventorViewer* viewer);

    void createEditModeInventorNodes();

    Base::Vector3d getCamCenterInSketchCoordinates() const;

    SbVec3f camCenterPointOnFocalPlane;
    float camMaxDimension;

    Base::Vector3d gridOrigin;
    Base::Rotation gridRotation;

private:
    ViewProviderGridExtension * vp;

    bool enabled = false;
    double computedGridValue = 10;

    bool isTooManySegmentsNotified = false;

    // scenograph
    SoSeparator * GridRoot;
};

} // namespace PartGui


GridExtensionP::GridExtensionP(ViewProviderGridExtension * vp):
    camCenterPointOnFocalPlane(SbVec3f(0., 0., 0.)),
    camMaxDimension(200.),
    vp(vp),
    GridRoot(nullptr)
{
    SbColor lineCol(0.7f, 0.7f, 0.7f);
    GridLineColor = lineCol.getPackedValue();
    GridDivLineColor = GridLineColor;

    createEditModeInventorNodes();
}

GridExtensionP::~GridExtensionP()
{
    Gui::coinRemoveAllChildren(GridRoot);
    GridRoot->unref();
}

void GridExtensionP::setGridOrientation(Base::Vector3d origin, Base::Rotation rotation)
{
    gridOrigin = origin;
    gridRotation = rotation;
}

double GridExtensionP::getGridSize() const
{
    return computedGridValue;
}

void GridExtensionP::getClosestGridPoint(double &x, double &y) const
{
    auto closestdim = [](double &dim, double gridValue) {
        dim = dim / gridValue;
        dim = dim < 0.0 ? ceil(dim - 0.5) : floor(dim + 0.5);
        dim *= gridValue;
    };

    closestdim(x, computedGridValue);
    closestdim(y, computedGridValue);

    //Base::Console().Log("gridvalue=%f, (x,y)=(%f,%f)", computedGridValue, x, y);
}

bool GridExtensionP::checkCameraZoomChange(const Gui::View3DInventorViewer* viewer)
{
    float newCamMaxDimension = viewer->getMaxDimension();
    if (fabs(newCamMaxDimension - camMaxDimension) > 0) { //ie if user zoomed.
        camMaxDimension = newCamMaxDimension;
        return true;
    }

    return false;
}

bool GridExtensionP::checkCameraTranslationChange(const Gui::View3DInventorViewer* viewer)
{
    //Then we check if user moved by more than 10% of camera dimension (must be after updating camera dimension).
    SbVec3f newCamCenterPointOnFocalPlane = viewer->getCenterPointOnFocalPlane();

    if ((camCenterPointOnFocalPlane - newCamCenterPointOnFocalPlane).length() > 0.1 * camMaxDimension) {
        camCenterPointOnFocalPlane = newCamCenterPointOnFocalPlane;
        return true;
    }

    return false;
}

void GridExtensionP::computeGridSize(const Gui::View3DInventorViewer* viewer)
{

    auto capGridSize = [](auto & value){
        value = std::max(static_cast<float>(value), std::numeric_limits<float>::min());
        value = std::min(static_cast<float>(value), std::numeric_limits<float>::max());
    };

    if (!vp->GridAuto.getValue()) {
        computedGridValue = vp->GridSize.getValue();
        capGridSize(computedGridValue);
        return;
    }

    short pixelWidth = -1;
    short pixelHeight = -1;
    viewer->getViewportRegion().getViewportSizePixels().getValue(pixelWidth, pixelHeight);
    if (pixelWidth < 0 || pixelHeight < 0) {
        computedGridValue = vp->GridSize.getValue();
        return;
    }

    int numberOfLines = static_cast<int>(std::max(pixelWidth, pixelHeight)) / GridSizePixelThreshold;

    // If number of subdivision is 1, grid auto spacing can't work as it uses it as a factor
    // In such case, we apply a default factor of 10
    auto safeGridNumberSubdivision = GridNumberSubdivision <= 1 ? 10 : GridNumberSubdivision;

    computedGridValue = vp->GridSize.getValue() * pow(safeGridNumberSubdivision, 1 + floor(log(camMaxDimension / numberOfLines / vp->GridSize.getValue()) / log(safeGridNumberSubdivision)));

    //cap the grid size
    capGridSize(computedGridValue);
}

void GridExtensionP::createGrid(bool cameraUpdate)
{
    auto view = dynamic_cast<Gui::View3DInventor*>(Gui::Application::Instance->editDocument()->getActiveView());

    if(!view)
        return;

    Gui::View3DInventorViewer* viewer = view->getViewer();

    bool cameraDimensionsChanged = checkCameraZoomChange(viewer);

    bool cameraCenterMoved = checkCameraTranslationChange(viewer);

    bool gridNeedUpdating = cameraDimensionsChanged || cameraCenterMoved;

    if (!gridNeedUpdating && cameraUpdate)
        return;

    Gui::coinRemoveAllChildren(GridRoot);

    computeGridSize(viewer);

    auto getColor = [](auto unpackedcolor) {
        SoBaseColor* lineColor = new SoBaseColor;
        float transparency;
        SbColor lineCol(0.7f, 0.7f, 0.7f);
        lineCol.setPackedValue(unpackedcolor, transparency);
        lineColor->rgb.setValue(lineCol);
        return lineColor;
    };

    //First we create the subdivision lines
    createGridPart(GridNumberSubdivision, true,
                   (GridNumberSubdivision == 1), GridLinePattern,
                   getColor(GridLineColor), GridLineWidth);

    //Second we create the wider lines marking every nth lines
    if (GridNumberSubdivision > 1) {
        createGridPart(GridNumberSubdivision, false, true,
            GridDivLinePattern, getColor(GridDivLineColor), GridDivLineWidth);
    }
}

void GridExtensionP::createGridPart(int numberSubdiv, bool subDivLines, bool divLines, int pattern, SoBaseColor* color, int lineWidth)
{
    SoGroup* parent = new Gui::SoSkipBoundingGroup();
    GridRoot->addChild(parent);
    SoVertexProperty* vts;

    parent->addChild(color);

    SoDrawStyle* DefaultStyle = new SoDrawStyle;
    DefaultStyle->lineWidth = lineWidth;
    DefaultStyle->linePattern = pattern;
    parent->addChild(DefaultStyle);

    SoPickStyle* PickStyle = new SoPickStyle;
    PickStyle->style = SoPickStyle::UNPICKABLE;
    parent->addChild(PickStyle);

    SoLineSet* grid = new SoLineSet;
    vts = new SoVertexProperty;
    grid->vertexProperty = vts;

    float gridDimension = 1.5 * camMaxDimension;
    int vlines = static_cast<int>(gridDimension / computedGridValue); // total number of vertical lines
    int nlines = 2 * vlines; // total number of lines

    if (nlines > 2000) {
        if(!isTooManySegmentsNotified) {
            Base::Console().Warning("The grid is too dense, so it is being disabled. Consider zooming in or changing the grid configuration\n");
            isTooManySegmentsNotified = true;
        }

        Gui::coinRemoveAllChildren(GridRoot);
        return;
    }
    else {
        isTooManySegmentsNotified = false;
    }

    // set the grid indices
    grid->numVertices.setNum(nlines);
    auto * vertices = grid->numVertices.startEditing();
    for (int i = 0; i < nlines; i++)
        vertices[i] = 2;
    grid->numVertices.finishEditing();

    // set the grid coordinates
    vts->vertex.setNum(2 * nlines);
    SbVec3f* vertex_coords = vts->vertex.startEditing();

    float minX, minY, maxX, maxY;
    Base::Vector3d camCenterOnSketch = getCamCenterInSketchCoordinates();
    minX = static_cast<float>(camCenterOnSketch.x);
    minY = static_cast<float>(camCenterOnSketch.y);

    minX -= (gridDimension / 2);
    minY -= (gridDimension / 2);
    maxX = minX + gridDimension;
    maxY = minY + gridDimension;

    // vertical lines
    int i_offset_x = static_cast<int>(minX / computedGridValue);
    for (int i = 0; i < vlines; i++) {
        int iStep = (i + i_offset_x);
        if (((iStep % numberSubdiv == 0) && divLines) || ((iStep % numberSubdiv != 0) && subDivLines)) {
            vertex_coords[2 * i].setValue(iStep * computedGridValue, minY, 0);
            vertex_coords[2 * i + 1].setValue(iStep * computedGridValue, maxY, 0);
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
    int i_offset_y = static_cast<int>(minY / computedGridValue) - vlines;
    for (int i = vlines; i < nlines; i++) {
        int iStep = (i + i_offset_y);
        if (((iStep % numberSubdiv == 0) && divLines) || ((iStep % numberSubdiv != 0) && subDivLines)) {
            vertex_coords[2 * i].setValue(minX, iStep * computedGridValue, 0);
            vertex_coords[2 * i + 1].setValue(maxX, iStep * computedGridValue, 0);
        }
        else {
            vertex_coords[2 * i].setValue(0, 0, 0);
            vertex_coords[2 * i + 1].setValue(0, 0, 0);
        }
    }
    vts->vertex.finishEditing();

    parent->addChild(vts);
    parent->addChild(grid);
}

Base::Vector3d GridExtensionP::getCamCenterInSketchCoordinates() const
{
    Base::Vector3d xaxis(1, 0, 0), yaxis(0, 1, 0);

    gridRotation.multVec(xaxis,xaxis);
    gridRotation.multVec(yaxis,yaxis);

    float x,y,z;
    camCenterPointOnFocalPlane.getValue(x, y, z);

    Base::Vector3d center (x,y,z);

    center.TransformToCoordinateSystem(gridOrigin, xaxis, yaxis);

    return center;
}

void GridExtensionP::setEnabled(bool enable)
{
    enabled=enable;

    drawGrid();

}

bool GridExtensionP::getEnabled()
{
    return enabled;
}

void GridExtensionP::createEditModeInventorNodes()
{
    // Create Grid Coin nodes ++++++++++++++++++++++++++++++++++++++++++
    GridRoot = new SoSeparator();
    GridRoot->ref();
    GridRoot->setName("GridRoot");

}

SoSeparator * GridExtensionP::getGridRoot()
{
    return GridRoot;
}

void GridExtensionP::drawGrid(bool cameraUpdate) {
    if (vp->ShowGrid.getValue() && enabled) {
        createGrid(cameraUpdate);
    }
    else {
        Gui::coinRemoveAllChildren(GridRoot);
    }
}

ViewProviderGridExtension::ViewProviderGridExtension()
{

    EXTENSION_ADD_PROPERTY_TYPE(ShowGrid, (false), "Grid", (App::PropertyType)(App::Prop_None), "Switch the grid on/off");
    EXTENSION_ADD_PROPERTY_TYPE(GridSize, (10.0), "Grid", (App::PropertyType)(App::Prop_None), "Gap size of the grid");
    EXTENSION_ADD_PROPERTY_TYPE(GridAuto, (true), "Grid", (App::PropertyType)(App::Prop_None), "Change size of grid based on view area.");

    initExtensionType(ViewProviderGridExtension::getExtensionClassTypeId());

    GridSize.setConstraints(&GridSizeRange);

    pImpl = std::make_unique<GridExtensionP>(this);


}

ViewProviderGridExtension::~ViewProviderGridExtension() = default;

void ViewProviderGridExtension::setGridEnabled(bool enable)
{
    pImpl->setEnabled(enable);
}

void ViewProviderGridExtension::drawGrid(bool cameraUpdate)
{
    pImpl->drawGrid(cameraUpdate);
}

void ViewProviderGridExtension::setGridOrientation(Base::Vector3d origin, Base::Rotation rotation)
{
    pImpl->setGridOrientation(origin, rotation);
}

SoSeparator* ViewProviderGridExtension::getGridNode()
{
    return pImpl->getGridRoot();
}

double ViewProviderGridExtension::getGridSize() const
{
    return pImpl->getGridSize();
}

void ViewProviderGridExtension::getClosestGridPoint(double &x, double &y) const
{
    return pImpl->getClosestGridPoint(x, y);
}

void ViewProviderGridExtension::extensionUpdateData(const App::Property* prop)
{
    if(pImpl->getEnabled()) {
        if (prop->getTypeId() == Part::PropertyPartShape::getClassTypeId()) {
            pImpl->drawGrid();
        }
    }
}

void ViewProviderGridExtension::extensionOnChanged(const App::Property* prop)
{
    if(pImpl->getEnabled()) {
        if (prop == &ShowGrid ||
            prop == &GridAuto ||
            prop == &GridSize ) {
                pImpl->drawGrid();
        }
    }
}

void ViewProviderGridExtension::setGridSizePixelThreshold(int value)
{
    pImpl->GridSizePixelThreshold = value;
    drawGrid(false);
}

void ViewProviderGridExtension::setGridNumberSubdivision(int value)
{
    pImpl->GridNumberSubdivision = value;
    drawGrid(false);
}

void ViewProviderGridExtension::setGridLinePattern(int pattern)
{
    pImpl->GridLinePattern = pattern;
    drawGrid(false);
}

void ViewProviderGridExtension::setGridDivLinePattern(int pattern)
{
    pImpl->GridDivLinePattern = pattern;
    drawGrid(false);
}

void ViewProviderGridExtension::setGridLineWidth(int width)
{
    pImpl->GridLineWidth = width;
    drawGrid(false);
}

void ViewProviderGridExtension::setGridDivLineWidth(int width)
{
    pImpl->GridDivLineWidth = width;
    drawGrid(false);
}

void ViewProviderGridExtension::setGridLineColor(const App::Color & color)
{
    pImpl->GridLineColor = color.getPackedValue();
    drawGrid(false);
}

void ViewProviderGridExtension::setGridDivLineColor(const App::Color & color)
{
    pImpl->GridDivLineColor = color.getPackedValue();
    drawGrid(false);
}

bool ViewProviderGridExtension::extensionHandleChangedPropertyType(Base::XMLReader& reader,
    const char* TypeName,
    App::Property* prop)
{
    Base::Type inputType = Base::Type::fromName(TypeName);

    if (prop->getTypeId().isDerivedFrom(App::PropertyFloat::getClassTypeId()) &&
        inputType.isDerivedFrom(App::PropertyFloat::getClassTypeId())) {
        // Do not directly call the property's Restore method in case the implementation
        // has changed. So, create a temporary PropertyFloat object and assign the value.
        App::PropertyFloat floatProp;
        floatProp.Restore(reader);
        static_cast<App::PropertyFloat*>(prop)->setValue(floatProp.getValue());
        return true;
    }

    return false;
}

namespace Gui {
    EXTENSION_PROPERTY_SOURCE_TEMPLATE(PartGui::ViewProviderGridExtensionPython, PartGui::ViewProviderGridExtension)

// explicit template instantiation
    template class PartGuiExport ViewProviderExtensionPythonT<PartGui::ViewProviderGridExtension>;
}
