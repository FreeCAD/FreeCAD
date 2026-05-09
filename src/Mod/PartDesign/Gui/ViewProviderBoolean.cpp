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

#include "ViewProviderBoolean.h"

#include "StyleParameters.h"
#include "TaskBooleanParameters.h"

#include <Base/ServiceProvider.h>
#include <Base/Tools.h>
#include <Mod/PartDesign/App/FeatureBoolean.h>
#include <App/Document.h>
#include <App/GroupExtension.h>
#include <Gui/ActiveObjectList.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
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
    if (!_shownBody) {
        Gui::ViewProviderDocumentObject::update(prop);
        return;
    }

    // A tool body is temporarily shown via setDisplayMaskMode("Group").
    // The normal ViewProvider::update() briefly sets pcModeSwitch=-1 (hide/show
    // optimization) which hides the entire Group subtree, making the tool body flash.
    // Call updateData() directly to skip that cycle. User1 on Visibility suppresses
    // VP<->App Visibility syncing and extensionShow/Hide member propagation.
    if (isUpdatesEnabled()) {
        Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(
            App::Property::User1,
            &Visibility
        );
        updateData(prop);
    }
}

void ViewProviderBoolean::show()
{
    // ViewProviderGeoFeatureGroupExtension::extensionShow() would iterate
    // Boolean->Group and set every tool body's App Visibility = true,
    // re-showing bodies that were intentionally hidden. User1 suppresses that
    // propagation. It also suppresses the VP→App Visibility sync in onChanged,
    // so we apply that sync manually after the guard drops.
    {
        Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(
            App::Property::User1,
            &Visibility
        );
        Gui::ViewProviderDocumentObject::show();
    }
    if (auto* obj = getObject(); obj && Visibility.getValue() != obj->Visibility.getValue()) {
        obj->Visibility.setValue(Visibility.getValue());
    }
}

bool ViewProviderBoolean::isShow() const
{
    // setDisplayMaskMode("Group") sets pcModeSwitch >= 0 even when Boolean is hidden; use App
    // Visibility.
    if (auto* obj = getObject()) {
        return obj->Visibility.getValue();
    }
    return ViewProvider::isShow();
}

// Returns true if target is contained anywhere inside container's Group hierarchy.
static bool containsRecursively(App::DocumentObject* container, App::DocumentObject* target)
{
    auto* ext = container->getExtensionByType<App::GroupExtension>(/*no_except=*/true);
    if (!ext) {
        return false;
    }
    for (auto* member : ext->Group.getValues()) {
        if (!member) {
            continue;
        }
        if (member == target || containsRecursively(member, target)) {
            return true;
        }
    }
    return false;
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

    const auto& group = feature->Group.getValues();
    App::DocumentObject* activatedBody = vp ? vp->getObject() : nullptr;

    // Find the direct Group member that is, or transitively contains, the activated body.
    App::DocumentObject* matchingMember = nullptr;
    if (activatedBody) {
        for (auto* obj : group) {
            if (obj == activatedBody || containsRecursively(obj, activatedBody)) {
                matchingMember = obj;
                break;
            }
        }
    }

    // Show or hide the body by calling Gui::ViewProvider::show/hide non-virtually,
    // bypassing ViewProviderBody::show(). ViewProviderBody::show() would call
    // tip->Visibility.setValue(true), changing App Visibility and triggering
    // GroupExtension cascades that visually toggle the Boolean.
    // User1 suppresses extensionShow/extensionHide member propagation and the
    // VP->App Visibility sync in onChanged.
    // Note: ViewProviderBody inherits from PartGui::ViewProviderPart, not
    // PartDesignGui::ViewProvider, so we cast to Gui::ViewProviderDocumentObject
    // to reach any VP type (bodies included).
    const auto setBodyVisible = [](App::DocumentObject* body, bool visible) {
        auto* rawVP = Gui::Application::Instance->getViewProvider(body);
        auto* vpdo = dynamic_cast<Gui::ViewProviderDocumentObject*>(rawVP);
        if (!vpdo) {
            return;
        }
        Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(
            App::Property::User1,
            &vpdo->Visibility
        );
        if (visible) {
            rawVP->Gui::ViewProvider::show();
        }
        else {
            rawVP->Gui::ViewProvider::hide();
        }
    };

    // For nested Booleans, the activated body may live inside an intermediate body that is itself a
    // direct Group member
    const auto switchIntermediateToGroup = [](App::DocumentObject* body) {
        if (auto* bodyVP = Gui::Application::Instance->getViewProvider(body)) {
            bodyVP->setDisplayMaskMode("Group");
        }
    };

    // Restore an intermediate body VP to hidden (pcModeSwitch = -1) without touching
    // App Visibility as the body was never "shown" in the App sense, only in the scene graph
    const auto restoreIntermediateToHidden = [](App::DocumentObject* body) {
        auto* bodyVP = Gui::Application::Instance->getViewProvider(body);
        auto* vpdo = dynamic_cast<Gui::ViewProviderDocumentObject*>(bodyVP);
        if (!vpdo) {
            return;
        }
        Base::ObjectStatusLocker<App::Property::Status, App::Property> guard(
            App::Property::User1,
            &vpdo->Visibility
        );
        bodyVP->Gui::ViewProvider::hide();
    };

    if (matchingMember) {
        setDisplayMaskMode("Group");
        _shownBody = matchingMember;
        if (matchingMember == activatedBody) {
            setBodyVisible(matchingMember, true);
            _indirectActivation = false;
        }
        else {
            // Indirect: open the intermediate bodys scene so the nested chain is visible
            switchIntermediateToGroup(matchingMember);
            _indirectActivation = true;
        }
    }
    else if (_shownBody) {
        if (_indirectActivation) {
            restoreIntermediateToHidden(_shownBody);
        }
        else {
            setBodyVisible(_shownBody, false);
        }
        setDisplayMaskMode(getDefaultDisplayMode());
        _shownBody = nullptr;
        _indirectActivation = false;
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

const char* ViewProviderBoolean::getDefaultDisplayMode() const
{
    return "Flat Lines";
}

void ViewProviderBoolean::onChanged(const App::Property* prop)
{

    ViewProvider::onChanged(prop);

    if (prop == &Display) {
        const auto getDisplayMode = [this]() {
            if (Display.getValue() != 0) {
                return "Group";
            }

            if (auto bodyViewProvider = getBodyViewProvider()) {
                return bodyViewProvider->DisplayMode.getValueAsString();
            }

            return getDefaultDisplayMode();
        };

        setDisplayMode(getDisplayMode());
    }

    if (prop == &Visibility) {
        updateBasePreviewVisibility();
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
