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
{
    ui->setupUi(this);
    setupConnections();

    if (material != "ShapeMaterial") {
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
}

QColor DlgMaterialPropertiesImp::diffuseColor() const
{
    return ui->diffuseColor->color();
}

/**
 * Sets the ambient color.
 */
void DlgMaterialPropertiesImp::onAmbientColorChanged()
{
    QColor col = ui->ambientColor->color();
    float r = (float)col.red() / 255.0f;
    float g = (float)col.green() / 255.0f;
    float b = (float)col.blue() / 255.0f;
    App::Color ambient(r, g, b);

    for (std::vector<ViewProvider*>::iterator it= Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            auto ShapeMaterial = static_cast<App::PropertyMaterial*>(prop);
            App::Material mat = ShapeMaterial->getValue();
            mat.ambientColor = ambient;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the diffuse color.
 */
void DlgMaterialPropertiesImp::onDiffuseColorChanged()
{
    QColor col = ui->diffuseColor->color();
    float r = (float)col.red() / 255.0f;
    float g = (float)col.green() / 255.0f;
    float b = (float)col.blue() / 255.0f;
    App::Color diffuse(r, g, b);

    for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            auto ShapeMaterial = static_cast<App::PropertyMaterial*>(prop);
            App::Material mat = ShapeMaterial->getValue();
            mat.diffuseColor = diffuse;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the emissive color.
 */
void DlgMaterialPropertiesImp::onEmissiveColorChanged()
{
    QColor col = ui->emissiveColor->color();
    float r = (float)col.red() / 255.0f;
    float g = (float)col.green() / 255.0f;
    float b = (float)col.blue() / 255.0f;
    App::Color emissive(r, g, b);

    for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            auto ShapeMaterial = static_cast<App::PropertyMaterial*>(prop);
            App::Material mat = ShapeMaterial->getValue();
            mat.emissiveColor = emissive;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the specular color.
 */
void DlgMaterialPropertiesImp::onSpecularColorChanged()
{
    QColor col = ui->specularColor->color();
    float r = (float)col.red() / 255.0f;
    float g = (float)col.green() / 255.0f;
    float b = (float)col.blue() / 255.0f;
    App::Color specular(r, g, b);

    for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            auto ShapeMaterial = static_cast<App::PropertyMaterial*>(prop);
            App::Material mat = ShapeMaterial->getValue();
            mat.specularColor = specular;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the current shininess.
 */
void DlgMaterialPropertiesImp::onShininessValueChanged(int sh)
{
    float shininess = (float)sh / 100.0f;
    for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            auto ShapeMaterial = static_cast<App::PropertyMaterial*>(prop);
            App::Material mat = ShapeMaterial->getValue();
            mat.shininess = shininess;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the document objects and their view providers to manipulate the material.
 */
void DlgMaterialPropertiesImp::setViewProviders(const std::vector<Gui::ViewProvider*>& Obj)
{
    Objects = Obj;

    for (std::vector<ViewProvider*>::iterator it = Objects.begin(); it != Objects.end(); ++it) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            auto ShapeMaterial = static_cast<App::PropertyMaterial*>(prop);
            App::Material mat = ShapeMaterial->getValue();
            int r = int(mat.ambientColor.r * 255.0f);
            int g = int(mat.ambientColor.g * 255.0f);
            int b = int(mat.ambientColor.b * 255.0f);
            ui->ambientColor->setColor(QColor(r, g, b));
            r = int(mat.diffuseColor.r * 255.0f);
            g = int(mat.diffuseColor.g * 255.0f);
            b = int(mat.diffuseColor.b * 255.0f);
            ui->diffuseColor->setColor(QColor(r, g, b));
            r = int(mat.emissiveColor.r * 255.0f);
            g = int(mat.emissiveColor.g * 255.0f);
            b = int(mat.emissiveColor.b * 255.0f);
            ui->emissiveColor->setColor(QColor(r, g, b));
            r = int(mat.specularColor.r * 255.0f);
            g = int(mat.specularColor.g * 255.0f);
            b = int(mat.specularColor.b * 255.0f);
            ui->specularColor->setColor(QColor(r, g, b));
            ui->shininess->blockSignals(true);
            ui->shininess->setValue((int)(100.0f * (mat.shininess + 0.001f)));
            ui->shininess->blockSignals(false);
            break;
        }
    }
}

#include "moc_DlgMaterialPropertiesImp.cpp"
