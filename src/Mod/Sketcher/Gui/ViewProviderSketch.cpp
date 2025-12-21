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

#include <boost/core/ignore_unused.hpp>
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
#include <QListWidget>
#include <QMenu>
#include <QMessageBox>
#include <QScreen>
#include <QTextStream>
#include <QWindow>

#include <limits>

#include <fmt/format.h>

#include <Base/Console.h>
#include <Base/ServiceProvider.h>
#include <Base/Vector3D.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
// #include <Gui/Inventor/SoFCSwitch.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/GeoList.h>
#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/SolverGeometryExtension.h>

#include "DrawSketchHandler.h"
#include "EditDatumDialog.h"
#include "EditModeCoinManager.h"
#include "SnapManager.h"
#include "StyleParameters.h"
#include "TaskDlgEditSketch.h"
#include "TaskSketcherValidation.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "ViewProviderSketchGeometryExtension.h"
#include "Workbench.h"

#include <Mod/Part/Gui/SoFCShapeObject.h>


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

template<typename T>
T getPreferencesViewParameter(const std::string& string, T defaultvalue)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");

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

    Base::Color elementAppColor = colorprop->getValue();
    unsigned long color = (unsigned long)(elementAppColor.getPackedValue());
    color = hGrp->GetUnsigned(string.c_str(), color);
    elementAppColor.setPackedValue((uint32_t)color);
    colorprop->setValue(elementAppColor);
}

void ViewProviderSketch::ParameterObserver::updateShapeAppearanceProperty(const std::string& string, App::Property* property)
{
    auto matProp = static_cast<App::PropertyMaterialList*>(property);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    unsigned long shcol = hGrp->GetUnsigned(string.c_str(), 0x54abff40);
    float r = ((shcol >> 24) & 0xff) / 255.0;
    float g = ((shcol >> 16) & 0xff) / 255.0;
    float b = ((shcol >> 8) & 0xff) / 255.0;
    float a = (shcol & 0xff) / 255.0;
    matProp->setDiffuseColor(r, g, b);
    matProp->setTransparency(1 - a);
}

void ViewProviderSketch::ParameterObserver::updateGridSize(const std::string& string,
                                                           App::Property* property)
{
    (void)property;
    (void)string;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Sketcher/General");

    Client.GridSize.setValue(
        Base::Quantity::parse(hGrp->GetGroup("GridSize")->GetASCII("GridSize", "10.0"))
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
        Base::Console().developerError(
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
        Base::Console().developerError(
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
        {"LeaveSketchWithEscape",
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
              auto color = Base::Color(v);
              Client.setGridLineColor(color);
          },
          nullptr}},
        {"GridDivLineColor",
         {[this, packedDefaultGridColor](const std::string& string,
                                         [[maybe_unused]] App::Property* property) {
              auto v = getSketcherGeneralParameter(string, packedDefaultGridColor);
              auto color = Base::Color(v);
              Client.setGridDivLineColor(color);
          },
          nullptr}},
        {"SegmentsPerGeometry",
         {[this](const std::string& string,
                                         [[maybe_unused]] App::Property* property) {
              auto v = getPreferencesViewParameter(string, 50); //LINT
              Client.viewProviderParameters.stdCountSegments = v;
          },
          nullptr}},
        {"SketchEdgeColor",
         {[this](const std::string& string, App::Property* property) {
              if (Client.AutoColor.getValue()) {
                  updateColorProperty(string, property, 1.0f, 1.0f, 1.0f);
              }
          },
          &Client.LineColor}},
        {"SketchVertexColor",
         {[this](const std::string& string, App::Property* property) {
              if (Client.AutoColor.getValue()) {
                  updateColorProperty(string, property, 1.0f, 1.0f, 1.0f);
              }
          },
          &Client.PointColor}},
        {"SketchFaceColor",
         {[this](const std::string& string, App::Property* property) {
              if (Client.AutoColor.getValue()) {
                  updateShapeAppearanceProperty(string, property);
              }
          },
          &Client.ShapeAppearance}},
    };

    for (auto& val : parameterMap) {
        auto string = val.first;
        auto update = std::get<0>(val.second);
        auto property = std::get<1>(val.second);

        update(string, property);
    }

    // unsubscribed parameters which update a property on just once upon construction (and before
    // restore if properties are being restored from a file)
    updateBoolProperty("ShowGrid", &Client.ShowGrid, false);
    updateBoolProperty("GridAuto", &Client.GridAuto, true);
    updateGridSize("GridSize", &Client.GridSize);
}

void ViewProviderSketch::ParameterObserver::OnChange(Base::Subject<const char*>& rCaller,
                                                     const char* sReason)
{
    (void)rCaller;

    updateFromParameter(sReason);
}

void SketcherGui::ViewProviderSketch::ParameterObserver::updateFromParameter(const char* parameter)
{
    auto key = parameterMap.find(parameter);

    if (key != parameterMap.end()) {
        auto string = key->first;
        auto update = std::get<0>(key->second);
        auto property = std::get<1>(key->second);

        update(string, property);
    }
}


/************** ViewProviderSketch::ToolManager *********************/
ViewProviderSketch::ToolManager::ToolManager(ViewProviderSketch * vp): vp(vp)
{}

std::unique_ptr<QWidget> ViewProviderSketch::ToolManager::createToolWidget() const
{
    if(vp && vp->sketchHandler) {
        return vp->sketchHandler->createToolWidget();
    }
    else {
        return nullptr;
    }
}

bool ViewProviderSketch::ToolManager::isWidgetVisible() const
{
    if(vp && vp->sketchHandler) {
        return vp->sketchHandler->isWidgetVisible();
    }
    else {
        return false;
    }
}

QPixmap ViewProviderSketch::ToolManager::getToolIcon() const
{
    if(vp && vp->sketchHandler) {
        return vp->sketchHandler->getToolIcon();
    }
    else {
        return QPixmap();
    }
}

QString ViewProviderSketch::ToolManager::getToolWidgetText() const
{
    if(vp && vp->sketchHandler) {
        return vp->sketchHandler->getToolWidgetText();
    }
    else {
        return QString();
    }
}

/*************************** SoSketchFaces **************************/

SO_NODE_SOURCE(SoSketchFaces);

SoSketchFaces::SoSketchFaces(){
    SO_NODE_CONSTRUCTOR(SoSketchFaces);

    SO_NODE_ADD_FIELD(color, (SbColor(1.0f, 1.0f, 1.0f)));
    SO_NODE_ADD_FIELD(transparency, (0.8));
    //
    auto* material = new SoMaterial;
    material->diffuseColor.connectFrom(&color);
    material->transparency.connectFrom(&transparency);

    SoSeparator::addChild(material);
    SoSeparator::addChild(coords);
    SoSeparator::addChild(norm);
    SoSeparator::addChild(faceset);
}

void SoSketchFaces::initClass()
{
    SO_NODE_INIT_CLASS(SoSketchFaces, SoFCShape, "FCShape");
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
    , toolManager(this)
    , Mode(STATUS_NONE)
    , pcSketchFaces(new SoSketchFaces)
    , pcSketchFacesToggle(new SoToggleSwitch)
    , listener(nullptr)
    , editCoinManager(nullptr)
    , snapManager(nullptr)
    , pObserver(std::make_unique<ViewProviderSketch::ParameterObserver>(*this))
    , sketchHandler(nullptr)
    , viewOrientationFactor(1)
    , blockContextMenu(false)
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

    ADD_PROPERTY_TYPE(
        AutoColor,
        (true),
        "Object Style",
        (App::PropertyType)(App::Prop_None),
        "If true, this sketch will be colored based on user preferences. Turn it off to set color explicitly.");

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

    updateColorPropertiesVisibility();

    pcSketchFacesToggle->addChild(pcSketchFaces);
}

ViewProviderSketch::~ViewProviderSketch()
{
    connectionToolWidget.disconnect();
}

void ViewProviderSketch::slotUndoDocument(const Gui::Document& /*doc*/)
{
    // If undo is triggered during a drag operation, the drag must be canceled to avoid using
    // stale geometry IDs, which would lead to a crash.
    if (!drag.Dragged.empty() || !drag.DragConstraintSet.empty()) {
        drag.reset();
        setSketchMode(STATUS_NONE);
        resetPositionText();
    }

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
    // If redo is triggered during a drag operation, the drag must be canceled to avoid using
    // stale geometry IDs, which would lead to a crash.
    if (!drag.Dragged.empty() || !drag.DragConstraintSet.empty()) {
        drag.reset();
        setSketchMode(STATUS_NONE);
        resetPositionText();
    }

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

void ViewProviderSketch::activateHandler(std::unique_ptr<DrawSketchHandler> newHandler)
{
    assert(editCoinManager);
    assert(!sketchHandler);

    sketchHandler = std::move(newHandler);
    setSketchMode(STATUS_SKETCH_UseHandler);
    sketchHandler->activate(this);

    // make sure receiver has focus so immediately pressing Escape will be handled by
    // ViewProviderSketch::keyPressed() and dismiss the active handler, and not the entire
    // sketcher editor
    ensureFocus();
}

void ViewProviderSketch::deactivateHandler()
{
    assert(isInEditMode());
    if (sketchHandler) {
        sketchHandler->deactivate();
        sketchHandler = nullptr;
    }
    setSketchMode(STATUS_NONE);
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

    // Give back the focus to the MDI to make sure VPSketch receive keyboard events.
    ensureFocus();
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

void ViewProviderSketch::ensureFocus()
{
    Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
    mdi->setFocus();
}

void ViewProviderSketch::preselectAtPoint(Base::Vector2d point)
{
    if (Mode != STATUS_SELECT_Point && Mode != STATUS_SELECT_Edge
        && Mode != STATUS_SELECT_Constraint && Mode != STATUS_SKETCH_Drag
        && Mode != STATUS_SKETCH_DragConstraint
        && Mode != STATUS_SKETCH_UseRubberBand) {

        Gui::MDIView* mdi = this->getActiveView();
        Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(mdi);

        if (!view)
            return;

        Gui::View3DInventorViewer* viewer = view->getViewer();

        Base::Placement Plm = getEditingPlacement();

        auto inPlacementCoords = [&Plm](const Base::Vector3d & point) {
            Base::Vector3d pnt;
            Plm.multVec(point, pnt);
            return pnt;
        };

        auto pnt = inPlacementCoords(Base::Vector3d(point.x,point.y,0));

        SbVec3f sbpoint(static_cast<float>(pnt.x), static_cast<float>(pnt.y), static_cast<float>(pnt.z));

        SbVec2s screencoords = viewer->getPointOnViewport(sbpoint);

        std::unique_ptr<SoPickedPoint> Point(this->getPointOnRay(screencoords, viewer));

        if (detectAndShowPreselection(Point.get()) && sketchHandler) {
            sketchHandler->applyCursor();
        }
    }
}

// **********************************************************************************

void ViewProviderSketch::setSketchMode(SketchMode mode)
{
    Mode = mode;
    Gui::Application::Instance->commandManager().testActive();
}

bool ViewProviderSketch::keyPressed(bool pressed, int key)
{
    switch (key) {
        case SoKeyboardEvent::ESCAPE: {
            // make the handler quit but not the edit mode
            if (isInEditMode() && sketchHandler) {
                sketchHandler->registerPressedKey(pressed, key); // delegate
                return true;
            }
            if (isInEditMode() && !drag.DragConstraintSet.empty()) {
                if (!pressed) {
                    drag.DragConstraintSet.clear();
                }
                return true;
            }
            if (isInEditMode() && !drag.Dragged.empty()) {
                if (!pressed) {
                    commitDragMove(drag.xInit, drag.yInit);
                    setSketchMode(STATUS_NONE);
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
    if (fabs(RN * RA) < std::numeric_limits<float>::epsilon())
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

    std::unique_ptr<SnapManager::SnapHandle> snapHandle;
    try {
        getCoordsOnSketchPlane(pos, normal, x, y);
        snapHandle = std::make_unique<SnapManager::SnapHandle>(snapManager.get(), Base::Vector2d(x, y));
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
                        // Base::Console().log("start dragging, point:%d\n",this->DragPoint);
                        setSketchMode(STATUS_SELECT_Point);
                        done = true;
                    }
                    else if (preselection.isPreselectCurveValid()) {
                        // Base::Console().log("start dragging, point:%d\n",this->DragPoint);
                        setSketchMode(STATUS_SELECT_Edge);
                        done = true;
                    }
                    else if (preselection.isCrossPreselected()) {
                        // Base::Console().log("start dragging, point:%d\n",this->DragPoint);
                        setSketchMode(STATUS_SELECT_Cross);
                        done = true;
                    }
                    else if (!preselection.PreselectConstraintSet.empty()) {
                        // Base::Console().log("start dragging, point:%d\n",this->DragPoint);
                        setSketchMode(STATUS_SELECT_Constraint);
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
                        if (Mode != STATUS_SELECT_Wire) {
                            setSketchMode(STATUS_NONE);
                        }
                    }
                    else {
                        DoubleClick::prvClickTime = SbTime::getTimeOfDay();
                        DoubleClick::prvClickPos = cursorPos;
                        DoubleClick::prvCursorPos = cursorPos;
                        DoubleClick::newCursorPos = cursorPos;
                        if (!done)
                            setSketchMode(STATUS_SKETCH_StartRubberBand);
                    }

                    return done;
                }
                case STATUS_SKETCH_UseHandler: {
                    Base::Vector2d snappedPos = snapHandle->compute();
                    return sketchHandler->pressButton(snappedPos);
                }
                default:
                    return false;
            }
        }
        else {// Button 1 released
            // Do things depending on the mode of the user interaction
            switch (Mode) {
                case STATUS_SELECT_Point:
                    if (pp) {
                        // Base::Console().log("Select Point:%d\n",this->DragPoint);
                        //  Do selection
                        std::stringstream ss;
                        ss << "Vertex" << preselection.getPreselectionVertexIndex();

                        preselectToSelection(ss, pp, true);
                    }
                    setSketchMode(STATUS_NONE);
                    return true;
                case STATUS_SELECT_Edge:
                    if (pp) {
                        // Base::Console().log("Select Point:%d\n",this->DragPoint);
                        std::stringstream ss;
                        if (preselection.isEdge())
                            ss << "Edge" << preselection.getPreselectionEdgeIndex();
                        else// external geometry
                            ss << "ExternalEdge" << preselection.getPreselectionExternalEdgeIndex();

                        preselectToSelection(ss, pp, true);
                    }
                    setSketchMode(STATUS_NONE);
                    return true;
                case STATUS_SELECT_Cross:
                    if (pp) {
                        // Base::Console().log("Select Point:%d\n",this->DragPoint);
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

                        preselectToSelection(ss, pp, true);
                    }
                    setSketchMode(STATUS_NONE);
                    return true;
                case STATUS_SELECT_Wire: {
                    toggleWireSelelection(preselection.PreselectCurve);
                    setSketchMode(STATUS_NONE);
                    return true;
                }
                case STATUS_SELECT_Constraint:
                    if (pp) {
                        auto sels = preselection.PreselectConstraintSet;
                        for (int id : sels) {
                            std::stringstream ss;
                            ss << Sketcher::PropertyConstraintList::getConstraintName(id);

                            preselectToSelection(ss, pp, true);
                        }
                    }
                    setSketchMode(STATUS_NONE);
                    return true;
                case STATUS_SKETCH_Drag:
                    commitDragMove(x, y);
                    setSketchMode(STATUS_NONE);
                    return true;
                case STATUS_SKETCH_DragConstraint:
                    if (!drag.DragConstraintSet.empty()) {
                        auto idset = drag.DragConstraintSet;
                        // restore the old positions before opening the transaction and setting the new positions
                        for (int id : idset) {
                            moveConstraint(id, Base::Vector2d(drag.xInit, drag.yInit));
                        }

                        getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Drag Constraint"));
                        std::vector<Sketcher::Constraint*> constraints = getConstraints();
                        for (int id : idset) {
                            Sketcher::Constraint* constr = constraints[id]->clone();
                            moveConstraint(constr, id, Base::Vector2d(x, y));
                            constraints[id] = constr;
                        }

                        Sketcher::SketchObject* obj = getSketchObject();
                        obj->Constraints.setValues(std::move(constraints));

                        preselection.PreselectConstraintSet = drag.DragConstraintSet;
                        drag.DragConstraintSet.clear();
                        getDocument()->commitCommand();
                        tryAutoRecomputeIfNotSolve(getSketchObject());
                    }
                    setSketchMode(STATUS_NONE);
                    return true;
                case STATUS_SKETCH_StartRubberBand:// a single click happened, so clear selection
                                                   // unless user hold control.
                    if (!(QApplication::keyboardModifiers() & Qt::ControlModifier)) {
                        Gui::Selection().clearSelection();
                    }
                    setSketchMode(STATUS_NONE);
                    return true;
                case STATUS_SKETCH_UseRubberBand:
                    doBoxSelection(DoubleClick::prvCursorPos, cursorPos, viewer);
                    rubberband->setWorking(false);
                    blockContextMenu = true;

                    // use draw(false, false) to avoid solver geometry with outdated construction flags
                    draw(false, false);

                    // a redraw is required in order to clear the rubberband
                    const_cast<Gui::View3DInventorViewer*>(viewer)->redraw();
                    setSketchMode(STATUS_NONE);
                    return true;
                case STATUS_SKETCH_UseHandler: {
                    sketchHandler->applyCursor();
                    Base::Vector2d snappedPos = snapHandle->compute();
                    return sketchHandler->releaseButton(snappedPos);
                }
                case STATUS_NONE:
                default:
                    return false;
            }
        }
    }
    // Right mouse button ****************************************************
    else if (Button == 2) {
        if (pressed) {
            blockContextMenu = false;

            // Do things depending on the mode of the user interaction
            switch (Mode) {
                case STATUS_NONE: {
                    if (preselection.isPreselectPointValid()) {
                        // Base::Console().log("start dragging, point:%d\n",this->DragPoint);
                        setSketchMode(STATUS_SELECT_Point);
                    }
                    else if (preselection.isPreselectCurveValid()) {
                        // Base::Console().log("start dragging, point:%d\n",this->DragPoint);
                        setSketchMode(STATUS_SELECT_Edge);
                    }
                    else if (preselection.isCrossPreselected()) {
                        // Base::Console().log("start dragging, point:%d\n",this->DragPoint);
                        setSketchMode(STATUS_SELECT_Cross);
                    }
                    else if (!preselection.PreselectConstraintSet.empty()) {
                        // Base::Console().log("start dragging, point:%d\n",this->DragPoint);
                        setSketchMode(STATUS_SELECT_Constraint);
                    }
                    break;
                }
                case STATUS_SKETCH_UseRubberBand:
                    // Cancel rubberband
                    rubberband->setWorking(false);
                    blockContextMenu = true;

                    // a redraw is required in order to clear the rubberband
                    draw(true, false);
                    const_cast<Gui::View3DInventorViewer*>(viewer)->redraw();
                    setSketchMode(STATUS_NONE);
                    return true;
                default:
                    break;
            }
        }
        else if (!pressed) {
            switch (Mode) {
                case STATUS_SKETCH_UseHandler:
                    // delegate to handler whether to quit or do otherwise
                    sketchHandler->pressRightButton(Base::Vector2d(x, y));
                    return true;
                case STATUS_NONE:
                    generateContextMenu();
                    return true;
                case STATUS_SELECT_Point:
                    if (pp) {
                        // Base::Console().log("Select Point:%d\n",this->DragPoint);
                        //  Do selection
                        std::stringstream ss;
                        ss << "Vertex" << preselection.getPreselectionVertexIndex();

                        preselectToSelection(ss, pp, false);
                    }
                    setSketchMode(STATUS_NONE);
                    generateContextMenu();
                    return true;
                case STATUS_SELECT_Edge:
                    if (pp) {
                        // Base::Console().log("Select Point:%d\n",this->DragPoint);
                        std::stringstream ss;
                        if (preselection.isEdge()) {
                            ss << "Edge" << preselection.getPreselectionEdgeIndex();
                        }
                        else {  // external geometry
                            ss << "ExternalEdge" << preselection.getPreselectionExternalEdgeIndex();
                        }

                        preselectToSelection(ss, pp, false);
                    }
                    setSketchMode(STATUS_NONE);
                    generateContextMenu();
                    return true;
                case STATUS_SELECT_Cross:
                    if (pp) {
                        // Base::Console().log("Select Point:%d\n",this->DragPoint);
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

                        preselectToSelection(ss, pp, false);
                    }
                    setSketchMode(STATUS_NONE);
                    generateContextMenu();
                    return true;
                case STATUS_SELECT_Constraint: {
                    if (pp) {
                        auto sels = preselection.PreselectConstraintSet;
                        for (int id : sels) {
                            std::stringstream ss;
                            ss << Sketcher::PropertyConstraintList::getConstraintName(id);

                            preselectToSelection(ss, pp, false);
                        }
                    }
                    setSketchMode(STATUS_NONE);
                    generateContextMenu();
                    return true;
                }
                case STATUS_SKETCH_Drag:
                case STATUS_SKETCH_DragConstraint:
                case STATUS_SKETCH_StartRubberBand:
                case STATUS_SKETCH_UseRubberBand:
                case STATUS_SELECT_Wire:
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
        Base::Console().log("double click point:%d\n", preselection.PreselectPoint);
    }
    else if (preselection.isPreselectCurveValid()) {
        // We cannot do toggleWireSelelection directly here because the released event with
        //STATUS_NONE return false which clears the selection.
        setSketchMode(STATUS_SELECT_Wire);
    }
    else if (preselection.isCrossPreselected()) {
        Base::Console().log("double click cross:%d\n",
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

void ViewProviderSketch::toggleWireSelelection(int clickedGeoId)
{
    Sketcher::SketchObject* obj = getSketchObject();

    const Part::Geometry* geo1 = obj->getGeometry(clickedGeoId);
    if (isPoint(*geo1) || isCircle(*geo1) || isEllipse(*geo1) || isPeriodicBSplineCurve(*geo1)) {
        return;
    }

    const char* type1 = (clickedGeoId >= 0) ? "Edge" : "ExternalEdge";
    std::stringstream ss1;
    ss1 << type1 << clickedGeoId + 1;
    bool selecting = isSelected(ss1.str());

    std::vector<int> connectedEdges = { clickedGeoId };
    bool partHasBeenAdded = true;
    while (partHasBeenAdded) {
        partHasBeenAdded = false;
        for (int geoId = 0; geoId <= obj->getHighestCurveIndex(); geoId++) {
            if (geoId == clickedGeoId || std::ranges::find(connectedEdges, geoId) != connectedEdges.end()) {
                continue;
            }

            const Part::Geometry* geo = obj->getGeometry(geoId);
            if (isPoint(*geo) || isCircle(*geo) || isEllipse(*geo) || isPeriodicBSplineCurve(*geo1)) {
                continue;
            }

            Base::Vector3d p11 = obj->getPoint(geoId, PointPos::start);
            Base::Vector3d p12 = obj->getPoint(geoId, PointPos::end);
            bool connected = false;
            for (auto conGeoId : connectedEdges) {
                Base::Vector3d p21 = obj->getPoint(conGeoId, PointPos::start);
                Base::Vector3d p22 = obj->getPoint(conGeoId, PointPos::end);
                if ((p11 - p21).Length() < Precision::Confusion()
                    || (p11 - p22).Length() < Precision::Confusion()
                    || (p12 - p21).Length() < Precision::Confusion()
                    || (p12 - p22).Length() < Precision::Confusion()) {
                    connected = true;
                }
            }

            if (connected) {
                connectedEdges.push_back(geoId);
                partHasBeenAdded = true;
                break;
            }
        }
    }

    for (auto geoId : connectedEdges) {
        std::stringstream ss;
        const char* type = (geoId >= 0) ? "Edge" : "ExternalEdge";
        ss << type << geoId + 1;
        if (!selecting && isSelected(ss.str())) {
            rmvSelection(ss.str());
        }
        else if (selecting && !isSelected(ss.str())) {
            addSelection2(ss.str());
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

    std::unique_ptr<SnapManager::SnapHandle> snapHandle;
    try {
        double x, y;
        getCoordsOnSketchPlane(line.getPosition(), line.getDirection(), x, y);
        snapHandle = std::make_unique<SnapManager::SnapHandle>(snapManager.get(), Base::Vector2d(x, y));
    }
    catch (const Base::ZeroDivisionError&) {
        return false;
    }

    bool preselectChanged = false;
    if (Mode != STATUS_SELECT_Point && Mode != STATUS_SELECT_Edge
        && Mode != STATUS_SELECT_Constraint && Mode != STATUS_SKETCH_Drag
        && Mode != STATUS_SKETCH_DragConstraint && Mode != STATUS_SKETCH_UseRubberBand) {

        std::unique_ptr<SoPickedPoint> Point(this->getPointOnRay(cursorPos, viewer));

        preselectChanged = detectAndShowPreselection(Point.get());
    }

    switch (Mode) {
        case STATUS_NONE:
            if (preselectChanged) {
                editCoinManager->drawConstraintIcons();
                updateColor();
                return true;
            }
            return false;
        case STATUS_SELECT_Point:
            if (!getSolvedSketch().hasConflicts() && preselection.isPreselectPointValid()) {
                int geoId;
                Sketcher::PointPos pos;
                getSketchObject()->getGeoVertexIndex(preselection.PreselectPoint, geoId, pos);

                initDragging(geoId, pos, viewer);
            }
            else {
                setSketchMode(STATUS_NONE);
            }
            resetPreselectPoint();
            return true;
        case STATUS_SELECT_Edge:
            if (!getSolvedSketch().hasConflicts() && preselection.isPreselectCurveValid()) {
                int geoId = preselection.PreselectCurve;
                Sketcher::PointPos pos = Sketcher::PointPos::none;

                initDragging(geoId, pos, viewer);
            }
            else {
                setSketchMode(STATUS_NONE);
            }
            resetPreselectPoint();
            return true;
        case STATUS_SELECT_Constraint: {
            setSketchMode(STATUS_SKETCH_DragConstraint);
            drag.DragConstraintSet = preselection.PreselectConstraintSet;
            Base::Vector2d selectionPos = snapHandle->compute();
            drag.xInit = selectionPos.x;
            drag.yInit = selectionPos.y;
            resetPreselectPoint();
            return true;
        }
        case STATUS_SKETCH_Drag: {
            Base::Vector2d dragPos = snapHandle->compute();
            doDragStep(dragPos.x, dragPos.y);
            return true;
        }
        case STATUS_SKETCH_DragConstraint:
            if (!drag.DragConstraintSet.empty()) {
                Base::Vector2d dragPos = snapHandle->compute();
                auto idset = drag.DragConstraintSet;
                for (int id : idset) {
                    moveConstraint(id, Base::Vector2d(dragPos.x, dragPos.y));
                }
            }
            return true;
        case STATUS_SKETCH_UseHandler:
            sketchHandler->mouseMove(*snapHandle);
            if (preselectChanged) {
                editCoinManager->drawConstraintIcons();
                sketchHandler->applyCursor();
                updateColor();
            }
            return true;
        case STATUS_SKETCH_StartRubberBand: {
            setSketchMode(STATUS_SKETCH_UseRubberBand);
            rubberband->setWorking(true);
            return true;
        }
        case STATUS_SKETCH_UseRubberBand: {
            // Here we must use the device-pixel-ratio to compute the correct y coordinate
            // (#0003130)
            qreal dpr = viewer->getGLWidget()->devicePixelRatioF();
            DoubleClick::newCursorPos = cursorPos;

            // depending on selection direction (touch selection (right to left) or window selection (left to right))
            // set the appropriate color and line style using theme design tokens
            bool isRightToLeft = DoubleClick::prvCursorPos.getValue()[0] > DoubleClick::newCursorPos.getValue()[0];

            auto* styleParameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>();

            // try to get colors from theme tokens
            auto touchColorValue = styleParameterManager->resolve(StyleParameters::SketcherRubberbandTouchSelectionColor).asValue<Base::Color>();
            auto windowColorValue = styleParameterManager->resolve(StyleParameters::SketcherRubberbandWindowSelectionColor).asValue<Base::Color>();

            auto color = isRightToLeft ? touchColorValue : windowColorValue;

            rubberband->setColor(color.r, color.g, color.b, color.a);
            rubberband->setLineStipple(isRightToLeft);  // dashed for touch, solid for window

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

void ViewProviderSketch::initDragging(int geoId, Sketcher::PointPos pos, Gui::View3DInventorViewer* viewer)
{
    if (geoId < 0) {
        return; // don't drag externals
    }

    drag.reset();
    setSketchMode(STATUS_SKETCH_Drag);
    drag.Dragged.push_back(GeoElementId(geoId, pos));

    // Adding selected geos that should be dragged as well.
    for (auto& geoIdi : selection.SelCurvSet) {
        if (geoIdi < 0) {
            continue; //skip externals
        }

        if (geoIdi == geoId) {
            // geoId is already added because it was the preselected.
            // 2 cases : either the edge was added or a point of it.
            // If its a point then we replace it by the edge.
            // If it's the edge it's replaced by itself so it's ok.
            drag.Dragged[0].Pos = Sketcher::PointPos::none;
        }
        else {
            // For group dragging, we skip the internal geos.
            const Part::Geometry* geo = getSketchObject()->getGeometry(geoId);
            if (!GeometryFacade::isInternalAligned(geo)) {
                drag.Dragged.push_back(GeoElementId(geoIdi));
            }
        }
    }
    for (auto& pointId : selection.SelPointSet) {
        int geoIdi;
        Sketcher::PointPos posi;
        getSketchObject()->getGeoVertexIndex(pointId, geoIdi, posi);
        if (geoIdi < 0) {
            continue; //skip externals
        }

        bool add = true;
        for (auto& pair : drag.Dragged) {
            int geoIdj = pair.GeoId;
            Sketcher::PointPos posj = pair.Pos;
            if (geoIdi == geoIdj && (posi == posj || posj == Sketcher::PointPos::none)) {
                add = false;
                break;
            }
        }
        if (add) {
            drag.Dragged.push_back(GeoElementId(geoIdi, posi));
        }
    }

    auto setRelative = [&]() {
        drag.relative = true;

        // Calculate the click position and use it as the initial point
        SbLine line2;
        getProjectingLine(DoubleClick::prvCursorPos, viewer, line2);
        getCoordsOnSketchPlane(
            line2.getPosition(), line2.getDirection(), drag.xInit, drag.yInit);
        snapManager->snap(Base::Vector2d(drag.xInit, drag.yInit), SnapType::All);
    };

    if (drag.Dragged.size() == 1 && pos == Sketcher::PointPos::none) {
        const Part::Geometry* geo = getSketchObject()->getGeometry(geoId);

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
                    setSketchMode(STATUS_NONE);
                    return;
                }

                // The B-Spline is constrained to be non-rational (equal weights), moving
                // produces a bad effect because OCCT will normalize the values of the
                // weights.
                auto grp = getSolvedSketch().getDependencyGroup(geoId,
                    Sketcher::PointPos::none);

                int bsplinegeoid = -1;

                std::vector<int> polegeoids;

                for (auto c : getSketchObject()->Constraints.getValues()) {
                    if (c->Type == Sketcher::InternalAlignment
                        && c->AlignmentType == BSplineControlPoint
                        && c->First == geoId) {

                        bsplinegeoid = c->Second;
                        break;
                    }
                }

                if (bsplinegeoid == -1) {
                    setSketchMode(STATUS_NONE);
                    return;
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
                    setSketchMode(STATUS_NONE);
                    return;
                }
            }
        }

        if (geo->is<Part::GeomLineSegment>() || geo->is<Part::GeomBSplineCurve>()) {
            setRelative();
        }

        if (geo->is<Part::GeomBSplineCurve>()) {
            getSketchObject()->initTemporaryBSplinePieceMove(
                geoId,
                Sketcher::PointPos::none,
                Base::Vector3d(drag.xInit, drag.yInit, 0.0),
                false);
            return;
        }
    }
    else if (drag.Dragged.size() > 1) {
        setRelative();
    }

    getSketchObject()->initTemporaryMove(drag.Dragged, false);
}

void ViewProviderSketch::doDragStep(double x, double y)
{
    Base::Vector3d vec(x - drag.xInit, y - drag.yInit, 0);

    if (drag.Dragged.size() == 1) {
        // special single bspline point handling.
        Sketcher::PointPos PosId = drag.Dragged[0].Pos;

        if (PosId == Sketcher::PointPos::none) {
            int GeoId = drag.Dragged[0].GeoId;
            auto geo = getSketchObject()->getGeometry(GeoId);
            auto gf = GeometryFacade::getFacade(geo);

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
        }
    }

    if (getSketchObject()->moveGeometriesTemporary(drag.Dragged, vec, drag.relative) == 0) {
        setPositionText(Base::Vector2d(x, y));
        draw(true, false);
    }
}

void ViewProviderSketch::commitDragMove(double x, double y)
{
    const char* cmdName = (drag.Dragged.size() == 1) ?
        (drag.Dragged[0].Pos == Sketcher::PointPos::none ?
        QT_TRANSLATE_NOOP("Command", "Drag Curve") : QT_TRANSLATE_NOOP("Command", "Drag Point"))
        : QT_TRANSLATE_NOOP("Command", "Drag geometries");

    getDocument()->openCommand(cmdName);

    Base::Vector3d vec(x - drag.xInit, y - drag.yInit, 0);

    if (drag.Dragged.size() == 1) {
        // special single bspline point handling.
        Sketcher::PointPos PosId = drag.Dragged[0].Pos;

        if (PosId == Sketcher::PointPos::none) {
            int GeoId = drag.Dragged[0].GeoId;
            auto geo = getSketchObject()->getGeometry(GeoId);
            auto gf = GeometryFacade::getFacade(geo);

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
        }
    }

    std::stringstream cmd;
    cmd << "moveGeometries(";
    cmd << "[";
    for (size_t i = 0; i < drag.Dragged.size(); ++i) {
        if (i > 0) {
            cmd << ", ";
        }
        cmd << "(" << drag.Dragged[i].GeoId << ", " << static_cast<int>(drag.Dragged[i].Pos) << ")";
    }
    cmd << "], App.Vector(" << vec.x << ", " << vec.y << ", 0)";

    if (drag.relative) {
        cmd << ", True";
    }
    cmd << ")";

    try {
        Gui::cmdAppObjectArgs(getObject(), cmd.str().c_str());
    }
    catch (const Base::Exception& e) {
        getDocument()->abortCommand();
        Base::Console().developerError("ViewProviderSketch", "Drag: %s\n", e.what());
    }

    getDocument()->commitCommand();

    tryAutoRecomputeIfNotSolve(getSketchObject());
    drag.reset();
    resetPositionText();
}

void ViewProviderSketch::moveConstraint(int constNum, const Base::Vector2d& toPos, OffsetMode offset)
{
    if (auto constr = getConstraint(constNum)) {
        moveConstraint(constr, constNum, toPos, offset);
    }
}

void ViewProviderSketch::moveConstraint(Sketcher::Constraint* Constr, int constNum, const Base::Vector2d& toPos, OffsetMode offset)
{
    // are we in edit?
    if (!isInEditMode())
        return;

#ifdef FC_DEBUG
    Sketcher::SketchObject* obj = getSketchObject();
    int intGeoCount = obj->getHighestCurveIndex() + 1;
    int extGeoCount = obj->getExternalGeometryCount();
#endif

    // with memory allocation
    const std::vector<Part::Geometry*> geomlist = getSolvedSketch().extractGeometry(true, true);

    // lambda to finalize the move
    auto cleanAndDraw = [this, geomlist](){
        // delete the cloned objects
        for (Part::Geometry* geomPtr : geomlist) {
            if (geomPtr) {
                delete geomPtr;
            }
        }

        draw(true, false);
    };

#ifdef FC_DEBUG
    assert(int(geomlist.size()) == extGeoCount + intGeoCount);
    assert((Constr->First >= -extGeoCount && Constr->First < intGeoCount)
           || Constr->First != GeoEnum::GeoUndef);
    boost::ignore_unused(intGeoCount);
    boost::ignore_unused(extGeoCount);
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
            const Part::Geometry *geo1 = GeoList::getGeometryFromGeoId (geomlist, Constr->First);
            const Part::Geometry *geo2 = GeoList::getGeometryFromGeoId (geomlist, Constr->Second);

            if (isLineSegment(*geo2)) {
                if (isCircleOrArc(*geo1) && Constr->FirstPos == Sketcher::PointPos::none){
                    std::swap(geo1, geo2); // see below
                }
                else {
                    // point to line distance
                    auto lineSeg = static_cast<const Part::GeomLineSegment *>(geo2); //NOLINT
                    Base::Vector3d l2p1 = lineSeg->getStartPoint();
                    Base::Vector3d l2p2 = lineSeg->getEndPoint();
                    // calculate the projection of p1 onto line2
                    p2.ProjectToLine(p1-l2p1, l2p2-l2p1);
                    p2 += p1;
                }
            }

            if (isCircleOrArc(*geo2)) {
                if (Constr->FirstPos != Sketcher::PointPos::none){ // circular to point distance
                    auto [rad, ct] = getRadiusCenterCircleArc(geo2);

                    Base::Vector3d v = p1 - ct;
                    v = v.Normalize();
                    p2 = ct + rad * v;
                }
                else if (isCircleOrArc(*geo1)) { // circular to circular distance
                    GetCirclesMinimalDistance(geo1, geo2, p1, p2);
                }
                else if (isLineSegment(*geo1)){ // circular to line distance
                    auto lineSeg = static_cast<const Part::GeomLineSegment*>(geo1); //NOLINT
                    Base::Vector3d l2p1 = lineSeg->getStartPoint();
                    Base::Vector3d l2p2 = lineSeg->getEndPoint();

                    auto [rad, ct] = getRadiusCenterCircleArc(geo2);

                    p1.ProjectToLine(ct - l2p1, l2p2 - l2p1);// project on the line translated to origin
                    Base::Vector3d v = p1;
                    p1 += ct;
                    v.Normalize();
                    p2 = ct + v * rad;
                }
            }
        }
        else if (Constr->FirstPos != Sketcher::PointPos::none) {
            p2 = getSolvedSketch().getPoint(Constr->First, Constr->FirstPos);
        }
        else if (Constr->First != GeoEnum::GeoUndef) {
            const Part::Geometry* geo = GeoList::getGeometryFromGeoId(geomlist, Constr->First);
            if (geo->is<Part::GeomLineSegment>()) {
                const Part::GeomLineSegment* lineSeg =
                    static_cast<const Part::GeomLineSegment*>(geo);
                p1 = lineSeg->getStartPoint();
                p2 = lineSeg->getEndPoint();
            }
            else if (geo->is<Part::GeomArcOfCircle>()) {
                auto* arc = static_cast<const Part::GeomArcOfCircle*>(geo);
                Base::Vector3d center = arc->getCenter();
                double startangle, endangle;
                arc->getRange(startangle, endangle, /*emulateCCW=*/true);

                if (Constr->Type == Distance && Constr->Second == GeoEnum::GeoUndef){
                    double arcAngle = (startangle + endangle) / 2.;
                    Base::Vector2d arcDirection(std::cos(arcAngle), std::sin(arcAngle));
                    Base::Vector2d centerToToPos = toPos - Base::Vector2d(center.x, center.y);
                    Constr->LabelDistance = centerToToPos * arcDirection;

                    cleanAndDraw();
                    return;
                }
                else {
                    // radius and diameter
                    p1 = center;
                    double angle = Constr->LabelPosition;
                    if (angle == 10) {
                        angle = (startangle + endangle) / 2;
                    }
                    else {
                        Base::Vector3d tmpDir = Base::Vector3d(toPos.x, toPos.y, 0) - p1;
                        angle = atan2(tmpDir.y, tmpDir.x);
                    }
                    double radius = arc->getRadius();
                    if (Constr->Type == Sketcher::Diameter) {
                        p1 = center - radius * Base::Vector3d(cos(angle), sin(angle), 0.);
                    }

                    p2 = center + radius * Base::Vector3d(cos(angle), sin(angle), 0.);
                }
            }
            else if (geo->is<Part::GeomCircle>()) {
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
            dir = Base::Vector3d((p2.x - p1.x >= std::numeric_limits<float>::epsilon()) ? 1 : -1, 0, 0);
        else if (Constr->Type == DistanceY)
            dir = Base::Vector3d(0, (p2.y - p1.y >= std::numeric_limits<float>::epsilon()) ? 1 : -1, 0);

        double offsetVal = 0.0;
        if (offset == OffsetConstraint) {
            if (auto* view = qobject_cast<Gui::View3DInventor*>(this->getActiveView())) {
                Gui::View3DInventorViewer* viewer = view->getViewer();
                float fHeight = -1.0;
                float fWidth = -1.0;
                viewer->getDimensions(fHeight, fWidth);
                offsetVal = (fHeight + fWidth) * 0.01;
            }
        }

        if (Constr->Type == Radius || Constr->Type == Diameter || Constr->Type == Weight) {
            double distance = vec.x * dir.x + vec.y * dir.y;
            if (distance > offsetVal) {
                distance -= offsetVal;
            }
            Constr->LabelDistance = distance;
            Constr->LabelPosition = atan2(dir.y, dir.x);
        }
        else {
            Base::Vector3d normal(-dir.y, dir.x, 0);
            double distance = vec.x * normal.x + vec.y * normal.y - offsetVal;
            Constr->LabelDistance = distance;
            if (Constr->Type == Distance || Constr->Type == DistanceX
                || Constr->Type == DistanceY) {
                vec = Base::Vector3d(toPos.x, toPos.y, 0) - (p2 + p1) / 2;
                Constr->LabelPosition = vec.x * dir.x + vec.y * dir.y;
            }
        }
    }
    else if (Constr->Type == Angle) {
        moveAngleConstraint(Constr, constNum, toPos);
    }

    cleanAndDraw();
}

void ViewProviderSketch::moveAngleConstraint(Sketcher::Constraint* constr, int constNum, const Base::Vector2d& toPos)
{
    Sketcher::SketchObject* obj = getSketchObject();
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

                ap3 = intersection + dir1 - dir2; //- dir2 instead of std::swap(dir1, dir2) and dir1 = -dir1
                sign1 = isLeftOfLine(p11, p12, ap3);
                sign2 = isLeftOfLine(p21, p22, ap3);
            }

            bool inverse = !(sign1 == sign3 && sign2 == sign4);
            if (inverse) {
                obj->inverseAngleConstraint(constr);
            }

            p0 = Base::Vector3d(intersection.x, intersection.y, 0.);
        }
        else {// angle-via-point
            Base::Vector3d p = getSolvedSketch().getPoint(constr->Third, constr->ThirdPos);
            p0 = Base::Vector3d(p.x, p.y, 0);
            Base::Vector3d dir1 = getSolvedSketch().calculateNormalAtPoint(constr->First, p.x, p.y);
            dir1.RotateZ(-std::numbers::pi / 2);// convert to vector of tangency by rotating
            Base::Vector3d dir2 = getSolvedSketch().calculateNormalAtPoint(constr->Second, p.x, p.y);
            dir2.RotateZ(-std::numbers::pi / 2);

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
            Base::Vector3d center = arc->getCenter();
            double startangle, endangle;
            arc->getRange(startangle, endangle, /*emulateCCW=*/true);
            double arcAngle = (startangle + endangle) / 2.;
            Base::Vector2d arcDirection(std::cos(arcAngle), std::sin(arcAngle));
            Base::Vector2d centerToToPos = toPos - Base::Vector2d(center.x, center.y);
            constr->LabelDistance = centerToToPos * arcDirection;
            return;
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
                updateColor();
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
                    }
                    else if (shapetype.size() > 12 && shapetype.substr(0, 12) == "ExternalEdge") {
                        int GeoId = std::atoi(&shapetype[12]) - 1;
                        GeoId = -GeoId - 3;
                        selection.SelCurvSet.insert(GeoId);
                    }
                    else if (shapetype.size() > 6 && shapetype.substr(0, 6) == "Vertex") {
                        int VtId = std::atoi(&shapetype[6]) - 1;
                        addSelectPoint(VtId);
                    }
                    else if (shapetype == "RootPoint") {
                        addSelectPoint(Selection::RootPoint);
                    }
                    else if (shapetype == "H_Axis") {
                        selection.SelCurvSet.insert(Selection::HorizontalAxis);
                    }
                    else if (shapetype == "V_Axis") {
                        selection.SelCurvSet.insert(Selection::VerticalAxis);
                    }
                    else if (shapetype.size() > 10 && shapetype.substr(0, 10) == "Constraint") {
                        int ConstrId =
                            Sketcher::PropertyConstraintList::getIndexFromConstraintName(shapetype);
                        selection.SelConstraintSet.insert(ConstrId);
                        editCoinManager->drawConstraintIcons();
                    }
                    updateColor();
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
                        }
                        else if (shapetype.size() > 12
                                 && shapetype.substr(0, 12) == "ExternalEdge") {
                            int GeoId = std::atoi(&shapetype[12]) - 1;
                            GeoId = -GeoId - 3;
                            selection.SelCurvSet.erase(GeoId);
                        }
                        else if (shapetype.size() > 6 && shapetype.substr(0, 6) == "Vertex") {
                            int VtId = std::atoi(&shapetype[6]) - 1;
                            removeSelectPoint(VtId);
                        }
                        else if (shapetype == "RootPoint") {
                            removeSelectPoint(Sketcher::GeoEnum::RtPnt);
                        }
                        else if (shapetype == "H_Axis") {
                            selection.SelCurvSet.erase(Sketcher::GeoEnum::HAxis);
                        }
                        else if (shapetype == "V_Axis") {
                            selection.SelCurvSet.erase(Sketcher::GeoEnum::VAxis);
                        }
                        else if (shapetype.size() > 10 && shapetype.substr(0, 10) == "Constraint") {
                            int ConstrId =
                                Sketcher::PropertyConstraintList::getIndexFromConstraintName(
                                    shapetype);
                            selection.SelConstraintSet.erase(ConstrId);
                            editCoinManager->drawConstraintIcons();
                        }
                        updateColor();
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
                    }
                    else if (shapetype.size() > 12 && shapetype.substr(0, 12) == "ExternalEdge") {
                        int GeoId = std::atoi(&shapetype[12]) - 1;
                        GeoId = -GeoId - 3;
                        resetPreselectPoint();
                        preselection.PreselectCurve = GeoId;
                    }
                    else if (shapetype.size() > 6 && shapetype.substr(0, 6) == "Vertex") {
                        int PtIndex = std::atoi(&shapetype[6]) - 1;
                        setPreselectPoint(PtIndex);
                    }
                }
            }
        }
        else if (msg.Type == Gui::SelectionChanges::RmvPreselect) {
            resetPreselectPoint();
        }
    }
}

bool ViewProviderSketch::detectAndShowPreselection(SoPickedPoint* Point)
{
    assert(isInEditMode());

    if (Point) {

        EditModeCoinManager::PreselectionResult result = editCoinManager->detectPreselection(Point);

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

    auto inBBCoords = [&Plm, &proj](const Base::Vector3d & point) {
        Base::Vector3d pnt;
        Plm.multVec(point, pnt);
        return proj(pnt);
    };

    int VertexId = -1; // the loop below should be in sync with the main loop in
                       // ViewProviderSketch::draw so that the vertex indices are calculated
                       // correctly
    int GeoId = 0;

    bool touchMode = false;
    // check if selection goes from the right to the left side (for touch-selection where even
    // partially boxed objects get selected)
    if (corners[0].getValue()[0] > corners[1].getValue()[0])
        touchMode = true;

    auto selectVertex = [this](int vertexid) {
        std::stringstream ss;
        ss << "Vertex" << vertexid;
        addSelection2(ss.str());
    };

    auto selectEdge = [this](int edgeid) {
        std::stringstream ss;
        if (edgeid >= 0) {
            ss << "Edge" << edgeid;
        }
        else {
            ss << "ExternalEdge" << -edgeid - 1;
        }
        addSelection2(ss.str());
    };

    auto selectVertexIfInsideBox = [&polygon, &VertexId, &selectVertex](const Base::Vector3d & point) {
        if (polygon.Contains(Base::Vector2d(point.x, point.y))) {
            selectVertex( VertexId + 1);
            return true; // inside
        }

        return false; // outside
    };

    auto selectEdgeIfInsideBox = [&touchMode, &polygon, &GeoId, &inBBCoords, &selectEdge,
                                  numSegments = viewProviderParameters.stdCountSegments](auto geo){

        if constexpr (std::is_same<decltype(geo), Part::GeomBSplineCurve>::value) {
            numSegments *= geo->countKnots();  // one less segments than knots
        }

        double segment = (geo->getLastParameter() - geo->getFirstParameter()) / numSegments;

        bool bpolyInside = true;

        for (int i = 0; i < numSegments; i++) {
            Base::Vector3d pnt = geo->value(geo->getFirstParameter() + i * segment);
            pnt = inBBCoords(pnt);
            if (!polygon.Contains(Base::Vector2d(pnt.x, pnt.y))) {
                    bpolyInside = false;
                    if (!touchMode) {
                        break;
                    }
                }
                else if (touchMode) {
                    bpolyInside = true;
                    break;
            }
        }

        if (bpolyInside) {
            selectEdge(GeoId+1);
        }
    };

    for (std::vector<Part::Geometry*>::const_iterator it = geomlist.begin();
         it != geomlist.end() - 2;
         ++it, ++GeoId) {

        if (GeoId >= intGeoCount)
            GeoId = -extGeoCount;

        if ((*it)->is<Part::GeomPoint>()) {
            // ----- Check if single point lies inside box selection -----/
            const Part::GeomPoint* point = static_cast<const Part::GeomPoint*>(*it);
            Base::Vector3d pnt0 = inBBCoords(point->getPoint());
            VertexId++;

            selectVertexIfInsideBox(pnt0);
        }
        else if ((*it)->is<Part::GeomLineSegment>()) {
            // ----- Check if line segment lies inside box selection -----/
            const Part::GeomLineSegment* lineSeg = static_cast<const Part::GeomLineSegment*>(*it);
            Base::Vector3d pnt1 = inBBCoords(lineSeg->getStartPoint());
            Base::Vector3d pnt2 = inBBCoords(lineSeg->getEndPoint());

            VertexId++;
            bool pnt1Inside = selectVertexIfInsideBox(pnt1);

            VertexId++;
            bool pnt2Inside = selectVertexIfInsideBox(pnt2);
            polygon.Contains(Base::Vector2d(pnt1.x, pnt1.y));
            polygon.Contains(Base::Vector2d(pnt2.x, pnt2.y));


            if ((pnt1Inside && pnt2Inside) && !touchMode) {
                selectEdge(GeoId+1);
            }
            // check if line intersects with polygon
            else if (touchMode) {
                Base::Polygon2d lineAsPolygon;
                lineAsPolygon.Add(Base::Vector2d(pnt1.x, pnt1.y));
                lineAsPolygon.Add(Base::Vector2d(pnt2.x, pnt2.y));
                std::list<Base::Polygon2d> resultList;
                polygon.Intersect(lineAsPolygon, resultList);
                if (!resultList.empty()) {
                    selectEdge(GeoId+1);
                }
            }
        }
        else if ((*it)->isDerivedFrom<Part::GeomConic>()) {
            // ----- Check if circle lies inside box selection -----/
            /// TODO: Make it impossible to miss the conic if it's big and the selection pretty
            /// thin.
            const Part::GeomConic* circle = static_cast<const Part::GeomConic*>(*it);
            Base::Vector3d pnt0 = inBBCoords(circle->getCenter());
            VertexId++;

            bool pnt0Inside = selectVertexIfInsideBox(pnt0);

            if (pnt0Inside || touchMode) {
                selectEdgeIfInsideBox(circle);
            }
        }
        else if ((*it)->isDerivedFrom<Part::GeomArcOfConic>()) {
            // Check if arc lies inside box selection
            const Part::GeomArcOfConic* aoc = static_cast<const Part::GeomArcOfConic*>(*it);

            Base::Vector3d pnt0 = inBBCoords(aoc->getStartPoint(/*emulateCCW=*/true));
            VertexId++;
            bool pnt0Inside = selectVertexIfInsideBox(pnt0);

            Base::Vector3d pnt1 = inBBCoords(aoc->getEndPoint(/*emulateCCW=*/true));
            VertexId++;
            bool pnt1Inside = selectVertexIfInsideBox(pnt1);

            Base::Vector3d pnt2 = inBBCoords(aoc->getCenter());
            VertexId++;
            selectVertexIfInsideBox(pnt2);

            if ((pnt0Inside && pnt1Inside) || touchMode) {
                selectEdgeIfInsideBox(aoc);
            }
        }
        else if ((*it)->is<Part::GeomBSplineCurve>()) {
            const Part::GeomBSplineCurve* spline = static_cast<const Part::GeomBSplineCurve*>(*it);

            Base::Vector3d pnt1 = inBBCoords(spline->getStartPoint());
            VertexId++;
            bool pnt1Inside = selectVertexIfInsideBox(pnt1);

            Base::Vector3d pnt2 = inBBCoords(spline->getEndPoint());
            VertexId++;
            bool pnt2Inside = selectVertexIfInsideBox(pnt2);

            if ((pnt1Inside && pnt2Inside) || touchMode) {
                selectEdgeIfInsideBox(spline);
            }
        }
        else {
            Base::Console().developerError("ViewProviderSketch::doBoxSelection",
                                           "Geometry type is unsupported. Selection may be unsynchronised and fail.");
        }
    }

    Base::Vector3d pnt0 = proj(Plm.getPosition());
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

bool ViewProviderSketch::selectAll()
{
    // logic of this func has been stolen partly from doBoxSelection()
    if (!isInEditMode()) {
        return false;
    }

    // Check if the focus is on the constraints or element list widget.
    QWidget* focusedWidget = QApplication::focusWidget();
    auto* focusedList = qobject_cast<QListWidget*>(focusedWidget);
    bool focusOnConstraintWidget = false;
    bool focusOnElementWidget = false;
    if (focusedList) {
        if (focusedWidget->objectName().toStdString() == "listWidgetConstraints") {
            focusOnConstraintWidget = true;
        }
        else if (focusedWidget->objectName().toStdString() == "listWidgetElements") {
            focusOnElementWidget = true;
        }
        else {
            focusedList = nullptr;
        }
    }

    std::vector<int> ids;
    if (focusedList) {
        for (int i = 0; i < focusedList->count(); ++i) {
            QListWidgetItem* item = focusedList->item(i);
            if (item && !item->isHidden()) {
                ids.push_back(item->data(Qt::UserRole).toInt());
            }
        }
    }

    bool noWidgetSelected = !focusOnConstraintWidget && !focusOnElementWidget;

    Sketcher::SketchObject* sketchObject = getSketchObject();
    if (!sketchObject) {
        return false;
    }

    Gui::Selection().clearSelection();

    if (focusOnElementWidget || noWidgetSelected) {
        int intGeoCount = sketchObject->getHighestCurveIndex() + 1;
        int extGeoCount = sketchObject->getExternalGeometryCount();

        const std::vector<Part::Geometry*> geomlist = sketchObject->getCompleteGeometry();

        int GeoId = 0;

        auto selectVertex = [this](int geoId, Sketcher::PointPos pos) {
            int vertexId = this->getSketchObject()->getVertexIndexGeoPos(geoId, pos);
            addSelection2(fmt::format("Vertex{}", vertexId + 1));
        };

        auto selectEdge = [this](int GeoId) {
            if (GeoId >= 0) {
                addSelection2(fmt::format("Edge{}", GeoId + 1));
            } else {
                addSelection2(fmt::format("ExternalEdge{}", GeoEnum::RefExt - GeoId + 1));
            }
        };

        bool hasUnselectedGeometry = false;

        for (std::vector<Part::Geometry*>::const_iterator it = geomlist.begin();
             it != geomlist.end() - 2; // -2 to exclude H_Axis and V_Axis
             ++it, ++GeoId) {

            if (GeoId >= intGeoCount) {
                GeoId = -extGeoCount;
            }

            if (focusedList && std::ranges::find(ids, GeoId) == ids.end()) {
                continue;
            }

            if ((*it)->is<Part::GeomPoint>()) {
                selectVertex(GeoId, Sketcher::PointPos::start);
            }
            else if ((*it)->is<Part::GeomLineSegment>() || (*it)->is<Part::GeomBSplineCurve>()) {
                selectVertex(GeoId, Sketcher::PointPos::start);
                selectVertex(GeoId, Sketcher::PointPos::end);
                selectEdge(GeoId);
            }
            else if ((*it)->isDerivedFrom<Part::GeomConic>()) {
                selectVertex(GeoId, Sketcher::PointPos::mid);
                selectEdge(GeoId);
            }
            else if ((*it)->isDerivedFrom<Part::GeomArcOfConic>()) {
                selectVertex(GeoId, Sketcher::PointPos::start);
                selectVertex(GeoId, Sketcher::PointPos::end);
                selectVertex(GeoId, Sketcher::PointPos::mid);
                selectEdge(GeoId);
            }
            else {
                hasUnselectedGeometry = true;
            }
        }

        if (!focusOnElementWidget) {
            addSelection2("RootPoint");
        }

        if (hasUnselectedGeometry) {
            Base::Console().error("Select All: Not all geometry was selected");
        }
    }

    if (focusOnConstraintWidget || noWidgetSelected) {
        const std::vector<Sketcher::Constraint*>& constraints = sketchObject->Constraints.getValues();
        for (size_t i = 0; i < constraints.size(); ++i) {
            if (focusedList && std::ranges::find(ids, i) == ids.end()) {
                continue;
            }
            addSelection2(fmt::format("Constraint{}", i + 1));
        }
    }

    return true;
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
    if (mdi && mdi->isDerivedFrom<Gui::View3DInventor>()) {
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
    // In order to allow one to tweak geometry and insert scaling factors, this function needs to
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

        if ((*it)->getGeometry()->is<Part::GeomCircle>()) {// circle
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

                                        if (auto pos = std::ranges::find(polegeoids, ic->First);
                                            pos != polegeoids.end()) {
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
    if (mdi && mdi->isDerivedFrom<Gui::View3DInventor>()) {
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
    editCoinManager->drawEdit(EditCurve, currentGeometryCreationMode());
}

void ViewProviderSketch::drawEdit(const std::list<std::vector<Base::Vector2d>>& list)
{
    editCoinManager->drawEdit(list, currentGeometryCreationMode());
}

void ViewProviderSketch::drawEditMarkers(const std::vector<Base::Vector2d>& EditMarkers,
                                         unsigned int augmentationlevel)
{
    editCoinManager->drawEditMarkers(EditMarkers, augmentationlevel);
}

void ViewProviderSketch::updateData(const App::Property* prop) {
    if (prop == &getSketchObject()->InternalShape) {
        const auto& shape = getSketchObject()->InternalShape.getValue();
        setupCoinGeometry(shape,
                pcSketchFaces,
                Deviation.getValue(),
                AngularDeflection.getValue());
    }

    if (prop != &getSketchObject()->Constraints) {
        signalElementsChanged();
    }

    // clang-format on
    // update placement changes while in edit mode
    if (isInEditMode() && prop) {
        // check if the changed property is `placement` or `attachmentoffset`
        if (strcmp(prop->getName(), "Placement") == 0
            || strcmp(prop->getName(), "AttachmentOffset") == 0) {

#ifdef FC_DEBUG
            Base::Console().warning("updating editing transform!\n");
#endif
            // recalculate the placement matrix
            Sketcher::SketchObject* sketchObj = getSketchObject();
            if (sketchObj) {
                // use globalPlacement for both attached and unattached sketches
                Base::Placement plm = sketchObj->Placement.getValue();

#ifdef FC_DEBUG
                // log what is actually being set
                Base::Console().warning(
                    "Placement: pos=(%f,%f,%f)\n",
                    plm.getPosition().x,
                    plm.getPosition().y,
                    plm.getPosition().z
                );
#endif

                // update the document's editing transform
                getDocument()->setEditingTransform(plm.toMatrix());
            }
        }
    }
    if (std::string(prop->getName()) != "ShapeMaterial") {
        // We don't want material to override the colors of sketches.
        ViewProvider2DObject::updateData(prop);
    }
    // clang-format off
}

void ViewProviderSketch::slotSolverUpdate()
{
    if (!isInEditMode() )
        return;

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
        if (mdi->isDerivedFrom<Gui::View3DInventor>())
            draw(false, true);

        signalConstraintsChanged();
    }
}

void ViewProviderSketch::onChanged(const App::Property* prop)
{
    ViewProvider2DObject::onChanged(prop);

    if (prop == &VisualLayerList) {
        if (isInEditMode()) {
            // Configure and rebuild Coin SceneGraph
            editCoinManager->updateGeometryLayersConfiguration();
        }
        return;
    }

    if (prop == &AutoColor) {
        updateColorPropertiesVisibility();
        return;
    }

    if (prop == &Visibility) {
        pcSketchFacesToggle->on = Visibility.getValue();
        return;
    }

    if (prop == &ShapeAppearance) {
        pcSketchFaces->color.setValue(Base::convertTo<SbColor>(ShapeAppearance.getDiffuseColor()));
        pcSketchFaces->transparency.setValue(ShapeAppearance.getTransparency());
    }
}

void SketcherGui::ViewProviderSketch::updateColorPropertiesVisibility()
{
    auto usesAutomaticColors = AutoColor.getValue();

    // when auto color is enabled don't save color information in the document
    // so it does not cause unnecessary updates if multiple users use different colors
    LineColor.setStatus(App::Property::Transient, usesAutomaticColors);
    PointColor.setStatus(App::Property::Transient, usesAutomaticColors);
    ShapeAppearance.setStatus(App::Property::Transient, usesAutomaticColors);

    // and mark this property as read-only hidden so it's not possible to change manually
    LineColor.setStatus(App::Property::ReadOnly, usesAutomaticColors);
    LineColor.setStatus(App::Property::Hidden, usesAutomaticColors);
    PointColor.setStatus(App::Property::ReadOnly, usesAutomaticColors);
    PointColor.setStatus(App::Property::Hidden, usesAutomaticColors);
    ShapeAppearance.setStatus(App::Property::ReadOnly, usesAutomaticColors);
    ShapeAppearance.setStatus(App::Property::Hidden, usesAutomaticColors);
}

void SketcherGui::ViewProviderSketch::startRestoring()
{
    // small hack: before restoring mark AutoColor property as non-touched
    // this allows us to test if this property was restored in the finishRestoring method
    AutoColor.setStatus(App::Property::Touched, false);
}

void SketcherGui::ViewProviderSketch::finishRestoring()
{
    ViewProvider2DObject::finishRestoring();

    // if AutoColor was not touched it means that the document is from older version of FreeCAD
    // that meaans that we need to run migration strategy and come up with a proper value
    if (!AutoColor.isTouched()) {
        // white is the normally provided default for FreeCAD sketch colors
        auto white = Base::Color(1.f, 1.f, 1.f, 1.f);

        auto colorWasNeverChanged =
            LineColor.getValue() == white &&
            PointColor.getValue() == white;

        AutoColor.setValue(colorWasNeverChanged);
    }

    if (AutoColor.getValue()) {
        // update colors according to current user preferences
        pObserver->updateFromParameter("SketchEdgeColor");
        pObserver->updateFromParameter("SketchVertexColor");
        pObserver->updateFromParameter("SketchFaceColor");

        updateColorPropertiesVisibility();
    }

    if (getSketchObject()->MakeInternals.getValue()) {
        updateVisual();
    }
}

// clang-format on
bool ViewProviderSketch::getElementPicked(const SoPickedPoint* pp, std::string& subname) const
{
    if (pp->getPath()->containsNode(pcSketchFaces) && !isInEditMode()) {
        if (ViewProvider2DObject::getElementPicked(pp, subname)) {
            subname = SketchObject::internalPrefix() + subname;
            auto& elementMap = getSketchObject()->getInternalElementMap();

            if (auto it = elementMap.find(subname); it != elementMap.end()) {
                subname = it->second;
            }

            return true;
        }
    }

    return ViewProvider2DObject::getElementPicked(pp, subname);
}

bool ViewProviderSketch::getDetailPath(
    const char* subname,
    SoFullPath* pPath,
    bool append,
    SoDetail*& det
) const
{
    const auto getLastPartOfName = [](const char* subname) -> const char* {
        const char* realName = strrchr(subname, '.');

        return realName ? realName + 1 : subname;
    };

    if (!isInEditMode() && subname) {
        const char* realName = getLastPartOfName(subname);

        realName = SketchObject::convertInternalName(realName);
        if (realName) {
            auto len = pPath->getLength();
            if (append) {
                pPath->append(pcRoot);
                pPath->append(pcModeSwitch);
            }

            if (!ViewProvider2DObject::getDetailPath(realName, pPath, false, det)) {
                pPath->truncate(len);
                return false;
            }
            return true;
        }
    }

    return ViewProvider2DObject::getDetailPath(subname, pPath, append, det);
}
// clang-format off

void ViewProviderSketch::attach(App::DocumentObject* pcFeat)
{
    ViewProvider2DObject::attach(pcFeat);

    getAnnotation()->addChild(pcSketchFacesToggle);
}

void ViewProviderSketch::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    menu->addAction(tr("Edit Sketch"), receiver, member);
    // Call the extensions
    ViewProvider::setupContextMenu(menu, receiver, member);
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
        QMessageBox msgBox(Gui::getMainWindow());
        msgBox.setText(tr("A dialog is already open in the task panel"));
        msgBox.setInformativeText(tr("Close this dialog?"));
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::Yes);
        int ret = msgBox.exec();
        if (ret == QMessageBox::Yes)
            Gui::Control().closeDialog();
        else
            return false;
    }

    Sketcher::SketchObject* sketch = getSketchObject();

    if(sketch->isFreezed()) {
        return false; // Disallow edit of a frozen sketch
    }

    if (!sketch->evaluateConstraints()) {
        QMessageBox box(Gui::getMainWindow());
        box.setIcon(QMessageBox::Critical);
        box.setWindowTitle(tr("Invalid Sketch"));
        box.setText(tr("Open the sketch validation tool?"));
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

    // update the documents stored transform
    getDocument()->setEditingTransform(plm.toMatrix());

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
                QStringLiteral(
                    "ActiveSketch = App.getDocument('%1').getObject('%2')\n"
                    "tv = Show.TempoVis(App.ActiveDocument, tag= ActiveSketch.ViewObject.TypeId)\n"
                    "ActiveSketch.ViewObject.TempoVis = tv\n"
                    "if ActiveSketch.ViewObject.EditingWorkbench:\n"
                    "  tv.activateWorkbench(ActiveSketch.ViewObject.EditingWorkbench)\n"
                    "if ActiveSketch.ViewObject.HideDependent:\n"
                    "  tv.hide(tv.get_all_dependent(%3, '%4'))\n"
                    "if ActiveSketch.ViewObject.ShowSupport:\n"
                    "  tv.show([ref[0] for ref in ActiveSketch.AttachmentSupport if not (ref[0].isDerivedFrom(\"App::Plane\") or ref[0].isDerivedFrom(\"App::LocalCoordinateSystem\"))])\n"
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
            Base::Console().developerError(
                "ViewProviderSketch", "setEdit: visibility automation failed with an error: \n");
            e.reportException();
        }
    }
    catch (Base::PyException&) {
        Base::Console().developerWarning(
            "ViewProviderSketch",
            "setEdit: could not import Show module. Visibility automation will not work.\n");
    }

    // start the edit dialog
    if (!sketchDlg)
        sketchDlg = new TaskDlgEditSketch(this);

    connectionToolWidget = sketchDlg->registerToolWidgetChanged(std::bind(&SketcherGui::ViewProviderSketch::slotToolWidgetChanged, this, sp::_1));

    Gui::Control().showDialog(sketchDlg);

    // This call to the solver is needed to initialize the DoF and solve time controls
    // The false parameter indicates that the geometry of the SketchObject shall not be updateData
    // so as not to trigger an onChanged that would set the document as modified and trigger a
    // recompute if we just close the sketch without touching anything.
    if (getSketchObject()->AttachmentSupport.getValue()) {
        if (!getSketchObject()->evaluateSupport())
            getSketchObject()->validateExternalLinks();
    }

    //NOLINTBEGIN
    connectUndoDocument = getDocument()->signalUndoDocument.connect(
        std::bind(&ViewProviderSketch::slotUndoDocument, this, sp::_1));
    connectRedoDocument = getDocument()->signalRedoDocument.connect(
        std::bind(&ViewProviderSketch::slotRedoDocument, this, sp::_1));
    connectSolverUpdate = getSketchObject()
            ->signalSolverUpdate.connect(boost::bind(&ViewProviderSketch::slotSolverUpdate, this));
    //NOLINTEND

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

    // Enable solver initial solution update while dragging.
    getSketchObject()->setRecalculateInitialSolutionWhileMovingPoint(
        viewProviderParameters.recalculateInitialSolutionWhileDragging);

    // intercept del key press from main app
    listener = new ShortcutListener(this);

    Gui::getMainWindow()->installEventFilter(listener);

    Workbench::enterEditMode();

    // Give focus to the MDI so that keyboard events are caught after starting edit.
    // Else pressing ESC right after starting edit will not be caught to exit edit mode.
    ensureFocus();

    return true;
}

QString ViewProviderSketch::appendConflictMsg(const std::vector<int>& conflicting)
{
    return appendConstraintMsg(tr("Remove the following constraint:"),
                               tr("Remove at least one of the following constraints:"),
                               conflicting);
}

QString ViewProviderSketch::appendRedundantMsg(const std::vector<int>& redundant)
{
    return appendConstraintMsg(tr("Remove the following redundant constraint:"),
                               tr("Remove the following redundant constraints:"),
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
    return appendConstraintMsg(tr("Remove the following malformed constraint:"),
                               tr("Remove the following malformed constraints:"),
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
                results.append(QStringLiteral("%1").arg(i));
            else
                results.append(QStringLiteral(", %1").arg(i));
        }
    }
    else {
        const int numToShow = 3;
        int more = ints.size() - numToShow;
        for (int i = 0; i < numToShow; ++i) {
            results.append(QStringLiteral("%1, ").arg(ints[i]));
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

    if (getSketchObject()->Geometry.getSize() == 0 &&
        getSketchObject()->ExternalGeo.getSize() <= 2) { // X- and Y-Axis
        signalSetUp(QStringLiteral("empty"), tr("Empty sketch"), QString(), QString());
    }
    else if (dofs < 0 || hasConflicts) {// over-constrained sketch
        signalSetUp(
            QStringLiteral("conflicting_constraints"),
            tr("Over-constrained:") + QLatin1String(" "),
            QStringLiteral("#conflicting"),
            QStringLiteral("(%1)").arg(intListHelper(getSketchObject()->getLastConflicting())));
    }
    else if (hasMalformed) {// malformed constraints
        signalSetUp(QStringLiteral("malformed_constraints"),
                    tr("Malformed constraints:") + QLatin1String(" "),
                    QStringLiteral("#malformed"),
                    QStringLiteral("(%1)").arg(
                        intListHelper(getSketchObject()->getLastMalformedConstraints())));
    }
    else if (hasRedundancies) {
        signalSetUp(
            QStringLiteral("redundant_constraints"),
            tr("Redundant constraints:") + QLatin1String(" "),
            QStringLiteral("#redundant"),
            QStringLiteral("(%1)").arg(intListHelper(getSketchObject()->getLastRedundant())));
    }
    else if (hasPartiallyRedundant) {
        signalSetUp(QStringLiteral("partially_redundant_constraints"),
                    tr("Partially redundant:") + QLatin1String(" "),
                    QStringLiteral("#partiallyredundant"),
                    QStringLiteral("(%1)").arg(
                        intListHelper(getSketchObject()->getLastPartiallyRedundant())));
    }
    else if (getSketchObject()->getLastSolverStatus() != 0) {
        signalSetUp(QStringLiteral("solver_failed"),
                    tr("Solver failed to converge"),
                    QStringLiteral(""),
                    QStringLiteral(""));
    }
    else if (dofs > 0) {
        signalSetUp(QStringLiteral("under_constrained"),
                    tr("Under-constrained:") + QLatin1String(" "),
                    QStringLiteral("#dofs"),
                    tr("%n Degrees of Freedom", "", dofs));
    }
    else {
        signalSetUp(
            QStringLiteral("fully_constrained"), tr("Fully constrained"), QString(), QString());
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
    connectSolverUpdate.disconnect();

    // when pressing ESC make sure to close the dialog
    Gui::Control().closeDialog();

    // visibility automation
    try {
        QString cmdstr =
            QStringLiteral("ActiveSketch = App.getDocument('%1').getObject('%2')\n"
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
        Base::Console().developerError(
            "ViewProviderSketch",
            "unsetEdit: visibility automation failed with an error: %s \n",
            e.what());
    }
}

void ViewProviderSketch::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);
    Base::PyGILStateLocker lock;
    // visibility automation: save camera
    if (!this->TempoVis.getValue().isNone()) {
        try {
            QString cmdstr =
                QStringLiteral(
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
            Base::Console().developerError(
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

    auto *camSensorData = new VPRender {this, viewer->getSoRenderManager()};
    cameraSensor.setData(camSensorData);
    cameraSensor.setDeleteCallback(&ViewProviderSketch::camSensDeleteCB, camSensorData);
    cameraSensor.attach(viewer->getCamera());

    blockContextMenu = false;

    if (auto* window = viewer->window()->windowHandle()) {
        screenChangeConnection = QObject::connect(window, &QWindow::screenChanged, [this](QScreen*) {
            if (isInEditMode() && editCoinManager) {
                editCoinManager->updateElementSizeParameters();
                draw();
            }
        });
    }
}

void ViewProviderSketch::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    auto dataPtr = static_cast<VPRender*>(cameraSensor.getData());
    delete dataPtr;
    cameraSensor.setData(nullptr);
    cameraSensor.setDeleteCallback(nullptr, nullptr);
    cameraSensor.detach();

    viewer->removeGraphicsItem(rubberband.get());
    viewer->setEditing(false);
    viewer->setSelectionEnabled(true);

    blockContextMenu = false;

    QObject::disconnect(screenChangeConnection);
}

void ViewProviderSketch::camSensDeleteCB(void* data, SoSensor *s)
{
    auto *proxyVPrdr = static_cast<VPRender*>(data);
    if (!proxyVPrdr)
        return;

    // The camera object the observer was attached to is gone, try to re-attach the sensor
    // to the new camera.
    // This happens i.e. when the user switches the camera type from orthographic to
    // perspective.
    SoCamera *camera = proxyVPrdr->renderMgr->getCamera();
    if (camera) {
        static_cast<SoNodeSensor *>(s)->attach(camera);
    }
}

void ViewProviderSketch::camSensCB(void* data, SoSensor*)
{
    VPRender* proxyVPrdr = static_cast<VPRender*>(data);
    if (!proxyVPrdr)
        return;

    auto vp = proxyVPrdr->vp;
    auto cam = proxyVPrdr->renderMgr->getCamera();

    if (cam == nullptr)
        Base::Console().developerWarning("ViewProviderSketch", "Camera is nullptr!\n");
    else
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
        Base::Console().log("Switching side, now %s, redrawing\n",
                            tmpFactor < 0 ? "back" : "front");
        viewOrientationFactor = tmpFactor;
        draw();

        QString cmdStr = QStringLiteral("ActiveSketch.ViewObject.TempoVis.sketchClipPlane("
                                        "ActiveSketch, ActiveSketch.ViewObject.SectionView, %1)\n")
                             .arg(tmpFactor < 0 ? QLatin1String("True") : QLatin1String("False"));
        Base::Interpreter().runStringObject(cmdStr.toLatin1());
    }

    // Stretch the axes to cover the whole viewport.
    Gui::View3DInventor* view = qobject_cast<Gui::View3DInventor*>(this->getActiveView());
    if (view) {
        Base::Placement plc = getEditingPlacement();
        const Base::BoundBox2d vpBBox = view->getViewer()
                ->getViewportOnXYPlaneOfPlacement(plc);
        editCoinManager->updateAxesLength(vpBBox);
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
        Base::Console().developerWarning(
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
                if (getSketchObject()->getGeometry(GeoId)->is<Part::GeomPoint>()) {
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
                Base::Console().developerError("ViewProviderSketch", "%s\n", e.what());
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
                            Base::Console().developerError("ViewProviderSketch", "%s\n", e.what());
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
                Base::Console().developerError("ViewProviderSketch", "%s\n", e.what());
            }

            stream.str(std::string());
        }

        for (rit = delExternalGeometries.rbegin(); rit != delExternalGeometries.rend(); ++rit) {
            try {
                Gui::cmdAppObjectArgs(getObject(), "delExternal(%d)", *rit);
            }
            catch (const Base::Exception& e) {
                Base::Console().developerError("ViewProviderSketch", "%s\n", e.what());
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
        static QPixmap px(Gui::BitmapFactory().pixmapFromSvg("Sketcher_NotFullyConstrained", QSize(10, 10)));
        mergedicon = Gui::BitmapFactoryInst::mergePixmap(
            mergedicon, px, Gui::BitmapFactoryInst::BottomRight);
    }

    return Gui::ViewProvider::mergeColorfulOverlayIcons(mergedicon);
}

void ViewProviderSketch::slotToolWidgetChanged(QWidget* newwidget)
{
    if (sketchHandler)
        sketchHandler->toolWidgetChanged(newwidget);
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
        editDocName.c_str(), editObjName.c_str(), (editSubName + getSketchObject()->convertSubName(subNameSuffix)).c_str());
}

void ViewProviderSketch::rmvSelection(const std::string& subNameSuffix)
{
    Gui::Selection().rmvSelection(
        editDocName.c_str(), editObjName.c_str(), (editSubName + getSketchObject()->convertSubName(subNameSuffix)).c_str());
}

bool ViewProviderSketch::addSelection(const std::string& subNameSuffix, float x, float y, float z)
{
    return Gui::Selection().addSelection(
        editDocName.c_str(), editObjName.c_str(), (editSubName + getSketchObject()->convertSubName(subNameSuffix)).c_str(), x, y, z);
}

bool ViewProviderSketch::addSelection2(const std::string& subNameSuffix, float x, float y, float z)
{
    return Gui::Selection().addSelection2(
        editDocName.c_str(),
        editObjName.c_str(),
        (editSubName + getSketchObject()->convertSubName(subNameSuffix)).c_str(),
        x,
        y,
        z);
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

Sketcher::Constraint* ViewProviderSketch::getConstraint(int constid) const
{
    Sketcher::SketchObject* obj = getSketchObject();
    const std::vector<Sketcher::Constraint*>& constrlist = obj->Constraints.getValues();
    if (constid >= 0 || constid < int(constrlist.size())) {
        return constrlist[constid];
    }

    return nullptr;
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
    if (!(mdi && mdi->isDerivedFrom<Gui::View3DInventor>()))
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

qreal ViewProviderSketch::getDevicePixelRatio() const
{
    if (auto activeView = qobject_cast<Gui::View3DInventor*>(this->getActiveView())) {
        auto glWidget = activeView->getViewer()->getGLWidget();
        return glWidget->devicePixelRatio();
    }

    return QApplication::primaryScreen()->devicePixelRatio();
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
    if (!(mdi && mdi->isDerivedFrom<Gui::View3DInventor>()))
        return 0;
    Gui::View3DInventorViewer* viewer = static_cast<Gui::View3DInventor*>(mdi)->getViewer();
    SoCamera* pCam = viewer->getSoRenderManager()->getCamera();
    if (!pCam)
        return 0;

    try {
        SbViewVolume vol = pCam->getViewVolume();

        getCoordsOnSketchPlane(pos0, vol.getProjectionDirection(), x0, y0);
        getCoordsOnSketchPlane(pos1, vol.getProjectionDirection(), x1, y1);

        return Base::toDegrees(-atan2((y1 - y0), (x1 - x0)));
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
void ViewProviderSketch::generateContextMenu()
{
    if (blockContextMenu) return;

    int selectedEdges = 0;
    int selectedLines = 0;
    int selectedConics = 0;
    int selectedPoints = 0;
    int selectedConstraints = 0;
    int selectedBsplines = 0;
    int selectedBsplineKnots = 0;
    int selectedOrigin = 0;
    int selectedEndPoints = 0;
    bool onlyOrigin = false;

    Gui::MenuItem menu;
    menu.setCommand("Sketcher Context");

    std::vector<Gui::SelectionObject> selection =
        Gui::Selection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // if something is selected, count different elements in the current selection
    if (selection.size() > 0) {
        const std::vector<std::string> SubNames = selection[0].getSubNames();
        const Sketcher::SketchObject* obj;
        bool shouldAddChangeConstraintValue = false;
        if (selection[0].getObject()->isDerivedFrom<Sketcher::SketchObject>()) {
            obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
            for (auto& name : SubNames) {
                int geoId = std::atoi(name.substr(4, 4000).c_str()) - 1;
                const Part::Geometry* geo = getSketchObject()->getGeometry(geoId);
                if (name.substr(0, 4) == "Edge" || name.substr(0, 12) == "ExternalEdge") {
                    ++selectedEdges;

                    if (geoId >= 0) {
                        if (isLineSegment(*geo)) {
                            ++selectedLines;
                        }
                        else if (geo->is<Part::GeomBSplineCurve>()) {
                            ++selectedBsplines;
                        }
                        else {
                            ++selectedConics;
                        }
                    }
                }
                else if (name.substr(0, 4) == "Vert") {
                    ++selectedPoints;
                    Sketcher::PointPos posId;
                    getIdsFromName(name, obj, geoId, posId);
                    if (isBsplineKnotOrEndPoint(obj, geoId, posId)) {
                        ++selectedBsplineKnots;
                    }

                    ++selectedEndPoints;
                }
                else if (name.substr(0, 4) == "Cons") {
                    if (selectedConstraints == 0) {
                        int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(name);
                        const Constraint *constraint = obj->Constraints[ConstrId];
                        shouldAddChangeConstraintValue = constraint->isDimensional();
                    }
                    else {
                        shouldAddChangeConstraintValue = false;
                    }
                    ++selectedConstraints;
                }
                else if (name.substr(2, 5) == "Axis") {
                    ++selectedEdges;
                    ++selectedLines;
                    ++selectedOrigin;
                }
                else if (name.substr(0, 4) == "Root") {
                    ++selectedPoints;
                    ++selectedOrigin;
                }
            }
        }
        if (selectedPoints + selectedEdges == selectedOrigin) {
            onlyOrigin = true;
        }
        // build context menu items depending on the selection
        if (selectedBsplines > 0 && selectedBsplines == selectedEdges && selectedPoints == 0
            && !onlyOrigin) {
            menu << "Sketcher_BSplineInsertKnot"
                 << "Sketcher_BSplineIncreaseDegree"
                 << "Sketcher_BSplineDecreaseDegree";
        }
        else if (selectedBsplineKnots > 0 && selectedBsplineKnots == selectedPoints
                 && selectedEdges == 0 && !onlyOrigin) {
            if (selectedBsplineKnots == 1) {
                menu << "Sketcher_BSplineIncreaseKnotMultiplicity"
                     << "Sketcher_BSplineDecreaseKnotMultiplicity";
            }
        }
        if (selectedEdges >= 1 && selectedPoints == 0 && selectedBsplines == 0 && !onlyOrigin) {
            menu << "Sketcher_Dimension";
            if (selectedConics == 0) {
                menu << "Sketcher_ConstrainHorVer"
                     << "Sketcher_ConstrainHorizontal"
                     << "Sketcher_ConstrainVertical";

                if (selectedLines > 1) {
                    menu << "Sketcher_ConstrainParallel";

                    if (selectedLines == 2) {
                        menu << "Sketcher_ConstrainPerpendicular"
                             << "Sketcher_ConstrainTangent";
                    }

                    menu << "Sketcher_ConstrainEqual";
                }
                menu << "Sketcher_ConstrainBlock";
            }
            else if (selectedConics > 1 && selectedLines == 0) {
                menu << "Sketcher_ConstrainCoincidentUnified"
                     << "Sketcher_ConstrainTangent"
                     << "Sketcher_ConstrainEqual";
            }
            else if (selectedConics == 1 && selectedLines == 1) {
                menu << "Sketcher_ConstrainPerpendicular"
                     << "Sketcher_ConstrainTangent";
            }
        }
        else if (selectedEdges == 1 && selectedPoints >= 1 && !onlyOrigin) {
            menu << "Sketcher_Dimension";
            if (selectedConics == 0 && selectedBsplines == 0) {
                menu << "Sketcher_ConstrainCoincidentUnified"
                     << "Sketcher_ConstrainHorVer"
                     << "Sketcher_ConstrainHorizontal"
                     << "Sketcher_ConstrainVertical";
                if (selectedPoints == 2) {
                    menu << "Sketcher_ConstrainSymmetric";
                }
                if (selectedPoints == 1) {
                    menu << "Sketcher_ConstrainPerpendicular"
                         << "Sketcher_ConstrainTangent"
                         << "Sketcher_ConstrainSymmetric";
                }
            }
            else {
                menu << "Sketcher_ConstrainCoincidentUnified"
                     << "Sketcher_ConstrainPerpendicular"
                     << "Sketcher_ConstrainTangent";
            }
        }
        else if (selectedEdges == 0 && selectedPoints >= 1 && !onlyOrigin) {
            menu << "Sketcher_Dimension";

            if (selectedPoints > 1) {
                menu << "Sketcher_ConstrainCoincidentUnified"
                     << "Sketcher_ConstrainHorVer"
                     << "Sketcher_ConstrainHorizontal"
                     << "Sketcher_ConstrainVertical";
            }
            if (selectedPoints == 2) {
                menu << "Sketcher_ConstrainPerpendicular"
                     << "Sketcher_ConstrainTangent";
                if (selectedEndPoints == 2) {
                    menu << "Sketcher_JoinCurves";
                }
            }
            if (selectedPoints == 3) {
                menu << "Sketcher_ConstrainSymmetric";
            }
        }
        else if (selectedLines >= 1 && selectedPoints >= 1 && !onlyOrigin) {
            menu << "Sketcher_Dimension";

            if (selectedPoints == 1) {
                menu << "Sketcher_ConstrainCoincidentUnified";
            }

            menu << "Sketcher_ConstrainHorVer"
                 << "Sketcher_ConstrainHorizontal"
                 << "Sketcher_ConstrainVertical";

            if (selectedLines > 1) {
                menu << "Sketcher_ConstrainParallel";
            }

            if (selectedLines == 2 && selectedPoints == 1) {
                menu << "Sketcher_ConstrainPerpendicular"
                     << "Sketcher_ConstrainTangent";
            }

            if (selectedLines == 1 && selectedPoints == 1) {
                menu << "Sketcher_ConstrainSymmetric";
            }
        }

        // context menu if only constraints are selected
        else if (selectedConstraints >= 1) {
            if (shouldAddChangeConstraintValue) {
                menu << "Sketcher_ChangeDimensionConstraint";
            }
            menu << "Sketcher_ToggleDrivingConstraint"
                 << "Sketcher_ToggleActiveConstraint"
                 << "Sketcher_SelectElementsAssociatedWithConstraints"
                 << "Separator"
                 << "Std_Delete";
        }
        // add the rest of the context menu if geometry is selected
        if (selectedPoints != 0 || selectedEdges != 0) {
            menu << "Separator"
                 << "Sketcher_ToggleConstruction"
                 << "Separator"
                 << "Sketcher_Translate"
                 << "Sketcher_Rotate"
                 << "Sketcher_Scale"
                 << "Sketcher_Offset"
                 << "Sketcher_Symmetry"
                 << "Separator"
                 << "Sketcher_CompDimensionTools"
                 << "Sketcher_CompConstrainTools"
                 << "Separator"
                 << "Sketcher_SelectConstraints"
                 << "Separator"
                 << "Sketcher_CopyClipboard"
                 << "Sketcher_Cut"
                 << "Sketcher_Paste"
                 << "Separator"
                 << "Std_Delete";
        }
    }
    // context menu without a selection
    else {
        menu << "Sketcher_ViewSketch"
             << "Sketcher_ViewSection"
             << "Std_ViewFitAll"
             << "Separator"
             << "Sketcher_CreatePoint"
             << "Sketcher_CreatePolyline"
             << "Sketcher_CreateArc"
             << "Sketcher_CreateCircle"
             << "Sketcher_CreateRectangle"
             << "Sketcher_CreateHexagon"
             << "Sketcher_CreateBSpline"
             << "Separator"
             << "Sketcher_ToggleConstruction"
             << "Separator"
             << "Sketcher_CreateFillet"
             << "Sketcher_CreateChamfer"
             << "Sketcher_Trimming"
             << "Sketcher_Extend"
             << "Separator"
             << "Sketcher_Projection"
             << "Sketcher_Intersection"
             << "Separator"
             << "Sketcher_CompDimensionTools"
             << "Sketcher_CompConstrainTools"
             << "Separator"
             << "Sketcher_DeleteAllGeometry"
             << "Sketcher_DeleteAllConstraints"
             << "Separator"
             << "Sketcher_Paste"
             << "Separator"
             << "Sketcher_LeaveSketch";
    }
    // create context menu
    Gui::Application::Instance->setupContextMenu("Sketch", &menu);
    QMenu contextMenu(
        qobject_cast<Gui::View3DInventor*>(this->getActiveView())->getViewer()->getGLWidget());
    Gui::MenuManager::getInstance()->setupContextMenu(&menu, contextMenu);
    contextMenu.exec(QCursor::pos());
}

void ViewProviderSketch::preselectToSelection(const std::stringstream& ss,
                                              boost::scoped_ptr<SoPickedPoint>& pp,
                                              bool toggle)
{
    // If toggle true and preselection already selected remove from selection
    if (toggle && isSelected(ss.str())) {
        rmvSelection(ss.str());
    }
    // add to selection
    else {
        addSelection2(ss.str(), pp->getPoint()[0], pp->getPoint()[1], pp->getPoint()[2]);
        drag.resetIds();
    }
}
// clang-format on
