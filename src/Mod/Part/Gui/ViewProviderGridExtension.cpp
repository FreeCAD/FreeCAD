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
#endif

#include <Base/Parameter.h>
#include <App/Application.h>
#include <Mod/Part/App/PropertyTopoShape.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Gui/SoFCBoundingBox.h>

#include "ViewProviderGridExtension.h"


using namespace PartGui;
using namespace std;

EXTENSION_PROPERTY_SOURCE(PartGui::ViewProviderGridExtension, Gui::ViewProviderExtension)

enum class GridStyle {
    Dashed = 0,
    Light = 1,
};

const char* ViewProviderGridExtension::GridStyleEnums[] = { "Dashed","Light",nullptr };
App::PropertyQuantityConstraint::Constraints ViewProviderGridExtension::GridSizeRange = { 0.001,DBL_MAX,1.0 };

namespace PartGui {

class GridExtensionP : public ParameterGrp::ObserverType {
public:
    explicit GridExtensionP(ViewProviderGridExtension *);
    ~GridExtensionP();

    void drawGrid(bool cameraUpdate = false);

    void setEnabled(bool enable);
    bool getEnabled();

    SoSeparator * getGridRoot();

    // Configurable parameters (to be configured by specific VP)
    int GridSizePixelThreshold = 15;
    int GridNumberSubdivision = 10;
    int GridLinePattern = 0x0f0f;
    int GridDivLinePattern = 0xffff;
    int GridLineWidth = 1;
    int GridDivLineWidth = 2;
    unsigned int GridLineColor;
    unsigned int GridDivLineColor;

    // VP Independent parameters through observer
    int unitsUserSchema = 0;

    /** Observer for parameter group. */
    void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;

private:
    double computeGridSize(const Gui::View3DInventorViewer* viewer);
    void createGrid(bool cameraUpdate = false);
    void createGridPart(double Step, int numberSubdiv, bool divLines, bool subDivLines, int pattern, SoBaseColor* color, int lineWidth = 1);

    bool checkCameraZoomChange(const Gui::View3DInventorViewer* viewer);
    bool checkCameraTranslationChange(const Gui::View3DInventorViewer* viewer);

    void createEditModeInventorNodes();

    SbVec3f camCenterOnSketch;
    float camMaxDimension;

private:
    ViewProviderGridExtension * vp;

    bool enabled = false;

    // scenograph
    SoSeparator * GridRoot;
};

} // namespace PartGui


GridExtensionP::GridExtensionP(ViewProviderGridExtension * vp):
    camCenterOnSketch(SbVec3f(0., 0., 0.)),
    camMaxDimension(200.),
    vp(vp),
    GridRoot(nullptr)
{
    SbColor lineCol(0.7f, 0.7f, 0.7f);
    GridLineColor = lineCol.getPackedValue();
    GridDivLineColor = GridLineColor;

    createEditModeInventorNodes();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Units");
    unitsUserSchema = hGrp->GetInt("UserSchema", 0); //2 3 5 7 are imperial schemas. 2 3 inches, 5 7 feet
    hGrp->Attach(this);
}

GridExtensionP::~GridExtensionP()
{
    Gui::coinRemoveAllChildren(GridRoot);
    GridRoot->unref();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Units");
    hGrp->Detach(this);
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
    SbVec3f newCamCenterOnSketch = viewer->getCenterPointOnFocalPlane();

    if ((camCenterOnSketch - newCamCenterOnSketch).length() > 0.1 * camMaxDimension) {
        camCenterOnSketch = newCamCenterOnSketch;
        return true;
    }

    return false;
}

double GridExtensionP::computeGridSize(const Gui::View3DInventorViewer* viewer)
{
    short pixelWidth = -1;
    short pixelHeight = -1;
    viewer->getViewportRegion().getViewportSizePixels().getValue(pixelWidth, pixelHeight);
    if (pixelWidth < 0 || pixelHeight < 0)
        return vp->GridSize.getValue();

    int numberOfLines = static_cast<int>(std::max(pixelWidth, pixelHeight)) / GridSizePixelThreshold;

    double unitMultiplier = (unitsUserSchema == 2 || unitsUserSchema == 3) ? 25.4 : (unitsUserSchema == 5 || unitsUserSchema == 7) ? 304.8 : 1;

    double newGridSize = unitMultiplier * pow(GridNumberSubdivision, 1 + floor(log(camMaxDimension / unitMultiplier / numberOfLines) / log(GridNumberSubdivision)));

    //cap the grid size
    newGridSize = std::max(newGridSize, 0.000001);
    newGridSize = std::min(newGridSize, 10000000.0);

    if (newGridSize != vp->GridSize.getValue()) // avoid unnecessary property update
        vp->GridSize.setValue(newGridSize); //grid size must be set for grid snap. But we need to block it from calling createGrid.

    return newGridSize;
}

void GridExtensionP::createGrid(bool cameraUpdate)
{
    Gui::MDIView* mdi = Gui::Application::Instance->editDocument()->getActiveView();
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(mdi)->getViewer();

    bool cameraDimensionsChanged = checkCameraZoomChange(viewer);

    bool cameraCenterMoved = checkCameraTranslationChange(viewer);

    bool gridNeedUpdating = cameraDimensionsChanged || cameraCenterMoved;

    if (!gridNeedUpdating && cameraUpdate)
        return;

    Gui::coinRemoveAllChildren(GridRoot);

    double step;
    if (vp->GridAuto.getValue())
        step = computeGridSize(viewer);
    else
        step = vp->GridSize.getValue();

    auto getColor = [](auto unpackedcolor) {
        SoBaseColor* lineColor = new SoBaseColor;
        float transparency;
        SbColor lineCol(0.7f, 0.7f, 0.7f);
        lineCol.setPackedValue(unpackedcolor, transparency);
        lineColor->rgb.setValue(lineCol);
        return lineColor;
    };

    //First we create the subdivision lines
    createGridPart(step, GridNumberSubdivision, true,
                   (GridNumberSubdivision == 1), GridLinePattern,
                   getColor(GridLineColor), GridLineWidth);

    //Second we create the wider lines marking every nth lines
    if (GridNumberSubdivision > 1) {
        createGridPart(step, GridNumberSubdivision, false, true,
            GridDivLinePattern, getColor(GridDivLineColor), GridDivLineWidth);
    }
}

void GridExtensionP::createGridPart(double Step, int numberSubdiv, bool subDivLines, bool divLines, int pattern, SoBaseColor* color, int lineWidth)
{
    SoGroup* parent = new Gui::SoSkipBoundingGroup();
    GridRoot->addChild(parent);
    SoVertexProperty* vts;

    parent->addChild(color);

    if (vp->GridStyle.getValue() == static_cast<long>(GridStyle::Dashed)) {
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
    int vlines = static_cast<int>(gridDimension / Step); // total number of vertical lines
    int nlines = 2 * vlines; // total number of lines
    if (nlines > 2000) {
        Gui::coinRemoveAllChildren(GridRoot);
        return;
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

void GridExtensionP::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    (void) rCaller;

    if (strcmp(sReason, "UserSchema") == 0) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Units");
        unitsUserSchema = hGrp->GetInt("UserSchema", 0); //2 3 5 7 are imperial schemas. 2 3 inches, 5 7 feet
    }
}

ViewProviderGridExtension::ViewProviderGridExtension()
{

    EXTENSION_ADD_PROPERTY_TYPE(ShowGrid, (false), "Grid", (App::PropertyType)(App::Prop_None), "Switch the grid on/off");
    EXTENSION_ADD_PROPERTY_TYPE(GridSize, (10.0), "Grid", (App::PropertyType)(App::Prop_None), "Gap size of the grid");
    EXTENSION_ADD_PROPERTY_TYPE(GridStyle, (0L), "Grid", (App::PropertyType)(App::Prop_None), "Appearance style of the grid");
    EXTENSION_ADD_PROPERTY_TYPE(GridSnap, (false), "Grid", (App::PropertyType)(App::Prop_None), "Switch the grid snap on/off");
    EXTENSION_ADD_PROPERTY_TYPE(GridAuto, (true), "Grid", (App::PropertyType)(App::Prop_None), "Change size of grid based on view area.");

    initExtensionType(ViewProviderGridExtension::getExtensionClassTypeId());

    GridStyle.setEnums(GridStyleEnums);
    GridSize.setConstraints(&GridSizeRange);

    pImpl = std::make_unique<GridExtensionP>(this);
}

ViewProviderGridExtension::~ViewProviderGridExtension()
{}

void ViewProviderGridExtension::setGridEnabled(bool enable)
{
    pImpl->setEnabled(enable);
}

void ViewProviderGridExtension::drawGrid(bool cameraUpdate)
{
    pImpl->drawGrid(cameraUpdate);
}

SoSeparator* ViewProviderGridExtension::getGridNode()
{
    return pImpl->getGridRoot();
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
            prop == &GridSize ||
            prop == &GridStyle) {
                pImpl->drawGrid();
        }
    }
}

void ViewProviderGridExtension::setGridSizePixelThreshold(int value)
{
    pImpl->GridSizePixelThreshold = value;
}

void ViewProviderGridExtension::setGridNumberSubdivision(int value)
{
    pImpl->GridNumberSubdivision = value;
}

void ViewProviderGridExtension::setGridLinePattern(int pattern)
{
    pImpl->GridLinePattern = pattern;
}

void ViewProviderGridExtension::setGridDivLinePattern(int pattern)
{
    pImpl->GridDivLinePattern = pattern;
}

void ViewProviderGridExtension::setGridLineWidth(int width)
{
    pImpl->GridLineWidth = width;
}

void ViewProviderGridExtension::setGridDivLineWidth(int width)
{
    pImpl->GridDivLineWidth = width;
}

void ViewProviderGridExtension::setGridLineColor(unsigned int color)
{
    pImpl->GridLineColor = color;
}

void ViewProviderGridExtension::setGridDivLineColor(unsigned int color)
{
    pImpl->GridDivLineColor = color;
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
