// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 David Carter <dcarter@david.carter.ca>             *
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
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTexture3.h>
#endif

#include "ViewProviderTextureExtension.h"
#include <Gui/BitmapFactory.h>
#include <App/Material.h>


using namespace Gui;


EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderTextureExtension, Gui::ViewProviderExtension)


ViewProviderTextureExtension::ViewProviderTextureExtension()
{
    initExtensionType(ViewProviderTextureExtension::getExtensionClassTypeId());

    pcSwitchAppearance = new SoSwitch;
    pcSwitchAppearance->ref();
    pcSwitchAppearance->setName("SwitchAppearance");
    pcSwitchTexture = new SoSwitch;
    pcSwitchTexture->ref();
    pcSwitchTexture->setName("SwitchTexture");

    pcShapeTexture2D = new SoTexture2;
    pcShapeTexture2D->ref();
    pcShapeTexture2D->setName("ShapeTexture2D");

    pcTextureGroup3D = new SoGroup;
    pcTextureGroup3D->ref();
    pcTextureGroup3D->setName("TextureGroup3D");
}

void ViewProviderTextureExtension::setup(SoMaterial* pcShapeMaterial)
{
    // Materials go first, with textured faces drawing over them
    pcSwitchAppearance->addChild(pcShapeMaterial);
    pcSwitchAppearance->addChild(pcSwitchTexture);
    pcSwitchTexture->addChild(pcShapeTexture2D);
    pcSwitchTexture->addChild(pcTextureGroup3D);
    pcSwitchAppearance->whichChild.setValue(0);
    pcSwitchTexture->whichChild.setValue(SO_SWITCH_NONE);
}

ViewProviderTextureExtension::~ViewProviderTextureExtension()
{
    pcSwitchAppearance->unref();
    pcSwitchTexture->unref();
    pcShapeTexture2D->unref();
    pcTextureGroup3D->unref();
}

SoSwitch* ViewProviderTextureExtension::getAppearance() const
{
    return pcSwitchAppearance;
}

SoGroup* ViewProviderTextureExtension::getTextureGroup3D() const
{
    return pcTextureGroup3D;
}

void ViewProviderTextureExtension::setCoinAppearance(SoMaterial* pcShapeMaterial, const App::Material& source)
{
#if 0
    if (!source.image.empty()) {
        Base::Console().log("setCoinAppearance(Texture)\n");
        activateTexture2D();

        QByteArray by = QByteArray::fromBase64(QString::fromStdString(source.image).toUtf8());
        auto image = QImage::fromData(by, "PNG");  //.scaled(64, 64, Qt::KeepAspectRatio);

        SoSFImage texture;
        Gui::BitmapFactory().convert(image, texture);
        pcShapeTexture2D->image = texture;
    } else {
        Base::Console().log("setCoinAppearance(Material)\n");
        activateMaterial();
    }
#endif
    activateMaterial();

    // Always set the material for items such as lines that don't support textures
    pcShapeMaterial->ambientColor.setValue(source.ambientColor.r,
                                           source.ambientColor.g,
                                           source.ambientColor.b);
    pcShapeMaterial->diffuseColor.setValue(source.diffuseColor.r,
                                           source.diffuseColor.g,
                                           source.diffuseColor.b);
    pcShapeMaterial->specularColor.setValue(source.specularColor.r,
                                            source.specularColor.g,
                                            source.specularColor.b);
    pcShapeMaterial->emissiveColor.setValue(source.emissiveColor.r,
                                            source.emissiveColor.g,
                                            source.emissiveColor.b);
    pcShapeMaterial->shininess.setValue(source.shininess);
    pcShapeMaterial->transparency.setValue(source.transparency);
}

void ViewProviderTextureExtension::activateMaterial()
{
    pcSwitchAppearance->whichChild.setValue(0);
    pcSwitchTexture->whichChild.setValue(SO_SWITCH_NONE);
}

void ViewProviderTextureExtension::activateTexture2D()
{
    pcSwitchAppearance->whichChild.setValue(1);
    pcSwitchTexture->whichChild.setValue(0);
}

void ViewProviderTextureExtension::activateTexture3D()
{
    pcSwitchAppearance->whichChild.setValue(1);
    pcSwitchTexture->whichChild.setValue(1);
}

void ViewProviderTextureExtension::activateMixed3D()
{
    pcSwitchAppearance->whichChild.setValue(SO_SWITCH_ALL);
    pcSwitchTexture->whichChild.setValue(1);
}

// ------------------------------------------------------------------------------------------------

EXTENSION_PROPERTY_SOURCE(Gui::ViewProviderFaceTexture, Gui::ViewProviderTextureExtension)


ViewProviderFaceTexture::ViewProviderFaceTexture()
{
    initExtensionType(ViewProviderFaceTexture::getExtensionClassTypeId());

    // Support for textured faces
    pcShapeTexture3D = new SoTexture3;
    pcShapeTexture3D->ref();
    pcShapeTexture3D->setName("ShapeTexture3D");
    pcShapeCoordinates = new SoCoordinate3;
    pcShapeCoordinates->ref();
    pcShapeCoordinates->setName("ShapeCoordinates");
    pcShapeFaceset = new SoIndexedFaceSet;
    pcShapeFaceset->ref();
    pcShapeFaceset->setName("ShapeFaceset");
}

ViewProviderFaceTexture::~ViewProviderFaceTexture()
{
    pcShapeTexture3D->unref();
    pcShapeCoordinates->unref();
    pcShapeFaceset->unref();
}

void ViewProviderFaceTexture::setup(SoMaterial* mat)
{
    ViewProviderTextureExtension::setup(mat);

    getTextureGroup3D()->addChild(pcShapeTexture3D);
    getTextureGroup3D()->addChild(pcShapeCoordinates);
    getTextureGroup3D()->addChild(pcShapeFaceset);
}
