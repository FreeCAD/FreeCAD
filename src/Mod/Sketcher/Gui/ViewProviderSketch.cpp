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
#endif

#include <Inventor/SbTime.h>
#include <boost/scoped_ptr.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Tools.h>
#include <Base/Parameter.h>
#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection.h>
#include <Gui/Utilities.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/DlgEditFileIncludeProptertyExternal.h>
#include <Gui/SoFCBoundingBox.h>
#include <Gui/SoFCUnifiedSelection.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/Sketch.h>

#include "SoZoomTranslation.h"
#include "SoDatumLabel.h"
#include "EditDatumDialog.h"
#include "ViewProviderSketch.h"
#include "DrawSketchHandler.h"
#include "TaskDlgEditSketch.h"

// The first is used to point at a SoDatumLabel for some
// constraints, and at a SoMaterial for others...
#define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
#define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
#define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
#define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
#define CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION 4
#define CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON 5
#define CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID 6

using namespace SketcherGui;
using namespace Sketcher;

SbColor ViewProviderSketch::VertexColor           (1.0f,0.149f,0.0f);   // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch::CurveColor            (1.0f,1.0f,1.0f);     // #FFFFFF -> (255,255,255)
SbColor ViewProviderSketch::CurveDraftColor       (0.0f,0.0f,0.86f);    // #0000DC -> (  0,  0,220)
SbColor ViewProviderSketch::CurveExternalColor    (0.8f,0.2f,0.6f);     // #CC3399 -> (204, 51,153)
SbColor ViewProviderSketch::CrossColorH           (0.8f,0.4f,0.4f);     // #CC6666 -> (204,102,102)
SbColor ViewProviderSketch::CrossColorV           (0.4f,0.8f,0.4f);     // #66CC66 -> (102,204,102)
SbColor ViewProviderSketch::FullyConstrainedColor (0.0f,1.0f,0.0f);     // #00FF00 -> (  0,255,  0)
SbColor ViewProviderSketch::ConstrDimColor        (1.0f,0.149f,0.0f);   // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch::ConstrIcoColor        (1.0f,0.149f,0.0f);   // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch::PreselectColor        (0.88f,0.88f,0.0f);   // #E1E100 -> (225,225,  0)
SbColor ViewProviderSketch::SelectColor           (0.11f,0.68f,0.11f);  // #1CAD1C -> ( 28,173, 28)
SbColor ViewProviderSketch::PreselectSelectedColor(0.36f,0.48f,0.11f);  // #5D7B1C -> ( 93,123, 28)
// Variables for holding previous click
SbTime  ViewProviderSketch::prvClickTime;
SbVec3f ViewProviderSketch::prvClickPoint;
SbVec2s ViewProviderSketch::prvCursorPos;
SbVec2s ViewProviderSketch::newCursorPos;

//**************************************************************************
// Edit data structure

/// Data structure while editing the sketch
struct EditData {
    EditData():
    sketchHandler(0),
    editDatumDialog(false),
    buttonPress(false),
    DragPoint(-1),
    DragCurve(-1),
    PreselectPoint(-1),
    PreselectCurve(-1),
    PreselectCross(-1),
    blockedPreselection(false),
    FullyConstrained(false),
    //ActSketch(0),
    EditRoot(0),
    PointsMaterials(0),
    CurvesMaterials(0),
    PointsCoordinate(0),
    CurvesCoordinate(0),
    CurveSet(0), RootCrossSet(0), EditCurveSet(0),
    PointSet(0), pickStyleAxes(0)
    {}

    // pointer to the active handler for new sketch objects
    DrawSketchHandler *sketchHandler;
    bool editDatumDialog;
    bool buttonPress;

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
    std::set<int> PreselectConstraintSet;
    bool blockedPreselection;
    bool FullyConstrained;
    bool visibleBeforeEdit;

    // instance of the solver
    Sketcher::Sketch ActSketch;
    // container to track our own selected parts
    std::set<int> SelPointSet;
    std::set<int> SelCurvSet; // also holds cross axes at -1 and -2
    std::set<int> SelConstraintSet;
    std::vector<int> CurvIdToGeoId; // conversion of SoLineSet index to GeoId

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
    SoCoordinate3 *PointsCoordinate;
    SoCoordinate3 *CurvesCoordinate;
    SoCoordinate3 *RootCrossCoordinate;
    SoCoordinate3 *EditCurvesCoordinate;
    SoLineSet     *CurveSet;  
    SoLineSet     *RootCrossSet;
    SoLineSet     *EditCurveSet;
    SoMarkerSet   *PointSet;

    SoText2       *textX;
    SoTranslation *textPos;

    SoGroup       *constrGroup;
    SoPickStyle   *pickStyleAxes;
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

PROPERTY_SOURCE(SketcherGui::ViewProviderSketch, PartGui::ViewProvider2DObject)


ViewProviderSketch::ViewProviderSketch()
  : edit(0),
    Mode(STATUS_NONE)
{
    // FIXME Should this be placed in here?
    ADD_PROPERTY_TYPE(Autoconstraints,(true),"Auto Constraints",(App::PropertyType)(App::Prop_None),"Create auto constraints");

    sPixmap = "Sketcher_Sketch";
    LineColor.setValue(1,1,1);
    PointColor.setValue(1,1,1);
    PointSize.setValue(4);

    zCross=0.001f;
    zLines=0.005f;
    zConstr=0.006f; // constraint not construction
    zHighLine=0.007f;
    zPoints=0.008f;
    zHighlight=0.009f;
    zText=0.011f;
    zEdit=0.001f;

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
}

ViewProviderSketch::~ViewProviderSketch()
{
    delete rubberband;
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
}

void ViewProviderSketch::deactivateHandler()
{
    assert(edit);
    assert(edit->sketchHandler != 0);
    edit->sketchHandler->deactivated(this);
    edit->sketchHandler->unsetCursor();
    delete(edit->sketchHandler);
    edit->sketchHandler = 0;
    Mode = STATUS_NONE;
}

/// removes the active handler
void ViewProviderSketch::purgeHandler(void)
{
    deactivateHandler();

    // ensure that we are in sketch only selection mode
    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    Gui::View3DInventorViewer *viewer;
    viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(FALSE);
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
            if (edit && edit->editDatumDialog) {
                edit->editDatumDialog = false;
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
                    getSketchObject()->movePoint(edit->DragCurve, Sketcher::none, Base::Vector3d(0,0,0), true);
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
    if (GridSnap.getValue() != false) {
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

void ViewProviderSketch::getCoordsOnSketchPlane(double &u, double &v,const SbVec3f &point, const SbVec3f &normal)
{
    // Plane form
    Base::Vector3d R0(0,0,0),RN(0,0,1),RX(1,0,0),RY(0,1,0);

    // move to position of Sketch
    Base::Placement Plz = getSketchObject()->Placement.getValue();
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
                        (point - prvClickPoint).length() <  dblClickRadius &&
                        (SbTime::getTimeOfDay() - prvClickTime).getValue() < dci) {

                        // Double Click Event Occured
                        editDoubleClicked();
                        // Reset Double Click Static Variables
                        prvClickTime = SbTime();
                        prvClickPoint = SbVec3f(0.0f, 0.0f, 0.0f);
                        Mode = STATUS_NONE;

                    } else {
                        prvClickTime = SbTime::getTimeOfDay();
                        prvClickPoint = point;
                        prvCursorPos = cursorPos;
                        newCursorPos = cursorPos;
                        if (!done)
                            Mode = STATUS_SKETCH_StartRubberBand;
                    }

                    return done;
                }
                case STATUS_SKETCH_UseHandler:
                    return edit->sketchHandler->pressButton(Base::Vector2D(x,y));
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

                        if (Gui::Selection().isSelected(getSketchObject()->getDocument()->getName()
                           ,getSketchObject()->getNameInDocument(),ss.str().c_str()) ) {
                             Gui::Selection().rmvSelection(getSketchObject()->getDocument()->getName()
                                                          ,getSketchObject()->getNameInDocument(), ss.str().c_str());
                        } else {
                            Gui::Selection().addSelection(getSketchObject()->getDocument()->getName()
                                                         ,getSketchObject()->getNameInDocument()
                                                         ,ss.str().c_str()
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
                        if (Gui::Selection().isSelected(getSketchObject()->getDocument()->getName()
                                                       ,getSketchObject()->getNameInDocument(),ss.str().c_str()) ) {
                            Gui::Selection().rmvSelection(getSketchObject()->getDocument()->getName()
                                                         ,getSketchObject()->getNameInDocument(), ss.str().c_str());
                        } else {
                            // Add edge to the selection
                            Gui::Selection().addSelection(getSketchObject()->getDocument()->getName()
                                                         ,getSketchObject()->getNameInDocument()
                                                         ,ss.str().c_str()
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
                        if (Gui::Selection().isSelected(getSketchObject()->getDocument()->getName()
                                                       ,getSketchObject()->getNameInDocument(),ss.str().c_str()) ) {
                            Gui::Selection().rmvSelection(getSketchObject()->getDocument()->getName()
                                                         ,getSketchObject()->getNameInDocument(), ss.str().c_str());
                        } else {
                            // Add cross to the selection
                            Gui::Selection().addSelection(getSketchObject()->getDocument()->getName()
                                                         ,getSketchObject()->getNameInDocument()
                                                         ,ss.str().c_str()
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
                        for(std::set<int>::iterator it = edit->PreselectConstraintSet.begin(); it != edit->PreselectConstraintSet.end(); ++it) {
                            std::stringstream ss;
                            ss << "Constraint" << *it + 1;

                            // If the constraint already selected remove
                            if (Gui::Selection().isSelected(getSketchObject()->getDocument()->getName()
                                                           ,getSketchObject()->getNameInDocument(),ss.str().c_str()) ) {
                                Gui::Selection().rmvSelection(getSketchObject()->getDocument()->getName()
                                                             ,getSketchObject()->getNameInDocument(), ss.str().c_str());
                            } else {
                                // Add constraint to current selection
                                Gui::Selection().addSelection(getSketchObject()->getDocument()->getName()
                                                             ,getSketchObject()->getNameInDocument()
                                                             ,ss.str().c_str()
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
                        if (GeoId != Sketcher::Constraint::GeoUndef && PosId != Sketcher::none) {
                            Gui::Command::openCommand("Drag Point");
                            try {
                                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.movePoint(%i,%i,App.Vector(%f,%f,0),%i)"
                                                       ,getObject()->getNameInDocument()
                                                       ,GeoId, PosId, x-xInit, y-yInit, relative ? 1 : 0
                                                       );
                                Gui::Command::commitCommand();
                                Gui::Command::updateActive();
                            }
                            catch (const Base::Exception& e) {
                                Gui::Command::abortCommand();
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
                            geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) { 
                            Gui::Command::openCommand("Drag Curve");
                            try {
                                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.movePoint(%i,%i,App.Vector(%f,%f,0),%i)"
                                                       ,getObject()->getNameInDocument()
                                                       ,edit->DragCurve, Sketcher::none, x-xInit, y-yInit, relative ? 1 : 0
                                                       );
                                Gui::Command::commitCommand();
                                Gui::Command::updateActive();
                            }
                            catch (const Base::Exception& e) {
                                Gui::Command::abortCommand();
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
                        Gui::Command::openCommand("Drag Constraint");
                        for(std::set<int>::iterator it = edit->DragConstraintSet.begin();
                            it != edit->DragConstraintSet.end(); ++it) {
                            moveConstraint(*it, Base::Vector2D(x, y));
                            //updateColor();
                        }
                        edit->PreselectConstraintSet = edit->DragConstraintSet;
                        edit->DragConstraintSet.clear();
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
                    
                    //disable framebuffer drawing in viewer
                    const_cast<Gui::View3DInventorViewer *>(viewer)->setRenderType(Gui::View3DInventorViewer::Native);
                    
                    // a redraw is required in order to clear the rubberband
                    draw(true);
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_UseHandler:
                    return edit->sketchHandler->releaseButton(Base::Vector2D(x,y));
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
                            //Get Viewer
                            Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
                            Gui::View3DInventorViewer *viewer;
                            viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

                            Gui::MenuItem *geom = new Gui::MenuItem();
                            geom->setCommand("Sketcher geoms");
                            *geom << "Sketcher_CreatePoint"
                                  << "Sketcher_CreateArc"
                                  << "Sketcher_Create3PointArc"
                                  << "Sketcher_CreateCircle"
                                  << "Sketcher_Create3PointCircle"
                                  << "Sketcher_CreateLine"
                                  << "Sketcher_CreatePolyline"
                                  << "Sketcher_CreateRectangle"
                                  << "Sketcher_CreateHexagon"
                                  << "Sketcher_CreateFillet"
                                  << "Sketcher_Trimming"
                                  << "Sketcher_External"
                                  << "Sketcher_ToggleConstruction"
                                /*<< "Sketcher_CreateText"*/
                                /*<< "Sketcher_CreateDraftLine"*/;

                            Gui::Application::Instance->setupContextMenu("View", geom);
                            //Create the Context Menu using the Main View Qt Widget
                            QMenu contextMenu(viewer->getGLWidget());
                            Gui::MenuManager::getInstance()->setupContextMenu(geom, contextMenu);
                            contextMenu.exec(QCursor::pos());

                            return true;
                        }
                    }
                case STATUS_SELECT_Point:
                    break;
                case STATUS_SELECT_Edge:
                    {
                        //Get Viewer
                        Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
                        Gui::View3DInventorViewer *viewer ;
                        viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

                        Gui::MenuItem *geom = new Gui::MenuItem();
                        geom->setCommand("Sketcher constraints");
                        *geom << "Sketcher_ConstrainVertical"
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
                            *geom << "Sketcher_ConstrainParallel"
                                  << "Sketcher_ConstrainPerpendicular";
                        }

                        Gui::Application::Instance->setupContextMenu("View", geom);
                        //Create the Context Menu using the Main View Qt Widget
                        QMenu contextMenu(viewer->getGLWidget());
                        Gui::MenuManager::getInstance()->setupContextMenu(geom, contextMenu);
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

        for(std::set<int>::iterator it = edit->PreselectConstraintSet.begin();
            it != edit->PreselectConstraintSet.end(); ++it) {

            Constraint *Constr = constrlist[*it];

            // if its the right constraint
            if (Constr->Type == Sketcher::Distance ||
                Constr->Type == Sketcher::DistanceX || 
                Constr->Type == Sketcher::DistanceY ||
                Constr->Type == Sketcher::Radius ||
                Constr->Type == Sketcher::Angle ||
                Constr->Type == Sketcher::SnellsLaw ) {

                // Coin's SoIdleSensor causes problems on some platform while Qt seems to work properly (#0001517)
                EditDatumDialog *editDatumDialog = new EditDatumDialog(this, *it);
                QCoreApplication::postEvent(editDatumDialog, new QEvent(QEvent::User));
                edit->editDatumDialog = true; // avoid to double handle "ESC"
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

    bool preselectChanged;
    if (Mode != STATUS_SELECT_Point &&
        Mode != STATUS_SELECT_Edge &&
        Mode != STATUS_SELECT_Constraint &&
        Mode != STATUS_SKETCH_DragPoint &&
        Mode != STATUS_SKETCH_DragCurve &&
        Mode != STATUS_SKETCH_DragConstraint &&
        Mode != STATUS_SKETCH_UseRubberBand) {

        SoPickedPoint *pp = this->getPointOnRay(cursorPos, viewer);
        preselectChanged = detectPreselection(pp, viewer, cursorPos);
        delete pp;
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
            if (!edit->ActSketch.hasConflicts() &&
                edit->PreselectPoint != -1 && edit->DragPoint != edit->PreselectPoint) {
                Mode = STATUS_SKETCH_DragPoint;
                edit->DragPoint = edit->PreselectPoint;
                int GeoId;
                Sketcher::PointPos PosId;
                getSketchObject()->getGeoVertexIndex(edit->DragPoint, GeoId, PosId);
                if (GeoId != Sketcher::Constraint::GeoUndef && PosId != Sketcher::none) {
                    edit->ActSketch.initMove(GeoId, PosId, false);
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
            if (!edit->ActSketch.hasConflicts() &&
                edit->PreselectCurve != -1 && edit->DragCurve != edit->PreselectCurve) {
                Mode = STATUS_SKETCH_DragCurve;
                edit->DragCurve = edit->PreselectCurve;
                edit->ActSketch.initMove(edit->DragCurve, Sketcher::none, false);
                const Part::Geometry *geo = getSketchObject()->getGeometry(edit->DragCurve);
                if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
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
                Base::Vector3d vec(x-xInit,y-yInit,0);
                if (GeoId != Sketcher::Constraint::GeoUndef && PosId != Sketcher::none) {
                    if (edit->ActSketch.movePoint(GeoId, PosId, vec, relative) == 0) {
                        setPositionText(Base::Vector2D(x,y));
                        draw(true);
                        signalSolved(QString::fromLatin1("Solved in %1 sec").arg(edit->ActSketch.SolveTime));
                    } else {
                        signalSolved(QString::fromLatin1("Unsolved (%1 sec)").arg(edit->ActSketch.SolveTime));
                        //Base::Console().Log("Error solving:%d\n",ret);
                    }
                }
            }
            return true;
        case STATUS_SKETCH_DragCurve:
            if (edit->DragCurve != -1) {
                Base::Vector3d vec(x-xInit,y-yInit,0);
                if (edit->ActSketch.movePoint(edit->DragCurve, Sketcher::none, vec, relative) == 0) {
                    setPositionText(Base::Vector2D(x,y));
                    draw(true);
                    signalSolved(QString::fromLatin1("Solved in %1 sec").arg(edit->ActSketch.SolveTime));
                } else {
                    signalSolved(QString::fromLatin1("Unsolved (%1 sec)").arg(edit->ActSketch.SolveTime));
                }
            }
            return true;
        case STATUS_SKETCH_DragConstraint:
            if (edit->DragConstraintSet.empty() == false) {
                for(std::set<int>::iterator it = edit->DragConstraintSet.begin();
                    it != edit->DragConstraintSet.end(); ++it)
                    moveConstraint(*it, Base::Vector2D(x,y));
            }
            return true;
        case STATUS_SKETCH_UseHandler:
            edit->sketchHandler->mouseMove(Base::Vector2D(x,y));
            if (preselectChanged) {
                this->drawConstraintIcons();
                this->updateColor();
            }
            return true;
        case STATUS_SKETCH_StartRubberBand: {
            Mode = STATUS_SKETCH_UseRubberBand;
            rubberband->setWorking(true);
            viewer->setRenderType(Gui::View3DInventorViewer::Image);
            return true;
        }
        case STATUS_SKETCH_UseRubberBand: {
            newCursorPos = cursorPos;
            rubberband->setCoords(prvCursorPos.getValue()[0],
                       viewer->getGLWidget()->height() - prvCursorPos.getValue()[1],
                       newCursorPos.getValue()[0],
                       viewer->getGLWidget()->height() - newCursorPos.getValue()[1]);
            viewer->redraw();
            return true;
        }
        default:
            return false;
    }

    return false;
}

void ViewProviderSketch::moveConstraint(int constNum, const Base::Vector2D &toPos)
{
    // are we in edit?
    if (!edit)
        return;

    const std::vector<Sketcher::Constraint *> &constrlist = getSketchObject()->Constraints.getValues();
    Constraint *Constr = constrlist[constNum];

    int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;
    int extGeoCount = getSketchObject()->getExternalGeometryCount();
    // with memory allocation
    const std::vector<Part::Geometry *> geomlist = edit->ActSketch.extractGeometry(true, true);

    assert(int(geomlist.size()) == extGeoCount + intGeoCount);
    assert((Constr->First >= -extGeoCount && Constr->First < intGeoCount)
           || Constr->First != Constraint::GeoUndef);

    if (Constr->Type == Distance || Constr->Type == DistanceX || Constr->Type == DistanceY ||
        Constr->Type == Radius) {

        Base::Vector3d p1(0.,0.,0.), p2(0.,0.,0.);
        if (Constr->SecondPos != Sketcher::none) { // point to point distance
            p1 = edit->ActSketch.getPoint(Constr->First, Constr->FirstPos);
            p2 = edit->ActSketch.getPoint(Constr->Second, Constr->SecondPos);
        } else if (Constr->Second != Constraint::GeoUndef) { // point to line distance
            p1 = edit->ActSketch.getPoint(Constr->First, Constr->FirstPos);
            const Part::Geometry *geo = GeoById(geomlist, Constr->Second);
            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo);
                Base::Vector3d l2p1 = lineSeg->getStartPoint();
                Base::Vector3d l2p2 = lineSeg->getEndPoint();
                // calculate the projection of p1 onto line2
                p2.ProjToLine(p1-l2p1, l2p2-l2p1);
                p2 += p1;
            } else
                return;
        } else if (Constr->FirstPos != Sketcher::none) {
            p2 = edit->ActSketch.getPoint(Constr->First, Constr->FirstPos);
        } else if (Constr->First != Constraint::GeoUndef) {
            const Part::Geometry *geo = GeoById(geomlist, Constr->First);
            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo);
                p1 = lineSeg->getStartPoint();
                p2 = lineSeg->getEndPoint();
            } else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo);
                double radius = arc->getRadius();
                p1 = arc->getCenter();
                double angle = Constr->LabelPosition;
                if (angle == 10) {
                    double startangle, endangle;
                    arc->getRange(startangle, endangle);
                    angle = (startangle + endangle)/2;
                }
                else {
                    Base::Vector3d tmpDir =  Base::Vector3d(toPos.fX, toPos.fY, 0) - p1;
                    angle = atan2(tmpDir.y, tmpDir.x);
                }
                p2 = p1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
            }
            else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) { 
                const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo);
                double radius = circle->getRadius();
                p1 = circle->getCenter();
                Base::Vector3d tmpDir =  Base::Vector3d(toPos.fX, toPos.fY, 0) - p1;
                double angle = atan2(tmpDir.y, tmpDir.x);
                p2 = p1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
            }
            else 
                return;
        } else
            return;

        Base::Vector3d vec = Base::Vector3d(toPos.fX, toPos.fY, 0) - p2;

        Base::Vector3d dir;
        if (Constr->Type == Distance || Constr->Type == Radius)
            dir = (p2-p1).Normalize();
        else if (Constr->Type == DistanceX)
            dir = Base::Vector3d( (p2.x - p1.x >= FLT_EPSILON) ? 1 : -1, 0, 0);
        else if (Constr->Type == DistanceY)
            dir = Base::Vector3d(0, (p2.y - p1.y >= FLT_EPSILON) ? 1 : -1, 0);

        if (Constr->Type == Radius) {
            Constr->LabelDistance = vec.x * dir.x + vec.y * dir.y;
            Constr->LabelPosition = atan2(dir.y, dir.x);
        } else {
            Base::Vector3d norm(-dir.y,dir.x,0);
            Constr->LabelDistance = vec.x * norm.x + vec.y * norm.y;
            if (Constr->Type == Distance ||
                Constr->Type == DistanceX || Constr->Type == DistanceY) {
                vec = Base::Vector3d(toPos.fX, toPos.fY, 0) - (p2 + p1) / 2;
                Constr->LabelPosition = vec.x * dir.x + vec.y * dir.y;
            }
        }
    }
    else if (Constr->Type == Angle) {

        Base::Vector3d p0(0.,0.,0.);
        if (Constr->Second != Constraint::GeoUndef) { // line to line angle
            Base::Vector3d dir1, dir2;
            if(Constr->Third == Constraint::GeoUndef) { //angle between two lines
                const Part::Geometry *geo1 = GeoById(geomlist, Constr->First);
                const Part::Geometry *geo2 = GeoById(geomlist, Constr->Second);
                if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                    geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                    return;
                const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment *>(geo1);
                const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment *>(geo2);

                bool flip1 = (Constr->FirstPos == end);
                bool flip2 = (Constr->SecondPos == end);
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
                    p0 = Base::Vector3d(x,y,0);
                }
            } else {//angle-via-point
                Base::Vector3d p = edit->ActSketch.getPoint(Constr->Third, Constr->ThirdPos);
                p0 = Base::Vector3d(p.x, p.y, 0);
                dir1 = edit->ActSketch.calculateNormalAtPoint(Constr->First, p.x, p.y);
                dir1.RotateZ(-M_PI/2);//convert to vector of tangency by rotating
                dir2 = edit->ActSketch.calculateNormalAtPoint(Constr->Second, p.x, p.y);
                dir2.RotateZ(-M_PI/2);
            }

        } else if (Constr->First != Constraint::GeoUndef) { // line/arc angle
            const Part::Geometry *geo = GeoById(geomlist, Constr->First);
            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo);
                p0 = (lineSeg->getEndPoint()+lineSeg->getStartPoint())/2;
            }
            else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo);
                p0 = arc->getCenter();
            }
            else {
                return;
            }
        } else
            return;

        Base::Vector3d vec = Base::Vector3d(toPos.fX, toPos.fY, 0) - p0;
        Constr->LabelDistance = vec.Length()/2;
    }

    // delete the cloned objects
    for (std::vector<Part::Geometry *>::const_iterator it=geomlist.begin(); it != geomlist.end(); ++it)
        if (*it) delete *it;

    draw(true);
}

Base::Vector3d ViewProviderSketch::seekConstraintPosition(const Base::Vector3d &origPos,
                                                          const Base::Vector3d &norm,
                                                          const Base::Vector3d &dir, float step,
                                                          const SoNode *constraint)
{
    assert(edit);
    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
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
        return PartGui::ViewProvider2DObject::isSelectable();
}

void ViewProviderSketch::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // are we in edit?
    if (edit) {
        bool handled=false;
        if (Mode == STATUS_SKETCH_UseHandler) {
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
                        addSelectPoint(-1);
                        this->updateColor();
                    }
                    else if (shapetype == "H_Axis") {
                        edit->SelCurvSet.insert(-1);
                        this->updateColor();
                    }
                    else if (shapetype == "V_Axis") {
                        edit->SelCurvSet.insert(-2);
                        this->updateColor();
                    }
                    else if (shapetype.size() > 10 && shapetype.substr(0,10) == "Constraint") {
                        int ConstrId = std::atoi(&shapetype[10]) - 1;
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
                            removeSelectPoint(-1);
                            this->updateColor();
                        }
                        else if (shapetype == "H_Axis") {
                            edit->SelCurvSet.erase(-1);
                            this->updateColor();
                        }
                        else if (shapetype == "V_Axis") {
                            edit->SelCurvSet.erase(-2);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 10 && shapetype.substr(0,10) == "Constraint") {
                            int ConstrId = std::atoi(&shapetype[10]) - 1;
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
            //    new QListWidgetItem(QString::fromAscii(temp.c_str()), selectionView);
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

    for (int i=0; i < edit->constrGroup->getNumChildren(); ++i)
        if (edit->constrGroup->getChild(i) == tailFather) {
            SoSeparator *sep = static_cast<SoSeparator *>(tailFather);
            if(sep->getNumChildren() > CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID) {
                SoInfo *constrIds = NULL;
                if(tail == sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON)) {
                    // First icon was hit
                    constrIds = static_cast<SoInfo *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID));

                } else {
                    // Assume second icon was hit
                    if(CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID<sep->getNumChildren()){
                        constrIds = static_cast<SoInfo *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID));
                    }
                }
                if(constrIds) {
                    QString constrIdsStr = QString::fromAscii(constrIds->string.getValue().getString());
                    if(edit->combinedConstrBoxes.count(constrIdsStr) && dynamic_cast<SoImage *>(tail)) {
                        // If it's a combined constraint icon

                        // Screen dimensions of the icon
                        SbVec3s iconSize = getDisplayedSize(static_cast<SoImage *>(tail));
                        // Center of the icon
                        SbVec2f iconCoords = viewer->screenCoordsOfPath(path);
                        // Coordinates of the mouse cursor on the icon, origin at top-left
                        int iconX = cursorPos[0] - iconCoords[0] + iconSize[0]/2,
                            iconY = iconCoords[1] - cursorPos[1] + iconSize[1]/2;

                        for(ConstrIconBBVec::iterator b = edit->combinedConstrBoxes[constrIdsStr].begin();
                            b != edit->combinedConstrBoxes[constrIdsStr].end(); ++b) {
                            if(b->first.contains(iconX, iconY))
                                // We've found a bounding box that contains the mouse pointer!
                                for(std::set<int>::iterator k = b->second.begin();
                                    k != b->second.end(); ++k)
                                    constrIndices.insert(*k);
                        }
                    } else {
                        // It's a constraint icon, not a combined one
                        QStringList constrIdStrings = constrIdsStr.split(QString::fromAscii(","));
                        while(!constrIdStrings.empty())
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
                if (PtIndex == -1)
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
            Gui::Selection().setPreselect(getSketchObject()->getDocument()->getName()
                                         ,getSketchObject()->getNameInDocument()
                                         ,ss.str().c_str()
                                         ,Point->getPoint()[0]
                                         ,Point->getPoint()[1]
                                         ,Point->getPoint()[2]);
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
                ss << "ExternalEdge" << -GeoIndex - 2; // convert index start from -3 to 1
            bool accepted =
            Gui::Selection().setPreselect(getSketchObject()->getDocument()->getName()
                                         ,getSketchObject()->getNameInDocument()
                                         ,ss.str().c_str()
                                         ,Point->getPoint()[0]
                                         ,Point->getPoint()[1]
                                         ,Point->getPoint()[2]);
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
            Gui::Selection().setPreselect(getSketchObject()->getDocument()->getName()
                                         ,getSketchObject()->getNameInDocument()
                                         ,ss.str().c_str()
                                         ,Point->getPoint()[0]
                                         ,Point->getPoint()[1]
                                         ,Point->getPoint()[2]);
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
                ss << "Constraint" << *it + 1;
                accepted &=
                Gui::Selection().setPreselect(getSketchObject()->getDocument()->getName()
                                             ,getSketchObject()->getNameInDocument()
                                             ,ss.str().c_str()
                                             ,Point->getPoint()[0]
                                             ,Point->getPoint()[1]
                                             ,Point->getPoint()[2]);

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

void ViewProviderSketch::doBoxSelection(const SbVec2s &startPos, const SbVec2s &endPos,
                                        const Gui::View3DInventorViewer *viewer)
{
    std::vector<SbVec2s> corners0;
    corners0.push_back(startPos);
    corners0.push_back(endPos);
    std::vector<SbVec2f> corners = viewer->getGLPolygon(corners0);

    // all calculations with polygon and proj are in dimensionless [0 1] screen coordinates
    Base::Polygon2D polygon;
    polygon.Add(Base::Vector2D(corners[0].getValue()[0], corners[0].getValue()[1]));
    polygon.Add(Base::Vector2D(corners[0].getValue()[0], corners[1].getValue()[1]));
    polygon.Add(Base::Vector2D(corners[1].getValue()[0], corners[1].getValue()[1]));
    polygon.Add(Base::Vector2D(corners[1].getValue()[0], corners[0].getValue()[1]));

    Gui::ViewVolumeProjection proj(viewer->getSoRenderManager()->getCamera()->getViewVolume());

    Sketcher::SketchObject *sketchObject = getSketchObject();
    App::Document *doc = sketchObject->getDocument();

    Base::Placement Plm = sketchObject->Placement.getValue();

    int intGeoCount = sketchObject->getHighestCurveIndex() + 1;
    int extGeoCount = sketchObject->getExternalGeometryCount();

    const std::vector<Part::Geometry *> geomlist = sketchObject->getCompleteGeometry(); // without memory allocation
    assert(int(geomlist.size()) == extGeoCount + intGeoCount);
    assert(int(geomlist.size()) >= 2);

    Base::Vector3d pnt0, pnt1, pnt2, pnt;
    int VertexId = -1; // the loop below should be in sync with the main loop in ViewProviderSketch::draw
                       // so that the vertex indices are calculated correctly
    int GeoId = 0;
    for (std::vector<Part::Geometry *>::const_iterator it = geomlist.begin(); it != geomlist.end()-2; ++it, ++GeoId) {

        if (GeoId >= intGeoCount)
            GeoId = -extGeoCount;

        if ((*it)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            // ----- Check if single point lies inside box selection -----/
            const Part::GeomPoint *point = dynamic_cast<const Part::GeomPoint *>(*it);
            Plm.multVec(point->getPoint(), pnt0);
            pnt0 = proj(pnt0);
            VertexId += 1;

            if (polygon.Contains(Base::Vector2D(pnt0.x, pnt0.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

        } else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            // ----- Check if line segment lies inside box selection -----/
            const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(*it);
            Plm.multVec(lineSeg->getStartPoint(), pnt1);
            Plm.multVec(lineSeg->getEndPoint(), pnt2);
            pnt1 = proj(pnt1);
            pnt2 = proj(pnt2);
            VertexId += 2;

            bool pnt1Inside = polygon.Contains(Base::Vector2D(pnt1.x, pnt1.y));
            bool pnt2Inside = polygon.Contains(Base::Vector2D(pnt2.x, pnt2.y));
            if (pnt1Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            if (pnt2Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            if (pnt1Inside && pnt2Inside) {
                std::stringstream ss;
                ss << "Edge" << GeoId + 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

        } else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) { 
            // ----- Check if circle lies inside box selection -----/
            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(*it);
            pnt0 = circle->getCenter();
            VertexId += 1;

            Plm.multVec(pnt0, pnt0);
            pnt0 = proj(pnt0);

            if (polygon.Contains(Base::Vector2D(pnt0.x, pnt0.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());

                int countSegments = 12;
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
                    if (!polygon.Contains(Base::Vector2D(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        break;
                    }
                }

                if (bpolyInside) {
                    ss.clear();
                    ss.str("");
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(),ss.str().c_str());
                }
            }
        } else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) { 
            // ----- Check if circle lies inside box selection -----/
            const Part::GeomEllipse *ellipse = dynamic_cast<const Part::GeomEllipse *>(*it);
            pnt0 = ellipse->getCenter();
            VertexId += 1;

            Plm.multVec(pnt0, pnt0);
            pnt0 = proj(pnt0);

            if (polygon.Contains(Base::Vector2D(pnt0.x, pnt0.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());

                int countSegments = 12;
                float segment = float(2 * M_PI) / countSegments;

                // circumscribed polygon radius
                float a = float(ellipse->getMajorRadius()) / cos(segment/2);
                float b = float(ellipse->getMinorRadius()) / cos(segment/2);
                float phi = float(ellipse->getAngleXU());

                bool bpolyInside = true;
                pnt0 = ellipse->getCenter();
                float angle = 0.f;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(pnt0.x + a * cos(angle) * cos(phi) - b * sin(angle) * sin(phi),
                                         pnt0.y + a * cos(angle) * sin(phi) + b * sin(angle) * cos(phi),
                                         0.f);
                    Plm.multVec(pnt, pnt);
                    pnt = proj(pnt);
                    if (!polygon.Contains(Base::Vector2D(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        break;
                    }
                }

                if (bpolyInside) {
                    ss.clear();
                    ss.str("");
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(),ss.str().c_str());
                }
            }

        } else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfCircle *aoc = dynamic_cast<const Part::GeomArcOfCircle *>(*it);

            pnt0 = aoc->getStartPoint();
            pnt1 = aoc->getEndPoint();
            pnt2 = aoc->getCenter();
            VertexId += 3;

            Plm.multVec(pnt0, pnt0);
            Plm.multVec(pnt1, pnt1);
            Plm.multVec(pnt2, pnt2);
            pnt0 = proj(pnt0);
            pnt1 = proj(pnt1);
            pnt2 = proj(pnt2);

            bool pnt0Inside = polygon.Contains(Base::Vector2D(pnt0.x, pnt0.y));
            if (pnt0Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId - 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            bool pnt1Inside = polygon.Contains(Base::Vector2D(pnt1.x, pnt1.y));
            if (pnt1Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            if (polygon.Contains(Base::Vector2D(pnt2.x, pnt2.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            if (pnt0Inside && pnt1Inside) {
                double startangle, endangle;
                aoc->getRange(startangle, endangle);
                if (startangle > endangle) // if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle-startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                float segment = float(range) / countSegments;

                // circumscribed polygon radius
                float radius = float(aoc->getRadius()) / cos(segment/2);

                bool bpolyInside = true;
                pnt0 = aoc->getCenter();
                float angle = float(startangle) + segment/2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(pnt0.x + radius * cos(angle),
                                         pnt0.y + radius * sin(angle),
                                         0.f);
                    Plm.multVec(pnt, pnt);
                    pnt = proj(pnt);
                    if (!polygon.Contains(Base::Vector2D(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        break;
                    }
                }

                if (bpolyInside) {
                    std::stringstream ss;
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
                }
            }

        } else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfEllipse *aoe = dynamic_cast<const Part::GeomArcOfEllipse *>(*it);

            pnt0 = aoe->getStartPoint();
            pnt1 = aoe->getEndPoint();
            pnt2 = aoe->getCenter();
            VertexId += 3;

            Plm.multVec(pnt0, pnt0);
            Plm.multVec(pnt1, pnt1);
            Plm.multVec(pnt2, pnt2);
            pnt0 = proj(pnt0);
            pnt1 = proj(pnt1);
            pnt2 = proj(pnt2);

            bool pnt0Inside = polygon.Contains(Base::Vector2D(pnt0.x, pnt0.y));
            if (pnt0Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId - 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            bool pnt1Inside = polygon.Contains(Base::Vector2D(pnt1.x, pnt1.y));
            if (pnt1Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            if (polygon.Contains(Base::Vector2D(pnt2.x, pnt2.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            if (pnt0Inside && pnt1Inside) {
                double startangle, endangle;
                aoe->getRange(startangle, endangle);
                if (startangle > endangle) // if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle-startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                float segment = float(range) / countSegments;

                                                // circumscribed polygon radius
                float a = float(aoe->getMajorRadius()) / cos(segment/2);
                float b = float(aoe->getMinorRadius()) / cos(segment/2);
                float phi = float(aoe->getAngleXU());
 
                bool bpolyInside = true;
                pnt0 = aoe->getCenter();
                float angle = float(startangle) + segment/2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(pnt0.x + a * cos(angle) * cos(phi) - b * sin(angle) * sin(phi),
                                         pnt0.y + a * cos(angle) * sin(phi) + b * sin(angle) * cos(phi),
                                         0.f);
                    Plm.multVec(pnt, pnt);
                    pnt = proj(pnt);
                    if (!polygon.Contains(Base::Vector2D(pnt.x, pnt.y))) {
                        bpolyInside = false;
                        break;
                    }
                }

                if (bpolyInside) {
                    std::stringstream ss;
                    ss << "Edge" << GeoId + 1;
                    Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
                }
            }

        } else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            const Part::GeomBSplineCurve *spline = dynamic_cast<const Part::GeomBSplineCurve *>(*it);
            std::vector<Base::Vector3d> poles = spline->getPoles();
            VertexId += poles.size();
            // TODO
        }
    }

    pnt0 = proj(Plm.getPosition());
    if (polygon.Contains(Base::Vector2D(pnt0.x, pnt0.y)))
        Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), "RootPoint");
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
    
    // colors of the point set
    if (edit->FullyConstrained) {
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = FullyConstrainedColor;
    }
    else {
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = VertexColor;
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
    
    float x,y,z;
    
    int j=0; // vertexindex
    
    for (int  i=0; i < CurvNum; i++) {
        int GeoId = edit->CurvIdToGeoId[i];
        // CurvId has several vertex a ssociated to 1 material
        //edit->CurveSet->numVertices => [i] indicates number of vertex for line i.
        int indexes = (edit->CurveSet->numVertices[i]);

        bool selected = (edit->SelCurvSet.find(GeoId) != edit->SelCurvSet.end());
        bool preselected = (edit->PreselectCurve == GeoId);

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
        else if (GeoId < -2) {  // external Geometry
            color[i] = CurveExternalColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zConstr);
            }
        }
        else if (getSketchObject()->getGeometry(GeoId)->Construction) {
            color[i] = CurveDraftColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zLines);
            }
        }
        else if (edit->FullyConstrained) {
            color[i] = FullyConstrainedColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zLines);
            }
        }
        else {
            color[i] = CurveColor;
            for (int k=j; j<k+indexes; j++) {
                verts[j].getValue(x,y,z);
                verts[j] = SbVec3f(x,y,zLines);
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

    if (edit->SelCurvSet.find(-2) != edit->SelCurvSet.end())
        crosscolor[1] = SelectColor;
    else if (edit->PreselectCross == 2)
        crosscolor[1] = PreselectColor;
    else
        crosscolor[1] = CrossColorV;

    // colors of the constraints
    for (int i=0; i < edit->constrGroup->getNumChildren(); i++) {
        SoSeparator *s = dynamic_cast<SoSeparator *>(edit->constrGroup->getChild(i));

        // Check Constraint Type
        Sketcher::Constraint* constraint = getSketchObject()->Constraints.getValues()[i];
        ConstraintType type = constraint->Type;
        bool hasDatumLabel  = (type == Sketcher::Angle ||
                               type == Sketcher::Radius ||
                               type == Sketcher::Symmetric ||
                               type == Sketcher::Distance ||
                               type == Sketcher::DistanceX ||
                               type == Sketcher::DistanceY);

        // Non DatumLabel Nodes will have a material excluding coincident
        bool hasMaterial = false;

        SoMaterial *m;
        if (!hasDatumLabel && type != Sketcher::Coincident && type !=InternalAlignment) {
            hasMaterial = true;
            m = dynamic_cast<SoMaterial *>(s->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
        }

        if (edit->SelConstraintSet.find(i) != edit->SelConstraintSet.end()) {
            if (hasDatumLabel) {
                SoDatumLabel *l = dynamic_cast<SoDatumLabel *>(s->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                l->textColor = SelectColor;
            } else if (hasMaterial) {
                m->diffuseColor = SelectColor;
            } else if (type == Sketcher::Coincident) {
                int index;
                index = edit->ActSketch.getPointId(constraint->First, constraint->FirstPos) + 1;
                if (index >= 0 && index < PtNum) pcolor[index] = SelectColor;
                index = edit->ActSketch.getPointId(constraint->Second, constraint->SecondPos) + 1;
                if (index >= 0 && index < PtNum) pcolor[index] = SelectColor;
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
                                int indexes=(edit->CurveSet->numVertices[i]);
                                color[i] = SelectColor;
                                break;
                            }
                        }
                    }
                    break;
                    case EllipseFocus1:
                    case EllipseFocus2:
                    {
                        int index = edit->ActSketch.getPointId(constraint->First, constraint->FirstPos) + 1;
                        if (index >= 0 && index < PtNum) pcolor[index] = SelectColor;
                    }
                    break;
                    default:
                    break;
                }
            }
        } else if (edit->PreselectConstraintSet.count(i)) {
            if (hasDatumLabel) {
                SoDatumLabel *l = dynamic_cast<SoDatumLabel *>(s->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                l->textColor = PreselectColor;
            } else if (hasMaterial) {
                m->diffuseColor = PreselectColor;
            }
        }
        else {
            if (hasDatumLabel) {
                SoDatumLabel *l = dynamic_cast<SoDatumLabel *>(s->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                l->textColor = ConstrDimColor;
            } else if (hasMaterial) {
                m->diffuseColor = ConstrDimColor;
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

QString ViewProviderSketch::iconTypeFromConstraint(Constraint *constraint)
{
    /*! TODO: Consider pushing this functionality up into Constraint */
    switch(constraint->Type) {
    case Horizontal:
        return QString::fromAscii("small/Constraint_Horizontal_sm");
    case Vertical:
        return QString::fromAscii("small/Constraint_Vertical_sm");
    case PointOnObject:
        return QString::fromAscii("small/Constraint_PointOnObject_sm");
    case Tangent:
        return QString::fromAscii("small/Constraint_Tangent_sm");
    case Parallel:
        return QString::fromAscii("small/Constraint_Parallel_sm");
    case Perpendicular:
        return QString::fromAscii("small/Constraint_Perpendicular_sm");
    case Equal:
        return QString::fromAscii("small/Constraint_EqualLength_sm");
    case Symmetric:
        return QString::fromAscii("small/Constraint_Symmetric_sm");
    case SnellsLaw:
        return QString::fromAscii("small/Constraint_SnellsLaw_sm");
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
    static QColor constrIconSelColor ((int)(SelectColor[0] * 255.0f),
                                      (int)(SelectColor[1] * 255.0f),
                                      (int)(SelectColor[2] * 255.0f));
    static QColor constrIconPreselColor ((int)(PreselectColor[0] * 255.0f),
                                         (int)(PreselectColor[1] * 255.0f),
                                         (int)(PreselectColor[2] * 255.0f));

    if (edit->PreselectConstraintSet.count(constraintId))
        return constrIconPreselColor;
    else if (edit->SelConstraintSet.find(constraintId) != edit->SelConstraintSet.end())
        return constrIconSelColor;
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
                if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                    geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                    multipleIcons = true;
                }
            }
            break;
        case Parallel:
            multipleIcons = true;
            break;
        case Perpendicular:
            // second icon is available only when there is no common point
            if ((*it)->FirstPos == Sketcher::none && (*it)->Third == Constraint::GeoUndef)
                multipleIcons = true;
            break;
        case Equal:
            multipleIcons = true;
            break;
        default:
            break;
        }

        // Find the Constraint Icon SoImage Node
        SoSeparator *sep = static_cast<SoSeparator *>(edit->constrGroup->getChild(constrId));

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

        if ((*it)->Type==Symmetric) {
            Base::Vector3d startingpoint = getSketchObject()->getPoint((*it)->First,(*it)->FirstPos);
            Base::Vector3d endpoint = getSketchObject()->getPoint((*it)->Second,(*it)->SecondPos);

            double x0,y0,x1,y1;
            SbVec3f pos0(startingpoint.x,startingpoint.y,startingpoint.z);
            SbVec3f pos1(endpoint.x,endpoint.y,endpoint.z);
            
            Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
            Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();
            SoCamera* pCam = viewer->getSoRenderManager()->getCamera();
            if (!pCam) return;

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
                thisIcon.label = QString::fromAscii((*it)->Name.c_str());
            iconQueue.push_back(thisIcon);

            // Note that the second translation is meant to be applied after the first.
            // So, to get the position of the second icon, we add the two translations together
            //
            // See note ~30 lines up.
            translationPtr = static_cast<SoTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION));
            if(dynamic_cast<SoZoomTranslation *>(translationPtr))
                thisIcon.position += static_cast<SoZoomTranslation *>(translationPtr)->abPos.getValue();
            else
                thisIcon.position += translationPtr->translation.getValue();

            thisIcon.destination = dynamic_cast<SoImage *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_ICON));
            thisIcon.infoPtr = static_cast<SoInfo *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_CONSTRAINTID));
        }
        else
            if ((*it)->Name.empty())
                thisIcon.label = QString();
            else
                thisIcon.label = QString::fromAscii((*it)->Name.c_str());

        iconQueue.push_back(thisIcon);
    }

    combineConstraintIcons(iconQueue);
}

void ViewProviderSketch::combineConstraintIcons(IconQueue iconQueue)
{
    // getScaleFactor gives us a ratio of pixels per some kind of real units
    // (Translation: this number is somewhat made-up.)
    float maxDistSquared = pow(0.05 * getScaleFactor(), 2);

    // There's room for optimisation here; we could reuse the combined icons...
    edit->combinedConstrBoxes.clear();

    while(!iconQueue.empty()) {
        // A group starts with an item popped off the back of our initial queue
        IconQueue thisGroup;
        thisGroup.push_back(iconQueue.back());
        ViewProviderSketch::constrIconQueueItem init = iconQueue.back();
        iconQueue.pop_back();
        
        // we group only icons not being Symmetry icons, because we want those on the line
        if(init.type != QString::fromLatin1("small/Constraint_Symmetric_sm")){
            
            IconQueue::iterator i = iconQueue.begin();
            while(i != iconQueue.end()) {
                bool addedToGroup = false;
                
                for(IconQueue::iterator j = thisGroup.begin();
                    j != thisGroup.end(); ++j)
                    if(i->position.equals(j->position, maxDistSquared) && (*i).type != QString::fromLatin1("small/Constraint_Symmetric_sm")) {
                        // Found an icon in iconQueue that's close enough to
                        // a member of thisGroup, so move it into thisGroup
                        thisGroup.push_back(*i);
                        i = iconQueue.erase(i);
                        addedToGroup = true;
                        break;
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
    SbVec3f avPos(0, 0, 0);
    for(IconQueue::iterator i = iconQueue.begin(); i != iconQueue.end(); ++i) {
        clearCoinImage(i->destination);
        avPos = avPos + i->position;
    }
    avPos = avPos/iconQueue.size();

    QImage compositeIcon;
    float closest = FLT_MAX;  // Closest distance between avPos and any icon
    SoImage *thisDest;
    SoInfo *thisInfo;

    // Tracks all constraint IDs that are combined into this icon
    QString idString;
    int lastVPad;

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
            idString.append(QString::fromAscii(","));
        idString.append(QString::number(i->constraintId));

        if((avPos - i->position).length() < closest) {
            thisDest = i->destination;
            thisInfo = i->infoPtr;
            closest = (avPos - i->position).length();
        }

        i = iconQueue.erase(i);
        while(i != iconQueue.end()) {
            if(i->type != thisType) {
                ++i;
                continue;
            }

            if((avPos - i->position).length() < closest) {
                thisDest = i->destination;
                thisInfo = i->infoPtr;
                closest = (avPos - i->position).length();
            }

            labels.append(i->label);
            ids.push_back(i->constraintId);
            labelColors.append(constrColor(i->constraintId));

            if(constrColorPriority(i->constraintId) > maxColorPriority) {
                maxColorPriority = constrColorPriority(i->constraintId);
                iconColor= constrColor(i->constraintId);
            }

            idString.append(QString::fromAscii(",") +
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
                for(std::vector<int>::iterator j = ids.begin();
                    j != ids.end(); ++j)
                    nextIds.insert(*j);
            } else
                nextIds.insert(*(id++));

            ConstrIconBB newBB(bb->adjusted(0, oldHeight, 0, oldHeight),
                               nextIds);

            boundingBoxes.push_back(newBB);
        }
    }

    edit->combinedConstrBoxes[idString] = boundingBoxes;
    thisInfo->string.setValue(idString.toAscii().data());
    sendConstraintIconToCoin(compositeIcon, thisDest);
}


/// Note: labels, labelColors, and boundignBoxes are all
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
    QString joinStr = QString::fromAscii(", ");

    QImage icon = Gui::BitmapFactory().pixmap(type.toAscii()).toImage();

    QFont font = QApplication::font();
    font.setPixelSize(11);
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

            cursorOffset += qfm.width(labelStr);
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

    i.infoPtr->string.setValue(QString::number(i.constraintId).toAscii().data());
    sendConstraintIconToCoin(image, i.destination);
}

float ViewProviderSketch::getScaleFactor()
{
    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    if (mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();
        return viewer->getSoRenderManager()->getCamera()->getViewVolume(viewer->getSoRenderManager()->getCamera()->aspectRatio.getValue()).getWorldToScreenScale(SbVec3f(0.f, 0.f, 0.f), 0.1f) / 3;
    } else {
        return 1.f;
    }
}

void ViewProviderSketch::draw(bool temp)
{
    assert(edit);

    // Get Bounding box dimensions for Datum text
    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();

    // Render Geometry ===================================================
    std::vector<Base::Vector3d> Coords;
    std::vector<Base::Vector3d> Points;
    std::vector<unsigned int> Index;

    int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;
    int extGeoCount = getSketchObject()->getExternalGeometryCount();

    const std::vector<Part::Geometry *> *geomlist;
    std::vector<Part::Geometry *> tempGeo;
    if (temp)
        tempGeo = edit->ActSketch.extractGeometry(true, true); // with memory allocation
    else
        tempGeo = getSketchObject()->getCompleteGeometry(); // without memory allocation
    geomlist = &tempGeo;


    assert(int(geomlist->size()) == extGeoCount + intGeoCount);
    assert(int(geomlist->size()) >= 2);

    edit->CurvIdToGeoId.clear();
    int GeoId = 0;

    // RootPoint
    Points.push_back(Base::Vector3d(0.,0.,0.));

    for (std::vector<Part::Geometry *>::const_iterator it = geomlist->begin(); it != geomlist->end()-2; ++it, GeoId++) {
        if (GeoId >= intGeoCount)
            GeoId = -extGeoCount;
        if ((*it)->getTypeId() == Part::GeomPoint::getClassTypeId()) { // add a point
            const Part::GeomPoint *point = dynamic_cast<const Part::GeomPoint *>(*it);
            Points.push_back(point->getPoint());
        }
        else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) { // add a line
            const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(*it);
            // create the definition struct for that geom
            Coords.push_back(lineSeg->getStartPoint());
            Coords.push_back(lineSeg->getEndPoint());
            Points.push_back(lineSeg->getStartPoint());
            Points.push_back(lineSeg->getEndPoint());
            Index.push_back(2);
            edit->CurvIdToGeoId.push_back(GeoId);
        }
        else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) { // add a circle
            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(*it);
            Handle_Geom_Circle curve = Handle_Geom_Circle::DownCast(circle->handle());

            int countSegments = 50;
            Base::Vector3d center = circle->getCenter();
            double segment = (2 * M_PI) / countSegments;
            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(i*segment);
                Coords.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
            }

            gp_Pnt pnt = curve->Value(0);
            Coords.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(center);
        }
        else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) { // add an ellipse
            const Part::GeomEllipse *ellipse = dynamic_cast<const Part::GeomEllipse *>(*it);
            Handle_Geom_Ellipse curve = Handle_Geom_Ellipse::DownCast(ellipse->handle());

            int countSegments = 50;
            Base::Vector3d center = ellipse->getCenter();
            double segment = (2 * M_PI) / countSegments;
            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(i*segment);
                Coords.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
            }

            gp_Pnt pnt = curve->Value(0);
            Coords.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(center);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) { // add an arc
            const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(*it);
            Handle_Geom_TrimmedCurve curve = Handle_Geom_TrimmedCurve::DownCast(arc->handle());

            double startangle, endangle;
            arc->getRange(startangle, endangle);
            if (startangle > endangle) // if arc is reversed
                std::swap(startangle, endangle);

            double range = endangle-startangle;
            int countSegments = std::max(6, int(50.0 * range / (2 * M_PI)));
            double segment = range / countSegments;

            Base::Vector3d center = arc->getCenter();
            Base::Vector3d start  = arc->getStartPoint();
            Base::Vector3d end    = arc->getEndPoint();

            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(startangle);
                Coords.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
                startangle += segment;
            }

            // end point
            gp_Pnt pnt = curve->Value(endangle);
            Coords.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(start);
            Points.push_back(end);
            Points.push_back(center);
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) { // add an arc
            const Part::GeomArcOfEllipse *arc = dynamic_cast<const Part::GeomArcOfEllipse *>(*it);
            Handle_Geom_TrimmedCurve curve = Handle_Geom_TrimmedCurve::DownCast(arc->handle());

            double startangle, endangle;
            arc->getRange(startangle, endangle);
            if (startangle > endangle) // if arc is reversed
                std::swap(startangle, endangle);

            double range = endangle-startangle;
            int countSegments = std::max(6, int(50.0 * range / (2 * M_PI)));
            double segment = range / countSegments;

            Base::Vector3d center = arc->getCenter();
            Base::Vector3d start  = arc->getStartPoint();
            Base::Vector3d end    = arc->getEndPoint();

            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(startangle);
                Coords.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
                startangle += segment;
            }

            // end point
            gp_Pnt pnt = curve->Value(endangle);
            Coords.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
            Points.push_back(start);
            Points.push_back(end);
            Points.push_back(center);
        }
        else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) { // add a bspline
            const Part::GeomBSplineCurve *spline = dynamic_cast<const Part::GeomBSplineCurve *>(*it);
            Handle_Geom_BSplineCurve curve = Handle_Geom_BSplineCurve::DownCast(spline->handle());

            double first = curve->FirstParameter();
            double last = curve->LastParameter();
            if (first > last) // if arc is reversed
                std::swap(first, last);

            double range = last-first;
            int countSegments = 50;
            double segment = range / countSegments;

            for (int i=0; i < countSegments; i++) {
                gp_Pnt pnt = curve->Value(first);
                Coords.push_back(Base::Vector3d(pnt.X(), pnt.Y(), pnt.Z()));
                first += segment;
            }

            // end point
            gp_Pnt end = curve->Value(last);
            Coords.push_back(Base::Vector3d(end.X(), end.Y(), end.Z()));

            std::vector<Base::Vector3d> poles = spline->getPoles();
            for (std::vector<Base::Vector3d>::iterator it = poles.begin(); it != poles.end(); ++it) {
                Points.push_back(*it);
            }

            Index.push_back(countSegments+1);
            edit->CurvIdToGeoId.push_back(GeoId);
        }
        else {
        }
    }

    edit->CurvesCoordinate->point.setNum(Coords.size());
    edit->CurveSet->numVertices.setNum(Index.size());
    edit->CurvesMaterials->diffuseColor.setNum(Index.size());
    edit->PointsCoordinate->point.setNum(Points.size());
    edit->PointsMaterials->diffuseColor.setNum(Points.size());
    
    SbVec3f *verts = edit->CurvesCoordinate->point.startEditing();
    int32_t *index = edit->CurveSet->numVertices.startEditing();
    SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();

    int i=0; // setting up the line set
    for (std::vector<Base::Vector3d>::const_iterator it = Coords.begin(); it != Coords.end(); ++it,i++)
      verts[i].setValue(it->x,it->y,zLines);
    
    i=0; // setting up the indexes of the line set
    for (std::vector<unsigned int>::const_iterator it = Index.begin(); it != Index.end(); ++it,i++)
        index[i] = *it;

    i=0; // setting up the point set
    for (std::vector<Base::Vector3d>::const_iterator it = Points.begin(); it != Points.end(); ++it,i++)
        pverts[i].setValue(it->x,it->y,zPoints);

    edit->CurvesCoordinate->point.finishEditing();
    edit->CurveSet->numVertices.finishEditing();
    edit->PointsCoordinate->point.finishEditing();

    // set cross coordinates
    edit->RootCrossSet->numVertices.set1Value(0,2);
    edit->RootCrossSet->numVertices.set1Value(1,2);

    float MiX = -exp(ceil(log(std::abs(MinX))));
    MiX = std::min(MiX,(float)-exp(ceil(log(std::abs(0.1f*MaxX)))));
    float MaX = exp(ceil(log(std::abs(MaxX))));
    MaX = std::max(MaX,(float)exp(ceil(log(std::abs(0.1f*MinX)))));
    float MiY = -exp(ceil(log(std::abs(MinY))));
    MiY = std::min(MiY,(float)-exp(ceil(log(std::abs(0.1f*MaxY)))));
    float MaY = exp(ceil(log(std::abs(MaxY))));
    MaY = std::max(MaY,(float)exp(ceil(log(std::abs(0.1f*MinY)))));

    edit->RootCrossCoordinate->point.set1Value(0,SbVec3f(MiX, 0.0f, zCross));
    edit->RootCrossCoordinate->point.set1Value(1,SbVec3f(MaX, 0.0f, zCross));
    edit->RootCrossCoordinate->point.set1Value(2,SbVec3f(0.0f, MiY, zCross));
    edit->RootCrossCoordinate->point.set1Value(3,SbVec3f(0.0f, MaY, zCross));

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
    // go through the constraints and update the position
    i = 0;
    for (std::vector<Sketcher::Constraint *>::const_iterator it=constrlist.begin();
         it != constrlist.end(); ++it, i++) {
        // check if the type has changed
        if ((*it)->Type != edit->vConstrType[i]) {
            // clearing the type vector will force a rebuild of the visual nodes
            edit->vConstrType.clear();
            goto Restart;
        }
        // root separator for this constraint
        SoSeparator *sep = dynamic_cast<SoSeparator *>(edit->constrGroup->getChild(i));
        const Constraint *Constr = *it;
        
        bool major_radius = false; // this is checked in the radius to reuse code
        // distinquish different constraint types to build up
        switch (Constr->Type) {
            case Horizontal: // write the new position of the Horizontal constraint Same as vertical position.
            case Vertical: // write the new position of the Vertical constraint
                {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    // get the geometry
                    const Part::Geometry *geo = GeoById(*geomlist, Constr->First);
                    // Vertical can only be a GeomLineSegment
                    assert(geo->getTypeId() == Part::GeomLineSegment::getClassTypeId());
                    const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo);

                    // calculate the half distance between the start and endpoint
                    Base::Vector3d midpos = ((lineSeg->getEndPoint()+lineSeg->getStartPoint())/2);

                    //Get a set of vectors perpendicular and tangential to these
                    Base::Vector3d dir = (lineSeg->getEndPoint()-lineSeg->getStartPoint()).Normalize();
                    Base::Vector3d norm(-dir.y,dir.x,0);

                    Base::Vector3d relpos = seekConstraintPosition(midpos, norm, dir, 2.5, edit->constrGroup->getChild(i));

                    dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(midpos.x, midpos.y, zConstr); //Absolute Reference

                    //Reference Position that is scaled according to zoom
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relpos.x, relpos.y, 0);

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


                    if (Constr->Third != Constraint::GeoUndef || //perpty via point
                            Constr->FirstPos != Sketcher::none) { //endpoint-to-curve or endpoint-to-endpoint perpty

                        int ptGeoId;
                        Sketcher::PointPos ptPosId;
                        do {//dummy loop to use break =) Maybe goto?
                            ptGeoId = Constr->First;
                            ptPosId = Constr->FirstPos;
                            if (ptPosId != Sketcher::none) break;
                            ptGeoId = Constr->Second;
                            ptPosId = Constr->SecondPos;
                            if (ptPosId != Sketcher::none) break;
                            ptGeoId = Constr->Third;
                            ptPosId = Constr->ThirdPos;
                            if (ptPosId != Sketcher::none) break;
                            assert(0);//no point found!
                        } while (false);
                        if (temp)
                            midpos1 = edit->ActSketch.getPoint(ptGeoId, ptPosId);
                        else
                            midpos1 = getSketchObject()->getPoint(ptGeoId, ptPosId);

                        norm1 = edit->ActSketch.calculateNormalAtPoint(Constr->Second, midpos1.x, midpos1.y);
                        norm1.Normalize();
                        dir1 = norm1; dir1.RotateZ(-M_PI/2.0);

                    } else if (Constr->FirstPos == Sketcher::none) {

                        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment *>(geo1);
                            midpos1 = ((lineSeg1->getEndPoint()+lineSeg1->getStartPoint())/2);
                            dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint()).Normalize();
                            norm1 = Base::Vector3d(-dir1.y,dir1.x,0.);
                        } else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo1);
                            double startangle, endangle, midangle;
                            arc->getRange(startangle, endangle);
                            midangle = (startangle + endangle)/2;
                            norm1 = Base::Vector3d(cos(midangle),sin(midangle),0);
                            dir1 = Base::Vector3d(-norm1.y,norm1.x,0);
                            midpos1 = arc->getCenter() + arc->getRadius() * norm1;
                        } else if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo1);
                            norm1 = Base::Vector3d(cos(M_PI/4),sin(M_PI/4),0);
                            dir1 = Base::Vector3d(-norm1.y,norm1.x,0);
                            midpos1 = circle->getCenter() + circle->getRadius() * norm1;
                        } else
                            break;

                        if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment *>(geo2);
                            midpos2 = ((lineSeg2->getEndPoint()+lineSeg2->getStartPoint())/2);
                            dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint()).Normalize();
                            norm2 = Base::Vector3d(-dir2.y,dir2.x,0.);
                        } else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo2);
                            double startangle, endangle, midangle;
                            arc->getRange(startangle, endangle);
                            midangle = (startangle + endangle)/2;
                            norm2 = Base::Vector3d(cos(midangle),sin(midangle),0);
                            dir2 = Base::Vector3d(-norm2.y,norm2.x,0);
                            midpos2 = arc->getCenter() + arc->getRadius() * norm2;
                        } else if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo2);
                            norm2 = Base::Vector3d(cos(M_PI/4),sin(M_PI/4),0);
                            dir2 = Base::Vector3d(-norm2.y,norm2.x,0);
                            midpos2 = circle->getCenter() + circle->getRadius() * norm2;
                        } else
                            break;
                        twoIcons = true;

                    }

                    Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 2.5, edit->constrGroup->getChild(i));
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr);
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                    if (twoIcons) {
                        Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 2.5, edit->constrGroup->getChild(i));

                        Base::Vector3d secondPos = midpos2 - midpos1;
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr);
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);
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
                            double r1a,r1b,r2a,r2b;
                            double angle1,angle1plus=0.,  angle2, angle2plus=0.;//angle1 = rotation of object as a whole; angle1plus = arc angle (t parameter for ellipses).
                            if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) { 
                                const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo1);
                                r1a = circle->getRadius();
                                angle1 = M_PI/4;
                                midpos1 = circle->getCenter();
                            } else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo1);
                                r1a = arc->getRadius();
                                double startangle, endangle;
                                arc->getRange(startangle, endangle);
                                angle1 = (startangle + endangle)/2;
                                midpos1 = arc->getCenter();
                            } else if (geo1->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                const Part::GeomEllipse *ellipse = dynamic_cast<const Part::GeomEllipse *>(geo1);
                                r1a = ellipse->getMajorRadius();
                                r1b = ellipse->getMinorRadius();
                                angle1 = ellipse->getAngleXU();
                                angle1plus = M_PI/4;
                                midpos1 = ellipse->getCenter();
                            } else if (geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                                const Part::GeomArcOfEllipse *aoe = dynamic_cast<const Part::GeomArcOfEllipse *>(geo1);
                                r1a = aoe->getMajorRadius();
                                r1b = aoe->getMinorRadius();
                                double startangle, endangle;
                                aoe->getRange(startangle, endangle);
                                angle1 = aoe->getAngleXU();
                                angle1plus = (startangle + endangle)/2;
                                midpos1 = aoe->getCenter();
                            } else
                                break;

                            if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo2);
                                r2a = circle->getRadius();
                                angle2 = M_PI/4;
                                midpos2 = circle->getCenter();
                            } else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo2);
                                r2a = arc->getRadius();
                                double startangle, endangle;
                                arc->getRange(startangle, endangle);
                                angle2 = (startangle + endangle)/2;
                                midpos2 = arc->getCenter();
                            } else if (geo2->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
                                const Part::GeomEllipse *ellipse = dynamic_cast<const Part::GeomEllipse *>(geo2);
                                r2a = ellipse->getMajorRadius();
                                r2b = ellipse->getMinorRadius();
                                angle2 = ellipse->getAngleXU();
                                angle2plus = M_PI/4;
                                midpos2 = ellipse->getCenter();
                            } else if (geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
                                const Part::GeomArcOfEllipse *aoe = dynamic_cast<const Part::GeomArcOfEllipse *>(geo2);
                                r2a = aoe->getMajorRadius();
                                r2b = aoe->getMinorRadius();
                                double startangle, endangle;
                                aoe->getRange(startangle, endangle);
                                angle2 = aoe->getAngleXU();
                                angle2plus = (startangle + endangle)/2;
                                midpos2 = aoe->getCenter();
                            } else
                                break;

                            if( geo1->getTypeId() == Part::GeomEllipse::getClassTypeId() || 
                                geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ){
                                
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
                                geo2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ) {                            
                                
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
                        const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment *>(geo1);
                        const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment *>(geo2);

                        // calculate the half distance between the start and endpoint
                        midpos1 = ((lineSeg1->getEndPoint()+lineSeg1->getStartPoint())/2);
                        midpos2 = ((lineSeg2->getEndPoint()+lineSeg2->getStartPoint())/2);
                        //Get a set of vectors perpendicular and tangential to these
                        dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint()).Normalize();
                        dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint()).Normalize();
                        norm1 = Base::Vector3d(-dir1.y,dir1.x,0.);
                        norm2 = Base::Vector3d(-dir2.y,dir2.x,0.);
                    }

                    Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 2.5, edit->constrGroup->getChild(i));
                    Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 2.5, edit->constrGroup->getChild(i));

                    dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr); //Absolute Reference

                    //Reference Position that is scaled according to zoom
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                    Base::Vector3d secondPos = midpos2 - midpos1;
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr); //Absolute Reference

                    //Reference Position that is scaled according to zoom
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->translation = SbVec3f(relpos2.x - relpos1.x, relpos2.y -relpos1.y, 0);

                }
                break;
            case Distance:
            case DistanceX:
            case DistanceY:
                {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                    Base::Vector3d pnt1(0.,0.,0.), pnt2(0.,0.,0.);
                    if (Constr->SecondPos != Sketcher::none) { // point to point distance
                        if (temp) {
                            pnt1 = edit->ActSketch.getPoint(Constr->First, Constr->FirstPos);
                            pnt2 = edit->ActSketch.getPoint(Constr->Second, Constr->SecondPos);
                        } else {
                            pnt1 = getSketchObject()->getPoint(Constr->First, Constr->FirstPos);
                            pnt2 = getSketchObject()->getPoint(Constr->Second, Constr->SecondPos);
                        }
                    } else if (Constr->Second != Constraint::GeoUndef) { // point to line distance
                        if (temp) {
                            pnt1 = edit->ActSketch.getPoint(Constr->First, Constr->FirstPos);
                        } else {
                            pnt1 = getSketchObject()->getPoint(Constr->First, Constr->FirstPos);
                        }
                        const Part::Geometry *geo = GeoById(*geomlist, Constr->Second);
                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo);
                            Base::Vector3d l2p1 = lineSeg->getStartPoint();
                            Base::Vector3d l2p2 = lineSeg->getEndPoint();
                            // calculate the projection of p1 onto line2
                            pnt2.ProjToLine(pnt1-l2p1, l2p2-l2p1);
                            pnt2 += pnt1;
                        } else
                            break;
                    } else if (Constr->FirstPos != Sketcher::none) {
                        if (temp) {
                            pnt2 = edit->ActSketch.getPoint(Constr->First, Constr->FirstPos);
                        } else {
                            pnt2 = getSketchObject()->getPoint(Constr->First, Constr->FirstPos);
                        }
                    } else if (Constr->First != Constraint::GeoUndef) {
                        const Part::Geometry *geo = GeoById(*geomlist, Constr->First);
                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo);
                            pnt1 = lineSeg->getStartPoint();
                            pnt2 = lineSeg->getEndPoint();
                        } else
                            break;
                    } else
                        break;

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                    if ((Constr->Type == DistanceX || Constr->Type == DistanceY) &&
                        Constr->FirstPos != Sketcher::none && Constr->Second == Constraint::GeoUndef)
                        // display negative sign for absolute coordinates
                        asciiText->string = SbString(Base::Quantity(Constr->Value,Base::Unit::Length).getUserString().toUtf8().constData());
                    else // hide negative sign
                        asciiText->string = SbString(Base::Quantity(std::abs(Constr->Value),Base::Unit::Length).getUserString().toUtf8().constData());

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
                          (Constr->Type == Tangent && Constr->Third != Constraint::GeoUndef) || //Tangency via point
                          (Constr->Type == Tangent && Constr->FirstPos != Sketcher::none) //endpoint-to-curve or endpoint-to-endpoint tangency
                            ) {

                        //find the point of tangency/point that is on object
                        //just any point among first/second/third should be OK
                        int ptGeoId;
                        Sketcher::PointPos ptPosId;
                        do {//dummy loop to use break =) Maybe goto?
                            ptGeoId = Constr->First;
                            ptPosId = Constr->FirstPos;
                            if (ptPosId != Sketcher::none) break;
                            ptGeoId = Constr->Second;
                            ptPosId = Constr->SecondPos;
                            if (ptPosId != Sketcher::none) break;
                            ptGeoId = Constr->Third;
                            ptPosId = Constr->ThirdPos;
                            if (ptPosId != Sketcher::none) break;
                            assert(0);//no point found!
                        } while (false);
                        pos = edit->ActSketch.getPoint(ptGeoId, ptPosId);

                        Base::Vector3d norm = edit->ActSketch.calculateNormalAtPoint(Constr->Second, pos.x, pos.y);
                        norm.Normalize();
                        Base::Vector3d dir = norm; dir.RotateZ(-M_PI/2.0);

                        relPos = seekConstraintPosition(pos, norm, dir, 2.5, edit->constrGroup->getChild(i));
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(pos.x, pos.y, zConstr); //Absolute Reference
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relPos.x, relPos.y, 0);
                    }
                    else if (Constr->Type == Tangent) {
                        // get the geometry
                        const Part::Geometry *geo1 = GeoById(*geomlist, Constr->First);
                        const Part::Geometry *geo2 = GeoById(*geomlist, Constr->Second);

                        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                            geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment *>(geo1);
                            const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment *>(geo2);
                            // tangency between two lines
                            Base::Vector3d midpos1 = ((lineSeg1->getEndPoint()+lineSeg1->getStartPoint())/2);
                            Base::Vector3d midpos2 = ((lineSeg2->getEndPoint()+lineSeg2->getStartPoint())/2);
                            Base::Vector3d dir1 = (lineSeg1->getEndPoint()-lineSeg1->getStartPoint()).Normalize();
                            Base::Vector3d dir2 = (lineSeg2->getEndPoint()-lineSeg2->getStartPoint()).Normalize();
                            Base::Vector3d norm1 = Base::Vector3d(-dir1.y,dir1.x,0.f);
                            Base::Vector3d norm2 = Base::Vector3d(-dir2.y,dir2.x,0.f);

                            Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 2.5, edit->constrGroup->getChild(i));
                            Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 2.5, edit->constrGroup->getChild(i));

                            dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr); //Absolute Reference

                            //Reference Position that is scaled according to zoom
                            dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                            Base::Vector3d secondPos = midpos2 - midpos1;
                            dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr); //Absolute Reference

                            //Reference Position that is scaled according to zoom
                            dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_SECOND_TRANSLATION))->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);

                            break;
                        }
                        else if (geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            std::swap(geo1,geo2);
                        }

                        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo1);
                            Base::Vector3d dir = (lineSeg->getEndPoint() - lineSeg->getStartPoint()).Normalize();
                            Base::Vector3d norm(-dir.y, dir.x, 0);
                            if (geo2->getTypeId()== Part::GeomCircle::getClassTypeId()) { 
                                const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo2);
                                // tangency between a line and a circle
                                float length = (circle->getCenter() - lineSeg->getStartPoint())*dir;

                                pos = lineSeg->getStartPoint() + dir * length;
                                relPos = norm * 1;  //TODO Huh?
                            }
                            else if (geo2->getTypeId()== Part::GeomEllipse::getClassTypeId() || 
                                     geo2->getTypeId()== Part::GeomArcOfEllipse::getClassTypeId()) { 
                                     
                                Base::Vector3d center;
                                if(geo2->getTypeId()== Part::GeomEllipse::getClassTypeId()){
                                    const Part::GeomEllipse *ellipse = dynamic_cast<const Part::GeomEllipse *>(geo2);
                                    center=ellipse->getCenter();
                                } else {
                                    const Part::GeomArcOfEllipse *aoc = dynamic_cast<const Part::GeomArcOfEllipse *>(geo2);
                                    center=aoc->getCenter();                                    
                                }
                                                    
                                // tangency between a line and an ellipse
                                float length = (center - lineSeg->getStartPoint())*dir;

                                pos = lineSeg->getStartPoint() + dir * length;
                                relPos = norm * 1;  
                            }                            
                            else if (geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo2);
                                // tangency between a line and an arc
                                float length = (arc->getCenter() - lineSeg->getStartPoint())*dir;

                                pos = lineSeg->getStartPoint() + dir * length;
                                relPos = norm * 1;  //TODO Huh?
                            }
                        }

                        if (geo1->getTypeId()== Part::GeomCircle::getClassTypeId() && 
                            geo2->getTypeId()== Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle *circle1 = dynamic_cast<const Part::GeomCircle *>(geo1);
                            const Part::GeomCircle *circle2 = dynamic_cast<const Part::GeomCircle *>(geo2);
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
                            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo1);
                            const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo2);
                            // tangency between a circle and an arc
                            Base::Vector3d dir = (arc->getCenter() - circle->getCenter()).Normalize();
                            pos =  circle->getCenter() + dir *  circle->getRadius();
                            relPos = dir * 1;
                        }
                        else if (geo1->getTypeId()== Part::GeomArcOfCircle::getClassTypeId() &&
                                 geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle *arc1 = dynamic_cast<const Part::GeomArcOfCircle *>(geo1);
                            const Part::GeomArcOfCircle *arc2 = dynamic_cast<const Part::GeomArcOfCircle *>(geo2);
                            // tangency between two arcs
                            Base::Vector3d dir = (arc2->getCenter() - arc1->getCenter()).Normalize();
                            pos =  arc1->getCenter() + dir *  arc1->getRadius();
                            relPos = dir * 1;
                        }
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->abPos = SbVec3f(pos.x, pos.y, zConstr); //Absolute Reference
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = SbVec3f(relPos.x, relPos.y, 0);
                    }
                }
                break;
            case Symmetric:
                {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);

                    Base::Vector3d pnt1 = edit->ActSketch.getPoint(Constr->First, Constr->FirstPos);
                    Base::Vector3d pnt2 = edit->ActSketch.getPoint(Constr->Second, Constr->SecondPos);

                    SbVec3f p1(pnt1.x,pnt1.y,zConstr);
                    SbVec3f p2(pnt2.x,pnt2.y,zConstr);
                    SbVec3f dir = (p2-p1);
                    dir.normalize();
                    SbVec3f norm (-dir[1],dir[0],0);

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                    asciiText->datumtype    = SoDatumLabel::SYMMETRIC;

                    asciiText->pnts.setNum(2);
                    SbVec3f *verts = asciiText->pnts.startEditing();

                    verts[0] = p1;
                    verts[1] = p2;

                    asciiText->pnts.finishEditing();

                    dynamic_cast<SoTranslation *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION))->translation = (p1 + p2)/2;
                }
                break;
            case Angle:
                {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    assert((Constr->Second >= -extGeoCount && Constr->Second < intGeoCount) ||
                           Constr->Second == Constraint::GeoUndef);

                    SbVec3f p0;
                    double startangle,range,endangle;
                    if (Constr->Second != Constraint::GeoUndef) {
                        Base::Vector3d dir1, dir2;
                        if(Constr->Third == Constraint::GeoUndef) { //angle between two lines
                            const Part::Geometry *geo1 = GeoById(*geomlist, Constr->First);
                            const Part::Geometry *geo2 = GeoById(*geomlist, Constr->Second);
                            if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                                geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                                break;
                            const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment *>(geo1);
                            const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment *>(geo2);

                            bool flip1 = (Constr->FirstPos == end);
                            bool flip2 = (Constr->SecondPos == end);
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
                        } else {//angle-via-point
                            Base::Vector3d p = edit->ActSketch.getPoint(Constr->Third, Constr->ThirdPos);
                            p0 = SbVec3f(p.x, p.y, 0);
                            dir1 = edit->ActSketch.calculateNormalAtPoint(Constr->First, p.x, p.y);
                            dir1.RotateZ(-M_PI/2);//convert to vector of tangency by rotating
                            dir2 = edit->ActSketch.calculateNormalAtPoint(Constr->Second, p.x, p.y);
                            dir2.RotateZ(-M_PI/2);
                        }

                        startangle = atan2(dir1.y,dir1.x);
                        range = atan2(-dir1.y*dir2.x+dir1.x*dir2.y,
                                      dir1.x*dir2.x+dir1.y*dir2.y);
                        endangle = startangle + range;

                    } else if (Constr->First != Constraint::GeoUndef) {
                        const Part::Geometry *geo = GeoById(*geomlist, Constr->First);
                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo);
                            p0 = Base::convertTo<SbVec3f>((lineSeg->getEndPoint()+lineSeg->getStartPoint())/2);

                            Base::Vector3d dir = lineSeg->getEndPoint()-lineSeg->getStartPoint();
                            startangle = 0.;
                            range = atan2(dir.y,dir.x);
                            endangle = startangle + range;
                        }
                        else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo);
                            p0 = Base::convertTo<SbVec3f>(arc->getCenter());

                            Base::Vector3d dir = arc->getEndPoint()-arc->getStartPoint();
                            arc->getRange(startangle, endangle);
                            range = endangle - startangle;
                        }
                        else {
                            break;
                        }
                    } else
                        break;

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                    asciiText->string    = SbString(Base::Quantity(Base::toDegrees<double>(std::abs(Constr->Value)),Base::Unit::Angle).getUserString().toUtf8().constData());
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
            case Radius:
                {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);

                    Base::Vector3d pnt1(0.,0.,0.), pnt2(0.,0.,0.);
                    if (Constr->First != Constraint::GeoUndef) {
                        const Part::Geometry *geo = GeoById(*geomlist, Constr->First);

                        if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo);
                            double radius = arc->getRadius();
                            double angle = (double) Constr->LabelPosition;
                            if (angle == 10) {
                                double startangle, endangle;
                                arc->getRange(startangle, endangle);
                                angle = (startangle + endangle)/2;
                            }
                            pnt1 = arc->getCenter();
                            pnt2 = pnt1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                        }
                        else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) { 
                            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo);
                            double radius = circle->getRadius();
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

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL));
                    asciiText->string = SbString(Base::Quantity(Constr->Value,Base::Unit::Length).getUserString().toUtf8().constData());

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
                break;
        }
    }

    this->drawConstraintIcons();
    this->updateColor();

    // delete the cloned objects
    if (temp)
        for (std::vector<Part::Geometry *>::iterator it=tempGeo.begin(); it != tempGeo.end(); ++it)
            if (*it) delete *it;

    if (mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) { 
        static_cast<Gui::View3DInventor *>(mdi)->getViewer()->redraw();
    }
}

void ViewProviderSketch::rebuildConstraintsVisual(void)
{
    const std::vector<Sketcher::Constraint *> &constrlist = getSketchObject()->Constraints.getValues();
    // clean up
    edit->constrGroup->removeAllChildren();
    edit->vConstrType.clear();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    int fontSize = hGrp->GetInt("EditSketcherFontSize", 17);

    for (std::vector<Sketcher::Constraint *>::const_iterator it=constrlist.begin(); it != constrlist.end(); ++it) {
        // root separator for one constraint
        SoSeparator *sep = new SoSeparator();
        sep->ref();
        // no caching for fluctuand data structures
        sep->renderCaching = SoSeparator::OFF;

        // every constrained visual node gets its own material for preselection and selection
        SoMaterial *mat = new SoMaterial;
        mat->ref();
        mat->diffuseColor = ConstrDimColor;
        // Get sketch normal
        Base::Vector3d RN(0,0,1);

        // move to position of Sketch
        Base::Placement Plz = getSketchObject()->Placement.getValue();
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
            case Angle:
            {
                SoDatumLabel *text = new SoDatumLabel();
                text->norm.setValue(norm);
                text->string = "";
                text->textColor = ConstrDimColor;
                text->size.setValue(fontSize);
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
            {
                // #define CONSTRAINT_SEPARATOR_INDEX_MATERIAL_OR_DATUMLABEL 0
                sep->addChild(mat);
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_TRANSLATION 1
                sep->addChild(new SoZoomTranslation());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_ICON 2
                sep->addChild(new SoImage());
                // #define CONSTRAINT_SEPARATOR_INDEX_FIRST_CONSTRAINTID 3
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
                    if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
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

void ViewProviderSketch::drawEdit(const std::vector<Base::Vector2D> &EditCurve)
{
    assert(edit);

    edit->EditCurveSet->numVertices.setNum(1);
    edit->EditCurvesCoordinate->point.setNum(EditCurve.size());
    SbVec3f *verts = edit->EditCurvesCoordinate->point.startEditing();
    int32_t *index = edit->EditCurveSet->numVertices.startEditing();

    int i=0; // setting up the line set
    for (std::vector<Base::Vector2D>::const_iterator it = EditCurve.begin(); it != EditCurve.end(); ++it,i++)
        verts[i].setValue(it->fX,it->fY,zEdit);

    index[0] = EditCurve.size();
    edit->EditCurvesCoordinate->point.finishEditing();
    edit->EditCurveSet->numVertices.finishEditing();
}

void ViewProviderSketch::updateData(const App::Property *prop)
{
    ViewProvider2DObject::updateData(prop);

    if (edit && (prop == &(getSketchObject()->Geometry) ||
                 prop == &(getSketchObject()->Constraints))) {
        edit->FullyConstrained = false;
        solveSketch();
        draw(true);
    }
    if (edit && &(getSketchObject()->Constraints)) {
        // send the signal for the TaskDlg.
        signalConstraintsChanged();
    }
    if (edit && &(getSketchObject()->Geometry)) {
        // send the signal for the TaskDlg.
        signalElementsChanged();
    }
}

void ViewProviderSketch::onChanged(const App::Property *prop)
{
    // call father
    PartGui::ViewProvider2DObject::onChanged(prop);
}

void ViewProviderSketch::attach(App::DocumentObject *pcFeat)
{
    ViewProviderPart::attach(pcFeat);
}

void ViewProviderSketch::setupContextMenu(QMenu *menu, QObject *receiver, const char *member)
{
    menu->addAction(QObject::tr("Edit sketch"), receiver, member);
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
        msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
        msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
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
        QMessageBox::critical(Gui::getMainWindow(), tr("Invalid sketch"),
            tr("The sketch is invalid and cannot be edited.\nUse the sketch validation tool."));
        return false;
    }

    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    // create the container for the additional edit data
    assert(!edit);
    edit = new EditData();

    createEditInventorNodes();
    edit->visibleBeforeEdit = this->isVisible();
    this->hide(); // avoid that the wires interfere with the edit lines

    ShowGrid.setValue(true);
    TightGrid.setValue(false);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    float transparency;

    // set the point color
    unsigned long color = (unsigned long)(VertexColor.getPackedValue());
    color = hGrp->GetUnsigned("EditedVertexColor", color);
    VertexColor.setPackedValue((uint32_t)color, transparency);
    // set the curve color
    color = (unsigned long)(CurveColor.getPackedValue());
    color = hGrp->GetUnsigned("EditedEdgeColor", color);
    CurveColor.setPackedValue((uint32_t)color, transparency);
    // set the construction curve color
    color = (unsigned long)(CurveDraftColor.getPackedValue());
    color = hGrp->GetUnsigned("ConstructionColor", color);
    CurveDraftColor.setPackedValue((uint32_t)color, transparency);
    // set the cross lines color
    //CrossColorV.setPackedValue((uint32_t)color, transparency);
    //CrossColorH.setPackedValue((uint32_t)color, transparency);
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

    solveSketch();
    draw();

    return true;
}

QString ViewProviderSketch::appendConflictMsg(const std::vector<int> &conflicting)
{
    QString msg;
    QTextStream ss(&msg);
    if (conflicting.size() > 0) {
        if (conflicting.size() == 1)
            ss << tr("Please remove the following constraint:");
        else 
            ss << tr("Please remove at least one of the following constraints:");        
        ss << "\n";
        ss << conflicting[0];
        for (unsigned int i=1; i < conflicting.size(); i++)
            ss << ", " << conflicting[i];
        ss << "\n";
    }
    return msg;
}

QString ViewProviderSketch::appendRedundantMsg(const std::vector<int> &redundant)
{
    QString msg;
    QTextStream ss(&msg);
    if (redundant.size() > 0) {
        if (redundant.size() == 1)
            ss << tr("Please remove the following redundant constraint:");
        else
            ss << tr("Please remove the following redundant constraints:");
        ss << "\n";
        ss << redundant[0];
        for (unsigned int i=1; i < redundant.size(); i++)
            ss << ", " << redundant[i];
        
        ss << "\n";
    }
    return msg;
}

const std::vector<int> &ViewProviderSketch::getConflicting(void) const
{
    return edit->ActSketch.getConflicting();
}

const std::vector<int> &ViewProviderSketch::getRedundant(void) const
{
    return edit->ActSketch.getRedundant();  
}

void ViewProviderSketch::solveSketch(void)
{
    // set up the sketch and diagnose possible conflicts
    int dofs = edit->ActSketch.setUpSketch(getSketchObject()->getCompleteGeometry(),
                                           getSketchObject()->Constraints.getValues(),
                                           getSketchObject()->getExternalGeometryCount());
    if (getSketchObject()->Geometry.getSize() == 0) {
        signalSetUp(tr("Empty sketch"));
        signalSolved(QString());
    }
    else if (dofs < 0) { // over-constrained sketch
        std::string msg;
        SketchObject::appendConflictMsg(edit->ActSketch.getConflicting(), msg);
        signalSetUp(QString::fromLatin1("<font color='red'>%1<a href=\"#conflicting\"><span style=\" text-decoration: underline; color:#0000ff;\">%2</span></a><br/>%3</font><br/>")
                    .arg(tr("Over-constrained sketch "))
                    .arg(tr("(click to select)"))
                    .arg(QString::fromStdString(msg)));        
        signalSolved(QString());
    }
    else if (edit->ActSketch.hasConflicts()) { // conflicting constraints
        signalSetUp(QString::fromLatin1("<font color='red'>%1<a href=\"#conflicting\"><span style=\" text-decoration: underline; color:#0000ff;\">%2</span></a><br/>%3</font><br/>")
                    .arg(tr("Sketch contains conflicting constraints "))
                    .arg(tr("(click to select)"))
                    .arg(appendConflictMsg(edit->ActSketch.getConflicting())));
        signalSolved(QString());
    }
    else {
        if (edit->ActSketch.hasRedundancies()) { // redundant constraints
            signalSetUp(QString::fromLatin1("<font color='orangered'>%1<a href=\"#redundant\"><span style=\" text-decoration: underline; color:#0000ff;\">%2</span></a><br/>%3</font><br/>")
                        .arg(tr("Sketch contains redundant constraints "))
                        .arg(tr("(click to select)"))
                        .arg(appendRedundantMsg(edit->ActSketch.getRedundant())));
        }
        if (edit->ActSketch.solve() == 0) { // solving the sketch
            if (dofs == 0) {
                // color the sketch as fully constrained
                edit->FullyConstrained = true;
                if (!edit->ActSketch.hasRedundancies()) {
                    signalSetUp(QString::fromLatin1("<font color='green'>%1</font>").arg(tr("Fully constrained sketch")));
                }
            }
            else if (!edit->ActSketch.hasRedundancies()) {
                if (dofs == 1)
                    signalSetUp(tr("Under-constrained sketch with 1 degree of freedom"));
                else
                    signalSetUp(tr("Under-constrained sketch with %1 degrees of freedom").arg(dofs));
            }
            
            signalSolved(tr("Solved in %1 sec").arg(edit->ActSketch.SolveTime));
        }
        else {
            signalSolved(tr("Unsolved (%1 sec)").arg(edit->ActSketch.SolveTime));
        }
    }
}

void ViewProviderSketch::createEditInventorNodes(void)
{
    assert(edit);

    edit->EditRoot = new SoSeparator;
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

    SoDrawStyle *DrawStyle = new SoDrawStyle;
    DrawStyle->setName("PointsDrawStyle");
    DrawStyle->pointSize = 8;
    pointsRoot->addChild(DrawStyle);

    edit->PointSet = new SoMarkerSet;
    edit->PointSet->setName("PointSet");
    edit->PointSet->markerIndex = SoMarkerSet::CIRCLE_FILLED_7_7;
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

    DrawStyle = new SoDrawStyle;
    DrawStyle->setName("CurvesDrawStyle");
    DrawStyle->lineWidth = 3;
    curvesRoot->addChild(DrawStyle);

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

    DrawStyle = new SoDrawStyle;
    DrawStyle->setName("RootCrossDrawStyle");
    DrawStyle->lineWidth = 2;
    crossRoot->addChild(DrawStyle);

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

    DrawStyle = new SoDrawStyle;
    DrawStyle->setName("EditCurvesDrawStyle");
    DrawStyle->lineWidth = 3;
    editCurvesRoot->addChild(DrawStyle);

    edit->EditCurveSet = new SoLineSet;
    edit->EditCurveSet->setName("EditCurveLineSet");
    editCurvesRoot->addChild(edit->EditCurveSet);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    float transparency;
    SbColor cursorTextColor(0,0,1);
    cursorTextColor.setPackedValue((uint32_t)hGrp->GetUnsigned("CursorTextColor", cursorTextColor.getPackedValue()), transparency);

    // stuff for the edit coordinates ++++++++++++++++++++++++++++++++++++++
    SoSeparator *Coordsep = new SoSeparator();
    Coordsep->setName("CoordSeparator");
    // no caching for fluctuand data structures
    Coordsep->renderCaching = SoSeparator::OFF;

    SoMaterial *CoordTextMaterials = new SoMaterial;
    CoordTextMaterials->setName("CoordTextMaterials");
    CoordTextMaterials->diffuseColor = cursorTextColor;
    Coordsep->addChild(CoordTextMaterials);

    SoFont *font = new SoFont();
    font->size = 10.0;
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
    DrawStyle = new SoDrawStyle;
    DrawStyle->setName("ConstraintDrawStyle");
    DrawStyle->lineWidth = 1;
    edit->EditRoot->addChild(DrawStyle);

    // add the group where all the constraints has its SoSeparator
    edit->constrGroup = new SoGroup();
    edit->constrGroup->setName("ConstraintGroup");
    edit->EditRoot->addChild(edit->constrGroup);
}

void ViewProviderSketch::unsetEdit(int ModNum)
{
    ShowGrid.setValue(false);
    TightGrid.setValue(true);

    if (edit->sketchHandler)
        deactivateHandler();

    edit->EditRoot->removeAllChildren();
    pcRoot->removeChild(edit->EditRoot);

    if (edit->visibleBeforeEdit)
        this->show();
    else
        this->hide();

    delete edit;
    edit = 0;

    try {
        // and update the sketch
        getSketchObject()->getDocument()->recompute();
    }
    catch (...) {
    }

    // clear the selection and set the new/edited sketch(convenience)
    Gui::Selection().clearSelection();
    std::string ObjName = getSketchObject()->getNameInDocument();
    std::string DocName = getSketchObject()->getDocument()->getName();
    Gui::Selection().addSelection(DocName.c_str(),ObjName.c_str());

    // when pressing ESC make sure to close the dialog
    Gui::Control().closeDialog();
}

void ViewProviderSketch::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Base::Placement plm = getSketchObject()->Placement.getValue();
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

    viewer->setEditing(TRUE);
    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(FALSE);
    
    viewer->addGraphicsItem(rubberband);
    rubberband->setViewer(viewer);
}

void ViewProviderSketch::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    viewer->removeGraphicsItem(rubberband);
    viewer->setEditing(FALSE);
    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(TRUE);
}

void ViewProviderSketch::setPositionText(const Base::Vector2D &Pos, const SbString &text)
{
    edit->textX->string = text;
    edit->textPos->translation = SbVec3f(Pos.fX,Pos.fY,zText);
}

void ViewProviderSketch::setPositionText(const Base::Vector2D &Pos)
{
    SbString text;
    text.sprintf(" (%.1f,%.1f)", Pos.fX, Pos.fY);
    edit->textX->string = text;
    edit->textPos->translation = SbVec3f(Pos.fX,Pos.fY,zText);
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
            pverts[oldPtId].setValue(x,y,zPoints);
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
            pverts[oldPtId].setValue(x,y,zPoints);
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
        pverts[PtId].setValue(x,y,zPoints);
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
            pverts[*it].setValue(x,y,zPoints);
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

bool ViewProviderSketch::onDelete(const std::vector<std::string> &subList)
{
    if (edit) {
        std::vector<std::string> SubNames = subList;

        Gui::Selection().clearSelection();
        resetPreselectPoint();
        edit->PreselectCurve = -1;
        edit->PreselectCross = -1;
        edit->PreselectConstraintSet.clear();

        std::set<int> delGeometries, delCoincidents, delConstraints;
        // go through the selected subelements
        for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
            if (it->size() > 4 && it->substr(0,4) == "Edge") {
                int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
                delGeometries.insert(GeoId);
            } else if (it->size() > 12 && it->substr(0,12) == "ExternalEdge") {
                int GeoId = std::atoi(it->substr(12,4000).c_str()) - 1;
                GeoId = -GeoId - 3;
                delGeometries.insert(GeoId);
            } else if (it->size() > 6 && it->substr(0,6) == "Vertex") {
                int VtId = std::atoi(it->substr(6,4000).c_str()) - 1;
                int GeoId;
                Sketcher::PointPos PosId;
                getSketchObject()->getGeoVertexIndex(VtId, GeoId, PosId);
                if (getSketchObject()->getGeometry(GeoId)->getTypeId()
                    == Part::GeomPoint::getClassTypeId())
                    delGeometries.insert(GeoId);
                else
                    delCoincidents.insert(VtId);
            } else if (*it == "RootPoint") {
                delCoincidents.insert(-1);
            } else if (it->size() > 10 && it->substr(0,10) == "Constraint") {
                int ConstrId = std::atoi(it->substr(10,4000).c_str()) - 1;
                delConstraints.insert(ConstrId);
            }
        }

        std::set<int>::const_reverse_iterator rit;
        for (rit = delConstraints.rbegin(); rit != delConstraints.rend(); rit++) {
            try {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.delConstraint(%i)"
                                       ,getObject()->getNameInDocument(), *rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
            }
        }

        for (rit = delCoincidents.rbegin(); rit != delCoincidents.rend(); rit++) {
            try {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.delConstraintOnPoint(%i)"
                                       ,getObject()->getNameInDocument(), *rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
            }
        }

        for (rit = delGeometries.rbegin(); rit != delGeometries.rend(); rit++) {
            try {
                if (*rit >= 0)
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.delGeometry(%i)"
                                           ,getObject()->getNameInDocument(), *rit);
                else if (*rit < -2) // external geometry
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.delExternal(%i)"
                                           ,getObject()->getNameInDocument(), -3-*rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
            }
        }

        this->drawConstraintIcons();
        this->updateColor();
        // if in edit not delete the object
        return false;
    }
    // if not in edit delete the whole object
    return true;
}
