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

#ifndef MESH_IMPORTER_H
#define MESH_IMPORTER_H

#include <string>
#include <vector>

namespace App
{
class Document;
class Color;
}  // namespace App

namespace MeshCore
{
struct Material;
}
namespace Mesh
{
class MeshObject;
class Feature;

class Importer
{
public:
    explicit Importer(App::Document*);
    void load(const std::string& fileName);

private:
    void addVertexColors(Feature*, const std::vector<App::Color>&);
    void addFaceColors(Feature*, const std::vector<App::Color>&);
    void addColors(Feature*, const std::string& property, const std::vector<App::Color>&);
    Feature* createMesh(const std::string& name, MeshObject&);
    void createMeshFromSegments(const std::string& name, MeshCore::Material& mat, MeshObject& mesh);

private:
    App::Document* document;
};

}  // namespace Mesh

#endif  // MESH_IMPORTER_H
