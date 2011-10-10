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
#include <App/Material.h>

#include "DlgMaterialPropertiesImp.h"
#include "Widgets.h"
#include "SpinBox.h"
#include "ViewProvider.h"

using namespace Gui::Dialog;


/* TRANSLATOR Gui::Dialog::DlgMaterialPropertiesImp */

/**
 *  Constructs a Gui::Dialog::DlgMaterialPropertiesImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgMaterialPropertiesImp::DlgMaterialPropertiesImp(const std::string& mat, QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl), material(mat)
{
    this->setupUi(this);
    if (material != "ShapeMaterial") {
        this->textLabel1->hide();
        this->diffuseColor->hide();
    }

    ambientColor->setModal(false);
    diffuseColor->setModal(false);
    emissiveColor->setModal(false);
    specularColor->setModal(false);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgMaterialPropertiesImp::~DlgMaterialPropertiesImp()
{
}

/**
 * Sets the ambient color.
 */
void DlgMaterialPropertiesImp::on_ambientColor_changed()
{
    QColor col = ambientColor->color();
    float r = (float)col.red()/255.0f;
    float g = (float)col.green()/255.0f;
    float b = (float)col.blue()/255.0f;
    App::Color ambient(r,g,b);

    for (std::vector<ViewProvider*>::iterator it= Objects.begin();it!=Objects.end();it++) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            App::PropertyMaterial* ShapeMaterial = (App::PropertyMaterial*)prop;
            App::Material mat = ShapeMaterial->getValue();
            mat.ambientColor = ambient;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the diffuse color.
 */
void DlgMaterialPropertiesImp::on_diffuseColor_changed()
{
    QColor col = diffuseColor->color();
    float r = (float)col.red()/255.0f;
    float g = (float)col.green()/255.0f;
    float b = (float)col.blue()/255.0f;
    App::Color diffuse(r,g,b);

    for (std::vector<ViewProvider*>::iterator it= Objects.begin();it!=Objects.end();it++) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            App::PropertyMaterial* ShapeMaterial = (App::PropertyMaterial*)prop;
            App::Material mat = ShapeMaterial->getValue();
            mat.diffuseColor = diffuse;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the emissive color.
 */
void DlgMaterialPropertiesImp::on_emissiveColor_changed()
{
    QColor col = emissiveColor->color();
    float r = (float)col.red()/255.0f;
    float g = (float)col.green()/255.0f;
    float b = (float)col.blue()/255.0f;
    App::Color emissive(r,g,b);

    for (std::vector<ViewProvider*>::iterator it= Objects.begin();it!=Objects.end();it++) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            App::PropertyMaterial* ShapeMaterial = (App::PropertyMaterial*)prop;
            App::Material mat = ShapeMaterial->getValue();
            mat.emissiveColor = emissive;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the specular color.
 */
void DlgMaterialPropertiesImp::on_specularColor_changed()
{
    QColor col = specularColor->color();
    float r = (float)col.red()/255.0f;
    float g = (float)col.green()/255.0f;
    float b = (float)col.blue()/255.0f;
    App::Color specular(r,g,b);

    for (std::vector<ViewProvider*>::iterator it= Objects.begin();it!=Objects.end();it++) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            App::PropertyMaterial* ShapeMaterial = (App::PropertyMaterial*)prop;
            App::Material mat = ShapeMaterial->getValue();
            mat.specularColor = specular;
            ShapeMaterial->setValue(mat);
        }
    }
}

/**
 * Sets the current shininess.
 */
void DlgMaterialPropertiesImp::on_shininess_valueChanged(int sh)
{
    float shininess = (float)sh/100.0f;
    for (std::vector<ViewProvider*>::iterator it= Objects.begin();it!=Objects.end();it++) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            App::PropertyMaterial* ShapeMaterial = (App::PropertyMaterial*)prop;
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

    for (std::vector<ViewProvider*>::iterator it= Objects.begin();it!=Objects.end();it++) {
        App::Property* prop = (*it)->getPropertyByName(material.c_str());
        if (prop && prop->getTypeId().isDerivedFrom(App::PropertyMaterial::getClassTypeId())) {
            App::PropertyMaterial* ShapeMaterial = (App::PropertyMaterial*)prop;
            App::Material mat = ShapeMaterial->getValue();
            int r = int(mat.ambientColor.r * 255.0f);
            int g = int(mat.ambientColor.g * 255.0f);
            int b = int(mat.ambientColor.b * 255.0f);
            ambientColor->setColor( QColor(r,g,b) );
            r = int(mat.diffuseColor.r * 255.0f);
            g = int(mat.diffuseColor.g * 255.0f);
            b = int(mat.diffuseColor.b * 255.0f);
            diffuseColor->setColor( QColor(r,g,b) );
            r = int(mat.emissiveColor.r * 255.0f);
            g = int(mat.emissiveColor.g * 255.0f);
            b = int(mat.emissiveColor.b * 255.0f);
            emissiveColor->setColor( QColor(r,g,b) );
            r = int(mat.specularColor.r * 255.0f);
            g = int(mat.specularColor.g * 255.0f);
            b = int(mat.specularColor.b * 255.0f);
            specularColor->setColor( QColor(r,g,b) );
            shininess->blockSignals(true);
            shininess->setValue((int)(100.0f * (mat.shininess+0.001f)));
            shininess->blockSignals(false);
            break;
        }
    }
}

#include "moc_DlgMaterialPropertiesImp.cpp"
