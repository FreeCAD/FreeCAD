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

#include "PreCompiled.h"

#include "Core/Iterator.h"
#include <Base/Builder3D.h>
#include <Base/Placement.h>

#include "WriterInventor.h"


using namespace MeshCore;

class WriterInventorImp
{
    Base::InventorBuilder& builder;

public:
    static bool isStreamInvalid(std::ostream& out)
    {
        return !out || out.bad();
    }

    WriterInventorImp(Base::InventorBuilder& builder)
        : builder(builder)
    {}

    void setupStream(std::ostream& out)
    {
        out.precision(6);
        out.setf(std::ios::fixed | std::ios::showpoint);
    }

    void addInfoNode()
    {
        Base::InfoItem info {"Created by FreeCAD <https://www.freecad.org>"};
        builder.addNode(info);
    }

    void addLabel(const MeshCore::MeshKernel& kernel)
    {
        std::stringstream str;
        str << "Triangle mesh contains " << kernel.CountPoints() << " vertices and "
            << kernel.CountFacets() << " faces";
        Base::LabelItem label {str.str().c_str()};
        builder.addNode(label);
    }

    void addTransformNode(const Base::Matrix4D& mat, bool append)
    {
        if (!append) {
            return;
        }

        Base::Placement placement;
        placement.fromMatrix(mat);

        Base::TransformItem item {placement};
        builder.addNode(item);
    }

    void addNormalNode(const MeshCore::MeshKernel& kernel)
    {
        MeshFacetIterator clIter(kernel), clEnd(kernel);
        const MeshGeomFacet* geomFacet = nullptr;

        // write out the normals of the facets
        std::vector<Base::Vector3f> normals;
        normals.reserve(kernel.CountFacets());

        clIter.Begin();
        clEnd.End();

        while (clIter < clEnd) {
            geomFacet = &(*clIter);
            normals.push_back(geomFacet->GetNormal());
            ++clIter;
        }

        builder.addNode(Base::NormalItem {normals});

        Base::NormalBindingItem binding;
        binding.setValue(Base::BindingElement::Binding::PerFace);
        builder.addNode(binding);
    }

    void addCoordinateNode(const MeshCore::MeshKernel& kernel)
    {
        const MeshPointArray& points = kernel.GetPoints();

        // coordinates of the vertices
        std::vector<Base::Vector3f> coords;
        coords.reserve(points.size());
        coords.insert(coords.begin(), points.begin(), points.end());

        builder.addNode(Base::Coordinate3Item {coords});
    }

    void addMaterialNode(const Material* material)
    {
        if (!material) {
            return;
        }

        auto transformColors = [](const std::vector<App::Color>& input) {
            std::vector<Base::ColorRGB> output;
            output.reserve(input.size());
            std::transform(input.cbegin(),
                           input.cend(),
                           std::back_inserter(output),
                           [](const App::Color& col) {
                               return Base::ColorRGB {col.r, col.g, col.b};
                           });

            return output;
        };

        Base::MaterialItem mat;
        mat.setAmbientColor(transformColors(material->ambientColor));
        mat.setDiffuseColor(transformColors(material->diffuseColor));
        mat.setSpecularColor(transformColors(material->specularColor));
        mat.setEmissiveColor(transformColors(material->emissiveColor));
        mat.setShininess(material->shininess);
        mat.setTransparency(material->transparency);
        builder.addNode(mat);
    }

    void addMaterialBindingNode(const Material* material)
    {
        if (!material) {
            return;
        }

        Base::MaterialBindingItem binding;
        switch (material->binding) {
            case MeshIO::PER_FACE:
                binding.setValue(Base::BindingElement::Binding::PerFace);
                break;
            case MeshIO::PER_VERTEX:
                binding.setValue(Base::BindingElement::Binding::PerVertex);
                break;
            default:
                binding.setValue(Base::BindingElement::Binding::Overall);
                break;
        }

        builder.addNode(binding);
    }

    void addIndexedFaceSetNode(const MeshCore::MeshKernel& kernel)
    {
        // and finally the facets with their point indices
        const MeshFacetArray& faces = kernel.GetFacets();
        std::vector<int> indices;
        indices.reserve(4 * faces.size());
        for (const auto& it : faces) {
            indices.push_back(static_cast<int>(it._aulPoints[0]));
            indices.push_back(static_cast<int>(it._aulPoints[1]));
            indices.push_back(static_cast<int>(it._aulPoints[2]));
            indices.push_back(-1);
        }

        builder.addNode(Base::IndexedFaceSetItem {indices});
    }
};

WriterInventor::WriterInventor(const MeshKernel& kernel, const Material* material)
    : _kernel(kernel)
    , _material(material)
{}

void WriterInventor::SetTransform(const Base::Matrix4D& mat)
{
    _transform = mat;
    if (mat != Base::Matrix4D()) {
        apply_transform = true;
    }
}

bool WriterInventor::Save(std::ostream& out)
{
    if (WriterInventorImp::isStreamInvalid(out)) {
        return false;
    }

    Base::InventorBuilder builder(out);
    builder.beginSeparator();

    WriterInventorImp writer {builder};
    writer.setupStream(out);
    writer.addInfoNode();
    writer.addLabel(_kernel);
    writer.addTransformNode(_transform, apply_transform);
    writer.addNormalNode(_kernel);
    writer.addCoordinateNode(_kernel);
    writer.addMaterialNode(_material);
    writer.addMaterialBindingNode(_material);
    writer.addIndexedFaceSetNode(_kernel);

    builder.endSeparator();

    return true;
}
