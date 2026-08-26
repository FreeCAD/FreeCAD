// SPDX-License-Identifier: LGPL-2.1-or-later

#include "AssemblyViewCapture.h"

#include "ActiveAssemblyResolver.h"
#include "../App/GraphRevision.h"
#include "../App/NativeMutationSupport.h"

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/MDIView.h>
#include <Gui/View3DInventor.h>
#include <Gui/ViewProvider.h>

#ifdef CADX_HAVE_ASSEMBLY
#include <Mod/Assembly/App/AssemblyObject.h>
#endif

namespace CadX
{

AssemblyViewCaptureResult AssemblyViewCapture::capture(
    const AssemblyViewCaptureOptions& options) const
{
#ifndef CADX_HAVE_ASSEMBLY
    (void)options;
    return {false, "CADX_UNSUPPORTED_OBJECT", "the Assembly module is not built", {}, {}};
#else
    if (options.geometryDetail != "summary") {
        return {false,
                "CADX_SNAPSHOT_ARGUMENTS_INVALID",
                "geometry_detail='none' is not supported by the semantic capture provider",
                {}, {}};
    }
    const auto resolved = ActiveAssemblyResolver().resolve();
    if (!resolved.ok) {
        return {false, resolved.errorCode, resolved.diagnostic, {}, {}};
    }
    if (!Gui::Application::Instance) {
        return {false, "CADX_NO_ACTIVE_DOCUMENT", "FreeCAD GUI is not initialized", {}, {}};
    }
    auto* guiDocument = Gui::Application::Instance->activeDocument();
    auto* activeView = guiDocument
        ? dynamic_cast<Gui::View3DInventor*>(guiDocument->getActiveView())
        : nullptr;
    auto* document = guiDocument ? guiDocument->getDocument() : nullptr;
    auto* object = document
        ? document->getObject(resolved.context.assemblyObjectName.c_str())
        : nullptr;
    auto* assembly = dynamic_cast<Assembly::AssemblyObject*>(object);
    if (!activeView || !document || !assembly) {
        return {false,
                "CADX_ACTIVE_ASSEMBLY_STALE",
                "the active Assembly disappeared during capture",
                {}, {}};
    }

    // This is the sole semantic graph capture implementation. The GUI adds
    // only presentation state after the semantic records have been produced.
    const auto semantic = captureNativeAssemblyCapture(document, assembly);
    if (!semantic) {
        return {false, semantic.errorCode, semantic.diagnostic, {}, {}};
    }
    auto capture = semantic.capture;
    capture.activeViewId = resolved.context.activeViewId;

    if (options.includeViewState) {
        capture.cameraState = activeView->getCamera();
    }

    for (auto& node : capture.nodes) {
        if (!options.includeViewState) {
            node.presentation = {};
            continue;
        }
        node.presentation.viewId = resolved.context.activeViewId;
        App::Document* nodeDocument = nullptr;
        for (auto* candidate : App::GetApplication().getDocuments()) {
            if (candidate && candidate->Uid.getValueStr() == node.native.documentUid) {
                nodeDocument = candidate;
                break;
            }
        }
        if (nodeDocument) {
            auto* nodeObject = nodeDocument->getObject(node.native.objectName.c_str());
            auto* nodeGuiDocument = Gui::Application::Instance->getDocument(nodeDocument);
            auto* viewProvider = nodeGuiDocument && nodeObject
                ? nodeGuiDocument->getViewProvider(nodeObject)
                : nullptr;
            node.presentation.visible = viewProvider && viewProvider->isVisible();
        }
    }

    const auto ending = ActiveAssemblyResolver().resolve();
    if (!ending.ok || ending.context.documentUid != resolved.context.documentUid
        || ending.context.assemblyObjectName != resolved.context.assemblyObjectName
        || ending.context.activeViewId != resolved.context.activeViewId) {
        capture.endGuardMatches = false;
        return {false,
                "CADX_VIEW_CHANGED",
                "the active document, view, or Assembly changed during capture",
                {}, {}};
    }

    AssemblyViewState viewState;
    viewState.viewId = resolved.context.activeViewId;
    viewState.projection = "3d";
    viewState.cameraState = capture.cameraState;
    std::string presentation;
    for (const auto& node : capture.nodes) {
        presentation += node.id + (node.presentation.visible ? "|1" : "|0");
    }
    viewState.presentationChecksum = sha256Revision(viewState.cameraState + "|" + presentation);
    return {true, {}, {}, std::move(capture), std::move(viewState)};
#endif
}

}  // namespace CadX
