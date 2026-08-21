// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDPart.h"

#include <QMenu>

#include <cstring>

#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPath.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoGroup.h>

#include <App/GeoFeatureGroupExtension.h>
#include <Gui/Application.h>
#include <Mod/MbDFEM/App/MbDMassMarker.h>
#include <Mod/MbDFEM/App/MbDMarker.h>
#include <Mod/MbDFEM/App/MbDPart.h>

#include "ViewProviderMbDMarker.h"
#include "ViewProviderUtils.h"

using namespace MbDFEMGui;

namespace
{

bool pickedPathContainsObjectRoot(const SoPickedPoint* pp, App::DocumentObject* object)
{
    auto* viewProvider = object ? Gui::Application::Instance->getViewProvider(object) : nullptr;
    auto* root = viewProvider ? viewProvider->getRoot() : nullptr;
    return pp && root && pp->getPath() && pp->getPath()->containsNode(root);
}

}  // namespace

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDPart, PartGui::ViewProviderPart)

ViewProviderMbDPart::ViewProviderMbDPart()
{
    sPixmap = "Document";

    markerChildRoot = new SoGroup;
    markerChildRoot->ref();
    pcRoot->addChild(markerChildRoot);
}

ViewProviderMbDPart::~ViewProviderMbDPart()
{
    markerChildRoot->unref();
    markerChildRoot = nullptr;
}

void ViewProviderMbDPart::attach(App::DocumentObject* object)
{
    PartGui::ViewProviderPart::attach(object);
    setOriginInTreeVisible(object, true);
}

bool ViewProviderMbDPart::canAddToSceneGraph() const
{
    auto* part = getObject<MbDFEM::MbDPart>();
    auto* parent = part ? App::GeoFeatureGroupExtension::getGroupOfObject(part) : nullptr;
    const int parentElementVisible =
        parent && part ? parent->isElementVisible(part->getNameInDocument()) : -1;
    return parentElementVisible != 0 && (!part || part->Visibility.getValue()) && Visibility.getValue()
        && PartGui::ViewProviderPart::canAddToSceneGraph();
}

SoGroup* ViewProviderMbDPart::getChildRoot() const
{
    return markerChildRoot;
}

std::vector<App::DocumentObject*> ViewProviderMbDPart::claimChildren() const
{
    auto* part = getObject<MbDFEM::MbDPart>();
    if (!part) {
        return {};
    }

    std::vector<App::DocumentObject*> children;
    if (auto* origin = getOriginObject(part)) {
        children.push_back(origin);
    }
    if (auto* massMarker = part->getMassMarker()) {
        children.push_back(massMarker);
    }
    if (auto* markersFolder = part->getMarkersFolder()) {
        children.push_back(markersFolder);
    }
    return children;
}

std::vector<App::DocumentObject*> ViewProviderMbDPart::claimChildren3D() const
{
    auto* part = getObject<MbDFEM::MbDPart>();
    if (!part) {
        return {};
    }

    std::vector<App::DocumentObject*> children;
    if (auto* origin = getOriginObject(part)) {
        children.push_back(origin);
    }

    // Markers are defined in the MbDPart coordinate system. Always claim them in 3D so
    // marker-only visuals, such as the axis triad, inherit the part/assembly transform.
    auto markerChildren = part->markers.getValues();
    children.insert(children.end(), markerChildren.begin(), markerChildren.end());
    if (auto* massMarker = part->getMassMarker()) {
        children.push_back(massMarker);
    }
    return children;
}

bool ViewProviderMbDPart::getDetailPath(const char* subname,
                                        SoFullPath* path,
                                        bool append,
                                        SoDetail*& det) const
{
    if (subname && std::strchr(subname, '.')
        && delegateSubobjectDetailPath(this, subname, path, append, det)) {
        return true;
    }
    if (PartGui::ViewProviderPart::getDetailPath(subname, path, append, det)) {
        return true;
    }
    return false;
}

bool ViewProviderMbDPart::getElementPicked(const SoPickedPoint* pp, std::string& subname) const
{
    auto* part = getObject<MbDFEM::MbDPart>();
    if (part) {
        if (auto* massMarker = part->getMassMarker()) {
            if (pickedPathContainsObjectRoot(pp, massMarker)) {
                subname = std::string(massMarker->getNameInDocument()) + ".";
                return true;
            }
        }
        for (auto* object : part->markers.getValues()) {
            auto* marker = freecad_cast<MbDFEM::MbDMarker*>(object);
            if (marker && pickedPathContainsObjectRoot(pp, marker)) {
                subname = std::string(marker->getNameInDocument()) + ".";
                return true;
            }
        }
    }

    return PartGui::ViewProviderPart::getElementPicked(pp, subname);
}

void ViewProviderMbDPart::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addMbDFEMContextMenuCommands(menu, {"MbDFEM_CreateMbDMarker", "MbDFEM_CreateMbDJoint"});

    if (auto* otherMenu = addOtherContextMenu(menu)) {
        PartGui::ViewProviderPart::setupContextMenu(otherMenu, receiver, member);
    }
}

void ViewProviderMbDPart::onChanged(const App::Property* prop)
{
    PartGui::ViewProviderPart::onChanged(prop);

    if (prop == &Visibility) {
        updateMarkerVisibility();
    }
}

void ViewProviderMbDPart::updateMarkerVisibility()
{
    auto* part = getObject<MbDFEM::MbDPart>();
    if (!part) {
        return;
    }

    for (auto* object : part->markers.getValues()) {
        auto* marker = freecad_cast<MbDFEM::MbDMarker*>(object);
        auto* viewProvider = marker ? Gui::Application::Instance->getViewProvider(marker) : nullptr;
        auto* markerViewProvider = freecad_cast<ViewProviderMbDMarker*>(viewProvider);
        if (markerViewProvider) {
            markerViewProvider->updateTriadVisibility();
        }
    }
    auto* massMarker = part->getMassMarker();
    auto* viewProvider = massMarker ? Gui::Application::Instance->getViewProvider(massMarker) : nullptr;
    auto* markerViewProvider = freecad_cast<ViewProviderMbDMarker*>(viewProvider);
    if (markerViewProvider) {
        markerViewProvider->updateTriadVisibility();
    }
}
