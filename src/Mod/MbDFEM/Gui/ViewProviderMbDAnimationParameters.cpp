// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ViewProviderMbDAnimationParameters.h"

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Command.h>

using namespace MbDFEMGui;

PROPERTY_SOURCE(MbDFEMGui::ViewProviderMbDAnimationParameters, Gui::ViewProviderDocumentObject)

ViewProviderMbDAnimationParameters::ViewProviderMbDAnimationParameters()
{
    sPixmap = "Document";
}

bool ViewProviderMbDAnimationParameters::doubleClicked()
{
    auto* object = getObject();
    auto* document = object ? object->getDocument() : nullptr;
    if (!object || !document) {
        return false;
    }

    const std::string documentName = document->getName();
    const std::string objectName = object->getNameInDocument();
    const std::string command = "import FreeCAD as App\n"
                                "import FreeCADMbDAnimationPanel\n"
                                "obj = App.getDocument('"
        + documentName + "').getObject('" + objectName
        + "')\n"
          "FreeCADMbDAnimationPanel.show_animation_task_panel(obj)";

    Gui::Command::runCommand(Gui::Command::App, command.c_str());
    return true;
}
