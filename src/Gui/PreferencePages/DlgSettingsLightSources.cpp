// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <Inventor/events/SoEvent.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoComplexity.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSphere.h>
#endif

#include "DlgSettingsLightSources.h"
#include "ui_DlgSettingsLightSources.h"

#include <Utilities.h>
#include <Base/Builder3D.h>
#include <Base/Tools.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/View3DSettings.h>


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsLightSources */

DlgSettingsLightSources::DlgSettingsLightSources(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsLightSources)
{
    ui->setupUi(this);

    view = ui->viewer;

    configureViewer();

    const auto connectLightEvents = [&](QuantitySpinBox* horizontalAngleSpinBox,
                                        QuantitySpinBox* verticalAngleSpinBox,
                                        QSpinBox* intensitySpinBox,
                                        ColorButton* colorButton,
                                        QCheckBox* enabledCheckbox,
                                        auto updateLightFunction) {
        connect(horizontalAngleSpinBox,
                qOverload<double>(&QuantitySpinBox::valueChanged),
                this,
                updateLightFunction);
        connect(verticalAngleSpinBox,
                qOverload<double>(&QuantitySpinBox::valueChanged),
                this,
                updateLightFunction);
        connect(intensitySpinBox,
                qOverload<int>(&QSpinBox::valueChanged),
                this,
                updateLightFunction);
        connect(colorButton, &ColorButton::changed, this, updateLightFunction);
#if QT_VERSION >= QT_VERSION_CHECK(6,7,0)
        connect(enabledCheckbox, &QCheckBox::checkStateChanged, this, updateLightFunction);
#else
        connect(enabledCheckbox, &QCheckBox::stateChanged, this, updateLightFunction);
#endif
    };

    const auto updateLight = [&](SoDirectionalLight* light,
                                 QuantitySpinBox* horizontalAngleSpinBox,
                                 QuantitySpinBox* verticalAngleSpinBox,
                                 QSpinBox* intensitySpinBox,
                                 ColorButton* colorButton,
                                 QCheckBox* enabledCheckbox,
                                 std::function<void(bool)> setLightEnabled) {
        light->color = Base::convertTo<SbColor>(colorButton->color());
        light->intensity = Base::fromPercent(intensitySpinBox->value());
        light->direction =
            Base::convertTo<SbVec3f>(azimuthElevationToDirection(horizontalAngleSpinBox->rawValue(),
                                                                 verticalAngleSpinBox->rawValue()));
        setLightEnabled(enabledCheckbox->isChecked());
    };

    const auto updateHeadLight = [&] {
        updateLight(view->getHeadlight(),
                    ui->mainLightHorizontalAngle,
                    ui->mainLightVerticalAngle,
                    ui->mainLightIntensitySpinBox,
                    ui->mainLightColor,
                    ui->mainLightEnable,
                    [&](bool enabled) {
                        view->setHeadlightEnabled(enabled);
                    });
    };

    const auto updateFillLight = [&] {
        updateLight(view->getFillLight(),
                    ui->fillLightHorizontalAngle,
                    ui->fillLightVerticalAngle,
                    ui->fillLightIntensitySpinBox,
                    ui->fillLightColor,
                    ui->fillLightEnable,
                    [&](bool enabled) {
                        view->setFillLightEnabled(enabled);
                    });
    };

    const auto updateBackLight = [&] {
        updateLight(view->getBacklight(),
                    ui->backLightHorizontalAngle,
                    ui->backLightVerticalAngle,
                    ui->backLightIntensitySpinBox,
                    ui->backLightColor,
                    ui->backLightEnable,
                    [&](bool enabled) {
                        view->setBacklightEnabled(enabled);
                    });
    };

    connectLightEvents(ui->mainLightHorizontalAngle,
                       ui->mainLightVerticalAngle,
                       ui->mainLightIntensitySpinBox,
                       ui->mainLightColor,
                       ui->mainLightEnable,
                       updateHeadLight);
    connectLightEvents(ui->backLightHorizontalAngle,
                       ui->backLightVerticalAngle,
                       ui->backLightIntensitySpinBox,
                       ui->backLightColor,
                       ui->backLightEnable,
                       updateBackLight);
    connectLightEvents(ui->fillLightHorizontalAngle,
                       ui->fillLightVerticalAngle,
                       ui->fillLightIntensitySpinBox,
                       ui->fillLightColor,
                       ui->fillLightEnable,
                       updateFillLight);

    const auto updateAmbientLight = [&] {
        view->getEnvironment()->ambientColor =
            Base::convertTo<SbColor>(ui->ambientLightColor->color());
        view->getEnvironment()->ambientIntensity =
            Base::fromPercent(ui->ambientLightIntensitySpinBox->value());
    };

    connect(ui->ambientLightIntensitySpinBox,
            qOverload<int>(&QSpinBox::valueChanged),
            this,
            updateAmbientLight);
    connect(ui->ambientLightColor, &ColorButton::changed, this, updateAmbientLight);

    connect(ui->zoomInButton, &QToolButton::clicked, this, &DlgSettingsLightSources::zoomIn);
    connect(ui->zoomOutButton, &QToolButton::clicked, this, &DlgSettingsLightSources::zoomOut);

    DlgSettingsLightSources::loadSettings();
}

static inline SoMaterial* createMaterial(void)
{
    const QColor ambientColor {0xff333333}, diffuseColor {0xffd2d2ff}, emissiveColor {0xff000000},
        specularColor {0xffcccccc};

    auto material = new SoMaterial();

    material->ambientColor.setValue(Base::convertTo<SbColor>(ambientColor));
    material->diffuseColor.setValue(Base::convertTo<SbColor>(diffuseColor));
    material->emissiveColor.setValue(Base::convertTo<SbColor>(emissiveColor));
    material->specularColor.setValue(Base::convertTo<SbColor>(specularColor));

    material->shininess = 0.9f;

    return material;
}

static inline SoSphere* createSphere(void)
{
    auto sphere = new SoSphere();
    sphere->radius = 3;

    return sphere;
}

static inline SoComplexity* createGoodComplexity()
{
    auto complexity = new SoComplexity();
    complexity->value = 1.0;

    return complexity;
}

void DlgSettingsLightSources::configureViewer()
{
    const SbVec3f defaultViewDirection {0.0f, 1.0f, 0.3f};

    View3DSettings(hGrp, view).applySettings();

    view->setRedirectToSceneGraph(true);
    view->setViewing(true);
    view->setPopupMenuEnabled(false);
    view->setEnabledNaviCube(false);

    const auto root = static_cast<SoSeparator*>(view->getSceneGraph());
    root->addChild(createGoodComplexity());
    root->addChild(createMaterial());
    root->addChild(createSphere());

    const auto callback = new SoEventCallback();
    root->addChild(callback);
    callback->addEventCallback(SoEvent::getClassTypeId(),
                               []([[maybe_unused]] void* ud, SoEventCallback* cb) {
                                   cb->setHandled();
                               });

    view->setCameraType(SoOrthographicCamera::getClassTypeId());
    view->setViewDirection(defaultViewDirection);
    view->viewAll();

    camera = dynamic_cast<SoOrthographicCamera*>(view->getCamera());
    const float cameraHeight = camera->height.getValue() * 2.0f;
    camera->height = cameraHeight;
    zoomStep = cameraHeight / 14.0f;
}

Base::Vector3d DlgSettingsLightSources::azimuthElevationToDirection(double azimuth,
                                                                    double elevation)
{
    azimuth = Base::toRadians(azimuth);
    elevation = Base::toRadians(elevation);

    auto direction = Base::Vector3d {std::sin(azimuth) * std::cos(elevation),
                                     std::cos(azimuth) * std::cos(elevation),
                                     std::sin(elevation)};

    direction.Normalize();

    return direction;
}

std::pair<double, double>
DlgSettingsLightSources::directionToAzimuthElevation(Base::Vector3d direction)
{
    const auto azimuth = std::atan2(direction[0], direction[1]);
    const auto elevation =
        std::atan2(direction[2],
                   std::sqrt(direction[1] * direction[1] + direction[0] * direction[0]));

    return {Base::toDegrees(azimuth), Base::toDegrees(elevation)};
}

void DlgSettingsLightSources::saveSettings()
{
    for (const auto& widget : findChildren<QWidget*>()) {
        if (const auto pref = dynamic_cast<PrefWidget*>(widget)) {
            pref->onSave();
        }
    }

    const auto saveAngles = [&](QuantitySpinBox* horizontalAngleSpinBox,
                                QuantitySpinBox* verticalAngleSpinBox,
                                const char* parameter) {
        try {
            const auto direction = azimuthElevationToDirection(horizontalAngleSpinBox->rawValue(),
                                                               verticalAngleSpinBox->rawValue());

            hGrp->SetASCII(parameter,
                           Base::vectorToString(Base::convertTo<Base::Vector3f>(direction)));
        }
        catch (...) {
        }
    };

    saveAngles(ui->mainLightHorizontalAngle, ui->mainLightVerticalAngle, "HeadlightDirection");
    saveAngles(ui->backLightHorizontalAngle, ui->backLightVerticalAngle, "BacklightDirection");
    saveAngles(ui->fillLightHorizontalAngle, ui->fillLightVerticalAngle, "FillLightDirection");
}

void DlgSettingsLightSources::loadSettings()
{
    for (const auto& widget : findChildren<QWidget*>()) {
        if (const auto pref = dynamic_cast<PrefWidget*>(widget)) {
            pref->onRestore();
        }
    }

    const auto loadAngles = [&](QuantitySpinBox* horizontalAngleSpinBox,
                                QuantitySpinBox* verticalAngleSpinBox,
                                const char* parameter) {
        try {
            const auto direction = Base::stringToVector(hGrp->GetASCII(parameter));
            const auto [azimuth, elevation] =
                directionToAzimuthElevation(Base::convertTo<Base::Vector3d>(direction));

            horizontalAngleSpinBox->setValue(azimuth);
            verticalAngleSpinBox->setValue(elevation);
        }
        catch (...) {
        }
    };

    loadAngles(ui->mainLightHorizontalAngle, ui->mainLightVerticalAngle, "HeadlightDirection");
    loadAngles(ui->backLightHorizontalAngle, ui->backLightVerticalAngle, "BacklightDirection");
    loadAngles(ui->fillLightHorizontalAngle, ui->fillLightVerticalAngle, "FillLightDirection");
}

void DlgSettingsLightSources::resetSettingsToDefaults()
{
    PreferencePage::resetSettingsToDefaults();

    hGrp->RemoveASCII("HeadlightDirection");
    hGrp->RemoveASCII("BacklightDirection");
    hGrp->RemoveASCII("FillLightDirection");

    loadSettings();
    configureViewer();
}

void DlgSettingsLightSources::zoomIn() const
{
    if (camera == nullptr) {
        return;
    }

    camera->height = camera->height.getValue() - zoomStep;
}

void DlgSettingsLightSources::zoomOut() const
{
    if (camera == nullptr) {
        return;
    }

    camera->height = camera->height.getValue() + zoomStep;
}

void DlgSettingsLightSources::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    PreferencePage::changeEvent(event);
}

#include "moc_DlgSettingsLightSources.cpp"
