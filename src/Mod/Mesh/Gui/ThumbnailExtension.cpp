// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QBuffer>
#include <QByteArray>
#include <QOpenGLFramebufferObjectFormat>

#include <Inventor/SbRotation.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>

#include <Gui/SoFCOffscreenRenderer.h>

#include "ThumbnailExtension.h"
#include "ViewProvider.h"


using namespace MeshGui;

Mesh::Extension3MF::Resource ThumbnailExtension3MF::addMesh(const Mesh::MeshObject& mesh)
{
    SoCoordinate3* coord = new SoCoordinate3();
    SoIndexedFaceSet* faces = new SoIndexedFaceSet();

    SoOrthographicCamera* cam = new SoOrthographicCamera();

    // The mesh geometry carries no normals or material. Without a SoShapeHints the face set is not
    // shaded (it renders as a flat, single-color silhouette); without a SoMaterial it falls back to
    // Coin's default near-white color. Either way the result blends into the white background and is
    // then erased by the transparent-background pass (which turns every white pixel transparent),
    // leaving an empty image. Give it shading and a distinct color so it survives and is visible.
    SoShapeHints* hints = new SoShapeHints();
    hints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    hints->creaseAngle = 0.5F;

    SoMaterial* material = new SoMaterial();
    material->diffuseColor.setValue(0.6F, 0.6F, 0.65F);

    SbRotation rot(-0.35355F, -0.14644F, -0.35355F, -0.85355F);

    // Light the faces the camera sees by aligning the light with the camera view direction.
    SoDirectionalLight* light = new SoDirectionalLight();
    SbVec3f lightDir(0.0F, 0.0F, -1.0F);
    rot.multVec(lightDir, lightDir);
    light->direction.setValue(lightDir);

    SoSeparator* root = new SoSeparator();
    root->ref();
    root->addChild(cam);
    root->addChild(light);
    root->addChild(hints);
    root->addChild(material);
    root->addChild(coord);
    root->addChild(faces);

    ViewProviderMeshBuilder().createMesh(mesh.getKernel(), coord, faces);

    cam->orientation.setValue(rot);
    SbViewportRegion vpr(256, 256);
    cam->viewAll(root, vpr);

    Gui::SoQtOffscreenRenderer renderer(vpr);
    // The renderer defaults to a 32-bit float internal texture format (GL_RGB32F_ARB). Reading
    // that framebuffer back into an 8-bit QImage yields a corrupted (magenta) image on some
    // drivers, so request a standard 8-bit format like the regular screenshot path does.
    renderer.setInternalTextureFormat(QOpenGLFramebufferObjectFormat().internalTextureFormat());
    renderer.setBackgroundColor(SbColor4f(1.0F, 1.0F, 1.0F, 0.0F));
    QImage img;
    renderer.render(root);
    renderer.writeToImage(img);
    root->unref();

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, "PNG");
    buffer.close();

    Mesh::Extension3MF::Resource res;
    res.extension = "png";
    res.contentType = "image/png";
    res.relationshipType
        = "http://schemas.openxmlformats.org/package/2006/relationships/metadata/thumbnail";
    res.fileContent = std::string(data.data(), data.size());
    setContentName(res);

    index++;
    return res;
}

void ThumbnailExtension3MF::setContentName(Mesh::Extension3MF::Resource& res)
{
    if (index == 0) {
        res.relationshipTarget = "/Metadata/thumbnail.png";
        res.fileNameInZip = "Metadata/thumbnail.png";
    }
    else {
        std::string suf = std::to_string(index);
        res.relationshipTarget = "/Metadata/thumbnail" + suf + ".png";
        res.fileNameInZip = "Metadata/thumbnail" + suf + ".png";
    }
}
