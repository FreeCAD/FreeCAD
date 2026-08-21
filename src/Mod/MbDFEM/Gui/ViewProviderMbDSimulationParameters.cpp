// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDSimulationParameters.h"

#include <QMenu>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Command.h>

#include "ViewProviderUtils.h"

using namespace MbDFEMGui;

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDSimulationParameters, Gui::ViewProviderDocumentObject)

ViewProviderMbDSimulationParameters::ViewProviderMbDSimulationParameters()
{
    sPixmap = "Document";
}

bool ViewProviderMbDSimulationParameters::doubleClicked()
{
    auto* object = getObject();
    auto* document = object ? object->getDocument() : nullptr;
    if (!object || !document) {
        return false;
    }

    const std::string documentName = document->getName();
    const std::string objectName = object->getNameInDocument();
    const std::string command = "import FreeCAD as App\n"
                                "import FreeCADMbDSimulationPanel\n"
                                "obj = App.getDocument('"
        + documentName + "').getObject('" + objectName
        + "')\n"
          "FreeCADMbDSimulationPanel.show_simulation_task_panel(obj)";

    Gui::Command::runCommand(Gui::Command::App, command.c_str());
    return true;
}

void ViewProviderMbDSimulationParameters::setupContextMenu(QMenu* menu,
                                                           QObject* receiver,
                                                           const char* member)
{
    addMbDFEMContextMenuCommands(menu, {"MbDFEM_CreateMbDMarker", "MbDFEM_CreateMbDJoint"});

    if (auto* otherMenu = addOtherContextMenu(menu)) {
        Gui::ViewProviderDocumentObject::setupContextMenu(otherMenu, receiver, member);
    }
}
