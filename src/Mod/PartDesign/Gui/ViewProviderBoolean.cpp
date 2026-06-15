// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                    *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include <QMenu>
#include <Inventor/nodes/SoTransform.h>

#include <utility>

#include "ViewProviderBoolean.h"

#include "StyleParameters.h"
#include "TaskBooleanParameters.h"

#include <Base/ServiceProvider.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <App/Document.h>
#include <App/GroupExtension.h>
#include <Gui/ActiveObjectList.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/MDIView.h>
#include <Gui/Utilities.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/Sketcher/Gui/TaskDlgEditSketch.h>


using namespace PartDesignGui;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProviderBoolean, PartDesignGui::ViewProvider)

const char* PartDesignGui::ViewProviderBoolean::DisplayEnum[] = {"Result", "Tools", nullptr};


ViewProviderBoolean::ViewProviderBoolean()
    : pcToolsPreview(new SoGroup)
    , pcBasePreviewToggle(new SoToggleSwitch)
{
    sPixmap = "PartDesign_Boolean.svg";

    ViewProviderGeoFeatureGroupExtension::initExtension(this);

    ADD_PROPERTY(Display, ((long)0));
    Display.setEnums(DisplayEnum);
}

ViewProviderBoolean::~ViewProviderBoolean() = default;

void ViewProviderBoolean::attach(App::DocumentObject* pcObject)
{
    ViewProvider::attach(pcObject);
    _bodyActivationConn = getDocument()->signalActivatedViewProvider.connect(
        [this](const Gui::ViewProviderDocumentObject* vp, const char* name) {
            onBodyActivated(vp, name);
        }
    );
}

void ViewProviderBoolean::update(const App::Property* prop)
{
    if (!_activeBodyExposure || prop == &getObject()->Visibility) {
        Gui::ViewProviderDocumentObject::update(prop);
        return;
    }

    // A tool body is temporarily shown via setDisplayMaskMode("Group").
    // The normal ViewProvider::update() briefly sets pcModeSwitch=-1 (hide/show
    // optimization) which hides the entire Group subtree, making the tool body flash.
    // Call updateData() directly to skip that cycle.
    if (isUpdatesEnabled()) {
        updateData(prop);
    }
}

const char* ViewProviderBoolean::getConfiguredDisplayMode()
{
    if (Display.getValue() != 0) {
        return "Group";
    }

    if (auto bodyViewProvider = getBodyViewProvider()) {
        return bodyViewProvider->DisplayMode.getValueAsString();
    }

    return getDefaultDisplayMode();
}

static void setTemporaryVisible(Gui::ViewProvider* vp, bool visible)
{
    // This is scene-graph-only visibility and must not mutate App Visibility.
    if (visible) {
        vp->Gui::ViewProvider::show();
    }
    else {
        vp->Gui::ViewProvider::hide();
    }
}

void ViewProviderBoolean::restoreActiveBodyExposure()
{
    if (!_activeBodyExposure) {
        return;
    }

    auto exposure = std::move(*_activeBodyExposure);
    _activeBodyExposure.reset();

    auto* body = exposure.body.get<App::DocumentObject>();
    if (body) {
        if (auto* bodyVP = Gui::Application::Instance->getViewProvider(body)) {
            bodyVP->setDefaultMode(exposure.bodyMode);
            setTemporaryVisible(bodyVP, exposure.bodyWasVisible);
        }
    }

    setDefaultMode(exposure.booleanMode);
    setTemporaryVisible(this, exposure.booleanWasVisible);
}

void ViewProviderBoolean::syncActiveBodyVisibility()
{
    auto* activeView = getDocument()->getActiveView();
    auto* activeBody = activeView ? activeView->getActiveObject<App::DocumentObject*>(PDBODYKEY)
                                  : nullptr;
    auto* activeBodyVP = activeBody ? dynamic_cast<Gui::ViewProviderDocumentObject*>(
                                          Gui::Application::Instance->getViewProvider(activeBody)
                                      )
                                    : nullptr;
    onBodyActivated(activeBodyVP, PDBODYKEY);
}

void ViewProviderBoolean::onBodyActivated(const Gui::ViewProviderDocumentObject* vp, const char* name)
{
    if (strcmp(name, PDBODYKEY) != 0) {
        return;
    }

    auto* feature = getObject<PartDesign::Boolean>();
    if (!feature) {
        return;
    }

    App::DocumentObject* activatedBody = vp ? vp->getObject() : nullptr;
    App::DocumentObject* matchingMember = nullptr;
    if (activatedBody) {
        for (auto* member : feature->Group.getValues()) {
            auto* group = member
                ? member->getExtensionByType<App::GroupExtension>(/*no_except=*/true)
                : nullptr;
            if (member == activatedBody || (group && group->hasObject(activatedBody, true))) {
                matchingMember = member;
                break;
            }
        }
    }

    const bool indirectActivation = matchingMember && matchingMember != activatedBody;
    const bool sameExposure = matchingMember && _activeBodyExposure
        && _activeBodyExposure->body.get<App::DocumentObject>() == matchingMember
        && _activeBodyExposure->indirect == indirectActivation;
    if (!sameExposure) {
        restoreActiveBodyExposure();
    }
    if (!matchingMember) {
        return;
    }

    auto* bodyVP = Gui::Application::Instance->getViewProvider(matchingMember);
    if (!bodyVP) {
        restoreActiveBodyExposure();
        return;
    }

    if (!sameExposure) {
        _activeBodyExposure.emplace(
            ActiveBodyExposure {
                .body = App::DocumentObjectWeakPtrT(matchingMember),
                .bodyMode = bodyVP->getActualMode(),
                .booleanMode = getActualMode(),
                .bodyWasVisible = bodyVP->Gui::ViewProvider::isShow(),
                .booleanWasVisible = Gui::ViewProvider::isShow(),
                .indirect = indirectActivation,
            }
        );
    }

    setDisplayMaskMode("Group");
    if (_activeBodyExposure->indirect) {
        bodyVP->setDisplayMaskMode("Group");
    }
    else {
        setTemporaryVisible(bodyVP, true);
    }
}

void ViewProviderBoolean::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Boolean"));

    ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProviderBoolean::onDelete(const std::vector<std::string>& s)
{
    auto* feature = getObject<PartDesign::Boolean>();

    // if abort command deleted the object the bodies are visible again
    for (auto body : feature->Group.getValues()) {
        if (auto vp = Gui::Application::Instance->getViewProvider(body)) {
            vp->show();
        }
    }

    return ViewProvider::onDelete(s);
}

void ViewProviderBoolean::beforeDelete()
{
    restoreActiveBodyExposure();
    ViewProvider::beforeDelete();
}

const char* ViewProviderBoolean::getDefaultDisplayMode() const
{
    return "Flat Lines";
}

void ViewProviderBoolean::onChanged(const App::Property* prop)
{

    ViewProvider::onChanged(prop);

    if (prop == &Display) {
        setDisplayMode(getConfiguredDisplayMode());
        if (_activeBodyExposure) {
            _activeBodyExposure->booleanMode = getActualMode();
            setDisplayMaskMode("Group");
        }
    }

    if (prop == &Visibility) {
        updateBasePreviewVisibility();
        if (_activeBodyExposure) {
            _activeBodyExposure->booleanWasVisible = Visibility.getValue();
        }
        syncActiveBodyVisibility();
    }
}

void ViewProviderBoolean::updateData(const App::Property* prop)
{
    auto feature = getObject<PartDesign::Boolean>();

    if (prop == &feature->Type) {
        const auto* styleParameterManager
            = Base::provideService<Gui::StyleParameters::ParameterManager>();
        const auto type = feature->Type.getValueAsString();

        const std::map<std::string_view, Gui::StyleParameters::ParameterDefinition<Base::Color>> lookup {
            {"Cut", StyleParameters::PreviewSubtractiveColor},
            {"Common", StyleParameters::PreviewCommonColor},
            {"Fuse", StyleParameters::PreviewAdditiveColor},
        };

        if (lookup.contains(type)) {
            PreviewColor.setValue(styleParameterManager->resolve(lookup.at(type)));
        }

        updateBasePreviewVisibility();
    }

    ViewProvider::updateData(prop);

    if (prop == &feature->Group) {
        syncActiveBodyVisibility();
    }
}

void ViewProviderBoolean::attachPreview()
{
    ViewProvider::attachPreview();

    pcPreviewRoot->addChild(this->pcToolsPreview);
    pcPreviewRoot->addChild(this->pcBasePreviewToggle);
}

void ViewProviderBoolean::updatePreview()
{
    const auto* styleParameterManager = Base::provideService<Gui::StyleParameters::ParameterManager>();

    const double toolOpacity = styleParameterManager->resolve(StyleParameters::PreviewToolOpacity).value;
    const double toolTransparency = 1.0 - toolOpacity;

    auto boolean = getObject<PartDesign::Boolean>();

    if (!boolean) {
        return;
    }

    const auto addToolPreview = [this, toolTransparency](App::DocumentObject* tool) {
        const auto feature = freecad_cast<Part::Feature*>(tool);

        if (!feature) {
            return;
        }

        Part::TopoShape toolShape = feature->Shape.getShape();

        auto pcToolPreview = new PartGui::SoPreviewShape;
        updatePreviewShape(toolShape, pcToolPreview);

        pcToolPreview->transparency.setValue(static_cast<float>(toolTransparency));
        pcToolPreview->color.connectFrom(&pcPreviewShape->color);
        pcToolPreview->lineWidth.connectFrom(&pcPreviewShape->lineWidth);

        pcToolsPreview->addChild(pcToolPreview);
    };

    const auto addBaseShapePreview = [this, toolTransparency, boolean]() {
        auto baseFeature = dynamic_cast<PartDesign::Feature*>(boolean->BaseFeature.getValue());
        if (!baseFeature) {
            return;
        }

        auto baseFeatureViewProvider = freecad_cast<ViewProvider*>(
            Gui::Application::Instance->getViewProvider(baseFeature)
        );
        if (!baseFeatureViewProvider) {
            return;
        }

        auto pcBaseShapePreview = new PartGui::SoPreviewShape;
        updatePreviewShape(baseFeature->Shape.getShape(), pcBaseShapePreview);

        pcBaseShapePreview->transparency.setValue(static_cast<float>(toolTransparency));
        pcBaseShapePreview->color.setValue(
            baseFeatureViewProvider->ShapeAppearance.getDiffuseColor().asValue<SbColor>()
        );
        pcBaseShapePreview->lineWidth.connectFrom(&pcPreviewShape->lineWidth);

        pcBasePreviewToggle->addChild(pcBaseShapePreview);
    };

    try {
        const auto& tools = boolean->Group.getValues();

        if (tools.empty()) {
            return;
        }

        Gui::coinRemoveAllChildren(pcToolsPreview);
        Gui::coinRemoveAllChildren(pcBasePreviewToggle);

        addBaseShapePreview();
        std::ranges::for_each(tools, addToolPreview);
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }

    ViewProvider::updatePreview();
}

TaskDlgFeatureParameters* ViewProviderBoolean::getEditDialog()
{
    return new TaskDlgBooleanParameters(this);
}

void ViewProviderBoolean::updateBasePreviewVisibility()
{
    auto feature = getObject<PartDesign::Boolean>();

    // enable base preview for Common operation only and when the final result is shown
    pcBasePreviewToggle->on = strcmp(feature->Type.getValueAsString(), "Common") == 0
        && Visibility.getValue();
}
