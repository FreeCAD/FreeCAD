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
#include <Inventor/draggers/SoDirectionalLightDragger.h>
#include <Inventor/events/SoEvent.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoEventCallback.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSphere.h>
#endif

#include "DlgSettingsLightSources.h"
#include "ui_DlgSettingsLightSources.h"
#include <App/Application.h>
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
    createViewer();
}

static inline
SbVec3f getDirectionVector(const SbRotation &rotation)
{
    SbVec3f dir {0.0f, 0.0f, -1.0f};
    rotation.multVec(dir, dir);
    return dir;
}

static inline
void setLightDirection(const SbRotation &rotation, Gui::View3DInventorViewer *viewer)
{
    viewer->getHeadlight()->direction = getDirectionVector(rotation);
}

static inline
void setLightDraggerDirection(const SbRotation &rotation, SoDirectionalLightDragger *light_dragger)
{
    light_dragger->rotation = rotation;
}

static inline
void setValueSilently(QDoubleSpinBox *spn, const float val)
{
    Q_ASSERT_X(spn, "setValueSilently", "QDoubleSpinBox has been deleted");

    spn->blockSignals(true);
    spn->setValue(val);
    spn->blockSignals(false);
}

void DlgSettingsLightSources::dragMotionCallback(void *data, SoDragger *drag)
{
    auto lightdrag = dynamic_cast <SoDirectionalLightDragger *> (drag);
    auto self = static_cast<DlgSettingsLightSources*>(data);

    const SbRotation rotation = lightdrag->rotation.getValue();

    setLightDirection(rotation, self->view);

    setValueSilently(self->ui->q0_spnBox, rotation[0]);
    setValueSilently(self->ui->q1_spnBox, rotation[1]);
    setValueSilently(self->ui->q2_spnBox, rotation[2]);
    setValueSilently(self->ui->q3_spnBox, rotation[3]);

    const SbVec3f dir = getDirectionVector(rotation);

    setValueSilently(self->ui->x_spnBox, dir[0]);
    setValueSilently(self->ui->y_spnBox, dir[1]);
    setValueSilently(self->ui->z_spnBox, dir[2]);
}

static inline
SoMaterial *createMaterial(void)
{
    const QColor  ambientColor {0xff333333},
                  diffuseColor {0xffd2d2ff},
                 emissiveColor {0xff000000},
                 specularColor {0xffcccccc};

    auto material = new SoMaterial ();
    material->ambientColor.setValue  (ambientColor.redF(),  ambientColor.greenF(),  ambientColor.blueF());
    material->diffuseColor.setValue  (diffuseColor.redF(),  diffuseColor.greenF(),  diffuseColor.blueF());
    material->emissiveColor.setValue(emissiveColor.redF(), emissiveColor.greenF(), emissiveColor.blueF());
    material->specularColor.setValue(specularColor.redF(), specularColor.greenF(), specularColor.blueF());

    material->shininess = 0.9f;

    return material;
}

static inline
SoSphere *createSphere(void)
{
    auto sphere = new SoSphere();
    sphere->radius = 2;

    return sphere;
}

void DlgSettingsLightSources::createViewer()
{
    const QColor default_bg_color {180, 180, 180};
    const SbVec3f default_view_direction {1.0f, 1.0f, -5.0f};

    // NOLINTBEGIN
    view->setRedirectToSceneGraph(true);
    view->setViewing(true);
    view->setPopupMenuEnabled(false);

    view->setBackgroundColor(default_bg_color);
    view->setGradientBackground(Gui::View3DInventorViewer::NoGradient);
    view->setEnabledNaviCube(false);

    auto root = static_cast<SoSeparator*>(view->getSceneGraph());
    root->addChild(createDragger());
    root->addChild(createMaterial());
    root->addChild(createSphere());

    auto callback = new SoEventCallback();
    root->addChild(callback);
    callback->addEventCallback(SoEvent::getClassTypeId(),
                               [] (void* ud, SoEventCallback* cb) {
        Q_UNUSED(ud)
        cb->setHandled();
    });

    view->setCameraType(SoOrthographicCamera::getClassTypeId());
    view->setViewDirection(default_view_direction);
    view->viewAll();

    camera = dynamic_cast <SoOrthographicCamera *> (view->getCamera());
    const float camera_height = camera->height.getValue() * 2.0f;
    camera->height = camera_height;
    cam_step = camera_height / 14.0f;
    // NOLINTEND
}

SoDirectionalLightDragger* DlgSettingsLightSources::createDragger()
{
    // NOLINTBEGIN
    lightDragger = new SoDirectionalLightDragger();
    if (SoDragger* translator = dynamic_cast<SoDragger *>(lightDragger->getPart("translator", false))) {
        translator->setPartAsDefault("xTranslator.translatorActive", nullptr);
        translator->setPartAsDefault("yTranslator.translatorActive", nullptr);
        translator->setPartAsDefault("zTranslator.translatorActive", nullptr);
        translator->setPartAsDefault("xTranslator.translator", nullptr);
        translator->setPartAsDefault("yTranslator.translator", nullptr);
        translator->setPartAsDefault("zTranslator.translator", nullptr);
        SoNode* node = translator->getPart("yzTranslator.translator", false);
        if (node && node->isOfType(SoGroup::getClassTypeId())) {
            auto ps = new SoPickStyle();
            ps->style = SoPickStyle::UNPICKABLE;
            static_cast<SoGroup*>(node)->insertChild(ps, 0);
        }
    }

    lightDragger->addMotionCallback(dragMotionCallback, this);
    return lightDragger;
    // NOLINTEND
}

void DlgSettingsLightSources::saveSettings()
{
    ui->checkBoxLight1->onSave();
    ui->light1Color->onSave();
    ui->sliderIntensity1->onSave();
    saveDirection();
}

void DlgSettingsLightSources::loadSettings()
{
    ui->checkBoxLight1->onRestore();
    ui->light1Color->onRestore();
    ui->sliderIntensity1->onRestore();
    loadDirection();
    lightColor();
}

void DlgSettingsLightSources::resetSettingsToDefaults()
{
    ParameterGrp::handle grp = ui->sliderIntensity1->getWindowParameter();

    grp->SetFloat("HeadlightRotationX", 0.0);
    grp->SetFloat("HeadlightRotationY", 0.0);
    grp->SetFloat("HeadlightRotationZ", 0.0);
    grp->SetFloat("HeadlightRotationW", 1.0);

    grp->SetASCII("HeadlightDirection", "(0.0,0.0,-1.0)");

    PreferencePage::resetSettingsToDefaults();
}

void DlgSettingsLightSources::saveDirection()
{
    if (lightDragger) {
        const SbRotation rotation = lightDragger->rotation.getValue();
        const SbVec3f dir = getDirectionVector(rotation);
        const QString headlightDir = QString::fromLatin1("(%1,%2,%3)").arg(dir[0]).arg(dir[1]).arg(dir[2]);

        ParameterGrp::handle grp = ui->sliderIntensity1->getWindowParameter();

        grp->SetFloat("HeadlightRotationX", rotation[0]);
        grp->SetFloat("HeadlightRotationY", rotation[1]);
        grp->SetFloat("HeadlightRotationZ", rotation[2]);
        grp->SetFloat("HeadlightRotationW", rotation[3]);

        grp->SetASCII("HeadlightDirection", qPrintable(headlightDir));
    }
}

void DlgSettingsLightSources::loadDirection()
{
    ParameterGrp::handle grp = ui->sliderIntensity1->getWindowParameter();
    SbRotation rotation = lightDragger->rotation.getValue();

    auto get_q = [&grp](const char *name, const float def){return static_cast <float> (grp->GetFloat(name, def));};

    const float q0 = get_q("HeadlightRotationX", rotation[0]),
                q1 = get_q("HeadlightRotationY", rotation[1]),
                q2 = get_q("HeadlightRotationZ", rotation[2]),
                q3 = get_q("HeadlightRotationW", rotation[3]);

    rotation.setValue(q0, q1, q2, q3);

    setLightDirection(rotation, ui->viewer);
    setLightDraggerDirection(rotation, lightDragger);

    setValueSilently(ui->q0_spnBox, rotation[0]);
    setValueSilently(ui->q1_spnBox, rotation[1]);
    setValueSilently(ui->q2_spnBox, rotation[2]);
    setValueSilently(ui->q3_spnBox, rotation[3]);

    const SbVec3f dir = getDirectionVector(rotation);

    setValueSilently(ui->x_spnBox, dir[0]);
    setValueSilently(ui->y_spnBox, dir[1]);
    setValueSilently(ui->z_spnBox, dir[2]);
}

void DlgSettingsLightSources::toggleLight(bool on)
{
    if (view) {
        view->setHeadlightEnabled(on);
    }
}

void DlgSettingsLightSources::lightIntensity(int value)
{
    if (view) {
        view->getHeadlight()->intensity = static_cast <float> (value) / 100.0f;
    }
}

void DlgSettingsLightSources::lightColor()
{
    if (view) {
        const QColor color = ui->light1Color->color();
        view->getHeadlight()->color.setValue(color.redF(),
                                             color.greenF(),
                                             color.blueF());
    }
}

void DlgSettingsLightSources::pushIn(void)
{
    if (camera == nullptr)
        return;

    camera->height = camera->height.getValue() - cam_step;
}

void DlgSettingsLightSources::pullOut(void)
{
    if (camera == nullptr)
        return;

    camera->height = camera->height.getValue() + cam_step;
}

void DlgSettingsLightSources::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    PreferencePage::changeEvent(event);
}

void DlgSettingsLightSources::updateDraggerQS()
{
    const float q0 = ui->q0_spnBox->value(),
                q1 = ui->q1_spnBox->value(),
                q2 = ui->q2_spnBox->value(),
                q3 = ui->q3_spnBox->value();

    const SbRotation rotation {q0, q1, q2, q3};

    setLightDirection(rotation, view);
    setLightDraggerDirection(rotation, lightDragger);

    const SbVec3f dir = getDirectionVector(rotation);

    setValueSilently(ui->x_spnBox, dir[0]);
    setValueSilently(ui->y_spnBox, dir[1]);
    setValueSilently(ui->z_spnBox, dir[2]);
}

void DlgSettingsLightSources::updateDraggerXYZ()
{
    const float x = ui->x_spnBox->value(),
                y = ui->y_spnBox->value(),
                z = ui->z_spnBox->value();

    const SbRotation rotation {SbVec3f{0.0f, 0.0f, -1.0f}, SbVec3f{x, y, z}};

    setLightDirection(rotation, view);
    setLightDraggerDirection(rotation, lightDragger);

    setValueSilently(ui->q0_spnBox, rotation[0]);
    setValueSilently(ui->q1_spnBox, rotation[1]);
    setValueSilently(ui->q2_spnBox, rotation[2]);
    setValueSilently(ui->q3_spnBox, rotation[3]);
}


#include "moc_DlgSettingsLightSources.cpp"
