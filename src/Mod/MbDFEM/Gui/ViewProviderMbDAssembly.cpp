// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDAssembly.h"

#include <cstring>

#include <QMenu>

#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoPath.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>

#include <App/Document.h>
#include <Gui/ActionFunction.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Mod/MbDFEM/App/MbDAssembly.h>
#include <Mod/MbDFEM/App/MbDMarker.h>
#include <Mod/MbDFEM/App/MbDPart.h>
#include <Mod/Part/App/PartFeature.h>

#include "ViewProviderAxisTriad.h"
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

bool pickedMarkerSubname(const SoPickedPoint* pp,
                         MbDFEM::MbDPart* part,
                         std::string& markerSubname)
{
    if (!part) {
        return false;
    }

    for (auto* object : part->markers.getValues()) {
        auto* marker = freecad_cast<MbDFEM::MbDMarker*>(object);
        if (marker && pickedPathContainsObjectRoot(pp, marker)) {
            markerSubname = std::string(part->getNameInDocument()) + "."
                + marker->getNameInDocument() + ".";
            return true;
        }
    }

    return false;
}

}  // namespace

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDAssembly, Gui::ViewProviderPart)

ViewProviderMbDAssembly::ViewProviderMbDAssembly()
{
    sPixmap = "Document";

    ADD_PROPERTY_TYPE(
        AxisTriad,
        (false),
        "Display Options",
        App::Prop_None,
        "Show an RGB axis triad at the assembly coordinate system"
    );

    axisTriadSwitch = createAxisTriadSwitch(AxisTriad.getValue());
    pcRoot->addChild(axisTriadSwitch);
}

void ViewProviderMbDAssembly::attach(App::DocumentObject* object)
{
    Gui::ViewProviderPart::attach(object);
    setOriginInTreeVisible(object, true);
}

void ViewProviderMbDAssembly::finishRestoring()
{
    Gui::ViewProviderPart::finishRestoring();
}

std::vector<App::DocumentObject*> ViewProviderMbDAssembly::claimChildren() const
{
    auto* assembly = getObject<MbDFEM::MbDAssembly>();
    if (!assembly) {
        return {};
    }

    std::vector<App::DocumentObject*> children;
    if (auto* origin = getOriginObject(assembly)) {
        children.push_back(origin);
    }
    auto folders = assembly->getCategoryFolders();
    children.insert(children.end(), folders.begin(), folders.end());
    auto parameters = assembly->getParameterObjects();
    children.insert(children.end(), parameters.begin(), parameters.end());
    return children;
}

std::vector<App::DocumentObject*> ViewProviderMbDAssembly::claimChildren3D() const
{
    auto* assembly = getObject<MbDFEM::MbDAssembly>();
    if (!assembly) {
        return {};
    }

    std::vector<App::DocumentObject*> children;
    if (auto* origin = getOriginObject(assembly)) {
        children.push_back(origin);
    }

    // All children in these lists are positioned in the MbDAssembly coordinate system.
    // Claiming them unconditionally keeps their local placement chained to the assembly.
    auto categoryChildren = assembly->getCategoryChildren();
    children.insert(children.end(), categoryChildren.begin(), categoryChildren.end());
    return children;
}

bool ViewProviderMbDAssembly::canDropObjects() const
{
    return true;
}

bool ViewProviderMbDAssembly::canDropObject(App::DocumentObject* obj) const
{
    auto* feature = freecad_cast<Part::Feature*>(obj);
    return feature && !feature->Shape.getShape().isNull();
}

void ViewProviderMbDAssembly::dropObject(App::DocumentObject* obj)
{
    auto* assembly = getObject<MbDFEM::MbDAssembly>();
    auto* feature = freecad_cast<Part::Feature*>(obj);
    if (!assembly || !feature || !assembly->getDocument()) {
        return;
    }

    App::Document* document = assembly->getDocument();
    if (feature->getDocument() != document) {
        return;
    }

    document->openTransaction("Create MbDPart from Part feature");
    try {
        std::string name = std::string(feature->getNameInDocument()) + "_MbDPart";
        auto* part = static_cast<MbDFEM::MbDPart*>(
            document->addObject("MbDFEM::MbDPart", name.c_str()));
        part->Label.setValue(feature->Label.getValue());
        part->Placement.setValue(feature->Placement.getValue());
        part->Shape.setValue(feature->Shape.getShape());
        assembly->addPart(part);
        document->commitTransaction();
        document->recompute();
        if (auto* guiDocument = Gui::Application::Instance->getDocument(document)) {
            guiDocument->setShow(part->getNameInDocument());
        }
    }
    catch (...) {
        document->abortTransaction();
        throw;
    }
}

bool ViewProviderMbDAssembly::getDetailPath(const char* subname,
                                            SoFullPath* path,
                                            bool append,
                                            SoDetail*& det) const
{
    if (subname && std::strchr(subname, '.')
        && delegateSubobjectDetailPath(this, subname, path, append, det)) {
        return true;
    }
    if (Gui::ViewProviderPart::getDetailPath(subname, path, append, det)) {
        return true;
    }
    return false;
}

bool ViewProviderMbDAssembly::getElementPicked(const SoPickedPoint* pp, std::string& subname) const
{
    auto* assembly = getObject<MbDFEM::MbDAssembly>();
    if (assembly) {
        for (auto* object : assembly->getCategoryChildren()) {
            auto* part = freecad_cast<MbDFEM::MbDPart*>(object);
            if (pickedMarkerSubname(pp, part, subname)) {
                return true;
            }
        }
    }

    return Gui::ViewProviderPart::getElementPicked(pp, subname);
}

void ViewProviderMbDAssembly::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto* func = new Gui::ActionFunction(menu);
    addAxisTriadContextMenuAction(
        menu, func, AxisTriad.getValue(), [this](bool visible) { setAxisTriadVisible(visible); });

    Gui::ViewProviderPart::setupContextMenu(menu, receiver, member);
}

void ViewProviderMbDAssembly::onChanged(const App::Property* prop)
{
    if (prop == &AxisTriad) {
        updateAxisTriad();
    }

    Gui::ViewProviderPart::onChanged(prop);
}

void ViewProviderMbDAssembly::setAxisTriadVisible(bool visible)
{
    AxisTriad.setValue(visible);
}

void ViewProviderMbDAssembly::updateAxisTriad()
{
    updateAxisTriadSwitch(axisTriadSwitch, AxisTriad.getValue());
}
