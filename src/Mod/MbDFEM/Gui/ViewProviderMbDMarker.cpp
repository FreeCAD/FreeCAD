// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDMarker.h"

#include <QMenu>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/GeoFeatureGroupExtension.h>

#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Selection/SoFCSelection.h>

#include <Inventor/nodes/SoSwitch.h>

#include <Mod/MbDFEM/App/MbDMassMarker.h>
#include <Mod/MbDFEM/App/MbDMarker.h>
#include <Mod/MbDFEM/App/MbDPart.h>

#include "ViewProviderAxisTriad.h"
#include "ViewProviderUtils.h"

using namespace MbDFEMGui;

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDMarker, PartGui::ViewProviderPart)

ViewProviderMbDMarker::ViewProviderMbDMarker()
{
    sPixmap = "Document";
}

void ViewProviderMbDMarker::attach(App::DocumentObject* object)
{
    PartGui::ViewProviderPart::attach(object);

    if (!object || !object->getDocument()) {
        return;
    }

    axisTriadSelection = new Gui::SoFCSelection;
    axisTriadSelection->applySettings();
    axisTriadSelection->objectName = object->getNameInDocument();
    axisTriadSelection->documentName = object->getDocument()->getName();
    axisTriadSelection->style = Gui::SoFCSelection::EMISSIVE_DIFFUSE;

    axisTriadSelection->addChild(
        object->isDerivedFrom<MbDFEM::MbDMassMarker>() ? createMassMarkerAxisTriad()
                                                       : createAxisTriad()
    );

    axisTriadSwitch = new SoSwitch;
    axisTriadSwitch->whichChild = effectiveVisibility() ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    axisTriadSwitch->addChild(axisTriadSelection);
    pcRoot->addChild(axisTriadSwitch);
}

bool ViewProviderMbDMarker::canAddToSceneGraph() const
{
    return Visibility.getValue() && PartGui::ViewProviderPart::canAddToSceneGraph();
}

bool ViewProviderMbDMarker::doubleClicked()
{
    auto* object = getObject();
    auto* document = object ? object->getDocument() : nullptr;
    if (!object || !document || !object->isDerivedFrom<MbDFEM::MbDMassMarker>()) {
        return PartGui::ViewProviderPart::doubleClicked();
    }

    const std::string documentName = document->getName();
    const std::string objectName = object->getNameInDocument();
    const std::string command = "import FreeCAD as App\n"
                                "import FreeCADMbDMassMarkerPanel\n"
                                "obj = App.getDocument('"
        + documentName + "').getObject('" + objectName
        + "')\n"
          "FreeCADMbDMassMarkerPanel.show_mass_marker_task_panel(obj)";

    Gui::Command::runCommand(Gui::Command::App, command.c_str());
    return true;
}

void ViewProviderMbDMarker::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addMbDFEMContextMenuCommands(menu, {"MbDFEM_CreateMbDMarker", "MbDFEM_CreateMbDJoint"});

    if (auto* otherMenu = addOtherContextMenu(menu)) {
        PartGui::ViewProviderPart::setupContextMenu(otherMenu, receiver, member);
    }
}

void ViewProviderMbDMarker::onChanged(const App::Property* prop)
{
    PartGui::ViewProviderPart::onChanged(prop);

    if (prop == &Visibility) {
        updateTriadVisibility();
    }
}

void ViewProviderMbDMarker::updateTriadVisibility()
{
    updateAxisTriadSwitch(axisTriadSwitch, effectiveVisibility());
}

bool ViewProviderMbDMarker::effectiveVisibility() const
{
    auto* marker = getObject<MbDFEM::MbDMarker>();
    if (!marker || !marker->Visibility.getValue() || !Visibility.getValue()) {
        return false;
    }

    auto* part = freecad_cast<MbDFEM::MbDPart*>(
        App::GeoFeatureGroupExtension::getGroupOfObject(marker));
    if (part) {
        if (!part->Visibility.getValue()) {
            return false;
        }

        auto* partViewProvider = Gui::Application::Instance->getViewProvider(part);
        auto* partViewProviderPart = freecad_cast<PartGui::ViewProviderPart*>(partViewProvider);
        if (partViewProviderPart && !partViewProviderPart->Visibility.getValue()) {
            return false;
        }
    }

    return true;
}
