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
#include <QColor>
#include <QEvent>
#include <QGridLayout>
#include <Inventor/draggers/SoDirectionalLightDragger.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#endif

#include "DlgSettingsLightSources.h"
#include "ui_DlgSettingsLightSources.h"
#include <Gui/View3DInventorViewer.h>


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsLightSources */

DlgSettingsLightSources::DlgSettingsLightSources(QWidget* parent)
  : PreferencePage(parent)
  , ui(new Ui_DlgSettingsLightSources)
{
    ui->setupUi(this);
    setupConnection();
}

DlgSettingsLightSources::~DlgSettingsLightSources()
{
    delete view;
}

void DlgSettingsLightSources::setupConnection()
{
    connect(ui->checkBoxLight1, &QCheckBox::toggled,
            this, &DlgSettingsLightSources::toggleLight);
    connect(ui->sliderIntensity1, &QSlider::valueChanged,
            this, &DlgSettingsLightSources::lightIntensity);
    connect(ui->light1Color, &Gui::ColorButton::changed,
            this, &DlgSettingsLightSources::lightColor);
}

void DlgSettingsLightSources::showEvent(QShowEvent* event)
{
    Q_UNUSED(event)
    if (!view) {
        QGroupBox* box = ui->groupBoxLight;
        QWidget* widget = createViewer(box);
        auto grid = new QGridLayout(box);
        grid->addWidget(widget);
        box->setLayout(grid);

        loadDirection();
    }
}

void DlgSettingsLightSources::dragMotionCallback(void *data, SoDragger *drag)
{
    auto lightdrag = static_cast<SoDirectionalLightDragger*>(drag);  // NOLINT
    auto self = static_cast<DlgSettingsLightSources*>(data);
    SbRotation rotation = lightdrag->rotation.getValue();
    SbVec3f dir(0, 0, -1);
    rotation.multVec(dir, dir);
    self->view->getHeadlight()->direction = dir;
}

QWidget* DlgSettingsLightSources::createViewer(QWidget* parent)
{
    // NOLINTBEGIN
    view = new Gui::View3DInventorViewer(parent);
    view->setRedirectToSceneGraph(true);
    view->setViewing(true);
    view->setPopupMenuEnabled(false);

    view->setBackgroundColor(QColor(255, 255, 255));
    view->setGradientBackground(Gui::View3DInventorViewer::NoGradient);
    view->setEnabledNaviCube(false);

    auto root = static_cast<SoSeparator*>(view->getSceneGraph());
    root->addChild(createDragger());

    view->setCameraType(SoOrthographicCamera::getClassTypeId());
    view->setViewDirection(SbVec3f(1, 1, -5));
    view->viewAll();
    // NOLINTEND

    const int size = 250;
    view->resize(size, size);

    return view;
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
}

void DlgSettingsLightSources::saveDirection()
{
    if (lightDragger) {
        ParameterGrp::handle grp = ui->sliderIntensity1->getWindowParameter();
        SbRotation rotation = lightDragger->rotation.getValue();
        grp->SetFloat("HeadlightRotationX", rotation[0]);
        grp->SetFloat("HeadlightRotationY", rotation[1]);
        grp->SetFloat("HeadlightRotationZ", rotation[2]);
        grp->SetFloat("HeadlightRotationW", rotation[3]);

        SbVec3f dir(0, 0, -1);
        rotation.multVec(dir, dir);

        QString headlightDir = QString::fromLatin1("(%1,%2,%3)").arg(dir[0]).arg(dir[1]).arg(dir[2]);
        grp->SetASCII("HeadlightDirection", headlightDir.toLatin1());
    }
}

void DlgSettingsLightSources::loadDirection()
{
    ParameterGrp::handle grp = ui->sliderIntensity1->getWindowParameter();
    SbRotation rotation = lightDragger->rotation.getValue();
    // NOLINTBEGIN
    float q1 = float(grp->GetFloat("HeadlightRotationX", rotation[0]));
    float q2 = float(grp->GetFloat("HeadlightRotationY", rotation[1]));
    float q3 = float(grp->GetFloat("HeadlightRotationZ", rotation[2]));
    float q4 = float(grp->GetFloat("HeadlightRotationW", rotation[3]));
    // NOLINTEND
    rotation.setValue(q1, q2, q3, q4);
    lightDragger->rotation.setValue(rotation);

    SbVec3f direction(0, 0, -1);
    rotation.multVec(direction, direction);
    view->getHeadlight()->direction = direction;
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
        float intensity = float(value) / 100.0F;
        view->getHeadlight()->intensity = intensity;
    }
}

void DlgSettingsLightSources::lightColor()
{
    if (view) {
        QColor color = ui->light1Color->color();
        float red = float(color.redF());
        float green = float(color.greenF());
        float blue = float(color.blueF());
        view->getHeadlight()->color = SbColor(red, green, blue);
    }
}

void DlgSettingsLightSources::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    PreferencePage::changeEvent(event);
}


#include "moc_DlgSettingsLightSources.cpp"
