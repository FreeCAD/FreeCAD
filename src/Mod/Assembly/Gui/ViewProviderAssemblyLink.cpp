// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Ondsel <development@ondsel.com>                     *
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


#include <QAction>
#include <QActionGroup>
#include <QMenu>
#include <cstring>
#include <algorithm>
#include <vector>
#include <sstream>
#include <iostream>

#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>

#include <App/Application.h>
#include <App/Link.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Part.h>
#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>
#include <Base/BoundBox.h>
#include <Base/Color.h>

#include <Gui/Action.h>
#include <Gui/ActionFunction.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Inventor/SoFCBoundingBox.h>
#include <Gui/MainWindow.h>
#include <Gui/ViewParams.h>

#include <Mod/Assembly/App/AssemblyObject.h>
#include <Mod/Assembly/App/AssemblyLink.h>
#include <Mod/Import/App/StepLightweightWorkspaceRuntime.h>

#include "LightweightWorkspaceStatusPanel.h"
#include "ViewProviderAssembly.h"
#include "ViewProviderAssemblyLink.h"


using namespace Assembly;
using namespace AssemblyGui;

namespace
{
constexpr int defaultLargeAssemblyThreshold = 200;
constexpr const char* lightweightProxyBoundsMinPropertyName = "LightweightProxyBoundsMin";
constexpr const char* lightweightProxyBoundsMaxPropertyName = "LightweightProxyBoundsMax";
constexpr const char* lightweightWorkspaceShardLinkPropertyName = "LightweightWorkspaceShardLink";

Import::StepLightweightWorkspaceShardState inspectWorkspaceShard(const AssemblyLink* link)
{
    if (!link) {
        return {};
    }

    return Import::StepLightweightWorkspaceRuntime::inspectLinkedShard(*link);
}

bool loadWorkspaceShard(AssemblyLink* link)
{
    if (!link) {
        return false;
    }

    auto state = inspectWorkspaceShard(link);
    if (!state.isWorkspaceShard) {
        return false;
    }
    if (state.isOpen && state.isFullyLoaded) {
        return true;
    }

    auto* linkedObject = Import::StepLightweightWorkspaceRuntime::loadLinkedShard(*link);
    if (!linkedObject) {
        return false;
    }

    if (auto* doc = linkedObject->getDocument()) {
        Import::StepLightweightWorkspaceRuntime::noteDocumentAccess(*doc);
    }
    return true;
}

std::size_t prefetchWorkspaceShardNeighbors(AssemblyLink* link)
{
    if (!link) {
        return 0;
    }

    return Import::StepLightweightWorkspaceRuntime::prefetchLinkedShardNeighbors(*link);
}
}

PROPERTY_SOURCE(AssemblyGui::ViewProviderAssemblyLink, Gui::ViewProviderPart)

ViewProviderAssemblyLink::ViewProviderAssemblyLink()
    : lightweightPlaceholderSwitch(new SoSwitch())
    , lightweightPlaceholderRoot(new SoSeparator())
    , lightweightPlaceholderColor(new SoBaseColor())
    , lightweightPlaceholderStyle(new SoDrawStyle())
    , lightweightPlaceholderBox(new Gui::SoFCBoundingBox())
{
    lightweightPlaceholderSwitch->ref();
    lightweightPlaceholderRoot->ref();
}

ViewProviderAssemblyLink::~ViewProviderAssemblyLink()
{
    if (lightweightPlaceholderSwitch) {
        lightweightPlaceholderSwitch->unref();
    }
    if (lightweightPlaceholderRoot) {
        lightweightPlaceholderRoot->unref();
    }
}

bool ViewProviderAssemblyLink::lightweightProxyBounds(
    const App::DocumentObject& object,
    Base::BoundBox3d& bounds
)
{
    const auto* minProperty = dynamic_cast<const App::PropertyVector*>(
        object.getPropertyByName(lightweightProxyBoundsMinPropertyName)
    );
    const auto* maxProperty = dynamic_cast<const App::PropertyVector*>(
        object.getPropertyByName(lightweightProxyBoundsMaxPropertyName)
    );
    if (!minProperty || !maxProperty) {
        return false;
    }

    const Base::Vector3d min = minProperty->getValue();
    const Base::Vector3d max = maxProperty->getValue();
    bounds = Base::BoundBox3d(min.x, min.y, min.z, max.x, max.y, max.z);
    return bounds.IsValid();
}

void ViewProviderAssemblyLink::attach(App::DocumentObject* object)
{
    ViewProviderPart::attach(object);

    if (!lightweightPlaceholderSwitch || !lightweightPlaceholderRoot) {
        return;
    }

    if (!lightweightPlaceholderRoot->getNumChildren()) {
        lightweightPlaceholderRoot->renderCaching = SoSeparator::ON;

        Base::Color color;
        color.setPackedValue(
            static_cast<uint32_t>(Gui::ViewParams::instance()->getDefaultLinkColor())
        );
        lightweightPlaceholderColor->rgb.setValue(color.r, color.g, color.b);

        lightweightPlaceholderStyle->style.setValue(SoDrawStyle::LINES);
        lightweightPlaceholderStyle->lineWidth = std::max(
            1L,
            Gui::ViewParams::instance()->getDefaultShapeLineWidth()
        );

        lightweightPlaceholderBox->coordsOn = false;
        lightweightPlaceholderBox->dimensionsOn = false;

        lightweightPlaceholderRoot->addChild(lightweightPlaceholderColor);
        lightweightPlaceholderRoot->addChild(lightweightPlaceholderStyle);
        lightweightPlaceholderRoot->addChild(lightweightPlaceholderBox);
    }

    if (!lightweightPlaceholderSwitch->getNumChildren()) {
        lightweightPlaceholderSwitch->whichChild = SO_SWITCH_NONE;
        lightweightPlaceholderSwitch->addChild(lightweightPlaceholderRoot);
    }

    if (pcRoot->findChild(lightweightPlaceholderSwitch) < 0) {
        pcRoot->addChild(lightweightPlaceholderSwitch);
    }

    updateLightweightPlaceholder();
}

void ViewProviderAssemblyLink::updateData(const App::Property* prop)
{
    ViewProviderPart::updateData(prop);

    if (!prop) {
        updateLightweightPlaceholder();
        return;
    }

    const char* propName = prop->getName();
    if (std::strcmp(propName, "LinkedObject") == 0
        || std::strcmp(propName, lightweightProxyBoundsMinPropertyName) == 0
        || std::strcmp(propName, lightweightProxyBoundsMaxPropertyName) == 0
        || std::strcmp(propName, lightweightWorkspaceShardLinkPropertyName) == 0) {
        updateLightweightPlaceholder();
    }
}

void ViewProviderAssemblyLink::finishRestoring()
{
    ViewProviderPart::finishRestoring();
    updateLightweightPlaceholder();
}

void ViewProviderAssemblyLink::setLightweightPlaceholderVisible(bool visible)
{
    if (lightweightPlaceholderVisible == visible) {
        return;
    }

    lightweightPlaceholderVisible = visible;
    syncLightweightPlaceholderVisibility();
}

void ViewProviderAssemblyLink::syncLightweightPlaceholderVisibility()
{
    if (!lightweightPlaceholderSwitch) {
        return;
    }

    const int whichChild = lightweightPlaceholderRenderable && lightweightPlaceholderVisible
        ? 0
        : SO_SWITCH_NONE;
    if (lightweightPlaceholderSwitch->whichChild.getValue() == whichChild) {
        return;
    }

    lightweightPlaceholderSwitch->whichChild = whichChild;
}

void ViewProviderAssemblyLink::updateLightweightPlaceholder()
{
    if (!lightweightPlaceholderSwitch || !lightweightPlaceholderBox) {
        return;
    }

    auto* link = freecad_cast<Assembly::AssemblyLink*>(getObject());
    if (!link) {
        lightweightPlaceholderRenderable = false;
        syncLightweightPlaceholderVisibility();
        return;
    }

    const auto* shardMarker = dynamic_cast<const App::PropertyBool*>(
        link->getPropertyByName(lightweightWorkspaceShardLinkPropertyName)
    );
    if (!shardMarker || !shardMarker->getValue() || link->getLinkedAssembly()) {
        lightweightPlaceholderRenderable = false;
        syncLightweightPlaceholderVisibility();
        return;
    }

    Base::BoundBox3d bounds;
    if (!lightweightProxyBounds(*link, bounds)) {
        lightweightPlaceholderRenderable = false;
        syncLightweightPlaceholderVisibility();
        return;
    }

    lightweightPlaceholderBox->minBounds.setValue(bounds.MinX, bounds.MinY, bounds.MinZ);
    lightweightPlaceholderBox->maxBounds.setValue(bounds.MaxX, bounds.MaxY, bounds.MaxZ);
    lightweightPlaceholderRenderable = true;
    syncLightweightPlaceholderVisibility();
}

QIcon ViewProviderAssemblyLink::getIcon() const
{
    auto* assembly = dynamic_cast<Assembly::AssemblyLink*>(getObject());
    if (assembly->isRigidLike()) {
        return Gui::BitmapFactory().pixmap("Assembly_AssemblyLinkRigid.svg");
    }
    else {
        return Gui::BitmapFactory().pixmap("Assembly_AssemblyLink.svg");
    }
}

bool ViewProviderAssemblyLink::setEdit(int mode)
{
    auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());

    if (!assemblyLink->isRigidLike() && mode == (int)ViewProvider::Transform) {
        Base::Console().userTranslatedNotification("Flexible sub-assemblies cannot be transformed.");
        return true;
    }

    return ViewProviderPart::setEdit(mode);
}

bool ViewProviderAssemblyLink::doubleClicked()
{
    auto* link = freecad_cast<AssemblyLink*>(getObject());
    if (!link) {
        return true;
    }

    auto shardState = inspectWorkspaceShard(link);
    if (shardState.isWorkspaceShard && (!shardState.isOpen || shardState.isPartial)) {
        loadWorkspaceShard(link);
        LightweightWorkspaceStatusPanel::refreshPanel(link->getDocument());
    }

    auto* assembly = link->getLinkedAssembly();
    if (!assembly) {
        return true;
    }
    if (auto* linkedDoc = assembly->getDocument()) {
        Import::StepLightweightWorkspaceRuntime::noteDocumentAccess(*linkedDoc);
    }

    auto* vpa = freecad_cast<ViewProviderAssembly*>(
        Gui::Application::Instance->getViewProvider(assembly)
    );
    if (!vpa) {
        return true;
    }

    auto doc = assembly->getDocument();
    auto guiDoc = vpa->getDocument();
    if (!doc || !guiDoc) {
        return true;
    }

    Gui::MDIView* mdi = guiDoc->getActiveView();

    // Ensure the linked assembly document is fully loaded and has a view
    if (doc->testStatus(App::Document::PartialDoc) || !mdi) {
        Gui::Application::Instance->reopen(doc);

        // reopening invalidates the pointer.
        auto* assembly = link->getLinkedAssembly();
        if (!assembly) {
            return true;
        }

        vpa = freecad_cast<ViewProviderAssembly*>(
            Gui::Application::Instance->getViewProvider(assembly)
        );
        if (!vpa) {
            return true;
        }
        if (auto* linkedDoc = assembly->getDocument()) {
            Import::StepLightweightWorkspaceRuntime::noteDocumentAccess(*linkedDoc);
        }
    }

    return vpa->doubleClicked();
}

bool ViewProviderAssemblyLink::onDelete(const std::vector<std::string>& subNames)
{
    Q_UNUSED(subNames)

    Gui::Command::doCommand(
        Gui::Command::Doc,
        "App.getDocument(\"%s\").getObject(\"%s\").removeObjectsFromDocument()",
        getObject()->getDocument()->getName(),
        getObject()->getNameInDocument()
    );

    // getObject()->purgeTouched();

    return ViewProviderPart::onDelete(subNames);
}

void ViewProviderAssemblyLink::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    auto func = new Gui::ActionFunction(menu);
    QAction* act;
    auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
    if (assemblyLink->useLightweightMode()) {
        if (assemblyLink->Rigid.getValue()) {
            act = menu->addAction(QObject::tr("Prefer flexible when fully loaded"));
            act->setToolTip(QObject::tr(
                "Lightweight mode behaves rigidly. This changes the sub-assembly back to "
                "flexible once it is fully loaded."
            ));
        }
        else {
            act = menu->addAction(QObject::tr("Prefer rigid when fully loaded"));
            act->setToolTip(QObject::tr(
                "Lightweight mode behaves rigidly. This changes the sub-assembly to remain "
                "rigid once it is fully loaded."
            ));
        }
    }
    else if (assemblyLink->isRigid()) {
        act = menu->addAction(QObject::tr("Turn flexible"));
        act->setToolTip(
            QObject::tr("Your sub-assembly is currently rigid. This will make it flexible instead.")
        );
    }
    else {
        act = menu->addAction(QObject::tr("Turn rigid"));
        act->setToolTip(
            QObject::tr("Your sub-assembly is currently flexible. This will make it rigid instead.")
        );
    }

    func->trigger(act, [this]() {
        auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
        getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Toggle Rigid"));
        Gui::cmdAppObjectArgs(
            assemblyLink,
            "Rigid = %s",
            assemblyLink->Rigid.getValue() ? "False" : "True"
        );

        getDocument()->commitCommand();
        Gui::Selection().clearSelection();
    });

    QMenu* loadModeMenu = menu->addMenu(QObject::tr("Load mode"));
    auto* loadModeActionGroup = new QActionGroup(loadModeMenu);
    loadModeActionGroup->setExclusive(true);
    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Assembly");
    const int largeAssemblyThreshold =
        hGrp->GetInt("LargeAssemblyThreshold", defaultLargeAssemblyThreshold);
    const QString autoLoadModeToolTip =
        largeAssemblyThreshold > 0
            ? QObject::tr(
                  "Use lightweight mode automatically once the linked assembly exceeds %1 "
                  "components."
              )
                  .arg(largeAssemblyThreshold)
            : QObject::tr(
                  "Automatic lightweight switching is currently disabled because the large "
                  "assembly threshold is set to 0."
              );
    auto addLoadModeAction =
        [&](const char* value, const QString& text, const QString& toolTip, bool checked) {
            QAction* loadModeAction = loadModeMenu->addAction(text);
            loadModeAction->setToolTip(toolTip);
            loadModeAction->setCheckable(true);
            loadModeAction->setChecked(checked);
            loadModeActionGroup->addAction(loadModeAction);
            func->trigger(loadModeAction, [this, value]() {
                auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
                getDocument()->openCommand(QT_TRANSLATE_NOOP("Command", "Set Assembly Load Mode"));
                Gui::cmdAppObjectArgs(assemblyLink, "LoadMode = '%s'", value);
                getDocument()->commitCommand();
                Gui::Selection().clearSelection();
            });
        };

    const char* currentLoadMode = assemblyLink->LoadMode.getValueAsString();
    addLoadModeAction(
        "Normal",
        QObject::tr("Normal"),
        QObject::tr("Fully load the linked assembly and synchronize all internal joints."),
        std::strcmp(currentLoadMode, "Normal") == 0
    );
    addLoadModeAction(
        "Auto",
        QObject::tr("Auto"),
        autoLoadModeToolTip,
        std::strcmp(currentLoadMode, "Auto") == 0
    );
    addLoadModeAction(
        "Lightweight",
        QObject::tr("Lightweight"),
        QObject::tr(
            "Keep the linked document partial and suppress internal joint expansion for faster "
            "navigation."
        ),
        std::strcmp(currentLoadMode, "Lightweight") == 0
    );

    const auto shardState = inspectWorkspaceShard(assemblyLink);
    if (shardState.isWorkspaceShard) {
        const auto workspaceState
            = Import::StepLightweightWorkspaceRuntime::inspect(*assemblyLink->getDocument());
        const int maxLoadedShards
            = Import::StepLightweightWorkspaceRuntime::configuredMaxLoadedShards();

        QAction* metricsAction = menu->addAction(QObject::tr("Show lightweight workspace metrics"));
        metricsAction->setToolTip(QObject::tr(
            "Open a live summary of lightweight shard loads, proxies, trims, and prefetches for "
            "this workspace."
        ));
        func->trigger(metricsAction, [this]() {
            auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
            if (!assemblyLink) {
                return;
            }

            LightweightWorkspaceStatusPanel::showPanel(assemblyLink->getDocument());
        });

        QAction* pinAction = menu->addAction(
            shardState.isPinned
                ? QObject::tr("Unpin linked shard")
                : QObject::tr("Pin linked shard")
        );
        pinAction->setToolTip(
            shardState.isPinned
                ? QObject::tr(
                      "Allow this shard to be unloaded again by automatic trim passes and remove its persisted pin."
                  )
                : QObject::tr(
                      "Protect this shard from automatic trim passes and persist that choice in the lightweight cache."
                  )
        );
        func->trigger(pinAction, [this]() {
            auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
            if (!assemblyLink) {
                return;
            }

            const auto shardState = inspectWorkspaceShard(assemblyLink);
            if (shardState.isPinned) {
                Import::StepLightweightWorkspaceRuntime::unpinLinkedShard(*assemblyLink);
            }
            else {
                Import::StepLightweightWorkspaceRuntime::pinLinkedShard(*assemblyLink);
            }
            LightweightWorkspaceStatusPanel::refreshPanel(assemblyLink->getDocument());
        });

        QAction* prefetchAction = menu->addAction(QObject::tr("Prefetch nearby shards"));
        prefetchAction->setToolTip(QObject::tr(
            "Preload spatially nearby shards around this workspace link without forcing this "
            "shard to load first."
        ));
        prefetchAction->setEnabled(
            workspaceState.shards.size() > 1
            && maxLoadedShards > 0
            && workspaceState.fullyLoadedShardCount < static_cast<std::size_t>(maxLoadedShards)
        );
        func->trigger(prefetchAction, [this]() {
            auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
            if (!assemblyLink) {
                return;
            }

            prefetchWorkspaceShardNeighbors(assemblyLink);
            LightweightWorkspaceStatusPanel::refreshPanel(assemblyLink->getDocument());
        });

        if (!shardState.isOpen || shardState.isPartial) {
            QAction* loadAction = menu->addAction(QObject::tr("Load linked shard"));
            loadAction->setToolTip(QObject::tr(
                "Fully load the cached lightweight STEP shard so the linked sub-assembly can be "
                "expanded and edited."
            ));
            func->trigger(loadAction, [this]() {
                auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
                if (!assemblyLink) {
                    return;
                }

                loadWorkspaceShard(assemblyLink);
                LightweightWorkspaceStatusPanel::refreshPanel(assemblyLink->getDocument());
                Gui::Selection().clearSelection();
            });
        }
        else if (shardState.isFullyLoaded) {
            QAction* unloadAction = menu->addAction(QObject::tr("Unload linked shard"));
            unloadAction->setToolTip(QObject::tr(
                "Close the cached shard document again to recover memory while keeping the "
                "master workspace open."
            ));
            func->trigger(unloadAction, [this]() {
                auto* assemblyLink = dynamic_cast<Assembly::AssemblyLink*>(getObject());
                if (!assemblyLink) {
                    return;
                }

                Import::StepLightweightWorkspaceRuntime::unloadLinkedShard(*assemblyLink);
                LightweightWorkspaceStatusPanel::refreshPanel(assemblyLink->getDocument());
                Gui::Selection().clearSelection();
            });
        }
    }

    Gui::CommandManager& mgr = Gui::Application::Instance->commandManager();
    Gui::Command* cmd = mgr.getCommandByName("Assembly_LinkSelectLinked");
    if (cmd && cmd->getAction()) {
        QAction* action = cmd->getAction()->action();
        if (action) {
            menu->addAction(action);
        }
    }

    Q_UNUSED(receiver)
    Q_UNUSED(member)
}
