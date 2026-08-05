// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDPart.h"

#include <QAction>
#include <QMenu>

#include <algorithm>
#include <cstring>

#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPath.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoGroup.h>

#include <App/Document.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Gui/ActionFunction.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Mod/MbDFEM/App/MbDAssembly.h>
#include <Mod/MbDFEM/App/MbDMarker.h>
#include <Mod/MbDFEM/App/MbDPart.h>

#include "ViewProviderAxisTriad.h"
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

    ADD_PROPERTY_TYPE(
        AxisTriad,
        (false),
        "Display Options",
        App::Prop_None,
        "Show an RGB axis triad at the part coordinate system"
    );

    axisTriadSwitch = createAxisTriadSwitch(AxisTriad.getValue());
    pcRoot->addChild(axisTriadSwitch);

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
    auto* func = new Gui::ActionFunction(menu);
    QAction* action = menu->addAction(QObject::tr("Be Grounded"));
    action->setEnabled(getObject<MbDFEM::MbDPart>() != nullptr);
    func->trigger(action, [this]() { beGrounded(); });

    addAxisTriadContextMenuAction(
        menu, func, AxisTriad.getValue(), [this](bool visible) { setAxisTriadVisible(visible); });

    if (auto* command =
            Gui::Application::Instance->commandManager().getCommandByName("MbDFEM_CreateMbDMarker")) {
        command->addTo(menu);
        command->testActive();
    }

    PartGui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}

void ViewProviderMbDPart::onChanged(const App::Property* prop)
{
    PartGui::ViewProviderPart::onChanged(prop);

    if (prop == &AxisTriad) {
        updateAxisTriad();
    }
    else if (prop == &Visibility) {
        updateMarkerVisibility();
    }
}

void ViewProviderMbDPart::beGrounded()
{
    auto* part = getObject<MbDFEM::MbDPart>();
    if (!part || !part->getDocument()) {
        return;
    }

    MbDFEM::MbDAssembly* assembly = nullptr;
    for (auto* object : part->getDocument()->getObjects()) {
        auto* candidate = freecad_cast<MbDFEM::MbDAssembly*>(object);
        if (!candidate) {
            continue;
        }
        const auto parts = candidate->parts.getValues();
        if (std::find(parts.begin(), parts.end(), part) != parts.end()) {
            assembly = candidate;
            break;
        }
    }
    if (!assembly) {
        return;
    }

    App::Document* document = part->getDocument();
    document->openTransaction("Ground MbDPart");
    try {
        assembly->groundPart(part);
        document->commitTransaction();
        document->recompute();
        if (auto* guiDocument = Gui::Application::Instance->getDocument(document)) {
            guiDocument->setShow(assembly->getNameInDocument());
        }
    }
    catch (...) {
        document->abortTransaction();
        throw;
    }
}

void ViewProviderMbDPart::setAxisTriadVisible(bool visible)
{
    AxisTriad.setValue(visible);
}

void ViewProviderMbDPart::updateAxisTriad()
{
    updateAxisTriadSwitch(axisTriadSwitch, AxisTriad.getValue());
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
}
