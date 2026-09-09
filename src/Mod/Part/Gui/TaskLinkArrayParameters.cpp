// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2026 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>    *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include <QCoreApplication>
#include <QMessageBox>
#include <QPushButton>
#include <QString>

#include <algorithm>
#include <optional>
#include <string>

#include <gp_Ax2.hxx>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeature.h>
#include <App/Link.h>
#include <App/Part.h>
#include <App/SuppressibleExtension.h>
#include <Base/Console.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/ComboLinks.h>
#include <Gui/Control.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/LinkArray.h>
#include <Mod/Part/App/LinkArrayCircular.h>
#include <Mod/Part/App/LinkArrayLinear.h>
#include <Mod/Part/App/LinkArrayPath.h>
#include <Mod/Part/App/LinkArrayPoint.h>
#include <Mod/Part/App/LinkArrayPolar.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/PolarPatternExtension.h>
#include <Mod/Part/App/TopoShape.h>

#include "PatternInstanceControls.h"
#include "PatternParametersWidget.h"
#include "TaskLinkArrayParameters.h"
#include "ui_TaskLinkArrayParameters.h"

using namespace PartGui;

namespace
{
std::string stripToSelectableSubName(std::string subName)
{
    if (!subName.empty() && subName.back() == '.') {
        subName.pop_back();
    }

    const auto dot = subName.rfind('.');
    if (dot == std::string::npos) {
        return subName;
    }

    const std::string tail = subName.substr(dot + 1);
    if (tail.rfind("Face", 0) == 0 || tail.rfind("Edge", 0) == 0 || tail.rfind("Vertex", 0) == 0
        || tail == "X_Axis" || tail == "Y_Axis" || tail == "Z_Axis" || tail == "XY_Plane"
        || tail == "XZ_Plane" || tail == "YZ_Plane") {
        return tail;
    }

    return subName;
}

std::vector<std::string> cleanSubNames(std::vector<std::string> subNames)
{
    if (subNames.size() == 1) {
        subNames.front() = stripToSelectableSubName(subNames.front());
    }

    return subNames;
}

Gui::View3DInventorViewer* active3DViewer()
{
    if (auto* view = freecad_cast<Gui::View3DInventor*>(Gui::getMainWindow()->activeWindow())) {
        return view->getViewer();
    }

    return nullptr;
}

bool isSuppressed(App::DocumentObject* obj)
{
    auto* suppressible = obj ? obj->getExtension<App::SuppressibleExtension>() : nullptr;
    return suppressible && suppressible->Suppressed.getValue();
}

std::optional<Base::Vector3d> viewProviderCenter(
    App::DocumentObject* obj,
    Gui::View3DInventorViewer* viewer,
    bool transform
)
{
    auto* viewProvider = obj ? Gui::Application::Instance->getViewProvider(obj) : nullptr;
    if (!viewProvider) {
        return std::nullopt;
    }

    Base::BoundBox3d bbox = viewProvider->getBoundingBox(nullptr, nullptr, transform, viewer);
    if (!bbox.IsValid()) {
        return std::nullopt;
    }

    return bbox.GetCenter();
}

}  // namespace

namespace PartGui
{

void showLinkArrayTask(App::DocumentObject* object, const App::SubObjectT& reference)
{
    auto* array = freecad_cast<Part::LinkArray*>(object);
    if (!array) {
        return;
    }
    if (Gui::Control().activeDialog(array->getDocument())) {
        return;
    }

    Gui::Control().showDialog(new PartGui::TaskDlgLinkArrayParameters(array, reference));
}

}  // namespace PartGui

/* TRANSLATOR PartGui::TaskLinkArrayParameters */

QString TaskLinkArrayParameters::taskTitle(Part::LinkArray* array)
{
    if (array->isDerivedFrom<Part::LinkArrayCircular>()) {
        return tr("Circular Link Array");
    }
    if (array->isDerivedFrom<Part::LinkArrayPath>()) {
        return tr("Path Link Array");
    }
    if (array->isDerivedFrom<Part::LinkArrayPoint>()) {
        return tr("Point Link Array");
    }

    if (array->isDerivedFrom<Part::LinkArrayPolar>()) {
        return tr("Polar Link Array");
    }

    return tr("Linear Link Array");
}

const char* TaskLinkArrayParameters::taskIcon(Part::LinkArray* array)
{
    if (array->isDerivedFrom<Part::LinkArrayCircular>()) {
        return "Part_CircularLinkArray";
    }
    if (array->isDerivedFrom<Part::LinkArrayPath>()) {
        return "Part_PathLinkArray";
    }
    if (array->isDerivedFrom<Part::LinkArrayPoint>()) {
        return "Part_PointLinkArray";
    }
    if (array->isDerivedFrom<Part::LinkArrayPolar>()) {
        return "Part_PolarLinkArray";
    }
    return "LinkArray";
}

TaskLinkArrayParameters::TaskLinkArrayParameters(
    Part::LinkArray* array,
    const App::SubObjectT& reference,
    QWidget* parent
)
    : Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap(taskIcon(array)), taskTitle(array), true, parent)
    , Gui::SelectionObserver(false, Gui::ResolveMode::OldStyleElement)
    , array(array)
    , arrayReference(reference)
{
    proxy = new QWidget(this);
    ui = std::make_unique<Ui_TaskLinkArrayParameters>();
    ui->setupUi(proxy);
    groupLayout()->addWidget(proxy);
    setupLinkedObjectButton();
    applyInitialSelection();
    Gui::View3DInventorViewer* viewer = active3DViewer();

    if (auto* circular = freecad_cast<Part::LinkArrayCircular*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
        setupCircularPatternParameterUI(
            proxy,
            ui->parametersWidgetPlaceholder,
            this,
            &circular->Axis,
            &circular->RadialDistance,
            &circular->TangentialDistance,
            &circular->NumberCircles,
            &circular->Symmetry
        );
        setupInstanceControls(viewer);
        return;
    }
    if (auto* path = freecad_cast<Part::LinkArrayPath*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
        setupPathPatternParameterUI(
            proxy,
            ui->parametersWidgetPlaceholder,
            this,
            &path->Path,
            &path->Count,
            &path->SpacingMode,
            &path->Spacing,
            &path->StartOffset,
            &path->EndOffset,
            &path->ReversePath,
            &path->Align
        );
        setupInstanceControls(viewer);
        return;
    }
    if (auto* point = freecad_cast<Part::LinkArrayPoint*>(array)) {
        ui->parametersWidgetPlaceholder2->hide();
        setupPointPatternParameterUI(proxy, ui->parametersWidgetPlaceholder, this, &point->PointObject);
        setupInstanceControls(viewer);
        return;
    }

    setupPatternParameterUI(
        proxy,
        ui->parametersWidgetPlaceholder,
        ui->parametersWidgetPlaceholder2,
        viewer,
        this
    );
    if (!array->isDerivedFrom<Part::LinkArrayLinear>()) {
        ui->parametersWidgetPlaceholder2->hide();
    }
    updatePatternSpacingLabels();
    setupInstanceControls(viewer);
}

TaskLinkArrayParameters::~TaskLinkArrayParameters()
{
    cancelPendingUpdate();
    instanceControls.reset();
    array = nullptr;
    exitLinkedObjectSelectionMode();
    exitReferenceSelectionMode();
}

App::DocumentObject* TaskLinkArrayParameters::getPatternObject() const
{
    return array;
}

