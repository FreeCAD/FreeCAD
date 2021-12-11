/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel <juergen.riegel@web.de>             *
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
# include <Standard_math.hxx>
# include <Poly_Polygon3D.hxx>
# include <Geom_BSplineCurve.hxx>
# include <Geom_Circle.hxx>
# include <Geom_Ellipse.hxx>
# include <Geom_TrimmedCurve.hxx>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/SoPath.h>
# include <Inventor/SbBox3f.h>
# include <Inventor/SbImage.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/details/SoPointDetail.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoImage.h>
# include <Inventor/nodes/SoInfo.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoPointSet.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoAsciiText.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/nodes/SoFont.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/SbTime.h>

/// Qt Include Files
# include <QAction>
# include <QApplication>
# include <QColor>
# include <QDialog>
# include <QFont>
# include <QImage>
# include <QMenu>
# include <QMessageBox>
# include <QPainter>
# include <QTextStream>
# include <QKeyEvent>
# include <QDesktopWidget>

# include <boost_bind_bind.hpp>
# include <boost/scoped_ptr.hpp>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Converter.h>
#include <Base/Tools.h>
#include <Base/Parameter.h>
#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Base/Interpreter.h>
#include <Base/UnitsSchema.h>
#include <Base/UnitsApi.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/Tools.h>
#include <Gui/Utilities.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/DlgEditFileIncludePropertyExternal.h>
#include <Gui/SoFCBoundingBox.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/Inventor/MarkerBitmaps.h>
#include <Gui/Inventor/SmSwitchboard.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/BodyBase.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/Sketch.h>
#include <Mod/Sketcher/App/GeometryFacade.h>

#include "SoZoomTranslation.h"
#include "SoDatumLabel.h"
#include "EditDatumDialog.h"
#include "ViewProviderSketch.h"
#include "DrawSketchHandler.h"
#include "TaskDlgEditSketch.h"
#include "TaskSketcherValidation.h"
#include "Utils.h"
#include "ViewProviderSketchGeometryExtension.h"
#include <Mod/Sketcher/App/SolverGeometryExtension.h>

FC_LOG_LEVEL_INIT("Sketch",true,true)

// The first is used to point at a SoDatumLabel for some
// constraints, and at a SoMaterial for others...
#define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
#define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
#define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
#define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
#define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
#define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
#define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6

// Macros to define information layer node child positions within type
#define GEOINFO_BSPLINE_DEGREE_POS 0
#define GEOINFO_BSPLINE_DEGREE_TEXT 3
#define GEOINFO_BSPLINE_POLYGON 1

using namespace SketcherGui;
using namespace Sketcher;
namespace bp = boost::placeholders;

SbColor ViewProviderSketch::VertexColor                             (1.0f,0.149f,0.0f);   // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch::CurveColor                              (1.0f,1.0f,1.0f);     // #FFFFFF -> (255,255,255)
SbColor ViewProviderSketch::CurveDraftColor                         (0.0f,0.0f,0.86f);    // #0000DC -> (  0,  0,220)
SbColor ViewProviderSketch::CurveExternalColor                      (0.8f,0.2f,0.6f);     // #CC3399 -> (204, 51,153)
SbColor ViewProviderSketch::CrossColorH                             (0.8f,0.4f,0.4f);     // #CC6666 -> (204,102,102)
SbColor ViewProviderSketch::CrossColorV                             (0.47f,1.0f,0.51f);   // #83FF83 -> (120,255,131)
SbColor ViewProviderSketch::FullyConstrainedColor                   (0.0f,1.0f,0.0f);     // #00FF00 -> (  0,255,  0)
SbColor ViewProviderSketch::ConstrDimColor                          (1.0f,0.149f,0.0f);   // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch::ConstrIcoColor                          (1.0f,0.149f,0.0f);   // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch::NonDrivingConstrDimColor                (0.0f,0.149f,1.0f);   // #0026FF -> (  0, 38,255)
SbColor ViewProviderSketch::ExprBasedConstrDimColor                 (1.0f,0.5f,0.149f);   // #FF7F26 -> (255, 127,38)
SbColor ViewProviderSketch::InformationColor                        (0.0f,1.0f,0.0f);     // #00FF00 -> (  0,255,  0)
SbColor ViewProviderSketch::PreselectColor                          (0.88f,0.88f,0.0f);   // #E1E100 -> (225,225,  0)
SbColor ViewProviderSketch::SelectColor                             (0.11f,0.68f,0.11f);  // #1CAD1C -> ( 28,173, 28)
SbColor ViewProviderSketch::PreselectSelectedColor                  (0.36f,0.48f,0.11f);  // #5D7B1C -> ( 93,123, 28)
SbColor ViewProviderSketch::CreateCurveColor                        (0.8f,0.8f,0.8f);     // #CCCCCC -> (204,204,204)
SbColor ViewProviderSketch::DeactivatedConstrDimColor               (0.8f,0.8f,0.8f);     // #CCCCCC -> (204,204,204)
SbColor ViewProviderSketch::InternalAlignedGeoColor                 (0.7f,0.7f,0.5f);     // #B2B27F -> (178,178,127)
SbColor ViewProviderSketch::FullyConstraintElementColor             (0.50f,0.81f,0.62f);  // #80D0A0 -> (128,208,160)
SbColor ViewProviderSketch::FullyConstraintConstructionElementColor (0.56f,0.66f,0.99f);  // #8FA9FD -> (143,169,253)
SbColor ViewProviderSketch::FullyConstraintInternalAlignmentColor   (0.87f,0.87f,0.78f);  // #DEDEC8 -> (222,222,200)
SbColor ViewProviderSketch::FullyConstraintConstructionPointColor   (1.0f,0.58f,0.50f);   // #FF9580 -> (255,149,128)
SbColor ViewProviderSketch::InvalidSketchColor                      (1.0f,0.42f,0.0f);    // #FF6D00 -> (255,109,  0)
// Variables for holding previous click
SbTime  ViewProviderSketch::prvClickTime;
SbVec2s ViewProviderSketch::prvClickPos;
SbVec2s ViewProviderSketch::prvCursorPos;
SbVec2s ViewProviderSketch::newCursorPos;

//**************************************************************************
// Edit data structure

/// Data structure while editing the sketch
struct EditData {
    EditData():
    sketchHandler(0),
    buttonPress(false),
    handleEscapeButton(false),
    DragPoint(-1),
    DragCurve(-1),
    PreselectPoint(-1),
    PreselectCurve(-1),
    PreselectCross(-1),
    MarkerSize(7),
    coinFontSize(17), // this value is in pixels, 17 pixels
    constraintIconSize(15),
    pixelScalingFactor(1.0),
    blockedPreselection(false),
    FullyConstrained(false),
    //ActSketch(0), // if you are wondering, it went to SketchObject, accessible via getSolvedSketch() and via SketchObject interface as appropriate
    EditRoot(0),
    PointsMaterials(0),
    CurvesMaterials(0),
    RootCrossMaterials(0),
    EditCurvesMaterials(0),
    EditMarkersMaterials(0),
    PointsCoordinate(0),
    CurvesCoordinate(0),
    RootCrossCoordinate(0),
    EditCurvesCoordinate(0),
    EditMarkersCoordinate(0),
    CurveSet(0),
    RootCrossSet(0),
    EditCurveSet(0),
    EditMarkerSet(0),
    PointSet(0),
    textX(0),
    textPos(0),
    constrGroup(0),
    infoGroup(0),
    pickStyleAxes(0),
    PointsDrawStyle(0),
    CurvesDrawStyle(0),
    RootCrossDrawStyle(0),
    EditCurvesDrawStyle(0),
    EditMarkersDrawStyle(0),
    ConstraintDrawStyle(0),
    InformationDrawStyle(0)
    {}

    // pointer to the active handler for new sketch objects
    DrawSketchHandler *sketchHandler;
    bool buttonPress;
    bool handleEscapeButton;

    // dragged point
    int DragPoint;
    // dragged curve
    int DragCurve;
    // dragged constraints
    std::set<int> DragConstraintSet;

    SbColor PreselectOldColor;
    int PreselectPoint;
    int PreselectCurve;
    int PreselectCross;
    int MarkerSize;
    int coinFontSize;
    int constraintIconSize;
    double pixelScalingFactor;
    std::set<int> PreselectConstraintSet;
    bool blockedPreselection;
    bool FullyConstrained;

    // container to track our own selected parts
    std::set<int> SelPointSet;
    std::set<int> SelCurvSet; // also holds cross axes at -1 and -2
    std::set<int> SelConstraintSet;
    std::vector<int> CurvIdToGeoId; // conversion of SoLineSet index to GeoId
    std::vector<int> PointIdToGeoId; // conversion of SoCoordinate3 index to GeoId

    // helper data structures for the constraint rendering
    std::vector<ConstraintType> vConstrType;

    // For each of the combined constraint icons drawn, also create a vector
    // of bounding boxes and associated constraint IDs, to go from the icon's
    // pixel coordinates to the relevant constraint IDs.
    //
    // The outside map goes from a string representation of a set of constraint
    // icons (like the one used by the constraint IDs we insert into the Coin
    // rendering tree) to a vector of those bounding boxes paired with relevant
    // constraint IDs.
    std::map<QString, ViewProviderSketch::ConstrIconBBVec> combinedConstrBoxes;

    // nodes for the visuals
    SoSeparator   *EditRoot;
    SoMaterial    *PointsMaterials;
    SoMaterial    *CurvesMaterials;
    SoMaterial    *RootCrossMaterials;
    SoMaterial    *EditCurvesMaterials;
    SoMaterial    *EditMarkersMaterials;
    SoCoordinate3 *PointsCoordinate;
    SoCoordinate3 *CurvesCoordinate;
    SoCoordinate3 *RootCrossCoordinate;
    SoCoordinate3 *EditCurvesCoordinate;
    SoCoordinate3 *EditMarkersCoordinate;
    SoLineSet     *CurveSet;
    SoLineSet     *RootCrossSet;
    SoLineSet     *EditCurveSet;
    SoMarkerSet   *EditMarkerSet;
    SoMarkerSet   *PointSet;

    SoText2       *textX;
    SoTranslation *textPos;

    SmSwitchboard *constrGroup;
    SoGroup       *infoGroup;
    SoPickStyle   *pickStyleAxes;

    SoDrawStyle * PointsDrawStyle;
    SoDrawStyle * CurvesDrawStyle;
    SoDrawStyle * RootCrossDrawStyle;
    SoDrawStyle * EditCurvesDrawStyle;
    SoDrawStyle * EditMarkersDrawStyle;
    SoDrawStyle * ConstraintDrawStyle;
    SoDrawStyle * InformationDrawStyle;
};


// this function is used to simulate cyclic periodic negative geometry indices (for external geometry)
const Part::Geometry* GeoById(const std::vector<Part::Geometry*> GeoList, int Id)
{
    if (Id >= 0)
        return GeoList[Id];
    else
        return GeoList[GeoList.size()+Id];
}

//**************************************************************************
// Construction/Destruction

/* TRANSLATOR SketcherGui::ViewProviderSketch */

PROPERTY_SOURCE_WITH_EXTENSIONS(SketcherGui::ViewProviderSketch, PartGui::ViewProvider2DObjectGrid)


ViewProviderSketch::ViewProviderSketch()
  : SelectionObserver(false),
    edit(0),
    Mode(STATUS_NONE),
    visibleInformationChanged(true),
    combrepscalehyst(0),
    isShownVirtualSpace(false),
    listener(0)
{
    PartGui::ViewProviderAttachExtension::initExtension(this);

    ADD_PROPERTY_TYPE(Autoconstraints,(true),"Auto Constraints",(App::PropertyType)(App::Prop_None),"Create auto constraints");
    ADD_PROPERTY_TYPE(AvoidRedundant,(true),"Auto Constraints",(App::PropertyType)(App::Prop_None),"Avoid redundant autoconstraint");
    ADD_PROPERTY_TYPE(TempoVis,(Py::None()),"Visibility automation",(App::PropertyType)(App::Prop_None),"Object that handles hiding and showing other objects when entering/leaving sketch.");
    ADD_PROPERTY_TYPE(HideDependent,(true),"Visibility automation",(App::PropertyType)(App::Prop_None),"If true, all objects that depend on the sketch are hidden when opening editing.");
    ADD_PROPERTY_TYPE(ShowLinks,(true),"Visibility automation",(App::PropertyType)(App::Prop_None),"If true, all objects used in links to external geometry are shown when opening sketch.");
    ADD_PROPERTY_TYPE(ShowSupport,(true),"Visibility automation",(App::PropertyType)(App::Prop_None),"If true, all objects this sketch is attached to are shown when opening sketch.");
    ADD_PROPERTY_TYPE(RestoreCamera,(true),"Visibility automation",(App::PropertyType)(App::Prop_None),"If true, camera position before entering sketch is remembered, and restored after closing it.");
    ADD_PROPERTY_TYPE(ForceOrtho,(false),"Visibility automation",(App::PropertyType)(App::Prop_None),"If true, camera type will be forced to orthographic view when entering editing mode.");
    ADD_PROPERTY_TYPE(SectionView,(false),"Visibility automation",(App::PropertyType)(App::Prop_None),"If true, only objects (or part of) located behind the sketch plane are visible.");
    ADD_PROPERTY_TYPE(EditingWorkbench,("SketcherWorkbench"),"Visibility automation",(App::PropertyType)(App::Prop_None),"Name of the workbench to activate when editing this sketch.");

    {//visibility automation: update defaults to follow preferences
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
        this->HideDependent.setValue(hGrp->GetBool("HideDependent", true));
        this->ShowLinks.setValue(hGrp->GetBool("ShowLinks", true));
        this->ShowSupport.setValue(hGrp->GetBool("ShowSupport", true));
        this->RestoreCamera.setValue(hGrp->GetBool("RestoreCamera", true));
        this->ForceOrtho.setValue(hGrp->GetBool("ForceOrtho", false));
        this->SectionView.setValue(hGrp->GetBool("SectionView", false));

        // well it is not visibility automation but a good place nevertheless
        this->ShowGrid.setValue(hGrp->GetBool("ShowGrid", false));
        this->GridSize.setValue(Base::Quantity::parse(QString::fromLatin1(hGrp->GetGroup("GridSize")->GetASCII("Hist0", "10.0").c_str())).getValue());
        this->GridSnap.setValue(hGrp->GetBool("GridSnap", false));
        this->Autoconstraints.setValue(hGrp->GetBool("AutoConstraints", true));
        this->AvoidRedundant.setValue(hGrp->GetBool("AvoidRedundantAutoconstraints", true));
        this->GridAutoSize.setValue(false); //Grid size is managed by this class
    }

    sPixmap = "Sketcher_Sketch";
    LineColor.setValue(1,1,1);
    PointColor.setValue(1,1,1);
    PointSize.setValue(4);

    zCross=0.001f;
    zEdit=0.001f;
    zInfo=0.004f;
    zLowLines=0.005f;
    //zLines=0.005f;    // ZLines removed in favour of 3 height groups intended for NormalLines, ConstructionLines, ExternalLines
    zMidLines=0.006f;
    zHighLines=0.007f;  // Lines that are somehow selected to be in the high position (higher than other line categories)
    zHighLine=0.008f;   // highlighted line (of any group)
    zConstr=0.009f; // constraint not construction
    //zPoints=0.010f;
    zLowPoints = 0.010f;
    zHighPoints = 0.011f;
    zHighlight=0.012f;
    zText=0.012f;


    xInit=0;
    yInit=0;
    relative=false;

    unsigned long color;
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    // edge color
    App::Color edgeColor = LineColor.getValue();
    color = (unsigned long)(edgeColor.getPackedValue());
    color = hGrp->GetUnsigned("SketchEdgeColor", color);
    edgeColor.setPackedValue((uint32_t)color);
    LineColor.setValue(edgeColor);

    // vertex color
    App::Color vertexColor = PointColor.getValue();
    color = (unsigned long)(vertexColor.getPackedValue());
    color = hGrp->GetUnsigned("SketchVertexColor", color);
    vertexColor.setPackedValue((uint32_t)color);
    PointColor.setValue(vertexColor);

    //rubberband selection
    rubberband = new Gui::Rubberband();

    // Status message states:


    subscribeToParameters();
}

ViewProviderSketch::~ViewProviderSketch()
{
    delete rubberband;
    unsubscribeToParameters();
}

void ViewProviderSketch::slotUndoDocument(const Gui::Document& /*doc*/)
{
    // Note 1: this slot is only operative during edit mode (see signal connection/disconnection)
    // Note 2: ViewProviderSketch::UpdateData does not generate updates during undo/redo
    //         transactions as mid-transaction data may not be in a valid state (e.g. constraints
    //         may reference invalid geometry). However undo/redo notify SketchObject after the undo/redo
    //         and before this slot is called.
    // Note 3: Note that recomputes are no longer inhibited during the call to this slot.
    forceUpdateData();
}

void ViewProviderSketch::slotRedoDocument(const Gui::Document& /*doc*/)
{
    // Note 1: this slot is only operative during edit mode (see signal connection/disconnection)
    // Note 2: ViewProviderSketch::UpdateData does not generate updates during undo/redo
    //         transactions as mid-transaction data may not be in a valid state (e.g. constraints
    //         may reference invalid geometry). However undo/redo notify SketchObject after the undo/redo
    //         and before this slot is called.
    // Note 3: Note that recomputes are no longer inhibited during the call to this slot.
    forceUpdateData();
}

void ViewProviderSketch::forceUpdateData()
{
    if(!getSketchObject()->noRecomputes) { // the sketch was already solved in SketchObject in onUndoRedoFinished
        Gui::Command::updateActive();
    }
}

// handler management ***************************************************************
void ViewProviderSketch::activateHandler(DrawSketchHandler *newHandler)
{
    assert(edit);
    assert(edit->sketchHandler == 0);
    edit->sketchHandler = newHandler;
    Mode = STATUS_SKETCH_UseHandler;
    edit->sketchHandler->sketchgui = this;
    edit->sketchHandler->activated(this);

    // make sure receiver has focus so immediately pressing Escape will be handled by
    // ViewProviderSketch::keyPressed() and dismiss the active handler, and not the entire
    // sketcher editor
    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    mdi->setFocus();
}

void ViewProviderSketch::deactivateHandler()
{
    assert(edit);
    if(edit->sketchHandler != 0){
        std::vector<Base::Vector2d> editCurve;
        editCurve.clear();
        drawEdit(editCurve); // erase any line
        resetPositionText();
        edit->sketchHandler->deactivated(this);
        edit->sketchHandler->unsetCursor();
        delete(edit->sketchHandler);
    }
    edit->sketchHandler = 0;
    Mode = STATUS_NONE;
}

/// removes the active handler
void ViewProviderSketch::purgeHandler(void)
{
    deactivateHandler();
    Gui::Selection().clearSelection();

    // ensure that we are in sketch only selection mode
    Gui::MDIView *mdi = Gui::Application::Instance->editDocument()->getActiveView();
    Gui::View3DInventorViewer *viewer;
    viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(false);
}

void ViewProviderSketch::setAxisPickStyle(bool on)
{
    assert(edit);
    if (on)
        edit->pickStyleAxes->style = SoPickStyle::SHAPE;
    else
        edit->pickStyleAxes->style = SoPickStyle::UNPICKABLE;
}

// **********************************************************************************

bool ViewProviderSketch::keyPressed(bool pressed, int key)
{
    switch (key)
    {
    case SoKeyboardEvent::ESCAPE:
        {
            // make the handler quit but not the edit mode
            if (edit && edit->sketchHandler) {
                if (!pressed)
                    edit->sketchHandler->quit();
                return true;
            }
            if (edit && (edit->DragConstraintSet.empty() == false)) {
                if (!pressed) {
                    edit->DragConstraintSet.clear();
                }
                return true;
            }
            if (edit && edit->DragCurve >= 0) {
                if (!pressed) {
                    getSketchObject()->movePoint(edit->DragCurve, Sketcher::PointPos::none, Base::Vector3d(0,0,0), true);
                    edit->DragCurve = -1;
                    resetPositionText();
                    Mode = STATUS_NONE;
                }
                return true;
            }
            if (edit && edit->DragPoint >= 0) {
                if (!pressed) {
                    int GeoId;
                    Sketcher::PointPos PosId;
                    getSketchObject()->getGeoVertexIndex(edit->DragPoint, GeoId, PosId);
                    getSketchObject()->movePoint(GeoId, PosId, Base::Vector3d(0,0,0), true);
                    edit->DragPoint = -1;
                    resetPositionText();
                    Mode = STATUS_NONE;
                }
                return true;
            }
            if (edit) {
                // #0001479: 'Escape' key dismissing dialog cancels Sketch editing
                // If we receive a button release event but not a press event before
                // then ignore this one.
                if (!pressed && !edit->buttonPress)
                    return true;
                edit->buttonPress = pressed;

                // More control over Sketcher edit mode Esc key behavior
                // https://forum.freecadweb.org/viewtopic.php?f=3&t=42207
                return edit->handleEscapeButton;
            }
            return false;
        }
    default:
        {
            if (edit && edit->sketchHandler)
                edit->sketchHandler->registerPressedKey(pressed,key);
        }
    }

    return true; // handle all other key events
}

void ViewProviderSketch::snapToGrid(double &x, double &y)
{
    if (GridSnap.getValue() && ShowGrid.getValue()) {
        // Snap Tolerance in pixels
        const double snapTol = GridSize.getValue() / 5;

        double tmpX = x, tmpY = y;

        // Find Nearest Snap points
        tmpX = tmpX / GridSize.getValue();
        tmpX = tmpX < 0.0 ? ceil(tmpX - 0.5) : floor(tmpX + 0.5);
        tmpX *= GridSize.getValue();

        tmpY = tmpY / GridSize.getValue();
        tmpY = tmpY < 0.0 ? ceil(tmpY - 0.5) : floor(tmpY + 0.5);
        tmpY *= GridSize.getValue();

        // Check if x within snap tolerance
        if (x < tmpX + snapTol && x > tmpX - snapTol)
            x = tmpX; // Snap X Mouse Position

         // Check if y within snap tolerance
        if (y < tmpY + snapTol && y > tmpY - snapTol)
            y = tmpY; // Snap Y Mouse Position
    }
}

void ViewProviderSketch::getProjectingLine(const SbVec2s& pnt, const Gui::View3DInventorViewer *viewer, SbLine& line) const
{
    const SbViewportRegion& vp = viewer->getSoRenderManager()->getViewportRegion();

    short x,y; pnt.getValue(x,y);
    SbVec2f siz = vp.getViewportSize();
    float dX, dY; siz.getValue(dX, dY);

    float fRatio = vp.getViewportAspectRatio();
    float pX = (float)x / float(vp.getViewportSizePixels()[0]);
    float pY = (float)y / float(vp.getViewportSizePixels()[1]);

    // now calculate the real points respecting aspect ratio information
    //
    if (fRatio > 1.0f) {
        pX = (pX - 0.5f*dX) * fRatio + 0.5f*dX;
    }
    else if (fRatio < 1.0f) {
        pY = (pY - 0.5f*dY) / fRatio + 0.5f*dY;
    }

    SoCamera* pCam = viewer->getSoRenderManager()->getCamera();
    if (!pCam) return;
    SbViewVolume  vol = pCam->getViewVolume();

    vol.projectPointToLine(SbVec2f(pX,pY), line);
}

Base::Placement ViewProviderSketch::getEditingPlacement() const {
    auto doc = Gui::Application::Instance->editDocument();
    if(!doc || doc->getInEdit()!=this)
        return getSketchObject()->globalPlacement();

    // TODO: won't work if there is scale. Hmm... what to do...
    return Base::Placement(doc->getEditingTransform());
}

void ViewProviderSketch::getCoordsOnSketchPlane(double &u, double &v,const SbVec3f &point, const SbVec3f &normal)
{
    // Plane form
    Base::Vector3d R0(0,0,0),RN(0,0,1),RX(1,0,0),RY(0,1,0);

    // move to position of Sketch
    Base::Placement Plz = getEditingPlacement();
    R0 = Plz.getPosition() ;
    Base::Rotation tmp(Plz.getRotation());
    tmp.multVec(RN,RN);
    tmp.multVec(RX,RX);
    tmp.multVec(RY,RY);
    Plz.setRotation(tmp);

    // line
    Base::Vector3d R1(point[0],point[1],point[2]),RA(normal[0],normal[1],normal[2]);
    if (fabs(RN*RA) < FLT_EPSILON)
        throw Base::DivisionByZeroError("View direction is parallel to sketch plane");
    // intersection point on plane
    Base::Vector3d S = R1 + ((RN * (R0-R1))/(RN*RA))*RA;

    // distance to x Axle of the sketch
    S.TransformToCoordinateSystem(R0,RX,RY);

    u = S.x;
    v = S.y;
}

bool ViewProviderSketch::mouseButtonPressed(int Button, bool pressed, const SbVec2s &cursorPos,
                                            const Gui::View3DInventorViewer *viewer)
{
    assert(edit);

    // Calculate 3d point to the mouse position
    SbLine line;
    getProjectingLine(cursorPos, viewer, line);
    SbVec3f point = line.getPosition();
    SbVec3f normal = line.getDirection();

    // use scoped_ptr to make sure that instance gets deleted in all cases
    boost::scoped_ptr<SoPickedPoint> pp(this->getPointOnRay(cursorPos, viewer));

    // Radius maximum to allow double click event
    const int dblClickRadius = 5;

    double x,y;
    SbVec3f pos = point;
    if (pp) {
        const SoDetail *detail = pp->getDetail();
        if (detail && detail->getTypeId() == SoPointDetail::getClassTypeId()) {
            pos = pp->getPoint();
        }
    }

    try {
        getCoordsOnSketchPlane(x,y,pos,normal);
        snapToGrid(x, y);
    }
    catch (const Base::DivisionByZeroError&) {
        return false;
    }

    // Left Mouse button ****************************************************
    if (Button == 1) {
        if (pressed) {
            // Do things depending on the mode of the user interaction
            switch (Mode) {
                case STATUS_NONE:{
                    bool done=false;
                    if (edit->PreselectPoint != -1) {
                        //Base::Console().Log("start dragging, point:%d\n",this->DragPoint);
                        Mode = STATUS_SELECT_Point;
                        done = true;
                    } else if (edit->PreselectCurve != -1) {
                        //Base::Console().Log("start dragging, point:%d\n",this->DragPoint);
                        Mode = STATUS_SELECT_Edge;
                        done = true;
                    } else if (edit->PreselectCross != -1) {
                        //Base::Console().Log("start dragging, point:%d\n",this->DragPoint);
                        Mode = STATUS_SELECT_Cross;
                        done = true;
                    } else if (edit->PreselectConstraintSet.empty() != true) {
                        //Base::Console().Log("start dragging, point:%d\n",this->DragPoint);
                        Mode = STATUS_SELECT_Constraint;
                        done = true;
                    }

                    // Double click events variables
                    float dci = (float) QApplication::doubleClickInterval()/1000.0f;

                    if (done &&
                        SbVec2f(cursorPos - prvClickPos).length() <  dblClickRadius &&
                        (SbTime::getTimeOfDay() - prvClickTime).getValue() < dci) {

                        // Double Click Event Occurred
                        editDoubleClicked();
                        // Reset Double Click Static Variables
                        prvClickTime = SbTime();
                        prvClickPos = SbVec2s(-16000,-16000); //certainly far away from any clickable place, to avoid re-trigger of double-click if next click happens fast.

                        Mode = STATUS_NONE;
                    } else {
                        prvClickTime = SbTime::getTimeOfDay();
                        prvClickPos = cursorPos;
                        prvCursorPos = cursorPos;
                        newCursorPos = cursorPos;
                        if (!done)
                            Mode = STATUS_SKETCH_StartRubberBand;
                    }

                    return done;
                }
                case STATUS_SKETCH_UseHandler:
                    return edit->sketchHandler->pressButton(Base::Vector2d(x,y));
                default:
                    return false;
            }
        } else { // Button 1 released
            // Do things depending on the mode of the user interaction
            switch (Mode) {
                case STATUS_SELECT_Point:
                    if (pp) {
                        //Base::Console().Log("Select Point:%d\n",this->DragPoint);
                        // Do selection
                        std::stringstream ss;
                        ss << "Vertex" << edit->PreselectPoint + 1;

#define SEL_PARAMS editDocName.c_str(),editObjName.c_str(),\
                   (editSubName+ss.str()).c_str()
                        if (Gui::Selection().isSelected(SEL_PARAMS) ) {
                             Gui::Selection().rmvSelection(SEL_PARAMS);
                        } else {
                            Gui::Selection().addSelection2(SEL_PARAMS
                                                         ,pp->getPoint()[0]
                                                         ,pp->getPoint()[1]
                                                         ,pp->getPoint()[2]);
                            this->edit->DragPoint = -1;
                            this->edit->DragCurve = -1;
                            this->edit->DragConstraintSet.clear();
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SELECT_Edge:
                    if (pp) {
                        //Base::Console().Log("Select Point:%d\n",this->DragPoint);
                        std::stringstream ss;
                        if (edit->PreselectCurve >= 0)
                            ss << "Edge" << edit->PreselectCurve + 1;
                        else // external geometry
                            ss << "ExternalEdge" << -edit->PreselectCurve - 2;

                        // If edge already selected move from selection
                        if (Gui::Selection().isSelected(SEL_PARAMS) ) {
                            Gui::Selection().rmvSelection(SEL_PARAMS);
                        } else {
                            // Add edge to the selection
                            Gui::Selection().addSelection2(SEL_PARAMS
                                                         ,pp->getPoint()[0]
                                                         ,pp->getPoint()[1]
                                                         ,pp->getPoint()[2]);
                            this->edit->DragPoint = -1;
                            this->edit->DragCurve = -1;
                            this->edit->DragConstraintSet.clear();
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SELECT_Cross:
                    if (pp) {
                        //Base::Console().Log("Select Point:%d\n",this->DragPoint);
                        std::stringstream ss;
                        switch(edit->PreselectCross){
                            case 0: ss << "RootPoint" ; break;
                            case 1: ss << "H_Axis"    ; break;
                            case 2: ss << "V_Axis"    ; break;
                        }

                        // If cross already selected move from selection
                        if (Gui::Selection().isSelected(SEL_PARAMS) ) {
                            Gui::Selection().rmvSelection(SEL_PARAMS);
                        } else {
                            // Add cross to the selection
                            Gui::Selection().addSelection2(SEL_PARAMS
                                                         ,pp->getPoint()[0]
                                                         ,pp->getPoint()[1]
                                                         ,pp->getPoint()[2]);
                            this->edit->DragPoint = -1;
                            this->edit->DragCurve = -1;
                            this->edit->DragConstraintSet.clear();
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SELECT_Constraint:
                    if (pp) {
                        auto sels = edit->PreselectConstraintSet;
                        for(int id : sels) {
                            std::stringstream ss;
                            ss << Sketcher::PropertyConstraintList::getConstraintName(id);

                            // If the constraint already selected remove
                            if (Gui::Selection().isSelected(SEL_PARAMS) ) {
                                Gui::Selection().rmvSelection(SEL_PARAMS);
                            } else {
                                // Add constraint to current selection
                                Gui::Selection().addSelection2(SEL_PARAMS
                                                             ,pp->getPoint()[0]
                                                             ,pp->getPoint()[1]
                                                             ,pp->getPoint()[2]);
                                this->edit->DragPoint = -1;
                                this->edit->DragCurve = -1;
                                this->edit->DragConstraintSet.clear();
                            }
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_DragPoint:
                    if (edit->DragPoint != -1) {
                        int GeoId;
                        Sketcher::PointPos PosId;
                        getSketchObject()->getGeoVertexIndex(edit->DragPoint, GeoId, PosId);
                        if (GeoId != Sketcher::GeoEnum::GeoUndef && PosId != Sketcher::PointPos::none) {
                            getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Drag Point"));
                            try {
                                Gui::cmdAppObjectArgs(getObject(), "movePoint(%i,%i,App.Vector(%f,%f,0),%i)"
                                        ,GeoId, static_cast<int>(PosId), x-xInit, y-yInit, 0);
                                getDocument()->commitCommand();

                                tryAutoRecomputeIfNotSolve(getSketchObject());
                            }
                            catch (const Base::Exception& e) {
                                getDocument()->abortCommand();
                                Base::Console().Error("Drag point: %s\n", e.what());
                            }
                        }
                        setPreselectPoint(edit->DragPoint);
                        edit->DragPoint = -1;
                        //updateColor();
                    }
                    resetPositionText();
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_DragCurve:
                    if (edit->DragCurve != -1) {
                        const Part::Geometry *geo = getSketchObject()->getGeometry(edit->DragCurve);
                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId() ||
                            geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                            geo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                            geo->getTypeId() == Part::GeomEllipse::getClassTypeId()||
                            geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()||
                            geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()||
                            geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()||
                            geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                            getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Drag Curve"));

                            auto geo = getSketchObject()->getGeometry(edit->DragCurve);
                            auto gf = GeometryFacade::getFacade(geo);

                            Base::Vector3d vec(x-xInit,y-yInit,0);

                            // BSpline weights have a radius corresponding to the weight value
                            // However, in order for them proportional to the B-Spline size,
                            // the scenograph has a size scalefactor times the weight
                            // This code normalizes the information sent to the solver.
                            if(gf->getInternalType() == InternalType::BSplineControlPoint) {
                                auto circle = static_cast<const Part::GeomCircle *>(geo);
                                Base::Vector3d center = circle->getCenter();

                                Base::Vector3d dir = vec - center;

                                double scalefactor = 1.0;

                                if(circle->hasExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()))
                                {
                                    auto vpext = std::static_pointer_cast<const SketcherGui::ViewProviderSketchGeometryExtension>(
                                                    circle->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()).lock());

                                    scalefactor = vpext->getRepresentationFactor();
                                }

                                vec = center + dir / scalefactor;
                            }

                            try {
                                Gui::cmdAppObjectArgs(getObject(), "movePoint(%i,%i,App.Vector(%f,%f,0),%i)"
                                        ,edit->DragCurve, static_cast<int>(Sketcher::PointPos::none), vec.x, vec.y, relative ? 1 : 0);
                                getDocument()->commitCommand();

                                tryAutoRecomputeIfNotSolve(getSketchObject());
                            }
                            catch (const Base::Exception& e) {
                                getDocument()->abortCommand();
                                Base::Console().Error("Drag curve: %s\n", e.what());
                            }
                        }
                        edit->PreselectCurve = edit->DragCurve;
                        edit->DragCurve = -1;
                        //updateColor();
                    }
                    resetPositionText();
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_DragConstraint:
                    if (edit->DragConstraintSet.empty() == false) {
                        getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Drag Constraint"));
                        auto idset = edit->DragConstraintSet;
                        for(int id : idset) {
                            moveConstraint(id, Base::Vector2d(x, y));
                            //updateColor();
                        }
                        edit->PreselectConstraintSet = edit->DragConstraintSet;
                        edit->DragConstraintSet.clear();
                        getDocument()->commitCommand();
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_StartRubberBand: // a single click happened, so clear selection
                    Mode = STATUS_NONE;
                    Gui::Selection().clearSelection();
                    return true;
                case STATUS_SKETCH_UseRubberBand:
                    doBoxSelection(prvCursorPos, cursorPos, viewer);
                    rubberband->setWorking(false);

                    // a redraw is required in order to clear the rubberband
                    draw(true,false);
                    const_cast<Gui::View3DInventorViewer*>(viewer)->redraw();
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_UseHandler: {
                    return edit->sketchHandler->releaseButton(Base::Vector2d(x,y));
                }
                case STATUS_NONE:
                default:
                    return false;
            }
        }
    }
    // Right mouse button ****************************************************
    else if (Button == 2) {
        if (!pressed) {
            switch (Mode) {
                case STATUS_SKETCH_UseHandler:
                    // make the handler quit
                    edit->sketchHandler->quit();
                    return true;
                case STATUS_NONE:
                    {
                        // A right click shouldn't change the Edit Mode
                        if (edit->PreselectPoint != -1) {
                            return true;
                        } else if (edit->PreselectCurve != -1) {
                            return true;
                        } else if (edit->PreselectConstraintSet.empty() != true) {
                            return true;
                        } else {
                            Gui::MenuItem geom;
                            geom.setCommand("Sketcher geoms");
                            geom << "Sketcher_CreatePoint"
                                  << "Sketcher_CreateArc"
                                  << "Sketcher_Create3PointArc"
                                  << "Sketcher_CreateCircle"
                                  << "Sketcher_Create3PointCircle"
                                  << "Sketcher_CreateLine"
                                  << "Sketcher_CreatePolyline"
                                  << "Sketcher_CreateRectangle"
                                  << "Sketcher_CreateHexagon"
                                  << "Sketcher_CreateFillet"
                                  << "Sketcher_CreatePointFillet"
                                  << "Sketcher_Trimming"
                                  << "Sketcher_Extend"
                                  << "Sketcher_External"
                                  << "Sketcher_ToggleConstruction"
                                /*<< "Sketcher_CreateText"*/
                                /*<< "Sketcher_CreateDraftLine"*/
                                  << "Separator";

                            Gui::Application::Instance->setupContextMenu("View", &geom);
                            //Create the Context Menu using the Main View Qt Widget
                            QMenu contextMenu(viewer->getGLWidget());
                            Gui::MenuManager::getInstance()->setupContextMenu(&geom, contextMenu);
                            contextMenu.exec(QCursor::pos());

                            return true;
                        }
                    }
                case STATUS_SELECT_Point:
                    break;
                case STATUS_SELECT_Edge:
                    {
                        Gui::MenuItem geom;
                        geom.setCommand("Sketcher constraints");
                        geom << "Sketcher_ConstrainVertical"
                        << "Sketcher_ConstrainHorizontal";

                        // Gets a selection vector
                        std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

                        bool rightClickOnSelectedLine = false;

                        /*
                         * Add Multiple Line Constraints to the menu
                         */
                        // only one sketch with its subelements are allowed to be selected
                        if (selection.size() == 1) {
                            // get the needed lists and objects
                            const std::vector<std::string> &SubNames = selection[0].getSubNames();

                            // Two Objects are selected
                            if (SubNames.size() == 2) {
                                // go through the selected subelements
                                for (std::vector<std::string>::const_iterator it=SubNames.begin();
                                     it!=SubNames.end();++it) {

                                    // If the object selected is of type edge
                                    if (it->size() > 4 && it->substr(0,4) == "Edge") {
                                        // Get the index of the object selected
                                        int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
                                        if (edit->PreselectCurve == GeoId)
                                            rightClickOnSelectedLine = true;
                                    } else {
                                        // The selection is not exclusively edges
                                        rightClickOnSelectedLine = false;
                                    }
                                } // End of Iteration
                            }
                        }

                        if (rightClickOnSelectedLine) {
                            geom << "Sketcher_ConstrainParallel"
                                  << "Sketcher_ConstrainPerpendicular";
                        }

                        Gui::Application::Instance->setupContextMenu("View", &geom);
                        //Create the Context Menu using the Main View Qt Widget
                        QMenu contextMenu(viewer->getGLWidget());
                        Gui::MenuManager::getInstance()->setupContextMenu(&geom, contextMenu);
                        contextMenu.exec(QCursor::pos());

                        return true;
                    }
                case STATUS_SELECT_Cross:
                case STATUS_SELECT_Constraint:
                case STATUS_SKETCH_DragPoint:
                case STATUS_SKETCH_DragCurve:
                case STATUS_SKETCH_DragConstraint:
                case STATUS_SKETCH_StartRubberBand:
                case STATUS_SKETCH_UseRubberBand:
                    break;
            }
        }
    }

    return false;
}

void ViewProviderSketch::editDoubleClicked(void)
{
    if (edit->PreselectPoint != -1) {
        Base::Console().Log("double click point:%d\n",edit->PreselectPoint);
    }
    else if (edit->PreselectCurve != -1) {
        Base::Console().Log("double click edge:%d\n",edit->PreselectCurve);
    }
    else if (edit->PreselectCross != -1) {
        Base::Console().Log("double click cross:%d\n",edit->PreselectCross);
    }
    else if (edit->PreselectConstraintSet.empty() != true) {
        // Find the constraint
        const std::vector<Sketcher::Constraint *> &constrlist = getSketchObject()->Constraints.getValues();

        auto sels = edit->PreselectConstraintSet;
        for(int id : sels) {

            Constraint *Constr = constrlist[id];

            // if its the right constraint
            if (Constr->isDimensional()) {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Modify sketch constraints"));
                EditDatumDialog editDatumDialog(this, id);
                editDatumDialog.exec();
            }
        }
    }
}

bool ViewProviderSketch::mouseMove(const SbVec2s &cursorPos, Gui::View3DInventorViewer *viewer)
{
    // maximum radius for mouse moves when selecting a geometry before switching to drag mode
    const int dragIgnoredDistance = 3;

    if (!edit)
        return false;

    // ignore small moves after selection
    switch (Mode) {
        case STATUS_SELECT_Point:
        case STATUS_SELECT_Edge:
        case STATUS_SELECT_Constraint:
        case STATUS_SKETCH_StartRubberBand:
            short dx, dy;
            (cursorPos - prvCursorPos).getValue(dx, dy);
            if(std::abs(dx) < dragIgnoredDistance && std::abs(dy) < dragIgnoredDistance)
                return false;
        default:
            break;
    }

    // Calculate 3d point to the mouse position
    SbLine line;
    getProjectingLine(cursorPos, viewer, line);

    double x,y;
    try {
        getCoordsOnSketchPlane(x,y,line.getPosition(),line.getDirection());
        snapToGrid(x, y);
    }
    catch (const Base::DivisionByZeroError&) {
        return false;
    }

    bool preselectChanged = false;
    if (Mode != STATUS_SELECT_Point &&
        Mode != STATUS_SELECT_Edge &&
        Mode != STATUS_SELECT_Constraint &&
        Mode != STATUS_SKETCH_DragPoint &&
        Mode != STATUS_SKETCH_DragCurve &&
        Mode != STATUS_SKETCH_DragConstraint &&
        Mode != STATUS_SKETCH_UseRubberBand) {

        boost::scoped_ptr<SoPickedPoint> pp(this->getPointOnRay(cursorPos, viewer));
        preselectChanged = detectPreselection(pp.get(), viewer, cursorPos);
    }

    switch (Mode) {
        case STATUS_NONE:
            if (preselectChanged) {
                this->drawConstraintIcons();
                this->updateColor();
                return true;
            }
            return false;
        case STATUS_SELECT_Point:
            if (!getSolvedSketch().hasConflicts() &&
                edit->PreselectPoint != -1 && edit->DragPoint != edit->PreselectPoint) {
                Mode = STATUS_SKETCH_DragPoint;
                edit->DragPoint = edit->PreselectPoint;
                int GeoId;
                Sketcher::PointPos PosId;
                getSketchObject()->getGeoVertexIndex(edit->DragPoint, GeoId, PosId);
                if (GeoId != Sketcher::GeoEnum::GeoUndef && PosId != Sketcher::PointPos::none) {
                    getSketchObject()->initTemporaryMove(GeoId, PosId, false);
                    relative = false;
                    xInit = 0;
                    yInit = 0;
                }
            } else {
                Mode = STATUS_NONE;
            }
            resetPreselectPoint();
            edit->PreselectCurve = -1;
            edit->PreselectCross = -1;
            edit->PreselectConstraintSet.clear();
            return true;
        case STATUS_SELECT_Edge:
            if (!getSolvedSketch().hasConflicts() &&
                edit->PreselectCurve != -1 && edit->DragCurve != edit->PreselectCurve) {
                Mode = STATUS_SKETCH_DragCurve;
                edit->DragCurve = edit->PreselectCurve;
                const Part::Geometry *geo = getSketchObject()->getGeometry(edit->DragCurve);

                // BSpline Control points are edge draggable only if their radius is movable
                // This is because dragging gives unwanted cosmetic results due to the scale ratio.
                // This is an heuristic as it does not check all indirect routes.
                if(GeometryFacade::isInternalType(geo, InternalType::BSplineControlPoint)) {
                    if(geo->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {
                        auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                                        geo->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

                        // Edge parameters are Independent, so weight won't move
                        if(solvext->getEdge()==Sketcher::SolverGeometryExtension::Independent) {
                            Mode = STATUS_NONE;
                            return false;
                        }

                        // The B-Spline is constrained to be non-rational (equal weights), moving produces a bad effect
                        // because OCCT will normalize the values of the weights.
                        auto grp = getSolvedSketch().getDependencyGroup(edit->DragCurve, Sketcher::PointPos::none);

                        int bsplinegeoid = -1;

                        std::vector<int> polegeoids;

                        for( auto c : getSketchObject()->Constraints.getValues()) {
                            if( c->Type == Sketcher::InternalAlignment &&
                                c->AlignmentType == BSplineControlPoint &&
                                c->First == edit->DragCurve ) {

                                bsplinegeoid = c->Second;
                                break;
                            }
                        }

                        if(bsplinegeoid == -1) {
                            Mode = STATUS_NONE;
                            return false;
                        }

                        for( auto c : getSketchObject()->Constraints.getValues()) {
                            if( c->Type == Sketcher::InternalAlignment &&
                                c->AlignmentType == BSplineControlPoint &&
                                c->Second == bsplinegeoid ) {

                                polegeoids.push_back(c->First);
                            }
                        }

                        bool allingroup = true;

                        for( auto polegeoid : polegeoids ) {
                            std::pair< int, Sketcher::PointPos > thispole = std::make_pair(polegeoid,Sketcher::PointPos::none);

                            if(grp.find(thispole) == grp.end()) // not found
                                allingroup  = false;
                        }

                        if(allingroup) { // it is constrained to be non-rational
                            Mode = STATUS_NONE;
                            return false;
                        }

                    }

                }

                if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId() ||
                    geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                    relative = true;
                    //xInit = x;
                    //yInit = y;
                    // Since the cursor moved from where it was clicked, and this is a relative move,
                    // calculate the click position and use it as initial point.
                    SbLine line2;
                    getProjectingLine(prvCursorPos, viewer, line2);
                    getCoordsOnSketchPlane(xInit,yInit,line2.getPosition(),line2.getDirection());
                    snapToGrid(xInit, yInit);
                } else {
                    relative = false;
                    xInit = 0;
                    yInit = 0;
                }

                getSketchObject()->initTemporaryMove(edit->DragCurve, Sketcher::PointPos::none, false);

            } else {
                Mode = STATUS_NONE;
            }
            resetPreselectPoint();
            edit->PreselectCurve = -1;
            edit->PreselectCross = -1;
            edit->PreselectConstraintSet.clear();
            return true;
        case STATUS_SELECT_Constraint:
            Mode = STATUS_SKETCH_DragConstraint;
            edit->DragConstraintSet = edit->PreselectConstraintSet;
            resetPreselectPoint();
            edit->PreselectCurve = -1;
            edit->PreselectCross = -1;
            edit->PreselectConstraintSet.clear();
            return true;
        case STATUS_SKETCH_DragPoint:
            if (edit->DragPoint != -1) {
                //Base::Console().Log("Drag Point:%d\n",edit->DragPoint);
                int GeoId;
                Sketcher::PointPos PosId;
                getSketchObject()->getGeoVertexIndex(edit->DragPoint, GeoId, PosId);
                Base::Vector3d vec(x,y,0);
                if (GeoId != Sketcher::GeoEnum::GeoUndef && PosId != Sketcher::PointPos::none) {
                    if (getSketchObject()->moveTemporaryPoint(GeoId, PosId, vec, false) == 0) {
                        setPositionText(Base::Vector2d(x,y));
                        draw(true,false);
                    }
                }
            }
            return true;
        case STATUS_SKETCH_DragCurve:
            if (edit->DragCurve != -1) {
                auto geo = getSketchObject()->getGeometry(edit->DragCurve);
                auto gf = GeometryFacade::getFacade(geo);

                Base::Vector3d vec(x-xInit,y-yInit,0);

                // BSpline weights have a radius corresponding to the weight value
                // However, in order for them proportional to the B-Spline size,
                // the scenograph has a size scalefactor times the weight
                // This code normalizes the information sent to the solver.
                if(gf->getInternalType() == InternalType::BSplineControlPoint) {
                    auto circle = static_cast<const Part::GeomCircle *>(geo);
                    Base::Vector3d center = circle->getCenter();

                    Base::Vector3d dir = vec - center;

                    double scalefactor = 1.0;

                    if(circle->hasExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()))
                    {
                        auto vpext = std::static_pointer_cast<const SketcherGui::ViewProviderSketchGeometryExtension>(
                                        circle->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()).lock());

                        scalefactor = vpext->getRepresentationFactor();
                    }

                    vec = center + dir / scalefactor;
                }

                if (getSketchObject()->moveTemporaryPoint(edit->DragCurve, Sketcher::PointPos::none, vec, relative) == 0) {
                    setPositionText(Base::Vector2d(x,y));
                    draw(true,false);
                }
            }
            return true;
        case STATUS_SKETCH_DragConstraint:
            if (edit->DragConstraintSet.empty() == false) {
                auto idset = edit->DragConstraintSet;
                for(int id : idset)
                    moveConstraint(id, Base::Vector2d(x,y));
            }
            return true;
        case STATUS_SKETCH_UseHandler:
            edit->sketchHandler->mouseMove(Base::Vector2d(x,y));
            if (preselectChanged) {
                this->drawConstraintIcons();
                this->updateColor();
            }
            return true;
        case STATUS_SKETCH_StartRubberBand: {
            Mode = STATUS_SKETCH_UseRubberBand;
            rubberband->setWorking(true);
            return true;
        }
        case STATUS_SKETCH_UseRubberBand: {
            // Here we must use the device-pixel-ratio to compute the correct y coordinate (#0003130)
            qreal dpr = viewer->getGLWidget()->devicePixelRatioF();
            newCursorPos = cursorPos;
            rubberband->setCoords(prvCursorPos.getValue()[0],
                       viewer->getGLWidget()->height()*dpr - prvCursorPos.getValue()[1],
                       newCursorPos.getValue()[0],
                       viewer->getGLWidget()->height()*dpr - newCursorPos.getValue()[1]);
            viewer->redraw();
            return true;
        }
        default:
            return false;
    }

    return false;
}

void ViewProviderSketch::moveConstraint(int constNum, const Base::Vector2d &toPos)
{
    // are we in edit?
    if (!edit)
        return;

    const std::vector<Sketcher::Constraint *> &constrlist = getSketchObject()->Constraints.getValues();
    Constraint *Constr = constrlist[constNum];

#ifdef _DEBUG
    int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;
    int extGeoCount = getSketchObject()->getExternalGeometryCount();
#endif

    // with memory allocation
    const std::vector<Part::Geometry *> geomlist = getSolvedSketch().extractGeometry(true, true);

#ifdef _DEBUG
    assert(int(geomlist.size()) == extGeoCount + intGeoCount);
    assert((Constr->First >= -extGeoCount && Constr->First < intGeoCount)
           || Constr->First != Constraint::GeoUndef);
#endif

    if (Constr->Type == Distance || Constr->Type == DistanceX || Constr->Type == DistanceY ||
        Constr->Type == Radius || Constr->Type == Diameter || Constr-> Type == Weight) {

        Base::Vector3d p1(0.,0.,0.), p2(0.,0.,0.);
        if (Constr->SecondPos != Sketcher::PointPos::none) { // point to point distance
            p1 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
            p2 = getSolvedSketch().getPoint(Constr->Second, Constr->SecondPos);
        } else if (Constr->Second != GeoEnum::GeoUndef) { // point to line distance
            p1 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
            const Part::Geometry *geo = GeoById(geomlist, Constr->Second);
            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);
                Base::Vector3d l2p1 = lineSeg->getStartPoint();
                Base::Vector3d l2p2 = lineSeg->getEndPoint();
                // calculate the projection of p1 onto line2
                p2.ProjectToLine(p1-l2p1, l2p2-l2p1);
                p2 += p1;
            } else
                return;
        } else if (Constr->FirstPos != Sketcher::PointPos::none) {
            p2 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
        } else if (Constr->First != GeoEnum::GeoUndef) {
            const Part::Geometry *geo = GeoById(geomlist, Constr->First);
            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);
                p1 = lineSeg->getStartPoint();
                p2 = lineSeg->getEndPoint();
            } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                double radius = arc->getRadius();
                Base::Vector3d center = arc->getCenter();
                p1 = center;

                double angle = Constr->LabelPosition;
                if (angle == 10) {
                    double startangle, endangle;
                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                    angle = (startangle + endangle)/2;
                }
                else {
                    Base::Vector3d tmpDir =  Base::Vector3d(toPos.x, toPos.y, 0) - p1;
                    angle = atan2(tmpDir.y, tmpDir.x);
                }

                if(Constr->Type == Sketcher::Diameter)
                    p1 = center - radius * Base::Vector3d(cos(angle),sin(angle),0.);

                p2 = center + radius * Base::Vector3d(cos(angle),sin(angle),0.);
            }
            else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo);
                double radius = circle->getRadius();
                Base::Vector3d center = circle->getCenter();
                p1 = center;

                Base::Vector3d tmpDir =  Base::Vector3d(toPos.x, toPos.y, 0) - p1;

                Base::Vector3d dir = radius * tmpDir.Normalize();

                if(Constr->Type == Sketcher::Diameter)
                    p1 = center - dir;

                if(Constr->Type == Sketcher::Weight) {

                    double scalefactor = 1.0;

                    if(circle->hasExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()))
                    {
                        auto vpext = std::static_pointer_cast<const SketcherGui::ViewProviderSketchGeometryExtension>(
                                        circle->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()).lock());

                        scalefactor = vpext->getRepresentationFactor();
                    }

                    p2 = center + dir * scalefactor;

                }
                else
                    p2 = center + dir;
            }
            else
                return;
        } else
            return;

        Base::Vector3d vec = Base::Vector3d(toPos.x, toPos.y, 0) - p2;

        Base::Vector3d dir;
        if (Constr->Type == Distance || Constr->Type == Radius || Constr->Type == Diameter || Constr->Type == Weight)
            dir = (p2-p1).Normalize();
        else if (Constr->Type == DistanceX)
            dir = Base::Vector3d( (p2.x - p1.x >= FLT_EPSILON) ? 1 : -1, 0, 0);
        else if (Constr->Type == DistanceY)
            dir = Base::Vector3d(0, (p2.y - p1.y >= FLT_EPSILON) ? 1 : -1, 0);

        if (Constr->Type == Radius || Constr->Type == Diameter || Constr->Type == Weight) {
            Constr->LabelDistance = vec.x * dir.x + vec.y * dir.y;
            Constr->LabelPosition = atan2(dir.y, dir.x);
        } else {
            Base::Vector3d normal(-dir.y,dir.x,0);
            Constr->LabelDistance = vec.x * normal.x + vec.y * normal.y;
            if (Constr->Type == Distance ||
                Constr->Type == DistanceX || Constr->Type == DistanceY) {
                vec = Base::Vector3d(toPos.x, toPos.y, 0) - (p2 + p1) / 2;
                Constr->LabelPosition = vec.x * dir.x + vec.y * dir.y;
            }
        }
    }
    else if (Constr->Type == Angle) {

        Base::Vector3d p0(0.,0.,0.);
        double factor = 0.5;
        if (Constr->Second != GeoEnum::GeoUndef) { // line to line angle
            Base::Vector3d dir1, dir2;
            if(Constr->Third == GeoEnum::GeoUndef) { //angle between two lines
                const Part::Geometry *geo1 = GeoById(geomlist, Constr->First);
                const Part::Geometry *geo2 = GeoById(geomlist, Constr->Second);
                if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                    geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                    return;
                const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment *>(geo1);
                const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment *>(geo2);

                bool flip1 = (Constr->FirstPos == Sketcher::PointPos::end);
                bool flip2 = (Constr->SecondPos == Sketcher::PointPos::end);
                dir1 = (flip1 ? -1. : 1.) * (lineSeg1->getEndPoint()-lineSeg1->getStartPoint());
                dir2 = (flip2 ? -1. : 1.) * (lineSeg2->getEndPoint()-lineSeg2->getStartPoint());
                Base::Vector3d pnt1 = flip1 ? lineSeg1->getEndPoint() : lineSeg1->getStartPoint();
                Base::Vector3d pnt2 = flip2 ? lineSeg2->getEndPoint() : lineSeg2->getStartPoint();

                // line-line intersection
                {
                    double det = dir1.x*dir2.y - dir1.y*dir2.x;
                    if ((det > 0 ? det : -det) < 1e-10)
                        return;// lines are parallel - constraint unmoveable (DeepSOIC: why?..)
                    double c1 = dir1.y*pnt1.x - dir1.x*pnt1.y;
                    double c2 = dir2.y*pnt2.x - dir2.x*pnt2.y;
                    double x = (dir1.x*c2 - dir2.x*c1)/det;
                    double y = (dir1.y*c2 - dir2.y*c1)/det;
                    // intersection point
                    p0 = Base::Vector3d(x,y,0);

                    Base::Vector3d vec = Base::Vector3d(toPos.x, toPos.y, 0) - p0;
                    factor = factor * Base::sgn<double>((dir1+dir2) * vec);
                }
            } else {//angle-via-point
                Base::Vector3d p = getSolvedSketch().getPoint(Constr->Third, Constr->ThirdPos);
                p0 = Base::Vector3d(p.x, p.y, 0);
                dir1 = getSolvedSketch().calculateNormalAtPoint(Constr->First, p.x, p.y);
                dir1.RotateZ(-M_PI/2);//convert to vector of tangency by rotating
                dir2 = getSolvedSketch().calculateNormalAtPoint(Constr->Second, p.x, p.y);
                dir2.RotateZ(-M_PI/2);

                Base::Vector3d vec = Base::Vector3d(toPos.x, toPos.y, 0) - p0;
                factor = factor * Base::sgn<double>((dir1+dir2) * vec);
            }

        } else if (Constr->First != GeoEnum::GeoUndef) { // line/arc angle
            const Part::Geometry *geo = GeoById(geomlist, Constr->First);
            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);
                p0 = (lineSeg->getEndPoint()+lineSeg->getStartPoint())/2;
            }
            else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                p0 = arc->getCenter();
            }
            else {
                return;
            }
        } else
            return;

        Base::Vector3d vec = Base::Vector3d(toPos.x, toPos.y, 0) - p0;
        Constr->LabelDistance = factor * vec.Length();
    }

    // delete the cloned objects
    for (std::vector<Part::Geometry *>::const_iterator it=geomlist.begin(); it != geomlist.end(); ++it)
        if (*it) delete *it;

    draw(true,false);
}

Base::Vector3d ViewProviderSketch::seekConstraintPosition(const Base::Vector3d &origPos,
                                                          const Base::Vector3d &norm,
                                                          const Base::Vector3d &dir, float step,
                                                          const SoNode *constraint)
{
    assert(edit);
    Gui::MDIView *mdi = Gui::Application::Instance->editViewOfNode(edit->EditRoot);
    if (!(mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())))
        return Base::Vector3d(0, 0, 0);
    Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

    SoRayPickAction rp(viewer->getSoRenderManager()->getViewportRegion());

    float scaled_step = step * getScaleFactor();

    int multiplier = 0;
    Base::Vector3d relPos, freePos;
    bool isConstraintAtPosition = true;
    while (isConstraintAtPosition && multiplier < 10) {
        // Calculate new position of constraint
        relPos = norm * 0.5f + dir * multiplier;
        freePos = origPos + relPos * scaled_step;

        rp.setRadius(0.1f);
        rp.setPickAll(true);
        rp.setRay(SbVec3f(freePos.x, freePos.y, -1.f), SbVec3f(0, 0, 1) );
        //problem
        rp.apply(edit->constrGroup); // We could narrow it down to just the SoGroup containing the constraints

        // returns a copy of the point
        SoPickedPoint *pp = rp.getPickedPoint();
        const SoPickedPointList ppl = rp.getPickedPointList();

        if (ppl.getLength() <= 1 && pp) {
            SoPath *path = pp->getPath();
            int length = path->getLength();
            SoNode *tailFather1 = path->getNode(length-2);
            SoNode *tailFather2 = path->getNode(length-3);

            // checking if a constraint is the same as the one selected
            if (tailFather1 == constraint || tailFather2 == constraint)
                isConstraintAtPosition = false;
        }
        else {
            isConstraintAtPosition = false;
        }

        multiplier *= -1; // search in both sides
        if (multiplier >= 0)
            multiplier++; // Increment the multiplier
    }
    if (multiplier == 10)
        relPos = norm * 0.5f; // no free position found
    return relPos * step;
}

bool ViewProviderSketch::isSelectable(void) const
{
    if (isEditing())
        return false;
    else
        return PartGui::ViewProvider2DObjectGrid::isSelectable();
}

void ViewProviderSketch::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // are we in edit?
    if (edit) {
        // ignore external object
        if(msg.Object.getObjectName().size() && msg.Object.getDocument()!=getObject()->getDocument())
            return;

        bool handled=false;
        if (Mode == STATUS_SKETCH_UseHandler) {
            App::AutoTransaction committer;
            handled = edit->sketchHandler->onSelectionChanged(msg);
        }
        if (handled)
            return;

        std::string temp;
        if (msg.Type == Gui::SelectionChanges::ClrSelection) {
            // if something selected in this object?
            if (edit->SelPointSet.size() > 0 || edit->SelCurvSet.size() > 0 || edit->SelConstraintSet.size() > 0) {
                // clear our selection and update the color of the viewed edges and points
                clearSelectPoints();
                edit->SelCurvSet.clear();
                edit->SelConstraintSet.clear();
                this->drawConstraintIcons();
                this->updateColor();
            }
        }
        else if (msg.Type == Gui::SelectionChanges::AddSelection) {
            // is it this object??
            if (strcmp(msg.pDocName,getSketchObject()->getDocument()->getName())==0
                && strcmp(msg.pObjectName,getSketchObject()->getNameInDocument())== 0) {
                if (msg.pSubName) {
                    std::string shapetype(msg.pSubName);
                    if (shapetype.size() > 4 && shapetype.substr(0,4) == "Edge") {
                        int GeoId = std::atoi(&shapetype[4]) - 1;
                        edit->SelCurvSet.insert(GeoId);
                        this->updateColor();
                    }
                    else if (shapetype.size() > 12 && shapetype.substr(0,12) == "ExternalEdge") {
                        int GeoId = std::atoi(&shapetype[12]) - 1;
                        GeoId = -GeoId - 3;
                        edit->SelCurvSet.insert(GeoId);
                        this->updateColor();
                    }
                    else if (shapetype.size() > 6 && shapetype.substr(0,6) == "Vertex") {
                        int VtId = std::atoi(&shapetype[6]) - 1;
                        addSelectPoint(VtId);
                        this->updateColor();
                    }
                    else if (shapetype == "RootPoint") {
                        addSelectPoint(Sketcher::GeoEnum::RtPnt);
                        this->updateColor();
                    }
                    else if (shapetype == "H_Axis") {
                        edit->SelCurvSet.insert(Sketcher::GeoEnum::HAxis);
                        this->updateColor();
                    }
                    else if (shapetype == "V_Axis") {
                        edit->SelCurvSet.insert(Sketcher::GeoEnum::VAxis);
                        this->updateColor();
                    }
                    else if (shapetype.size() > 10 && shapetype.substr(0,10) == "Constraint") {
                        int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(shapetype);
                        edit->SelConstraintSet.insert(ConstrId);
                        this->drawConstraintIcons();
                        this->updateColor();
                    }
                }
            }
        }
        else if (msg.Type == Gui::SelectionChanges::RmvSelection) {
            // Are there any objects selected
            if (edit->SelPointSet.size() > 0 || edit->SelCurvSet.size() > 0 || edit->SelConstraintSet.size() > 0) {
                // is it this object??
                if (strcmp(msg.pDocName,getSketchObject()->getDocument()->getName())==0
                    && strcmp(msg.pObjectName,getSketchObject()->getNameInDocument())== 0) {
                    if (msg.pSubName) {
                        std::string shapetype(msg.pSubName);
                        if (shapetype.size() > 4 && shapetype.substr(0,4) == "Edge") {
                            int GeoId = std::atoi(&shapetype[4]) - 1;
                            edit->SelCurvSet.erase(GeoId);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 12 && shapetype.substr(0,12) == "ExternalEdge") {
                            int GeoId = std::atoi(&shapetype[12]) - 1;
                            GeoId = -GeoId - 3;
                            edit->SelCurvSet.erase(GeoId);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 6 && shapetype.substr(0,6) == "Vertex") {
                            int VtId = std::atoi(&shapetype[6]) - 1;
                            removeSelectPoint(VtId);
                            this->updateColor();
                        }
                        else if (shapetype == "RootPoint") {
                            removeSelectPoint(Sketcher::GeoEnum::RtPnt);
                            this->updateColor();
                        }
                        else if (shapetype == "H_Axis") {
                            edit->SelCurvSet.erase(Sketcher::GeoEnum::HAxis);
                            this->updateColor();
                        }
                        else if (shapetype == "V_Axis") {
                            edit->SelCurvSet.erase(Sketcher::GeoEnum::VAxis);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 10 && shapetype.substr(0,10) == "Constraint") {
                            int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(shapetype);
                            edit->SelConstraintSet.erase(ConstrId);
                            this->drawConstraintIcons();
                            this->updateColor();
                        }
                    }
                }
            }
        }
        else if (msg.Type == Gui::SelectionChanges::SetSelection) {
            // remove all items
            //selectionView->clear();
            //std::vector<SelectionSingleton::SelObj> objs = Gui::Selection().getSelection(Reason.pDocName);
            //for (std::vector<SelectionSingleton::SelObj>::iterator it = objs.begin(); it != objs.end(); ++it) {
            //    // build name
            //    temp = it->DocName;
            //    temp += ".";
            //    temp += it->FeatName;
            //    if (it->SubName && it->SubName[0] != '\0') {
            //        temp += ".";
            //        temp += it->SubName;
            //    }
            //    new QListWidgetItem(QString::fromLatin1(temp.c_str()), selectionView);
            //}
        }
        else if (msg.Type == Gui::SelectionChanges::SetPreselect) {
            if (strcmp(msg.pDocName,getSketchObject()->getDocument()->getName())==0
               && strcmp(msg.pObjectName,getSketchObject()->getNameInDocument())== 0) {
                if (msg.pSubName) {
                    std::string shapetype(msg.pSubName);
                    if (shapetype.size() > 4 && shapetype.substr(0,4) == "Edge") {
                        int GeoId = std::atoi(&shapetype[4]) - 1;
                        resetPreselectPoint();
                        edit->PreselectCurve = GeoId;
                        edit->PreselectCross = -1;
                        edit->PreselectConstraintSet.clear();

                        if (edit->sketchHandler)
                            edit->sketchHandler->applyCursor();
                        this->updateColor();
                    }
                    else if (shapetype.size() > 6 && shapetype.substr(0,6) == "Vertex") {
                        int PtIndex = std::atoi(&shapetype[6]) - 1;
                        setPreselectPoint(PtIndex);
                        edit->PreselectCurve = -1;
                        edit->PreselectCross = -1;
                        edit->PreselectConstraintSet.clear();

                        if (edit->sketchHandler)
                            edit->sketchHandler->applyCursor();
                        this->updateColor();
                    }
                }
            }
        }
        else if (msg.Type == Gui::SelectionChanges::RmvPreselect) {
            resetPreselectPoint();
            edit->PreselectCurve = -1;
            edit->PreselectCross = -1;
            edit->PreselectConstraintSet.clear();
            if (edit->sketchHandler)
                edit->sketchHandler->applyCursor();
            this->updateColor();
        }
    }
}

std::set<int> ViewProviderSketch::detectPreselectionConstr(const SoPickedPoint *Point,
                                                           const Gui::View3DInventorViewer *viewer,
                                                           const SbVec2s &cursorPos)
{
    std::set<int> constrIndices;
    SoPath *path = Point->getPath();
    SoNode *tail = path->getTail();
    SoNode *tailFather = path->getNode(path->getLength()-2);

    for (int i=0; i < edit->constrGroup->getNumChildren(); ++i) {
        if (edit->constrGroup->getChild(i) == tailFather) {
            SoSeparator *sep = static_cast<SoSeparator *>(tailFather);
            if (sep->getNumChildren() > CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID) {
                SoInfo *constrIds = NULL;
                if (tail == sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON)) {
                    // First icon was hit
                    constrIds = static_cast<SoInfo *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID));
                }
                else {
                    // Assume second icon was hit
                    if (CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID<sep->getNumChildren()) {
                        constrIds = static_cast<SoInfo *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID));
                    }
                }

                if (constrIds) {
                    QString constrIdsStr = QString::fromLatin1(constrIds->string.getValue().getString());
                    if (edit->combinedConstrBoxes.count(constrIdsStr) && dynamic_cast<SoImage *>(tail)) {
                        // If it's a combined constraint icon

                        // Screen dimensions of the icon
                        SbVec3s iconSize = getDisplayedSize(static_cast<SoImage *>(tail));
                        // Center of the icon
                        //SbVec2f iconCoords = viewer->screenCoordsOfPath(path);

                        // The use of the Path to get the screen coordinates to get the icon center coordinates
                        // does not work.
                        //
                        // This implementation relies on the use of ZoomTranslation to get the absolute and relative
                        // positions of the icons.
                        //
                        // In the case of second icons (the same constraint has two icons at two different positions),
                        // the translation vectors have to be added, as the second ZoomTranslation operates on top of
                        // the first.
                        //
                        // Coordinates are projected on the sketch plane and then to the screen in the interval [0 1]
                        // Then this result is converted to pixels using the scale factor.

                        SbVec3f absPos;
                        SbVec3f trans;

                        absPos = static_cast<SoZoomTranslation *>(static_cast<SoSeparator *>(tailFather)->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos.getValue();

                        trans = static_cast<SoZoomTranslation *>(static_cast<SoSeparator *>(tailFather)->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation.getValue();

                        if (tail != sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON)) {

                            absPos += static_cast<SoZoomTranslation *>(static_cast<SoSeparator *>(tailFather)->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->abPos.getValue();

                            trans += static_cast<SoZoomTranslation *>(static_cast<SoSeparator *>(tailFather)->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->translation.getValue();
                        }

                        Base::Placement sketchPlacement = getEditingPlacement();
                        Base::Vector3d sketchPos(sketchPlacement.getPosition());
                        Base::Rotation sketchRot(sketchPlacement.getRotation());

                        // get global coordinates from sketcher coordinates
                        SbVec3f constrPos = absPos + trans*getScaleFactor();
                        Base::Vector3d pos(constrPos[0],constrPos[1],0);
                        sketchRot.multVec(pos,pos);
                        pos = pos + sketchPos;

                        SoCamera* pCam = viewer->getSoRenderManager()->getCamera();

                        if (!pCam)
                            continue;

                        SbViewVolume vol = pCam->getViewVolume();
                        Gui::ViewVolumeProjection proj(vol);

                        // dimensionless [0 1] (or 1.5 see View3DInventorViewer.cpp )
                        Base::Vector3d screencoords = proj(pos);

                        int width = viewer->getGLWidget()->width(),
                            height = viewer->getGLWidget()->height();

                        if (width >= height) {
                            // "Landscape" orientation, to square
                            screencoords.x *= height;
                            screencoords.x += (width-height) / 2.0;
                            screencoords.y *= height;
                        }
                        else {
                            // "Portrait" orientation
                            screencoords.x *= width;
                            screencoords.y *= width;
                            screencoords.y += (height-width) / 2.0;
                        }

                        SbVec2f iconCoords(screencoords.x,screencoords.y);

                        // cursorPos is SbVec2s in screen coordinates coming from SoEvent in mousemove
                        //
                        // Coordinates of the mouse cursor on the icon, origin at top-left for Qt
                        // but bottom-left for OIV.
                        // The coordinates are needed in Qt format, i.e. from top to bottom.
                        int iconX = cursorPos[0] - iconCoords[0] + iconSize[0]/2,
                            iconY = cursorPos[1] - iconCoords[1] + iconSize[1]/2;
                        iconY = iconSize[1] - iconY;

                        for (ConstrIconBBVec::iterator b = edit->combinedConstrBoxes[constrIdsStr].begin();
                            b != edit->combinedConstrBoxes[constrIdsStr].end(); ++b) {

#ifdef FC_DEBUG
                            // Useful code to debug coordinates and bounding boxes that does not need to be compiled in for
                            // any debug operations.

                            /*Base::Console().Log("Abs(%f,%f),Trans(%f,%f),Coords(%d,%d),iCoords(%f,%f),icon(%d,%d),isize(%d,%d),boundingbox([%d,%d],[%d,%d])\n", absPos[0],absPos[1],trans[0], trans[1], cursorPos[0], cursorPos[1], iconCoords[0], iconCoords[1], iconX, iconY, iconSize[0], iconSize[1], b->first.topLeft().x(),b->first.topLeft().y(),b->first.bottomRight().x(),b->first.bottomRight().y());*/
#endif

                            if (b->first.contains(iconX, iconY)) {
                                // We've found a bounding box that contains the mouse pointer!
                                for (std::set<int>::iterator k = b->second.begin(); k != b->second.end(); ++k)
                                    constrIndices.insert(*k);
                            }
                        }
                    }
                    else {
                        // It's a constraint icon, not a combined one
                        QStringList constrIdStrings = constrIdsStr.split(QString::fromLatin1(","));
                        while (!constrIdStrings.empty())
                            constrIndices.insert(constrIdStrings.takeAt(0).toInt());
                    }
                }
            }
            else {
                // other constraint icons - eg radius...
                constrIndices.clear();
                constrIndices.insert(i);
            }
            break;
        }
    }

    return constrIndices;
}

bool ViewProviderSketch::detectPreselection(const SoPickedPoint *Point,
                                            const Gui::View3DInventorViewer *viewer,
                                            const SbVec2s &cursorPos)
{
    assert(edit);

    int PtIndex = -1;
    int GeoIndex = -1; // valid values are 0,1,2,... for normal geometry and -3,-4,-5,... for external geometry
    int CrossIndex = -1;
    std::set<int> constrIndices;

    if (Point) {
        //Base::Console().Log("Point pick\n");
        SoPath *path = Point->getPath();
        SoNode *tail = path->getTail();
        SoNode *tailFather2 = path->getNode(path->getLength()-3);

        // checking for a hit in the points
        if (tail == edit->PointSet) {
            const SoDetail *point_detail = Point->getDetail(edit->PointSet);
            if (point_detail && point_detail->getTypeId() == SoPointDetail::getClassTypeId()) {
                // get the index
                PtIndex = static_cast<const SoPointDetail *>(point_detail)->getCoordinateIndex();
                PtIndex -= 1; // shift corresponding to RootPoint
                if (PtIndex == Sketcher::GeoEnum::RtPnt)
                    CrossIndex = 0; // RootPoint was hit
            }
        } else {
            // checking for a hit in the curves
            if (tail == edit->CurveSet) {
                const SoDetail *curve_detail = Point->getDetail(edit->CurveSet);
                if (curve_detail && curve_detail->getTypeId() == SoLineDetail::getClassTypeId()) {
                    // get the index
                    int curveIndex = static_cast<const SoLineDetail *>(curve_detail)->getLineIndex();
                    GeoIndex = edit->CurvIdToGeoId[curveIndex];
                }
            // checking for a hit in the cross
            } else if (tail == edit->RootCrossSet) {
                const SoDetail *cross_detail = Point->getDetail(edit->RootCrossSet);
                if (cross_detail && cross_detail->getTypeId() == SoLineDetail::getClassTypeId()) {
                    // get the index (reserve index 0 for root point)
                    CrossIndex = 1 + static_cast<const SoLineDetail *>(cross_detail)->getLineIndex();
                }
            } else {
                // checking if a constraint is hit
                if (tailFather2 == edit->constrGroup)
                    constrIndices = detectPreselectionConstr(Point, viewer, cursorPos);
            }
        }

        if (PtIndex != -1 && PtIndex != edit->PreselectPoint) { // if a new point is hit
            std::stringstream ss;
            ss << "Vertex" << PtIndex + 1;
            bool accepted =
            Gui::Selection().setPreselect(SEL_PARAMS
                                         ,Point->getPoint()[0]
                                         ,Point->getPoint()[1]
                                         ,Point->getPoint()[2]) != 0;
            edit->blockedPreselection = !accepted;
            if (accepted) {
                setPreselectPoint(PtIndex);
                edit->PreselectCurve = -1;
                edit->PreselectCross = -1;
                edit->PreselectConstraintSet.clear();
                if (edit->sketchHandler)
                    edit->sketchHandler->applyCursor();
                return true;
            }
        } else if (GeoIndex != -1 && GeoIndex != edit->PreselectCurve) {  // if a new curve is hit
            std::stringstream ss;
            if (GeoIndex >= 0)
                ss << "Edge" << GeoIndex + 1;
            else // external geometry
                ss << "ExternalEdge" << -GeoIndex + Sketcher::GeoEnum::RefExt + 1; // convert index start from -3 to 1
            bool accepted =
            Gui::Selection().setPreselect(SEL_PARAMS
                                         ,Point->getPoint()[0]
                                         ,Point->getPoint()[1]
                                         ,Point->getPoint()[2]) != 0;
            edit->blockedPreselection = !accepted;
            if (accepted) {
                resetPreselectPoint();
                edit->PreselectCurve = GeoIndex;
                edit->PreselectCross = -1;
                edit->PreselectConstraintSet.clear();
                if (edit->sketchHandler)
                    edit->sketchHandler->applyCursor();
                return true;
            }
        } else if (CrossIndex != -1 && CrossIndex != edit->PreselectCross) {  // if a cross line is hit
            std::stringstream ss;
            switch(CrossIndex){
                case 0: ss << "RootPoint" ; break;
                case 1: ss << "H_Axis"    ; break;
                case 2: ss << "V_Axis"    ; break;
            }
            bool accepted =
            Gui::Selection().setPreselect(SEL_PARAMS
                                         ,Point->getPoint()[0]
                                         ,Point->getPoint()[1]
                                         ,Point->getPoint()[2]) != 0;
            edit->blockedPreselection = !accepted;
            if (accepted) {
                if (CrossIndex == 0)
                    setPreselectPoint(-1);
                else
                    resetPreselectPoint();
                edit->PreselectCurve = -1;
                edit->PreselectCross = CrossIndex;
                edit->PreselectConstraintSet.clear();
                if (edit->sketchHandler)
                    edit->sketchHandler->applyCursor();
                return true;
            }
        } else if (constrIndices.empty() == false && constrIndices != edit->PreselectConstraintSet) { // if a constraint is hit
            bool accepted = true;
            for(std::set<int>::iterator it = constrIndices.begin(); it != constrIndices.end(); ++it) {
                std::stringstream ss;
                ss << Sketcher::PropertyConstraintList::getConstraintName(*it);

                accepted &=
                Gui::Selection().setPreselect(SEL_PARAMS
                                             ,Point->getPoint()[0]
                                             ,Point->getPoint()[1]
                                             ,Point->getPoint()[2]) != 0;

                edit->blockedPreselection = !accepted;
                //TODO: Should we clear preselections that went through, if one fails?
            }
            if (accepted) {
                resetPreselectPoint();
                edit->PreselectCurve = -1;
                edit->PreselectCross = -1;
                edit->PreselectConstraintSet = constrIndices;
                if (edit->sketchHandler)
                    edit->sketchHandler->applyCursor();
                return true;//Preselection changed
            }
        } else if ((PtIndex == -1 && GeoIndex == -1 && CrossIndex == -1 && constrIndices.empty()) &&
                   (edit->PreselectPoint != -1 || edit->PreselectCurve != -1 || edit->PreselectCross != -1
                    || edit->PreselectConstraintSet.empty() != true || edit->blockedPreselection)) {
            // we have just left a preselection
            resetPreselectPoint();
            edit->PreselectCurve = -1;
            edit->PreselectCross = -1;
            edit->PreselectConstraintSet.clear();
            edit->blockedPreselection = false;
            if (edit->sketchHandler)
                edit->sketchHandler->applyCursor();
            return true;
        }
        Gui::Selection().setPreselectCoord(Point->getPoint()[0]
                                          ,Point->getPoint()[1]
                                          ,Point->getPoint()[2]);
// if(Point)
    } else if (edit->PreselectCurve != -1 || edit->PreselectPoint != -1 ||
               edit->PreselectConstraintSet.empty() != true || edit->PreselectCross != -1 || edit->blockedPreselection) {
        resetPreselectPoint();
        edit->PreselectCurve = -1;
        edit->PreselectCross = -1;
        edit->PreselectConstraintSet.clear();
        edit->blockedPreselection = false;
        if (edit->sketchHandler)
            edit->sketchHandler->applyCursor();
        return true;
    }

    return false;
}

SbVec3s ViewProviderSketch::getDisplayedSize(const SoImage *iconPtr) const
{
#if (COIN_MAJOR_VERSION >= 3)
    SbVec3s iconSize = iconPtr->image.getValue().getSize();
#else
    SbVec2s size;
    int nc;
    const unsigned char * bytes = iconPtr->image.getValue(size, nc);
    SbImage img (bytes, size, nc);
    SbVec3s iconSize = img.getSize();
#endif
    if (iconPtr->width.getValue() != -1)
        iconSize[0] = iconPtr->width.getValue();
    if (iconPtr->height.getValue() != -1)
        iconSize[1] = iconPtr->height.getValue();
    return iconSize;
}

void ViewProviderSketch::centerSelection()
{
    Gui::MDIView *mdi = this->getActiveView();
    Gui::View3DInventor *view = qobject_cast<Gui::View3DInventor*>(mdi);
    if (!view || !edit)
        return;

    SoGroup* group = new SoGroup();
    group->ref();

    for (int i=0; i < edit->constrGroup->getNumChildren(); i++) {
        if (edit->SelConstraintSet.find(i) != edit->SelConstraintSet.end()) {
            SoSeparator *sep = dynamic_cast<SoSeparator *>(edit->constrGroup->getChild(i));
            if (sep)
                group->addChild(sep);
        }
    }

    Gui::View3DInventorViewer* viewer = view->getViewer();
    SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.apply(group);
    group->unref();

    SbBox3f box = action.getBoundingBox();
    if (!box.isEmpty()) {
        SoCamera* camera = viewer->getSoRenderManager()->getCamera();
        SbVec3f direction;
        camera->orientation.getValue().multVec(SbVec3f(0, 0, 1), direction);
        SbVec3f box_cnt = box.getCenter();
        SbVec3f cam_pos = box_cnt + camera->focalDistance.getValue() * direction;
        camera->position.setValue(cam_pos);
    }
}

void ViewProviderSketch::doBoxSelection(const SbVec2s &startPos, const SbVec2s &endPos,
                                        const Gui::View3DInventorViewer *viewer)
{
    std::vector<SbVec2s> corners0;
    corners0.push_back(startPos);
    corners0.push_back(endPos);
    std::vector<SbVec2f> corners = viewer->getGLPolygon(corners0);

    // all calculations with polygon and proj are in dimensionless [0 1] screen coordinates
    Base::Polygon2d polygon;
    polygon.Add(Base::Vector2d(corners[0].getValue()[0], corners[0].getValue()[1]));
    polygon.Add(Base::Vector2d(corners[0].getValue()[0], corners[1].getValue()[1]));
    polygon.Add(Base::Vector2d(corners[1].getValue()[0], corners[1].getValue()[1]));
    polygon.Add(Base::Vector2d(corners[1].getValue()[0], corners[0].getValue()[1]));

    Gui::ViewVolumeProjection proj(viewer->getSoRenderManager()->getCamera()->getViewVolume());

    Sketcher::SketchObject *sketchObject = getSketchObject();

    Base::Placement Plm = getEditingPlacement();

    int intGeoCount = sketchObject->getHighestCurveIndex() + 1;
    int extGeoCount = sketchObject->getExternalGeometryCount();

    const std::vector<Part::Geometry *> geomlist = sketchObject->getCompleteGeometry(); // without memory allocation
    assert(int(geomlist.size()) == extGeoCount + intGeoCount);
    assert(int(geomlist.size()) >= 2);

    Base::Vector3d pnt0, pnt1, pnt2, pnt;
    int VertexId = -1; // the loop below should be in sync with the main loop in ViewProviderSketch::draw
                       // so that the vertex indices are calculated correctly
    int GeoId = 0;

    bool touchMode = false;
    //check if selection goes from the right to the left side (for touch-selection where even partially boxed objects get selected)
    if(corners[0].getValue()[0] > corners[1].getValue()[0])
        touchMode = true;

    for (std::vector<Part::Geometry *>::const_iterator it = geomlist.begin(); it != geomlist.end()-2; ++it, ++GeoId) {

        if (GeoId >= intGeoCount)
            GeoId = -extGeoCount;

        if ((*it)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            // ----- Check if single point lies inside box selection -----/
            const Part::GeomPoint *point = static_cast<const Part::GeomPoint *>(*it);
            Plm.multVec(point->getPoint(), pnt0);
            pnt0 = proj(pnt0);
            VertexId += 1;

            if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

        } else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            // ----- Check if line segment lies inside box selection -----/
            const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(*it);
            Plm.multVec(lineSeg->getStartPoint(), pnt1);
            Plm.multVec(lineSeg->getEndPoint(), pnt2);
            pnt1 = proj(pnt1);
            pnt2 = proj(pnt2);
            VertexId += 2;

            bool pnt1Inside = polygon.Contains(Base::Vector2d(pnt1.x, pnt1.y));
            bool pnt2Inside = polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y));
            if (pnt1Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

            if (pnt2Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

            if ((pnt1Inside && pnt2Inside) && !touchMode) {
                std::stringstream ss;
                ss << "Edge" << GeoId + 1;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }
            //check if line intersects with polygon
            else if (touchMode) {
                    Base::Polygon2d lineAsPolygon;
                    lineAsPolygon.Add(Base::Vector2d(pnt1.x, pnt1.y));
                    lineAsPolygon.Add(Base::Vector2d(pnt2.x, pnt2.y));
                    std::list<Base::Polygon2d> resultList;
                    polygon.Intersect(lineAsPolygon, resultList);
                    if (!resultList.empty()) {
                        std::stringstream ss;
                        ss << "Edge" << GeoId + 1;
                        Gui::Selection().addSelection2(SEL_PARAMS);
                    }
                }

        } else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            // ----- Check if circle lies inside box selection -----/
            ///TODO: Make it impossible to miss the circle if it's big and the selection pretty thin.
            const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(*it);
            pnt0 = circle->getCenter();
            VertexId += 1;

            Plm.multVec(pnt0, pnt0);
            pnt0 = proj(pnt0);

            if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y)) || touchMode) {
                if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y))) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }
                int countSegments = 12;
                if (touchMode)
                    countSegments = 36;

                float segment = float(2 * M_PI) / countSegments;

                // circumscribed polygon radius
                float radius = float(circle->getRadius()) / cos(segment/2);

                bool bpolyInside = true;
                pnt0 = circle->getCenter();
                float angle = 0.f;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(pnt0.x + radius * cos(angle),
                                         pnt0.y + radius * sin(angle),
                                         0.f);
                    Plm.multVec(pnt, pnt);
                    pnt = proj(pnt);
                    if (!polygon.Contains(Base::Vector2d(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        if (!touchMode)
                            break;
                    }
                    else if (touchMode) {
                        bpolyInside = true;
                        break;
                    }
                }

                if (bpolyInside) {
                    std::stringstream ss;
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }
            }
        } else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            // ----- Check if ellipse lies inside box selection -----/
            const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(*it);
            pnt0 = ellipse->getCenter();
            VertexId += 1;

            Plm.multVec(pnt0, pnt0);
            pnt0 = proj(pnt0);

            if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y)) || touchMode) {
                if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y))) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }

                int countSegments = 12;
                if (touchMode)
                    countSegments = 24;
                double segment = (2 * M_PI) / countSegments;

                // circumscribed polygon radius
                double a = (ellipse->getMajorRadius()) / cos(segment/2);
                double b = (ellipse->getMinorRadius()) / cos(segment/2);
                Base::Vector3d majdir = ellipse->getMajorAxisDir();
                Base::Vector3d mindir = Base::Vector3d(-majdir.y, majdir.x, 0.0);

                bool bpolyInside = true;
                pnt0 = ellipse->getCenter();
                double angle = 0.;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = pnt0 + (cos(angle)*a)*majdir + sin(angle)*b*mindir;
                    Plm.multVec(pnt, pnt);
                    pnt = proj(pnt);
                    if (!polygon.Contains(Base::Vector2d(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        if (!touchMode)
                            break;
                    }
                    else if (touchMode) {
                        bpolyInside = true;
                        break;
                    }
                }

                if (bpolyInside) {
                    std::stringstream ss;
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }
            }

        } else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfCircle *aoc = static_cast<const Part::GeomArcOfCircle *>(*it);

            pnt0 = aoc->getStartPoint(/*emulateCCW=*/true);
            pnt1 = aoc->getEndPoint(/*emulateCCW=*/true);
            pnt2 = aoc->getCenter();
            VertexId += 3;

            Plm.multVec(pnt0, pnt0);
            Plm.multVec(pnt1, pnt1);
            Plm.multVec(pnt2, pnt2);
            pnt0 = proj(pnt0);
            pnt1 = proj(pnt1);
            pnt2 = proj(pnt2);

            bool pnt0Inside = polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y));
            bool pnt1Inside = polygon.Contains(Base::Vector2d(pnt1.x, pnt1.y));
            bool bpolyInside = true;

            if ((pnt0Inside && pnt1Inside) || touchMode) {
                double startangle, endangle;
                aoc->getRange(startangle, endangle, /*emulateCCW=*/true);

                if (startangle > endangle) // if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle-startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                if (touchMode)
                    countSegments=countSegments*2.5;
                float segment = float(range) / countSegments;

                // circumscribed polygon radius
                float radius = float(aoc->getRadius()) / cos(segment/2);

                pnt0 = aoc->getCenter();
                float angle = float(startangle) + segment/2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(pnt0.x + radius * cos(angle),
                                         pnt0.y + radius * sin(angle),
                                         0.f);
                    Plm.multVec(pnt, pnt);
                    pnt = proj(pnt);
                    if (!polygon.Contains(Base::Vector2d(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        if (!touchMode)
                            break;
                    }
                    else if(touchMode) {
                        bpolyInside = true;
                        break;
                    }
                }

                if (bpolyInside) {
                    std::stringstream ss;
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }
            }

            if (pnt0Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId - 1;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

            if (pnt1Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

            if (polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }
        } else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(*it);

            pnt0 = aoe->getStartPoint(/*emulateCCW=*/true);
            pnt1 = aoe->getEndPoint(/*emulateCCW=*/true);
            pnt2 = aoe->getCenter();

            VertexId += 3;

            Plm.multVec(pnt0, pnt0);
            Plm.multVec(pnt1, pnt1);
            Plm.multVec(pnt2, pnt2);
            pnt0 = proj(pnt0);
            pnt1 = proj(pnt1);
            pnt2 = proj(pnt2);

            bool pnt0Inside = polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y));
            bool pnt1Inside = polygon.Contains(Base::Vector2d(pnt1.x, pnt1.y));
            bool bpolyInside = true;

            if ((pnt0Inside && pnt1Inside) || touchMode) {
                double startangle, endangle;
                aoe->getRange(startangle, endangle, /*emulateCCW=*/true);

                if (startangle > endangle) // if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle-startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                if (touchMode)
                    countSegments=countSegments*2.5;
                double segment = (range) / countSegments;

                // circumscribed polygon radius
                double a = (aoe->getMajorRadius()) / cos(segment/2);
                double b = (aoe->getMinorRadius()) / cos(segment/2);
                Base::Vector3d majdir = aoe->getMajorAxisDir();
                Base::Vector3d mindir = Base::Vector3d(-majdir.y, majdir.x, 0.0);

                pnt0 = aoe->getCenter();
                double angle = (startangle) + segment/2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = pnt0 + cos(angle)*a*majdir + sin(angle)*b*mindir;

                    Plm.multVec(pnt, pnt);
                    pnt = proj(pnt);
                    if (!polygon.Contains(Base::Vector2d(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        if (!touchMode)
                            break;
                    }
                    else if (touchMode) {
                        bpolyInside = true;
                        break;
                    }
                }

                if (bpolyInside) {
                    std::stringstream ss;
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }
            }
            if (pnt0Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId - 1;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

            if (pnt1Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

            if (polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

        } else if ((*it)->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(*it);
            pnt0 = aoh->getStartPoint();
            pnt1 = aoh->getEndPoint();
            pnt2 = aoh->getCenter();

            VertexId += 3;

            Plm.multVec(pnt0, pnt0);
            Plm.multVec(pnt1, pnt1);
            Plm.multVec(pnt2, pnt2);
            pnt0 = proj(pnt0);
            pnt1 = proj(pnt1);
            pnt2 = proj(pnt2);

            bool pnt0Inside = polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y));
            bool pnt1Inside = polygon.Contains(Base::Vector2d(pnt1.x, pnt1.y));
            bool bpolyInside = true;

            if ((pnt0Inside && pnt1Inside) || touchMode) {
                double startangle, endangle;

                aoh->getRange(startangle, endangle, /*emulateCCW=*/true);

                if (startangle > endangle) // if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle-startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                if (touchMode)
                    countSegments=countSegments*2.5;

                float segment = float(range) / countSegments;

                // circumscribed polygon radius
                float a = float(aoh->getMajorRadius()) / cos(segment/2);
                float b = float(aoh->getMinorRadius()) / cos(segment/2);
                float phi = float(aoh->getAngleXU());

                pnt0 = aoh->getCenter();
                float angle = float(startangle) + segment/2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(pnt0.x + a * cosh(angle) * cos(phi) - b * sinh(angle) * sin(phi),
                                         pnt0.y + a * cosh(angle) * sin(phi) + b * sinh(angle) * cos(phi),
                                         0.f);

                    Plm.multVec(pnt, pnt);
                    pnt = proj(pnt);
                    if (!polygon.Contains(Base::Vector2d(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        if (!touchMode)
                            break;
                    }
                    else if (touchMode) {
                        bpolyInside = true;
                        break;
                    }
                }

                if (bpolyInside) {
                    std::stringstream ss;
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }
                if (pnt0Inside) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId - 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }

                if (pnt1Inside) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }

                if (polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y))) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }

            }

        } else if ((*it)->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(*it);

            pnt0 = aop->getStartPoint();
            pnt1 = aop->getEndPoint();
            pnt2 = aop->getCenter();

            VertexId += 3;

            Plm.multVec(pnt0, pnt0);
            Plm.multVec(pnt1, pnt1);
            Plm.multVec(pnt2, pnt2);
            pnt0 = proj(pnt0);
            pnt1 = proj(pnt1);
            pnt2 = proj(pnt2);

            bool pnt0Inside = polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y));
            bool pnt1Inside = polygon.Contains(Base::Vector2d(pnt1.x, pnt1.y));
            bool bpolyInside = true;

            if ((pnt0Inside && pnt1Inside) || touchMode) {
                double startangle, endangle;

                aop->getRange(startangle, endangle, /*emulateCCW=*/true);

                if (startangle > endangle) // if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle-startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                if (touchMode)
                    countSegments=countSegments*2.5;

                float segment = float(range) / countSegments;
                //In local coordinate system, value() of parabola is:
                //P(U) = O + U*U/(4.*F)*XDir + U*YDir
                                                // circumscribed polygon radius
                float focal = float(aop->getFocal()) / cos(segment/2);
                float phi = float(aop->getAngleXU());

                pnt0 = aop->getCenter();
                float angle = float(startangle) + segment/2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(pnt0.x + angle * angle / 4 / focal * cos(phi) - angle * sin(phi),
                                         pnt0.y + angle * angle / 4 / focal * sin(phi) + angle * cos(phi),
                                         0.f);

                    Plm.multVec(pnt, pnt);
                    pnt = proj(pnt);
                    if (!polygon.Contains(Base::Vector2d(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        if (!touchMode)
                            break;
                    }
                    else if (touchMode) {
                        bpolyInside = true;
                        break;
                    }
                }

                if (bpolyInside) {
                    std::stringstream ss;
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }
                if (pnt0Inside) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId - 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }

                if (pnt1Inside) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }

                if (polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y))) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId + 1;
                    Gui::Selection().addSelection2(SEL_PARAMS);
                }
            }

        } else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            const Part::GeomBSplineCurve *spline = static_cast<const Part::GeomBSplineCurve *>(*it);
            //std::vector<Base::Vector3d> poles = spline->getPoles();
            VertexId += 2;

            Plm.multVec(spline->getStartPoint(), pnt1);
            Plm.multVec(spline->getEndPoint(), pnt2);
            pnt1 = proj(pnt1);
            pnt2 = proj(pnt2);

            bool pnt1Inside = polygon.Contains(Base::Vector2d(pnt1.x, pnt1.y));
            bool pnt2Inside = polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y));
            if (pnt1Inside || (touchMode && pnt2Inside)) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

            if (pnt2Inside || (touchMode && pnt1Inside)) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }

            // This is a rather approximated approach. No it does not guarantee that the whole curve is boxed, specially
            // for periodic curves, but it works reasonably well. Including all poles, which could be done, generally
            // forces the user to select much more than the curve (all the poles) and it would not select the curve in cases
            // where it is indeed comprised in the box.
            // The implementation of the touch mode is also far from a desirable "touch" as it only recognizes touched points not the curve itself
            if ((pnt1Inside && pnt2Inside) || (touchMode && (pnt1Inside || pnt2Inside))) {
                std::stringstream ss;
                ss << "Edge" << GeoId + 1;
                Gui::Selection().addSelection2(SEL_PARAMS);
            }
        }
    }

    pnt0 = proj(Plm.getPosition());
    if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y))) {
        std::stringstream ss;
        ss << "RootPoint";
        Gui::Selection().addSelection2(SEL_PARAMS);
    }
}

void ViewProviderSketch::updateColor(void)
{
    assert(edit);
    //Base::Console().Log("Draw preseletion\n");

    int PtNum = edit->PointsMaterials->diffuseColor.getNum();
    SbColor *pcolor = edit->PointsMaterials->diffuseColor.startEditing();
    int CurvNum = edit->CurvesMaterials->diffuseColor.getNum();
    SbColor *color = edit->CurvesMaterials->diffuseColor.startEditing();
    SbColor *crosscolor = edit->RootCrossMaterials->diffuseColor.startEditing();

    SbVec3f *verts = edit->CurvesCoordinate->point.startEditing();
  //int32_t *index = edit->CurveSet->numVertices.startEditing();
    SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();

    ParameterGrp::handle hGrpp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    // 1->Normal Geometry, 2->Construction, 3->External
    int topid = hGrpp->GetInt("TopRenderGeometryId",1);
    int midid = hGrpp->GetInt("MidRenderGeometryId",2);

    float zNormPoint = (topid==1?zHighPoints:(midid==1 && topid!=2)?zHighPoints:zLowPoints);
    float zConstrPoint = (topid==2?zHighPoints:(midid==2 && topid!=1)?zHighPoints:zLowPoints);

    float x,y,z;

    // use a lambda function to only access the geometry when needed
    // and properly handle the case where it's null
    auto isConstructionGeom = [](Sketcher::SketchObject* obj, int GeoId) -> bool {
        const Part::Geometry* geom = obj->getGeometry(GeoId);
        if (geom)
            return Sketcher::GeometryFacade::getConstruction(geom);
        return false;
    };

    auto isDefinedGeomPoint = [](Sketcher::SketchObject* obj, int GeoId) -> bool {
        const Part::Geometry* geom = obj->getGeometry(GeoId);
        if (geom)
            return geom->getTypeId() == Part::GeomPoint::getClassTypeId() && !Sketcher::GeometryFacade::getConstruction(geom);
        return false;
    };

    auto isInternalAlignedGeom = [](Sketcher::SketchObject* obj, int GeoId) -> bool {
        const Part::Geometry* geom = obj->getGeometry(GeoId);
        if (geom) {
            auto gf = Sketcher::GeometryFacade::getFacade(geom);
            return gf->isInternalAligned();
        }
        return false;
    };

    auto isFullyConstraintElement = [](Sketcher::SketchObject* obj, int GeoId) -> bool {

        const Part::Geometry* geom = obj->getGeometry(GeoId);

        if(geom) {
            if(geom->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {

                auto solvext = std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                                    geom->getExtension(Sketcher::SolverGeometryExtension::getClassTypeId()).lock());

                return (solvext->getGeometry() == Sketcher::SolverGeometryExtension::FullyConstraint);
            }
        }
        return false;
    };

    bool invalidSketch =    getSketchObject()->getLastHasRedundancies()           ||
                            getSketchObject()->getLastHasConflicts()              ||
                            getSketchObject()->getLastHasMalformedConstraints();

    // colors of the point set
    if( invalidSketch ) {
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = InvalidSketchColor;
    }
    else if (edit->FullyConstrained) {
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = FullyConstrainedColor;
    }
    else {
        for (int  i=0; i < PtNum; i++) {
            int GeoId = edit->PointIdToGeoId[i];

            bool constrainedElement = isFullyConstraintElement(getSketchObject(), GeoId);

            if(isInternalAlignedGeom(getSketchObject(), GeoId)) {
                if(constrainedElement)
                    pcolor[i] = FullyConstraintInternalAlignmentColor;
                else
                    pcolor[i] = InternalAlignedGeoColor;
            }
            else {
                if(!isDefinedGeomPoint(getSketchObject(), GeoId)) {

                    if(constrainedElement)
                        pcolor[i] = FullyConstraintConstructionPointColor;
                    else
                        pcolor[i] = VertexColor;
                }
                else { // this is a defined GeomPoint
                    if(constrainedElement)
                        pcolor[i] = FullyConstraintElementColor;
                    else
                        pcolor[i] = CurveColor;
                }
            }
        }
    }

    for (int  i=0; i < PtNum; i++) { // 0 is the origin
        pverts[i].getValue(x,y,z);
        const Part::Geometry * tmp = getSketchObject()->getGeometry(edit->PointIdToGeoId[i]);
        if(tmp && z < zHighlight) {
            if(Sketcher::GeometryFacade::getConstruction(tmp))
                pverts[i].setValue(x,y,zConstrPoint);
            else
                pverts[i].setValue(x,y,zNormPoint);
        }
    }


    if (edit->PreselectCross == 0) {
        pcolor[0] = PreselectColor;
    }
    else if (edit->PreselectPoint != -1) {
        if (edit->PreselectPoint + 1 < PtNum)
            pcolor[edit->PreselectPoint + 1] = PreselectColor;
    }

    for (std::set<int>::iterator it = edit->SelPointSet.begin(); it != edit->SelPointSet.end(); ++it) {
        if (*it < PtNum) {
            pcolor[*it] = (*it==(edit->PreselectPoint + 1) && (edit->PreselectPoint != -1))
                ? PreselectSelectedColor : SelectColor;
        }
    }

    // colors of the curves
  //int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;
  //int extGeoCount = getSketchObject()->getExternalGeometryCount();



    float zNormLine = (topid==1?zHighLines:midid==1?zMidLines:zLowLines);
    float zConstrLine = (topid==2?zHighLines:midid==2?zMidLines:zLowLines);
    float zExtLine = (topid==3?zHighLines:midid==3?zMidLines:zLowLines);

    int j=0; // vertexindex

    for (int  i=0; i < CurvNum; i++) {
        int GeoId = edit->CurvIdToGeoId[i];
        // CurvId has several vertices associated to 1 material
        //edit->CurveSet->numVertices => [i] indicates number of vertex for line i.
        int indexes = (edit->CurveSet->numVertices[i]);

        bool selected = (edit->SelCurvSet.find(GeoId) != edit->SelCurvSet.end());
        bool preselected = (edit->PreselectCurve == GeoId);

        bool constrainedElement = isFullyConstraintElement(getSketchObject(), GeoId);

        if (selected && preselected) {
            color[i] = PreselectSelectedColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zHighLine);
            }
        }
        else if (selected){
            color[i] = SelectColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zHighLine);
            }
        }
        else if (preselected){
            color[i] = PreselectColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zHighLine);
            }
        }
        else if (GeoId <= Sketcher::GeoEnum::RefExt) {  // external Geometry
            color[i] = CurveExternalColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zExtLine);
            }
        }
        else if ( invalidSketch ) {
            color[i] = InvalidSketchColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zNormLine);
            }
        }
        else if (isConstructionGeom(getSketchObject(), GeoId)) {
            if(isInternalAlignedGeom(getSketchObject(), GeoId)) {
                if(constrainedElement)
                    color[i] = FullyConstraintInternalAlignmentColor;
                else
                    color[i] = InternalAlignedGeoColor;
            }
            else {
                if(constrainedElement)
                    color[i] = FullyConstraintConstructionElementColor;
                else
                    color[i] = CurveDraftColor;
            }

            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zConstrLine);
            }
        }
        else if (edit->FullyConstrained) {
            color[i] = FullyConstrainedColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zNormLine);
            }
        }
        else if (isFullyConstraintElement(getSketchObject(), GeoId)) {
            color[i] = FullyConstraintElementColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zNormLine);
            }
        }
        else {
            color[i] = CurveColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zNormLine);
            }
        }
    }

    // colors of the cross
    if (edit->SelCurvSet.find(-1) != edit->SelCurvSet.end())
        crosscolor[0] = SelectColor;
    else if (edit->PreselectCross == 1)
        crosscolor[0] = PreselectColor;
    else
        crosscolor[0] = CrossColorH;

    if (edit->SelCurvSet.find(Sketcher::GeoEnum::VAxis) != edit->SelCurvSet.end())
        crosscolor[1] = SelectColor;
    else if (edit->PreselectCross == 2)
        crosscolor[1] = PreselectColor;
    else
        crosscolor[1] = CrossColorV;

    int count = std::min(edit->constrGroup->getNumChildren(), getSketchObject()->Constraints.getSize());
    if(getSketchObject()->Constraints.hasInvalidGeometry())
        count = 0;

    // colors of the constraints
    for (int i=0; i < count; i++) {
        SoSeparator *s = static_cast<SoSeparator *>(edit->constrGroup->getChild(i));

        // Check Constraint Type
        Sketcher::Constraint* constraint = getSketchObject()->Constraints.getValues()[i];
        ConstraintType type = constraint->Type;
        bool hasDatumLabel  = (type == Sketcher::Angle ||
                               type == Sketcher::Radius ||
                               type == Sketcher::Diameter ||
                               type == Sketcher::Weight ||
                               type == Sketcher::Symmetric ||
                               type == Sketcher::Distance ||
                               type == Sketcher::DistanceX ||
                               type == Sketcher::DistanceY);

        // Non DatumLabel Nodes will have a material excluding coincident
        bool hasMaterial = false;

        SoMaterial *m = 0;
        if (!hasDatumLabel && type != Sketcher::Coincident && type != Sketcher::InternalAlignment) {
            hasMaterial = true;
            m = static_cast<SoMaterial *>(s->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
        }

        if (edit->SelConstraintSet.find(i) != edit->SelConstraintSet.end()) {
            if (hasDatumLabel) {
                SoDatumLabel *l = static_cast<SoDatumLabel *>(s->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                l->textColor = SelectColor;
            } else if (hasMaterial) {
                m->diffuseColor = SelectColor;
            } else if (type == Sketcher::Coincident) {
                auto selectpoint = [this, pcolor, PtNum](int geoid, Sketcher::PointPos pos){
                    if(geoid >= 0) {
                        int index = getSolvedSketch().getPointId(geoid, pos) + 1;
                        if (index >= 0 && index < PtNum)
                            pcolor[index] = SelectColor;
                    }
                };

                selectpoint(constraint->First, constraint->FirstPos);
                selectpoint(constraint->Second, constraint->SecondPos);
            } else if (type == Sketcher::InternalAlignment) {
                switch(constraint->AlignmentType) {
                    case EllipseMajorDiameter:
                    case EllipseMinorDiameter:
                    {
                        // color line
                        int CurvNum = edit->CurvesMaterials->diffuseColor.getNum();
                        for (int  i=0; i < CurvNum; i++) {
                            int cGeoId = edit->CurvIdToGeoId[i];

                            if(cGeoId == constraint->First) {
                                color[i] = SelectColor;
                                break;
                            }
                        }
                    }
                    break;
                    case EllipseFocus1:
                    case EllipseFocus2:
                    {
                        int index = getSolvedSketch().getPointId(constraint->First, constraint->FirstPos) + 1;
                        if (index >= 0 && index < PtNum) pcolor[index] = SelectColor;
                    }
                    break;
                    default:
                    break;
                }
            }
        } else if (edit->PreselectConstraintSet.count(i)) {
            if (hasDatumLabel) {
                SoDatumLabel *l = static_cast<SoDatumLabel *>(s->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                l->textColor = PreselectColor;
            } else if (hasMaterial) {
                m->diffuseColor = PreselectColor;
            }
        }
        else {
            if (hasDatumLabel) {
                SoDatumLabel *l = static_cast<SoDatumLabel *>(s->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));

                l->textColor = constraint->isActive ?
                                    (getSketchObject()->constraintHasExpression(i) ?
                                        ExprBasedConstrDimColor
                                        :(constraint->isDriving ?
                                            ConstrDimColor
                                            : NonDrivingConstrDimColor))
                                    :DeactivatedConstrDimColor;

            } else if (hasMaterial) {
                m->diffuseColor = constraint->isActive ?
                                    (constraint->isDriving ?
                                        ConstrDimColor
                                        :NonDrivingConstrDimColor)
                                    :DeactivatedConstrDimColor;
            }
        }
    }

    // end editing
    edit->CurvesMaterials->diffuseColor.finishEditing();
    edit->PointsMaterials->diffuseColor.finishEditing();
    edit->RootCrossMaterials->diffuseColor.finishEditing();
    edit->CurvesCoordinate->point.finishEditing();
    edit->CurveSet->numVertices.finishEditing();
}

bool ViewProviderSketch::isPointOnSketch(const SoPickedPoint *pp) const
{
    // checks if we picked a point on the sketch or any other nodes like the grid
    SoPath *path = pp->getPath();
    return path->containsNode(edit->EditRoot);
}

bool ViewProviderSketch::doubleClicked(void)
{
    Gui::Application::Instance->activeDocument()->setEdit(this);
    return true;
}

QString ViewProviderSketch::getPresentationString(const Constraint *constraint)
{
    Base::Reference<ParameterGrp>   hGrpSketcher; // param group that includes HideUnits and ShowDimensionalName option
    bool                            iHideUnits; // internal HideUnits setting
    bool                            iShowDimName; // internal ShowDimensionalName setting
    QString                         nameStr; // name parameter string
    QString                         valueStr; // dimensional value string
    QString                         presentationStr; // final return string
    QString                         unitStr;  // the actual unit string
    QString                         baseUnitStr; // the expected base unit string
    QString                         formatStr; // the user defined format for the representation string
    double                          factor; // unit scaling factor, currently not used
    Base::UnitSystem                unitSys; // current unit system

    if(!constraint->isActive)
        return QString::fromLatin1(" ");

    // get parameter group for Sketcher display settings
    hGrpSketcher = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Sketcher");
    // Get value of HideUnits option. Default is false.
    iHideUnits = hGrpSketcher->GetBool("HideUnits", 0);
    // Get Value of ShowDimensionalName option. Default is true.
    iShowDimName = hGrpSketcher->GetBool("ShowDimensionalName", false);
    // Get the defined format string
    formatStr = QString::fromStdString(hGrpSketcher->GetASCII("DimensionalStringFormat", "%N = %V"));

    // Get the current name parameter string of the constraint
    nameStr = QString::fromStdString(constraint->Name);

    // Get the current value string including units
    valueStr = constraint->getPresentationValue().getUserString(factor, unitStr);

    // Hide units if user has requested it, is being displayed in the base
    // units, and the schema being used has a clear base unit in the first
    // place. Otherwise, display units.
    if( iHideUnits && constraint->Type != Sketcher::Angle )
    {
        // Only hide the default length unit. Right now there is not an easy way
        // to get that from the Unit system so we have to manually add it here.
        // Hopefully this can be added in the future so this code won't have to
        // be updated if a new units schema is added.
        unitSys = Base::UnitsApi::getSchema();

        // If this is a supported unit system then define what the base unit is.
        switch (unitSys)
        {
            case Base::UnitSystem::SI1:
            case Base::UnitSystem::MmMin:
                baseUnitStr = QString::fromLatin1("mm");
                break;

            case Base::UnitSystem::SI2:
                baseUnitStr = QString::fromLatin1("m");
                break;

            case Base::UnitSystem::ImperialDecimal:
                baseUnitStr = QString::fromLatin1("in");
                break;

            case Base::UnitSystem::Centimeters:
                baseUnitStr = QString::fromLatin1("cm");
                break;

            default:
                // Nothing to do
                break;
        }

        if( !baseUnitStr.isEmpty() )
        {
            // expected unit string matches actual unit string. remove.
            if( QString::compare(baseUnitStr, unitStr)==0 )
            {
                // Example code from: Mod/TechDraw/App/DrawViewDimension.cpp:372
                QRegExp rxUnits(QString::fromUtf8(" \\D*$"));  //space + any non digits at end of string
                valueStr.remove(rxUnits);                      //getUserString(defaultDecimals) without units
            }
        }
    }

    if (constraint->Type == Sketcher::Diameter){
        valueStr.prepend(QChar(216)); // Diameter sign
    }
    else if (constraint->Type == Sketcher::Radius){
        valueStr.prepend(QChar(82)); // Capital letter R
    }

    /**
    Create the representation string from the user defined format string
    Format options are:
    %N - the constraint name parameter
    %V - the value of the dimensional constraint, including any unit characters
    */
    if (iShowDimName && !nameStr.isEmpty())
    {
        if (formatStr.contains(QLatin1String("%V")) || formatStr.contains(QLatin1String("%N")))
        {
            presentationStr = formatStr;
            presentationStr.replace(QLatin1String("%N"), nameStr);
            presentationStr.replace(QLatin1String("%V"), valueStr);
        }
        else
        {
            // user defined format string does not contain any valid parameter, using default format "%N = %V"
            presentationStr = nameStr + QLatin1String(" = ") + valueStr;
            FC_WARN("When parsing dimensional format string \""
                    << QString(formatStr).toStdString()
                    << "\", no valid parameter found, using default format.");
        }

        return presentationStr;
    }

    return valueStr;
}

QString ViewProviderSketch::iconTypeFromConstraint(Constraint *constraint)
{
    /*! TODO: Consider pushing this functionality up into Constraint */
    switch(constraint->Type) {
    case Horizontal:
        return QString::fromLatin1("Constraint_Horizontal");
    case Vertical:
        return QString::fromLatin1("Constraint_Vertical");
    case PointOnObject:
        return QString::fromLatin1("Constraint_PointOnObject");
    case Tangent:
        return QString::fromLatin1("Constraint_Tangent");
    case Parallel:
        return QString::fromLatin1("Constraint_Parallel");
    case Perpendicular:
        return QString::fromLatin1("Constraint_Perpendicular");
    case Equal:
        return QString::fromLatin1("Constraint_EqualLength");
    case Symmetric:
        return QString::fromLatin1("Constraint_Symmetric");
    case SnellsLaw:
        return QString::fromLatin1("Constraint_SnellsLaw");
    case Block:
        return QString::fromLatin1("Constraint_Block");
    default:
        return QString();
    }
}

void ViewProviderSketch::sendConstraintIconToCoin(const QImage &icon, SoImage *soImagePtr)
{
    SoSFImage icondata = SoSFImage();

    Gui::BitmapFactory().convert(icon, icondata);

    SbVec2s iconSize(icon.width(), icon.height());

    int four = 4;
    soImagePtr->image.setValue(iconSize, 4, icondata.getValue(iconSize, four));

    //Set Image Alignment to Center
    soImagePtr->vertAlignment = SoImage::HALF;
    soImagePtr->horAlignment = SoImage::CENTER;
}

void ViewProviderSketch::clearCoinImage(SoImage *soImagePtr)
{
    soImagePtr->setToDefaults();
}

QColor ViewProviderSketch::constrColor(int constraintId)
{
    static QColor constrIcoColor((int)(ConstrIcoColor [0] * 255.0f),
                                 (int)(ConstrIcoColor[1] * 255.0f),
                                 (int)(ConstrIcoColor[2] * 255.0f));
    static QColor nonDrivingConstrIcoColor((int)(NonDrivingConstrDimColor[0] * 255.0f),
                                 (int)(NonDrivingConstrDimColor[1] * 255.0f),
                                 (int)(NonDrivingConstrDimColor[2] * 255.0f));
    static QColor constrIconSelColor ((int)(SelectColor[0] * 255.0f),
                                      (int)(SelectColor[1] * 255.0f),
                                      (int)(SelectColor[2] * 255.0f));
    static QColor constrIconPreselColor ((int)(PreselectColor[0] * 255.0f),
                                         (int)(PreselectColor[1] * 255.0f),
                                         (int)(PreselectColor[2] * 255.0f));

    static QColor constrIconDisabledColor ((int)(DeactivatedConstrDimColor[0] * 255.0f),
                                           (int)(DeactivatedConstrDimColor[1] * 255.0f),
                                           (int)(DeactivatedConstrDimColor[2] * 255.0f));

    const std::vector<Sketcher::Constraint *> &constraints = getSketchObject()->Constraints.getValues();

    if (edit->PreselectConstraintSet.count(constraintId))
        return constrIconPreselColor;
    else if (edit->SelConstraintSet.find(constraintId) != edit->SelConstraintSet.end())
        return constrIconSelColor;
    else if(!constraints[constraintId]->isActive)
        return constrIconDisabledColor;
    else if(!constraints[constraintId]->isDriving)
        return nonDrivingConstrIcoColor;
    else
        return constrIcoColor;

}

int ViewProviderSketch::constrColorPriority(int constraintId)
{
    if (edit->PreselectConstraintSet.count(constraintId))
        return 3;
    else if (edit->SelConstraintSet.find(constraintId) != edit->SelConstraintSet.end())
        return 2;
    else
        return 1;
}

// public function that triggers drawing of most constraint icons
void ViewProviderSketch::drawConstraintIcons()
{
    const std::vector<Sketcher::Constraint *> &constraints = getSketchObject()->Constraints.getValues();
    int constrId = 0;

    std::vector<constrIconQueueItem> iconQueue;

    for (std::vector<Sketcher::Constraint *>::const_iterator it=constraints.begin();
         it != constraints.end(); ++it, ++constrId) {

        // Check if Icon Should be created
        bool multipleIcons = false;

        QString icoType = iconTypeFromConstraint(*it);
        if(icoType.isEmpty())
            continue;

        switch((*it)->Type) {

        case Tangent:
            {   // second icon is available only for colinear line segments
                const Part::Geometry *geo1 = getSketchObject()->getGeometry((*it)->First);
                const Part::Geometry *geo2 = getSketchObject()->getGeometry((*it)->Second);
                if (geo1 && geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                    geo2 && geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                    multipleIcons = true;
                }
            }
            break;
        case Horizontal:
        case Vertical:
            {   // second icon is available only for point alignment
                if ((*it)->Second != GeoEnum::GeoUndef &&
                    (*it)->FirstPos != Sketcher::PointPos::none &&
                    (*it)->SecondPos != Sketcher::PointPos::none) {
                    multipleIcons = true;
                }
            }
            break;
        case Parallel:
            multipleIcons = true;
            break;
        case Perpendicular:
            // second icon is available only when there is no common point
            if ((*it)->FirstPos == Sketcher::PointPos::none && (*it)->Third == GeoEnum::GeoUndef)
                multipleIcons = true;
            break;
        case Equal:
            multipleIcons = true;
            break;
        default:
            break;
        }

        // Double-check that we can safely access the Inventor nodes
        if (constrId >= edit->constrGroup->getNumChildren()) {
            Base::Console().Warning("Can't update constraint icons because view is not in sync with sketch\n");
            break;
        }

        // Find the Constraint Icon SoImage Node
        SoSeparator *sep = static_cast<SoSeparator *>(edit->constrGroup->getChild(constrId));
        int numChildren = sep->getNumChildren();

        SbVec3f absPos;
        // Somewhat hacky - we use SoZoomTranslations for most types of icon,
        // but symmetry icons use SoTranslations...
        SoTranslation *translationPtr = static_cast<SoTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION));
        if(dynamic_cast<SoZoomTranslation *>(translationPtr))
            absPos = static_cast<SoZoomTranslation *>(translationPtr)->abPos.getValue();
        else
            absPos = translationPtr->translation.getValue();

        SoImage *coinIconPtr = dynamic_cast<SoImage *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON));
        SoInfo *infoPtr = static_cast<SoInfo *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID));

        constrIconQueueItem thisIcon;
        thisIcon.type = icoType;
        thisIcon.constraintId = constrId;
        thisIcon.position = absPos;
        thisIcon.destination = coinIconPtr;
        thisIcon.infoPtr = infoPtr;
        thisIcon.visible = (*it)->isInVirtualSpace == getIsShownVirtualSpace();

        if ((*it)->Type==Symmetric) {
            Base::Vector3d startingpoint = getSketchObject()->getPoint((*it)->First,(*it)->FirstPos);
            Base::Vector3d endpoint = getSketchObject()->getPoint((*it)->Second,(*it)->SecondPos);

            double x0,y0,x1,y1;
            SbVec3f pos0(startingpoint.x,startingpoint.y,startingpoint.z);
            SbVec3f pos1(endpoint.x,endpoint.y,endpoint.z);

            Gui::MDIView *mdi = Gui::Application::Instance->editViewOfNode(edit->EditRoot);
            if (!(mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())))
                return;
            Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();
            SoCamera* pCam = viewer->getSoRenderManager()->getCamera();
            if (!pCam)
                return;

            try {
                SbViewVolume vol = pCam->getViewVolume();

                getCoordsOnSketchPlane(x0,y0,pos0,vol.getProjectionDirection());
                getCoordsOnSketchPlane(x1,y1,pos1,vol.getProjectionDirection());

                thisIcon.iconRotation = -atan2((y1-y0),(x1-x0))*180/M_PI;
            }
            catch (const Base::DivisionByZeroError&) {
                thisIcon.iconRotation = 0;
            }
        }
        else {
            thisIcon.iconRotation = 0;
        }

        if (multipleIcons) {
            if((*it)->Name.empty())
                thisIcon.label = QString::number(constrId + 1);
            else
                thisIcon.label = QString::fromUtf8((*it)->Name.c_str());
            iconQueue.push_back(thisIcon);

            // Note that the second translation is meant to be applied after the first.
            // So, to get the position of the second icon, we add the two translations together
            //
            // See note ~30 lines up.
            if (numChildren > CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID) {
                translationPtr = static_cast<SoTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION));
                if(dynamic_cast<SoZoomTranslation *>(translationPtr))
                    thisIcon.position += static_cast<SoZoomTranslation *>(translationPtr)->abPos.getValue();
                else
                    thisIcon.position += translationPtr->translation.getValue();

                thisIcon.destination = dynamic_cast<SoImage *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON));
                thisIcon.infoPtr = static_cast<SoInfo *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID));
            }
        }
        else {
            if ((*it)->Name.empty())
                thisIcon.label = QString();
            else
                thisIcon.label = QString::fromUtf8((*it)->Name.c_str());
        }

        iconQueue.push_back(thisIcon);
    }

    combineConstraintIcons(iconQueue);
}

void ViewProviderSketch::combineConstraintIcons(IconQueue iconQueue)
{
    // getScaleFactor gives us a ratio of pixels per some kind of real units
    float maxDistSquared = pow(getScaleFactor(), 2);

    // There's room for optimisation here; we could reuse the combined icons...
    edit->combinedConstrBoxes.clear();

    while(!iconQueue.empty()) {
        // A group starts with an item popped off the back of our initial queue
        IconQueue thisGroup;
        thisGroup.push_back(iconQueue.back());
        ViewProviderSketch::constrIconQueueItem init = iconQueue.back();
        iconQueue.pop_back();

        // we group only icons not being Symmetry icons, because we want those on the line
        // and only icons that are visible
        if(init.type != QString::fromLatin1("Constraint_Symmetric") && init.visible){

            IconQueue::iterator i = iconQueue.begin();


            while(i != iconQueue.end()) {
                if((*i).visible) {
                    bool addedToGroup = false;

                    for(IconQueue::iterator j = thisGroup.begin();
                        j != thisGroup.end(); ++j) {
                        float distSquared = pow(i->position[0]-j->position[0],2) + pow(i->position[1]-j->position[1],2);
                        if(distSquared <= maxDistSquared && (*i).type != QString::fromLatin1("Constraint_Symmetric")) {
                            // Found an icon in iconQueue that's close enough to
                            // a member of thisGroup, so move it into thisGroup
                            thisGroup.push_back(*i);
                            i = iconQueue.erase(i);
                            addedToGroup = true;
                            break;
                        }
                    }

                    if(addedToGroup) {
                        if(i == iconQueue.end())
                            // We just got the last icon out of iconQueue
                            break;
                        else
                            // Start looking through the iconQueue again, in case
                            // we have an icon that's now close enough to thisGroup
                            i = iconQueue.begin();
                    } else
                        ++i;
                }
                else // if !visible we skip it
                   i++;
            }

        }

        if(thisGroup.size() == 1) {
            drawTypicalConstraintIcon(thisGroup[0]);
        }
        else {
            drawMergedConstraintIcons(thisGroup);
        }
    }
}

void ViewProviderSketch::drawMergedConstraintIcons(IconQueue iconQueue)
{
    for(IconQueue::iterator i = iconQueue.begin(); i != iconQueue.end(); ++i) {
        clearCoinImage(i->destination);
    }

    QImage compositeIcon;
    SoImage *thisDest = iconQueue[0].destination;
    SoInfo *thisInfo = iconQueue[0].infoPtr;

    // Tracks all constraint IDs that are combined into this icon
    QString idString;
    int lastVPad = 0;

    QStringList labels;
    std::vector<int> ids;
    QString thisType;
    QColor iconColor;
    QList<QColor> labelColors;
    int maxColorPriority;
    double iconRotation;

    ConstrIconBBVec boundingBoxes;
    while(!iconQueue.empty()) {
        IconQueue::iterator i = iconQueue.begin();

        labels.clear();
        labels.append(i->label);

        ids.clear();
        ids.push_back(i->constraintId);

        thisType = i->type;
        iconColor = constrColor(i->constraintId);
        labelColors.clear();
        labelColors.append(iconColor);
        iconRotation= i->iconRotation;

        maxColorPriority = constrColorPriority(i->constraintId);

        if(idString.length())
            idString.append(QString::fromLatin1(","));
        idString.append(QString::number(i->constraintId));

        i = iconQueue.erase(i);
        while(i != iconQueue.end()) {
            if(i->type != thisType) {
                ++i;
                continue;
            }

            labels.append(i->label);
            ids.push_back(i->constraintId);
            labelColors.append(constrColor(i->constraintId));

            if(constrColorPriority(i->constraintId) > maxColorPriority) {
                maxColorPriority = constrColorPriority(i->constraintId);
                iconColor= constrColor(i->constraintId);
            }

            idString.append(QString::fromLatin1(",") +
                            QString::number(i->constraintId));

            i = iconQueue.erase(i);
        }

        // To be inserted into edit->combinedConstBoxes
        std::vector<QRect> boundingBoxesVec;
        int oldHeight = 0;

        // Render the icon here.
        if(compositeIcon.isNull()) {
            compositeIcon = renderConstrIcon(thisType,
                                             iconColor,
                                             labels,
                                             labelColors,
                                             iconRotation,
                                             &boundingBoxesVec,
                                             &lastVPad);
        } else {
            int thisVPad;
            QImage partialIcon = renderConstrIcon(thisType,
                                                  iconColor,
                                                  labels,
                                                  labelColors,
                                                  iconRotation,
                                                  &boundingBoxesVec,
                                                  &thisVPad);

            // Stack vertically for now.  Down the road, it might make sense
            // to figure out the best orientation automatically.
            oldHeight = compositeIcon.height();

            // This is overkill for the currently used (20 July 2014) font,
            // since it always seems to have the same vertical pad, but this
            // might not always be the case.  The 3 pixel buffer might need
            // to vary depending on font size too...
            oldHeight -= std::max(lastVPad - 3, 0);

            compositeIcon = compositeIcon.copy(0, 0,
                                               std::max(partialIcon.width(),
                                                        compositeIcon.width()),
                                               partialIcon.height() +
                                               compositeIcon.height());

            QPainter qp(&compositeIcon);
            qp.drawImage(0, oldHeight, partialIcon);

            lastVPad = thisVPad;
        }

        // Add bounding boxes for the icon we just rendered to boundingBoxes
        std::vector<int>::iterator id = ids.begin();
        std::set<int> nextIds;
        for(std::vector<QRect>::iterator bb = boundingBoxesVec.begin();
            bb != boundingBoxesVec.end(); ++bb) {
            nextIds.clear();

            if(bb == boundingBoxesVec.begin()) {
                // The first bounding box is for the icon at left, so assign
                // all IDs for that type of constraint to the icon.
                for(std::vector<int>::iterator j = ids.begin(); j != ids.end(); ++j)
                    nextIds.insert(*j);
            }
            else {
                nextIds.insert(*(id++));
            }

            ConstrIconBB newBB(bb->adjusted(0, oldHeight, 0, oldHeight),
                               nextIds);

            boundingBoxes.push_back(newBB);
        }
    }

    edit->combinedConstrBoxes[idString] = boundingBoxes;
    thisInfo->string.setValue(idString.toLatin1().data());
    sendConstraintIconToCoin(compositeIcon, thisDest);
}


/// Note: labels, labelColors, and boundingBoxes are all
/// assumed to be the same length.
QImage ViewProviderSketch::renderConstrIcon(const QString &type,
                                            const QColor &iconColor,
                                            const QStringList &labels,
                                            const QList<QColor> &labelColors,
                                            double iconRotation,
                                            std::vector<QRect> *boundingBoxes,
                                            int *vPad)
{
    // Constants to help create constraint icons
    QString joinStr = QString::fromLatin1(", ");

    QPixmap pxMap;
    std::stringstream constraintName;
    constraintName << type.toLatin1().data() << edit->constraintIconSize; // allow resizing by embedding size
    if (! Gui::BitmapFactory().findPixmapInCache(constraintName.str().c_str(), pxMap)) {
        pxMap = Gui::BitmapFactory().pixmapFromSvg(type.toLatin1().data(),QSizeF(edit->constraintIconSize,edit->constraintIconSize));
        Gui::BitmapFactory().addPixmapToCache(constraintName.str().c_str(), pxMap); // Cache for speed, avoiding pixmapFromSvg
    }
    QImage icon = pxMap.toImage();

    QFont font = QApplication::font();
    font.setPixelSize(static_cast<int>(1.0 * edit->constraintIconSize));
    font.setBold(true);
    QFontMetrics qfm = QFontMetrics(font);

    int labelWidth = qfm.boundingRect(labels.join(joinStr)).width();
    // See Qt docs on qRect::bottom() for explanation of the +1
    int pxBelowBase = qfm.boundingRect(labels.join(joinStr)).bottom() + 1;

    if(vPad)
        *vPad = pxBelowBase;

    QTransform rotation;
    rotation.rotate(iconRotation);

    QImage roticon = icon.transformed(rotation);
    QImage image = roticon.copy(0, 0, roticon.width() + labelWidth,
                                                        roticon.height() + pxBelowBase);

    // Make a bounding box for the icon
    if(boundingBoxes)
        boundingBoxes->push_back(QRect(0, 0, roticon.width(), roticon.height()));

    // Render the Icons
    QPainter qp(&image);
    qp.setCompositionMode(QPainter::CompositionMode_SourceIn);
    qp.fillRect(roticon.rect(), iconColor);

    // Render constraint label if necessary
    if (!labels.join(QString()).isEmpty()) {
        qp.setCompositionMode(QPainter::CompositionMode_SourceOver);
        qp.setFont(font);

        int cursorOffset = 0;

        //In Python: "for label, color in zip(labels, labelColors):"
        QStringList::const_iterator labelItr;
        QString labelStr;
        QList<QColor>::const_iterator colorItr;
        QRect labelBB;
        for(labelItr = labels.begin(), colorItr = labelColors.begin();
            labelItr != labels.end() && colorItr != labelColors.end();
            ++labelItr, ++colorItr) {

            qp.setPen(*colorItr);

            if(labelItr + 1 == labels.end()) // if this is the last label
                labelStr = *labelItr;
            else
                labelStr = *labelItr + joinStr;

            // Note: text can sometimes draw to the left of the starting
            //       position, eg italic fonts.  Check QFontMetrics
            //       documentation for more info, but be mindful if the
            //       icon.width() is ever very small (or removed).
            qp.drawText(icon.width() + cursorOffset, icon.height(), labelStr);

            if(boundingBoxes) {
                labelBB = qfm.boundingRect(labelStr);
                labelBB.moveTo(icon.width() + cursorOffset,
                               icon.height() - qfm.height() + pxBelowBase);
                boundingBoxes->push_back(labelBB);
            }

            cursorOffset += Gui::QtTools::horizontalAdvance(qfm, labelStr);
        }
    }

    return image;
}

void ViewProviderSketch::drawTypicalConstraintIcon(const constrIconQueueItem &i)
{
    QColor color = constrColor(i.constraintId);

    QImage image = renderConstrIcon(i.type,
                                    color,
                                    QStringList(i.label),
                                    QList<QColor>() << color,
                                    i.iconRotation);

    i.infoPtr->string.setValue(QString::number(i.constraintId).toLatin1().data());
    sendConstraintIconToCoin(image, i.destination);
}

float ViewProviderSketch::getScaleFactor()
{
    assert(edit);
    Gui::MDIView *mdi = Gui::Application::Instance->editViewOfNode(edit->EditRoot);
    if (mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();
        SoCamera* camera = viewer->getSoRenderManager()->getCamera();
        float scale = camera->getViewVolume(camera->aspectRatio.getValue()).getWorldToScreenScale(SbVec3f(0.f, 0.f, 0.f), 0.1f) / 3;
        return scale;
    }
    else {
        return 1.f;
    }
}

void ViewProviderSketch::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    (void) rCaller;
    //ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
    if (strcmp(sReason, "ViewScalingFactor") == 0   ||
        strcmp(sReason, "MarkerSize") == 0          ||
        strcmp(sReason, "EditSketcherFontSize") == 0 ) {
        if(edit) { // only if in edit mode, if not it gets updated when entering edit mode
            initItemsSizes();
            updateInventorNodeSizes();
            rebuildConstraintsVisual();
            draw();
        }
    }
}

void ViewProviderSketch::subscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Attach(this);
}

void ViewProviderSketch::unsubscribeToParameters()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Detach(this);
}

void ViewProviderSketch::updateInventorNodeSizes()
{
    assert(edit);
    edit->PointsDrawStyle->pointSize = 8 * edit->pixelScalingFactor;
    edit->PointSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", edit->MarkerSize);
    edit->CurvesDrawStyle->lineWidth = 3 * edit->pixelScalingFactor;
    edit->RootCrossDrawStyle->lineWidth = 2 * edit->pixelScalingFactor;
    edit->EditCurvesDrawStyle->lineWidth = 3 * edit->pixelScalingFactor;
    edit->EditMarkersDrawStyle->pointSize = 8 * edit->pixelScalingFactor;
    edit->EditMarkerSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", edit->MarkerSize);
    edit->ConstraintDrawStyle->lineWidth = 1 * edit->pixelScalingFactor;
    edit->InformationDrawStyle->lineWidth = 1 * edit->pixelScalingFactor;
}

void ViewProviderSketch::initItemsSizes()
{
    //Add scaling to Constraint icons
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    double viewScalingFactor = hGrp->GetFloat("ViewScalingFactor", 1.0);
    viewScalingFactor = Base::clamp<double>(viewScalingFactor, 0.5, 5.0);
    int markersize = hGrp->GetInt("MarkerSize", 7);

    int defaultFontSizePixels = QApplication::fontMetrics().height(); // returns height in pixels, not points
    int sketcherfontSize = hGrp->GetInt("EditSketcherFontSize", defaultFontSizePixels);

    int dpi = QApplication::desktop()->logicalDpiX();

    if(edit) {
        // simple scaling factor for hardcoded pixel values in the Sketcher
        edit->pixelScalingFactor = viewScalingFactor * dpi / 96; // 96 ppi is the standard pixel density for which pixel quantities were calculated

        // Coin documentation indicates the size of a font is:
        // SoSFFloat SoFont::size        Size of font. Defaults to 10.0.
        //
        // For 2D rendered bitmap fonts (like for SoText2), this value is the height of a character in screen pixels. For 3D text, this value is the world-space coordinates height of a character in the current units setting (see documentation for SoUnits node).
        //
        // However, with hdpi monitors, the coin font labels do not respect the size passed in pixels:
        // https://forum.freecadweb.org/viewtopic.php?f=3&t=54347&p=467610#p467610
        // https://forum.freecadweb.org/viewtopic.php?f=10&t=49972&start=40#p467471
        //
        // Because I (abdullah) have  96 dpi logical, 82 dpi physical, and I see a 35px font setting for a "1" in a datum label as 34px,
        // and I see kilsore and Elyas screenshots showing 41px and 61px in higher resolution monitors for the same configuration, I think
        // that coin pixel size has to be corrected by the logical dpi of the monitor. The rationale is that: a) it obviously needs dpi
        // correction, b) with physical dpi, the ratio of representation between kilsore and me is too far away.
        //
        // This means that the following correction does not have a documented basis, but appears necessary so that the Sketcher is usable in
        // HDPI monitors.

        edit->coinFontSize = std::lround(sketcherfontSize * 96.0f / dpi);
        edit->constraintIconSize = std::lround(0.8 * sketcherfontSize);

        // For marker size the global default is used.
        //
        // Rationale:
        // -> Other WBs use the default value as is
        // -> If a user has a HDPI, they will eventually change the value for the other WBs
        // -> If we correct the value here in addition, we would get two times a resize
        edit->MarkerSize = markersize;
    }
}

void ViewProviderSketch::draw(bool temp /*=false*/, bool rebuildinformationlayer /*=true*/)
{
    assert(edit);

    // Render Geometry ===================================================
    std::vector<Base::Vector3d> Coords;
    std::vector<Base::Vector3d> Points;
    std::vector<unsigned int> Index;

    int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;
    int extGeoCount = getSketchObject()->getExternalGeometryCount();

    const std::vector<Part::Geometry *> *geomlist;
    std::vector<Part::Geometry *> tempGeo;
    if (temp)
        tempGeo = getSolvedSketch().extractGeometry(true, true); // with memory allocation
    else
        tempGeo = getSketchObject()->getCompleteGeometry(); // without memory allocation
    geomlist = &tempGeo;


    assert(int(geomlist->size()) == extGeoCount + intGeoCount);
    assert(int(geomlist->size()) >= 2);

    edit->CurvIdToGeoId.clear();
    edit->PointIdToGeoId.clear();

    edit->PointIdToGeoId.push_back(-1); // root point

    // information layer
    if(rebuildinformationlayer) {
        // every time we start with empty information layer
        Gui::coinRemoveAllChildren(edit->infoGroup);
    }

    int currentInfoNode = 0;

    ParameterGrp::handle hGrpsk = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    std::vector<int> bsplineGeoIds;

    double combrepscale = 0; // the repscale that would correspond to this comb based only on this calculation.

    // end information layer

    int GeoId = 0;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    int stdcountsegments = hGrp->GetInt("SegmentsPerGeometry", 50);
    // value cannot be smaller than 3
    if (stdcountsegments < 3)
        stdcountsegments = 3;

    // RootPoint
    Points.emplace_back(0.,0.,0.);

    for (std::vector<Part::Geometry *>::const_iterator it = geomlist->begin(); it != geomlist->end()-2; ++it, GeoId++) {
        if (GeoId >= intGeoCount)
            GeoId = -extGeoCount;
        if ((*it)->getTypeId() == Part::GeomPoint::getClassTypeId()) { // add a point
            const Part::GeomPoint *point = static_cast<const Part::GeomPoint *>(*it);
            Points.push_back(point->getPoint());
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) { // add a line
            const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(*it);
            // create the definition struct for that geom
            Coords.push_back(lineSeg->getStartPoint());
            Coords.push_back(lineSeg->getEndPoint());
            Points.push_back(lineSeg->getStartPoint());
            Points.push_back(lineSeg->getEndPoint());
            Index.push_back(2);
            edit->CurvIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) { // add a circle
            const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(*it);
            Handle(Geom_Circle) curve = Handle(Geom_Circle)::DownCast(circle->handle());
            auto gf = GeometryFacade::getFacade(circle);

            int countSegments = stdcountsegments;
            Base::Vector3d center = circle->getCenter();

            // BSpline weights have a radius corresponding to the weight value
            // However, in order for them proportional to the B-Spline size,
            // the scenograph has a size scalefactor times the weight
            //
            // This code produces the scaled up version of the geometry for the scenograph
            if(gf->getInternalType() == InternalType::BSplineControlPoint) {
                for( auto c : getSketchObject()->Constraints.getValues()) {
                    if( c->Type == InternalAlignment && c->AlignmentType == BSplineControlPoint && c->First == GeoId) {
                        auto bspline = dynamic_cast<const Part::GeomBSplineCurve *>((*geomlist)[c->Second]);

                        if(bspline){
                            auto weights = bspline->getWeights();

                            double weight = 1.0;
                            if(c->InternalAlignmentIndex < int(weights.size()))
                                weight =  weights[c->InternalAlignmentIndex];

                            // tentative scaling factor:
                            // proportional to the length of the bspline
                            // inversely proportional to the number of poles
                            double scalefactor = bspline->length(bspline->getFirstParameter(), bspline->getLastParameter())/10.0/weights.size();

                            double vradius = weight*scalefactor;
                            if(!bspline->isRational()) {
                                // OCCT sets the weights to 1.0 if a bspline is non-rational, but if the user has a weight constraint on any
                                // pole it would cause a visual artifact of having a constraint with a different radius and an unscaled circle
                                // so better scale the circles.
                                std::vector<int> polegeoids;
                                polegeoids.reserve(weights.size());

                                for ( auto ic : getSketchObject()->Constraints.getValues()) {
                                    if( ic->Type == InternalAlignment && ic->AlignmentType == BSplineControlPoint && ic->Second == c->Second) {
                                        polegeoids.push_back(ic->First);
                                    }
                                }

                                for ( auto ic : getSketchObject()->Constraints.getValues()) {
                                    if( ic->Type == Weight ) {
                                        auto pos = std::find(polegeoids.begin(), polegeoids.end(), ic->First);

                                        if(pos != polegeoids.end()) {
                                            vradius = ic->getValue() * scalefactor;
                                            break; // one is enough, otherwise it would not be non-rational
                                        }
                                    }
                                }
                            }

                            // virtual circle or radius vradius
                            auto mcurve = [&center, vradius](double param, double &x, double &y) {
                                x = center.x + vradius*cos(param);
                                y = center.y + vradius*sin(param);
                            };

                            double x;
                            double y;
                            for (int i=0; i < countSegments; i++) {
                                double param = 2*M_PI*i/countSegments;
                                mcurve(param,x,y);
                                Coords.emplace_back(x, y, 0);
                            }

                            mcurve(0,x,y);
                            Coords.emplace_back(x, y, 0);

                            // save scale factor for any prospective dragging operation
                            // 1. Solver must be updated, in case a dragging operation starts
                            // 2. if temp geometry is being used (with memory allocation), then the copy we have here must be updated. If
                            //    no temp geometry is being used, then the normal geometry must be updated.
                            {// make solver be ready for a dragging operation
                                auto vpext = std::make_unique<SketcherGui::ViewProviderSketchGeometryExtension>();
                                vpext->setRepresentationFactor(scalefactor);

                                getSketchObject()->updateSolverExtension(GeoId, std::move(vpext));
                            }

                            if(!circle->hasExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()))
                            {
                                // It is ok to add this kind of extension to a const geometry because:
                                // 1. It does not modify the object in a way that affects property state, just ViewProvider representation
                                // 2. If it is lost (for example upon undo), redrawing will reinstate it with the correct value
                                const_cast<Part::GeomCircle *>(circle)->setExtension(std::make_unique<SketcherGui::ViewProviderSketchGeometryExtension>());
                            }

                            auto vpext = std::const_pointer_cast<SketcherGui::ViewProviderSketchGeometryExtension>(
                                            std::static_pointer_cast<const SketcherGui::ViewProviderSketchGeometryExtension>(
                                                circle->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()).lock()));

                            vpext->setRepresentationFactor(scalefactor);
                        }
                        break;
                    }
                }
            }
            else {

                double segment = (2 * M_PI) / countSegments;

                for (int i=0; i < countSegments; i++) {
                    gp_Pnt pnt = curve->Value(i*segment);
                    Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                }

                gp_Pnt pnt = curve->Value(0);
                Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
            }

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(center);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) { // add an ellipse
            const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(*it);
            Handle(Geom_Ellipse) curve = Handle(Geom_Ellipse)::DownCast(ellipse->handle());

            int countSegments = stdcountsegments;
            Base::Vector3d center = ellipse->getCenter();
            double segment = (2 * M_PI) / countSegments;
            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(i*segment);
                Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
            }

            gp_Pnt pnt = curve->Value(0);
            Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(center);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) { // add an arc
            const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(*it);
            Handle(Geom_TrimmedCurve) curve = Handle(Geom_TrimmedCurve)::DownCast(arc->handle());

            double startangle, endangle;
            arc->getRange(startangle, endangle, /*emulateCCW=*/false);
            if (startangle > endangle) // if arc is reversed
                std::swap(startangle, endangle);

            double range = endangle-startangle;
            int countSegments = std::max(6, int(stdcountsegments * range / (2 * M_PI)));
            double segment = range / countSegments;

            Base::Vector3d center = arc->getCenter();
            Base::Vector3d start  = arc->getStartPoint(/*emulateCCW=*/true);
            Base::Vector3d end    = arc->getEndPoint(/*emulateCCW=*/true);

            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(startangle);
                Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                startangle += segment;
            }

            // end point
            gp_Pnt pnt = curve->Value(endangle);
            Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(start);
            Points.push_back(end);
            Points.push_back(center);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) { // add an arc
            const Part::GeomArcOfEllipse *arc = static_cast<const Part::GeomArcOfEllipse *>(*it);
            Handle(Geom_TrimmedCurve) curve = Handle(Geom_TrimmedCurve)::DownCast(arc->handle());

            double startangle, endangle;
            arc->getRange(startangle, endangle, /*emulateCCW=*/false);
            if (startangle > endangle) // if arc is reversed
                std::swap(startangle, endangle);

            double range = endangle-startangle;
            int countSegments = std::max(6, int(stdcountsegments * range / (2 * M_PI)));
            double segment = range / countSegments;

            Base::Vector3d center = arc->getCenter();
            Base::Vector3d start  = arc->getStartPoint(/*emulateCCW=*/true);
            Base::Vector3d end    = arc->getEndPoint(/*emulateCCW=*/true);

            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(startangle);
                Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                startangle += segment;
            }

            // end point
            gp_Pnt pnt = curve->Value(endangle);
            Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(start);
            Points.push_back(end);
            Points.push_back(center);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(*it);
            Handle(Geom_TrimmedCurve) curve = Handle(Geom_TrimmedCurve)::DownCast(aoh->handle());

            double startangle, endangle;
            aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
            if (startangle > endangle) // if arc is reversed
                std::swap(startangle, endangle);

            double range = endangle-startangle;
            int countSegments = std::max(6, int(stdcountsegments * range / (2 * M_PI)));
            double segment = range / countSegments;

            Base::Vector3d center = aoh->getCenter();
            Base::Vector3d start  = aoh->getStartPoint();
            Base::Vector3d end    = aoh->getEndPoint();

            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(startangle);
                Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                startangle += segment;
            }

            // end point
            gp_Pnt pnt = curve->Value(endangle);
            Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(start);
            Points.push_back(end);
            Points.push_back(center);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(*it);
            Handle(Geom_TrimmedCurve) curve = Handle(Geom_TrimmedCurve)::DownCast(aop->handle());

            double startangle, endangle;
            aop->getRange(startangle, endangle, /*emulateCCW=*/true);
            if (startangle > endangle) // if arc is reversed
                std::swap(startangle, endangle);

            double range = endangle-startangle;
            int countSegments = std::max(6, int(stdcountsegments * range / (2 * M_PI)));
            double segment = range / countSegments;

            Base::Vector3d center = aop->getCenter();
            Base::Vector3d start  = aop->getStartPoint();
            Base::Vector3d end    = aop->getEndPoint();

            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(startangle);
                Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                startangle += segment;
            }

            // end point
            gp_Pnt pnt = curve->Value(endangle);
            Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(start);
            Points.push_back(end);
            Points.push_back(center);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) { // add a bspline
            bsplineGeoIds.push_back(GeoId);
            const Part::GeomBSplineCurve *spline = static_cast<const Part::GeomBSplineCurve *>(*it);
            Handle(Geom_BSplineCurve) curve = Handle(Geom_BSplineCurve)::DownCast(spline->handle());

            Base::Vector3d startp  = spline->getStartPoint();
            Base::Vector3d endp    = spline->getEndPoint();

            double first = curve->FirstParameter();
            double last = curve->LastParameter();
            if (first > last) // if arc is reversed
                std::swap(first, last);

            double range = last-first;
            int countSegments = stdcountsegments;
            double segment = range / countSegments;

            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(first);
                Coords.emplace_back(pnt.X(), pnt.Y(), pnt.Z());
                first += segment;
            }

            // end point
            gp_Pnt end = curve->Value(last);
            Coords.emplace_back(end.X(), end.Y(), end.Z());

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(startp);
            Points.push_back(endp);
            edit->PointIdToGeoId.push_back(GeoId);
            edit->PointIdToGeoId.push_back(GeoId);

            //***************************************************************************************************************
            // global information gathering for geometry information layer

            std::vector<Base::Vector3d> poles = spline->getPoles();

            Base::Vector3d midp = Base::Vector3d(0,0,0);

            for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it) {
                midp += (*it);
            }

            midp /= poles.size();

            double firstparam = spline->getFirstParameter();
            double lastparam =  spline->getLastParameter();

            const int ndiv = poles.size()>4?poles.size()*16:64;
            double step = (lastparam - firstparam ) / (ndiv -1);

            std::vector<double> paramlist(ndiv);
            std::vector<Base::Vector3d> pointatcurvelist(ndiv);
            std::vector<double> curvaturelist(ndiv);
            std::vector<Base::Vector3d> normallist(ndiv);

            double maxcurv = 0;
            double maxdisttocenterofmass = 0;

            for (int i = 0; i < ndiv; i++) {
                paramlist[i] = firstparam + i * step;
                pointatcurvelist[i] = spline->pointAtParameter(paramlist[i]);

                try {
                    curvaturelist[i] = spline->curvatureAt(paramlist[i]);
                }
                catch(Base::CADKernelError &e) {
                    // it is "just" a visualisation matter OCC could not calculate the curvature
                    // terminating here would mean that the other shapes would not be drawn.
                    // Solution: Report the issue and set dummy curvature to 0
                    e.ReportException();
                    Base::Console().Error("Curvature graph for B-Spline with GeoId=%d could not be calculated.\n", GeoId);
                    curvaturelist[i] = 0;
                }

                if (curvaturelist[i] > maxcurv)
                    maxcurv = curvaturelist[i];

                double tempf = ( pointatcurvelist[i] - midp ).Length();

                if (tempf > maxdisttocenterofmass)
                    maxdisttocenterofmass = tempf;

            }

            double temprepscale = 0;
            if (maxcurv > 0)
                temprepscale = (0.5 * maxdisttocenterofmass) / maxcurv; // just a factor to make a comb reasonably visible

            if (temprepscale > combrepscale)
                combrepscale = temprepscale;
        }
    }

    if ( (combrepscale > (2 * combrepscalehyst)) || (combrepscale < (combrepscalehyst/2)))
        combrepscalehyst = combrepscale ;


    // geometry information layer for bsplines, as they need a second round now that max curvature is known
    for (std::vector<int>::const_iterator it = bsplineGeoIds.begin(); it != bsplineGeoIds.end(); ++it) {

        const Part::Geometry *geo = GeoById(*geomlist, *it);

        const Part::GeomBSplineCurve *spline = static_cast<const Part::GeomBSplineCurve *>(geo);

        //----------------------------------------------------------
        // geometry information layer

        // polynom degree --------------------------------------------------------
        std::vector<Base::Vector3d> poles = spline->getPoles();

        Base::Vector3d midp = Base::Vector3d(0,0,0);

        for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it) {
            midp += (*it);
        }

        midp /= poles.size();

        if (rebuildinformationlayer) {
            SoSwitch *sw = new SoSwitch();

            sw->whichChild = hGrpsk->GetBool("BSplineDegreeVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = new SoSeparator();
            sep->ref();
            // no caching for frequently-changing data structures
            sep->renderCaching = SoSeparator::OFF;

            // every information visual node gets its own material for to-be-implemented preselection and selection
            SoMaterial *mat = new SoMaterial;
            mat->ref();
            mat->diffuseColor = InformationColor;

            SoTranslation *translate = new SoTranslation;

            translate->translation.setValue(midp.x,midp.y,zInfo);

            SoFont *font = new SoFont;
            font->name.setValue("Helvetica");
            font->size.setValue(edit->coinFontSize);

            SoText2 *degreetext = new SoText2;
            degreetext->string = SbString(spline->getDegree());

            sep->addChild(translate);
            sep->addChild(mat);
            sep->addChild(font);
            sep->addChild(degreetext);

            sw->addChild(sep);

            edit->infoGroup->addChild(sw);
            sep->unref();
            mat->unref();
        }
        else {
            SoSwitch *sw = static_cast<SoSwitch *>(edit->infoGroup->getChild(currentInfoNode));

            if (visibleInformationChanged)
                sw->whichChild = hGrpsk->GetBool("BSplineDegreeVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

            static_cast<SoTranslation *>(sep->getChild(GEOINFO_BSPLINE_DEGREE_POS))->translation.setValue(midp.x,midp.y,zInfo);

            static_cast<SoText2 *>(sep->getChild(GEOINFO_BSPLINE_DEGREE_TEXT))->string = SbString(spline->getDegree());
        }

        currentInfoNode++; // switch to next node

        // control polygon --------------------------------------------------------
        if (rebuildinformationlayer) {
            SoSwitch *sw = new SoSwitch();

            sw->whichChild = hGrpsk->GetBool("BSplineControlPolygonVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = new SoSeparator();
            sep->ref();
            // no caching for frequently-changing data structures
            sep->renderCaching = SoSeparator::OFF;

            // every information visual node gets its own material for to-be-implemented preselection and selection
            SoMaterial *mat = new SoMaterial;
            mat->ref();
            mat->diffuseColor = InformationColor;

            SoLineSet *polygon = new SoLineSet;

            SoCoordinate3 *polygoncoords = new SoCoordinate3;

            if (spline->isPeriodic()) {
                polygoncoords->point.setNum(poles.size()+1);
            }
            else {
                polygoncoords->point.setNum(poles.size());
            }

            SbVec3f *vts = polygoncoords->point.startEditing();

            int i=0;
            for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it, i++) {
                vts[i].setValue((*it).x,(*it).y,zInfo);
            }

            if (spline->isPeriodic()) {
                vts[poles.size()].setValue(poles[0].x,poles[0].y,zInfo);
            }

            polygoncoords->point.finishEditing();

            sep->addChild(mat);
            sep->addChild(polygoncoords);
            sep->addChild(polygon);

            sw->addChild(sep);

            edit->infoGroup->addChild(sw);
            sep->unref();
            mat->unref();
        }
        else {
            SoSwitch *sw = static_cast<SoSwitch *>(edit->infoGroup->getChild(currentInfoNode));

            if(visibleInformationChanged)
                sw->whichChild = hGrpsk->GetBool("BSplineControlPolygonVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

            SoCoordinate3 *polygoncoords = static_cast<SoCoordinate3 *>(sep->getChild(GEOINFO_BSPLINE_POLYGON));

            if(spline->isPeriodic()) {
                polygoncoords->point.setNum(poles.size()+1);
            }
            else {
                polygoncoords->point.setNum(poles.size());
            }

            SbVec3f *vts = polygoncoords->point.startEditing();

            int i=0;
            for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it, i++) {
                vts[i].setValue((*it).x,(*it).y,zInfo);
            }

            if(spline->isPeriodic()) {
                vts[poles.size()].setValue(poles[0].x,poles[0].y,zInfo);
            }

            polygoncoords->point.finishEditing();

        }
        currentInfoNode++; // switch to next node

        // curvature graph --------------------------------------------------------

        // reimplementation of python source:
        // https://github.com/tomate44/CurvesWB/blob/master/ParametricComb.py
        // by FreeCAD user Chris_G

        double firstparam = spline->getFirstParameter();
        double lastparam =  spline->getLastParameter();

        const int ndiv = poles.size()>4?poles.size()*16:64;
        double step = (lastparam - firstparam ) / (ndiv -1);

        std::vector<double> paramlist(ndiv);
        std::vector<Base::Vector3d> pointatcurvelist(ndiv);
        std::vector<double> curvaturelist(ndiv);
        std::vector<Base::Vector3d> normallist(ndiv);

        for(int i = 0; i < ndiv; i++) {
            paramlist[i] = firstparam + i * step;
            pointatcurvelist[i] = spline->pointAtParameter(paramlist[i]);

            try {
                curvaturelist[i] = spline->curvatureAt(paramlist[i]);
            }
            catch(Base::CADKernelError &e) {
                // it is "just" a visualisation matter OCC could not calculate the curvature
                // terminating here would mean that the other shapes would not be drawn.
                // Solution: Report the issue and set dummy curvature to 0
                e.ReportException();
                Base::Console().Error("Curvature graph for B-Spline with GeoId=%d could not be calculated.\n", GeoId);
                curvaturelist[i] = 0;
            }

            try {
                spline->normalAt(paramlist[i],normallist[i]);
            }
            catch(Base::Exception&) {
                normallist[i] = Base::Vector3d(0,0,0);
            }

        }

        std::vector<Base::Vector3d> pointatcomblist(ndiv);

        for(int i = 0; i < ndiv; i++) {
            pointatcomblist[i] = pointatcurvelist[i] - combrepscalehyst * curvaturelist[i] * normallist[i];
        }

        if (rebuildinformationlayer) {
            SoSwitch *sw = new SoSwitch();

            sw->whichChild = hGrpsk->GetBool("BSplineCombVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = new SoSeparator();
            sep->ref();
            // no caching for frequently-changing data structures
            sep->renderCaching = SoSeparator::OFF;

            // every information visual node gets its own material for to-be-implemented preselection and selection
            SoMaterial *mat = new SoMaterial;
            mat->ref();
            mat->diffuseColor = InformationColor;

            SoLineSet *comblineset = new SoLineSet;

            SoCoordinate3 *combcoords = new SoCoordinate3;

            combcoords->point.setNum(3*ndiv); // 2*ndiv +1 points of ndiv separate segments + ndiv points for last segment
            comblineset->numVertices.setNum(ndiv+1); // ndiv separate segments of radials + 1 segment connecting at comb end

            int32_t *index = comblineset->numVertices.startEditing();
            SbVec3f *vts = combcoords->point.startEditing();

            for(int i = 0; i < ndiv; i++) {
                vts[2*i].setValue(pointatcurvelist[i].x, pointatcurvelist[i].y, zInfo); // radials
                vts[2*i+1].setValue(pointatcomblist[i].x, pointatcomblist[i].y, zInfo);
                index[i] = 2;

                vts[2*ndiv+i].setValue(pointatcomblist[i].x, pointatcomblist[i].y, zInfo); // comb endpoint closing segment
            }

            index[ndiv] = ndiv; // comb endpoint closing segment

            combcoords->point.finishEditing();
            comblineset->numVertices.finishEditing();

            sep->addChild(mat);
            sep->addChild(combcoords);
            sep->addChild(comblineset);

            sw->addChild(sep);

            edit->infoGroup->addChild(sw);
            sep->unref();
            mat->unref();
        }
        else {
            SoSwitch *sw = static_cast<SoSwitch *>(edit->infoGroup->getChild(currentInfoNode));

            if(visibleInformationChanged)
                sw->whichChild = hGrpsk->GetBool("BSplineCombVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

            SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

            SoCoordinate3 *combcoords = static_cast<SoCoordinate3 *>(sep->getChild(GEOINFO_BSPLINE_POLYGON));

            SoLineSet *comblineset = static_cast<SoLineSet *>(sep->getChild(GEOINFO_BSPLINE_POLYGON+1));

            combcoords->point.setNum(3*ndiv); // 2*ndiv +1 points of ndiv separate segments + ndiv points for last segment
            comblineset->numVertices.setNum(ndiv+1); // ndiv separate segments of radials + 1 segment connecting at comb end

            int32_t *index = comblineset->numVertices.startEditing();
            SbVec3f *vts = combcoords->point.startEditing();

            for(int i = 0; i < ndiv; i++) {
                vts[2*i].setValue(pointatcurvelist[i].x, pointatcurvelist[i].y, zInfo); // radials
                vts[2*i+1].setValue(pointatcomblist[i].x, pointatcomblist[i].y, zInfo);
                index[i] = 2;

                vts[2*ndiv+i].setValue(pointatcomblist[i].x, pointatcomblist[i].y, zInfo); // comb endpoint closing segment
            }

            index[ndiv] = ndiv; // comb endpoint closing segment

            combcoords->point.finishEditing();
            comblineset->numVertices.finishEditing();

        }

        currentInfoNode++; // switch to next node

        // knot multiplicity --------------------------------------------------------
        std::vector<double> knots = spline->getKnots();
        std::vector<int> mult = spline->getMultiplicities();

        std::vector<double>::const_iterator itk;
        std::vector<int>::const_iterator itm;


        if (rebuildinformationlayer) {

            for( itk = knots.begin(), itm = mult.begin(); itk != knots.end() && itm != mult.end(); ++itk, ++itm) {

                SoSwitch *sw = new SoSwitch();

                sw->whichChild = hGrpsk->GetBool("BSplineKnotMultiplicityVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

                SoSeparator *sep = new SoSeparator();
                sep->ref();
                // no caching for frequently-changing data structures
                sep->renderCaching = SoSeparator::OFF;

                // every information visual node gets its own material for to-be-implemented preselection and selection
                SoMaterial *mat = new SoMaterial;
                mat->ref();
                mat->diffuseColor = InformationColor;

                SoTranslation *translate = new SoTranslation;

                Base::Vector3d knotposition = spline->pointAtParameter(*itk);

                translate->translation.setValue(knotposition.x, knotposition.y, zInfo);

                SoFont *font = new SoFont;
                font->name.setValue("Helvetica");
                font->size.setValue(edit->coinFontSize);

                SoText2 *degreetext = new SoText2;
                degreetext->string = SbString("(") + SbString(*itm) + SbString(")");

                sep->addChild(translate);
                sep->addChild(mat);
                sep->addChild(font);
                sep->addChild(degreetext);

                sw->addChild(sep);

                edit->infoGroup->addChild(sw);
                sep->unref();
                mat->unref();

                currentInfoNode++; // switch to next node
            }
        }
        else {
            for( itk = knots.begin(), itm = mult.begin(); itk != knots.end() && itm != mult.end(); ++itk, ++itm) {
                SoSwitch *sw = static_cast<SoSwitch *>(edit->infoGroup->getChild(currentInfoNode));

                if(visibleInformationChanged)
                    sw->whichChild = hGrpsk->GetBool("BSplineKnotMultiplicityVisible", true)?SO_SWITCH_ALL:SO_SWITCH_NONE;

                SoSeparator *sep = static_cast<SoSeparator *>(sw->getChild(0));

                Base::Vector3d knotposition = spline->pointAtParameter(*itk);

                static_cast<SoTranslation *>(sep->getChild(GEOINFO_BSPLINE_DEGREE_POS))->translation.setValue(knotposition.x,knotposition.y,zInfo);

                static_cast<SoText2 *>(sep->getChild(GEOINFO_BSPLINE_DEGREE_TEXT))->string = SbString("(") + SbString(*itm) + SbString(")");

                currentInfoNode++; // switch to next node
            }
        }

        // End of knot multiplicity

        // pole weights --------------------------------------------------------
        std::vector<double> weights = spline->getWeights();

        if (rebuildinformationlayer) {

            for (size_t index = 0; index < weights.size(); ++index) {

                SoSwitch* sw = new SoSwitch();

                sw->whichChild = hGrpsk->GetBool("BSplinePoleWeightVisible", true) ? SO_SWITCH_ALL : SO_SWITCH_NONE;

                SoSeparator* sep = new SoSeparator();
                sep->ref();
                // no caching for frequently-changing data structures
                sep->renderCaching = SoSeparator::OFF;

                // every information visual node gets its own material for to-be-implemented preselection and selection
                SoMaterial* mat = new SoMaterial;
                mat->ref();
                mat->diffuseColor = InformationColor;

                SoTranslation* translate = new SoTranslation;

                Base::Vector3d poleposition = poles[index];

                SoFont* font = new SoFont;
                font->name.setValue("Helvetica");
                font->size.setValue(edit->coinFontSize);

                translate->translation.setValue(poleposition.x, poleposition.y, zInfo);

                // set up string with weight value and the user-defined number of decimals
                QString WeightString  = QString::fromLatin1("%1").arg(weights[index], 0, 'f', Base::UnitsApi::getDecimals());

                SoText2* WeightText = new SoText2;
                // since the first and last control point of a spline is also treated as knot and thus
                // can also have a displayed multiplicity, we must assure the multiplicity is not visibly overwritten
                // therefore be output the weight in a second line
                SoMFString label;
                label.set1Value(0, SbString(""));
                label.set1Value(1, SbString("[") + SbString(WeightString.toStdString().c_str()) + SbString("]"));
                WeightText->string = label;

                sep->addChild(translate);
                sep->addChild(mat);
                sep->addChild(font);
                sep->addChild(WeightText);

                sw->addChild(sep);

                edit->infoGroup->addChild(sw);
                sep->unref();
                mat->unref();

                currentInfoNode++; // switch to next node
            }
        }
        else {
            for (size_t index = 0; index < weights.size(); ++index) {
                SoSwitch* sw = static_cast<SoSwitch*>(edit->infoGroup->getChild(currentInfoNode));

                if (visibleInformationChanged)
                    sw->whichChild = hGrpsk->GetBool("BSplinePoleWeightVisible", true) ? SO_SWITCH_ALL : SO_SWITCH_NONE;

                SoSeparator* sep = static_cast<SoSeparator*>(sw->getChild(0));

                Base::Vector3d poleposition = poles[index];

                static_cast<SoTranslation*>(sep->getChild(GEOINFO_BSPLINE_DEGREE_POS))
                    ->translation.setValue(poleposition.x, poleposition.y, zInfo);

                // set up string with weight value and the user-defined number of decimals
                QString WeightString = QString::fromLatin1("%1").arg(weights[index], 0, 'f', Base::UnitsApi::getDecimals());

                // since the first and last control point of a spline is also treated as knot and thus
                // can also have a displayed multiplicity, we must assure the multiplicity is not visibly overwritten
                // therefore be output the weight in a second line
                SoMFString label;
                label.set1Value(0, SbString(""));
                label.set1Value(1, SbString("[") + SbString(WeightString.toStdString().c_str()) + SbString("]"));

                static_cast<SoText2*>(sep->getChild(GEOINFO_BSPLINE_DEGREE_TEXT))
                                        ->string = label;

                currentInfoNode++; // switch to next node
            }
        }

        // End of pole weights
    }



    visibleInformationChanged=false; // whatever that changed in Information layer is already updated

    edit->CurvesCoordinate->point.setNum(Coords.size());
    edit->CurveSet->numVertices.setNum(Index.size());
    edit->CurvesMaterials->diffuseColor.setNum(Index.size());
    edit->PointsCoordinate->point.setNum(Points.size());
    edit->PointsMaterials->diffuseColor.setNum(Points.size());

    SbVec3f *verts = edit->CurvesCoordinate->point.startEditing();
    int32_t *index = edit->CurveSet->numVertices.startEditing();
    SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();

    float dMg = 100;

    int i=0; // setting up the line set
    for (std::vector<Base::Vector3d>::const_iterator it = Coords.begin(); it != Coords.end(); ++it,i++) {
        dMg = dMg>std::abs(it->x)?dMg:std::abs(it->x);
        dMg = dMg>std::abs(it->y)?dMg:std::abs(it->y);
        verts[i].setValue(it->x,it->y,zLowLines);
    }

    i=0; // setting up the indexes of the line set
    for (std::vector<unsigned int>::const_iterator it = Index.begin(); it != Index.end(); ++it,i++)
        index[i] = *it;

    i=0; // setting up the point set
    for (std::vector<Base::Vector3d>::const_iterator it = Points.begin(); it != Points.end(); ++it,i++){
        dMg = dMg>std::abs(it->x)?dMg:std::abs(it->x);
        dMg = dMg>std::abs(it->y)?dMg:std::abs(it->y);
        pverts[i].setValue(it->x,it->y,zLowPoints);
    }

    edit->CurvesCoordinate->point.finishEditing();
    edit->CurveSet->numVertices.finishEditing();
    edit->PointsCoordinate->point.finishEditing();

    // set cross coordinates
    edit->RootCrossSet->numVertices.set1Value(0,2);
    edit->RootCrossSet->numVertices.set1Value(1,2);

    // This code relies on Part2D, which is generally not updated in no update mode.
    // Additionally it does not relate to the actual sketcher geometry.

    /*
    Base::Console().Log("MinX:%d,MaxX:%d,MinY:%d,MaxY:%d\n",MinX,MaxX,MinY,MaxY);
    // make sure that nine of the numbers are exactly zero because log(0)
    // is not defined
    float xMin = std::abs(MinX) < FLT_EPSILON ? 0.01f : MinX;
    float xMax = std::abs(MaxX) < FLT_EPSILON ? 0.01f : MaxX;
    float yMin = std::abs(MinY) < FLT_EPSILON ? 0.01f : MinY;
    float yMax = std::abs(MaxY) < FLT_EPSILON ? 0.01f : MaxY;
    */

    float dMagF = exp(ceil(log(std::abs(dMg))));

    updateGridExtent(-dMagF, dMagF, -dMagF, dMagF);

    edit->RootCrossCoordinate->point.set1Value(0,SbVec3f(-dMagF, 0.0f, zCross));
    edit->RootCrossCoordinate->point.set1Value(1,SbVec3f(dMagF, 0.0f, zCross));
    edit->RootCrossCoordinate->point.set1Value(2,SbVec3f(0.0f, -dMagF, zCross));
    edit->RootCrossCoordinate->point.set1Value(3,SbVec3f(0.0f, dMagF, zCross));

    // Render Constraints ===================================================
    const std::vector<Sketcher::Constraint *> &constrlist = getSketchObject()->Constraints.getValues();
    // After an undo/redo it can happen that we have an empty geometry list but a non-empty constraint list
    // In this case just ignore the constraints. (See bug #0000421)
    if (geomlist->size() <= 2 && !constrlist.empty()) {
        rebuildConstraintsVisual();
        return;
    }
    // reset point if the constraint type has changed
Restart:
    // check if a new constraint arrived
    if (constrlist.size() != edit->vConstrType.size())
        rebuildConstraintsVisual();
    assert(int(constrlist.size()) == edit->constrGroup->getNumChildren());
    assert(int(edit->vConstrType.size()) == edit->constrGroup->getNumChildren());
    // update the virtual space
    updateVirtualSpace();
    // go through the constraints and update the position
    i = 0;
    for (std::vector<Sketcher::Constraint *>::const_iterator it=constrlist.begin();
         it != constrlist.end(); ++it, i++) {
        // check if the type has changed
        if ((*it)->Type != edit->vConstrType[i]) {
            // clearing the type vector will force a rebuild of the visual nodes
            edit->vConstrType.clear();
            //TODO: The 'goto' here is unsafe as it can happen that we cause an endless loop (see bug #0001956).
            goto Restart;
        }
        try{//because calculateNormalAtPoint, used in there, can throw
            // root separator for this constraint
            SoSeparator *sep = static_cast<SoSeparator *>(edit->constrGroup->getChild(i));
            const Constraint *Constr = *it;

            if(Constr->First < -extGeoCount || Constr->First >= intGeoCount
                    || (Constr->Second!=GeoEnum::GeoUndef
                        && (Constr->Second < -extGeoCount || Constr->Second >= intGeoCount))
                    || (Constr->Third!=GeoEnum::GeoUndef
                        && (Constr->Third < -extGeoCount || Constr->Third >= intGeoCount)))
            {
                // Constraint can refer to non-existent geometry during undo/redo
                continue;
            }

            // distinguish different constraint types to build up
            switch (Constr->Type) {
                case Block:
                case Horizontal: // write the new position of the Horizontal constraint Same as vertical position.
                case Vertical: // write the new position of the Vertical constraint
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        bool alignment = Constr->Type!=Block && Constr->Second != GeoEnum::GeoUndef;

                        // get the geometry
                        const Part::Geometry *geo = GeoById(*geomlist, Constr->First);

                        if (!alignment) {
                            // Vertical & Horiz can only be a GeomLineSegment, but Blocked can be anything.
                            Base::Vector3d midpos;
                            Base::Vector3d dir;
                            Base::Vector3d norm;

                            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);

                                // calculate the half distance between the start and endpoint
                                midpos = ((lineSeg->getEndPoint()+lineSeg->getStartPoint())/2);

                                //Get a set of vectors perpendicular and tangential to these
                                dir = (lineSeg->getEndPoint()-lineSeg->getStartPoint()).Normalize();

                                norm = Base::Vector3d(-dir.y,dir.x,0);
                            }
                            else if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                                const Part::GeomBSplineCurve *bsp = static_cast<const Part::GeomBSplineCurve *>(geo);
                                midpos = Base::Vector3d(0,0,0);

                                std::vector<Base::Vector3d> poles = bsp->getPoles();

                                // Move center of gravity towards start not to collide with bspline degree information.
                                double ws = 1.0 / poles.size();
                                double w = 1.0;

                                for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it) {
                                    midpos += w*(*it);
                                    w -= ws;
                                }

                                midpos /= poles.size();

                                dir = (bsp->getEndPoint() - bsp->getStartPoint()).Normalize();
                                norm = Base::Vector3d(-dir.y,dir.x,0);
                            }
                            else {
                                double ra=0,rb=0;
                                double angle,angleplus=0.;//angle = rotation of object as a whole; angleplus = arc angle (t parameter for ellipses).
                                if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                    const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo);
                                    ra = circle->getRadius();
                                    angle = M_PI/4;
                                    midpos = circle->getCenter();
                                } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                    const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                                    ra = arc->getRadius();
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle = (startangle + endangle)/2;
                                    midpos = arc->getCenter();
                                } else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo);
                                    ra = ellipse->getMajorRadius();
                                    rb = ellipse->getMinorRadius();
                                    Base::Vector3d majdir = ellipse->getMajorAxisDir();
                                    angle = atan2(majdir.y, majdir.x);
                                    angleplus = M_PI/4;
                                    midpos = ellipse->getCenter();
                                } else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                                    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo);
                                    ra = aoe->getMajorRadius();
                                    rb = aoe->getMinorRadius();
                                    double startangle, endangle;
                                    aoe->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoe->getMajorAxisDir();
                                    angle = atan2(majdir.y, majdir.x);
                                    angleplus = (startangle + endangle)/2;
                                    midpos = aoe->getCenter();
                                } else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                                    const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(geo);
                                    ra = aoh->getMajorRadius();
                                    rb = aoh->getMinorRadius();
                                    double startangle, endangle;
                                    aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoh->getMajorAxisDir();
                                    angle = atan2(majdir.y, majdir.x);
                                    angleplus = (startangle + endangle)/2;
                                    midpos = aoh->getCenter();
                                } else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                                    const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(geo);
                                    ra = aop->getFocal();
                                    double startangle, endangle;
                                    aop->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = - aop->getXAxisDir();
                                    angle = atan2(majdir.y, majdir.x);
                                    angleplus = (startangle + endangle)/2;
                                    midpos = aop->getFocus();
                                } else
                                    break;

                                if( geo->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                                    geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                                    geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ){

                                    Base::Vector3d majDir, minDir, rvec;
                                    majDir = Base::Vector3d(cos(angle),sin(angle),0);//direction of major axis of ellipse
                                    minDir = Base::Vector3d(-majDir.y,majDir.x,0);//direction of minor axis of ellipse
                                    rvec = (ra*cos(angleplus)) * majDir   +   (rb*sin(angleplus)) * minDir;
                                    midpos += rvec;
                                    rvec.Normalize();
                                    norm = rvec;
                                    dir = Base::Vector3d(-rvec.y,rvec.x,0);//DeepSOIC: I'm not sure what dir is supposed to mean.
                                }
                                else {
                                    norm = Base::Vector3d(cos(angle),sin(angle),0);
                                    dir = Base::Vector3d(-norm.y,norm.x,0);
                                    midpos += ra*norm;
                                }
                            }

                            Base::Vector3d relpos = seekConstraintPosition(midpos, norm, dir, 2.5, edit->constrGroup->getChild(i));

                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(midpos.x, midpos.y, zConstr); //Absolute Reference

                            //Reference Position that is scaled according to zoom
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relpos.x, relpos.y, 0);
                        }
                        else {
                            assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);
                            assert(Constr->FirstPos != Sketcher::PointPos::none && Constr->SecondPos != Sketcher::PointPos::none);

                            Base::Vector3d midpos1, dir1, norm1;
                            Base::Vector3d midpos2, dir2, norm2;

                            if (temp)
                                midpos1 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
                            else
                                midpos1 = getSketchObject()->getPoint(Constr->First, Constr->FirstPos);

                            if (temp)
                                midpos2 = getSolvedSketch().getPoint(Constr->Second, Constr->SecondPos);
                            else
                                midpos2 = getSketchObject()->getPoint(Constr->Second, Constr->SecondPos);

                            dir1 = (midpos2-midpos1).Normalize();
                            dir2 = -dir1;
                            norm1 = Base::Vector3d(-dir1.y,dir1.x,0.);
                            norm2 = norm1;

                            Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 4.0, edit->constrGroup->getChild(i));
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr);
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                            Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 4.0, edit->constrGroup->getChild(i));

                            Base::Vector3d secondPos = midpos2 - midpos1;
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr);
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);
                        }
                    }
                    break;
                case Perpendicular:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);
                        // get the geometry
                        const Part::Geometry *geo1 = GeoById(*geomlist, Constr->First);
                        const Part::Geometry *geo2 = GeoById(*geomlist, Constr->Second);

                        Base::Vector3d midpos1, dir1, norm1;
                        Base::Vector3d midpos2, dir2, norm2;
                        bool twoIcons = false;//a very local flag. It's set to true to indicate that the second dir+norm are valid and should be used


                        if (Constr->Third != GeoEnum::GeoUndef || //perpty via point
                                Constr->FirstPos != Sketcher::PointPos::none) { //endpoint-to-curve or endpoint-to-endpoint perpty

                            int ptGeoId;
                            Sketcher::PointPos ptPosId;
                            do {//dummy loop to use break =) Maybe goto?
                                ptGeoId = Constr->First;
                                ptPosId = Constr->FirstPos;
                                if (ptPosId != Sketcher::PointPos::none) break;
                                ptGeoId = Constr->Second;
                                ptPosId = Constr->SecondPos;
                                if (ptPosId != Sketcher::PointPos::none) break;
                                ptGeoId = Constr->Third;
                                ptPosId = Constr->ThirdPos;
                                if (ptPosId != Sketcher::PointPos::none) break;
                                assert(0);//no point found!
                            } while (false);
                            if (temp)
                                midpos1 = getSolvedSketch().getPoint(ptGeoId, ptPosId);
                            else
                                midpos1 = getSketchObject()->getPoint(ptGeoId, ptPosId);

                            norm1 = getSolvedSketch().calculateNormalAtPoint(Constr->Second, midpos1.x, midpos1.y);
                            norm1.Normalize();
                            dir1 = norm1; dir1.RotateZ(-M_PI/2.0);

                        } else if (Constr->FirstPos == Sketcher::PointPos::none) {

                            if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment *>(geo1);
                                midpos1 = ((lineSeg1->getEndPoint()+lineSeg1->getStartPoint())/2);
                                dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint()).Normalize();
                                norm1 = Base::Vector3d(-dir1.y,dir1.x,0.);
                            } else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo1);
                                double startangle, endangle, midangle;
                                arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                midangle = (startangle + endangle)/2;
                                norm1 = Base::Vector3d(cos(midangle),sin(midangle),0);
                                dir1 = Base::Vector3d(-norm1.y,norm1.x,0);
                                midpos1 = arc->getCenter() + arc->getRadius() * norm1;
                            } else if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo1);
                                norm1 = Base::Vector3d(cos(M_PI/4),sin(M_PI/4),0);
                                dir1 = Base::Vector3d(-norm1.y,norm1.x,0);
                                midpos1 = circle->getCenter() + circle->getRadius() * norm1;
                            } else
                                break;

                            if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment *>(geo2);
                                midpos2 = ((lineSeg2->getEndPoint()+lineSeg2->getStartPoint())/2);
                                dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint()).Normalize();
                                norm2 = Base::Vector3d(-dir2.y,dir2.x,0.);
                            } else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                double startangle, endangle, midangle;
                                arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                midangle = (startangle + endangle)/2;
                                norm2 = Base::Vector3d(cos(midangle),sin(midangle),0);
                                dir2 = Base::Vector3d(-norm2.y,norm2.x,0);
                                midpos2 = arc->getCenter() + arc->getRadius() * norm2;
                            } else if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo2);
                                norm2 = Base::Vector3d(cos(M_PI/4),sin(M_PI/4),0);
                                dir2 = Base::Vector3d(-norm2.y,norm2.x,0);
                                midpos2 = circle->getCenter() + circle->getRadius() * norm2;
                            } else
                                break;
                            twoIcons = true;
                        }

                        Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 4.0, edit->constrGroup->getChild(i));
                        static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr);
                        static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                        if (twoIcons) {
                            Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 4.0, edit->constrGroup->getChild(i));

                            Base::Vector3d secondPos = midpos2 - midpos1;
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr);
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);
                        }

                    }
                    break;
                case Parallel:
                case Equal:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);
                        // get the geometry
                        const Part::Geometry *geo1 = GeoById(*geomlist, Constr->First);
                        const Part::Geometry *geo2 = GeoById(*geomlist, Constr->Second);

                        Base::Vector3d midpos1, dir1, norm1;
                        Base::Vector3d midpos2, dir2, norm2;
                        if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                            geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                            if (Constr->Type == Equal) {
                                double r1a=0,r1b=0,r2a=0,r2b=0;
                                double angle1,angle1plus=0.,  angle2, angle2plus=0.;//angle1 = rotation of object as a whole; angle1plus = arc angle (t parameter for ellipses).
                                if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                    const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo1);
                                    r1a = circle->getRadius();
                                    angle1 = M_PI/4;
                                    midpos1 = circle->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                    const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo1);
                                    r1a = arc->getRadius();
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle1 = (startangle + endangle)/2;
                                    midpos1 = arc->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo1);
                                    r1a = ellipse->getMajorRadius();
                                    r1b = ellipse->getMinorRadius();
                                    Base::Vector3d majdir = ellipse->getMajorAxisDir();
                                    angle1 = atan2(majdir.y, majdir.x);
                                    angle1plus = M_PI/4;
                                    midpos1 = ellipse->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                                    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo1);
                                    r1a = aoe->getMajorRadius();
                                    r1b = aoe->getMinorRadius();
                                    double startangle, endangle;
                                    aoe->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoe->getMajorAxisDir();
                                    angle1 = atan2(majdir.y, majdir.x);
                                    angle1plus = (startangle + endangle)/2;
                                    midpos1 = aoe->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                                    const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(geo1);
                                    r1a = aoh->getMajorRadius();
                                    r1b = aoh->getMinorRadius();
                                    double startangle, endangle;
                                    aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoh->getMajorAxisDir();
                                    angle1 = atan2(majdir.y, majdir.x);
                                    angle1plus = (startangle + endangle)/2;
                                    midpos1 = aoh->getCenter();
                                } else if (geo1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                                    const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(geo1);
                                    r1a = aop->getFocal();
                                    double startangle, endangle;
                                    aop->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = - aop->getXAxisDir();
                                    angle1 = atan2(majdir.y, majdir.x);
                                    angle1plus = (startangle + endangle)/2;
                                    midpos1 = aop->getFocus();
                                } else
                                    break;

                                if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                    const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo2);
                                    r2a = circle->getRadius();
                                    angle2 = M_PI/4;
                                    midpos2 = circle->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                    const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                    r2a = arc->getRadius();
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle2 = (startangle + endangle)/2;
                                    midpos2 = arc->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo2);
                                    r2a = ellipse->getMajorRadius();
                                    r2b = ellipse->getMinorRadius();
                                    Base::Vector3d majdir = ellipse->getMajorAxisDir();
                                    angle2 = atan2(majdir.y, majdir.x);
                                    angle2plus = M_PI/4;
                                    midpos2 = ellipse->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                                    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo2);
                                    r2a = aoe->getMajorRadius();
                                    r2b = aoe->getMinorRadius();
                                    double startangle, endangle;
                                    aoe->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoe->getMajorAxisDir();
                                    angle2 = atan2(majdir.y, majdir.x);
                                    angle2plus = (startangle + endangle)/2;
                                    midpos2 = aoe->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
                                    const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(geo2);
                                    r2a = aoh->getMajorRadius();
                                    r2b = aoh->getMinorRadius();
                                    double startangle, endangle;
                                    aoh->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = aoh->getMajorAxisDir();
                                    angle2 = atan2(majdir.y, majdir.x);
                                    angle2plus = (startangle + endangle)/2;
                                    midpos2 = aoh->getCenter();
                                } else if (geo2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
                                    const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(geo2);
                                    r2a = aop->getFocal();
                                    double startangle, endangle;
                                    aop->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    Base::Vector3d majdir = -aop->getXAxisDir();
                                    angle2 = atan2(majdir.y, majdir.x);
                                    angle2plus = (startangle + endangle)/2;
                                    midpos2 = aop->getFocus();
                                } else
                                    break;

                                if( geo1->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                                    geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                                    geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ){

                                    Base::Vector3d majDir, minDir, rvec;
                                    majDir = Base::Vector3d(cos(angle1),sin(angle1),0);//direction of major axis of ellipse
                                    minDir = Base::Vector3d(-majDir.y,majDir.x,0);//direction of minor axis of ellipse
                                    rvec = (r1a*cos(angle1plus)) * majDir   +   (r1b*sin(angle1plus)) * minDir;
                                    midpos1 += rvec;
                                    rvec.Normalize();
                                    norm1 = rvec;
                                    dir1 = Base::Vector3d(-rvec.y,rvec.x,0);//DeepSOIC: I'm not sure what dir is supposed to mean.
                                }
                                else {
                                    norm1 = Base::Vector3d(cos(angle1),sin(angle1),0);
                                    dir1 = Base::Vector3d(-norm1.y,norm1.x,0);
                                    midpos1 += r1a*norm1;
                                }


                                if( geo2->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                                    geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                                    geo2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {

                                    Base::Vector3d majDir, minDir, rvec;
                                    majDir = Base::Vector3d(cos(angle2),sin(angle2),0);//direction of major axis of ellipse
                                    minDir = Base::Vector3d(-majDir.y,majDir.x,0);//direction of minor axis of ellipse
                                    rvec = (r2a*cos(angle2plus)) * majDir   +   (r2b*sin(angle2plus)) * minDir;
                                    midpos2 += rvec;
                                    rvec.Normalize();
                                    norm2 = rvec;
                                    dir2 = Base::Vector3d(-rvec.y,rvec.x,0);
                                }
                                else {
                                    norm2 = Base::Vector3d(cos(angle2),sin(angle2),0);
                                    dir2 = Base::Vector3d(-norm2.y,norm2.x,0);
                                    midpos2 += r2a*norm2;
                                }

                            } else // Parallel can only apply to a GeomLineSegment
                                break;
                        } else {
                            const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment *>(geo1);
                            const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment *>(geo2);

                            // calculate the half distance between the start and endpoint
                            midpos1 = ((lineSeg1->getEndPoint()+lineSeg1->getStartPoint())/2);
                            midpos2 = ((lineSeg2->getEndPoint()+lineSeg2->getStartPoint())/2);
                            //Get a set of vectors perpendicular and tangential to these
                            dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint()).Normalize();
                            dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint()).Normalize();
                            norm1 = Base::Vector3d(-dir1.y,dir1.x,0.);
                            norm2 = Base::Vector3d(-dir2.y,dir2.x,0.);
                        }

                        Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 4.0, edit->constrGroup->getChild(i));
                        Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 4.0, edit->constrGroup->getChild(i));

                        static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr); //Absolute Reference

                        //Reference Position that is scaled according to zoom
                        static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                        Base::Vector3d secondPos = midpos2 - midpos1;
                        static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr); //Absolute Reference

                        //Reference Position that is scaled according to zoom
                        static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->translation = SbVec3f(relpos2.x - relpos1.x, relpos2.y -relpos1.y, 0);

                    }
                    break;
                case Distance:
                case DistanceX:
                case DistanceY:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                        Base::Vector3d pnt1(0.,0.,0.), pnt2(0.,0.,0.);
                        if (Constr->SecondPos != Sketcher::PointPos::none) { // point to point distance
                            if (temp) {
                                pnt1 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
                                pnt2 = getSolvedSketch().getPoint(Constr->Second, Constr->SecondPos);
                            } else {
                                pnt1 = getSketchObject()->getPoint(Constr->First, Constr->FirstPos);
                                pnt2 = getSketchObject()->getPoint(Constr->Second, Constr->SecondPos);
                            }
                        } else if (Constr->Second != GeoEnum::GeoUndef) { // point to line distance
                            if (temp) {
                                pnt1 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
                            } else {
                                pnt1 = getSketchObject()->getPoint(Constr->First, Constr->FirstPos);
                            }
                            const Part::Geometry *geo = GeoById(*geomlist, Constr->Second);
                            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);
                                Base::Vector3d l2p1 = lineSeg->getStartPoint();
                                Base::Vector3d l2p2 = lineSeg->getEndPoint();
                                // calculate the projection of p1 onto line2
                                pnt2.ProjectToLine(pnt1-l2p1, l2p2-l2p1);
                                pnt2 += pnt1;
                            } else
                                break;
                        } else if (Constr->FirstPos != Sketcher::PointPos::none) {
                            if (temp) {
                                pnt2 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
                            } else {
                                pnt2 = getSketchObject()->getPoint(Constr->First, Constr->FirstPos);
                            }
                        } else if (Constr->First != GeoEnum::GeoUndef) {
                            const Part::Geometry *geo = GeoById(*geomlist, Constr->First);
                            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);
                                pnt1 = lineSeg->getStartPoint();
                                pnt2 = lineSeg->getEndPoint();
                            } else
                                break;
                        } else
                            break;

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));

                        // Get presentation string (w/o units if option is set)
                        asciiText->string = SbString( getPresentationString(Constr).toUtf8().constData() );

                        if (Constr->Type == Distance)
                            asciiText->datumtype = SoDatumLabel::DISTANCE;
                        else if (Constr->Type == DistanceX)
                            asciiText->datumtype = SoDatumLabel::DISTANCEX;
                        else if (Constr->Type == DistanceY)
                             asciiText->datumtype = SoDatumLabel::DISTANCEY;

                        // Assign the Datum Points
                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = SbVec3f (pnt1.x,pnt1.y,zConstr);
                        verts[1] = SbVec3f (pnt2.x,pnt2.y,zConstr);

                        asciiText->pnts.finishEditing();

                        //Assign the Label Distance
                        asciiText->param1 = Constr->LabelDistance;
                        asciiText->param2 = Constr->LabelPosition;
                    }
                    break;
                case PointOnObject:
                case Tangent:
                case SnellsLaw:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);

                        Base::Vector3d pos, relPos;
                        if (  Constr->Type == PointOnObject ||
                              Constr->Type == SnellsLaw ||
                              (Constr->Type == Tangent && Constr->Third != GeoEnum::GeoUndef) || //Tangency via point
                              (Constr->Type == Tangent && Constr->FirstPos != Sketcher::PointPos::none) //endpoint-to-curve or endpoint-to-endpoint tangency
                                ) {

                            //find the point of tangency/point that is on object
                            //just any point among first/second/third should be OK
                            int ptGeoId;
                            Sketcher::PointPos ptPosId;
                            do {//dummy loop to use break =) Maybe goto?
                                ptGeoId = Constr->First;
                                ptPosId = Constr->FirstPos;
                                if (ptPosId != Sketcher::PointPos::none) break;
                                ptGeoId = Constr->Second;
                                ptPosId = Constr->SecondPos;
                                if (ptPosId != Sketcher::PointPos::none) break;
                                ptGeoId = Constr->Third;
                                ptPosId = Constr->ThirdPos;
                                if (ptPosId != Sketcher::PointPos::none) break;
                                assert(0);//no point found!
                            } while (false);
                            pos = getSolvedSketch().getPoint(ptGeoId, ptPosId);

                            Base::Vector3d norm = getSolvedSketch().calculateNormalAtPoint(Constr->Second, pos.x, pos.y);
                            norm.Normalize();
                            Base::Vector3d dir = norm; dir.RotateZ(-M_PI/2.0);

                            relPos = seekConstraintPosition(pos, norm, dir, 2.5, edit->constrGroup->getChild(i));
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(pos.x, pos.y, zConstr); //Absolute Reference
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relPos.x, relPos.y, 0);
                        }
                        else if (Constr->Type == Tangent) {
                            // get the geometry
                            const Part::Geometry *geo1 = GeoById(*geomlist, Constr->First);
                            const Part::Geometry *geo2 = GeoById(*geomlist, Constr->Second);

                            if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                                geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment *>(geo1);
                                const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment *>(geo2);
                                // tangency between two lines
                                Base::Vector3d midpos1 = ((lineSeg1->getEndPoint()+lineSeg1->getStartPoint())/2);
                                Base::Vector3d midpos2 = ((lineSeg2->getEndPoint()+lineSeg2->getStartPoint())/2);
                                Base::Vector3d dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint()).Normalize();
                                Base::Vector3d dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint()).Normalize();
                                Base::Vector3d norm1 = Base::Vector3d(-dir1.y,dir1.x,0.f);
                                Base::Vector3d norm2 = Base::Vector3d(-dir2.y,dir2.x,0.f);

                                Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 4.0, edit->constrGroup->getChild(i));
                                Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 4.0, edit->constrGroup->getChild(i));

                                static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr); //Absolute Reference

                                //Reference Position that is scaled according to zoom
                                static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                                Base::Vector3d secondPos = midpos2 - midpos1;
                                static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr); //Absolute Reference

                                //Reference Position that is scaled according to zoom
                                static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);

                                break;
                            }
                            else if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                std::swap(geo1,geo2);
                            }

                            if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo1);
                                Base::Vector3d dir = (lineSeg->getEndPoint() - lineSeg->getStartPoint()).Normalize();
                                Base::Vector3d norm(-dir.y, dir.x, 0);
                                if (geo2->getTypeId()== Part::GeomCircle::getClassTypeId()) {
                                    const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo2);
                                    // tangency between a line and a circle
                                    float length = (circle->getCenter() - lineSeg->getStartPoint())*dir;

                                    pos = lineSeg->getStartPoint() + dir * length;
                                    relPos = norm * 1;  //TODO Huh?
                                }
                                else if (geo2->getTypeId()== Part::GeomEllipse::getClassTypeId() ||
                                         geo2->getTypeId()== Part::GeomArcOfEllipse::getClassTypeId()) {

                                    Base::Vector3d center;
                                    if(geo2->getTypeId()== Part::GeomEllipse::getClassTypeId()){
                                        const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo2);
                                        center=ellipse->getCenter();
                                    } else {
                                        const Part::GeomArcOfEllipse *aoc = static_cast<const Part::GeomArcOfEllipse *>(geo2);
                                        center=aoc->getCenter();
                                    }

                                    // tangency between a line and an ellipse
                                    float length = (center - lineSeg->getStartPoint())*dir;

                                    pos = lineSeg->getStartPoint() + dir * length;
                                    relPos = norm * 1;
                                }
                                else if (geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                                    const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                    // tangency between a line and an arc
                                    float length = (arc->getCenter() - lineSeg->getStartPoint())*dir;

                                    pos = lineSeg->getStartPoint() + dir * length;
                                    relPos = norm * 1;  //TODO Huh?
                                }
                            }

                            if (geo1->getTypeId()== Part::GeomCircle::getClassTypeId() &&
                                geo2->getTypeId()== Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle1 = static_cast<const Part::GeomCircle *>(geo1);
                                const Part::GeomCircle *circle2 = static_cast<const Part::GeomCircle *>(geo2);
                                // tangency between two cicles
                                Base::Vector3d dir = (circle2->getCenter() - circle1->getCenter()).Normalize();
                                pos =  circle1->getCenter() + dir *  circle1->getRadius();
                                relPos = dir * 1;
                            }
                            else if (geo2->getTypeId()== Part::GeomCircle::getClassTypeId()) {
                                std::swap(geo1,geo2);
                            }

                            if (geo1->getTypeId()== Part::GeomCircle::getClassTypeId() &&
                                geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo1);
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                // tangency between a circle and an arc
                                Base::Vector3d dir = (arc->getCenter() - circle->getCenter()).Normalize();
                                pos =  circle->getCenter() + dir *  circle->getRadius();
                                relPos = dir * 1;
                            }
                            else if (geo1->getTypeId()== Part::GeomArcOfCircle::getClassTypeId() &&
                                     geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc1 = static_cast<const Part::GeomArcOfCircle *>(geo1);
                                const Part::GeomArcOfCircle *arc2 = static_cast<const Part::GeomArcOfCircle *>(geo2);
                                // tangency between two arcs
                                Base::Vector3d dir = (arc2->getCenter() - arc1->getCenter()).Normalize();
                                pos =  arc1->getCenter() + dir *  arc1->getRadius();
                                relPos = dir * 1;
                            }
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(pos.x, pos.y, zConstr); //Absolute Reference
                            static_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relPos.x, relPos.y, 0);
                        }
                    }
                    break;
                case Symmetric:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);

                        Base::Vector3d pnt1 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
                        Base::Vector3d pnt2 = getSolvedSketch().getPoint(Constr->Second, Constr->SecondPos);

                        SbVec3f p1(pnt1.x,pnt1.y,zConstr);
                        SbVec3f p2(pnt2.x,pnt2.y,zConstr);
                        SbVec3f dir = (p2-p1);
                        dir.normalize();
                        SbVec3f norm (-dir[1],dir[0],0);

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                        asciiText->datumtype    = SoDatumLabel::SYMMETRIC;

                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = p1;
                        verts[1] = p2;

                        asciiText->pnts.finishEditing();

                        static_cast<SoTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = (p1 + p2)/2;
                    }
                    break;
                case Angle:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                        assert((Constr->Second >= -extGeoCount && Constr->Second < intGeoCount) ||
                               Constr->Second == GeoEnum::GeoUndef);

                        SbVec3f p0;
                        double startangle,range,endangle;
                        if (Constr->Second != GeoEnum::GeoUndef) {
                            Base::Vector3d dir1, dir2;
                            if(Constr->Third == GeoEnum::GeoUndef) { //angle between two lines
                                const Part::Geometry *geo1 = GeoById(*geomlist, Constr->First);
                                const Part::Geometry *geo2 = GeoById(*geomlist, Constr->Second);
                                if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                                    geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                                    break;
                                const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment *>(geo1);
                                const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment *>(geo2);

                                bool flip1 = (Constr->FirstPos == Sketcher::PointPos::end);
                                bool flip2 = (Constr->SecondPos == Sketcher::PointPos::end);
                                dir1 = (flip1 ? -1. : 1.) * (lineSeg1->getEndPoint()-lineSeg1->getStartPoint());
                                dir2 = (flip2 ? -1. : 1.) * (lineSeg2->getEndPoint()-lineSeg2->getStartPoint());
                                Base::Vector3d pnt1 = flip1 ? lineSeg1->getEndPoint() : lineSeg1->getStartPoint();
                                Base::Vector3d pnt2 = flip2 ? lineSeg2->getEndPoint() : lineSeg2->getStartPoint();

                                // line-line intersection
                                {
                                    double det = dir1.x*dir2.y - dir1.y*dir2.x;
                                    if ((det > 0 ? det : -det) < 1e-10) {
                                        // lines are coincident (or parallel) and in this case the center
                                        // of the point pairs with the shortest distance is used
                                        Base::Vector3d p1[2], p2[2];
                                        p1[0] = lineSeg1->getStartPoint();
                                        p1[1] = lineSeg1->getEndPoint();
                                        p2[0] = lineSeg2->getStartPoint();
                                        p2[1] = lineSeg2->getEndPoint();
                                        double length = DBL_MAX;
                                        for (int i=0; i <= 1; i++) {
                                            for (int j=0; j <= 1; j++) {
                                                double tmp = (p2[j]-p1[i]).Length();
                                                if (tmp < length) {
                                                    length = tmp;
                                                    p0.setValue((p2[j].x+p1[i].x)/2,(p2[j].y+p1[i].y)/2,0);
                                                }
                                            }
                                        }
                                    }
                                    else {
                                        double c1 = dir1.y*pnt1.x - dir1.x*pnt1.y;
                                        double c2 = dir2.y*pnt2.x - dir2.x*pnt2.y;
                                        double x = (dir1.x*c2 - dir2.x*c1)/det;
                                        double y = (dir1.y*c2 - dir2.y*c1)/det;
                                        p0 = SbVec3f(x,y,0);
                                    }
                                }

                                range = Constr->getValue(); // WYSIWYG
                                startangle = atan2(dir1.y,dir1.x);
                            }
                            else {//angle-via-point
                                Base::Vector3d p = getSolvedSketch().getPoint(Constr->Third, Constr->ThirdPos);
                                p0 = SbVec3f(p.x, p.y, 0);
                                dir1 = getSolvedSketch().calculateNormalAtPoint(Constr->First, p.x, p.y);
                                dir1.RotateZ(-M_PI/2);//convert to vector of tangency by rotating
                                dir2 = getSolvedSketch().calculateNormalAtPoint(Constr->Second, p.x, p.y);
                                dir2.RotateZ(-M_PI/2);

                                startangle = atan2(dir1.y,dir1.x);
                                range = atan2(dir1.x*dir2.y-dir1.y*dir2.x,
                                          dir1.x*dir2.x+dir1.y*dir2.y);
                            }

                            endangle = startangle + range;

                        } else if (Constr->First != GeoEnum::GeoUndef) {
                            const Part::Geometry *geo = GeoById(*geomlist, Constr->First);
                            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geo);
                                p0 = Base::convertTo<SbVec3f>((lineSeg->getEndPoint()+lineSeg->getStartPoint())/2);

                                Base::Vector3d dir = lineSeg->getEndPoint()-lineSeg->getStartPoint();
                                startangle = 0.;
                                range = atan2(dir.y,dir.x);
                                endangle = startangle + range;
                            }
                            else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                                p0 = Base::convertTo<SbVec3f>(arc->getCenter());

                                arc->getRange(startangle, endangle,/*emulateCCWXY=*/true);
                                range = endangle - startangle;
                            }
                            else {
                                break;
                            }
                        } else
                            break;

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                        asciiText->string    = SbString( getPresentationString(Constr).toUtf8().constData() );
                        asciiText->datumtype = SoDatumLabel::ANGLE;
                        asciiText->param1    = Constr->LabelDistance;
                        asciiText->param2    = startangle;
                        asciiText->param3    = range;

                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = p0;

                        asciiText->pnts.finishEditing();

                    }
                    break;
                case Diameter:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                        Base::Vector3d pnt1(0.,0.,0.), pnt2(0.,0.,0.);
                        if (Constr->First != GeoEnum::GeoUndef) {
                            const Part::Geometry *geo = GeoById(*geomlist, Constr->First);

                            if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                                double radius = arc->getRadius();
                                double angle = (double) Constr->LabelPosition;
                                if (angle == 10) {
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle = (startangle + endangle)/2;
                                }
                                Base::Vector3d center = arc->getCenter();
                                pnt1 = center - radius * Base::Vector3d(cos(angle),sin(angle),0.);
                                pnt2 = center + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                            }
                            else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo);
                                double radius = circle->getRadius();
                                double angle = (double) Constr->LabelPosition;
                                if (angle == 10) {
                                    angle = 0;
                                }
                                Base::Vector3d center = circle->getCenter();
                                pnt1 = center - radius * Base::Vector3d(cos(angle),sin(angle),0.);
                                pnt2 = center + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                            }
                            else
                                break;
                        } else
                            break;

                        SbVec3f p1(pnt1.x,pnt1.y,zConstr);
                        SbVec3f p2(pnt2.x,pnt2.y,zConstr);

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));

                        // Get display string with units hidden if so requested
                        asciiText->string = SbString( getPresentationString(Constr).toUtf8().constData() );

                        asciiText->datumtype    = SoDatumLabel::DIAMETER;
                        asciiText->param1       = Constr->LabelDistance;
                        asciiText->param2       = Constr->LabelPosition;

                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = p1;
                        verts[1] = p2;

                        asciiText->pnts.finishEditing();
                    }
                    break;
                    case Weight:
                    case Radius:
                    {
                        assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                        Base::Vector3d pnt1(0.,0.,0.), pnt2(0.,0.,0.);

                        if (Constr->First != GeoEnum::GeoUndef) {
                            const Part::Geometry *geo = GeoById(*geomlist, Constr->First);

                            if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geo);
                                double radius = arc->getRadius();
                                double angle = (double) Constr->LabelPosition;
                                if (angle == 10) {
                                    double startangle, endangle;
                                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                                    angle = (startangle + endangle)/2;
                                }
                                pnt1 = arc->getCenter();
                                pnt2 = pnt1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                            }
                            else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geo);
                                auto gf = GeometryFacade::getFacade(geo);

                                double radius;

                                if(Constr->Type == Weight) {
                                    double scalefactor = 1.0;

                                    if(circle->hasExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()))
                                    {
                                        auto vpext = std::static_pointer_cast<const SketcherGui::ViewProviderSketchGeometryExtension>(
                                                        circle->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()).lock());

                                        scalefactor = vpext->getRepresentationFactor();
                                    }

                                    radius = circle->getRadius()*scalefactor;
                                }
                                else {
                                    radius = circle->getRadius();
                                }

                                double angle = (double) Constr->LabelPosition;
                                if (angle == 10) {
                                    angle = 0;
                                }
                                pnt1 = circle->getCenter();
                                pnt2 = pnt1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                            }
                            else
                                break;
                        } else
                            break;

                        SbVec3f p1(pnt1.x,pnt1.y,zConstr);
                        SbVec3f p2(pnt2.x,pnt2.y,zConstr);

                        SoDatumLabel *asciiText = static_cast<SoDatumLabel *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));

                        // Get display string with units hidden if so requested
                        if(Constr->Type == Weight)
                            asciiText->string = SbString( QString::number(Constr->getValue()).toStdString().c_str());
                        else
                            asciiText->string = SbString( getPresentationString(Constr).toUtf8().constData() );

                        asciiText->datumtype    = SoDatumLabel::RADIUS;
                        asciiText->param1       = Constr->LabelDistance;
                        asciiText->param2       = Constr->LabelPosition;

                        asciiText->pnts.setNum(2);
                        SbVec3f *verts = asciiText->pnts.startEditing();

                        verts[0] = p1;
                        verts[1] = p2;

                        asciiText->pnts.finishEditing();
                    }
                    break;
                case Coincident: // nothing to do for coincident
                case None:
                case InternalAlignment:
                case NumConstraintTypes:
                    break;
            }

        } catch (Base::Exception &e) {
            Base::Console().Error("Exception during draw: %s\n", e.what());
        } catch (...){
            Base::Console().Error("Exception during draw: unknown\n");
        }

    }

    // Avoids unneeded calls to pixmapFromSvg
    if(Mode==STATUS_NONE || Mode==STATUS_SKETCH_UseHandler) {
       this->drawConstraintIcons();
       this->updateColor();
    }

    // delete the cloned objects
    if (temp) {
        for (std::vector<Part::Geometry *>::iterator it=tempGeo.begin(); it != tempGeo.end(); ++it) {
            if (*it)
                delete *it;
        }
    }

    Gui::MDIView *mdi = this->getActiveView();
    if (mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        static_cast<Gui::View3DInventor *>(mdi)->getViewer()->redraw();
    }
}

void ViewProviderSketch::rebuildConstraintsVisual(void)
{
    const std::vector<Sketcher::Constraint *> &constrlist = getSketchObject()->Constraints.getValues();
    // clean up
    Gui::coinRemoveAllChildren(edit->constrGroup);
    edit->vConstrType.clear();

    for (std::vector<Sketcher::Constraint *>::const_iterator it=constrlist.begin(); it != constrlist.end(); ++it) {
        // root separator for one constraint
        SoSeparator *sep = new SoSeparator();
        sep->ref();
        // no caching for frequently-changing data structures
        sep->renderCaching = SoSeparator::OFF;

        // every constrained visual node gets its own material for preselection and selection
        SoMaterial *mat = new SoMaterial;
        mat->ref();
        mat->diffuseColor = (*it)->isActive ?
                                ((*it)->isDriving ?
                                    ConstrDimColor
                                    :NonDrivingConstrDimColor)
                                :DeactivatedConstrDimColor;
        // Get sketch normal
        Base::Vector3d RN(0,0,1);

        // move to position of Sketch
        Base::Placement Plz = getEditingPlacement();
        Base::Rotation tmp(Plz.getRotation());
        tmp.multVec(RN,RN);
        Plz.setRotation(tmp);

        SbVec3f norm(RN.x, RN.y, RN.z);

        // distinguish different constraint types to build up
        switch ((*it)->Type) {
            case Distance:
            case DistanceX:
            case DistanceY:
            case Radius:
            case Diameter:
            case Weight:
            case Angle:
            {
                SoDatumLabel *text = new SoDatumLabel();
                text->norm.setValue(norm);
                text->string = "";
                text->textColor = (*it)->isActive ?
                                        ((*it)->isDriving ?
                                            ConstrDimColor
                                            :NonDrivingConstrDimColor)
                                        :DeactivatedConstrDimColor;
                text->size.setValue(edit->coinFontSize);
                text->lineWidth = 2 * edit->pixelScalingFactor;
                text->useAntialiasing = false;
                SoAnnotation *anno = new SoAnnotation();
                anno->renderCaching = SoSeparator::OFF;
                anno->addChild(text);
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(text);
                edit->constrGroup->addChild(anno);
                edit->vConstrType.push_back((*it)->Type);
                // nodes not needed
                sep->unref();
                mat->unref();
                continue; // jump to next constraint
            }
            break;
            case Horizontal:
            case Vertical:
            case Block:
            {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6
                sep->addChild(new SoInfo());

                // remember the type of this constraint node
                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            case Coincident: // no visual for coincident so far
                edit->vConstrType.push_back(Coincident);
                break;
            case Parallel:
            case Perpendicular:
            case Equal:
            {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6
                sep->addChild(new SoInfo());

                // remember the type of this constraint node
                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            case PointOnObject:
            case Tangent:
            case SnellsLaw:
            {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());

                if ((*it)->Type == Tangent) {
                    const Part::Geometry *geo1 = getSketchObject()->getGeometry((*it)->First);
                    const Part::Geometry *geo2 = getSketchObject()->getGeometry((*it)->Second);
                    if (!geo1 || !geo2) {
                        Base::Console().Warning("Tangent constraint references non-existing geometry\n");
                    }
                    else if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                             geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
                        sep->addChild(new SoZoomTranslation());
                        // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
                        sep->addChild(new SoImage());
                        // #define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6
                        sep->addChild(new SoInfo());
                    }
                }

                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            case Symmetric:
            {
                SoDatumLabel *arrows = new SoDatumLabel();
                arrows->norm.setValue(norm);
                arrows->string = "";
                arrows->textColor = ConstrDimColor;
                arrows->lineWidth = 2 * edit->pixelScalingFactor;

                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(arrows);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
                sep->addChild(new SoInfo());

                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            case InternalAlignment:
            {
                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            default:
                edit->vConstrType.push_back((*it)->Type);
        }

        edit->constrGroup->addChild(sep);
        // decrement ref counter again
        sep->unref();
        mat->unref();
    }
}

void ViewProviderSketch::updateVirtualSpace(void)
{
    const std::vector<Sketcher::Constraint *> &constrlist = getSketchObject()->Constraints.getValues();

    if(constrlist.size() == edit->vConstrType.size()) {

        edit->constrGroup->enable.setNum(constrlist.size());

        SbBool *sws = edit->constrGroup->enable.startEditing();

        for (size_t i = 0; i < constrlist.size(); i++)
            sws[i] = !(constrlist[i]->isInVirtualSpace != isShownVirtualSpace); // XOR of constraint mode and VP mode


        edit->constrGroup->enable.finishEditing();
    }
}

void ViewProviderSketch::setIsShownVirtualSpace(bool isshownvirtualspace)
{
    this->isShownVirtualSpace = isshownvirtualspace;

    updateVirtualSpace();

    signalConstraintsChanged();
}

bool ViewProviderSketch::getIsShownVirtualSpace() const
{
    return this->isShownVirtualSpace;
}


void ViewProviderSketch::drawEdit(const std::vector<Base::Vector2d> &EditCurve)
{
    assert(edit);

    edit->EditCurveSet->numVertices.setNum(1);
    edit->EditCurvesCoordinate->point.setNum(EditCurve.size());
    edit->EditCurvesMaterials->diffuseColor.setNum(EditCurve.size());
    SbVec3f *verts = edit->EditCurvesCoordinate->point.startEditing();
    int32_t *index = edit->EditCurveSet->numVertices.startEditing();
    SbColor *color = edit->EditCurvesMaterials->diffuseColor.startEditing();

    int i=0; // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditCurve.begin(); it != EditCurve.end(); ++it,i++) {
        verts[i].setValue(it->x,it->y,zEdit);
        color[i] = CreateCurveColor;
    }

    index[0] = EditCurve.size();
    edit->EditCurvesCoordinate->point.finishEditing();
    edit->EditCurveSet->numVertices.finishEditing();
    edit->EditCurvesMaterials->diffuseColor.finishEditing();
}

void ViewProviderSketch::drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel)
{
    assert(edit);

    // determine marker size
    int augmentedmarkersize = edit->MarkerSize;

    auto supportedsizes = Gui::Inventor::MarkerBitmaps::getSupportedSizes("CIRCLE_LINE");

    auto defaultmarker = std::find(supportedsizes.begin(), supportedsizes.end(), edit->MarkerSize);

    if(defaultmarker != supportedsizes.end()) {
        auto validAugmentationLevels = std::distance(defaultmarker,supportedsizes.end());

        if(augmentationlevel >= validAugmentationLevels)
            augmentationlevel = validAugmentationLevels - 1;

        augmentedmarkersize = *std::next(defaultmarker, augmentationlevel);
    }

    edit->EditMarkerSet->markerIndex.startEditing();
    edit->EditMarkerSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", augmentedmarkersize);

    // add the points to set
    edit->EditMarkersCoordinate->point.setNum(EditMarkers.size());
    edit->EditMarkersMaterials->diffuseColor.setNum(EditMarkers.size());
    SbVec3f *verts = edit->EditMarkersCoordinate->point.startEditing();
    SbColor *color = edit->EditMarkersMaterials->diffuseColor.startEditing();

    int i=0; // setting up the line set
    for (std::vector<Base::Vector2d>::const_iterator it = EditMarkers.begin(); it != EditMarkers.end(); ++it,i++) {
        verts[i].setValue(it->x,it->y,zEdit);
        color[i] = InformationColor;
    }

    edit->EditMarkersCoordinate->point.finishEditing();
    edit->EditMarkersMaterials->diffuseColor.finishEditing();
    edit->EditMarkerSet->markerIndex.finishEditing();
}

void ViewProviderSketch::updateData(const App::Property *prop)
{
    ViewProvider2DObjectGrid::updateData(prop);

    // In the case of an undo/redo transaction, updateData is triggered by SketchObject::onUndoRedoFinished() in the solve()
    // In the case of an internal transaction, touching the geometry results in a call to updateData.
    if ( edit && !getSketchObject()->getDocument()->isPerformingTransaction() &&
         !getSketchObject()->isPerformingInternalTransaction() &&
         (prop == &(getSketchObject()->Geometry) || prop == &(getSketchObject()->Constraints))) {

        edit->FullyConstrained = false;
        // At this point, we do not need to solve the Sketch
        // If we are adding geometry an update can be triggered before the sketch is actually solved.
        // Because a solve is mandatory to any addition (at least to update the DoF of the solver),
        // only when the solver geometry is the same in number than the sketch geometry an update
        // should trigger a redraw. This reduces even more the number of redraws per insertion of geometry

        // solver information is also updated when no matching geometry, so that if a solving fails
        // this failed solving info is presented to the user
        UpdateSolverInformation(); // just update the solver window with the last SketchObject solving information

        if(getSketchObject()->getExternalGeometryCount()+getSketchObject()->getHighestCurveIndex() + 1 ==
            getSolvedSketch().getGeometrySize()) {
            Gui::MDIView *mdi = Gui::Application::Instance->editDocument()->getActiveView();
            if (mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId()))
                draw(false,true);

            signalConstraintsChanged();
        }

        if(prop != &getSketchObject()->Constraints)
            signalElementsChanged();
    }
}

void ViewProviderSketch::onChanged(const App::Property *prop)
{
    // call father
    PartGui::ViewProvider2DObjectGrid::onChanged(prop);
}

void ViewProviderSketch::attach(App::DocumentObject *pcFeat)
{
    ViewProviderPart::attach(pcFeat);
}

void ViewProviderSketch::setupContextMenu(QMenu *menu, QObject *receiver, const char *member)
{
    menu->addAction(tr("Edit sketch"), receiver, member);
}

bool ViewProviderSketch::setEdit(int ModNum)
{
    // When double-clicking on the item for this sketch the
    // object unsets and sets its edit mode without closing
    // the task panel
    Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
    TaskDlgEditSketch *sketchDlg = qobject_cast<TaskDlgEditSketch *>(dlg);
    if (sketchDlg && sketchDlg->getSketchView() != this)
        sketchDlg = 0; // another sketch left open its task panel
    if (dlg && !sketchDlg) {
        QMessageBox msgBox;
        msgBox.setText(tr("A dialog is already open in the task panel"));
        msgBox.setInformativeText(tr("Do you want to close this dialog?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes)
            Gui::Control().reject();
        else
            return false;
    }

    Sketcher::SketchObject* sketch = getSketchObject();
    if (!sketch->evaluateConstraints()) {
        QMessageBox box(Gui::getMainWindow());
        box.setIcon(QMessageBox::Critical);
        box.setWindowTitle(tr("Invalid sketch"));
        box.setText(tr("Do you want to open the sketch validation tool?"));
        box.setInformativeText(tr("The sketch is invalid and cannot be edited."));
        box.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        box.setDefaultButton(QMessageBox::Yes);
        switch (box.exec())
        {
        case QMessageBox::Yes:
            Gui::Control().showDialog(new TaskSketcherValidation(getSketchObject()));
            break;
        default:
            break;
        }
        return false;
    }

    // clear the selection (convenience)
    Gui::Selection().clearSelection();
    Gui::Selection().rmvPreselect();

    this->attachSelection();

    // create the container for the additional edit data
    assert(!edit);
    edit = new EditData();

    // Init icon, font and marker sizes
    initItemsSizes();

    ParameterGrp::handle hSketch = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    edit->handleEscapeButton = !hSketch->GetBool("LeaveSketchWithEscape", true);

    createEditInventorNodes();

    auto editDoc = Gui::Application::Instance->editDocument();
    App::DocumentObject *editObj = getSketchObject();
    std::string editSubName;
    ViewProviderDocumentObject *editVp = 0;
    if(editDoc) {
        editDoc->getInEdit(&editVp,&editSubName);
        if(editVp)
            editObj = editVp->getObject();
    }

    //visibility automation
    try{
        Gui::Command::addModule(Gui::Command::Gui,"Show");
        try{
            QString cmdstr = QString::fromLatin1(
                        "ActiveSketch = App.getDocument('%1').getObject('%2')\n"
                        "tv = Show.TempoVis(App.ActiveDocument, tag= ActiveSketch.ViewObject.TypeId)\n"
                        "ActiveSketch.ViewObject.TempoVis = tv\n"
                        "if ActiveSketch.ViewObject.EditingWorkbench:\n"
                        "  tv.activateWorkbench(ActiveSketch.ViewObject.EditingWorkbench)\n"
                        "if ActiveSketch.ViewObject.HideDependent:\n"
                        "  tv.hide(tv.get_all_dependent(%3, '%4'))\n"
                        "if ActiveSketch.ViewObject.ShowSupport:\n"
                        "  tv.show([ref[0] for ref in ActiveSketch.Support if not ref[0].isDerivedFrom(\"PartDesign::Plane\")])\n"
                        "if ActiveSketch.ViewObject.ShowLinks:\n"
                        "  tv.show([ref[0] for ref in ActiveSketch.ExternalGeometry])\n"
                        "  tv.sketchClipPlane(ActiveSketch, ActiveSketch.ViewObject.SectionView)\n"
                        "tv.hide(ActiveSketch)\n"
                        "del(tv)\n"
                        "del(ActiveSketch)\n"
                        ).arg(QString::fromLatin1(getDocument()->getDocument()->getName()),
                              QString::fromLatin1(getSketchObject()->getNameInDocument()),
                              QString::fromLatin1(Gui::Command::getObjectCmd(editObj).c_str()),
                              QString::fromLatin1(editSubName.c_str()));
            QByteArray cmdstr_bytearray = cmdstr.toLatin1();
            Gui::Command::runCommand(Gui::Command::Gui, cmdstr_bytearray);
        } catch (Base::PyException &e){
            Base::Console().Error("ViewProviderSketch::setEdit: visibility automation failed with an error: \n");
            e.ReportException();
        }
    } catch (Base::PyException &){
        Base::Console().Warning("ViewProviderSketch::setEdit: could not import Show module. Visibility automation will not work.\n");
    }

    TightGrid.setValue(false);

    ViewProvider2DObjectGrid::setEdit(ModNum); // notify to handle grid according to edit mode property

    float transparency;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    // set the point color
    unsigned long color = (unsigned long)(VertexColor.getPackedValue());
    color = hGrp->GetUnsigned("EditedVertexColor", color);
    VertexColor.setPackedValue((uint32_t)color, transparency);
    // set the curve color
    color = (unsigned long)(CurveColor.getPackedValue());
    color = hGrp->GetUnsigned("EditedEdgeColor", color);
    CurveColor.setPackedValue((uint32_t)color, transparency);
    // set the create line (curve) color
    color = (unsigned long)(CreateCurveColor.getPackedValue());
    color = hGrp->GetUnsigned("CreateLineColor", color);
    CreateCurveColor.setPackedValue((uint32_t)color, transparency);
    // set the construction curve color
    color = (unsigned long)(CurveDraftColor.getPackedValue());
    color = hGrp->GetUnsigned("ConstructionColor", color);
    CurveDraftColor.setPackedValue((uint32_t)color, transparency);
    // set the internal alignment geometry color
    color = (unsigned long)(InternalAlignedGeoColor.getPackedValue());
    color = hGrp->GetUnsigned("InternalAlignedGeoColor", color);
    InternalAlignedGeoColor.setPackedValue((uint32_t)color, transparency);
    // set the color for a fully constrained element
    color = (unsigned long)(FullyConstraintElementColor.getPackedValue());
    color = hGrp->GetUnsigned("FullyConstraintElementColor", color);
    FullyConstraintElementColor.setPackedValue((uint32_t)color, transparency);
    // set the color for fully constrained construction element
    color = (unsigned long)(FullyConstraintConstructionElementColor.getPackedValue());
    color = hGrp->GetUnsigned("FullyConstraintConstructionElementColor", color);
    FullyConstraintConstructionElementColor.setPackedValue((uint32_t)color, transparency);
    // set the color for fully constrained internal alignment element
    color = (unsigned long)(FullyConstraintInternalAlignmentColor.getPackedValue());
    color = hGrp->GetUnsigned("FullyConstraintInternalAlignmentColor", color);
    FullyConstraintInternalAlignmentColor.setPackedValue((uint32_t)color, transparency);
    // set the color for fully constrained construction points
    color = (unsigned long)(FullyConstraintConstructionPointColor.getPackedValue());
    color = hGrp->GetUnsigned("FullyConstraintConstructionPointColor", color);
    FullyConstraintConstructionPointColor.setPackedValue((uint32_t)color, transparency);
    // set fullyconstraint element color
    color = (unsigned long)(FullyConstraintElementColor.getPackedValue());
    color = hGrp->GetUnsigned("FullyConstraintElementColor", color);
    FullyConstraintElementColor.setPackedValue((uint32_t)color, transparency);
    // set the cross lines color
    //CrossColorV.setPackedValue((uint32_t)color, transparency);
    //CrossColorH.setPackedValue((uint32_t)color, transparency);
    // set invalid sketch color
    color = (unsigned long)(InvalidSketchColor.getPackedValue());
    color = hGrp->GetUnsigned("InvalidSketchColor", color);
    InvalidSketchColor.setPackedValue((uint32_t)color, transparency);
    // set the fully constrained color
    color = (unsigned long)(FullyConstrainedColor.getPackedValue());
    color = hGrp->GetUnsigned("FullyConstrainedColor", color);
    FullyConstrainedColor.setPackedValue((uint32_t)color, transparency);
    // set the constraint dimension color
    color = (unsigned long)(ConstrDimColor.getPackedValue());
    color = hGrp->GetUnsigned("ConstrainedDimColor", color);
    ConstrDimColor.setPackedValue((uint32_t)color, transparency);
    // set the constraint color
    color = (unsigned long)(ConstrIcoColor.getPackedValue());
    color = hGrp->GetUnsigned("ConstrainedIcoColor", color);
    ConstrIcoColor.setPackedValue((uint32_t)color, transparency);
    // set non-driving constraint color
    color = (unsigned long)(NonDrivingConstrDimColor.getPackedValue());
    color = hGrp->GetUnsigned("NonDrivingConstrDimColor", color);
    NonDrivingConstrDimColor.setPackedValue((uint32_t)color, transparency);
    // set expression based constraint color
    color = (unsigned long)(ExprBasedConstrDimColor.getPackedValue());
    color = hGrp->GetUnsigned("ExprBasedConstrDimColor", color);
    ExprBasedConstrDimColor.setPackedValue((uint32_t)color, transparency);
    // set expression based constraint color
    color = (unsigned long)(DeactivatedConstrDimColor.getPackedValue());
    color = hGrp->GetUnsigned("DeactivatedConstrDimColor", color);
    DeactivatedConstrDimColor.setPackedValue((uint32_t)color, transparency);

    // set the external geometry color
    color = (unsigned long)(CurveExternalColor.getPackedValue());
    color = hGrp->GetUnsigned("ExternalColor", color);
    CurveExternalColor.setPackedValue((uint32_t)color, transparency);

    // set the highlight color
    unsigned long highlight = (unsigned long)(PreselectColor.getPackedValue());
    highlight = hGrp->GetUnsigned("HighlightColor", highlight);
    PreselectColor.setPackedValue((uint32_t)highlight, transparency);
    // set the selection color
    highlight = (unsigned long)(SelectColor.getPackedValue());
    highlight = hGrp->GetUnsigned("SelectionColor", highlight);
    SelectColor.setPackedValue((uint32_t)highlight, transparency);

    // start the edit dialog
    if (sketchDlg)
        Gui::Control().showDialog(sketchDlg);
    else
        Gui::Control().showDialog(new TaskDlgEditSketch(this));

    // This call to the solver is needed to initialize the DoF and solve time controls
    // The false parameter indicates that the geometry of the SketchObject shall not be updateData
    // so as not to trigger an onChanged that would set the document as modified and trigger a recompute
    // if we just close the sketch without touching anything.
    if (getSketchObject()->Support.getValue()) {
        if (!getSketchObject()->evaluateSupport())
            getSketchObject()->validateExternalLinks();
    }

    // There are geometry extensions introduced by the solver and geometry extensions introduced by the viewprovider.
    // 1. It is important that the solver has geometry with updated extensions.
    // 2. It is important that the viewprovider has up-to-date solver information
    //
    // The decision is to maintain the "first solve then draw" order, which is consistent with the rest of the Sketcher
    // for example in geometry creation. Then, the ViewProvider is responsible for updating the solver geometry when
    // appropriate, as it is the ViewProvider that is introducing its geometry extensions.
    //
    // In order to have updated solver information, solve must take "true", this cause the Geometry property to be updated
    // with the solver information, including solver extensions, and triggers a draw(true) via ViewProvider::UpdateData.
    getSketchObject()->solve(true);

    connectUndoDocument = getDocument()
        ->signalUndoDocument.connect(boost::bind(&ViewProviderSketch::slotUndoDocument, this, bp::_1));
    connectRedoDocument = getDocument()
        ->signalRedoDocument.connect(boost::bind(&ViewProviderSketch::slotRedoDocument, this, bp::_1));

    // Enable solver initial solution update while dragging.
    ParameterGrp::handle hGrp2 = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");

    getSketchObject()->setRecalculateInitialSolutionWhileMovingPoint(hGrp2->GetBool("RecalculateInitialSolutionWhileDragging",true));

    // intercept del key press from main app
    listener = new ShortcutListener(this);

    Gui::getMainWindow()->installEventFilter(listener);

    return true;
}

QString ViewProviderSketch::appendConflictMsg(const std::vector<int> &conflicting)
{
    return appendConstraintMsg(tr("Please remove the following constraint:"),
                        tr("Please remove at least one of the following constraints:"),
                        conflicting);
}

QString ViewProviderSketch::appendRedundantMsg(const std::vector<int> &redundant)
{
    return appendConstraintMsg(tr("Please remove the following redundant constraint:"),
                        tr("Please remove the following redundant constraints:"),
                        redundant);
}

QString ViewProviderSketch::appendPartiallyRedundantMsg(const std::vector<int> &partiallyredundant)
{
    return appendConstraintMsg(tr("The following constraint is partially redundant:"),
                        tr("The following constraints are partially redundant:"),
                        partiallyredundant);
}

QString ViewProviderSketch::appendMalformedMsg(const std::vector<int> &malformed)
{
    return appendConstraintMsg(tr("Please remove the following malformed constraint:"),
                        tr("Please remove the following malformed constraints:"),
                        malformed);
}

QString ViewProviderSketch::appendConstraintMsg(const QString & singularmsg,
                            const QString & pluralmsg,
                            const std::vector<int> &vector)
{
    QString msg;
    QTextStream ss(&msg);
    if (vector.size() > 0) {
        if (vector.size() == 1)
            ss << singularmsg;
        else
            ss << pluralmsg;
        ss << "\n";
        ss << vector[0];
        for (unsigned int i=1; i < vector.size(); i++)
            ss << ", " << vector[i];

        ss << "\n";
    }
    return msg;
}

inline QString intListHelper(const std::vector<int> &ints)
{
    QString results;
    if (ints.size() < 8) { // The 8 is a bit heuristic... more than that and we shift formats
        for (const auto i : ints) {
            if (results.isEmpty())
                results.append(QString::fromUtf8("%1").arg(i));
            else
                results.append(QString::fromUtf8(", %1").arg(i));
        }
    }
    else {
        const int numToShow = 3;
        int more = ints.size() - numToShow;
        for (int i = 0; i < numToShow; ++i) {
            results.append(QString::fromUtf8("%1, ").arg(ints[i]));
        }
        results.append(QCoreApplication::translate("ViewProviderSketch","and %1 more").arg(more));
    }
    std::string testString = results.toStdString();
    return results;
}

void ViewProviderSketch::UpdateSolverInformation()
{
    // Updates Solver Information with the Last solver execution at SketchObject level
    int dofs = getSketchObject()->getLastDoF();
    bool hasConflicts = getSketchObject()->getLastHasConflicts();
    bool hasRedundancies = getSketchObject()->getLastHasRedundancies();
    bool hasPartiallyRedundant = getSketchObject()->getLastHasPartialRedundancies();
    bool hasMalformed    = getSketchObject()->getLastHasMalformedConstraints();

    if (getSketchObject()->Geometry.getSize() == 0) {
        signalSetUp(QString::fromUtf8("empty_sketch"), tr("Empty sketch"), QString(), QString());
    }
    else if (dofs < 0 || hasConflicts) { // over-constrained sketch
        signalSetUp(QString::fromUtf8("conflicting_constraints"),
            tr("Over-constrained: "),
            QString::fromUtf8("#conflicting"),
            QString::fromUtf8("(%1)").arg(intListHelper(getSketchObject()->getLastConflicting())));
    }
    else if (hasMalformed) { // malformed constraints
        signalSetUp(QString::fromUtf8("malformed_constraints"),
            tr("Malformed constraints: "),
            QString::fromUtf8("#malformed"),
            QString::fromUtf8("(%1)").arg(intListHelper(getSketchObject()->getLastMalformedConstraints())));
    }
    else if (hasRedundancies) {
        signalSetUp(QString::fromUtf8("redundant_constraints"),
            tr("Redundant constraints:"),
            QString::fromUtf8("#redundant"),
            QString::fromUtf8("(%1)").arg(intListHelper(getSketchObject()->getLastRedundant())));
    }
    else if (hasPartiallyRedundant) {
        signalSetUp(QString::fromUtf8("partially_redundant_constraints"),
            tr("Partially redundant:"),
            QString::fromUtf8("#partiallyredundant"),
            QString::fromUtf8("(%1)").arg(intListHelper(getSketchObject()->getLastPartiallyRedundant())));
    }
    else if (getSketchObject()->getLastSolverStatus() != 0) {
        signalSetUp(QString::fromUtf8("solver_failed"),
            tr("Solver failed to converge"),
            QString::fromUtf8(""),
            QString::fromUtf8(""));
    } else if (dofs > 0) {
        signalSetUp(QString::fromUtf8("under_constrained"),
            tr("Under constrained:"),
            QString::fromUtf8("#dofs"),
            QString::fromUtf8("%1 %2").arg(dofs).arg(tr("DoF")));
    }
    else {
        signalSetUp(QString::fromUtf8("fully_constrained"), tr("Fully constrained"), QString(), QString());
        // color the sketch as fully constrained if it has geometry (other than the axes)
        if(getSolvedSketch().getGeometrySize()>2)
            edit->FullyConstrained = true;
    }
}


void ViewProviderSketch::createEditInventorNodes(void)
{
    assert(edit);

    edit->EditRoot = new SoSeparator;
    edit->EditRoot->ref();
    edit->EditRoot->setName("Sketch_EditRoot");
    pcRoot->addChild(edit->EditRoot);
    edit->EditRoot->renderCaching = SoSeparator::OFF ;

    // stuff for the points ++++++++++++++++++++++++++++++++++++++
    SoSeparator* pointsRoot = new SoSeparator;
    edit->EditRoot->addChild(pointsRoot);
    edit->PointsMaterials = new SoMaterial;
    edit->PointsMaterials->setName("PointsMaterials");
    pointsRoot->addChild(edit->PointsMaterials);

    SoMaterialBinding *MtlBind = new SoMaterialBinding;
    MtlBind->setName("PointsMaterialBinding");
    MtlBind->value = SoMaterialBinding::PER_VERTEX;
    pointsRoot->addChild(MtlBind);

    edit->PointsCoordinate = new SoCoordinate3;
    edit->PointsCoordinate->setName("PointsCoordinate");
    pointsRoot->addChild(edit->PointsCoordinate);

    edit->PointsDrawStyle = new SoDrawStyle;
    edit->PointsDrawStyle->setName("PointsDrawStyle");
    edit->PointsDrawStyle->pointSize = 8 * edit->pixelScalingFactor;
    pointsRoot->addChild(edit->PointsDrawStyle);

    edit->PointSet = new SoMarkerSet;
    edit->PointSet->setName("PointSet");
    edit->PointSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_FILLED", edit->MarkerSize);
    pointsRoot->addChild(edit->PointSet);

    // stuff for the Curves +++++++++++++++++++++++++++++++++++++++
    SoSeparator* curvesRoot = new SoSeparator;
    edit->EditRoot->addChild(curvesRoot);
    edit->CurvesMaterials = new SoMaterial;
    edit->CurvesMaterials->setName("CurvesMaterials");
    curvesRoot->addChild(edit->CurvesMaterials);

    MtlBind = new SoMaterialBinding;
    MtlBind->setName("CurvesMaterialsBinding");
    MtlBind->value = SoMaterialBinding::PER_FACE;
    curvesRoot->addChild(MtlBind);

    edit->CurvesCoordinate = new SoCoordinate3;
    edit->CurvesCoordinate->setName("CurvesCoordinate");
    curvesRoot->addChild(edit->CurvesCoordinate);

    edit->CurvesDrawStyle = new SoDrawStyle;
    edit->CurvesDrawStyle->setName("CurvesDrawStyle");
    edit->CurvesDrawStyle->lineWidth = 3 * edit->pixelScalingFactor;
    curvesRoot->addChild(edit->CurvesDrawStyle);

    edit->CurveSet = new SoLineSet;
    edit->CurveSet->setName("CurvesLineSet");
    curvesRoot->addChild(edit->CurveSet);

    // stuff for the RootCross lines +++++++++++++++++++++++++++++++++++++++
    SoGroup* crossRoot = new Gui::SoSkipBoundingGroup;
    edit->pickStyleAxes = new SoPickStyle();
    edit->pickStyleAxes->style = SoPickStyle::SHAPE;
    crossRoot->addChild(edit->pickStyleAxes);
    edit->EditRoot->addChild(crossRoot);
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("RootCrossMaterialBinding");
    MtlBind->value = SoMaterialBinding::PER_FACE;
    crossRoot->addChild(MtlBind);

    edit->RootCrossDrawStyle = new SoDrawStyle;
    edit->RootCrossDrawStyle->setName("RootCrossDrawStyle");
    edit->RootCrossDrawStyle->lineWidth = 2 * edit->pixelScalingFactor;
    crossRoot->addChild(edit->RootCrossDrawStyle);

    edit->RootCrossMaterials = new SoMaterial;
    edit->RootCrossMaterials->setName("RootCrossMaterials");
    edit->RootCrossMaterials->diffuseColor.set1Value(0,CrossColorH);
    edit->RootCrossMaterials->diffuseColor.set1Value(1,CrossColorV);
    crossRoot->addChild(edit->RootCrossMaterials);

    edit->RootCrossCoordinate = new SoCoordinate3;
    edit->RootCrossCoordinate->setName("RootCrossCoordinate");
    crossRoot->addChild(edit->RootCrossCoordinate);

    edit->RootCrossSet = new SoLineSet;
    edit->RootCrossSet->setName("RootCrossLineSet");
    crossRoot->addChild(edit->RootCrossSet);

    // stuff for the EditCurves +++++++++++++++++++++++++++++++++++++++
    SoSeparator* editCurvesRoot = new SoSeparator;
    edit->EditRoot->addChild(editCurvesRoot);
    edit->EditCurvesMaterials = new SoMaterial;
    edit->EditCurvesMaterials->setName("EditCurvesMaterials");
    editCurvesRoot->addChild(edit->EditCurvesMaterials);

    edit->EditCurvesCoordinate = new SoCoordinate3;
    edit->EditCurvesCoordinate->setName("EditCurvesCoordinate");
    editCurvesRoot->addChild(edit->EditCurvesCoordinate);

    edit->EditCurvesDrawStyle = new SoDrawStyle;
    edit->EditCurvesDrawStyle->setName("EditCurvesDrawStyle");
    edit->EditCurvesDrawStyle->lineWidth = 3 * edit->pixelScalingFactor;
    editCurvesRoot->addChild(edit->EditCurvesDrawStyle);

    edit->EditCurveSet = new SoLineSet;
    edit->EditCurveSet->setName("EditCurveLineSet");
    editCurvesRoot->addChild(edit->EditCurveSet);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    float transparency;
    SbColor cursorTextColor(0,0,1);
    cursorTextColor.setPackedValue((uint32_t)hGrp->GetUnsigned("CursorTextColor", cursorTextColor.getPackedValue()), transparency);

    // stuff for the EditMarkers +++++++++++++++++++++++++++++++++++++++
    SoSeparator* editMarkersRoot = new SoSeparator;
    edit->EditRoot->addChild(editMarkersRoot);
    edit->EditMarkersMaterials = new SoMaterial;
    edit->EditMarkersMaterials->setName("EditMarkersMaterials");
    editMarkersRoot->addChild(edit->EditMarkersMaterials);

    edit->EditMarkersCoordinate = new SoCoordinate3;
    edit->EditMarkersCoordinate->setName("EditMarkersCoordinate");
    editMarkersRoot->addChild(edit->EditMarkersCoordinate);

    edit->EditMarkersDrawStyle = new SoDrawStyle;
    edit->EditMarkersDrawStyle->setName("EditMarkersDrawStyle");
    edit->EditMarkersDrawStyle->pointSize = 8 * edit->pixelScalingFactor;
    editMarkersRoot->addChild(edit->EditMarkersDrawStyle);

    edit->EditMarkerSet = new SoMarkerSet;
    edit->EditMarkerSet->setName("EditMarkerSet");
    edit->EditMarkerSet->markerIndex = Gui::Inventor::MarkerBitmaps::getMarkerIndex("CIRCLE_LINE", edit->MarkerSize);
    editMarkersRoot->addChild(edit->EditMarkerSet);

    // stuff for the edit coordinates ++++++++++++++++++++++++++++++++++++++
    SoSeparator *Coordsep = new SoSeparator();
    SoPickStyle* ps = new SoPickStyle();
    ps->style.setValue(SoPickStyle::UNPICKABLE);
    Coordsep->addChild(ps);
    Coordsep->setName("CoordSeparator");
    // no caching for frequently-changing data structures
    Coordsep->renderCaching = SoSeparator::OFF;

    SoMaterial *CoordTextMaterials = new SoMaterial;
    CoordTextMaterials->setName("CoordTextMaterials");
    CoordTextMaterials->diffuseColor = cursorTextColor;
    Coordsep->addChild(CoordTextMaterials);

    SoFont *font = new SoFont();
    font->size.setValue(edit->coinFontSize);

    Coordsep->addChild(font);

    edit->textPos = new SoTranslation();
    Coordsep->addChild(edit->textPos);

    edit->textX = new SoText2();
    edit->textX->justification = SoText2::LEFT;
    edit->textX->string = "";
    Coordsep->addChild(edit->textX);
    edit->EditRoot->addChild(Coordsep);

    // group node for the Constraint visual +++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("ConstraintMaterialBinding");
    MtlBind->value = SoMaterialBinding::OVERALL ;
    edit->EditRoot->addChild(MtlBind);

    // use small line width for the Constraints
    edit->ConstraintDrawStyle = new SoDrawStyle;
    edit->ConstraintDrawStyle->setName("ConstraintDrawStyle");
    edit->ConstraintDrawStyle->lineWidth = 1 * edit->pixelScalingFactor;
    edit->EditRoot->addChild(edit->ConstraintDrawStyle);

    // add the group where all the constraints has its SoSeparator
    edit->constrGroup = new SmSwitchboard();
    edit->constrGroup->setName("ConstraintGroup");
    edit->EditRoot->addChild(edit->constrGroup);

    // group node for the Geometry information visual +++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->setName("InformationMaterialBinding");
    MtlBind->value = SoMaterialBinding::OVERALL ;
    edit->EditRoot->addChild(MtlBind);

    // use small line width for the information visual
    edit->InformationDrawStyle = new SoDrawStyle;
    edit->InformationDrawStyle->setName("InformationDrawStyle");
    edit->InformationDrawStyle->lineWidth = 1 * edit->pixelScalingFactor;
    edit->EditRoot->addChild(edit->InformationDrawStyle);

    // add the group where all the information entity has its SoSeparator
    edit->infoGroup = new SoGroup();
    edit->infoGroup->setName("InformationGroup");
    edit->EditRoot->addChild(edit->infoGroup);
}

void ViewProviderSketch::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);
    TightGrid.setValue(true);

    if(listener) {
        Gui::getMainWindow()->removeEventFilter(listener);
        delete listener;
    }

    if (edit) {
        if (edit->sketchHandler)
            deactivateHandler();

        Gui::coinRemoveAllChildren(edit->EditRoot);
        pcRoot->removeChild(edit->EditRoot);
        edit->EditRoot->unref();

        delete edit;
        edit = 0;
        this->detachSelection();

        App::AutoTransaction trans("Sketch recompute");
        try {
            // and update the sketch
            // getSketchObject()->getDocument()->recompute();
            Gui::Command::updateActive();
        }
        catch (...) {
        }
    }

    // clear the selection and set the new/edited sketch(convenience)
    Gui::Selection().clearSelection();
    Gui::Selection().addSelection(editDocName.c_str(),editObjName.c_str(),editSubName.c_str());

    connectUndoDocument.disconnect();
    connectRedoDocument.disconnect();

    // when pressing ESC make sure to close the dialog
    Gui::Control().closeDialog();

    //visibility autoation
    try{
        QString cmdstr = QString::fromLatin1(
                    "ActiveSketch = App.getDocument('%1').getObject('%2')\n"
                    "tv = ActiveSketch.ViewObject.TempoVis\n"
                    "if tv:\n"
                    "  tv.restore()\n"
                    "ActiveSketch.ViewObject.TempoVis = None\n"
                    "del(tv)\n"
                    "del(ActiveSketch)\n"
                    ).arg(QString::fromLatin1(getDocument()->getDocument()->getName())).arg(
                          QString::fromLatin1(getSketchObject()->getNameInDocument()));
        QByteArray cmdstr_bytearray = cmdstr.toLatin1();
        Gui::Command::runCommand(Gui::Command::Gui, cmdstr_bytearray);
    } catch (Base::PyException &e){
        Base::Console().Error("ViewProviderSketch::unsetEdit: visibility automation failed with an error: \n");
        e.ReportException();
    }

    ViewProvider2DObjectGrid::unsetEdit(ModNum); // notify grid that edit mode is being left
}

void ViewProviderSketch::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);
    //visibility automation: save camera
    if (! this->TempoVis.getValue().isNone()){
        try{
            QString cmdstr = QString::fromLatin1(
                        "ActiveSketch = App.getDocument('%1').getObject('%2')\n"
                        "if ActiveSketch.ViewObject.RestoreCamera:\n"
                        "  ActiveSketch.ViewObject.TempoVis.saveCamera()\n"
                        "  if ActiveSketch.ViewObject.ForceOrtho:\n"
                        "    ActiveSketch.ViewObject.Document.ActiveView.setCameraType('Orthographic')\n"
                        ).arg(QString::fromLatin1(getDocument()->getDocument()->getName())).arg(
                              QString::fromLatin1(getSketchObject()->getNameInDocument()));
            QByteArray cmdstr_bytearray = cmdstr.toLatin1();
            Gui::Command::runCommand(Gui::Command::Gui, cmdstr_bytearray);
        } catch (Base::PyException &e){
            Base::Console().Error("ViewProviderSketch::setEdit: visibility automation failed with an error: \n");
            e.ReportException();
        }
    }

    auto editDoc = Gui::Application::Instance->editDocument();
    editDocName.clear();
    if(editDoc) {
        ViewProviderDocumentObject *parent=0;
        editDoc->getInEdit(&parent,&editSubName);
        if(parent) {
            editDocName = editDoc->getDocument()->getName();
            editObjName = parent->getObject()->getNameInDocument();
        }
    }
    if(editDocName.empty()) {
        editDocName = getObject()->getDocument()->getName();
        editObjName = getObject()->getNameInDocument();
        editSubName.clear();
    }
    const char *dot = strrchr(editSubName.c_str(),'.');
    if(!dot)
        editSubName.clear();
    else
        editSubName.resize(dot-editSubName.c_str()+1);

    Base::Placement plm = getEditingPlacement();
    Base::Rotation tmp(plm.getRotation());

    SbRotation rot((float)tmp[0],(float)tmp[1],(float)tmp[2],(float)tmp[3]);

    // Will the sketch be visible from the new position (#0000957)?
    //
    SoCamera* camera = viewer->getSoRenderManager()->getCamera();
    SbVec3f curdir; // current view direction
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), curdir);
    SbVec3f focal = camera->position.getValue() +
                    camera->focalDistance.getValue() * curdir;

    SbVec3f newdir; // future view direction
    rot.multVec(SbVec3f(0, 0, -1), newdir);
    SbVec3f newpos = focal - camera->focalDistance.getValue() * newdir;

    SbVec3f plnpos = Base::convertTo<SbVec3f>(plm.getPosition());
    double dist = (plnpos - newpos).dot(newdir);
    if (dist < 0) {
        float focalLength = camera->focalDistance.getValue() - dist + 5;
        camera->position = focal - focalLength * curdir;
        camera->focalDistance.setValue(focalLength);
    }

    viewer->setCameraOrientation(rot);

    viewer->setEditing(true);
    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(false);

    viewer->addGraphicsItem(rubberband);
    rubberband->setViewer(viewer);

    viewer->setupEditingRoot();
}

void ViewProviderSketch::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    viewer->removeGraphicsItem(rubberband);
    viewer->setEditing(false);
    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(true);
}

void ViewProviderSketch::setPositionText(const Base::Vector2d &Pos, const SbString &text)
{
    edit->textX->string = text;
    edit->textPos->translation = SbVec3f(Pos.x,Pos.y,zText);
}

void ViewProviderSketch::setPositionText(const Base::Vector2d &Pos)
{
    SbString text;
    text.sprintf(" (%.1f,%.1f)", Pos.x, Pos.y);
    setPositionText(Pos,text);
}

void ViewProviderSketch::resetPositionText(void)
{
    edit->textX->string = "";
}

void ViewProviderSketch::setPreselectPoint(int PreselectPoint)
{
    if (edit) {
        int oldPtId = -1;
        if (edit->PreselectPoint != -1)
            oldPtId = edit->PreselectPoint + 1;
        else if (edit->PreselectCross == 0)
            oldPtId = 0;
        int newPtId = PreselectPoint + 1;
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        float x,y,z;
        if (oldPtId != -1 &&
            edit->SelPointSet.find(oldPtId) == edit->SelPointSet.end()) {
            // send to background
            pverts[oldPtId].getValue(x,y,z);
            pverts[oldPtId].setValue(x,y,zLowPoints);
        }
        // bring to foreground
        pverts[newPtId].getValue(x,y,z);
        pverts[newPtId].setValue(x,y,zHighlight);
        edit->PreselectPoint = PreselectPoint;
        edit->PointsCoordinate->point.finishEditing();
    }
}

void ViewProviderSketch::resetPreselectPoint(void)
{
    if (edit) {
        int oldPtId = -1;
        if (edit->PreselectPoint != -1)
            oldPtId = edit->PreselectPoint + 1;
        else if (edit->PreselectCross == 0)
            oldPtId = 0;
        if (oldPtId != -1 &&
            edit->SelPointSet.find(oldPtId) == edit->SelPointSet.end()) {
            // send to background
            SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
            float x,y,z;
            pverts[oldPtId].getValue(x,y,z);
            pverts[oldPtId].setValue(x,y,zLowPoints);
            edit->PointsCoordinate->point.finishEditing();
        }
        edit->PreselectPoint = -1;
    }
}

void ViewProviderSketch::addSelectPoint(int SelectPoint)
{
    if (edit) {
        int PtId = SelectPoint + 1;
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        // bring to foreground
        float x,y,z;
        pverts[PtId].getValue(x,y,z);
        pverts[PtId].setValue(x,y,zHighlight);
        edit->SelPointSet.insert(PtId);
        edit->PointsCoordinate->point.finishEditing();
    }
}

void ViewProviderSketch::removeSelectPoint(int SelectPoint)
{
    if (edit) {
        int PtId = SelectPoint + 1;
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        // send to background
        float x,y,z;
        pverts[PtId].getValue(x,y,z);
        pverts[PtId].setValue(x,y,zLowPoints);
        edit->SelPointSet.erase(PtId);
        edit->PointsCoordinate->point.finishEditing();
    }
}

void ViewProviderSketch::clearSelectPoints(void)
{
    if (edit) {
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        // send to background
        float x,y,z;
        for (std::set<int>::const_iterator it=edit->SelPointSet.begin();
             it != edit->SelPointSet.end(); ++it) {
            pverts[*it].getValue(x,y,z);
            pverts[*it].setValue(x,y,zLowPoints);
        }
        edit->PointsCoordinate->point.finishEditing();
        edit->SelPointSet.clear();
    }
}

int ViewProviderSketch::getPreselectPoint(void) const
{
    if (edit)
        return edit->PreselectPoint;
    return -1;
}

int ViewProviderSketch::getPreselectCurve(void) const
{
    if (edit)
        return edit->PreselectCurve;
    return -1;
}

int ViewProviderSketch::getPreselectCross(void) const
{
    if (edit)
        return edit->PreselectCross;
    return -1;
}

Sketcher::SketchObject *ViewProviderSketch::getSketchObject(void) const
{
    return dynamic_cast<Sketcher::SketchObject *>(pcObject);
}

const Sketcher::Sketch &ViewProviderSketch::getSolvedSketch(void) const
{
    return const_cast<const Sketcher::SketchObject *>(getSketchObject())->getSolvedSketch();
}

void ViewProviderSketch::deleteSelected()
{
    std::vector<Gui::SelectionObject> selection;
    selection = Gui::Selection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        Base::Console().Warning("Delete: Selection not restricted to one sketch and its subelements");
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    if(SubNames.size()>0) {
        App::Document* doc = getSketchObject()->getDocument();

        doc->openTransaction("Delete sketch geometry");

        onDelete(SubNames);

        doc->commitTransaction();
    }
}

bool ViewProviderSketch::onDelete(const std::vector<std::string> &subList)
{
    if (edit) {
        std::vector<std::string> SubNames = subList;

        Gui::Selection().clearSelection();
        resetPreselectPoint();
        edit->PreselectCurve = -1;
        edit->PreselectCross = -1;
        edit->PreselectConstraintSet.clear();

        std::set<int> delInternalGeometries, delExternalGeometries, delCoincidents, delConstraints;
        // go through the selected subelements
        for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
            if (it->size() > 4 && it->substr(0,4) == "Edge") {
                int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
                if( GeoId >= 0 ) {
                    delInternalGeometries.insert(GeoId);
                }
                else
                    delExternalGeometries.insert(Sketcher::GeoEnum::RefExt - GeoId);
            } else if (it->size() > 12 && it->substr(0,12) == "ExternalEdge") {
                int GeoId = std::atoi(it->substr(12,4000).c_str()) - 1;
                delExternalGeometries.insert(GeoId);
            } else if (it->size() > 6 && it->substr(0,6) == "Vertex") {
                int VtId = std::atoi(it->substr(6,4000).c_str()) - 1;
                int GeoId;
                Sketcher::PointPos PosId;
                getSketchObject()->getGeoVertexIndex(VtId, GeoId, PosId);
                if (getSketchObject()->getGeometry(GeoId)->getTypeId()
                    == Part::GeomPoint::getClassTypeId()) {
                    if(GeoId>=0)
                        delInternalGeometries.insert(GeoId);
                    else
                        delExternalGeometries.insert(Sketcher::GeoEnum::RefExt - GeoId);
                }
                else
                    delCoincidents.insert(VtId);
            } else if (*it == "RootPoint") {
                delCoincidents.insert(Sketcher::GeoEnum::RtPnt);
            } else if (it->size() > 10 && it->substr(0,10) == "Constraint") {
                int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(*it);
                delConstraints.insert(ConstrId);
            }
        }

        // We stored the vertices, but is there really a coincident constraint? Check
        const std::vector< Sketcher::Constraint * > &vals = getSketchObject()->Constraints.getValues();

        std::set<int>::const_reverse_iterator rit;

        for (rit = delConstraints.rbegin(); rit != delConstraints.rend(); ++rit) {
            try {
                Gui::cmdAppObjectArgs(getObject(), "delConstraint(%i)", *rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
            }
        }

        for (rit = delCoincidents.rbegin(); rit != delCoincidents.rend(); ++rit) {
            int GeoId;
            PointPos PosId;

            if (*rit == GeoEnum::RtPnt) { // RootPoint
                GeoId = Sketcher::GeoEnum::RtPnt;
                PosId = Sketcher::PointPos::start;
            } else {
                getSketchObject()->getGeoVertexIndex(*rit, GeoId, PosId);
            }

            if (GeoId != GeoEnum::GeoUndef) {
                for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin(); it != vals.end(); ++it) {
                    if (((*it)->Type == Sketcher::Coincident) && (((*it)->First == GeoId && (*it)->FirstPos == PosId) ||
                        ((*it)->Second == GeoId && (*it)->SecondPos == PosId)) ) {
                        try {
                            Gui::cmdAppObjectArgs(getObject(), "delConstraintOnPoint(%i,%i)", GeoId, (int)PosId);
                        }
                        catch (const Base::Exception& e) {
                            Base::Console().Error("%s\n", e.what());
                        }
                        break;
                    }
                }
            }
        }

        if(!delInternalGeometries.empty()) {
            std::stringstream stream;

            // NOTE: SketchObject delGeometries will sort the array, so filling it here with a reverse iterator would
            // lead to the worst case scenario for sorting.
            auto endit = std::prev(delInternalGeometries.end());
            for (auto it = delInternalGeometries.begin(); it != endit; ++it) {
                stream << *it << ",";
            }

            stream << *endit;

            try {
                Gui::cmdAppObjectArgs(getObject(), "delGeometries([%s])", stream.str().c_str());
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
            }

            stream.str(std::string());
        }

        for (rit = delExternalGeometries.rbegin(); rit != delExternalGeometries.rend(); ++rit) {
            try {
                Gui::cmdAppObjectArgs(getObject(), "delExternal(%i)", *rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
            }
        }

        int ret=getSketchObject()->solve();

        if(ret!=0){
            // if the sketched could not be solved, we first redraw to update the UI geometry as
            // onChanged did not update it.
            UpdateSolverInformation();
            draw(false,true);

            signalConstraintsChanged();
            signalElementsChanged();
        }

        // Notes on solving and recomputing:
        //
        // This function is generally called from StdCmdDelete::activated
        // Since 2015-05-03 that function includes a recompute at the end.

        // Since December 2018, the function is no longer called from StdCmdDelete::activated,
        // as there is an event filter installed that intercepts the del key event. So now we do
        // need to tidy up after ourselves again.

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool autoRecompute = hGrp->GetBool("AutoRecompute",false);

        if (autoRecompute) {
            Gui::Command::updateActive();
        }
        else {
            this->drawConstraintIcons();
            this->updateColor();
        }

        // if in edit not delete the object
        return false;
    }
    // if not in edit delete the whole object
    return PartGui::ViewProviderPart::onDelete(subList);
}

void ViewProviderSketch::showRestoreInformationLayer() {

    visibleInformationChanged = true ;
    draw(false,false);
}

QIcon ViewProviderSketch::mergeColorfulOverlayIcons (const QIcon & orig) const
{
    QIcon mergedicon = orig;

    if(!getSketchObject()->FullyConstrained.getValue()) {
        QPixmap px;

        static const char * const sketcher_notfullyconstrained_xpm[]={
            "9 9 3 1",
            ". c None",
            "# c #dbaf00",
            "a c #ffcc00",
            "##.....##",
            "#a#...#a#",
            "#aa#.#aa#",
            ".#a#.#a#.",
            ".#a#.#a#.",
            ".#a#.#a#.",
            "#aa#.#aa#",
            "#a#...#a#",
            "##.....##"};
        px = QPixmap( sketcher_notfullyconstrained_xpm );

        mergedicon = Gui::BitmapFactoryInst::mergePixmap(mergedicon, px, Gui::BitmapFactoryInst::BottomRight);

    }

    return Gui::ViewProvider::mergeColorfulOverlayIcons (mergedicon);
}
