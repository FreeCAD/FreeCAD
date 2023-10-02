/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/Document.h>

#include "Importer.h"
#include "MeshFeature.h"


using namespace Mesh;


Importer::Importer(App::Document* doc)
    : document(doc)
{}

void Importer::load(const std::string& fileName)
{
    MeshObject mesh;
    MeshCore::Material mat;

    if (mesh.load(fileName.c_str(), &mat)) {
        Base::FileInfo file(fileName.c_str());
        unsigned long segmct = mesh.countSegments();
        if (segmct > 1) {
            createMeshFromSegments(file.fileNamePure(), mat, mesh);
        }
        else if (mat.binding == MeshCore::MeshIO::PER_VERTEX
                 && mat.diffuseColor.size() == mesh.countPoints()) {
            Feature* feature = createMesh(file.fileNamePure(), mesh);
            addVertexColors(feature, mat.diffuseColor);
            feature->purgeTouched();
        }
        else if (mat.binding == MeshCore::MeshIO::PER_FACE
                 && mat.diffuseColor.size() == mesh.countFacets()) {
            Feature* feature = createMesh(file.fileNamePure(), mesh);
            addFaceColors(feature, mat.diffuseColor);
            feature->purgeTouched();
        }
        else {
            Feature* feature = createMesh(file.fileNamePure(), mesh);
            feature->purgeTouched();
        }
    }
}

void Importer::addVertexColors(Feature* feature, const std::vector<App::Color>& colors)
{
    addColors(feature, "VertexColors", colors);
}

void Importer::addFaceColors(Feature* feature, const std::vector<App::Color>& colors)
{
    addColors(feature, "FaceColors", colors);
}

void Importer::addColors(Feature* feature,
                         const std::string& property,
                         const std::vector<App::Color>& colors)
{
    App::PropertyColorList* prop = static_cast<App::PropertyColorList*>(
        feature->addDynamicProperty("App::PropertyColorList", property.c_str()));
    if (prop) {
        prop->setValues(colors);
    }
}

void Importer::createMeshFromSegments(const std::string& name,
                                      MeshCore::Material& mat,
                                      MeshObject& mesh)
{
    unsigned long segmct = mesh.countSegments();
    for (unsigned long i = 0; i < segmct; i++) {
        const Segment& group = mesh.getSegment(i);
        std::string groupName = group.getName();
        if (groupName.empty()) {
            groupName = name;
        }

        std::unique_ptr<MeshObject> segm(mesh.meshFromSegment(group.getIndices()));
        Feature* feature = createMesh(groupName, *segm);

        // if colors are set per face
        if (mat.binding == MeshCore::MeshIO::PER_FACE
            && mat.diffuseColor.size() == mesh.countFacets()) {

            std::vector<App::Color> diffuseColor;
            diffuseColor.reserve(group.getIndices().size());
            for (const auto& it : group.getIndices()) {
                diffuseColor.push_back(mat.diffuseColor[it]);
            }

            addFaceColors(feature, diffuseColor);
        }
        feature->purgeTouched();
    }
}

Feature* Importer::createMesh(const std::string& name, MeshObject& mesh)
{
    Mesh::Feature* pcFeature =
        static_cast<Mesh::Feature*>(document->addObject("Mesh::Feature", name.c_str()));
    pcFeature->Label.setValue(name);
    pcFeature->Mesh.swapMesh(mesh);
    return pcFeature;
}
