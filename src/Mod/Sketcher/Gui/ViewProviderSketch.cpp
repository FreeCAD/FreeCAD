/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Geom_TrimmedCurve.hxx>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/SoPath.h>
# include <Inventor/SbBox3f.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/details/SoPointDetail.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoImage.h>
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
# include <Inventor/sensors/SoIdleSensor.h>
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
#include <Gui/GLPainter.h>
#include <Gui/Selection.h>
#include <Gui/Utilities.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/DlgEditFileIncludeProptertyExternal.h>
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

// Variables for holding previous click
SbTime  ViewProviderSketch::prvClickTime;
SbVec3f ViewProviderSketch::prvClickPoint;
SbVec2s ViewProviderSketch::prvCursorPos;
SbVec2s ViewProviderSketch::newCursorPos;

//**************************************************************************
// Edit data structure

/// Data structure while edit the sketch
struct EditData {
    EditData():
    sketchHandler(0),
    editDatumDialog(false),
    DragPoint(-1),
    DragCurve(-1),
    DragConstraint(-1),
    PreselectPoint(-1),
    PreselectCurve(-1),
    PreselectCross(-1),
    PreselectConstraint(-1),
    blockedPreselection(false),
    FullyConstrained(false),
    //ActSketch(0),
    EditRoot(0),
    PointsMaterials(0),
    CurvesMaterials(0),
    PointsCoordinate(0),
    CurvesCoordinate(0),
    CurveSet(0), EditCurveSet(0), RootCrossSet(0),
    PointSet(0)
    {}

    // pointer to the active handler for new sketch objects
    DrawSketchHandler *sketchHandler;
    bool editDatumDialog;

    // dragged point
    int DragPoint;
    // dragged curve
    int DragCurve;
    // dragged constraint
    int DragConstraint;

    SbColor PreselectOldColor;
    int PreselectPoint;
    int PreselectCurve;
    int PreselectCross;
    int PreselectConstraint;
    bool blockedPreselection;
    bool FullyConstrained;

    // instance of the solver
    Sketcher::Sketch ActSketch;
    // container to track our own selected parts
    std::set<int> SelPointSet;
    std::set<int> SelCurvSet; // also holds cross axes at -1 and -2
    std::set<int> SelConstraintSet;
    std::vector<int> CurvIdToGeoId; // conversion of SoLineSet index to GeoId

    // helper data structure for the constraint rendering
    std::vector<ConstraintType> vConstrType;

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
    zConstr=0.002f;
    zLines=0.003f;
    zPoints=0.004f;
    zHighlight=0.005f;
    zText=0.006f;
    zEdit=0.001f;

    xInit=0;
    yInit=0;
    relative=false;
}

ViewProviderSketch::~ViewProviderSketch()
{
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

/// removes the active handler
void ViewProviderSketch::purgeHandler(void)
{
    assert(edit);
    assert(edit->sketchHandler != 0);
    edit->sketchHandler->unsetCursor();
    delete(edit->sketchHandler);
    edit->sketchHandler = 0;
    Mode = STATUS_NONE;

    // ensure that we are in sketch only selection mode
    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    Gui::View3DInventorViewer *viewer;
    viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(FALSE);
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
            if (edit && edit->DragConstraint >= 0) {
                if (!pressed) {
                    edit->DragConstraint = -1;
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
    const SbViewportRegion& vp = viewer->getViewportRegion();

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

    SoCamera* pCam = viewer->getCamera();
    if (!pCam) return;
    SbViewVolume  vol = pCam->getViewVolume();

    float focalDist = pCam->focalDistance.getValue();

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
        throw Base::Exception("View direction is parallel to sketch plane");
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

    getCoordsOnSketchPlane(x,y,pos,normal);
    snapToGrid(x, y);

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
                    } else if (edit->PreselectConstraint != -1) {
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
        } else { // released
            // Do things depending on the mode of the user interaction
            switch (Mode) {
                case STATUS_SELECT_Point:
                    if (pp) {
                        //Base::Console().Log("Select Point:%d\n",this->DragPoint);
                        // Do selection
                        std::stringstream ss;
                        ss << "Vertex" << edit->PreselectPoint;

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
                            this->edit->DragConstraint = -1;
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SELECT_Edge:
                    if (pp) {
                        //Base::Console().Log("Select Point:%d\n",this->DragPoint);
                        std::stringstream ss;
                        if (edit->PreselectCurve >= 0)
                            ss << "Edge" << edit->PreselectCurve;
                        else // external geometry
                            ss << "ExternalEdge" << -edit->PreselectCurve - 3;

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
                            this->edit->DragConstraint = -1;
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
                            this->edit->DragConstraint = -1;
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SELECT_Constraint:
                    if (pp) {

                        std::stringstream ss;
                        ss << "Constraint" << edit->PreselectConstraint;

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
                            this->edit->DragConstraint = -1;
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_DragPoint:
                    if (edit->DragPoint != -1 && pp) {
                        int GeoId;
                        Sketcher::PointPos PosId;
                        getSketchObject()->getGeoVertexIndex(edit->DragPoint, GeoId, PosId);
                        Gui::Command::openCommand("Drag Point");
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.movePoint(%i,%i,App.Vector(%f,%f,0),%i)"
                                               ,getObject()->getNameInDocument()
                                               ,GeoId, PosId, x-xInit, y-yInit, relative ? 1 : 0
                                               );
                        Gui::Command::commitCommand();
                        Gui::Command::updateActive();

                        setPreselectPoint(edit->DragPoint);
                        edit->DragPoint = -1;
                        //updateColor();
                    }
                    resetPositionText();
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_DragCurve:
                    if (edit->DragCurve != -1 && pp) {
                        const Part::Geometry *geo = getSketchObject()->getGeometry(edit->DragCurve);
                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId() ||
                            geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                            geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            Gui::Command::openCommand("Drag Curve");
                            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.movePoint(%i,%i,App.Vector(%f,%f,0),%i)"
                                                   ,getObject()->getNameInDocument()
                                                   ,edit->DragCurve, Sketcher::none, x-xInit, y-yInit, relative ? 1 : 0
                                                   );
                            Gui::Command::commitCommand();
                            Gui::Command::updateActive();
                        }
                        edit->PreselectCurve = edit->DragCurve;
                        edit->DragCurve = -1;
                        //updateColor();
                    }
                    resetPositionText();
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_DragConstraint:
                    if (edit->DragConstraint != -1 && pp) {
                        Gui::Command::openCommand("Drag Constraint");
                        moveConstraint(edit->DragConstraint, Base::Vector2D(x, y));
                        edit->PreselectConstraint = edit->DragConstraint;
                        edit->DragConstraint = -1;
                        //updateColor();
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_StartRubberBand: // a single click happened, so clear selection
                    Mode = STATUS_NONE;
                    Gui::Selection().clearSelection();
                    return true;
                case STATUS_SKETCH_UseRubberBand:
                    doBoxSelection(prvCursorPos, cursorPos, viewer);
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
                        } else if (edit->PreselectConstraint != -1) {
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
                                  << "Sketcher_CreateCircle"
                                  << "Sketcher_CreateLine"
                                  << "Sketcher_CreatePolyline"
                                  << "Sketcher_CreateRectangle"
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
                            QAction *used = contextMenu.exec(QCursor::pos());

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
                                        int index=std::atoi(it->substr(4,4000).c_str());
                                        if (edit->PreselectCurve == index)
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
                        QAction *used = contextMenu.exec(QCursor::pos());

                        return true;
                    }
                case STATUS_SELECT_Cross:
                case STATUS_SELECT_Constraint:
                case STATUS_SKETCH_DragPoint:
                case STATUS_SKETCH_DragCurve:
                case STATUS_SKETCH_DragConstraint:
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
    else if (edit->PreselectConstraint != -1) {
        // Find the constraint
        Base::Console().Log("double click constraint:%d\n",edit->PreselectConstraint);

        const std::vector<Sketcher::Constraint *> &constrlist = getSketchObject()->Constraints.getValues();
        Constraint *Constr = constrlist[edit->PreselectConstraint];

        // if its the right constraint
        if (Constr->Type == Sketcher::Distance ||
            Constr->Type == Sketcher::DistanceX || Constr->Type == Sketcher::DistanceY ||
            Constr->Type == Sketcher::Radius || Constr->Type == Sketcher::Angle) {

            EditDatumDialog * editDatumDialog = new EditDatumDialog(this, edit->PreselectConstraint);
            SoIdleSensor* sensor = new SoIdleSensor(EditDatumDialog::run, editDatumDialog);
            sensor->schedule();
            edit->editDatumDialog = true; // avoid to double handle "ESC"
        }
    }
}

bool ViewProviderSketch::mouseMove(const SbVec2s &cursorPos, Gui::View3DInventorViewer *viewer)
{
    if (!edit)
        return false;
    assert(edit);

    // Calculate 3d point to the mouse position
    SbLine line;
    getProjectingLine(cursorPos, viewer, line);

    double x,y;
    getCoordsOnSketchPlane(x,y,line.getPosition(),line.getDirection());
    snapToGrid(x, y);

    bool preselectChanged;
    if (Mode!=STATUS_SKETCH_DragPoint && Mode!=STATUS_SKETCH_DragCurve &&
        Mode!=STATUS_SKETCH_DragConstraint) {

        SoPickedPoint *pp = this->getPointOnRay(cursorPos, viewer);
        int PtIndex,GeoIndex,ConstrIndex,CrossIndex;
        preselectChanged = detectPreselection(pp,PtIndex,GeoIndex,ConstrIndex,CrossIndex);
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
                edit->ActSketch.initMove(GeoId, PosId, false);
                relative = false;
                xInit = 0;
                yInit = 0;
            } else {
                Mode = STATUS_NONE;
            }
            resetPreselectPoint();
            edit->PreselectCurve = -1;
            edit->PreselectCross = -1;
            edit->PreselectConstraint = -1;
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
                    xInit = x;
                    yInit = y;
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
            edit->PreselectConstraint = -1;
            return true;
        case STATUS_SELECT_Constraint:
            Mode = STATUS_SKETCH_DragConstraint;
            edit->DragConstraint = edit->PreselectConstraint;
            resetPreselectPoint();
            edit->PreselectCurve = -1;
            edit->PreselectCross = -1;
            edit->PreselectConstraint = -1;
            return true;
        case STATUS_SKETCH_DragPoint:
            if (edit->DragPoint != -1) {
                //Base::Console().Log("Drag Point:%d\n",edit->DragPoint);
                int GeoId;
                Sketcher::PointPos PosId;
                getSketchObject()->getGeoVertexIndex(edit->DragPoint, GeoId, PosId);
                Base::Vector3d vec(x-xInit,y-yInit,0);
                if (edit->ActSketch.movePoint(GeoId, PosId, vec, relative) == 0) {
                    setPositionText(Base::Vector2D(x,y));
                    draw(true);
                    signalSolved(QString::fromLatin1("Solved in %1 sec").arg(edit->ActSketch.SolveTime));
                } else {
                    signalSolved(QString::fromLatin1("Unsolved (%1 sec)").arg(edit->ActSketch.SolveTime));
                    //Base::Console().Log("Error solving:%d\n",ret);
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
            if (edit->DragConstraint != -1) {
                moveConstraint(edit->DragConstraint, Base::Vector2D(x,y));
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
            return true;
        }
        case STATUS_SKETCH_UseRubberBand: {
            Gui::GLPainter p;
            p.begin(viewer);
            p.setColor(1.0, 1.0, 0.0, 0.0);
            p.setLogicOp(GL_XOR);
            p.setLineWidth(3.0f);
            p.setLineStipple(2, 0x3F3F);
            // first redraw the old rectangle with XOR to restore the correct colors
            p.drawRect(prvCursorPos.getValue()[0],
                       viewer->getGLWidget()->height() - prvCursorPos.getValue()[1],
                       newCursorPos.getValue()[0],
                       viewer->getGLWidget()->height() - newCursorPos.getValue()[1]);
            newCursorPos = cursorPos;
            // now draw the new rectangle
            p.drawRect(prvCursorPos.getValue()[0],
                       viewer->getGLWidget()->height() - prvCursorPos.getValue()[1],
                       newCursorPos.getValue()[0],
                       viewer->getGLWidget()->height() - newCursorPos.getValue()[1]);
            p.end();
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
                double startangle, endangle;
                arc->getRange(startangle, endangle);
                double angle = (startangle + endangle)/2;
                p1 = arc->getCenter();
                p2 = p1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
            }
            else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo);
                double radius = circle->getRadius();
                p1 = circle->getCenter();
                Base::Vector3d tmpDir =  Base::Vector3d(toPos.fX, toPos.fY, 0) - p1;
                double angle = atan2f(tmpDir.y, tmpDir.x);
                p2 = p1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
            } else
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
            Constr->LabelPosition = atan2f(dir.y, dir.x);
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
            const Part::Geometry *geo1 = GeoById(geomlist, Constr->First);
            const Part::Geometry *geo2 = GeoById(geomlist, Constr->Second);
            if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                return;
            const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment *>(geo1);
            const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment *>(geo2);

            bool flip1 = (Constr->FirstPos == end);
            bool flip2 = (Constr->SecondPos == end);
            Base::Vector3d dir1 = (flip1 ? -1. : 1.) * (lineSeg1->getEndPoint()-lineSeg1->getStartPoint());
            Base::Vector3d dir2 = (flip2 ? -1. : 1.) * (lineSeg2->getEndPoint()-lineSeg2->getStartPoint());
            Base::Vector3d pnt1 = flip1 ? lineSeg1->getEndPoint() : lineSeg1->getStartPoint();
            Base::Vector3d pnt2 = flip2 ? lineSeg2->getEndPoint() : lineSeg2->getStartPoint();

            // line-line intersection
            {
                double det = dir1.x*dir2.y - dir1.y*dir2.x;
                if ((det > 0 ? det : -det) < 1e-10)
                    return;
                double c1 = dir1.y*pnt1.x - dir1.x*pnt1.y;
                double c2 = dir2.y*pnt2.x - dir2.x*pnt2.y;
                double x = (dir1.x*c2 - dir2.x*c1)/det;
                double y = (dir1.y*c2 - dir2.y*c1)/det;
                p0 = Base::Vector3d(x,y,0);
            }
        } else if (Constr->First != Constraint::GeoUndef) { // line angle
            const Part::Geometry *geo = GeoById(geomlist, Constr->First);
            if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                return;
            const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo);
            p0 = (lineSeg->getEndPoint()+lineSeg->getStartPoint())/2;
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
    SoRayPickAction rp(viewer->getViewportRegion());

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
        } else
            isConstraintAtPosition = false;

        multiplier *= -1; // search in both sides
        if  (multiplier >= 0)
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
                            int index=std::atoi(&shapetype[4]);
                            edit->SelCurvSet.insert(index);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 12 && shapetype.substr(0,12) == "ExternalEdge") {
                            int index=std::atoi(&shapetype[12]);
                            edit->SelCurvSet.insert(-index-3);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 6 && shapetype.substr(0,6) == "Vertex") {
                            int index=std::atoi(&shapetype[6]);
                            addSelectPoint(index);
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
                            int index=std::atoi(&shapetype[10]);
                            edit->SelConstraintSet.insert(index);
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
                            int index=std::atoi(&shapetype[4]);
                            edit->SelCurvSet.erase(index);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 12 && shapetype.substr(0,12) == "ExternalEdge") {
                            int index=std::atoi(&shapetype[12]);
                            edit->SelCurvSet.erase(-index-3);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 6 && shapetype.substr(0,6) == "Vertex") {
                            int index=std::atoi(&shapetype[6]);
                            removeSelectPoint(index);
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
                            int index=std::atoi(&shapetype[10]);
                            edit->SelConstraintSet.erase(index);
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

    }
}

bool ViewProviderSketch::detectPreselection(const SoPickedPoint *Point, int &PtIndex, int &GeoIndex, int &ConstrIndex, int &CrossIndex)
{
    assert(edit);

    PtIndex = -1;
    GeoIndex = -1; // valid values are 0,1,2,... for normal geometry and -3,-4,-5,... for external geometry
    CrossIndex = -1;
    ConstrIndex = -1;

    if (Point) {
        //Base::Console().Log("Point pick\n");
        SoPath *path = Point->getPath();
        SoNode *tail = path->getTail();
        SoNode *tailFather = path->getNode(path->getLength()-2);
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
                    int CurvIndex = static_cast<const SoLineDetail *>(curve_detail)->getLineIndex();
                    GeoIndex = edit->CurvIdToGeoId[CurvIndex];
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
                    for (int i=0; i < edit->constrGroup->getNumChildren(); i++)
                        if (edit->constrGroup->getChild(i) == tailFather) {
                            ConstrIndex = i;
                            //Base::Console().Log("Constr %d pick\n",i);
                            break;
                        }
            }
        }

        if (PtIndex != -1 && PtIndex != edit->PreselectPoint) { // if a new point is hit
            std::stringstream ss;
            ss << "Vertex" << PtIndex;
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
                edit->PreselectConstraint = -1;
                if (edit->sketchHandler)
                    edit->sketchHandler->applyCursor();
                return true;
            }
        } else if (GeoIndex != -1 && GeoIndex != edit->PreselectCurve) {  // if a new curve is hit
            std::stringstream ss;
            if (GeoIndex >= 0)
                ss << "Edge" << GeoIndex;
            else // external geometry
                ss << "ExternalEdge" << -GeoIndex - 3; // convert index start from -3 to 0
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
                edit->PreselectConstraint = -1;
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
                edit->PreselectConstraint = -1;
                if (edit->sketchHandler)
                    edit->sketchHandler->applyCursor();
                return true;
            }
        } else if (ConstrIndex != -1 && ConstrIndex != edit->PreselectConstraint) { // if a constraint is hit
            std::stringstream ss;
            ss << "Constraint" << ConstrIndex;
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
                edit->PreselectCurve = -1;
                edit->PreselectCross = -1;
                edit->PreselectConstraint = ConstrIndex;
                if (edit->sketchHandler)
                    edit->sketchHandler->applyCursor();
                return true;
            }
        } else if ((PtIndex == -1 && GeoIndex == -1 && CrossIndex == -1 && ConstrIndex == -1) &&
                   (edit->PreselectPoint != -1 || edit->PreselectCurve != -1 || edit->PreselectCross != -1
                    || edit->PreselectConstraint != -1 || edit->blockedPreselection)) {
            // we have just left a preselection
            resetPreselectPoint();
            edit->PreselectCurve = -1;
            edit->PreselectCross = -1;
            edit->PreselectConstraint = -1;
            edit->blockedPreselection = false;
            if (edit->sketchHandler)
                edit->sketchHandler->applyCursor();
            return true;
        }
        Gui::Selection().setPreselectCoord(Point->getPoint()[0]
                                          ,Point->getPoint()[1]
                                          ,Point->getPoint()[2]);
    } else if (edit->PreselectCurve != -1 || edit->PreselectPoint != -1 ||
               edit->PreselectConstraint != -1 || edit->PreselectCross != -1 || edit->blockedPreselection) {
        resetPreselectPoint();
        edit->PreselectCurve = -1;
        edit->PreselectCross = -1;
        edit->PreselectConstraint = -1;
        edit->blockedPreselection = false;
        if (edit->sketchHandler)
            edit->sketchHandler->applyCursor();
        return true;
    }

    return false;
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

    Gui::ViewVolumeProjection proj(viewer->getCamera()->getViewVolume());

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
                ss << "Vertex" << VertexId;
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
                ss << "Vertex" << VertexId - 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            if (pnt2Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            if (pnt1Inside && pnt2Inside) {
                std::stringstream ss;
                ss << "Edge" << GeoId;
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
                ss << "Vertex" << VertexId;
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
                    ss << "Edge" << GeoId;
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
                ss << "Vertex" << VertexId - 2;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            bool pnt1Inside = polygon.Contains(Base::Vector2D(pnt1.x, pnt1.y));
            if (pnt1Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId - 1;
                Gui::Selection().addSelection(doc->getName(), sketchObject->getNameInDocument(), ss.str().c_str());
            }

            if (polygon.Contains(Base::Vector2D(pnt2.x, pnt2.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
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
                    ss << "Edge" << GeoId;
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

    // colors of the point set
    if (edit->FullyConstrained)
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = FullyConstrainedColor;
    else
        for (int  i=0; i < PtNum; i++)
            pcolor[i] = VertexColor;

    if (edit->PreselectCross == 0)
        pcolor[0] = PreselectColor;
    else if (edit->PreselectPoint != -1)
        pcolor[edit->PreselectPoint + 1] = PreselectColor;

    for (std::set<int>::iterator it=edit->SelPointSet.begin();
         it != edit->SelPointSet.end(); it++)
        pcolor[*it] = SelectColor;

    // colors of the curves
    int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;
    int extGeoCount = getSketchObject()->getExternalGeometryCount();
    for (int  i=0; i < CurvNum; i++) {
        int GeoId = edit->CurvIdToGeoId[i];
        if (edit->SelCurvSet.find(GeoId) != edit->SelCurvSet.end())
            color[i] = SelectColor;
        else if (edit->PreselectCurve == GeoId)
            color[i] = PreselectColor;
        else if (GeoId < -2)  // external Geometry
            color[i] = CurveExternalColor;
        else if (getSketchObject()->getGeometry(GeoId)->Construction)
            color[i] = CurveDraftColor;
        else if (edit->FullyConstrained)
            color[i] = FullyConstrainedColor;
        else
            color[i] = CurveColor;
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
        if (!hasDatumLabel && type != Sketcher::Coincident) {
            hasMaterial = true;
            m = dynamic_cast<SoMaterial *>(s->getChild(0));
        }

        if (edit->SelConstraintSet.find(i) != edit->SelConstraintSet.end()) {
            if (hasDatumLabel) {
                SoDatumLabel *l = dynamic_cast<SoDatumLabel *>(s->getChild(0));
                l->textColor = SelectColor;
            } else if (hasMaterial) {
                m->diffuseColor = SelectColor;
            } else if (type == Sketcher::Coincident) {
                int index;
                index = edit->ActSketch.getPointId(constraint->First, constraint->FirstPos) + 1;
                if (index >= 0 && index < PtNum) pcolor[index] = SelectColor;
                index = edit->ActSketch.getPointId(constraint->Second, constraint->SecondPos) + 1;
                if (index >= 0 && index < PtNum) pcolor[index] = SelectColor;
            }
        } else if (edit->PreselectConstraint == i) {
            if (hasDatumLabel) {
                SoDatumLabel *l = dynamic_cast<SoDatumLabel *>(s->getChild(0));
                l->textColor = PreselectColor;
            } else if (hasMaterial) {
                m->diffuseColor = PreselectColor;
            }
        }
        else {
            if (hasDatumLabel) {
                SoDatumLabel *l = dynamic_cast<SoDatumLabel *>(s->getChild(0));
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

void ViewProviderSketch::drawConstraintIcons()
{
    const std::vector<Sketcher::Constraint *> &constraints = getSketchObject()->Constraints.getValues();
    int constrId = 0;
    for (std::vector<Sketcher::Constraint *>::const_iterator it=constraints.begin();
         it != constraints.end(); ++it, constrId++) {
        // Check if Icon Should be created
        int index1 = 2, index2 = -1; // Index for SoImage Nodes in SoContainer
        QString icoType;
        switch((*it)->Type) {
        case Horizontal:
            icoType = QString::fromAscii("small/Constraint_Horizontal_sm");
            break;
        case Vertical:
            icoType = QString::fromAscii("small/Constraint_Vertical_sm");
            break;
        case PointOnObject:
            icoType = QString::fromAscii("small/Constraint_PointOnObject_sm");
            break;
        case Tangent:
            icoType = QString::fromAscii("small/Constraint_Tangent_sm");
            {   // second icon is available only for colinear line segments
                const Part::Geometry *geo1 = getSketchObject()->getGeometry((*it)->First);
                const Part::Geometry *geo2 = getSketchObject()->getGeometry((*it)->Second);
                if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                    geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                    index2 = 4;
                }
            }
            break;
        case Parallel:
            icoType = QString::fromAscii("small/Constraint_Parallel_sm");
            index2 = 4;
            break;
        case Perpendicular:
            icoType = QString::fromAscii("small/Constraint_Perpendicular_sm");
            // second icon is available only when there is no common point
            if ((*it)->FirstPos == Sketcher::none)
                index2 = 4;
            break;
        case Equal:
            icoType = QString::fromAscii("small/Constraint_EqualLength_sm");
            index2 = 4;
            break;
        case Symmetric:
            icoType = QString::fromAscii("small/Constraint_Symmetric_sm");
            index1 = 2;
            break;
        default:
            continue; // Icon shouldn't be generated
        }

        // Constants to help create constraint icons
        const int constrImgSize = 16;

        QColor constrIcoColor((int)(ConstrIcoColor [0] * 255.0f), (int)(ConstrIcoColor[1] * 255.0f),(int)(ConstrIcoColor[2] * 255.0f));
        QColor constrIconSelColor ((int)(SelectColor[0] * 255.0f), (int)(SelectColor[1] * 255.0f),(int)(SelectColor[2] * 255.0f));
        QColor constrIconPreselColor ((int)(PreselectColor[0] * 255.0f), (int)(PreselectColor[1] * 255.0f),(int)(PreselectColor[2] * 255.0f));

        // Set Color for Icons
        QColor iconColor;
        if (edit->PreselectConstraint == constrId)
            iconColor = constrIconPreselColor;
        else if (edit->SelConstraintSet.find(constrId) != edit->SelConstraintSet.end())
            iconColor = constrIconSelColor;
        else
            iconColor = constrIcoColor;

        // Create Icons

        // Create a QPainter for the constraint icon rendering
        QPainter qp;
        QImage icon;

        icon = Gui::BitmapFactory().pixmap(icoType.toAscii()).toImage();

        // Assumes that digits are 9 pixel wide
        int imgwidth = icon.width() + ((index2 == -1) ? 0 : 9 * (1 + (constrId + 1)/10));
        QImage image = icon.copy(0, 0, imgwidth, icon.height());

        // Paint the Icons
        qp.begin(&image);
        qp.setCompositionMode(QPainter::CompositionMode_SourceIn);
        qp.fillRect(0,0, constrImgSize, constrImgSize, iconColor);

        // Render constraint index if necessary
        if (index2 != -1) {
            qp.setCompositionMode(QPainter::CompositionMode_SourceOver);
            qp.setPen(iconColor);
            QFont font = QApplication::font();
            font.setPixelSize(11);
            font.setBold(true);
            qp.setFont(font);
            qp.drawText(constrImgSize, image.height(), QString::number(constrId + 1));
        }
        qp.end();

        SoSFImage icondata = SoSFImage();

        Gui::BitmapFactory().convert(image, icondata);

        int nc = 4;
        SbVec2s iconSize(image.width(), image.height());

        // Find the Constraint Icon SoImage Node
        SoSeparator *sep = dynamic_cast<SoSeparator *>(edit->constrGroup->getChild(constrId));
        SoImage *constraintIcon1 = dynamic_cast<SoImage *>(sep->getChild(index1));

        constraintIcon1->image.setValue(iconSize, 4, icondata.getValue(iconSize, nc));

        //Set Image Alignment to Center
        constraintIcon1->vertAlignment = SoImage::HALF;
        constraintIcon1->horAlignment = SoImage::CENTER;

        // If more than one icon per constraint
        if (index2 != -1) {
            SoImage *constraintIcon2 = dynamic_cast<SoImage *>(sep->getChild(index2));
            constraintIcon2->image.setValue(iconSize, 4, icondata.getValue(iconSize, nc));
            //Set Image Alignment to Center
            constraintIcon2->vertAlignment = SoImage::HALF;
            constraintIcon2->horAlignment = SoImage::CENTER;
        }
    }
}

float ViewProviderSketch::getScaleFactor()
{
    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    if (mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();
        return viewer->getCamera()->getViewVolume(viewer->getCamera()->aspectRatio.getValue()).getWorldToScreenScale(SbVec3f(0.f, 0.f, 0.f), 0.1f) / 3;
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
    for (std::vector<Sketcher::Constraint *>::const_iterator it=constrlist.begin(); it != constrlist.end(); ++it,i++) {
        // check if the type has changed
        if ((*it)->Type != edit->vConstrType[i]) {
            // clearing the type vector will force a rebuild of the visual nodes
            edit->vConstrType.clear();
            goto Restart;
        }
        // root separator for this constraint
        SoSeparator *sep = dynamic_cast<SoSeparator *>(edit->constrGroup->getChild(i));
        const Constraint *Constr = *it;

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

                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->abPos = SbVec3f(midpos.x, midpos.y, zConstr); //Absolute Reference

                    //Reference Position that is scaled according to zoom
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->translation = SbVec3f(relpos.x, relpos.y, 0);

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

                    if (Constr->FirstPos == Sketcher::none) {
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

                    } else {
                        if (temp)
                            midpos1 = edit->ActSketch.getPoint(Constr->First, Constr->FirstPos);
                        else
                            midpos1 = getSketchObject()->getPoint(Constr->First, Constr->FirstPos);
                        norm1 = Base::Vector3d(0,1,0);
                        dir1 = Base::Vector3d(1,0,0);
                    }

                    Base::Vector3d relpos1 = seekConstraintPosition(midpos1, norm1, dir1, 2.5, edit->constrGroup->getChild(i));
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr);
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                    if (Constr->FirstPos == Sketcher::none) {
                        Base::Vector3d relpos2 = seekConstraintPosition(midpos2, norm2, dir2, 2.5, edit->constrGroup->getChild(i));

                        Base::Vector3d secondPos = midpos2 - midpos1;
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr);
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);
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
                            double r1,r2,angle1,angle2;
                            if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo1);
                                r1 = circle->getRadius();
                                angle1 = M_PI/4;
                                midpos1 = circle->getCenter();
                            } else if (geo1->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo1);
                                r1 = arc->getRadius();
                                double startangle, endangle;
                                arc->getRange(startangle, endangle);
                                angle1 = (startangle + endangle)/2;
                                midpos1 = arc->getCenter();
                            } else
                                break;

                            if (geo2->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                                const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo2);
                                r2 = circle->getRadius();
                                angle2 = M_PI/4;
                                midpos2 = circle->getCenter();
                            } else if (geo2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo2);
                                r2 = arc->getRadius();
                                double startangle, endangle;
                                arc->getRange(startangle, endangle);
                                angle2 = (startangle + endangle)/2;
                                midpos2 = arc->getCenter();
                            } else
                                break;

                            norm1 = Base::Vector3d(cos(angle1),sin(angle1),0);
                            dir1 = Base::Vector3d(-norm1.y,norm1.x,0);
                            midpos1 += r1*norm1;

                            norm2 = Base::Vector3d(cos(angle2),sin(angle2),0);
                            dir2 = Base::Vector3d(-norm2.y,norm2.x,0);
                            midpos2 += r2*norm2;
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

                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr); //Absolute Reference

                    //Reference Position that is scaled according to zoom
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                    Base::Vector3d secondPos = midpos2 - midpos1;
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr); //Absolute Reference

                    //Reference Position that is scaled according to zoom
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->translation = SbVec3f(relpos2.x - relpos1.x, relpos2.y -relpos1.y, 0);

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

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(0));
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
                {
                    assert(Constr->First >= -extGeoCount && Constr->First < intGeoCount);
                    assert(Constr->Second >= -extGeoCount && Constr->Second < intGeoCount);

                    Base::Vector3d pos, relPos;
                    if (Constr->Type == PointOnObject) {
                        pos = edit->ActSketch.getPoint(Constr->First, Constr->FirstPos);
                        relPos = Base::Vector3d(0.f, 1.f, 0.f);
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->abPos = SbVec3f(pos.x, pos.y, zConstr); //Absolute Reference
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->translation = SbVec3f(relPos.x, relPos.y, 0);
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

                            dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr); //Absolute Reference

                            //Reference Position that is scaled according to zoom
                            dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->translation = SbVec3f(relpos1.x, relpos1.y, 0);

                            Base::Vector3d secondPos = midpos2 - midpos1;
                            dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr); //Absolute Reference

                            //Reference Position that is scaled according to zoom
                            dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->translation = SbVec3f(relpos2.x -relpos1.x, relpos2.y -relpos1.y, 0);

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
                                relPos = norm * 1;
                            }
                            else if (geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo2);
                                // tangency between a line and an arc
                                float length = (arc->getCenter() - lineSeg->getStartPoint())*dir;

                                pos = lineSeg->getStartPoint() + dir * length;
                                relPos = norm * 1;
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
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->abPos = SbVec3f(pos.x, pos.y, zConstr); //Absolute Reference
                        dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->translation = SbVec3f(relPos.x, relPos.y, 0);
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

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(0));
                    asciiText->datumtype    = SoDatumLabel::SYMMETRIC;

                    asciiText->pnts.setNum(2);
                    SbVec3f *verts = asciiText->pnts.startEditing();

                    verts[0] = p1;
                    verts[1] = p2;

                    asciiText->pnts.finishEditing();

                    dynamic_cast<SoTranslation *>(sep->getChild(1))->translation = (p1 + p2)/2;
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
                        const Part::Geometry *geo1 = GeoById(*geomlist, Constr->First);
                        const Part::Geometry *geo2 = GeoById(*geomlist, Constr->Second);
                        if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                            geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                            break;
                        const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment *>(geo1);
                        const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment *>(geo2);

                        bool flip1 = (Constr->FirstPos == end);
                        bool flip2 = (Constr->SecondPos == end);
                        Base::Vector3d dir1 = (flip1 ? -1. : 1.) * (lineSeg1->getEndPoint()-lineSeg1->getStartPoint());
                        Base::Vector3d dir2 = (flip2 ? -1. : 1.) * (lineSeg2->getEndPoint()-lineSeg2->getStartPoint());
                        Base::Vector3d pnt1 = flip1 ? lineSeg1->getEndPoint() : lineSeg1->getStartPoint();
                        Base::Vector3d pnt2 = flip2 ? lineSeg2->getEndPoint() : lineSeg2->getStartPoint();

                        // line-line intersection
                        {
                            double det = dir1.x*dir2.y - dir1.y*dir2.x;
                            if ((det > 0 ? det : -det) < 1e-10)
                                break;
                            double c1 = dir1.y*pnt1.x - dir1.x*pnt1.y;
                            double c2 = dir2.y*pnt2.x - dir2.x*pnt2.y;
                            double x = (dir1.x*c2 - dir2.x*c1)/det;
                            double y = (dir1.y*c2 - dir2.y*c1)/det;
                            p0 = SbVec3f(x,y,0);
                        }

                        startangle = atan2(dir1.y,dir1.x);
                        range = atan2(-dir1.y*dir2.x+dir1.x*dir2.y,
                                      dir1.x*dir2.x+dir1.y*dir2.y);
                        endangle = startangle + range;

                    } else if (Constr->First != Constraint::GeoUndef) {
                        const Part::Geometry *geo = GeoById(*geomlist, Constr->First);
                        if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId())
                            break;
                        const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(geo);

                        p0 = Base::convertTo<SbVec3f>((lineSeg->getEndPoint()+lineSeg->getStartPoint())/2);

                        Base::Vector3d dir = lineSeg->getEndPoint()-lineSeg->getStartPoint();
                        startangle = 0.;
                        range = atan2(dir.y,dir.x);
                        endangle = startangle + range;
                    } else
                        break;

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(0));
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
                            double startangle, endangle;
                            arc->getRange(startangle, endangle);
                            double angle = (startangle + endangle)/2;
                            pnt1 = arc->getCenter();
                            pnt2 = pnt1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                        }
                        else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo);
                            double radius = circle->getRadius();
                            double angle = (double) Constr->LabelPosition;
                            pnt1 = circle->getCenter();
                            pnt2 = pnt1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                        } else
                            break;
                    } else
                        break;

                    SbVec3f p1(pnt1.x,pnt1.y,zConstr);
                    SbVec3f p2(pnt2.x,pnt2.y,zConstr);

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(0));
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
        static_cast<Gui::View3DInventor *>(mdi)->getViewer()->render();
    }
}

void ViewProviderSketch::rebuildConstraintsVisual(void)
{
    const std::vector<Sketcher::Constraint *> &constrlist = getSketchObject()->Constraints.getValues();
    // clean up
    edit->constrGroup->removeAllChildren();
    edit->vConstrType.clear();

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
                text->size.setValue(17);
                text->useAntialiasing = false;
                SoAnnotation *anno = new SoAnnotation();
                anno->renderCaching = SoSeparator::OFF;
                anno->addChild(text);
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
                sep->addChild(mat);
                sep->addChild(new SoZoomTranslation()); // 1.
                sep->addChild(new SoImage());       // 2. constraint icon

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
                // Add new nodes to Constraint Seperator
                sep->addChild(mat);
                sep->addChild(new SoZoomTranslation()); // 1.
                sep->addChild(new SoImage());           // 2. first constraint icon
                sep->addChild(new SoZoomTranslation()); // 3.
                sep->addChild(new SoImage());           // 4. second constraint icon

                // remember the type of this constraint node
                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            case PointOnObject:
            case Tangent:
            {
                // Add new nodes to Constraint Seperator
                sep->addChild(mat);
                sep->addChild(new SoZoomTranslation()); // 1.
                sep->addChild(new SoImage());           // 2. constraint icon

                if ((*it)->Type == Tangent) {
                    const Part::Geometry *geo1 = getSketchObject()->getGeometry((*it)->First);
                    const Part::Geometry *geo2 = getSketchObject()->getGeometry((*it)->Second);
                    if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                        geo2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        sep->addChild(new SoZoomTranslation());
                    sep->addChild(new SoImage());   // 3. second constraint icon
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

                sep->addChild(arrows);              // 0.
                sep->addChild(new SoTranslation()); // 1.
                sep->addChild(new SoImage());       // 2. constraint icon

                edit->vConstrType.push_back((*it)->Type);
            }
            break;
            default:
                edit->vConstrType.push_back(None);
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
            Gui::Control().closeDialog();
        else
            return false;
    }

    // clear the selection (convenience)
    Gui::Selection().clearSelection();

    // create the container for the additional edit data
    assert(!edit);
    edit = new EditData();

    createEditInventorNodes();
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
    // constraints dimensions, icons and external geometry colors are hard coded
    // ConstrDimColor;
    // ConstrIcoColor;
    // CurveExternalColor;

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
        signalSetUp(QString::fromLatin1("<font color='red'>%1<br/>%2</font>")
                    .arg(tr("Over-constrained sketch"))
                    .arg(QString::fromStdString(msg)));
        signalSolved(QString());
    }
    else if (edit->ActSketch.hasConflicts()) { // conflicting constraints
        signalSetUp(QString::fromLatin1("<font color='red'>%1<br/>%2</font>")
                    .arg(tr("Sketch contains conflicting constraints"))
                    .arg(appendConflictMsg(edit->ActSketch.getConflicting())));
        signalSolved(QString());
    }
    else {
        if (edit->ActSketch.hasRedundancies()) { // redundant constraints
            signalSetUp(QString::fromLatin1("<font color='orange'>%1<br/>%2</font>")
                        .arg(tr("Sketch contains redundant constraints"))
                        .arg(appendRedundantMsg(edit->ActSketch.getRedundant())));
        }
        if (edit->ActSketch.solve() == 0) { // solving the sketch
            if (dofs == 0) {
                // color the sketch as fully constrained
                edit->FullyConstrained = true;
                if (!edit->ActSketch.hasRedundancies())
                    signalSetUp(QString::fromLatin1("<font color='green'>%1</font>").arg(tr("Fully constrained sketch")));
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
    pcRoot->addChild(edit->EditRoot);
    edit->EditRoot->renderCaching = SoSeparator::OFF ;

    // stuff for the points ++++++++++++++++++++++++++++++++++++++
    edit->PointsMaterials = new SoMaterial;
    edit->EditRoot->addChild(edit->PointsMaterials);

    SoMaterialBinding *MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_VERTEX;
    edit->EditRoot->addChild(MtlBind);

    edit->PointsCoordinate = new SoCoordinate3;
    edit->EditRoot->addChild(edit->PointsCoordinate);

    SoDrawStyle *DrawStyle = new SoDrawStyle;
    DrawStyle->pointSize = 8;
    edit->EditRoot->addChild(DrawStyle);
    edit->PointSet = new SoMarkerSet;
    edit->PointSet->markerIndex = SoMarkerSet::CIRCLE_FILLED_7_7;
    edit->EditRoot->addChild(edit->PointSet);

    // stuff for the Curves +++++++++++++++++++++++++++++++++++++++
    edit->CurvesMaterials = new SoMaterial;
    edit->EditRoot->addChild(edit->CurvesMaterials);

    MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_FACE;
    edit->EditRoot->addChild(MtlBind);

    edit->CurvesCoordinate = new SoCoordinate3;
    edit->EditRoot->addChild(edit->CurvesCoordinate);

    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 3;
    edit->EditRoot->addChild(DrawStyle);

    edit->CurveSet = new SoLineSet;

    edit->EditRoot->addChild(edit->CurveSet);

    // stuff for the RootCross lines +++++++++++++++++++++++++++++++++++++++
    MtlBind = new SoMaterialBinding;
    MtlBind->value = SoMaterialBinding::PER_FACE;
    edit->EditRoot->addChild(MtlBind);

    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 2;
    edit->EditRoot->addChild(DrawStyle);

    edit->RootCrossMaterials = new SoMaterial;
    edit->RootCrossMaterials->diffuseColor.set1Value(0,CrossColorH);
    edit->RootCrossMaterials->diffuseColor.set1Value(1,CrossColorV);
    edit->EditRoot->addChild(edit->RootCrossMaterials);

    edit->RootCrossCoordinate = new SoCoordinate3;
    edit->EditRoot->addChild(edit->RootCrossCoordinate);

    edit->RootCrossSet = new SoLineSet;
    edit->RootCrossSet->numVertices.set1Value(0,2);
    edit->RootCrossSet->numVertices.set1Value(1,2);
    edit->EditRoot->addChild(edit->RootCrossSet);

    // stuff for the EditCurves +++++++++++++++++++++++++++++++++++++++
    edit->EditCurvesMaterials = new SoMaterial;
    edit->EditRoot->addChild(edit->EditCurvesMaterials);

    edit->EditCurvesCoordinate = new SoCoordinate3;
    edit->EditRoot->addChild(edit->EditCurvesCoordinate);

    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 3;
    edit->EditRoot->addChild(DrawStyle);

    edit->EditCurveSet = new SoLineSet;
    edit->EditRoot->addChild(edit->EditCurveSet);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    float transparency;
    SbColor cursorTextColor(0,0,1);
    cursorTextColor.setPackedValue((uint32_t)hGrp->GetUnsigned("CursorTextColor", cursorTextColor.getPackedValue()), transparency);

    // stuff for the edit coordinates ++++++++++++++++++++++++++++++++++++++
    SoMaterial *CoordTextMaterials = new SoMaterial;
    CoordTextMaterials->diffuseColor = cursorTextColor;
    edit->EditRoot->addChild(CoordTextMaterials);

    SoSeparator *Coordsep = new SoSeparator();
    // no caching for fluctuand data structures
    Coordsep->renderCaching = SoSeparator::OFF;

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
    MtlBind->value = SoMaterialBinding::OVERALL ;
    edit->EditRoot->addChild(MtlBind);

    // use small line width for the Constraints
    DrawStyle = new SoDrawStyle;
    DrawStyle->lineWidth = 1;
    edit->EditRoot->addChild(DrawStyle);

    // add the group where all the constraints has its SoSeparator
    edit->constrGroup = new SoGroup();
    edit->EditRoot->addChild(edit->constrGroup);
}

void ViewProviderSketch::unsetEdit(int ModNum)
{
    ShowGrid.setValue(false);
    TightGrid.setValue(true);

    edit->EditRoot->removeAllChildren();
    pcRoot->removeChild(edit->EditRoot);

    if (edit->sketchHandler)
        purgeHandler();

    delete edit;
    edit = 0;

    this->show();

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
    viewer->setCameraOrientation(rot);

    viewer->setEditing(TRUE);
    SoNode* root = viewer->getSceneGraph();
    static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(FALSE);
}

void ViewProviderSketch::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
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

int ViewProviderSketch::getPreselectConstraint(void) const
{
    if (edit)
        return edit->PreselectConstraint;
    return -1;
}

Sketcher::SketchObject *ViewProviderSketch::getSketchObject(void) const
{
    return dynamic_cast<Sketcher::SketchObject *>(pcObject);
}

bool ViewProviderSketch::onDelete(const std::vector<std::string> &subList)
{
    if (edit) {
        std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();
        const std::vector<std::string> &SubNames = selection[0].getSubNames();

        Gui::Selection().clearSelection();
        resetPreselectPoint();
        edit->PreselectCurve = -1;
        edit->PreselectCross = -1;
        edit->PreselectConstraint = -1;

        std::set<int> delGeometries, delCoincidents, delConstraints;
        // go through the selected subelements
        for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
            if (it->size() > 4 && it->substr(0,4) == "Edge") {
                int GeoId = std::atoi(it->substr(4,4000).c_str());
                delGeometries.insert(GeoId);
            } else if (it->size() > 12 && it->substr(0,12) == "ExternalEdge") {
                int GeoId = std::atoi(it->substr(12,4000).c_str());
                GeoId = -GeoId - 3;
                delGeometries.insert(GeoId);
            } else if (it->size() > 6 && it->substr(0,6) == "Vertex") {
                int VtId = std::atoi(it->substr(6,4000).c_str());
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
                int ConstrId = std::atoi(it->substr(10,4000).c_str());
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
