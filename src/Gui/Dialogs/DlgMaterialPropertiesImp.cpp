/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"

#include <App/PropertyStandard.h>

#include "Dialogs/DlgMaterialPropertiesImp.h"
#include "ui_DlgMaterialProperties.h"
#include "ViewProvider.h"

#include <Base/Tools.h>


using namespace Gui::Dialog;


/* TRANSLATOR Gui::Dialog::DlgMaterialPropertiesImp */

DlgMaterialPropertiesImp::DlgMaterialPropertiesImp(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui_DlgMaterialProperties)
{
    ui->setupUi(this);
    setupConnections();

    ui->ambientColor->setAutoChangeColor(true);
    ui->diffuseColor->setAutoChangeColor(true);
    ui->emissiveColor->setAutoChangeColor(true);
    ui->specularColor->setAutoChangeColor(true);
}

DlgMaterialPropertiesImp::~DlgMaterialPropertiesImp() = default;

void DlgMaterialPropertiesImp::setupConnections()
{
    // clang-format off
    connect(ui->ambientColor, &ColorButton::changed,
            this, &DlgMaterialPropertiesImp::onAmbientColorChanged);
    connect(ui->diffuseColor, &ColorButton::changed,
            this, &DlgMaterialPropertiesImp::onDiffuseColorChanged);
    connect(ui->emissiveColor, &ColorButton::clicked,
            this, &DlgMaterialPropertiesImp::onEmissiveColorChanged);
    connect(ui->specularColor, &ColorButton::clicked,
            this, &DlgMaterialPropertiesImp::onSpecularColorChanged);
    connect(ui->shininess, qOverload<int>(&QSpinBox::valueChanged),
            this, &DlgMaterialPropertiesImp::onShininessValueChanged);
    connect(ui->transparency, qOverload<int>(&QSpinBox::valueChanged),
            this, &DlgMaterialPropertiesImp::onTransparencyValueChanged);
    connect(ui->buttonReset, &QPushButton::clicked,
            this, &DlgMaterialPropertiesImp::onButtonReset);
    connect(ui->buttonDefault, &QPushButton::clicked,
            this, &DlgMaterialPropertiesImp::onButtonDefault);
    // clang-format on
}

void DlgMaterialPropertiesImp::setCustomMaterial(const App::Material& mat)
{
    customMaterial = mat;
    setButtonColors(customMaterial);
}

App::Material DlgMaterialPropertiesImp::getCustomMaterial() const
{
    return customMaterial;
}

void DlgMaterialPropertiesImp::setDefaultMaterial(const App::Material& mat)
{
    defaultMaterial = mat;
}

App::Material DlgMaterialPropertiesImp::getDefaultMaterial() const
{
    return defaultMaterial;
}

/**
 * Sets the ambient color.
 */
void DlgMaterialPropertiesImp::onAmbientColorChanged()
{
    customMaterial.ambientColor.setValue(ui->ambientColor->color());
}

/**
 * Sets the diffuse color.
 */
void DlgMaterialPropertiesImp::onDiffuseColorChanged()
{
    customMaterial.diffuseColor.setValue(ui->diffuseColor->color());
}

/**
 * Sets the emissive color.
 */
void DlgMaterialPropertiesImp::onEmissiveColorChanged()
{
    customMaterial.emissiveColor.setValue(ui->emissiveColor->color());
}

/**
 * Sets the specular color.
 */
void DlgMaterialPropertiesImp::onSpecularColorChanged()
{
    customMaterial.specularColor.setValue(ui->specularColor->color());
}

/**
 * Sets the current shininess.
 */
void DlgMaterialPropertiesImp::onShininessValueChanged(int sh)
{
    customMaterial.shininess = Base::fromPercent(sh);
}

/**
 * Sets the current transparency.
 */
void DlgMaterialPropertiesImp::onTransparencyValueChanged(int sh)
{
    customMaterial.transparency = Base::fromPercent(sh);
}

/**
 * Reset the colors to the Coin3D defaults
 */
void DlgMaterialPropertiesImp::onButtonReset()
{
    setCustomMaterial(getDefaultMaterial());
}

/**
 * Reset the colors to the current default
 */
void DlgMaterialPropertiesImp::onButtonDefault()
{
    App::Material mat = App::Material::getDefaultAppearance();
    setCustomMaterial(mat);
}

/**
 * Sets the button colors to match the current material settings.
 */
void DlgMaterialPropertiesImp::setButtonColors(const App::Material& mat)
{
    ui->ambientColor->setColor(mat.ambientColor.asValue<QColor>());
    ui->diffuseColor->setColor(mat.diffuseColor.asValue<QColor>());
    ui->emissiveColor->setColor(mat.emissiveColor.asValue<QColor>());
    ui->specularColor->setColor(mat.specularColor.asValue<QColor>());
    ui->shininess->blockSignals(true);
    ui->shininess->setValue((int)(100.0F * (mat.shininess + 0.001F)));
    ui->shininess->blockSignals(false);
    ui->transparency->blockSignals(true);
    ui->transparency->setValue((int)(100.0F * (mat.transparency + 0.001F)));
    ui->transparency->blockSignals(false);
}

#include "moc_DlgMaterialPropertiesImp.cpp"
