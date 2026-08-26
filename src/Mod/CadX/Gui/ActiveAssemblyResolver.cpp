// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ActiveAssemblyResolver.h"

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Type.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/ViewProviderDocumentObject.h>

namespace CadX
{

ActiveAssemblyResolution ActiveAssemblyResolver::resolve() const
{
    if (!Gui::Application::Instance) {
        return {false, "CADX_NO_ACTIVE_DOCUMENT", "FreeCAD GUI is not initialized", {}};
    }
    auto* guiDocument = Gui::Application::Instance->activeDocument();
    if (!guiDocument) {
        return {false, "CADX_NO_ACTIVE_DOCUMENT", "there is no active GUI document", {}};
    }
    auto* activeView = guiDocument->getActiveView();
    if (!activeView || !dynamic_cast<Gui::View3DInventor*>(activeView)) {
        return {false, "CADX_NO_ACTIVE_VIEW", "the active view is not a FreeCAD 3D view", {}};
    }
    auto* editProvider = freecad_cast<Gui::ViewProviderDocumentObject*>(guiDocument->getInEdit());
    if (!editProvider || !editProvider->getObject()) {
        return {false, "CADX_NO_ACTIVE_ASSEMBLY", "no object is active in Assembly edit mode", {}};
    }
    auto* editObject = editProvider->getObject();
    auto* activeAssembly = activeView->getActiveObject<App::DocumentObject*>("assembly");
    const auto assemblyType = Base::Type::fromName("Assembly::AssemblyObject");
    if (!activeAssembly || activeAssembly != editObject
        || !activeAssembly->getTypeId().isDerivedFrom(assemblyType)) {
        return {false,
                "CADX_NO_ACTIVE_ASSEMBLY",
                "the 3D view has no exact Assembly edit object",
                {}};
    }
    auto* document = guiDocument->getDocument();
    if (!document || activeAssembly->getDocument() != document || !document->containsObject(activeAssembly)) {
        return {false,
                "CADX_ACTIVE_ASSEMBLY_STALE",
                "the active Assembly does not belong to the active document",
                {}};
    }
    ActiveAssemblyContext context;
    context.documentUid = document->Uid.getValueStr();
    context.documentName = document->getName();
    context.assemblyObjectName = activeAssembly->getNameInDocument();
    context.assemblyLabel = activeAssembly->Label.getValue();
    context.activeViewId = activeView->getName();
    context.editModeProof = true;
    context.activeViewProof = true;
    return {true, {}, {}, std::move(context)};
}

}  // namespace CadX
