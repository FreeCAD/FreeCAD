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

#include <App/Color.h>
#include <App/Material.h>
#include <App/PropertyStandard.h>

#include "Application.h"
#include "DlgMaterialPropertiesImp.h"
#include "ui_DlgMaterialProperties.h"
#include "ViewProvider.h"


using namespace Gui::Dialog;


/* TRANSLATOR Gui::Dialog::DlgMaterialPropertiesImp */

/**
 *  Constructs a Gui::Dialog::DlgMaterialPropertiesImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgMaterialPropertiesImp::DlgMaterialPropertiesImp(const std::string& mat, QWidget* parent,
                                                   Qt::WindowFlags fl)
  : QDialog(parent, fl)
  , ui(new Ui_DlgMaterialProperties)
  , material(mat)
  , updateColors(true)
{
    ui->setupUi(this);
    setupConnections();

    if (material != "ShapeAppearance") {
        ui->textLabel1->hide();
        ui->diffuseColor->hide();
    }

    ui->ambientColor->setAutoChangeColor(true);
    ui->diffuseColor->setAutoChangeColor(true);
    ui->emissiveColor->setAutoChangeColor(true);
    ui->specularColor->setAutoChangeColor(true);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgMaterialPropertiesImp::~DlgMaterialPropertiesImp() = default;

void DlgMaterialPropertiesImp::setupConnections()
{
    Base::Console().Log("DlgMaterialPropertiesImp::setupConnections()\n");

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
    connect(ui->buttonReset, &QPushButton::clicked,
            this, &DlgMaterialPropertiesImp::onButtonReset);
    connect(ui->buttonDefault, &QPushButton::clicked,
            this, &DlgMaterialPropertiesImp::onButtonDefault);
}

QColor DlgMaterialPropertiesImp::diffuseColor() const
{
    return ui->diffuseColor->color();
}

App::Material DlgMaterialPropertiesImp::getMaterial()
{
    for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->isDerivedFrom<App::PropertyMaterialList>()) {
            auto ShapeAppearance = static_cast<App::PropertyMaterialList*>(prop);
            auto& materials = ShapeAppearance->getValues();
            auto mat = materials[0];

            mat.setType(App::Material::USER_DEFINED);

            // Reset any texture data
            mat.image = "";
            mat.imagePath = "";
            mat.uuid = "";

            return mat;
        }
    }

    return App::Material::getDefaultAppearance();
}

App::Color DlgMaterialPropertiesImp::getColor(const QColor& color) const
{
    float r = (float)color.red() / 255.0F;
    float g = (float)color.green() / 255.0F;
    float b = (float)color.blue() / 255.0F;

    return App::Color(r, g, b);
}

/**
 * Sets the ambient color.
 */
void DlgMaterialPropertiesImp::onAmbientColorChanged()
{
    if (updateColors) {
        App::Color ambient = getColor(ui->ambientColor->color());

        for (std::vector<ViewProvider*>::iterator it= Objects.begin(); it != Objects.end(); ++it) {
            App::Property* prop = (*it)->getPropertyByName(material.c_str());
            if (prop && prop->isDerivedFrom<App::PropertyMaterialList>()) {
                auto ShapeAppearance = static_cast<App::PropertyMaterialList*>(prop);
                auto mat = getMaterial();
                mat.ambientColor = ambient;
                ShapeAppearance->setValue(mat);
            }
        }
    }
}

/**
 * Sets the diffuse color.
 */
void DlgMaterialPropertiesImp::onDiffuseColorChanged()
{
    if (updateColors) {
        App::Color diffuse = getColor(ui->diffuseColor->color());

        for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
            App::Property* prop = (*it)->getPropertyByName(material.c_str());
            if (prop && prop->isDerivedFrom<App::PropertyMaterialList>()) {
                auto ShapeAppearance = static_cast<App::PropertyMaterialList*>(prop);
                auto mat = getMaterial();
                mat.diffuseColor = diffuse;
                ShapeAppearance->setValue(mat);
            }
        }
    }
}

/**
 * Sets the emissive color.
 */
void DlgMaterialPropertiesImp::onEmissiveColorChanged()
{
    if (updateColors) {
        App::Color emissive = getColor(ui->emissiveColor->color());

        for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
            App::Property* prop = (*it)->getPropertyByName(material.c_str());
            if (prop && prop->isDerivedFrom<App::PropertyMaterialList>()) {
                auto ShapeAppearance = static_cast<App::PropertyMaterialList*>(prop);
                auto mat = getMaterial();
                mat.emissiveColor = emissive;
                ShapeAppearance->setValue(mat);
            }
        }
    }
}

/**
 * Sets the specular color.
 */
void DlgMaterialPropertiesImp::onSpecularColorChanged()
{
    if (updateColors) {
        App::Color specular = getColor(ui->specularColor->color());

        for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
            App::Property* prop = (*it)->getPropertyByName(material.c_str());
            if (prop && prop->isDerivedFrom<App::PropertyMaterialList>()) {
                auto ShapeAppearance = static_cast<App::PropertyMaterialList*>(prop);
                auto mat = getMaterial();
                mat.specularColor = specular;
                ShapeAppearance->setValue(mat);
            }
        }
    }
}

/**
 * Sets the current shininess.
 */
void DlgMaterialPropertiesImp::onShininessValueChanged(int sh)
{
    if (updateColors) {
        float shininess = (float)sh / 100.0F;
        for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
            App::Property* prop = (*it)->getPropertyByName(material.c_str());
            if (prop && prop->isDerivedFrom<App::PropertyMaterialList>()) {
                auto ShapeAppearance = static_cast<App::PropertyMaterialList*>(prop);
                auto mat = getMaterial();
                mat.shininess = shininess;
                ShapeAppearance->setValue(mat);
            }
        }
    }
}

/**
 * Reset the colors to the Coin3D defaults
 */
void DlgMaterialPropertiesImp::onButtonReset()
{
    for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->isDerivedFrom<App::PropertyMaterialList>()) {
            auto ShapeAppearance = static_cast<App::PropertyMaterialList*>(prop);
            auto mat = getMaterial();

            App::Color defaultAmbient(0.2, 0.2, 0.2);
            App::Color defaultDiffuse(0.8, 0.8, 0.8);
            App::Color black(0, 0, 0);

            mat.ambientColor = defaultAmbient;
            mat.diffuseColor = defaultDiffuse;
            mat.emissiveColor = black;
            mat.specularColor = black;
            mat.shininess = 0.2;
            ShapeAppearance->setValue(mat);

            setButtonColors();
            break;
        }
    }
}

/**
 * Reset the colors to the current default
 */
void DlgMaterialPropertiesImp::onButtonDefault()
{
    for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->isDerivedFrom<App::PropertyMaterialList>()) {
            auto ShapeAppearance = static_cast<App::PropertyMaterialList*>(prop);
            auto mat = App::Material::getDefaultAppearance();

            ShapeAppearance->setValue(mat);

            setButtonColors();
            break;
        }
    }
}

/**
 * Sets the document objects and their view providers to manipulate the material.
 */
void DlgMaterialPropertiesImp::setViewProviders(const std::vector<Gui::ViewProvider*>& Obj)
{
    Objects = Obj;

    setButtonColors();
}

/**
 * Sets the button colors to match the current material settings.
 */
void DlgMaterialPropertiesImp::setButtonColors()
{
    for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->isDerivedFrom<App::PropertyMaterialList>()) {
            auto ShapeAppearance = static_cast<App::PropertyMaterialList*>(prop);
            auto& materials = ShapeAppearance->getValues();
            auto& mat = materials[0];
            int r = int(mat.ambientColor.r * 255.0F);
            int g = int(mat.ambientColor.g * 255.0F);
            int b = int(mat.ambientColor.b * 255.0F);
            ui->ambientColor->setColor(QColor(r, g, b));
            r = int(mat.diffuseColor.r * 255.0F);
            g = int(mat.diffuseColor.g * 255.0F);
            b = int(mat.diffuseColor.b * 255.0F);
            ui->diffuseColor->setColor(QColor(r, g, b));
            r = int(mat.emissiveColor.r * 255.0F);
            g = int(mat.emissiveColor.g * 255.0F);
            b = int(mat.emissiveColor.b * 255.0F);
            ui->emissiveColor->setColor(QColor(r, g, b));
            r = int(mat.specularColor.r * 255.0F);
            g = int(mat.specularColor.g * 255.0F);
            b = int(mat.specularColor.b * 255.0F);
            ui->specularColor->setColor(QColor(r, g, b));
            ui->shininess->blockSignals(true);
            ui->shininess->setValue((int)(100.0f * (mat.shininess + 0.001F)));
            ui->shininess->blockSignals(false);
            break;
        }
    }
}

/**
 *  Constructs a Gui::Dialog::DlgFaceMaterialPropertiesImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 *
 *  This differs from Gui::Dialog::DlgMaterialPropertiesImp in that it does not update
 *  the underlying object. This is undesirable when we're updating only a single face.
 */
DlgFaceMaterialPropertiesImp::DlgFaceMaterialPropertiesImp(const std::string& mat,
                                                           QWidget* parent,
                                                           Qt::WindowFlags fl)
    : DlgMaterialPropertiesImp(mat, parent, fl)
{
    Base::Console().Log("DlgFaceMaterialPropertiesImp::DlgFaceMaterialPropertiesImp()\n");

    // Override the base class on this. Otherwise the whole object changes
    updateColors = false;
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgFaceMaterialPropertiesImp::~DlgFaceMaterialPropertiesImp() = default;

App::Material DlgFaceMaterialPropertiesImp::getCustomAppearance() const
{
    App::Material material;
    material.setType(App::Material::USER_DEFINED);

    material.ambientColor = getColor(ui->ambientColor->color());
    material.diffuseColor = getColor(ui->diffuseColor->color());
    material.emissiveColor = getColor(ui->emissiveColor->color());
    material.specularColor = getColor(ui->specularColor->color());
    material.shininess = (float)ui->shininess->value() / 100.0F;

    return material;
}

#include "moc_DlgMaterialPropertiesImp.cpp"
