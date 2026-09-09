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
QString translate(const char* text)
{
    return QCoreApplication::translate("PartGui::TaskLinkArrayParameters", text);
}

QString objectLabel(App::DocumentObject* obj)
{
    QString label = QString::fromUtf8(obj->Label.getValue());
    if (label.isEmpty()) {
        label = QString::fromLatin1(obj->getNameInDocument());
    }

    return label;
}

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

QString taskTitle(Part::LinkArray* array)
{
    if (dynamic_cast<Part::LinkArrayCircular*>(array)) {
        return translate("Circular Link Array");
    }
    if (dynamic_cast<Part::LinkArrayPath*>(array)) {
        return translate("Path Link Array");
    }
    if (dynamic_cast<Part::LinkArrayPoint*>(array)) {
        return translate("Point Link Array");
    }

    if (dynamic_cast<Part::LinkArrayPolar*>(array)) {
        return translate("Polar Link Array");
    }

    return translate("Linear Link Array");
}

const char* taskIcon(Part::LinkArray* array)
{
    if (dynamic_cast<Part::LinkArrayCircular*>(array)) {
        return "Part_CircularLinkArray";
    }
    if (dynamic_cast<Part::LinkArrayPath*>(array)) {
        return "Part_PathLinkArray";
    }
    if (dynamic_cast<Part::LinkArrayPoint*>(array)) {
        return "Part_PointLinkArray";
    }
    if (dynamic_cast<Part::LinkArrayPolar*>(array)) {
        return "Part_PolarLinkArray";
    }
    return "LinkArray";
}

Base::Placement arrayGlobalPlacement(const Part::LinkArray* array)
{
    if (!array) {
        return {};
    }

    return App::GeoFeature::getGlobalPlacement(array);
}

void hideArraySource(App::DocumentObject* obj)
{
    if (obj) {
        obj->Visibility.setValue(false);
    }
}

Gui::View3DInventorViewer* active3DViewer()
{
    if (auto* view = Gui::getMainWindow()->activeWindow()) {
        if (view->isDerivedFrom<Gui::View3DInventor>()) {
            return static_cast<Gui::View3DInventor*>(view)->getViewer();
        }
    }

    return nullptr;
}

bool isSuppressed(App::DocumentObject* obj)
{
    auto* suppressible = obj ? obj->getExtensionByType<App::SuppressibleExtension>(true) : nullptr;
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

std::optional<Base::Vector3d> estimateElementCenter(
    Part::LinkArray* array,
    int index,
    Gui::View3DInventorViewer* viewer
)
{
    if (!array || index < 0) {
        return std::nullopt;
    }

    auto localCenter = viewProviderCenter(array->getTrueLinkedObject(false), viewer, false);
    if (!localCenter) {
        return std::nullopt;
    }

    Base::Vector3d center;
    array->getPlacementOf(std::to_string(index), nullptr).multVec(*localCenter, center);
    return center;
}
}  // namespace
