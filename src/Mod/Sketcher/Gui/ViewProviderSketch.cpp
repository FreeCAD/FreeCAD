/***************************************************************************
 *   Copyright (c) 2009 J�rgen Riegel <juergen.riegel@web.de>              *
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
#endif

#include <Inventor/SbTime.h>

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
SbColor ViewProviderSketch::CrossColorV           (0.8f,0.4f,0.4f);     // #CC6666 -> (204,102,102)
SbColor ViewProviderSketch::CrossColorH           (0.4f,0.8f,0.4f);     // #66CC66 -> (102,204,102)
SbColor ViewProviderSketch::FullyConstrainedColor (0.0f,1.0f,0.0f);     // #00FF00 -> (  0,255,  0)
SbColor ViewProviderSketch::ConstrDimColor        (1.0f,0.149f,0.0f);   // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch::ConstrIcoColor        (1.0f,0.149f,0.0f);   // #FF2600 -> (255, 38,  0)
SbColor ViewProviderSketch::PreselectColor        (0.88f,0.88f,0.0f);   // #E1E100 -> (225,225,  0)
SbColor ViewProviderSketch::SelectColor           (0.11f,0.68f,0.11f);  // #1CAD1C -> ( 28,173, 28)

// Variables for holding previous click
SbTime  ViewProviderSketch::prvClickTime;
SbVec3f ViewProviderSketch::prvClickPoint;

//**************************************************************************
// Edit data structure

/// Data structure while edit the sketch
struct EditData {
    EditData():
    sketchHandler(0),
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
    CurveSet(0),
    PointSet(0)
    {}

    // pointer to the active handler for new sketch objects
    DrawSketchHandler *sketchHandler;

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

    // pointer to the Solver
    Sketcher::Sketch ActSketch;
    // container to track our own selected parts
    std::set<int> SelPointSet;
    std::set<int> SelCurvSet; // also holds cross axes at -1 and -2
    std::set<int> SelConstraintSet;

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
    SoLineSet     *EditCurveSet;
    SoLineSet     *RootCrossSet;
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
    delete(edit->sketchHandler);
    edit->sketchHandler = 0;
    Mode = STATUS_NONE;
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
            return false;
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

bool ViewProviderSketch::mouseButtonPressed(int Button, bool pressed, const SbVec3f &point,
                                            const SbVec3f &normal, const SoPickedPoint *pp)
{
    assert(edit);

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
                    // Double click events variables
                    SbTime tmp = (SbTime::getTimeOfDay() - prvClickTime);
                    float dci = (float) QApplication::doubleClickInterval()/1000.0f;
                    float length = (point - prvClickPoint).length();

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

                    if (done && length <  dblClickRadius && tmp.getValue() < dci) {
                        // Double Click Event Occured
                        editDoubleClicked();
                        // Reset Double Click Static Variables
                        prvClickTime = SbTime();
                        prvClickPoint = SbVec3f(0.0f, 0.0f, 0.0f);
                        Mode = STATUS_NONE;

                    } else {
                        prvClickTime = SbTime::getTimeOfDay();
                        prvClickPoint = point;
                    }

                    return done;
                }
                case STATUS_SKETCH_UseHandler:
                    return edit->sketchHandler->pressButton(Base::Vector2D(x,y));
                default:
                    return false;
            }
        }
        else {
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
                            Gui::View3DInventorViewer *viewer ;
                            viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

                            Gui::MenuItem *geom = new Gui::MenuItem();
                            geom->setCommand("Sketcher geoms");
                            *geom /*<< "Sketcher_CreatePoint"*/
                                << "Sketcher_CreateArc"
                                << "Sketcher_CreateCircle"
                                << "Sketcher_CreateLine"
                                << "Sketcher_CreatePolyline"
                                << "Sketcher_CreateRectangle"
                                << "Sketcher_CreateFillet"
                                << "Sketcher_Trimming"
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
        }
    }
}

bool ViewProviderSketch::mouseMove(const SbVec3f &point, const SbVec3f &normal, const SoPickedPoint *pp)
{
    if (!edit)
        return false;
    assert(edit);

    double x,y;
    getCoordsOnSketchPlane(x,y,point,normal);
    snapToGrid(x, y);

    bool preselectChanged;
    if (Mode!=STATUS_SKETCH_DragPoint && Mode!=STATUS_SKETCH_DragCurve &&
        Mode!=STATUS_SKETCH_DragConstraint) {
        int PtIndex,CurvIndex,ConstrIndex,CrossIndex;
        preselectChanged = detectPreselection(pp,PtIndex,CurvIndex,ConstrIndex,CrossIndex);
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
                edit->ActSketch.initMove(GeoId, PosId);
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
                edit->ActSketch.initMove(edit->DragCurve, Sketcher::none);
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
                    signalSolved(0, edit->ActSketch.SolveTime);
                } else {
                    signalSolved(1, edit->ActSketch.SolveTime);
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
                    signalSolved(0, edit->ActSketch.SolveTime);
                } else {
                    signalSolved(1, edit->ActSketch.SolveTime);
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

    if (Constr->Type == Distance || Constr->Type == DistanceX || Constr->Type == DistanceY ||
        Constr->Type == Radius) {

        int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;
        int extGeoCount = getSketchObject()->getExternalGeometryCount();
        const std::vector<Part::Geometry *> geomlist = edit->ActSketch.extractGeometry(true, true);

        assert(int(geomlist.size()) == extGeoCount + intGeoCount);
        assert((Constr->First >= -extGeoCount && Constr->First < intGeoCount)
               || Constr->First != Constraint::GeoUndef);

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
                double angle = M_PI/4;
                p1 = circle->getCenter();
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

        if (Constr->Type == Radius)
            Constr->LabelDistance = vec.x * dir.x + vec.y * dir.y;
        else {
            Base::Vector3d norm(-dir.y,dir.x,0);
            Constr->LabelDistance = vec.x * norm.x + vec.y * norm.y;
        }
    }
    else if (Constr->Type == Angle) {
        int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;
        int extGeoCount = getSketchObject()->getExternalGeometryCount();
        const std::vector<Part::Geometry *> geomlist = edit->ActSketch.extractGeometry(true, true);

        assert(int(geomlist.size()) == extGeoCount + intGeoCount);
        assert((Constr->First >= -extGeoCount && Constr->First < intGeoCount)
               || Constr->First != Constraint::GeoUndef);

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
    draw(true);
}

bool ViewProviderSketch::isConstraintAtPosition(const Base::Vector3d &constrPos, const SoNode *constraint)
{
    assert(edit);
    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

    SoRayPickAction rp(viewer->getViewportRegion());
    rp.setRadius(0.1f);

    rp.setRay(SbVec3f(constrPos.x, constrPos.y,constrPos.z), SbVec3f(0, 0, 1) );
    //problem
    rp.apply(edit->constrGroup); // We could narrow it down to just the SoGroup containing the constraints

    // returns a copy of the point
    SoPickedPoint *pp = rp.getPickedPoint();

    if (pp) {
        SoPath *path = pp->getPath();
        int length = path->getLength();
        SoNode *tailFather = path->getNode(length-2);
        SoNode *tailFather2 = path->getNode(length-3);

        // checking if a constraint is the same as the one selected
        if (tailFather2 == constraint || tailFather == constraint) {
            return false;
        } else {
            return true;
        }
    } else {
        return false;
    }
}

Base::Vector3d ViewProviderSketch::seekConstraintPosition(const Base::Vector3d &suggestedPos,
                                                          const Base::Vector3d &dir, float step,
                                                          const SoNode *constraint)
{
    int multiplier = 0;
    Base::Vector3d freePos;
    do {
        // Calculate new position of constraint
        freePos = suggestedPos + (dir * (multiplier * step));
        multiplier++; // Increment the multiplier
    }
    while (isConstraintAtPosition(freePos, constraint));
    return freePos;
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

bool ViewProviderSketch::detectPreselection(const SoPickedPoint *Point, int &PtIndex, int &CurvIndex, int &ConstrIndex, int &CrossIndex)
{
    assert(edit);

    PtIndex = -1;
    CurvIndex = -1; // valid values are 0,1,2,... for normal geometry and -3,-4,-5,... for external geometry
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
            }
        } else {
            // checking for a hit in the curves
            if (tail == edit->CurveSet) {
                const SoDetail *curve_detail = Point->getDetail(edit->CurveSet);
                if (curve_detail && curve_detail->getTypeId() == SoLineDetail::getClassTypeId()) {
                    // get the index
                    CurvIndex = static_cast<const SoLineDetail *>(curve_detail)->getLineIndex();
                    int maxGeoId = getSketchObject()->getHighestCurveIndex();
                    if (CurvIndex > maxGeoId) { // hit on external geometry
                        int extGeoCount = getSketchObject()->getExternalGeometryCount();
                        CurvIndex = -extGeoCount + (CurvIndex - maxGeoId - 1);
                    }
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
        } else if (CurvIndex != -1 && CurvIndex != edit->PreselectCurve) {  // if a new curve is hit
            std::stringstream ss;
            if (CurvIndex >= 0)
                ss << "Edge" << CurvIndex;
            else // external geometry
                ss << "ExternalEdge" << -CurvIndex - 3; // convert index start from -3 to 0
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
                edit->PreselectCurve = CurvIndex;
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
        } else if ((PtIndex == -1 && CurvIndex == -1 && CrossIndex == -1 && ConstrIndex == -1) &&
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
    for (int  i=0; i < PtNum; i++) {
        if (edit->SelPointSet.find(i) != edit->SelPointSet.end())
            pcolor[i] = SelectColor;
        else if (edit->PreselectPoint == i)
            pcolor[i] = PreselectColor;
        else if (edit->FullyConstrained)
            pcolor[i] = FullyConstrainedColor;
        else
            pcolor[i] = VertexColor;
    }

    // colors of the curves
    int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;
    int extGeoCount = getSketchObject()->getExternalGeometryCount();
    for (int  i=0; i < CurvNum; i++) {
        int GeoId = (i < intGeoCount) ? i : -(extGeoCount - (i - intGeoCount));
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
        SoMaterial *m = dynamic_cast<SoMaterial *>(s->getChild(0));

        // Check Constraint Type
        ConstraintType type = getSketchObject()->Constraints.getValues()[i]->Type;
        bool hasDatumLabel  = (type == Sketcher::Angle ||
                              type == Sketcher::Radius ||
                              type == Sketcher::Distance || type == Sketcher::DistanceX || type == Sketcher::DistanceY);

        if (edit->SelConstraintSet.find(i) != edit->SelConstraintSet.end()) {
            m->diffuseColor = SelectColor;
            if (hasDatumLabel) {
                SoDatumLabel *l = dynamic_cast<SoDatumLabel *>(s->getChild(4));
                l->textColor = SelectColor;
            }
        } else if (edit->PreselectConstraint == i) {
            m->diffuseColor = PreselectColor;
            if (hasDatumLabel) {
                SoDatumLabel *l = dynamic_cast<SoDatumLabel *>(s->getChild(4));
                l->textColor = PreselectColor;
            }
        }
        else {
            m->diffuseColor = ConstrDimColor;
            if (hasDatumLabel) {
                SoDatumLabel *l = dynamic_cast<SoDatumLabel *>(s->getChild(4));
                l->textColor = ConstrDimColor;
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
            index2 = 4;
            break;
        case Equal:
            icoType = QString::fromAscii("small/Constraint_EqualLength_sm");
            index2 = 4;
            break;
        case Symmetric:
            icoType = QString::fromAscii("small/Constraint_Symmetric_sm");
            index1 = 3;
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

    for (std::vector<Part::Geometry *>::const_iterator it = geomlist->begin(); it != geomlist->end()-2; ++it) {
        if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) { // add a line
            const Part::GeomLineSegment *lineSeg = dynamic_cast<const Part::GeomLineSegment *>(*it);
            // create the definition struct for that geom
            Coords.push_back(lineSeg->getStartPoint());
            Coords.push_back(lineSeg->getEndPoint());
            Points.push_back(lineSeg->getStartPoint());
            Points.push_back(lineSeg->getEndPoint());
            Index.push_back(2);
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
            Points.push_back(center);
            Points.push_back(start);
            Points.push_back(end);
        }
        else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) { // add a circle
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
        }
        else {
            ;
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

                    float scale = dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->getScaleFactor();
                    Base::Vector3d constrPos = midpos + (norm * 2.5 * scale);

                    constrPos = seekConstraintPosition(constrPos, dir, 2.5 * scale, edit->constrGroup->getChild(i));

                    // Translate the Icon based on calculated position
                    Base::Vector3d relPos = constrPos - midpos; // Relative Position of Icons to Midpoint
                    relPos = relPos / scale; // Must Divide by Scale Factor

                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->abPos = SbVec3f(midpos.x, midpos.y, zConstr); //Absolute Reference

                    //Reference Position that is scaled according to zoom
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->translation = SbVec3f(relPos.x, relPos.y, 0);

                }
                break;
            case Parallel:
            case Perpendicular:
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
                        } else // Parallel or Perpendicular can only apply to a GeomLineSegment
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

                    // Get Current Scale Factor
                    float scale = dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->getScaleFactor();

                    Base::Vector3d constrPos1 = midpos1 + (norm1 * scale * 2.5 );
                    constrPos1 = seekConstraintPosition(constrPos1, dir1, scale * 2.5, edit->constrGroup->getChild(i));

                    Base::Vector3d constrPos2 = midpos2 + (norm2 * scale * 2.5);
                    constrPos2 = seekConstraintPosition(constrPos2, dir2, scale * 2.5, edit->constrGroup->getChild(i));

                    // Translate the Icon based on calculated position
                    Base::Vector3d relPos1 = constrPos1 - midpos1 ; // Relative Position of Icons to Midpoint1
                    Base::Vector3d relPos2 = constrPos2 - midpos2 ; // Relative Position of Icons to Midpoint2

                    relPos1 = relPos1 / scale;
                    relPos2 = relPos2 / scale;

                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr); //Absolute Reference

                    //Reference Position that is scaled according to zoom
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->translation = SbVec3f(relPos1.x, relPos1.y, 0);

                    Base::Vector3d secondPos = midpos2 - midpos1;
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr); //Absolute Reference

                    //Reference Position that is scaled according to zoom
                    dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->translation = SbVec3f(relPos2.x -relPos1.x, relPos2.y -relPos1.y, 0);

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

                    SbVec3f p1(pnt1.x,pnt1.y,zConstr);
                    SbVec3f p2(pnt2.x,pnt2.y,zConstr);

                    SbVec3f dir, norm;
                    if (Constr->Type == Distance)
                        dir = (p2-p1);
                    else if (Constr->Type == DistanceX)
                        dir = SbVec3f( (pnt2.x - pnt1.x >= FLT_EPSILON) ? 1 : -1, 0, 0);
                    else if (Constr->Type == DistanceY)
                        dir = SbVec3f(0, (pnt2.y - pnt1.y >= FLT_EPSILON) ? 1 : -1, 0);
                    dir.normalize();
                    norm = SbVec3f (-dir[1],dir[0],0);

                    // when the datum line is not parallel to p1-p2 the projection of
                    // p1-p2 on norm is not zero, p2 is considered as reference and p1
                    // is replaced by its projection p1_
                    float normproj12 = (p2-p1).dot(norm);

                    SbVec3f p1_ = p1 + normproj12 * norm;
                    SbVec3f midpos = (p1_ + p2)/2;

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(4));
                    if ((Constr->Type == DistanceX || Constr->Type == DistanceY) &&
                        Constr->FirstPos != Sketcher::none && Constr->Second == Constraint::GeoUndef)
                        // display negative sign for absolute coordinates
                        asciiText->string = SbString().sprintf("%.2f",Constr->Value);
                    else // hide negative sign
                        asciiText->string = SbString().sprintf("%.2f",std::abs(Constr->Value));

                    // Get Bounding box dimensions for Datum text
                    Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

                    // [FIX ME] Its an attempt to find the height of the text using the bounding box, but is in correct.
                    SoGetBoundingBoxAction bbAction(viewer->getViewportRegion());
                    bbAction.apply(sep->getChild(4));

                    float bx,by,bz;
                    bbAction.getBoundingBox().getSize(bx,by,bz);
                    SbVec3f textBB(bx,by,bz);
                    // bbAction.setCenter(, true)
                    // This is the bounding box containing the width and height of text

                    SbVec3f textBBCenter = bbAction.getBoundingBox().getCenter();

                    float length = Constr->LabelDistance;

                    // Get magnitude of angle between horizontal
                    double angle = atan2f(dir[1],dir[0]);
                    bool flip=false;
                    if (angle > M_PI_2+M_PI/12) {
                        angle -= M_PI;
                        flip = true;
                    } else if (angle <= -M_PI_2+M_PI/12) {
                        angle += M_PI;
                        flip = true;
                    }

                    SbVec3f textpos = midpos + norm * (length + ( (flip ? 1:-1) * textBBCenter[1] / 4));

                    // set position and rotation of Datums Text
                    SoTransform *transform = dynamic_cast<SoTransform *>(sep->getChild(2));
                    transform->rotation.setValue(SbVec3f(0, 0, 1), (float)angle);
                    transform->translation.setValue(textpos);

                    // Get the datum nodes
                    SoSeparator *sepDatum = dynamic_cast<SoSeparator *>(sep->getChild(1));
                    SoCoordinate3 *datumCord = dynamic_cast<SoCoordinate3 *>(sepDatum->getChild(0));

                    // [Fixme] This should be made neater - compute the vertical datum line length
                    float offset1 = (length + normproj12 < 0) ? -0.5  : 0.5;
                    float offset2 = (length < 0) ? -0.5  : 0.5;
                    offset1 *= getScaleFactor();
                    offset2 *= getScaleFactor();
                    // Calculate coordinates for perpendicular datum lines
                    datumCord->point.set1Value(0,p1);
                    datumCord->point.set1Value(1,p1_ + norm * (length + offset1));
                    datumCord->point.set1Value(2,p2);
                    datumCord->point.set1Value(3,p2  + norm * (length + offset2));

                    // Calculate the coordinates for the parallel datum lines
                    datumCord->point.set1Value(4,p1_    + norm * length);
                    datumCord->point.set1Value(5,midpos + norm * length - dir * (textBB[0]/1.7f) );
                    datumCord->point.set1Value(6,midpos + norm * length + dir * (textBB[0]/1.7f) );
                    datumCord->point.set1Value(7,p2     + norm * length);

                    // Use the coordinates calculated earlier to the lineset
                    SoLineSet *datumLineSet = dynamic_cast<SoLineSet *>(sepDatum->getChild(1));
                    datumLineSet->numVertices.set1Value(0,2);
                    datumLineSet->numVertices.set1Value(1,2);
                    datumLineSet->numVertices.set1Value(2,2);
                    datumLineSet->numVertices.set1Value(3,2);
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

                            // Get Current Scale Factor
                            float scale = dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->getScaleFactor();

                            Base::Vector3d constrPos1 = midpos1 + (norm1 * scale * 2.5);
                            constrPos1 = seekConstraintPosition(constrPos1, dir1, scale * 2.5, edit->constrGroup->getChild(i));

                            Base::Vector3d constrPos2 = midpos2 + (norm2 * scale * 2.5);
                            constrPos2 = seekConstraintPosition(constrPos2, dir2, scale * 2.5, edit->constrGroup->getChild(i));

                            // Translate the Icon based on calculated position
                            Base::Vector3d relPos1 = constrPos1 - midpos1 ; // Relative Position of Icons to Midpoint1
                            Base::Vector3d relPos2 = constrPos2 - midpos2 ; // Relative Position of Icons to Midpoint2

                            relPos1 = relPos1 / scale;
                            relPos2 = relPos2 / scale;

                            dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->abPos = SbVec3f(midpos1.x, midpos1.y, zConstr); //Absolute Reference

                            //Reference Position that is scaled according to zoom
                            dynamic_cast<SoZoomTranslation *>(sep->getChild(1))->translation = SbVec3f(relPos1.x, relPos1.y, 0);

                            Base::Vector3d secondPos = midpos2 - midpos1;
                            dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->abPos = SbVec3f(secondPos.x, secondPos.y, zConstr); //Absolute Reference

                            //Reference Position that is scaled according to zoom
                            dynamic_cast<SoZoomTranslation *>(sep->getChild(3))->translation = SbVec3f(relPos2.x -relPos1.x, relPos2.y -relPos1.y, 0);

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
                        }
                        else if (geo2->getTypeId()== Part::GeomCircle::getClassTypeId()) {
                            std::swap(geo1,geo2);
                        }

                        if (geo1->getTypeId()== Part::GeomCircle::getClassTypeId()) {
                            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geo1);
                            if (geo2->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geo2);
                                // tangency between a circle and an arc
                            }
                        }

                        if (geo1->getTypeId()== Part::GeomArcOfCircle::getClassTypeId() &&
                            geo1->getTypeId()== Part::GeomArcOfCircle::getClassTypeId()) {
                            const Part::GeomArcOfCircle *arc1 = dynamic_cast<const Part::GeomArcOfCircle *>(geo1);
                            const Part::GeomArcOfCircle *arc2 = dynamic_cast<const Part::GeomArcOfCircle *>(geo2);
                            // tangency between two arcs
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

                    // Get the datum nodes
                    SoSeparator *sepArrows = dynamic_cast<SoSeparator *>(sep->getChild(1));
                    SoCoordinate3 *arrowsCord = dynamic_cast<SoCoordinate3 *>(sepArrows->getChild(0));

                    arrowsCord->point.setNum(10);

                    // Calculate coordinates for the first arrow
                    arrowsCord->point.set1Value(0,p1);
                    arrowsCord->point.set1Value(1,p1 + dir * 5);
                    arrowsCord->point.set1Value(2,p1 + dir * 3 + norm * 2);
                    arrowsCord->point.set1Value(3,p1 + dir * 5);
                    arrowsCord->point.set1Value(4,p1 + dir * 3 - norm * 2);

                    // Calculate coordinates for the second arrow
                    arrowsCord->point.set1Value(5,p2);
                    arrowsCord->point.set1Value(6,p2 - dir * 5);
                    arrowsCord->point.set1Value(7,p2 - dir * 3 + norm * 2);
                    arrowsCord->point.set1Value(8,p2 - dir * 5);
                    arrowsCord->point.set1Value(9,p2 - dir * 3 - norm * 2);

                    // Use the coordinates calculated earlier to the lineset
                    SoLineSet *arrowsLineSet = dynamic_cast<SoLineSet *>(sepArrows->getChild(1));
                    arrowsLineSet->numVertices.set1Value(0,3);
                    arrowsLineSet->numVertices.set1Value(1,2);
                    arrowsLineSet->numVertices.set1Value(2,3);
                    arrowsLineSet->numVertices.set1Value(3,2);

                    dynamic_cast<SoTranslation *>(sep->getChild(2))->translation = (p1 + p2)/2;
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

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(4));
                    asciiText->string = SbString().sprintf("%.2f",Base::toDegrees<double>(std::abs(Constr->Value)));

                    // Get Bounding box dimensions for Datum text
                    Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

                    // [FIX ME] Its an attempt to find the height of the text using the bounding box, but is in correct.
                    SoGetBoundingBoxAction bbAction(viewer->getViewportRegion());
                    bbAction.apply(sep->getChild(4));

                    float bx,by,bz;
                    bbAction.getBoundingBox().getSize(bx,by,bz);
                    SbVec3f textBB(bx,by,bz);

                    SbVec3f textBBCenter = bbAction.getBoundingBox().getCenter();

                    float length = Constr->LabelDistance;
                    float r = 2*length;

                    SbVec3f v0(cos(startangle+range/2),sin(startangle+range/2),0);
                    SbVec3f textpos = p0 + v0 * r - SbVec3f(0,1,0) * textBBCenter[1]/4;

                    // leave some space for the text
                    if (range >= 0)
                        range = std::max(0.2*range, range - textBB[0]/(2*r));
                    else
                        range = std::min(0.2*range, range + textBB[0]/(2*r));

                    int countSegments = std::max(6, abs(int(50.0 * range / (2 * M_PI))));
                    double segment = range / (2*countSegments-2);

                    // set position and rotation of Datums Text
                    SoTransform *transform = dynamic_cast<SoTransform *>(sep->getChild(2));
                    transform->translation.setValue(textpos);

                    // Get the datum nodes
                    SoSeparator *sepDatum = dynamic_cast<SoSeparator *>(sep->getChild(1));
                    SoCoordinate3 *datumCord = dynamic_cast<SoCoordinate3 *>(sepDatum->getChild(0));

                    datumCord->point.setNum(2*countSegments+4);
                    int i=0;
                    int j=2*countSegments-1;
                    for (; i < countSegments; i++, j--) {
                        double angle = startangle + segment*i;
                        datumCord->point.set1Value(i,p0+SbVec3f(r*cos(angle),r*sin(angle),0));
                        angle = endangle - segment*i;
                        datumCord->point.set1Value(j,p0+SbVec3f(r*cos(angle),r*sin(angle),0));
                    }
                    SbVec3f v1(cos(startangle),sin(startangle),0);
                    SbVec3f v2(cos(endangle),sin(endangle),0);
                    float sf = getScaleFactor();
                    datumCord->point.set1Value(2*countSegments  ,p0+(r-0.5f * sf)*v1);
                    datumCord->point.set1Value(2*countSegments+1,p0+(r+0.5f * sf)*v1);
                    datumCord->point.set1Value(2*countSegments+2,p0+(r-0.5f * sf)*v2);
                    datumCord->point.set1Value(2*countSegments+3,p0+(r+0.5f * sf)*v2);

                    // Use the coordinates calculated earlier to the lineset
                    SoLineSet *datumLineSet = dynamic_cast<SoLineSet *>(sepDatum->getChild(1));
                    datumLineSet->numVertices.set1Value(0,countSegments);
                    datumLineSet->numVertices.set1Value(1,countSegments);
                    datumLineSet->numVertices.set1Value(2,2);
                    datumLineSet->numVertices.set1Value(3,2);
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
                            double angle = M_PI/4;
                            pnt1 = circle->getCenter();
                            pnt2 = pnt1 + radius * Base::Vector3d(cos(angle),sin(angle),0.);
                        } else
                            break;
                    } else
                        break;

                    SbVec3f p1(pnt1.x,pnt1.y,zConstr);
                    SbVec3f p2(pnt2.x,pnt2.y,zConstr);

                    SbVec3f dir = (p2-p1);
                    dir.normalize();
                    SbVec3f norm (-dir[1],dir[0],0);

                    float length = Constr->LabelDistance;
                    SbVec3f pos = p2 + length*dir;

                    SoDatumLabel *asciiText = dynamic_cast<SoDatumLabel *>(sep->getChild(4));
                    asciiText->string = SbString().sprintf("%.2f",Constr->Value);

                    // Get Bounding box dimensions for Datum text
                    Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
                    Gui::View3DInventorViewer *viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

                    // [FIX ME] Its an attempt to find the height of the text using the bounding box, but is in correct.
                    SoGetBoundingBoxAction bbAction(viewer->getViewportRegion());
                    bbAction.apply(sep->getChild(3));

                    float bx=0,by=0,bz=0;
                    SbBox3f bbox = bbAction.getBoundingBox();
                    if (!bbox.isEmpty())
                        bbox.getSize(bx,by,bz);
                    SbVec3f textBB(bx,by,bz);

                    SbVec3f textBBCenter = bbAction.getBoundingBox().getCenter();

                    // Get magnitude of angle between horizontal
                    double angle = atan2f(dir[1],dir[0]);
                    bool flip=false;
                    if (angle > M_PI_2+M_PI/12) {
                        angle -= M_PI;
                        flip = true;
                    } else if (angle <= -M_PI_2+M_PI/12) {
                        angle += M_PI;
                        flip = true;
                    }

                    SbVec3f textpos = pos + norm * ( (flip ? 1:-1) * textBBCenter[1] / 1.7f);

                    // set position and rotation of Datums Text
                    SoTransform *transform = dynamic_cast<SoTransform *>(sep->getChild(2));
                    transform->rotation.setValue(SbVec3f(0, 0, 1), (float)angle);
                    transform->translation.setValue(textpos);

                    // Get the datum nodes
                    SoSeparator *sepDatum = dynamic_cast<SoSeparator *>(sep->getChild(1));
                    SoCoordinate3 *datumCord = dynamic_cast<SoCoordinate3 *>(sepDatum->getChild(0));

                    SbVec3f p3 = pos + dir * (6+textBB[0]/1.7f);
                    if ((p3-p1).length() > (p2-p1).length())
                        p2 = p3;

                    // Calculate the coordinates for the parallel datum lines
                    datumCord->point.set1Value(0,p1);
                    datumCord->point.set1Value(1,pos - dir * (1+textBB[0]/1.7f) );
                    datumCord->point.set1Value(2,pos + dir * (1+textBB[0]/1.7f) );
                    datumCord->point.set1Value(3,p2);

                    // Use the coordinates calculated earlier to the lineset
                    SoLineSet *datumLineSet = dynamic_cast<SoLineSet *>(sepDatum->getChild(1));
                    datumLineSet->numVertices.set1Value(0,2);
                    datumLineSet->numVertices.set1Value(1,2);
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
        // no caching for fluctuand data structures
        sep->renderCaching = SoSeparator::OFF;
        // every constrained visual node gets its own material for preselection and selection
        SoMaterial *Material = new SoMaterial;
        Material->diffuseColor = ConstrDimColor;
        sep->addChild(Material);

        // distinguish different constraint types to build up
        switch ((*it)->Type) {
            case Distance:
            case DistanceX:
            case DistanceY:
            case Radius:
            case Angle:
                {
                    SoSeparator *sepDatum = new SoSeparator();
                    sepDatum->addChild(new SoCoordinate3());
                    SoLineSet *lineSet = new SoLineSet;
                    sepDatum->addChild(lineSet);

                    sep->addChild(sepDatum);

                    // Add the datum text
                    sep->addChild(new SoTransform());

                    // add font for the datum text
                    SoFont *font = new SoFont();
                    font->size = 8.f;
                    font->name = "FreeSans:bold, Helvetica, Arial, FreeSans:bold";

                    sep->addChild(font);

                    SoDatumLabel *text = new SoDatumLabel();
                    //text->justification =  SoDatumLabel::CENTER;
                    text->string = "";
                    text->textColor = ConstrDimColor;
                    sep->addChild(text);

                    edit->vConstrType.push_back((*it)->Type);
                }
                break;
            case Horizontal:
            case Vertical:
                {
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
                    SoSeparator *sepArrows = new SoSeparator();
                    sepArrows->addChild(new SoCoordinate3());
                    SoLineSet *lineSet = new SoLineSet;
                    sepArrows->addChild(lineSet);
                    sep->addChild(sepArrows);           // 1.

                    // Add new nodes to Constraint Seperator
                    sep->addChild(new SoTranslation()); // 2.
                    sep->addChild(new SoImage());       // 3. constraint icon

                    edit->vConstrType.push_back((*it)->Type);
                }
                break;
            default:
                edit->vConstrType.push_back(None);
            }

        edit->constrGroup->addChild(sep);
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

    if (edit && (prop == &(getSketchObject()->Geometry) || &(getSketchObject()->Constraints))) {
        edit->FullyConstrained = false;
        int dofs = edit->ActSketch.setUpSketch(getSketchObject()->getCompleteGeometry(),
                                               getSketchObject()->Constraints.getValues(),
                                               true, getSketchObject()->getExternalGeometryCount());
        std::string msg;
        if (getSketchObject()->Geometry.getSize() == 0) {
            signalSetUp(-1, 0, msg);
            signalSolved(-1, 0);
        }
        else if (dofs < 0) { // over-constrained sketch
            SketchObject::appendConflictMsg(edit->ActSketch.getConflicting(), msg);
            //Base::Console().Warning("Over-constrained sketch\n%s",msg.c_str());
            signalSetUp(3, 0, msg);
            signalSolved(-1,0);
        }
        else if (edit->ActSketch.hasConflicts()) { // conflicting constraints
            SketchObject::appendConflictMsg(edit->ActSketch.getConflicting(), msg);
            //Base::Console().Warning("Sketch with conflicting constraints\n%s",msg.c_str());
            signalSetUp(2, dofs, msg);
            signalSolved(-1,0);
        }
        else if (edit->ActSketch.solve() == 0) { // solving the sketch
            if (dofs == 0) {
                // color the sketch as fully constrained
                edit->FullyConstrained = true;
                //Base::Console().Message("Fully constrained sketch\n");
                signalSetUp(0, 0, msg);
            }
            else {
                //Base::Console().Message("Under-constrained sketch with %d degrees of freedom\n", dofs);
                signalSetUp(1, dofs, msg);
            }
            signalSolved(0,edit->ActSketch.SolveTime);
        }
        else {
            signalSolved(1,edit->ActSketch.SolveTime);
        }
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

    // create the container for the addtitional edit data
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

    // set up the sketch and diagnose possible conflicts
    int dofs = edit->ActSketch.setUpSketch(getSketchObject()->getCompleteGeometry(),
                                           getSketchObject()->Constraints.getValues(),
                                           true, getSketchObject()->getExternalGeometryCount());
    std::string msg;
    if (getSketchObject()->Geometry.getSize() == 0) {
        signalSetUp(-1, 0, msg);
        signalSolved(-1, 0);
    }
    else if (dofs < 0) { // over-constrained sketch
        SketchObject::appendConflictMsg(edit->ActSketch.getConflicting(), msg);
        //Base::Console().Warning("Over-constrained sketch\n%s",msg.c_str());
        signalSetUp(3, 0, msg);
        signalSolved(-1, 0);
    }
    else if (edit->ActSketch.hasConflicts()) { // conflicting constraints
        SketchObject::appendConflictMsg(edit->ActSketch.getConflicting(), msg);
        //Base::Console().Warning("Sketch with conflicting constraints\n%s",msg.c_str());
        signalSetUp(2, dofs, msg);
        signalSolved(-1, 0);
    }
    else if (edit->ActSketch.solve() == 0) { // solving the sketch
        if (dofs == 0) {
            // color the sketch as fully constrained
            edit->FullyConstrained = true;
            //Base::Console().Message("Fully constrained sketch\n");
            signalSetUp(0, 0, msg);
        }
        else {
            //Base::Console().Message("Under-constrained sketch with %d degrees of freedom\n", dofs);
            signalSetUp(1, dofs, msg);
        }
        signalSolved(0, edit->ActSketch.SolveTime);
    }
    else {
        signalSolved(1, edit->ActSketch.SolveTime);
    }

    draw();

    return true;
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

    // stuff for the edit coordinates ++++++++++++++++++++++++++++++++++++++
    SoMaterial *EditMaterials = new SoMaterial;
    EditMaterials->diffuseColor = SbColor(0,0,1);
    edit->EditRoot->addChild(EditMaterials);

    SoSeparator *Coordsep = new SoSeparator();
    // no caching for fluctuand data structures
    Coordsep->renderCaching = SoSeparator::OFF;

    SoFont *font = new SoFont();
    font->size = 15.0;
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

    // add font for the text shown constraints
    font = new SoFont();
    font->size = 8.0;
    edit->EditRoot->addChild(font);

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

    if (edit->sketchHandler) {
        edit->sketchHandler->unsetCursor();
        purgeHandler();
    }
    delete edit;
    edit = 0;

    this->show();

    // and update the sketch
    getSketchObject()->getDocument()->recompute();

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

void ViewProviderSketch::setPositionText(const Base::Vector2D &Pos)
{
    char buf[40];
    sprintf( buf, " (%.1f,%.1f)", Pos.fX,Pos.fY );
    edit->textX->string = buf;
    edit->textPos->translation = SbVec3f(Pos.fX,Pos.fY,zText);
}

void ViewProviderSketch::resetPositionText(void)
{
    edit->textX->string = "";
}

void ViewProviderSketch::setPreselectPoint(int PreselectPoint)
{
    if (edit) {
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        float x,y,z;
        if (edit->PreselectPoint != -1 &&
            edit->SelPointSet.find(edit->PreselectPoint) == edit->SelPointSet.end()) {
            // send to background
            pverts[edit->PreselectPoint].getValue(x,y,z);
            pverts[edit->PreselectPoint].setValue(x,y,zPoints);
        }
        // bring to foreground
        pverts[PreselectPoint].getValue(x,y,z);
        pverts[PreselectPoint].setValue(x,y,zHighlight);
        edit->PreselectPoint = PreselectPoint;
        edit->PointsCoordinate->point.finishEditing();
    }
}

void ViewProviderSketch::resetPreselectPoint(void)
{
    if (edit) {
        if (edit->PreselectPoint != -1 &&
            edit->SelPointSet.find(edit->PreselectPoint) == edit->SelPointSet.end()) {
            // send to background
            SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
            float x,y,z;
            pverts[edit->PreselectPoint].getValue(x,y,z);
            pverts[edit->PreselectPoint].setValue(x,y,zPoints);
            edit->PointsCoordinate->point.finishEditing();
        }
        edit->PreselectPoint = -1;
    }
}

void ViewProviderSketch::addSelectPoint(int SelectPoint)
{
    if (edit) {
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        // bring to foreground
        float x,y,z;
        pverts[SelectPoint].getValue(x,y,z);
        pverts[SelectPoint].setValue(x,y,zHighlight);
        edit->SelPointSet.insert(SelectPoint);
        edit->PointsCoordinate->point.finishEditing();
    }
}

void ViewProviderSketch::removeSelectPoint(int SelectPoint)
{
    if (edit) {
        SbVec3f *pverts = edit->PointsCoordinate->point.startEditing();
        // send to background
        float x,y,z;
        pverts[SelectPoint].getValue(x,y,z);
        pverts[SelectPoint].setValue(x,y,zPoints);
        edit->SelPointSet.erase(SelectPoint);
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
    //FIXME use the selection subelements instead of the Sel Sets...
    if (edit) {
        // We must tmp. block the signaling because otherwise we empty the sets while
        // looping through them which may cause a crash
        this->blockConnection(true);
        std::set<int>::const_reverse_iterator rit;
        for (rit = edit->SelConstraintSet.rbegin(); rit != edit->SelConstraintSet.rend(); rit++) {
            try {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.delConstraint(%i)"
                                       ,getObject()->getNameInDocument(), *rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
            }
        }

        for (rit = edit->SelCurvSet.rbegin(); rit != edit->SelCurvSet.rend(); rit++) {
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
        for (rit = edit->SelPointSet.rbegin(); rit != edit->SelPointSet.rend(); rit++) {
            try {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.delConstraintOnPoint(%i)"
                                       ,getObject()->getNameInDocument(), *rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
            }
        }

        this->blockConnection(false);
        Gui::Selection().clearSelection();
        resetPreselectPoint();
        edit->PreselectCurve = -1;
        edit->PreselectCross = -1;
        edit->PreselectConstraint = -1;
        this->drawConstraintIcons();
        this->updateColor();
        // if in edit not delete the object
        return false;
    }
    // if not in edit delete the whole object
    return true;
}
