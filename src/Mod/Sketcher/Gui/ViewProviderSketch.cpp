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
#include <Inventor/SbBox3f.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbTime.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/nodes/SoCamera.h>

#include <QApplication>
#include <QFontMetricsF>
#include <QMenu>
#include <QMessageBox>
#include <QScreen>
#include <QTextStream>
#endif

// clang-format off
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
// clang-format on

#include <Base/Console.h>
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Gui/SoFCUnifiedSelection.h>
#include <Gui/Utilities.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/GeoList.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>

#include "DrawSketchHandler.h"
#include "EditDatumDialog.h"
#include "EditModeCoinManager.h"
#include "SnapManager.h"
#include "TaskDlgEditSketch.h"
#include "TaskSketcherValidation.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "ViewProviderSketchGeometryExtension.h"
#include "Workbench.h"


// clang-format off
FC_LOG_LEVEL_INIT("Sketch", true, true)

using namespace SketcherGui;
using namespace Sketcher;
namespace sp = std::placeholders;

/************** ViewProviderSketch::ParameterObserver *********************/

template<typename T>
T getSketcherGeneralParameter(const std::string& string, T defaultvalue)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    if constexpr (std::is_same_v<decltype(defaultvalue), unsigned int>) {
        return static_cast<unsigned int>(hGrp->GetUnsigned(string.c_str(), defaultvalue));
    }
    else if constexpr (std::is_same_v<decltype(defaultvalue), int>) {
        return static_cast<int>(hGrp->GetInt(string.c_str(), defaultvalue));
    }
}

ViewProviderSketch::ParameterObserver::ParameterObserver(ViewProviderSketch& client)
    : Client(client)
{}

ViewProviderSketch::ParameterObserver::~ParameterObserver()
{
    unsubscribeToParameters();
}

void ViewProviderSketch::ParameterObserver::updateBoolProperty(const std::string& string,
                                                               App::Property* property,
                                                               bool defaultvalue)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    auto boolprop = static_cast<App::PropertyBool*>(property);
    boolprop->setValue(hGrp->GetBool(string.c_str(), defaultvalue));
}

void ViewProviderSketch::ParameterObserver::updateColorProperty(const std::string& string,
                                                                App::Property* property, float r,
                                                                float g, float b)
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");

    auto colorprop = static_cast<App::PropertyColor*>(property);

    colorprop->setValue(r, g, b);

    App::Color elementAppColor = colorprop->getValue();
    unsigned long color = (unsigned long)(elementAppColor.getPackedValue());
    color = hGrp->GetUnsigned(string.c_str(), color);
    elementAppColor.setPackedValue((uint32_t)color);
    colorprop->setValue(elementAppColor);
}

void ViewProviderSketch::ParameterObserver::updateGridSize(const std::string& string,
                                                           App::Property* property)
{
    (void)property;
    (void)string;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    Client.GridSize.setValue(
        Base::Quantity::parse(
            QString::fromLatin1(hGrp->GetGroup("GridSize")->GetASCII("GridSize", "10.0").c_str()))
            .getValue());
}

void ViewProviderSketch::ParameterObserver::updateEscapeKeyBehaviour(const std::string& string,
                                                                     App::Property* property)
{
    (void)property;
    (void)string;

    ParameterGrp::handle hSketch = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    Client.viewProviderParameters.handleEscapeButton =
        !hSketch->GetBool("LeaveSketchWithEscape", true);
}

void ViewProviderSketch::ParameterObserver::updateAutoRecompute(const std::string& string,
                                                                App::Property* property)
{
    (void)property;
    (void)string;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");
    Client.viewProviderParameters.autoRecompute = hGrp->GetBool("AutoRecompute", false);
}

void ViewProviderSketch::ParameterObserver::updateRecalculateInitialSolutionWhileDragging(
    const std::string& string, App::Property* property)
{
    (void)property;
    (void)string;

    ParameterGrp::handle hGrp2 = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher");

    Client.viewProviderParameters.recalculateInitialSolutionWhileDragging =
        hGrp2->GetBool("RecalculateInitialSolutionWhileDragging", true);
}

void ViewProviderSketch::ParameterObserver::subscribeToParameters()
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
        hGrp->Attach(this);

        ParameterGrp::handle hGrp2 = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        hGrp2->Attach(this);

        ParameterGrp::handle hGrpv = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        hGrpv->Attach(this);
    }
    catch (const Base::ValueError& e) {// ensure that if parameter strings are not well-formed, the
                                       // exception is not propagated
        Base::Console().DeveloperError(
            "ViewProviderSketch", "Malformed parameter string: %s\n", e.what());
    }
}

void ViewProviderSketch::ParameterObserver::unsubscribeToParameters()
{
    try {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher/General");
        hGrp->Detach(this);

        ParameterGrp::handle hGrp2 = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        hGrp2->Detach(this);

        ParameterGrp::handle hGrpv = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        hGrpv->Detach(this);
    }
    catch (const Base::ValueError& e) {// ensure that if parameter strings are not well-formed, the
                                       // exception is not propagated
        Base::Console().DeveloperError(
            "ViewProviderSketch", "Malformed parameter string: %s\n", e.what());
    }
}

void ViewProviderSketch::ParameterObserver::initParameters()
{
    // once initialize the map with the properties

    SbColor defaultGridColor(0.7f, 0.7f, 0.7f);
    unsigned int packedDefaultGridColor = defaultGridColor.getPackedValue();

    parameterMap = {
        {"HideDependent",
         {[this](const std::string& string, App::Property* property) {
              updateBoolProperty(string, property, true);
          },
          &Client.HideDependent}},
        {"ShowLinks",
         {[this](const std::string& string, App::Property* property) {
              updateBoolProperty(string, property, true);
          },
          &Client.ShowLinks}},
        {"ShowSupport",
         {[this](const std::string& string, App::Property* property) {
              updateBoolProperty(string, property, true);
          },
          &Client.ShowSupport}},
        {"RestoreCamera",
         {[this](const std::string& string, App::Property* property) {
              updateBoolProperty(string, property, true);
          },
          &Client.RestoreCamera}},
        {"ForceOrtho",
         {[this](const std::string& string, App::Property* property) {
              updateBoolProperty(string, property, false);
          },
          &Client.ForceOrtho}},
        {"SectionView",
         {[this](const std::string& string, App::Property* property) {
              updateBoolProperty(string, property, false);
          },
          &Client.SectionView}},
        {"AutoConstraints",
         {[this](const std::string& string, App::Property* property) {
              updateBoolProperty(string, property, true);
          },
          &Client.Autoconstraints}},
        {"AvoidRedundantAutoconstraints",
         {[this](const std::string& string, App::Property* property) {
              updateBoolProperty(string, property, true);
          },
          &Client.AvoidRedundant}},
        {"updateEscapeKeyBehaviour",
         {[this](const std::string& string, App::Property* property) {
              updateEscapeKeyBehaviour(string, property);
          },
          nullptr}},
        {"AutoRecompute",
         {[this](const std::string& string, App::Property* property) {
              updateAutoRecompute(string, property);
          },
          nullptr}},
        {"RecalculateInitialSolutionWhileDragging",
         {[this](const std::string& string, App::Property* property) {
              updateRecalculateInitialSolutionWhileDragging(string, property);
          },
          nullptr}},
        {"GridSizePixelThreshold",
         {[this](const std::string& string, [[maybe_unused]] App::Property* property) {
              auto v = getSketcherGeneralParameter(string, 15);
              Client.setGridSizePixelThreshold(v);
          },
          nullptr}},
        {"GridNumberSubdivision",
         {[this](const std::string& string, [[maybe_unused]] App::Property* property) {
              auto v = getSketcherGeneralParameter(string, 10);
              Client.setGridNumberSubdivision(v);
          },
          nullptr}},
        {"GridLinePattern",
         {[this](const std::string& string, [[maybe_unused]] App::Property* property) {
              auto v = getSketcherGeneralParameter(string, 0x0f0f);
              Client.setGridLinePattern(v);
          },
          nullptr}},
        {"GridDivLinePattern",
         {[this](const std::string& string, [[maybe_unused]] App::Property* property) {
              auto v = getSketcherGeneralParameter(string, 0xffff);
              Client.setGridDivLinePattern(v);
          },
          nullptr}},
        {"GridLineWidth",
         {[this](const std::string& string, [[maybe_unused]] App::Property* property) {
              auto v = getSketcherGeneralParameter(string, 1);
              Client.setGridLineWidth(v);
          },
          nullptr}},
        {"GridDivLineWidth",
         {[this](const std::string& string, [[maybe_unused]] App::Property* property) {
              auto v = getSketcherGeneralParameter(string, 2);
              Client.setGridDivLineWidth(v);
          },
          nullptr}},
        {"GridLineColor",
         {[this, packedDefaultGridColor](const std::string& string,
                                         [[maybe_unused]] App::Property* property) {
              auto v = getSketcherGeneralParameter(string, packedDefaultGridColor);
              auto color = App::Color(v);
              Client.setGridLineColor(color);
          },
          nullptr}},
        {"GridDivLineColor",
         {[this, packedDefaultGridColor](const std::string& string,
                                         [[maybe_unused]] App::Property* property) {
              auto v = getSketcherGeneralParameter(string, packedDefaultGridColor);
              auto color = App::Color(v);
              Client.setGridDivLineColor(color);
          },
          nullptr}},
    };

    for (auto& val : parameterMap) {
        auto string = val.first;
        auto update = std::get<0>(val.second);
        auto property = std::get<1>(val.second);

        update(string, property);
    }

    // unsubscribed parameters which update a property on just once upon construction (and before
    // restore if properties are being restored from a file)
    updateColorProperty("SketchEdgeColor", &Client.LineColor, 1.0f, 1.0f, 1.0f);
    updateColorProperty("SketchVertexColor", &Client.PointColor, 1.0f, 1.0f, 1.0f);
    updateBoolProperty("ShowGrid", &Client.ShowGrid, false);
    updateBoolProperty("GridAuto", &Client.GridAuto, true);
    updateGridSize("GridSize", &Client.GridSize);
}

void ViewProviderSketch::ParameterObserver::OnChange(Base::Subject<const char*>& rCaller,
                                                     const char* sReason)
{
    (void)rCaller;

    auto key = parameterMap.find(sReason);
    if (key != parameterMap.end()) {
        auto string = key->first;
        auto update = std::get<0>(key->second);
        auto property = std::get<1>(key->second);

        update(string, property);
    }
}

/*************************** ViewProviderSketch **************************/

// Struct for holding previous click information
SbTime ViewProviderSketch::DoubleClick::prvClickTime;
SbVec2s ViewProviderSketch::DoubleClick::prvClickPos;// used by double-click-detector
SbVec2s ViewProviderSketch::DoubleClick::prvCursorPos;
SbVec2s ViewProviderSketch::DoubleClick::newCursorPos;

//**************************************************************************
// Construction/Destruction

/* TRANSLATOR SketcherGui::ViewProviderSketch */

PROPERTY_SOURCE_WITH_EXTENSIONS(SketcherGui::ViewProviderSketch, PartGui::ViewProvider2DObject)


ViewProviderSketch::ViewProviderSketch()
    : SelectionObserver(false)
    , Mode(STATUS_NONE)
    , listener(nullptr)
    , editCoinManager(nullptr)
    , snapManager(nullptr)
    , pObserver(std::make_unique<ViewProviderSketch::ParameterObserver>(*this))
    , sketchHandler(nullptr)
    , viewOrientationFactor(1)
{
    PartGui::ViewProviderAttachExtension::initExtension(this);
    PartGui::ViewProviderGridExtension::initExtension(this);

    ADD_PROPERTY_TYPE(Autoconstraints,
                      (true),
                      "Auto Constraints",
                      (App::PropertyType)(App::Prop_None),
                      "Create auto constraints");
    ADD_PROPERTY_TYPE(AvoidRedundant,
                      (true),
                      "Auto Constraints",
                      (App::PropertyType)(App::Prop_None),
                      "Avoid redundant autoconstraint");
    ADD_PROPERTY_TYPE(
        TempoVis,
        (Py::None()),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "Object that handles hiding and showing other objects when entering/leaving sketch.");
    ADD_PROPERTY_TYPE(
        HideDependent,
        (true),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, all objects that depend on the sketch are hidden when opening editing.");
    ADD_PROPERTY_TYPE(
        ShowLinks,
        (true),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, all objects used in links to external geometry are shown when opening sketch.");
    ADD_PROPERTY_TYPE(
        ShowSupport,
        (true),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, all objects this sketch is attached to are shown when opening sketch.");
    ADD_PROPERTY_TYPE(RestoreCamera,
                      (true),
                      "Visibility automation",
                      (App::PropertyType)(App::Prop_ReadOnly),
                      "If true, camera position before entering sketch is remembered, and restored "
                      "after closing it.");
    ADD_PROPERTY_TYPE(
        ForceOrtho,
        (false),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, camera type will be forced to orthographic view when entering editing mode.");
    ADD_PROPERTY_TYPE(
        SectionView,
        (false),
        "Visibility automation",
        (App::PropertyType)(App::Prop_ReadOnly),
        "If true, only objects (or part of) located behind the sketch plane are visible.");
    ADD_PROPERTY_TYPE(EditingWorkbench,
                      ("SketcherWorkbench"),
                      "Visibility automation",
                      (App::PropertyType)(App::Prop_ReadOnly),
                      "Name of the workbench to activate when editing this sketch.");
    ADD_PROPERTY_TYPE(VisualLayerList,
                      (VisualLayer()),
                      "Layers",
                      (App::PropertyType)(App::Prop_ReadOnly),
                      "Information about the Visual Representation of layers");

    // TODO: This is part of a naive minimal implementation to substitute rendering order
    // Three equally visual layers to enable/disable layer.
    std::vector<VisualLayer> layers;
    layers.emplace_back();                // Normal layer
    layers.emplace_back(0x7E7E);          // Discontinuous line layer
    layers.emplace_back(0xFFFF, 3, false);// Hidden layer

    VisualLayerList.setValues(std::move(layers));

    // Default values that will be overridden by preferences (if existing)
    PointSize.setValue(4);

    // visibility automation and other parameters: update parameter and property defaults to follow
    // preferences
    pObserver->initParameters();
    pObserver->subscribeToParameters();

    sPixmap = "Sketcher_Sketch";

    // rubberband selection
    rubberband = std::make_unique<Gui::Rubberband>();

    cameraSensor.setFunction(&ViewProviderSketch::camSensCB);
}

ViewProviderSketch::~ViewProviderSketch()
{}

void ViewProviderSketch::slotUndoDocument(const Gui::Document& /*doc*/)
{
    // Note 1: this slot is only operative during edit mode (see signal connection/disconnection)
    // Note 2: ViewProviderSketch::UpdateData does not generate updates during undo/redo
    //         transactions as mid-transaction data may not be in a valid state (e.g. constraints
    //         may reference invalid geometry). However undo/redo notify SketchObject after the
    //         undo/redo and before this slot is called.
    // Note 3: Note that recomputes are no longer inhibited during the call to this slot.
    forceUpdateData();
}

void ViewProviderSketch::slotRedoDocument(const Gui::Document& /*doc*/)
{
    // Note 1: this slot is only operative during edit mode (see signal connection/disconnection)
    // Note 2: ViewProviderSketch::UpdateData does not generate updates during undo/redo
    //         transactions as mid-transaction data may not be in a valid state (e.g. constraints
    //         may reference invalid geometry). However undo/redo notify SketchObject after the
    //         undo/redo and before this slot is called.
    // Note 3: Note that recomputes are no longer inhibited during the call to this slot.
    forceUpdateData();
}

void ViewProviderSketch::forceUpdateData()
{
    if (!getSketchObject()
             ->noRecomputes) {// the sketch was already solved in SketchObject in onUndoRedoFinished
        Gui::Command::updateActive();
    }
}

/***************************** handler management ************************************/

void ViewProviderSketch::activateHandler(DrawSketchHandler* newHandler)
{
    assert(editCoinManager);
    assert(!sketchHandler);

    sketchHandler = std::unique_ptr<DrawSketchHandler>(newHandler);
    Mode = STATUS_SKETCH_UseHandler;
    sketchHandler->activate(this);

    // make sure receiver has focus so immediately pressing Escape will be handled by
    // ViewProviderSketch::keyPressed() and dismiss the active handler, and not the entire
    // sketcher editor
    Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    mdi->setFocus();
}

void ViewProviderSketch::deactivateHandler()
{
    assert(isInEditMode());
    if (sketchHandler) {
        sketchHandler->deactivate();
        sketchHandler = nullptr;
    }
    Mode = STATUS_NONE;
}

/// removes the active handler
void ViewProviderSketch::purgeHandler()
{
    deactivateHandler();
    Gui::Selection().clearSelection();

    // ensure that we are in sketch only selection mode
    auto* view = dynamic_cast<Gui::View3DInventor*>(Gui::Application::Instance->editDocument()->getActiveView());

    if(view) {
        Gui::View3DInventorViewer* viewer;
        viewer = static_cast<Gui::View3DInventor*>(view)->getViewer();
        viewer->setSelectionEnabled(false);
    }
}

void ViewProviderSketch::setAxisPickStyle(bool on)
{
    assert(isInEditMode());
    editCoinManager->setAxisPickStyle(on);
}

void ViewProviderSketch::moveCursorToSketchPoint(Base::Vector2d point)
{

    SbVec3f sbpoint(point.x, point.y, 0.f);

    Gui::MDIView* mdi = this->getActiveView();
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(mdi);

    if (!view)
        return;

    Gui::View3DInventorViewer* viewer = view->getViewer();

    SbVec2s screencoords = viewer->getPointOnViewport(sbpoint);

    short x, y;
    screencoords.getValue(x, y);

    short height = viewer->getGLWidget()->height();// Coin3D origin bottom left, QT origin top left

    QPoint newPos = viewer->getGLWidget()->mapToGlobal(QPoint(x, height - y));


    // QScreen *screen = view->windowHandle()->screen();
    // QScreen *screen = QGuiApplication::primaryScreen();

    // QCursor::setPos(screen, newPos);
    QCursor::setPos(newPos);
}

void ViewProviderSketch::preselectAtPoint(Base::Vector2d point)
{
    if (Mode != STATUS_SELECT_Point && Mode != STATUS_SELECT_Edge
        && Mode != STATUS_SELECT_Constraint && Mode != STATUS_SKETCH_DragPoint
        && Mode != STATUS_SKETCH_DragCurve && Mode != STATUS_SKETCH_DragConstraint
        && Mode != STATUS_SKETCH_UseRubberBand) {

        SbVec3f sbpoint(point.x, point.y, 0.f);

        Gui::MDIView* mdi = this->getActiveView();
        Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(mdi);

        if (!view)
            return;

        Gui::View3DInventorViewer* viewer = view->getViewer();

        SbVec2s screencoords = viewer->getPointOnViewport(sbpoint);

        std::unique_ptr<SoPickedPoint> Point(this->getPointOnRay(screencoords, viewer));

        detectAndShowPreselection(Point.get(), screencoords);
    }
}

// **********************************************************************************

bool ViewProviderSketch::keyPressed(bool pressed, int key)
{
    switch (key) {
        case SoKeyboardEvent::ESCAPE: {
            // make the handler quit but not the edit mode
            if (isInEditMode() && sketchHandler) {
                if (!pressed)
                    sketchHandler->quit();
                return true;
            }
            if (isInEditMode() && !drag.DragConstraintSet.empty()) {
                if (!pressed) {
                    drag.DragConstraintSet.clear();
                }
                return true;
            }
            if (isInEditMode() && drag.isDragCurveValid()) {
                if (!pressed) {
                    getSketchObject()->movePoint(
                        drag.DragCurve, Sketcher::PointPos::none, Base::Vector3d(0, 0, 0), true);
                    drag.DragCurve = Drag::InvalidCurve;
                    resetPositionText();
                    Mode = STATUS_NONE;
                }
                return true;
            }
            if (isInEditMode() && drag.isDragPointValid()) {
                if (!pressed) {
                    int GeoId;
                    Sketcher::PointPos PosId;
                    getSketchObject()->getGeoVertexIndex(drag.DragPoint, GeoId, PosId);
                    getSketchObject()->movePoint(GeoId, PosId, Base::Vector3d(0, 0, 0), true);
                    drag.DragPoint = Drag::InvalidPoint;
                    resetPositionText();
                    Mode = STATUS_NONE;
                }
                return true;
            }
            if (isInEditMode()) {
                // #0001479: 'Escape' key dismissing dialog cancels Sketch editing
                // If we receive a button release event but not a press event before
                // then ignore this one.
                if (!pressed && !viewProviderParameters.buttonPress)
                    return true;
                viewProviderParameters.buttonPress = pressed;

                // More control over Sketcher edit mode Esc key behavior
                // https://forum.freecad.org/viewtopic.php?f=3&t=42207
                return viewProviderParameters.handleEscapeButton;
            }
            return false;
        } break;
        default: {
            if (isInEditMode() && sketchHandler)
                sketchHandler->registerPressedKey(pressed, key);
        }
    }

    return true;// handle all other key events
}

void ViewProviderSketch::setAngleSnapping(bool enable, Base::Vector2d referencePoint)
{
    assert(snapManager);
    snapManager->setAngleSnapping(enable, referencePoint);
}

void ViewProviderSketch::getProjectingLine(const SbVec2s& pnt,
                                           const Gui::View3DInventorViewer* viewer,
                                           SbLine& line) const
{
    const SbViewportRegion& vp = viewer->getSoRenderManager()->getViewportRegion();

    short x, y;
    pnt.getValue(x, y);
    SbVec2f VPsize = vp.getViewportSize();
    float dX, dY;
    VPsize.getValue(dX, dY);

    float fRatio = vp.getViewportAspectRatio();
    float pX = (float)x / float(vp.getViewportSizePixels()[0]);
    float pY = (float)y / float(vp.getViewportSizePixels()[1]);

    // now calculate the real points respecting aspect ratio information
    //
    if (fRatio > 1.0f) {
        pX = (pX - 0.5f * dX) * fRatio + 0.5f * dX;
    }
    else if (fRatio < 1.0f) {
        pY = (pY - 0.5f * dY) / fRatio + 0.5f * dY;
    }

    SoCamera* pCam = viewer->getSoRenderManager()->getCamera();
    if (!pCam)
        return;
    SbViewVolume vol = pCam->getViewVolume();

    vol.projectPointToLine(SbVec2f(pX, pY), line);
}

Base::Placement ViewProviderSketch::getEditingPlacement() const
{
    auto doc = Gui::Application::Instance->editDocument();
    if (!doc || doc->getInEdit() != this)
        return getSketchObject()->globalPlacement();

    // TODO: won't work if there is scale. Hmm... what to do...
    return Base::Placement(doc->getEditingTransform());
}

void ViewProviderSketch::getCoordsOnSketchPlane(const SbVec3f& point, const SbVec3f& normal,
                                                double& u, double& v) const
{
    // Plane form
    Base::Vector3d R0(0, 0, 0), RN(0, 0, 1), RX(1, 0, 0), RY(0, 1, 0);

    // move to position of Sketch
    Base::Placement Plz = getEditingPlacement();
    R0 = Plz.getPosition();
    Base::Rotation tmp(Plz.getRotation());
    tmp.multVec(RN, RN);
    tmp.multVec(RX, RX);
    tmp.multVec(RY, RY);
    Plz.setRotation(tmp);

    // line
    Base::Vector3d R1(point[0], point[1], point[2]), RA(normal[0], normal[1], normal[2]);
    if (fabs(RN * RA) < FLT_EPSILON)
        throw Base::ZeroDivisionError("View direction is parallel to sketch plane");
    // intersection point on plane
    Base::Vector3d S = R1 + ((RN * (R0 - R1)) / (RN * RA)) * RA;

    // distance to x Axle of the sketch
    S.TransformToCoordinateSystem(R0, RX, RY);

    u = S.x;
    v = S.y;
}

bool ViewProviderSketch::mouseButtonPressed(int Button, bool pressed, const SbVec2s& cursorPos,
                                            const Gui::View3DInventorViewer* viewer)
{
    assert(isInEditMode());

    // Calculate 3d point to the mouse position
    SbLine line;
    getProjectingLine(cursorPos, viewer, line);
    SbVec3f point = line.getPosition();
    SbVec3f normal = line.getDirection();

    // use scoped_ptr to make sure that instance gets deleted in all cases
    boost::scoped_ptr<SoPickedPoint> pp(this->getPointOnRay(cursorPos, viewer));

    // Radius maximum to allow double click event
    const int dblClickRadius = 5;

    double x, y;
    SbVec3f pos = point;
    if (pp) {
        const SoDetail* detail = pp->getDetail();
        if (detail && detail->getTypeId() == SoPointDetail::getClassTypeId()) {
            pos = pp->getPoint();
        }
    }

    try {
        getCoordsOnSketchPlane(pos, normal, x, y);
        snapManager->snap(x, y);
    }
    catch (const Base::ZeroDivisionError&) {
        return false;
    }

    // Left Mouse button ****************************************************
    if (Button == 1) {
        if (pressed) {
            // Do things depending on the mode of the user interaction
            switch (Mode) {
                case STATUS_NONE: {
                    bool done = false;
                    if (preselection.isPreselectPointValid()) {
                        // Base::Console().Log("start dragging, point:%d\n",this->DragPoint);
                        Mode = STATUS_SELECT_Point;
                        done = true;
                    }
                    else if (preselection.isPreselectCurveValid()) {
                        // Base::Console().Log("start dragging, point:%d\n",this->DragPoint);
                        Mode = STATUS_SELECT_Edge;
                        done = true;
                    }
                    else if (preselection.isCrossPreselected()) {
                        // Base::Console().Log("start dragging, point:%d\n",this->DragPoint);
                        Mode = STATUS_SELECT_Cross;
                        done = true;
                    }
                    else if (!preselection.PreselectConstraintSet.empty()) {
                        // Base::Console().Log("start dragging, point:%d\n",this->DragPoint);
                        Mode = STATUS_SELECT_Constraint;
                        done = true;
                    }

                    // Double click events variables
                    float dci = (float)QApplication::doubleClickInterval() / 1000.0f;

                    if (done
                        && SbVec2f(cursorPos - DoubleClick::prvClickPos).length() < dblClickRadius
                        && (SbTime::getTimeOfDay() - DoubleClick::prvClickTime).getValue() < dci) {

                        // Double Click Event Occurred
                        editDoubleClicked();
                        // Reset Double Click Static Variables
                        DoubleClick::prvClickTime = SbTime();
                        DoubleClick::prvClickPos = SbVec2s(
                            -16000,
                            -16000);// certainly far away from any clickable place, to avoid
                                    // re-trigger of double-click if next click happens fast.

                        Mode = STATUS_NONE;
                    }
                    else {
                        DoubleClick::prvClickTime = SbTime::getTimeOfDay();
                        DoubleClick::prvClickPos = cursorPos;
                        DoubleClick::prvCursorPos = cursorPos;
                        DoubleClick::newCursorPos = cursorPos;
                        if (!done)
                            Mode = STATUS_SKETCH_StartRubberBand;
                    }

                    return done;
                }
                case STATUS_SKETCH_UseHandler:
                    return sketchHandler->pressButton(Base::Vector2d(x, y));
                default:
                    return false;
            }
        }
        else {// Button 1 released
            // Do things depending on the mode of the user interaction
            switch (Mode) {
                case STATUS_SELECT_Point:
                    if (pp) {
                        // Base::Console().Log("Select Point:%d\n",this->DragPoint);
                        //  Do selection
                        std::stringstream ss;
                        ss << "Vertex" << preselection.getPreselectionVertexIndex();

                        if (isSelected(ss.str())) {
                            rmvSelection(ss.str());
                        }
                        else {
                            addSelection2(
                                ss.str(), pp->getPoint()[0], pp->getPoint()[1], pp->getPoint()[2]);
                            drag.resetIds();
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SELECT_Edge:
                    if (pp) {
                        // Base::Console().Log("Select Point:%d\n",this->DragPoint);
                        std::stringstream ss;
                        if (preselection.isEdge())
                            ss << "Edge" << preselection.getPreselectionEdgeIndex();
                        else// external geometry
                            ss << "ExternalEdge" << preselection.getPreselectionExternalEdgeIndex();

                        // If edge already selected move from selection
                        if (isSelected(ss.str())) {
                            rmvSelection(ss.str());
                        }
                        else {
                            // Add edge to the selection
                            addSelection2(
                                ss.str(), pp->getPoint()[0], pp->getPoint()[1], pp->getPoint()[2]);
                            drag.resetIds();
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SELECT_Cross:
                    if (pp) {
                        // Base::Console().Log("Select Point:%d\n",this->DragPoint);
                        std::stringstream ss;
                        switch (preselection.PreselectCross) {
                            case Preselection::Axes::RootPoint:
                                ss << "RootPoint";
                                break;
                            case Preselection::Axes::HorizontalAxis:
                                ss << "H_Axis";
                                break;
                            case Preselection::Axes::VerticalAxis:
                                ss << "V_Axis";
                                break;
                            default:
                                break;
                        }

                        // If cross already selected move from selection
                        if (isSelected(ss.str())) {
                            rmvSelection(ss.str());
                        }
                        else {
                            // Add cross to the selection
                            addSelection2(
                                ss.str(), pp->getPoint()[0], pp->getPoint()[1], pp->getPoint()[2]);
                            drag.resetIds();
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SELECT_Constraint:
                    if (pp) {
                        auto sels = preselection.PreselectConstraintSet;
                        for (int id : sels) {
                            std::stringstream ss;
                            ss << Sketcher::PropertyConstraintList::getConstraintName(id);

                            // If the constraint already selected remove
                            if (isSelected(ss.str())) {
                                rmvSelection(ss.str());
                            }
                            else {
                                // Add constraint to current selection
                                addSelection2(ss.str(),
                                              pp->getPoint()[0],
                                              pp->getPoint()[1],
                                              pp->getPoint()[2]);
                                drag.resetIds();
                            }
                        }
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_DragPoint:
                    if (drag.isDragPointValid()) {
                        int GeoId;
                        Sketcher::PointPos PosId;

                        getSketchObject()->getGeoVertexIndex(drag.DragPoint, GeoId, PosId);

                        if (GeoId != Sketcher::GeoEnum::GeoUndef
                            && PosId != Sketcher::PointPos::none) {
                            getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Drag Point"));
                            try {
                                Gui::cmdAppObjectArgs(getObject(),
                                                      "movePoint(%d,%d,App.Vector(%f,%f,0),%d)",
                                                      GeoId,
                                                      static_cast<int>(PosId),
                                                      x - drag.xInit,
                                                      y - drag.yInit,
                                                      0);

                                getDocument()->commitCommand();

                                tryAutoRecomputeIfNotSolve(getSketchObject());
                            }
                            catch (const Base::Exception& e) {
                                getDocument()->abortCommand();
                                Base::Console().DeveloperError(
                                    "ViewProviderSketch", "Drag point: %s\n", e.what());
                            }
                        }
                        setPreselectPoint(drag.DragPoint);
                        drag.DragPoint = Drag::InvalidPoint;
                        // updateColor();
                    }
                    resetPositionText();
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_DragCurve:
                    if (drag.isDragCurveValid()) {
                        const Part::Geometry* geo = getSketchObject()->getGeometry(drag.DragCurve);
                        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                            || geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()
                            || geo->getTypeId() == Part::GeomCircle::getClassTypeId()
                            || geo->getTypeId() == Part::GeomEllipse::getClassTypeId()
                            || geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()
                            || geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()
                            || geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()
                            || geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                            getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Drag Curve"));

                            auto geo = getSketchObject()->getGeometry(drag.DragCurve);
                            auto gf = GeometryFacade::getFacade(geo);

                            Base::Vector3d vec(x - drag.xInit, y - drag.yInit, 0);

                            // BSpline weights have a radius corresponding to the weight value
                            // However, in order for them proportional to the B-Spline size,
                            // the scenograph has a size scalefactor times the weight
                            // This code normalizes the information sent to the solver.
                            if (gf->getInternalType() == InternalType::BSplineControlPoint) {
                                auto circle = static_cast<const Part::GeomCircle*>(geo);
                                Base::Vector3d center = circle->getCenter();

                                Base::Vector3d dir = vec - center;

                                double scalefactor = 1.0;

                                if (circle->hasExtension(
                                        SketcherGui::ViewProviderSketchGeometryExtension::
                                            getClassTypeId())) {
                                    auto vpext = std::static_pointer_cast<
                                        const SketcherGui::ViewProviderSketchGeometryExtension>(
                                        circle
                                            ->getExtension(
                                                SketcherGui::ViewProviderSketchGeometryExtension::
                                                    getClassTypeId())
                                            .lock());

                                    scalefactor = vpext->getRepresentationFactor();
                                }

                                vec = center + dir / scalefactor;
                            }

                            try {
                                Gui::cmdAppObjectArgs(getObject(),
                                                      "movePoint(%d,%d,App.Vector(%f,%f,0),%d)",
                                                      drag.DragCurve,
                                                      static_cast<int>(Sketcher::PointPos::none),
                                                      vec.x,
                                                      vec.y,
                                                      drag.relative ? 1 : 0);

                                getDocument()->commitCommand();

                                tryAutoRecomputeIfNotSolve(getSketchObject());
                            }
                            catch (const Base::Exception& e) {
                                getDocument()->abortCommand();
                                Base::Console().DeveloperError(
                                    "ViewProviderSketch", "Drag curve: %s\n", e.what());
                            }
                        }
                        preselection.PreselectCurve = drag.DragCurve;
                        drag.DragCurve = Drag::InvalidCurve;
                        // updateColor();
                    }
                    resetPositionText();
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_DragConstraint:
                    if (!drag.DragConstraintSet.empty()) {
                        getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Drag Constraint"));
                        auto idset = drag.DragConstraintSet;
                        for (int id : idset) {
                            moveConstraint(id, Base::Vector2d(x, y));
                            // updateColor();
                        }
                        preselection.PreselectConstraintSet = drag.DragConstraintSet;
                        drag.DragConstraintSet.clear();
                        getDocument()->commitCommand();
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_StartRubberBand:// a single click happened, so clear selection
                                                   // unless user hold control.
                    if (!(QApplication::keyboardModifiers() & Qt::ControlModifier)) {
                        Gui::Selection().clearSelection();
                    }
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_UseRubberBand:
                    doBoxSelection(DoubleClick::prvCursorPos, cursorPos, viewer);
                    rubberband->setWorking(false);

                    // a redraw is required in order to clear the rubberband
                    draw(true, false);
                    const_cast<Gui::View3DInventorViewer*>(viewer)->redraw();
                    Mode = STATUS_NONE;
                    return true;
                case STATUS_SKETCH_UseHandler: {
                    return sketchHandler->releaseButton(Base::Vector2d(x, y));
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
                    sketchHandler->quit();
                    return true;
                case STATUS_NONE: {
                    // A right click shouldn't change the Edit Mode
                    if (preselection.isPreselectPointValid()) {
                        return true;
                    }
                    else if (preselection.isPreselectCurveValid()) {
                        return true;
                    }
                    else if (!preselection.PreselectConstraintSet.empty()) {
                        return true;
                    }
                    else {
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
                        // Create the Context Menu using the Main View Qt Widget
                        QMenu contextMenu(viewer->getGLWidget());
                        Gui::MenuManager::getInstance()->setupContextMenu(&geom, contextMenu);
                        contextMenu.exec(QCursor::pos());

                        return true;
                    }
                }
                case STATUS_SELECT_Point:
                    break;
                case STATUS_SELECT_Edge: {
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
                        const std::vector<std::string>& SubNames = selection[0].getSubNames();

                        // Two Objects are selected
                        if (SubNames.size() == 2) {
                            // go through the selected subelements
                            for (std::vector<std::string>::const_iterator it = SubNames.begin();
                                 it != SubNames.end();
                                 ++it) {

                                // If the object selected is of type edge
                                if (it->size() > 4 && it->substr(0, 4) == "Edge") {
                                    // Get the index of the object selected
                                    int GeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
                                    if (preselection.PreselectCurve == GeoId)
                                        rightClickOnSelectedLine = true;
                                }
                                else {
                                    // The selection is not exclusively edges
                                    rightClickOnSelectedLine = false;
                                }
                            }// End of Iteration
                        }
                    }

                    if (rightClickOnSelectedLine) {
                        geom << "Sketcher_ConstrainParallel"
                             << "Sketcher_ConstrainPerpendicular";
                    }

                    Gui::Application::Instance->setupContextMenu("View", &geom);
                    // Create the Context Menu using the Main View Qt Widget
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

bool ViewProviderSketch::mouseWheelEvent(int delta, const SbVec2s& cursorPos,
                                         const Gui::View3DInventorViewer* viewer)
{
    assert(isInEditMode());

    Q_UNUSED(delta);
    Q_UNUSED(cursorPos);
    Q_UNUSED(viewer);

    editCoinManager->drawConstraintIcons();

    return true;
}

void ViewProviderSketch::editDoubleClicked()
{
    if (preselection.isPreselectPointValid()) {
        Base::Console().Log("double click point:%d\n", preselection.PreselectPoint);
    }
    else if (preselection.isPreselectCurveValid()) {
        Base::Console().Log("double click edge:%d\n", preselection.PreselectCurve);
    }
    else if (preselection.isCrossPreselected()) {
        Base::Console().Log("double click cross:%d\n",
                            static_cast<int>(preselection.PreselectCross));
    }
    else if (!preselection.PreselectConstraintSet.empty()) {
        // Find the constraint
        const std::vector<Sketcher::Constraint*>& constrlist =
            getSketchObject()->Constraints.getValues();

        auto sels = preselection.PreselectConstraintSet;
        for (int id : sels) {

            Constraint* Constr = constrlist[id];

            // if its the right constraint
            if (Constr->isDimensional()) {
                Gui::Command::openCommand(
                    QT_TRANSLATE_NOOP("Command", "Modify sketch constraints"));
                EditDatumDialog editDatumDialog(this, id);
                editDatumDialog.exec();
            }
        }
    }
}

bool ViewProviderSketch::mouseMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    // maximum radius for mouse moves when selecting a geometry before switching to drag mode
    const int dragIgnoredDistance = 3;

    static bool selectableConstraints = true;

    if (Mode < STATUS_SKETCH_UseHandler) {
        bool tmpSelCons = QApplication::keyboardModifiers() & Qt::ShiftModifier;
        if (tmpSelCons != !selectableConstraints) {
            selectableConstraints = !tmpSelCons;
            editCoinManager->setConstraintSelectability(selectableConstraints);
        }
    }

    if (!isInEditMode())
        return false;

    // ignore small moves after selection
    switch (Mode) {
        case STATUS_SELECT_Point:
        case STATUS_SELECT_Edge:
        case STATUS_SELECT_Constraint:
        case STATUS_SKETCH_StartRubberBand:
            short dx, dy;
            (cursorPos - DoubleClick::prvCursorPos).getValue(dx, dy);
            if (std::abs(dx) < dragIgnoredDistance && std::abs(dy) < dragIgnoredDistance)
                return false;
        default:
            break;
    }

    // Calculate 3d point to the mouse position
    SbLine line;
    getProjectingLine(cursorPos, viewer, line);

    double x, y;
    try {
        getCoordsOnSketchPlane(line.getPosition(), line.getDirection(), x, y);
        snapManager->snap(x, y);
    }
    catch (const Base::ZeroDivisionError&) {
        return false;
    }

    bool preselectChanged = false;
    if (Mode != STATUS_SELECT_Point && Mode != STATUS_SELECT_Edge
        && Mode != STATUS_SELECT_Constraint && Mode != STATUS_SKETCH_DragPoint
        && Mode != STATUS_SKETCH_DragCurve && Mode != STATUS_SKETCH_DragConstraint
        && Mode != STATUS_SKETCH_UseRubberBand) {

        std::unique_ptr<SoPickedPoint> Point(this->getPointOnRay(cursorPos, viewer));

        preselectChanged = detectAndShowPreselection(Point.get(), cursorPos);
    }

    switch (Mode) {
        case STATUS_NONE:
            if (preselectChanged) {
                editCoinManager->drawConstraintIcons();
                this->updateColor();
                return true;
            }
            return false;
        case STATUS_SELECT_Point:
            if (!getSolvedSketch().hasConflicts() && preselection.isPreselectPointValid()
                && drag.DragPoint != preselection.PreselectPoint) {
                Mode = STATUS_SKETCH_DragPoint;
                drag.DragPoint = preselection.PreselectPoint;
                int GeoId;
                Sketcher::PointPos PosId;

                getSketchObject()->getGeoVertexIndex(drag.DragPoint, GeoId, PosId);

                if (GeoId != Sketcher::GeoEnum::GeoUndef && PosId != Sketcher::PointPos::none) {
                    getSketchObject()->initTemporaryMove(GeoId, PosId, false);
                    drag.resetVector();
                }
            }
            else {
                Mode = STATUS_NONE;
            }
            resetPreselectPoint();
            return true;
        case STATUS_SELECT_Edge:
            if (!getSolvedSketch().hasConflicts() && preselection.isPreselectCurveValid()
                && drag.DragCurve != preselection.PreselectCurve) {
                Mode = STATUS_SKETCH_DragCurve;
                drag.DragCurve = preselection.PreselectCurve;
                const Part::Geometry* geo = getSketchObject()->getGeometry(drag.DragCurve);

                // BSpline Control points are edge draggable only if their radius is movable
                // This is because dragging gives unwanted cosmetic results due to the scale ratio.
                // This is an heuristic as it does not check all indirect routes.
                if (GeometryFacade::isInternalType(geo, InternalType::BSplineControlPoint)) {
                    if (geo->hasExtension(Sketcher::SolverGeometryExtension::getClassTypeId())) {
                        auto solvext =
                            std::static_pointer_cast<const Sketcher::SolverGeometryExtension>(
                                geo->getExtension(
                                       Sketcher::SolverGeometryExtension::getClassTypeId())
                                    .lock());

                        // Edge parameters are Independent, so weight won't move
                        if (solvext->getEdge() == Sketcher::SolverGeometryExtension::Independent) {
                            Mode = STATUS_NONE;
                            return false;
                        }

                        // The B-Spline is constrained to be non-rational (equal weights), moving
                        // produces a bad effect because OCCT will normalize the values of the
                        // weights.
                        auto grp = getSolvedSketch().getDependencyGroup(drag.DragCurve,
                                                                        Sketcher::PointPos::none);

                        int bsplinegeoid = -1;

                        std::vector<int> polegeoids;

                        for (auto c : getSketchObject()->Constraints.getValues()) {
                            if (c->Type == Sketcher::InternalAlignment
                                && c->AlignmentType == BSplineControlPoint
                                && c->First == drag.DragCurve) {

                                bsplinegeoid = c->Second;
                                break;
                            }
                        }

                        if (bsplinegeoid == -1) {
                            Mode = STATUS_NONE;
                            return false;
                        }

                        for (auto c : getSketchObject()->Constraints.getValues()) {
                            if (c->Type == Sketcher::InternalAlignment
                                && c->AlignmentType == BSplineControlPoint
                                && c->Second == bsplinegeoid) {

                                polegeoids.push_back(c->First);
                            }
                        }

                        bool allingroup = true;

                        for (auto polegeoid : polegeoids) {
                            std::pair<int, Sketcher::PointPos> thispole =
                                std::make_pair(polegeoid, Sketcher::PointPos::none);

                            if (grp.find(thispole) == grp.end())// not found
                                allingroup = false;
                        }

                        if (allingroup) {// it is constrained to be non-rational
                            Mode = STATUS_NONE;
                            return false;
                        }
                    }
                }

                if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                    || geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                    drag.relative = true;

                    // Since the cursor moved from where it was clicked, and this is a relative
                    // move, calculate the click position and use it as initial point.
                    SbLine line2;
                    getProjectingLine(DoubleClick::prvCursorPos, viewer, line2);
                    getCoordsOnSketchPlane(
                        line2.getPosition(), line2.getDirection(), drag.xInit, drag.yInit);
                    snapManager->snap(drag.xInit, drag.yInit);
                }
                else {
                    drag.resetVector();
                }

                if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                    getSketchObject()->initTemporaryBSplinePieceMove(
                        drag.DragCurve,
                        Sketcher::PointPos::none,
                        Base::Vector3d(drag.xInit, drag.yInit, 0.0),
                        false);
                }
                else {
                    getSketchObject()->initTemporaryMove(
                        drag.DragCurve, Sketcher::PointPos::none, false);
                }
            }
            else {
                Mode = STATUS_NONE;
            }
            resetPreselectPoint();
            return true;
        case STATUS_SELECT_Constraint:
            Mode = STATUS_SKETCH_DragConstraint;
            drag.DragConstraintSet = preselection.PreselectConstraintSet;
            resetPreselectPoint();
            return true;
        case STATUS_SKETCH_DragPoint:
            if (drag.isDragPointValid()) {
                // Base::Console().Log("Drag Point:%d\n",edit->DragPoint);
                int GeoId;
                Sketcher::PointPos PosId;
                getSketchObject()->getGeoVertexIndex(drag.DragPoint, GeoId, PosId);
                Base::Vector3d vec(x, y, 0);

                if (GeoId != Sketcher::GeoEnum::GeoUndef && PosId != Sketcher::PointPos::none) {
                    if (getSketchObject()->moveTemporaryPoint(GeoId, PosId, vec, false) == 0) {
                        setPositionText(Base::Vector2d(x, y));
                        draw(true, false);
                    }
                }
            }
            return true;
        case STATUS_SKETCH_DragCurve:
            if (drag.isDragCurveValid()) {
                auto geo = getSketchObject()->getGeometry(drag.DragCurve);
                auto gf = GeometryFacade::getFacade(geo);

                Base::Vector3d vec(x - drag.xInit, y - drag.yInit, 0);

                // BSpline weights have a radius corresponding to the weight value
                // However, in order for them proportional to the B-Spline size,
                // the scenograph has a size scalefactor times the weight
                // This code normalizes the information sent to the solver.
                if (gf->getInternalType() == InternalType::BSplineControlPoint) {
                    auto circle = static_cast<const Part::GeomCircle*>(geo);
                    Base::Vector3d center = circle->getCenter();

                    Base::Vector3d dir = vec - center;

                    double scalefactor = 1.0;

                    if (circle->hasExtension(
                            SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId())) {
                        auto vpext = std::static_pointer_cast<
                            const SketcherGui::ViewProviderSketchGeometryExtension>(
                            circle
                                ->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::
                                                   getClassTypeId())
                                .lock());

                        scalefactor = vpext->getRepresentationFactor();
                    }

                    vec = center + dir / scalefactor;
                }

                if (getSketchObject()->moveTemporaryPoint(
                        drag.DragCurve, Sketcher::PointPos::none, vec, drag.relative)
                    == 0) {
                    setPositionText(Base::Vector2d(x, y));
                    draw(true, false);
                }
            }
            return true;
        case STATUS_SKETCH_DragConstraint:
            if (!drag.DragConstraintSet.empty()) {
                auto idset = drag.DragConstraintSet;
                for (int id : idset)
                    moveConstraint(id, Base::Vector2d(x, y));
            }
            return true;
        case STATUS_SKETCH_UseHandler:
            sketchHandler->mouseMove(Base::Vector2d(x, y));
            if (preselectChanged) {
                editCoinManager->drawConstraintIcons();
                this->updateColor();
            }
            return true;
        case STATUS_SKETCH_StartRubberBand: {
            Mode = STATUS_SKETCH_UseRubberBand;
            rubberband->setWorking(true);
            return true;
        }
        case STATUS_SKETCH_UseRubberBand: {
            // Here we must use the device-pixel-ratio to compute the correct y coordinate
            // (#0003130)
            qreal dpr = viewer->getGLWidget()->devicePixelRatioF();
            DoubleClick::newCursorPos = cursorPos;
            rubberband->setCoords(
                DoubleClick::prvCursorPos.getValue()[0],
                viewer->getGLWidget()->height() * dpr - DoubleClick::prvCursorPos.getValue()[1],
                DoubleClick::newCursorPos.getValue()[0],
                viewer->getGLWidget()->height() * dpr - DoubleClick::newCursorPos.getValue()[1]);
            viewer->redraw();
            return true;
        }
        default:
            return false;
    }

    return false;
}

void ViewProviderSketch::moveConstraint(int constNum, const Base::Vector2d& toPos)
{
    // are we in edit?
    if (!isInEditMode())
        return;

    Sketcher::SketchObject* obj = getSketchObject();
    const std::vector<Sketcher::Constraint*>& constrlist = obj->Constraints.getValues();
    Constraint* Constr = constrlist[constNum];

#ifdef FC_DEBUG
    int intGeoCount = obj->getHighestCurveIndex() + 1;
    int extGeoCount = obj->getExternalGeometryCount();
#endif

    // with memory allocation
    const std::vector<Part::Geometry*> geomlist = getSolvedSketch().extractGeometry(true, true);

#ifdef FC_DEBUG
    assert(int(geomlist.size()) == extGeoCount + intGeoCount);
    assert((Constr->First >= -extGeoCount && Constr->First < intGeoCount)
           || Constr->First != GeoEnum::GeoUndef);
#endif

    if (Constr->Type == Distance || Constr->Type == DistanceX || Constr->Type == DistanceY
        || Constr->Type == Radius || Constr->Type == Diameter || Constr->Type == Weight) {

        Base::Vector3d p1(0., 0., 0.), p2(0., 0., 0.);
        if (Constr->SecondPos != Sketcher::PointPos::none) {// point to point distance
            p1 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
            p2 = getSolvedSketch().getPoint(Constr->Second, Constr->SecondPos);
        }
        else if (Constr->Second != GeoEnum::GeoUndef) {
            p1 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
            const Part::Geometry* geo = GeoList::getGeometryFromGeoId(geomlist, Constr->Second);
            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment* lineSeg =
                    static_cast<const Part::GeomLineSegment*>(geo);
                Base::Vector3d l2p1 = lineSeg->getStartPoint();
                Base::Vector3d l2p2 = lineSeg->getEndPoint();
                if (Constr->FirstPos != Sketcher::PointPos::none) {// point to line distance
                    // calculate the projection of p1 onto line2
                    p2.ProjectToLine(p1 - l2p1, l2p2 - l2p1);
                    p2 += p1;
                }
                else {
                    const Part::Geometry* geo1 =
                        GeoList::getGeometryFromGeoId(geomlist, Constr->First);
                    const Part::GeomCircle* circleSeg = static_cast<const Part::GeomCircle*>(geo1);
                    Base::Vector3d ct = circleSeg->getCenter();
                    double radius = circleSeg->getRadius();
                    p1.ProjectToLine(ct - l2p1,
                                     l2p2 - l2p1);// project on the line translated to origin
                    Base::Vector3d dir = p1;
                    dir.Normalize();
                    p1 += ct;
                    p2 = ct + dir * radius;
                }
            }
            else if (geo->getTypeId()
                     == Part::GeomCircle::getClassTypeId()) {// circle to circle distance
                const Part::Geometry* geo1 = GeoList::getGeometryFromGeoId(geomlist, Constr->First);
                if (geo1->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                    const Part::GeomCircle* circleSeg1 = static_cast<const Part::GeomCircle*>(geo1);
                    const Part::GeomCircle* circleSeg2 = static_cast<const Part::GeomCircle*>(geo);
                    GetCirclesMinimalDistance(circleSeg1, circleSeg2, p1, p2);
                } else if (Constr->FirstPos != Sketcher::PointPos::none) { //point to circle distance
                    auto circleSeg2 = static_cast<const Part::GeomCircle*>(geo);
                    p1 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
                    Base::Vector3d v = p1 - circleSeg2->getCenter();
                    v = v.Normalize();
                    p2 = circleSeg2->getCenter() + circleSeg2->getRadius() * v;
                }
            }
            else
                return;
        }
        else if (Constr->FirstPos != Sketcher::PointPos::none) {
            p2 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
        }
        else if (Constr->First != GeoEnum::GeoUndef) {
            const Part::Geometry* geo = GeoList::getGeometryFromGeoId(geomlist, Constr->First);
            if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment* lineSeg =
                    static_cast<const Part::GeomLineSegment*>(geo);
                p1 = lineSeg->getStartPoint();
                p2 = lineSeg->getEndPoint();
            }
            else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle* arc = static_cast<const Part::GeomArcOfCircle*>(geo);
                double radius = arc->getRadius();
                Base::Vector3d center = arc->getCenter();
                p1 = center;

                double angle = Constr->LabelPosition;
                if (angle == 10) {
                    double startangle, endangle;
                    arc->getRange(startangle, endangle, /*emulateCCW=*/true);
                    angle = (startangle + endangle) / 2;
                }
                else {
                    Base::Vector3d tmpDir = Base::Vector3d(toPos.x, toPos.y, 0) - p1;
                    angle = atan2(tmpDir.y, tmpDir.x);
                }

                if (Constr->Type == Sketcher::Diameter)
                    p1 = center - radius * Base::Vector3d(cos(angle), sin(angle), 0.);

                p2 = center + radius * Base::Vector3d(cos(angle), sin(angle), 0.);
            }
            else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                const Part::GeomCircle* circle = static_cast<const Part::GeomCircle*>(geo);
                double radius = circle->getRadius();
                Base::Vector3d center = circle->getCenter();
                p1 = center;

                Base::Vector3d tmpDir = Base::Vector3d(toPos.x, toPos.y, 0) - p1;

                Base::Vector3d dir = radius * tmpDir.Normalize();

                if (Constr->Type == Sketcher::Diameter)
                    p1 = center - dir;

                if (Constr->Type == Sketcher::Weight) {

                    double scalefactor = 1.0;

                    if (circle->hasExtension(
                            SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId())) {
                        auto vpext = std::static_pointer_cast<
                            const SketcherGui::ViewProviderSketchGeometryExtension>(
                            circle
                                ->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::
                                                   getClassTypeId())
                                .lock());

                        scalefactor = vpext->getRepresentationFactor();
                    }

                    p2 = center + dir * scalefactor;
                }
                else
                    p2 = center + dir;
            }
            else
                return;
        }
        else
            return;

        Base::Vector3d vec = Base::Vector3d(toPos.x, toPos.y, 0) - p2;

        Base::Vector3d dir;
        if (Constr->Type == Distance || Constr->Type == Radius || Constr->Type == Diameter
            || Constr->Type == Weight)
            dir = (p2 - p1).Normalize();
        else if (Constr->Type == DistanceX)
            dir = Base::Vector3d((p2.x - p1.x >= FLT_EPSILON) ? 1 : -1, 0, 0);
        else if (Constr->Type == DistanceY)
            dir = Base::Vector3d(0, (p2.y - p1.y >= FLT_EPSILON) ? 1 : -1, 0);

        if (Constr->Type == Radius || Constr->Type == Diameter || Constr->Type == Weight) {
            Constr->LabelDistance = vec.x * dir.x + vec.y * dir.y;
            Constr->LabelPosition = atan2(dir.y, dir.x);
        }
        else {
            Base::Vector3d normal(-dir.y, dir.x, 0);
            Constr->LabelDistance = vec.x * normal.x + vec.y * normal.y;
            if (Constr->Type == Distance || Constr->Type == DistanceX
                || Constr->Type == DistanceY) {
                vec = Base::Vector3d(toPos.x, toPos.y, 0) - (p2 + p1) / 2;
                Constr->LabelPosition = vec.x * dir.x + vec.y * dir.y;
            }
        }
    }
    else if (Constr->Type == Angle) {
        moveAngleConstraint(constNum, toPos);
    }

    // delete the cloned objects
    for (Part::Geometry* geomPtr : geomlist) {
        if (geomPtr) {
            delete geomPtr;
        }
    }

    draw(true, false);
}

void ViewProviderSketch::moveAngleConstraint(int constNum, const Base::Vector2d& toPos)
{
    Sketcher::SketchObject* obj = getSketchObject();
    const std::vector<Sketcher::Constraint*>& constrlist = obj->Constraints.getValues();
    Constraint* constr = constrlist[constNum];

    Base::Vector3d p0(0., 0., 0.);
    double factor = 0.5;
    if (constr->Second != GeoEnum::GeoUndef) {// line to line angle
        if (constr->Third == GeoEnum::GeoUndef) {// angle between two lines
            const Part::Geometry* geo1 = obj->getGeometry(constr->First);
            const Part::Geometry* geo2 = obj->getGeometry(constr->Second);

            if (!isLineSegment(*geo1) || !isLineSegment(*geo2)) {
                return;
            }
            const auto* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geo1);
            const auto* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geo2);

            Base::Vector2d l1[2], l2[2];
            l1[0] = Base::Vector2d(lineSeg1->getStartPoint().x, lineSeg1->getStartPoint().y);
            l1[1] = Base::Vector2d(lineSeg1->getEndPoint().x, lineSeg1->getEndPoint().y);
            l2[0] = Base::Vector2d(lineSeg2->getStartPoint().x, lineSeg2->getStartPoint().y);
            l2[1] = Base::Vector2d(lineSeg2->getEndPoint().x, lineSeg2->getEndPoint().y);

            // First we will check if the angle needs to be reversed to its supplementary
            bool flip1 = (constr->FirstPos == Sketcher::PointPos::end);
            bool flip2 = (constr->SecondPos == Sketcher::PointPos::end);
            Base::Vector2d p11 = flip1 ? l1[1] : l1[0];
            Base::Vector2d p12 = flip1 ? l1[0] : l1[1];
            Base::Vector2d p21 = flip2 ? l2[1] : l2[0];
            Base::Vector2d p22 = flip2 ? l2[0] : l2[1];

            // Get the intersection point in 2d of the two lines if possible
            Base::Line2d line1(p11, p12);
            Base::Line2d line2(p21, p22);
            Base::Vector2d intersection = Base::Vector2d(0., 0.);
            if (!line1.Intersect(line2, intersection)) {
                return;
            }

            Base::Vector2d dir1 = p12 - p11;
            Base::Vector2d dir2 = p22 - p21;

            Base::Vector2d ap3 = intersection + dir1 + dir2;

            auto isLeftOfLine = [](Base::Vector2d a, Base::Vector2d b, Base::Vector2d c) {
                return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x) > 0;
            };

            bool sign1 = isLeftOfLine(p11, p12, ap3);
            bool sign2 = isLeftOfLine(p21, p22, ap3);

            bool sign3 = isLeftOfLine(p11, p12, toPos);
            bool sign4 = isLeftOfLine(p21, p22, toPos);

            bool reverse = !(sign1 == sign3 && sign2 == sign4) && !(sign1 != sign3 && sign2 != sign4);

            if (reverse) {
                obj->reverseAngleConstraintToSupplementary(constr, constNum);

                ap3 = intersection + dir1 - dir2; //- dir2 instead fo std::swap(dir1, dir2) and dir1 = -dir1
                sign1 = isLeftOfLine(p11, p12, ap3);
                sign2 = isLeftOfLine(p21, p22, ap3);
            }

            p0 = Base::Vector3d(intersection.x, intersection.y, 0.);
            factor *= (sign1 == sign3 && sign2 == sign4) ? 1. : -1.;
        }
        else {// angle-via-point
            Base::Vector3d p = getSolvedSketch().getPoint(constr->Third, constr->ThirdPos);
            p0 = Base::Vector3d(p.x, p.y, 0);
            Base::Vector3d dir1 = getSolvedSketch().calculateNormalAtPoint(constr->First, p.x, p.y);
            dir1.RotateZ(-M_PI / 2);// convert to vector of tangency by rotating
            Base::Vector3d dir2 = getSolvedSketch().calculateNormalAtPoint(constr->Second, p.x, p.y);
            dir2.RotateZ(-M_PI / 2);

            Base::Vector3d vec = Base::Vector3d(toPos.x, toPos.y, 0) - p0;
            factor = factor * Base::sgn<double>((dir1 + dir2) * vec);
        }
    }
    else if (constr->First != GeoEnum::GeoUndef) {// line/arc angle
        const Part::Geometry* geo = obj->getGeometry(constr->First);

        if (isLineSegment(*geo)) {
            const auto* lineSeg = static_cast<const Part::GeomLineSegment*>(geo);
            p0 = (lineSeg->getEndPoint() + lineSeg->getStartPoint()) / 2;
        }
        else if (isArcOfCircle(*geo)) {
            const auto* arc = static_cast<const Part::GeomArcOfCircle*>(geo);
            p0 = arc->getCenter();
        }
        else {
            return;
        }
    }
    else {
        return;
    }

    Base::Vector3d vec = Base::Vector3d(toPos.x, toPos.y, 0) - p0;
    constr->LabelDistance = factor * vec.Length();
}

bool ViewProviderSketch::isSelectable() const
{
    if (isEditing())
        return false;
    else
        return PartGui::ViewProvider2DObject::isSelectable();
}

void ViewProviderSketch::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    // are we in edit?
    if (isInEditMode()) {
        // ignore external object
        if (!msg.Object.getObjectName().empty()
            && msg.Object.getDocument() != getObject()->getDocument())
            return;

        bool handled = false;
        if (Mode == STATUS_SKETCH_UseHandler) {
            App::AutoTransaction committer;
            handled = sketchHandler->onSelectionChanged(msg);
        }
        if (handled)
            return;

        std::string temp;
        if (msg.Type == Gui::SelectionChanges::ClrSelection) {
            // if something selected in this object?
            if (!selection.SelPointSet.empty() || !selection.SelCurvSet.empty()
                || !selection.SelConstraintSet.empty()) {
                // clear our selection and update the color of the viewed edges and points
                clearSelectPoints();
                selection.SelCurvSet.clear();
                selection.SelConstraintSet.clear();
                editCoinManager->drawConstraintIcons();
                this->updateColor();
            }
        }
        else if (msg.Type == Gui::SelectionChanges::AddSelection) {
            // is it this object??
            if (strcmp(msg.pDocName, getSketchObject()->getDocument()->getName()) == 0
                && strcmp(msg.pObjectName, getSketchObject()->getNameInDocument()) == 0) {
                if (msg.pSubName) {
                    std::string shapetype(msg.pSubName);
                    if (shapetype.size() > 4 && shapetype.substr(0, 4) == "Edge") {
                        int GeoId = std::atoi(&shapetype[4]) - 1;
                        selection.SelCurvSet.insert(GeoId);
                        this->updateColor();
                    }
                    else if (shapetype.size() > 12 && shapetype.substr(0, 12) == "ExternalEdge") {
                        int GeoId = std::atoi(&shapetype[12]) - 1;
                        GeoId = -GeoId - 3;
                        selection.SelCurvSet.insert(GeoId);
                        this->updateColor();
                    }
                    else if (shapetype.size() > 6 && shapetype.substr(0, 6) == "Vertex") {
                        int VtId = std::atoi(&shapetype[6]) - 1;
                        addSelectPoint(VtId);
                        this->updateColor();
                    }
                    else if (shapetype == "RootPoint") {
                        addSelectPoint(Selection::RootPoint);
                        this->updateColor();
                    }
                    else if (shapetype == "H_Axis") {
                        selection.SelCurvSet.insert(Selection::HorizontalAxis);
                        this->updateColor();
                    }
                    else if (shapetype == "V_Axis") {
                        selection.SelCurvSet.insert(Selection::VerticalAxis);
                        this->updateColor();
                    }
                    else if (shapetype.size() > 10 && shapetype.substr(0, 10) == "Constraint") {
                        int ConstrId =
                            Sketcher::PropertyConstraintList::getIndexFromConstraintName(shapetype);
                        selection.SelConstraintSet.insert(ConstrId);
                        editCoinManager->drawConstraintIcons();
                        this->updateColor();
                    }
                }
            }
        }
        else if (msg.Type == Gui::SelectionChanges::RmvSelection) {
            // Are there any objects selected
            if (!selection.SelPointSet.empty() || !selection.SelCurvSet.empty()
                || !selection.SelConstraintSet.empty()) {
                // is it this object??
                if (strcmp(msg.pDocName, getSketchObject()->getDocument()->getName()) == 0
                    && strcmp(msg.pObjectName, getSketchObject()->getNameInDocument()) == 0) {
                    if (msg.pSubName) {
                        std::string shapetype(msg.pSubName);
                        if (shapetype.size() > 4 && shapetype.substr(0, 4) == "Edge") {
                            int GeoId = std::atoi(&shapetype[4]) - 1;
                            selection.SelCurvSet.erase(GeoId);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 12
                                 && shapetype.substr(0, 12) == "ExternalEdge") {
                            int GeoId = std::atoi(&shapetype[12]) - 1;
                            GeoId = -GeoId - 3;
                            selection.SelCurvSet.erase(GeoId);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 6 && shapetype.substr(0, 6) == "Vertex") {
                            int VtId = std::atoi(&shapetype[6]) - 1;
                            removeSelectPoint(VtId);
                            this->updateColor();
                        }
                        else if (shapetype == "RootPoint") {
                            removeSelectPoint(Sketcher::GeoEnum::RtPnt);
                            this->updateColor();
                        }
                        else if (shapetype == "H_Axis") {
                            selection.SelCurvSet.erase(Sketcher::GeoEnum::HAxis);
                            this->updateColor();
                        }
                        else if (shapetype == "V_Axis") {
                            selection.SelCurvSet.erase(Sketcher::GeoEnum::VAxis);
                            this->updateColor();
                        }
                        else if (shapetype.size() > 10 && shapetype.substr(0, 10) == "Constraint") {
                            int ConstrId =
                                Sketcher::PropertyConstraintList::getIndexFromConstraintName(
                                    shapetype);
                            selection.SelConstraintSet.erase(ConstrId);
                            editCoinManager->drawConstraintIcons();
                            this->updateColor();
                        }
                    }
                }
            }
        }
        else if (msg.Type == Gui::SelectionChanges::SetSelection) {
            // remove all items
            // selectionView->clear();
            // std::vector<SelectionSingleton::SelObj> objs =
            // Gui::Selection().getSelection(Reason.pDocName); for
            // (std::vector<SelectionSingleton::SelObj>::iterator it = objs.begin(); it !=
            // objs.end(); ++it) {
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
            if (strcmp(msg.pDocName, getSketchObject()->getDocument()->getName()) == 0
                && strcmp(msg.pObjectName, getSketchObject()->getNameInDocument()) == 0) {
                if (msg.pSubName) {
                    std::string shapetype(msg.pSubName);
                    if (shapetype.size() > 4 && shapetype.substr(0, 4) == "Edge") {
                        int GeoId = std::atoi(&shapetype[4]) - 1;
                        resetPreselectPoint();
                        preselection.PreselectCurve = GeoId;

                        if (sketchHandler)
                            sketchHandler->applyCursor();
                        this->updateColor();
                    }
                    else if (shapetype.size() > 6 && shapetype.substr(0, 6) == "Vertex") {
                        int PtIndex = std::atoi(&shapetype[6]) - 1;
                        setPreselectPoint(PtIndex);

                        if (sketchHandler)
                            sketchHandler->applyCursor();
                        this->updateColor();
                    }
                }
            }
        }
        else if (msg.Type == Gui::SelectionChanges::RmvPreselect) {
            resetPreselectPoint();
            if (sketchHandler)
                sketchHandler->applyCursor();
            this->updateColor();
        }
    }
}

bool ViewProviderSketch::detectAndShowPreselection(SoPickedPoint* Point, const SbVec2s& cursorPos)
{
    assert(isInEditMode());

    if (Point) {

        EditModeCoinManager::PreselectionResult result =
            editCoinManager->detectPreselection(Point, cursorPos);

        if (result.PointIndex != -1
            && result.PointIndex != preselection.PreselectPoint) {// if a new point is hit
            std::stringstream ss;
            ss << "Vertex" << result.PointIndex + 1;
            bool accepted =
                setPreselect(
                    ss.str(), Point->getPoint()[0], Point->getPoint()[1], Point->getPoint()[2])
                != 0;
            preselection.blockedPreselection = !accepted;
            if (accepted) {
                setPreselectPoint(result.PointIndex);

                if (sketchHandler)
                    sketchHandler->applyCursor();
                return true;
            }
        }
        else if (result.GeoIndex != -1
                 && result.GeoIndex != preselection.PreselectCurve) {// if a new curve is hit
            std::stringstream ss;
            if (result.GeoIndex >= 0)
                ss << "Edge" << result.GeoIndex + 1;
            else// external geometry
                ss << "ExternalEdge"
                   << -result.GeoIndex + Sketcher::GeoEnum::RefExt
                        + 1;// convert index start from -3 to 1
            bool accepted =
                setPreselect(
                    ss.str(), Point->getPoint()[0], Point->getPoint()[1], Point->getPoint()[2])
                != 0;
            preselection.blockedPreselection = !accepted;
            if (accepted) {
                resetPreselectPoint();
                preselection.PreselectCurve = result.GeoIndex;

                if (sketchHandler)
                    sketchHandler->applyCursor();
                return true;
            }
        }
        else if (result.Cross != EditModeCoinManager::PreselectionResult::Axes::None
                 && static_cast<int>(result.Cross)
                     != static_cast<int>(preselection.PreselectCross)) {// if a cross line is hit
            std::stringstream ss;
            switch (result.Cross) {
                case EditModeCoinManager::PreselectionResult::Axes::RootPoint:
                    ss << "RootPoint";
                    break;
                case EditModeCoinManager::PreselectionResult::Axes::HorizontalAxis:
                    ss << "H_Axis";
                    break;
                case EditModeCoinManager::PreselectionResult::Axes::VerticalAxis:
                    ss << "V_Axis";
                    break;
                case EditModeCoinManager::PreselectionResult::Axes::None:
                    break;// silent warning - be explicit
            }
            bool accepted =
                setPreselect(
                    ss.str(), Point->getPoint()[0], Point->getPoint()[1], Point->getPoint()[2])
                != 0;
            preselection.blockedPreselection = !accepted;
            if (accepted) {
                if (result.Cross == EditModeCoinManager::PreselectionResult::Axes::RootPoint)
                    setPreselectRootPoint();
                else
                    resetPreselectPoint();
                preselection.PreselectCross =
                    static_cast<Preselection::Axes>(static_cast<int>(result.Cross));

                if (sketchHandler)
                    sketchHandler->applyCursor();
                return true;
            }
        }
        else if (!result.ConstrIndices.empty()
                 && result.ConstrIndices
                     != preselection.PreselectConstraintSet) {// if a constraint is hit
            bool accepted = true;
            for (std::set<int>::iterator it = result.ConstrIndices.begin();
                 it != result.ConstrIndices.end();
                 ++it) {
                std::stringstream ss;
                ss << Sketcher::PropertyConstraintList::getConstraintName(*it);

                accepted &=
                    setPreselect(
                        ss.str(), Point->getPoint()[0], Point->getPoint()[1], Point->getPoint()[2])
                    != 0;

                preselection.blockedPreselection = !accepted;
                // TODO: Should we clear preselections that went through, if one fails?
            }
            if (accepted) {
                resetPreselectPoint();
                preselection.PreselectConstraintSet = result.ConstrIndices;

                if (sketchHandler)
                    sketchHandler->applyCursor();
                return true;// Preselection changed
            }
        }
        else if ((result.PointIndex == -1 && result.GeoIndex == -1
                  && result.Cross == EditModeCoinManager::PreselectionResult::Axes::None
                  && result.ConstrIndices.empty())
                 && (preselection.isPreselectPointValid() || preselection.isPreselectCurveValid()
                     || preselection.isCrossPreselected()
                     || !preselection.PreselectConstraintSet.empty()
                     || preselection.blockedPreselection)) {
            // we have just left a preselection
            resetPreselectPoint();
            preselection.blockedPreselection = false;
            if (sketchHandler)
                sketchHandler->applyCursor();
            return true;
        }
        Gui::Selection().setPreselectCoord(
            Point->getPoint()[0], Point->getPoint()[1], Point->getPoint()[2]);
    }
    else if (preselection.isPreselectCurveValid() || preselection.isPreselectPointValid()
             || !preselection.PreselectConstraintSet.empty() || preselection.isCrossPreselected()
             || preselection.blockedPreselection) {
        resetPreselectPoint();
        preselection.blockedPreselection = false;
        if (sketchHandler)
            sketchHandler->applyCursor();
        return true;
    }

    return false;
}

void ViewProviderSketch::centerSelection()
{
    Gui::MDIView* mdi = this->getActiveView();
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(mdi);
    if (!view || !isInEditMode())
        return;

    SoGroup* group = editCoinManager->getSelectedConstraints();

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

void ViewProviderSketch::doBoxSelection(const SbVec2s& startPos, const SbVec2s& endPos,
                                        const Gui::View3DInventorViewer* viewer)
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

    Sketcher::SketchObject* sketchObject = getSketchObject();

    Base::Placement Plm = getEditingPlacement();

    int intGeoCount = sketchObject->getHighestCurveIndex() + 1;
    int extGeoCount = sketchObject->getExternalGeometryCount();

    const std::vector<Part::Geometry*> geomlist =
        sketchObject->getCompleteGeometry();// without memory allocation
    assert(int(geomlist.size()) == extGeoCount + intGeoCount);
    assert(int(geomlist.size()) >= 2);

    Base::Vector3d pnt0, pnt1, pnt2, pnt;
    int VertexId =
        -1;// the loop below should be in sync with the main loop in ViewProviderSketch::draw
           // so that the vertex indices are calculated correctly
    int GeoId = 0;

    bool touchMode = false;
    // check if selection goes from the right to the left side (for touch-selection where even
    // partially boxed objects get selected)
    if (corners[0].getValue()[0] > corners[1].getValue()[0])
        touchMode = true;

    for (std::vector<Part::Geometry*>::const_iterator it = geomlist.begin();
         it != geomlist.end() - 2;
         ++it, ++GeoId) {

        if (GeoId >= intGeoCount)
            GeoId = -extGeoCount;

        if ((*it)->getTypeId() == Part::GeomPoint::getClassTypeId()) {
            // ----- Check if single point lies inside box selection -----/
            const Part::GeomPoint* point = static_cast<const Part::GeomPoint*>(*it);
            Plm.multVec(point->getPoint(), pnt0);
            pnt0 = proj(pnt0);
            VertexId += 1;

            if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                addSelection2(ss.str());
            }
        }
        else if ((*it)->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            // ----- Check if line segment lies inside box selection -----/
            const Part::GeomLineSegment* lineSeg = static_cast<const Part::GeomLineSegment*>(*it);
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
                addSelection2(ss.str());
            }

            if (pnt2Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                addSelection2(ss.str());
            }

            if ((pnt1Inside && pnt2Inside) && !touchMode) {
                std::stringstream ss;
                ss << "Edge" << GeoId + 1;
                addSelection2(ss.str());
            }
            // check if line intersects with polygon
            else if (touchMode) {
                Base::Polygon2d lineAsPolygon;
                lineAsPolygon.Add(Base::Vector2d(pnt1.x, pnt1.y));
                lineAsPolygon.Add(Base::Vector2d(pnt2.x, pnt2.y));
                std::list<Base::Polygon2d> resultList;
                polygon.Intersect(lineAsPolygon, resultList);
                if (!resultList.empty()) {
                    std::stringstream ss;
                    ss << "Edge" << GeoId + 1;
                    addSelection2(ss.str());
                }
            }
        }
        else if ((*it)->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            // ----- Check if circle lies inside box selection -----/
            /// TODO: Make it impossible to miss the circle if it's big and the selection pretty
            /// thin.
            const Part::GeomCircle* circle = static_cast<const Part::GeomCircle*>(*it);
            pnt0 = circle->getCenter();
            VertexId += 1;

            Plm.multVec(pnt0, pnt0);
            pnt0 = proj(pnt0);

            if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y)) || touchMode) {
                if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y))) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId + 1;
                    addSelection2(ss.str());
                }
                int countSegments = 12;
                if (touchMode)
                    countSegments = 36;

                float segment = float(2 * M_PI) / countSegments;

                // circumscribed polygon radius
                float radius = float(circle->getRadius()) / cos(segment / 2);

                bool bpolyInside = true;
                pnt0 = circle->getCenter();
                float angle = 0.f;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(
                        pnt0.x + radius * cos(angle), pnt0.y + radius * sin(angle), 0.f);
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
                    addSelection2(ss.str());
                }
            }
        }
        else if ((*it)->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
            // ----- Check if ellipse lies inside box selection -----/
            const Part::GeomEllipse* ellipse = static_cast<const Part::GeomEllipse*>(*it);
            pnt0 = ellipse->getCenter();
            VertexId += 1;

            Plm.multVec(pnt0, pnt0);
            pnt0 = proj(pnt0);

            if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y)) || touchMode) {
                if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y))) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId + 1;
                    addSelection2(ss.str());
                }

                int countSegments = 12;
                if (touchMode)
                    countSegments = 24;
                double segment = (2 * M_PI) / countSegments;

                // circumscribed polygon radius
                double a = (ellipse->getMajorRadius()) / cos(segment / 2);
                double b = (ellipse->getMinorRadius()) / cos(segment / 2);
                Base::Vector3d majdir = ellipse->getMajorAxisDir();
                Base::Vector3d mindir = Base::Vector3d(-majdir.y, majdir.x, 0.0);

                bool bpolyInside = true;
                pnt0 = ellipse->getCenter();
                double angle = 0.;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = pnt0 + (cos(angle) * a) * majdir + sin(angle) * b * mindir;
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
                    addSelection2(ss.str());
                }
            }
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfCircle* aoc = static_cast<const Part::GeomArcOfCircle*>(*it);

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

                if (startangle > endangle)// if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle - startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                if (touchMode)
                    countSegments = countSegments * 2.5;
                float segment = float(range) / countSegments;

                // circumscribed polygon radius
                float radius = float(aoc->getRadius()) / cos(segment / 2);

                pnt0 = aoc->getCenter();
                float angle = float(startangle) + segment / 2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(
                        pnt0.x + radius * cos(angle), pnt0.y + radius * sin(angle), 0.f);
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
                    addSelection2(ss.str());
                }
            }

            if (pnt0Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId - 1;
                addSelection2(ss.str());
            }

            if (pnt1Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                addSelection2(ss.str());
            }

            if (polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                addSelection2(ss.str());
            }
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfEllipse* aoe = static_cast<const Part::GeomArcOfEllipse*>(*it);

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

                if (startangle > endangle)// if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle - startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                if (touchMode)
                    countSegments = countSegments * 2.5;
                double segment = (range) / countSegments;

                // circumscribed polygon radius
                double a = (aoe->getMajorRadius()) / cos(segment / 2);
                double b = (aoe->getMinorRadius()) / cos(segment / 2);
                Base::Vector3d majdir = aoe->getMajorAxisDir();
                Base::Vector3d mindir = Base::Vector3d(-majdir.y, majdir.x, 0.0);

                pnt0 = aoe->getCenter();
                double angle = (startangle) + segment / 2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = pnt0 + cos(angle) * a * majdir + sin(angle) * b * mindir;

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
                    addSelection2(ss.str());
                }
            }
            if (pnt0Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId - 1;
                addSelection2(ss.str());
            }

            if (pnt1Inside) {
                std::stringstream ss;
                ss << "Vertex" << VertexId;
                addSelection2(ss.str());
            }

            if (polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y))) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                addSelection2(ss.str());
            }
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfHyperbola* aoh = static_cast<const Part::GeomArcOfHyperbola*>(*it);
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

                if (startangle > endangle)// if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle - startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                if (touchMode)
                    countSegments = countSegments * 2.5;

                float segment = float(range) / countSegments;

                // circumscribed polygon radius
                float a = float(aoh->getMajorRadius()) / cos(segment / 2);
                float b = float(aoh->getMinorRadius()) / cos(segment / 2);
                float phi = float(aoh->getAngleXU());

                pnt0 = aoh->getCenter();
                float angle = float(startangle) + segment / 2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(
                        pnt0.x + a * cosh(angle) * cos(phi) - b * sinh(angle) * sin(phi),
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
                    addSelection2(ss.str());
                }
                if (pnt0Inside) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId - 1;
                    addSelection2(ss.str());
                }

                if (pnt1Inside) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId;
                    addSelection2(ss.str());
                }

                if (polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y))) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId + 1;
                    addSelection2(ss.str());
                }
            }
        }
        else if ((*it)->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfParabola* aop = static_cast<const Part::GeomArcOfParabola*>(*it);

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

                if (startangle > endangle)// if arc is reversed
                    std::swap(startangle, endangle);

                double range = endangle - startangle;
                int countSegments = std::max(2, int(12.0 * range / (2 * M_PI)));
                if (touchMode)
                    countSegments = countSegments * 2.5;

                float segment = float(range) / countSegments;
                // In local coordinate system, value() of parabola is:
                // P(U) = O + U*U/(4.*F)*XDir + U*YDir
                //  circumscribed polygon radius
                float focal = float(aop->getFocal()) / cos(segment / 2);
                float phi = float(aop->getAngleXU());

                pnt0 = aop->getCenter();
                float angle = float(startangle) + segment / 2;
                for (int i = 0; i < countSegments; ++i, angle += segment) {
                    pnt = Base::Vector3d(
                        pnt0.x + angle * angle / 4 / focal * cos(phi) - angle * sin(phi),
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
                    addSelection2(ss.str());
                }
                if (pnt0Inside) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId - 1;
                    addSelection2(ss.str());
                }

                if (pnt1Inside) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId;
                    addSelection2(ss.str());
                }

                if (polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y))) {
                    std::stringstream ss;
                    ss << "Vertex" << VertexId + 1;
                    addSelection2(ss.str());
                }
            }
        }
        else if ((*it)->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            const Part::GeomBSplineCurve* spline = static_cast<const Part::GeomBSplineCurve*>(*it);
            // std::vector<Base::Vector3d> poles = spline->getPoles();
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
                addSelection2(ss.str());
            }

            if (pnt2Inside || (touchMode && pnt1Inside)) {
                std::stringstream ss;
                ss << "Vertex" << VertexId + 1;
                addSelection2(ss.str());
            }

            // This is a rather approximated approach. No it does not guarantee that the whole curve
            // is boxed, specially for periodic curves, but it works reasonably well. Including all
            // poles, which could be done, generally forces the user to select much more than the
            // curve (all the poles) and it would not select the curve in cases where it is indeed
            // comprised in the box. The implementation of the touch mode is also far from a
            // desirable "touch" as it only recognizes touched points not the curve itself
            if ((pnt1Inside && pnt2Inside) || (touchMode && (pnt1Inside || pnt2Inside))) {
                std::stringstream ss;
                ss << "Edge" << GeoId + 1;
                addSelection2(ss.str());
            }
        }
    }

    pnt0 = proj(Plm.getPosition());
    if (polygon.Contains(Base::Vector2d(pnt0.x, pnt0.y))) {
        std::stringstream ss;
        ss << "RootPoint";
        addSelection2(ss.str());
    }
}

void ViewProviderSketch::updateColor()
{
    assert(isInEditMode());

    editCoinManager->updateColor();
}

bool ViewProviderSketch::doubleClicked()
{
    Gui::Application::Instance->activeDocument()->setEdit(this);
    return true;
}

float ViewProviderSketch::getScaleFactor() const
{
    assert(isInEditMode());
    Gui::MDIView* mdi =
        Gui::Application::Instance->editViewOfNode(editCoinManager->getRootEditNode());
    if (mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(mdi)->getViewer();
        SoCamera* camera = viewer->getSoRenderManager()->getCamera();
        float scale = camera->getViewVolume(camera->aspectRatio.getValue())
                          .getWorldToScreenScale(SbVec3f(0.f, 0.f, 0.f), 0.1f)
            / 3;
        return scale;
    }
    else {
        return 1.f;
    }
}

// This function ensures that the geometry used for drawing takes into account:
// 1. the OCC mandated weight, which is normalised for non-rational BSplines, but not normalised for
// rational BSplines. That includes properly sizing for drawing any weight constraint. This function
// ensures that both the geometry of the SketchObject and solver are updated with the new value of
// the scaling factor (via the extension)
// 2. the scaling factor, including inserting the scaling factor into the
// ViewProviderSketchGeometryExtension so as to enable That ensures that dragging operations on the
// circles of the poles of the B-Splines are properly rendered.
//
// This function takes a reference to a vector of deep copies to delete. These deep copies are
// necessary to transparently perform (1) while doing (2).
void ViewProviderSketch::scaleBSplinePoleCirclesAndUpdateSolverAndSketchObjectGeometry(
    GeoListFacade& geolistfacade, bool geometrywithmemoryallocation)
{
    // In order to allow to tweak geometry and insert scaling factors, this function needs to
    // change the geometry vector. This is highly exceptional for a drawing function and special
    // care needs to be taken. This is valid because:
    // 1. The treatment is exceptional and no other appropriate place is available to perform this
    // tweak
    // 2. The original object needs to remain const for the benefit of all other class hierarchy of
    // drawing functions
    // 3. When referring to actual geometry, the modified pointers are short lived, as they are
    // destroyed after drawing
    auto& tempGeo = geolistfacade.geomlist;

    int GeoId = 0;
    for (auto it = tempGeo.begin(); it != tempGeo.end() - 2; ++it, GeoId++) {
        if (GeoId >= geolistfacade.getInternalCount())
            GeoId = -geolistfacade.getExternalCount();

        if ((*it)->getGeometry()->getTypeId() == Part::GeomCircle::getClassTypeId()) {// circle
            const Part::GeomCircle* circle =
                static_cast<const Part::GeomCircle*>((*it)->getGeometry());
            auto& gf = (*it);

            // BSpline weights have a radius corresponding to the weight value
            // However, in order for them proportional to the B-Spline size,
            // the scenograph has a size scalefactor times the weight
            //
            // This code produces the scaled up version of the geometry for the scenograph
            if (gf->getInternalType() == InternalType::BSplineControlPoint) {
                for (auto c : getSketchObject()->Constraints.getValues()) {
                    if (c->Type == InternalAlignment && c->AlignmentType == BSplineControlPoint
                        && c->First == GeoId) {
                        auto bspline = dynamic_cast<const Part::GeomBSplineCurve*>(
                            tempGeo[c->Second]->getGeometry());

                        if (bspline) {
                            auto weights = bspline->getWeights();

                            double weight = 1.0;
                            if (c->InternalAlignmentIndex < int(weights.size()))
                                weight = weights[c->InternalAlignmentIndex];

                            // tentative scaling factor:
                            // proportional to the length of the bspline
                            // inversely proportional to the number of poles
                            double scalefactor = bspline->length(bspline->getFirstParameter(),
                                                                 bspline->getLastParameter())
                                / 10.0 / weights.size();

                            double vradius = weight * scalefactor;
                            if (!bspline->isRational()) {
                                // OCCT sets the weights to 1.0 if a bspline is non-rational, but if
                                // the user has a weight constraint on any pole it would cause a
                                // visual artifact of having a constraint with a different radius
                                // and an unscaled circle so better scale the circles.
                                std::vector<int> polegeoids;
                                polegeoids.reserve(weights.size());

                                for (auto ic : getSketchObject()->Constraints.getValues()) {
                                    if (ic->Type == InternalAlignment
                                        && ic->AlignmentType == BSplineControlPoint
                                        && ic->Second == c->Second) {
                                        polegeoids.push_back(ic->First);
                                    }
                                }

                                for (auto ic : getSketchObject()->Constraints.getValues()) {
                                    if (ic->Type == Weight) {
                                        auto pos = std::find(
                                            polegeoids.begin(), polegeoids.end(), ic->First);

                                        if (pos != polegeoids.end()) {
                                            vradius = ic->getValue() * scalefactor;
                                            break;// one is enough, otherwise it would not be
                                                  // non-rational
                                        }
                                    }
                                }
                            }

                            Part::GeomCircle* tmpcircle;

                            if (geometrywithmemoryallocation) {// with memory allocation
                                tmpcircle = const_cast<Part::GeomCircle*>(circle);
                                tmpcircle->setRadius(vradius);
                            }
                            else {// without memory allocation
                                tmpcircle = static_cast<Part::GeomCircle*>(circle->clone());
                                tmpcircle->setRadius(vradius);
                                tempGeo[GeoId] = GeometryFacade::getFacade(
                                    tmpcircle, true);// this is the circle that will be drawn, with
                                                     // the updated vradius, the facade takes
                                                     // ownership and will deallocate.
                            }

                            if (!circle->hasExtension(
                                    SketcherGui::ViewProviderSketchGeometryExtension::
                                        getClassTypeId())) {
                                // It is ok to add this kind of extension to a const geometry
                                // because:
                                // 1. It does not modify the object in a way that affects property
                                // state, just ViewProvider representation
                                // 2. If it is lost (for example upon undo), redrawing will
                                // reinstate it with the correct value
                                const_cast<Part::GeomCircle*>(circle)->setExtension(
                                    std::make_unique<
                                        SketcherGui::ViewProviderSketchGeometryExtension>());
                            }

                            auto vpext = std::const_pointer_cast<
                                SketcherGui::ViewProviderSketchGeometryExtension>(
                                std::static_pointer_cast<
                                    const SketcherGui::ViewProviderSketchGeometryExtension>(
                                    circle
                                        ->getExtension(
                                            SketcherGui::ViewProviderSketchGeometryExtension::
                                                getClassTypeId())
                                        .lock()));

                            vpext->setRepresentationFactor(scalefactor);

                            // save scale factor for any prospective dragging operation
                            // 1. Solver must be updated, in case a dragging operation starts
                            // 2. if temp geometry is being used (with memory allocation), then the
                            // copy we have here must be updated. If
                            //    no temp geometry is being used, then the normal geometry must be
                            //    updated.
                            // make solver be ready for a dragging operation
                            auto solverext = vpext->copy();

                            getSketchObject()->updateSolverExtension(GeoId, std::move(solverext));
                        }
                        break;
                    }
                }
            }
        }
    }
}

void ViewProviderSketch::draw(bool temp /*=false*/, bool rebuildinformationoverlay /*=true*/)
{
    assert(isInEditMode());

    // ============== Retrieve geometry to be represented =================================

    auto geolistfacade = temp ? getSolvedSketch().extractGeoListFacade() :// with memory allocation
        getSketchObject()->getGeoListFacade();// without memory allocation

    assert(int(geolistfacade.geomlist.size()) >= 2);

    // ============== Prepare geometry for representation ==================================

    // ************ Manage BSpline pole circle scaling  ****************************

    // This function ensures that the geometry used for drawing takes into account:
    // 1. the OCC mandated weight, which is normalised for non-rational BSplines, but not normalised
    // for rational BSplines. That includes properly sizing for drawing any weight constraint. This
    // function ensures that both the geometry of the SketchObject and solver are updated with the
    // new value of the scaling factor (via the extension)
    // 2. the scaling factor, including inserting the scaling factor into the
    // ViewProviderSketchGeometryExtension so as to enable That ensures that dragging operations on
    // the circles of the poles of the B-Splines are properly rendered.
    //
    // This function takes a reference to a vector of deep copies to delete. These deep copies are
    // necessary to transparently perform (1) while doing (2).

    scaleBSplinePoleCirclesAndUpdateSolverAndSketchObjectGeometry(geolistfacade, temp);

    // ============== Render geometry, constraints and geometry information overlays
    // ==================================

    editCoinManager->processGeometryConstraintsInformationOverlay(geolistfacade,
                                                                  rebuildinformationoverlay);

    // Avoids unneeded calls to pixmapFromSvg
    if (Mode == STATUS_NONE || Mode == STATUS_SKETCH_UseHandler) {
        editCoinManager->drawConstraintIcons(geolistfacade);
        editCoinManager->updateColor(geolistfacade);
    }

    Gui::MDIView* mdi = this->getActiveView();
    if (mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())) {
        static_cast<Gui::View3DInventor*>(mdi)->getViewer()->redraw();
    }
}

void ViewProviderSketch::setIsShownVirtualSpace(bool isshownvirtualspace)
{
    viewProviderParameters.isShownVirtualSpace = isshownvirtualspace;

    editCoinManager->updateVirtualSpace();

    signalConstraintsChanged();
}

bool ViewProviderSketch::getIsShownVirtualSpace() const
{
    return viewProviderParameters.isShownVirtualSpace;
}


void ViewProviderSketch::drawEdit(const std::vector<Base::Vector2d>& EditCurve)
{
    editCoinManager->drawEdit(EditCurve);
}

void ViewProviderSketch::drawEdit(const std::list<std::vector<Base::Vector2d>>& list)
{
    editCoinManager->drawEdit(list);
}

void ViewProviderSketch::drawEditMarkers(const std::vector<Base::Vector2d>& EditMarkers,
                                         unsigned int augmentationlevel)
{
    editCoinManager->drawEditMarkers(EditMarkers, augmentationlevel);
}

void ViewProviderSketch::updateData(const App::Property* prop)
{
    ViewProvider2DObject::updateData(prop);

    // In the case of an undo/redo transaction, updateData is triggered by
    // SketchObject::onUndoRedoFinished() in the solve() In the case of an internal transaction,
    // touching the geometry results in a call to updateData.
    if (isInEditMode() && !getSketchObject()->getDocument()->isPerformingTransaction()
        && !getSketchObject()->isPerformingInternalTransaction()
        && (prop == &(getSketchObject()->Geometry) || prop == &(getSketchObject()->Constraints))) {

        // At this point, we do not need to solve the Sketch
        // If we are adding geometry an update can be triggered before the sketch is actually
        // solved. Because a solve is mandatory to any addition (at least to update the DoF of the
        // solver), only when the solver geometry is the same in number than the sketch geometry an
        // update should trigger a redraw. This reduces even more the number of redraws per
        // insertion of geometry

        // solver information is also updated when no matching geometry, so that if a solving fails
        // this failed solving info is presented to the user
        UpdateSolverInformation();// just update the solver window with the last SketchObject
                                  // solving information

        if (getSketchObject()->getExternalGeometryCount()
                + getSketchObject()->getHighestCurveIndex() + 1
            == getSolvedSketch().getGeometrySize()) {
            Gui::MDIView* mdi = Gui::Application::Instance->editDocument()->getActiveView();
            if (mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId()))
                draw(false, true);

            signalConstraintsChanged();
        }

        if (prop != &getSketchObject()->Constraints)
            signalElementsChanged();
    }
}

void ViewProviderSketch::onChanged(const App::Property* prop)
{
    if (prop == &VisualLayerList) {
        if (isInEditMode()) {
            // Configure and rebuild Coin SceneGraph
            editCoinManager->updateGeometryLayersConfiguration();
        }
        return;
    }
    // call father
    ViewProviderPart::onChanged(prop);
}

void ViewProviderSketch::attach(App::DocumentObject* pcFeat)
{
    ViewProviderPart::attach(pcFeat);
}

void ViewProviderSketch::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    menu->addAction(tr("Edit sketch"), receiver, member);
    // Call the extensions
    Gui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProviderSketch::setEdit(int ModNum)
{
    Q_UNUSED(ModNum)
    // When double-clicking on the item for this sketch the
    // object unsets and sets its edit mode without closing
    // the task panel
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    TaskDlgEditSketch* sketchDlg = qobject_cast<TaskDlgEditSketch*>(dlg);
    if (sketchDlg && sketchDlg->getSketchView() != this)
        sketchDlg = nullptr;// another sketch left open its task panel
    if (dlg && !sketchDlg) {
        QMessageBox msgBox;
        msgBox.setText(tr("A dialog is already open in the task panel"));
        msgBox.setInformativeText(tr("Do you want to close this dialog?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes)
            Gui::Control().closeDialog();
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
        switch (box.exec()) {
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

    auto gridnode = getGridNode();
    Base::Placement plm = getEditingPlacement();
    setGridOrientation(plm.getPosition(), plm.getRotation());
    addNodeToRoot(gridnode);
    setGridEnabled(true);

    // create the container for the additional edit data
    assert(!isInEditMode());
    preselection.reset();
    selection.reset();
    editCoinManager = std::make_unique<EditModeCoinManager>(*this);
    snapManager = std::make_unique<SnapManager>(*this);

    auto editDoc = Gui::Application::Instance->editDocument();
    App::DocumentObject* editObj = getSketchObject();
    std::string editSubName;
    ViewProviderDocumentObject* editVp = nullptr;
    if (editDoc) {
        editDoc->getInEdit(&editVp, &editSubName);
        if (editVp)
            editObj = editVp->getObject();
    }

    // visibility automation
    try {
        Gui::Command::addModule(Gui::Command::Gui, "Show");
        try {
            QString cmdstr =
                QString::fromLatin1(
                    "ActiveSketch = App.getDocument('%1').getObject('%2')\n"
                    "tv = Show.TempoVis(App.ActiveDocument, tag= ActiveSketch.ViewObject.TypeId)\n"
                    "ActiveSketch.ViewObject.TempoVis = tv\n"
                    "if ActiveSketch.ViewObject.EditingWorkbench:\n"
                    "  tv.activateWorkbench(ActiveSketch.ViewObject.EditingWorkbench)\n"
                    "if ActiveSketch.ViewObject.HideDependent:\n"
                    "  tv.hide(tv.get_all_dependent(%3, '%4'))\n"
                    "if ActiveSketch.ViewObject.ShowSupport:\n"
                    "  tv.show([ref[0] for ref in ActiveSketch.Support if not "
                    "ref[0].isDerivedFrom(\"PartDesign::Plane\")])\n"
                    "if ActiveSketch.ViewObject.ShowLinks:\n"
                    "  tv.show([ref[0] for ref in ActiveSketch.ExternalGeometry])\n"
                    "tv.sketchClipPlane(ActiveSketch, ActiveSketch.ViewObject.SectionView)\n"
                    "tv.hide(ActiveSketch)\n"
                    "del(tv)\n"
                    "del(ActiveSketch)\n")
                    .arg(QString::fromLatin1(getDocument()->getDocument()->getName()),
                         QString::fromLatin1(getSketchObject()->getNameInDocument()),
                         QString::fromLatin1(Gui::Command::getObjectCmd(editObj).c_str()),
                         QString::fromLatin1(editSubName.c_str()));
            QByteArray cmdstr_bytearray = cmdstr.toLatin1();
            Gui::Command::runCommand(Gui::Command::Gui, cmdstr_bytearray);
        }
        catch (Base::PyException& e) {
            Base::Console().DeveloperError(
                "ViewProviderSketch", "setEdit: visibility automation failed with an error: \n");
            e.ReportException();
        }
    }
    catch (Base::PyException&) {
        Base::Console().DeveloperWarning(
            "ViewProviderSketch",
            "setEdit: could not import Show module. Visibility automation will not work.\n");
    }

    // start the edit dialog
    if (sketchDlg)
        Gui::Control().showDialog(sketchDlg);
    else
        Gui::Control().showDialog(new TaskDlgEditSketch(this));

    // This call to the solver is needed to initialize the DoF and solve time controls
    // The false parameter indicates that the geometry of the SketchObject shall not be updateData
    // so as not to trigger an onChanged that would set the document as modified and trigger a
    // recompute if we just close the sketch without touching anything.
    if (getSketchObject()->Support.getValue()) {
        if (!getSketchObject()->evaluateSupport())
            getSketchObject()->validateExternalLinks();
    }

    // There are geometry extensions introduced by the solver and geometry extensions introduced by
    // the viewprovider.
    // 1. It is important that the solver has geometry with updated extensions.
    // 2. It is important that the viewprovider has up-to-date solver information
    //
    // The decision is to maintain the "first solve then draw" order, which is consistent with the
    // rest of the Sketcher for example in geometry creation. Then, the ViewProvider is responsible
    // for updating the solver geometry when appropriate, as it is the ViewProvider that is
    // introducing its geometry extensions.
    //
    // In order to have updated solver information, solve must take "true", this cause the Geometry
    // property to be updated with the solver information, including solver extensions, and triggers
    // a draw(true) via ViewProvider::UpdateData.
    getSketchObject()->solve(true);

    //NOLINTBEGIN
    connectUndoDocument = getDocument()->signalUndoDocument.connect(
        std::bind(&ViewProviderSketch::slotUndoDocument, this, sp::_1));
    connectRedoDocument = getDocument()->signalRedoDocument.connect(
        std::bind(&ViewProviderSketch::slotRedoDocument, this, sp::_1));
    //NOLINTEND

    // Enable solver initial solution update while dragging.
    getSketchObject()->setRecalculateInitialSolutionWhileMovingPoint(
        viewProviderParameters.recalculateInitialSolutionWhileDragging);

    // intercept del key press from main app
    listener = new ShortcutListener(this);

    Gui::getMainWindow()->installEventFilter(listener);

    Workbench::enterEditMode();

    return true;
}

QString ViewProviderSketch::appendConflictMsg(const std::vector<int>& conflicting)
{
    return appendConstraintMsg(tr("Please remove the following constraint:"),
                               tr("Please remove at least one of the following constraints:"),
                               conflicting);
}

QString ViewProviderSketch::appendRedundantMsg(const std::vector<int>& redundant)
{
    return appendConstraintMsg(tr("Please remove the following redundant constraint:"),
                               tr("Please remove the following redundant constraints:"),
                               redundant);
}

QString ViewProviderSketch::appendPartiallyRedundantMsg(const std::vector<int>& partiallyredundant)
{
    return appendConstraintMsg(tr("The following constraint is partially redundant:"),
                               tr("The following constraints are partially redundant:"),
                               partiallyredundant);
}

QString ViewProviderSketch::appendMalformedMsg(const std::vector<int>& malformed)
{
    return appendConstraintMsg(tr("Please remove the following malformed constraint:"),
                               tr("Please remove the following malformed constraints:"),
                               malformed);
}

QString ViewProviderSketch::appendConstraintMsg(const QString& singularmsg,
                                                const QString& pluralmsg,
                                                const std::vector<int>& vector)
{
    QString msg;
    QTextStream ss(&msg);
    if (!vector.empty()) {
        if (vector.size() == 1)
            ss << singularmsg;
        else
            ss << pluralmsg;
        ss << "\n";
        ss << vector[0];
        for (unsigned int i = 1; i < vector.size(); i++)
            ss << ", " << vector[i];

        ss << "\n";
    }
    return msg;
}

inline QString intListHelper(const std::vector<int>& ints)
{
    QString results;
    if (ints.size() < 8) {// The 8 is a bit heuristic... more than that and we shift formats
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
        results.append(QCoreApplication::translate("ViewProviderSketch", "and %1 more").arg(more));
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
    bool hasMalformed = getSketchObject()->getLastHasMalformedConstraints();

    if (getSketchObject()->Geometry.getSize() == 0) {
        signalSetUp(QString::fromUtf8("empty_sketch"), tr("Empty sketch"), QString(), QString());
    }
    else if (dofs < 0 || hasConflicts) {// over-constrained sketch
        signalSetUp(
            QString::fromUtf8("conflicting_constraints"),
            tr("Over-constrained: "),
            QString::fromUtf8("#conflicting"),
            QString::fromUtf8("(%1)").arg(intListHelper(getSketchObject()->getLastConflicting())));
    }
    else if (hasMalformed) {// malformed constraints
        signalSetUp(QString::fromUtf8("malformed_constraints"),
                    tr("Malformed constraints: "),
                    QString::fromUtf8("#malformed"),
                    QString::fromUtf8("(%1)").arg(
                        intListHelper(getSketchObject()->getLastMalformedConstraints())));
    }
    else if (hasRedundancies) {
        signalSetUp(
            QString::fromUtf8("redundant_constraints"),
            tr("Redundant constraints:"),
            QString::fromUtf8("#redundant"),
            QString::fromUtf8("(%1)").arg(intListHelper(getSketchObject()->getLastRedundant())));
    }
    else if (hasPartiallyRedundant) {
        signalSetUp(QString::fromUtf8("partially_redundant_constraints"),
                    tr("Partially redundant:"),
                    QString::fromUtf8("#partiallyredundant"),
                    QString::fromUtf8("(%1)").arg(
                        intListHelper(getSketchObject()->getLastPartiallyRedundant())));
    }
    else if (getSketchObject()->getLastSolverStatus() != 0) {
        signalSetUp(QString::fromUtf8("solver_failed"),
                    tr("Solver failed to converge"),
                    QString::fromUtf8(""),
                    QString::fromUtf8(""));
    }
    else if (dofs > 0) {
        signalSetUp(QString::fromUtf8("under_constrained"),
                    tr("Under constrained:"),
                    QString::fromUtf8("#dofs"),
                    tr("%n DoF(s)", "", dofs));
    }
    else {
        signalSetUp(
            QString::fromUtf8("fully_constrained"), tr("Fully constrained"), QString(), QString());
    }
}

void ViewProviderSketch::unsetEdit(int ModNum)
{
    Q_UNUSED(ModNum);

    setGridEnabled(false);
    auto gridnode = getGridNode();
    pcRoot->removeChild(gridnode);

    Workbench::leaveEditMode();

    if (listener) {
        Gui::getMainWindow()->removeEventFilter(listener);
        delete listener;
    }

    if (isInEditMode()) {
        if (sketchHandler)
            deactivateHandler();

        editCoinManager = nullptr;
        snapManager = nullptr;
        preselection.reset();
        selection.reset();
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
    Gui::Selection().addSelection(editDocName.c_str(), editObjName.c_str(), editSubName.c_str());

    connectUndoDocument.disconnect();
    connectRedoDocument.disconnect();

    // when pressing ESC make sure to close the dialog
    Gui::Control().closeDialog();

    // visibility automation
    try {
        QString cmdstr =
            QString::fromLatin1("ActiveSketch = App.getDocument('%1').getObject('%2')\n"
                                "tv = ActiveSketch.ViewObject.TempoVis\n"
                                "if tv:\n"
                                "  tv.restore()\n"
                                "ActiveSketch.ViewObject.TempoVis = None\n"
                                "del(tv)\n"
                                "del(ActiveSketch)\n")
                .arg(QString::fromLatin1(getDocument()->getDocument()->getName()),
                     QString::fromLatin1(getSketchObject()->getNameInDocument()));
        QByteArray cmdstr_bytearray = cmdstr.toLatin1();
        Gui::Command::runCommand(Gui::Command::Gui, cmdstr_bytearray);
    }
    catch (Base::PyException& e) {
        Base::Console().DeveloperError(
            "ViewProviderSketch",
            "unsetEdit: visibility automation failed with an error: %s \n",
            e.what());
    }
}

void ViewProviderSketch::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);
    // visibility automation: save camera
    if (!this->TempoVis.getValue().isNone()) {
        try {
            QString cmdstr =
                QString::fromLatin1(
                    "ActiveSketch = App.getDocument('%1').getObject('%2')\n"
                    "if ActiveSketch.ViewObject.RestoreCamera:\n"
                    "  ActiveSketch.ViewObject.TempoVis.saveCamera()\n"
                    "  if ActiveSketch.ViewObject.ForceOrtho:\n"
                    "    "
                    "ActiveSketch.ViewObject.Document.ActiveView.setCameraType('Orthographic')\n")
                    .arg(QString::fromLatin1(getDocument()->getDocument()->getName()),
                         QString::fromLatin1(getSketchObject()->getNameInDocument()));
            QByteArray cmdstr_bytearray = cmdstr.toLatin1();
            Gui::Command::runCommand(Gui::Command::Gui, cmdstr_bytearray);
        }
        catch (Base::PyException& e) {
            Base::Console().DeveloperError(
                "ViewProviderSketch",
                "setEdit: visibility automation failed with an error: %s \n",
                e.what());
        }
    }

    auto editDoc = Gui::Application::Instance->editDocument();
    editDocName.clear();
    if (editDoc) {
        ViewProviderDocumentObject* parent = nullptr;
        editDoc->getInEdit(&parent, &editSubName);
        if (parent) {
            editDocName = editDoc->getDocument()->getName();
            editObjName = parent->getObject()->getNameInDocument();
        }
    }
    if (editDocName.empty()) {
        editDocName = getObject()->getDocument()->getName();
        editObjName = getObject()->getNameInDocument();
        editSubName.clear();
    }
    const char* dot = strrchr(editSubName.c_str(), '.');
    if (!dot)
        editSubName.clear();
    else
        editSubName.resize(dot - editSubName.c_str() + 1);

    Base::Placement plm = getEditingPlacement();
    Base::Rotation tmp(plm.getRotation());

    SbRotation rot((float)tmp[0], (float)tmp[1], (float)tmp[2], (float)tmp[3]);

    // Will the sketch be visible from the new position (#0000957)?
    //
    SoCamera* camera = viewer->getSoRenderManager()->getCamera();
    SbVec3f curdir;// current view direction
    camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), curdir);
    SbVec3f focal = camera->position.getValue() + camera->focalDistance.getValue() * curdir;

    SbVec3f newdir;// future view direction
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
    viewer->setSelectionEnabled(false);

    viewer->addGraphicsItem(rubberband.get());
    rubberband->setViewer(viewer);

    viewer->setupEditingRoot();

    cameraSensor.setData(new VPRender {this, viewer->getSoRenderManager()});
    cameraSensor.attach(viewer->getSoRenderManager()->getSceneGraph());
}

void ViewProviderSketch::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    auto dataPtr = static_cast<VPRender*>(cameraSensor.getData());
    delete dataPtr;
    cameraSensor.setData(nullptr);
    cameraSensor.detach();

    viewer->removeGraphicsItem(rubberband.get());
    viewer->setEditing(false);
    viewer->setSelectionEnabled(true);
}

void ViewProviderSketch::camSensCB(void* data, SoSensor*)
{
    VPRender* proxyVPrdr = static_cast<VPRender*>(data);
    if (!proxyVPrdr)
        return;

    auto vp = proxyVPrdr->vp;
    auto cam = proxyVPrdr->renderMgr->getCamera();

    vp->onCameraChanged(cam);
}

void ViewProviderSketch::onCameraChanged(SoCamera* cam)
{
    auto rotSk = Base::Rotation(getDocument()->getEditingTransform());// sketch orientation
    auto rotc = cam->orientation.getValue().getValue();
    auto rotCam =
        Base::Rotation(rotc[0],
                       rotc[1],
                       rotc[2],
                       rotc[3]);// camera orientation (needed because float to double conversion)

    // Is camera in the same hemisphere as positive sketch normal ?
    auto orientation = (rotCam.invert() * rotSk).multVec(Base::Vector3d(0, 0, 1));
    auto tmpFactor = orientation.z < 0 ? -1 : 1;

    if (tmpFactor != viewOrientationFactor) {// redraw only if viewing side changed
        Base::Console().Log("Switching side, now %s, redrawing\n",
                            tmpFactor < 0 ? "back" : "front");
        viewOrientationFactor = tmpFactor;
        draw();

        QString cmdStr = QStringLiteral("ActiveSketch.ViewObject.TempoVis.sketchClipPlane("
                                        "ActiveSketch, ActiveSketch.ViewObject.SectionView, %1)\n")
                             .arg(tmpFactor < 0 ? QLatin1String("True") : QLatin1String("False"));
        Base::Interpreter().runStringObject(cmdStr.toLatin1());
    }

    drawGrid(true);
}

int ViewProviderSketch::getPreselectPoint() const
{
    if (isInEditMode())
        return preselection.PreselectPoint;
    return -1;
}

int ViewProviderSketch::getPreselectCurve() const
{
    if (isInEditMode())
        return preselection.PreselectCurve;
    return -1;
}

int ViewProviderSketch::getPreselectCross() const
{
    // TODO: This function spreads over several files. It should be refactored into something less
    // "numeric" at a second stage.
    if (isInEditMode())
        return static_cast<int>(preselection.PreselectCross);
    return -1;
}

Sketcher::SketchObject* ViewProviderSketch::getSketchObject() const
{
    return dynamic_cast<Sketcher::SketchObject*>(pcObject);
}

const Sketcher::Sketch& ViewProviderSketch::getSolvedSketch() const
{
    return const_cast<const Sketcher::SketchObject*>(getSketchObject())->getSolvedSketch();
}

void ViewProviderSketch::deleteSelected()
{
    std::vector<Gui::SelectionObject> selection;
    selection = Gui::Selection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        Base::Console().DeveloperWarning(
            "ViewProviderSketch",
            "Delete: Selection not restricted to one sketch and its subelements\n");
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();

    if (!SubNames.empty()) {
        App::Document* doc = getSketchObject()->getDocument();

        doc->openTransaction("Delete sketch geometry");

        onDelete(SubNames);

        doc->commitTransaction();
    }
}

bool ViewProviderSketch::onDelete(const std::vector<std::string>& subList)
{
    if (isInEditMode()) {
        std::vector<std::string> SubNames = subList;

        Gui::Selection().clearSelection();
        resetPreselectPoint();

        std::set<int> delInternalGeometries, delExternalGeometries, delCoincidents, delConstraints;
        // go through the selected subelements
        for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end();
             ++it) {
            if (it->size() > 4 && it->substr(0, 4) == "Edge") {
                int GeoId = std::atoi(it->substr(4, 4000).c_str()) - 1;
                if (GeoId >= 0) {
                    delInternalGeometries.insert(GeoId);
                }
                else
                    delExternalGeometries.insert(Sketcher::GeoEnum::RefExt - GeoId);
            }
            else if (it->size() > 12 && it->substr(0, 12) == "ExternalEdge") {
                int GeoId = std::atoi(it->substr(12, 4000).c_str()) - 1;
                delExternalGeometries.insert(GeoId);
            }
            else if (it->size() > 6 && it->substr(0, 6) == "Vertex") {
                int VtId = std::atoi(it->substr(6, 4000).c_str()) - 1;
                int GeoId;
                Sketcher::PointPos PosId;
                getSketchObject()->getGeoVertexIndex(VtId, GeoId, PosId);
                if (getSketchObject()->getGeometry(GeoId)->getTypeId()
                    == Part::GeomPoint::getClassTypeId()) {
                    if (GeoId >= 0)
                        delInternalGeometries.insert(GeoId);
                    else
                        delExternalGeometries.insert(Sketcher::GeoEnum::RefExt - GeoId);
                }
                else
                    delCoincidents.insert(VtId);
            }
            else if (*it == "RootPoint") {
                delCoincidents.insert(Sketcher::GeoEnum::RtPnt);
            }
            else if (it->size() > 10 && it->substr(0, 10) == "Constraint") {
                int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(*it);
                delConstraints.insert(ConstrId);
            }
        }

        // We stored the vertices, but is there really a coincident constraint? Check
        const std::vector<Sketcher::Constraint*>& vals = getSketchObject()->Constraints.getValues();

        std::set<int>::const_reverse_iterator rit;

        for (rit = delConstraints.rbegin(); rit != delConstraints.rend(); ++rit) {
            try {
                Gui::cmdAppObjectArgs(getObject(), "delConstraint(%d)", *rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().DeveloperError("ViewProviderSketch", "%s\n", e.what());
            }
        }

        for (rit = delCoincidents.rbegin(); rit != delCoincidents.rend(); ++rit) {
            int GeoId;
            PointPos PosId;

            if (*rit == GeoEnum::RtPnt) {// RootPoint
                GeoId = Sketcher::GeoEnum::RtPnt;
                PosId = Sketcher::PointPos::start;
            }
            else {
                getSketchObject()->getGeoVertexIndex(*rit, GeoId, PosId);
            }

            if (GeoId != GeoEnum::GeoUndef) {
                for (std::vector<Sketcher::Constraint*>::const_iterator it = vals.begin();
                     it != vals.end();
                     ++it) {
                    if (((*it)->Type == Sketcher::Coincident)
                        && (((*it)->First == GeoId && (*it)->FirstPos == PosId)
                            || ((*it)->Second == GeoId && (*it)->SecondPos == PosId))) {
                        try {
                            Gui::cmdAppObjectArgs(
                                getObject(), "delConstraintOnPoint(%d,%d)", GeoId, (int)PosId);
                        }
                        catch (const Base::Exception& e) {
                            Base::Console().DeveloperError("ViewProviderSketch", "%s\n", e.what());
                        }
                        break;
                    }
                }
            }
        }

        if (!delInternalGeometries.empty()) {
            std::stringstream stream;

            // NOTE: SketchObject delGeometries will sort the array, so filling it here with a
            // reverse iterator would lead to the worst case scenario for sorting.
            auto endit = std::prev(delInternalGeometries.end());
            for (auto it = delInternalGeometries.begin(); it != endit; ++it) {
                stream << *it << ",";
            }

            stream << *endit;

            try {
                Gui::cmdAppObjectArgs(getObject(), "delGeometries([%s])", stream.str().c_str());
            }
            catch (const Base::Exception& e) {
                Base::Console().DeveloperError("ViewProviderSketch", "%s\n", e.what());
            }

            stream.str(std::string());
        }

        for (rit = delExternalGeometries.rbegin(); rit != delExternalGeometries.rend(); ++rit) {
            try {
                Gui::cmdAppObjectArgs(getObject(), "delExternal(%d)", *rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().DeveloperError("ViewProviderSketch", "%s\n", e.what());
            }
        }

        int ret = getSketchObject()->solve();

        if (ret != 0) {
            // if the sketched could not be solved, we first redraw to update the UI geometry as
            // onChanged did not update it.
            UpdateSolverInformation();
            draw(false, true);

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

        if (viewProviderParameters.autoRecompute) {
            Gui::Command::updateActive();
        }
        else {
            editCoinManager->drawConstraintIcons();
            this->updateColor();
        }

        // if in edit not delete the object
        return false;
    }
    // if not in edit delete the whole object
    return PartGui::ViewProviderPart::onDelete(subList);
}

QIcon ViewProviderSketch::mergeColorfulOverlayIcons(const QIcon& orig) const
{
    QIcon mergedicon = orig;

    if (!getSketchObject()->FullyConstrained.getValue()) {
        QPixmap px;

        static const char* const sketcher_notfullyconstrained_xpm[] = {"9 9 3 1",
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
        px = QPixmap(sketcher_notfullyconstrained_xpm);

        mergedicon = Gui::BitmapFactoryInst::mergePixmap(
            mergedicon, px, Gui::BitmapFactoryInst::BottomRight);
    }

    return Gui::ViewProvider::mergeColorfulOverlayIcons(mergedicon);
}

/*************************** functions ViewProviderSketch offers to friends such as
 * DrawHandlerSketch ************************/

void ViewProviderSketch::setConstraintSelectability(bool enabled /*= true*/)
{
    editCoinManager->setConstraintSelectability(enabled);
}

void ViewProviderSketch::setPositionText(const Base::Vector2d& Pos, const SbString& text)
{
    editCoinManager->setPositionText(Pos, text);
}

void ViewProviderSketch::setPositionText(const Base::Vector2d& Pos)
{
    editCoinManager->setPositionText(Pos);
}

void ViewProviderSketch::resetPositionText()
{
    editCoinManager->resetPositionText();
}

void ViewProviderSketch::setPreselectPoint(int PreselectPoint)
{
    preselection.PreselectPoint = PreselectPoint;
    preselection.PreselectCurve = Preselection::InvalidCurve;
    preselection.PreselectCross = Preselection::Axes::None;
    ;
    preselection.PreselectConstraintSet.clear();
}

void ViewProviderSketch::setPreselectRootPoint()
{
    preselection.PreselectPoint = Preselection::InvalidPoint;
    preselection.PreselectCurve = Preselection::InvalidCurve;
    preselection.PreselectCross = Preselection::Axes::RootPoint;
    preselection.PreselectConstraintSet.clear();
}


void ViewProviderSketch::resetPreselectPoint()
{
    preselection.PreselectPoint = Preselection::InvalidPoint;
    preselection.PreselectCurve = Preselection::InvalidCurve;
    preselection.PreselectCross = Preselection::Axes::None;
    ;
    preselection.PreselectConstraintSet.clear();
}

void ViewProviderSketch::addSelectPoint(int SelectPoint)
{
    selection.SelPointSet.insert(SelectPoint);
}

void ViewProviderSketch::removeSelectPoint(int SelectPoint)
{
    selection.SelPointSet.erase(SelectPoint);
}

void ViewProviderSketch::clearSelectPoints()
{
    selection.SelPointSet.clear();
}

bool ViewProviderSketch::isSelected(const std::string& subNameSuffix) const
{
    return Gui::Selection().isSelected(
        editDocName.c_str(), editObjName.c_str(), (editSubName + subNameSuffix).c_str());
}

void ViewProviderSketch::rmvSelection(const std::string& subNameSuffix)
{
    Gui::Selection().rmvSelection(
        editDocName.c_str(), editObjName.c_str(), (editSubName + subNameSuffix).c_str());
}

bool ViewProviderSketch::addSelection(const std::string& subNameSuffix, float x, float y, float z)
{
    return Gui::Selection().addSelection(
        editDocName.c_str(), editObjName.c_str(), (editSubName + subNameSuffix).c_str(), x, y, z);
}

bool ViewProviderSketch::addSelection2(const std::string& subNameSuffix, float x, float y, float z)
{
    return Gui::Selection().addSelection2(
        editDocName.c_str(), editObjName.c_str(), (editSubName + subNameSuffix).c_str(), x, y, z);
}

bool ViewProviderSketch::setPreselect(const std::string& subNameSuffix, float x, float y, float z)
{
    return Gui::Selection().setPreselect(
        editDocName.c_str(), editObjName.c_str(), (editSubName + subNameSuffix).c_str(), x, y, z);
}

/*************************** private functions to decouple Attorneys and Clients
 * ********************************************/

// Establishes a private collaboration interface with EditModeCoinManager to perform
// EditModeCoinManager tasks, while abstracting EditModeCoinManager from the specific
// ViewProviderSketch implementation, while allowing ViewProviderSketch to fully delegate coin
// management.

const std::vector<Sketcher::Constraint*> ViewProviderSketch::getConstraints() const
{
    return getSketchObject()->Constraints.getValues();
}

const GeoList ViewProviderSketch::getGeoList() const
{
    const std::vector<Part::Geometry*> tempGeo =
        getSketchObject()->getCompleteGeometry();// without memory allocation

    int intGeoCount = getSketchObject()->getHighestCurveIndex() + 1;

    auto geolist = GeoList::getGeoListModel(std::move(tempGeo), intGeoCount);

    return geolist;
}

bool ViewProviderSketch::constraintHasExpression(int constrid) const
{
    return getSketchObject()->constraintHasExpression(constrid);
}

std::unique_ptr<SoRayPickAction> ViewProviderSketch::getRayPickAction() const
{
    assert(isInEditMode());
    Gui::MDIView* mdi =
        Gui::Application::Instance->editViewOfNode(editCoinManager->getRootEditNode());
    if (!(mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())))
        return nullptr;
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(mdi)->getViewer();

    return std::make_unique<SoRayPickAction>(viewer->getSoRenderManager()->getViewportRegion());
}

SbVec2f ViewProviderSketch::getScreenCoordinates(SbVec2f sketchcoordinates) const
{

    Base::Placement sketchPlacement = getEditingPlacement();
    Base::Vector3d sketchPos(sketchPlacement.getPosition());
    Base::Rotation sketchRot(sketchPlacement.getRotation());

    // get global coordinates from sketcher coordinates
    Base::Vector3d pos(sketchcoordinates[0], sketchcoordinates[1], 0);
    sketchRot.multVec(pos, pos);
    pos = pos + sketchPos;

    Gui::MDIView* mdi = this->getActiveView();
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(mdi);
    if (!view || !isInEditMode())
        return SbVec2f(0, 0);

    Gui::View3DInventorViewer* viewer = view->getViewer();

    SoCamera* pCam = viewer->getSoRenderManager()->getCamera();

    if (!pCam)
        return SbVec2f(0, 0);

    SbViewVolume vol = pCam->getViewVolume();
    Gui::ViewVolumeProjection proj(vol);

    // dimensionless [0 1] (or 1.5 see View3DInventorViewer.cpp )
    Base::Vector3d screencoords = proj(pos);

    int width = viewer->getGLWidget()->width(), height = viewer->getGLWidget()->height();

    if (width >= height) {
        // "Landscape" orientation, to square
        screencoords.x *= height;
        screencoords.x += (width - height) / 2.0;
        screencoords.y *= height;
    }
    else {
        // "Portrait" orientation
        screencoords.x *= width;
        screencoords.y *= width;
        screencoords.y += (height - width) / 2.0;
    }

    SbVec2f iconCoords(screencoords.x, screencoords.y);

    return iconCoords;
}

QFont ViewProviderSketch::getApplicationFont() const
{
    return QApplication::font();
}

int ViewProviderSketch::defaultFontSizePixels() const
{
    QFontMetricsF metrics(QApplication::font());
    return static_cast<int>(metrics.height());
}

int ViewProviderSketch::getApplicationLogicalDPIX() const
{
    return int(QApplication::primaryScreen()->logicalDotsPerInchX());
}

int ViewProviderSketch::getViewOrientationFactor() const
{
    return viewOrientationFactor;
}

double ViewProviderSketch::getRotation(SbVec3f pos0, SbVec3f pos1) const
{
    double x0, y0, x1, y1;

    Gui::MDIView* mdi =
        Gui::Application::Instance->editViewOfNode(editCoinManager->getRootEditNode());
    if (!(mdi && mdi->isDerivedFrom(Gui::View3DInventor::getClassTypeId())))
        return 0;
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(mdi)->getViewer();
    SoCamera* pCam = viewer->getSoRenderManager()->getCamera();
    if (!pCam)
        return 0;

    try {
        SbViewVolume vol = pCam->getViewVolume();

        getCoordsOnSketchPlane(pos0, vol.getProjectionDirection(), x0, y0);
        getCoordsOnSketchPlane(pos1, vol.getProjectionDirection(), x1, y1);

        return -atan2((y1 - y0), (x1 - x0)) * 180 / M_PI;
    }
    catch (const Base::ZeroDivisionError&) {
        return 0;
    }
}


GeoListFacade ViewProviderSketch::getGeoListFacade() const
{
    return getSketchObject()->getGeoListFacade();
}

bool ViewProviderSketch::isSketchInvalid() const
{

    bool sketchinvalid = getSketchObject()->getLastHasRedundancies()
        || getSketchObject()->getLastHasConflicts()
        || getSketchObject()->getLastHasMalformedConstraints();
    return sketchinvalid;
}

bool ViewProviderSketch::isSketchFullyConstrained() const
{
    return getSketchObject()->FullyConstrained.getValue();
}

bool ViewProviderSketch::haveConstraintsInvalidGeometry() const
{
    return getSketchObject()->Constraints.hasInvalidGeometry();
}

void ViewProviderSketch::addNodeToRoot(SoSeparator* node)
{
    pcRoot->addChild(node);
}

void ViewProviderSketch::removeNodeFromRoot(SoSeparator* node)
{
    pcRoot->removeChild(node);
}

bool ViewProviderSketch::isConstraintPreselected(int constraintId) const
{
    return preselection.PreselectConstraintSet.count(constraintId);
}

bool ViewProviderSketch::isPointSelected(int pointId) const
{
    return selection.SelPointSet.find(pointId) != selection.SelPointSet.end();
}

bool ViewProviderSketch::isCurveSelected(int curveId) const
{
    return selection.SelCurvSet.find(curveId) != selection.SelCurvSet.end();
}

bool ViewProviderSketch::isConstraintSelected(int constraintId) const
{
    return selection.SelConstraintSet.find(constraintId) != selection.SelConstraintSet.end();
}

void ViewProviderSketch::executeOnSelectionPointSet(
    std::function<void(const int)>&& operation) const
{
    for (const auto v : selection.SelPointSet)
        operation(v);
}

bool ViewProviderSketch::isInEditMode() const
{
    return editCoinManager != nullptr;
}
// clang-format on
