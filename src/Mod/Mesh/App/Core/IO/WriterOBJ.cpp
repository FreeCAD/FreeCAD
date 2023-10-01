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
#include <Base/Console.h>
#include <Base/Sequencer.h>
#include <Base/Tools.h>

#include "WriterOBJ.h"


using namespace MeshCore;

struct WriterOBJ::Color_Less
{
    bool operator()(const App::Color& x, const App::Color& y) const
    {
        if (x.r != y.r) {
            return x.r < y.r;
        }
        if (x.g != y.g) {
            return x.g < y.g;
        }
        if (x.b != y.b) {
            return x.b < y.b;
        }
        return false;  // equal colors
    }
};

WriterOBJ::WriterOBJ(const MeshKernel& kernel, const Material* material)
    : _kernel(kernel)
    , _material(material)
{}

void WriterOBJ::SetGroups(const std::vector<Group>& g)
{
    _groups = g;
}

void WriterOBJ::SetTransform(const Base::Matrix4D& mat)
{
    _transform = mat;
    if (mat != Base::Matrix4D()) {
        apply_transform = true;
    }
}

bool WriterOBJ::Save(std::ostream& out)
{
    const MeshPointArray& rPoints = _kernel.GetPoints();
    const MeshFacetArray& rFacets = _kernel.GetFacets();

    if (!out || out.bad()) {
        return false;
    }

    Base::SequencerLauncher seq("saving...", _kernel.CountPoints() + _kernel.CountFacets());
    bool exportColorPerVertex = false;
    bool exportColorPerFace = false;

    if (_material) {
        if (_material->binding == MeshIO::PER_FACE) {
            if (_material->diffuseColor.size() != rFacets.size()) {
                Base::Console().Warning("Cannot export color information because there is a "
                                        "different number of faces and colors");
            }
            else {
                exportColorPerFace = true;
            }
        }
        else if (_material->binding == MeshIO::PER_VERTEX) {
            if (_material->diffuseColor.size() != rPoints.size()) {
                Base::Console().Warning("Cannot export color information because there is a "
                                        "different number of points and colors");
            }
            else {
                exportColorPerVertex = true;
            }
        }
        else if (_material->binding == MeshIO::OVERALL) {
            if (_material->diffuseColor.empty()) {
                Base::Console().Warning(
                    "Cannot export color information because there is no color defined");
            }
            else {
                exportColorPerVertex = true;
            }
        }
    }

    // Header
    out << "# Created by FreeCAD <http://www.freecad.org>\n";
    if (exportColorPerFace) {
        out << "mtllib " << _material->library << '\n';
    }

    out.precision(6);
    out.setf(std::ios::fixed | std::ios::showpoint);

    // vertices
    Base::Vector3f pt;
    std::size_t index = 0;
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it, ++index) {
        if (this->apply_transform) {
            pt = this->_transform * *it;
        }
        else {
            pt.Set(it->x, it->y, it->z);
        }

        if (exportColorPerVertex) {
            App::Color c;
            if (_material->binding == MeshIO::PER_VERTEX) {
                c = _material->diffuseColor[index];
            }
            else {
                c = _material->diffuseColor.front();
            }

            int r = static_cast<int>(c.r * 255.0f);
            int g = static_cast<int>(c.g * 255.0f);
            int b = static_cast<int>(c.b * 255.0f);

            out << "v " << pt.x << " " << pt.y << " " << pt.z << " " << r << " " << g << " " << b
                << '\n';
        }
        else {
            out << "v " << pt.x << " " << pt.y << " " << pt.z << '\n';
        }
        seq.next(true);  // allow to cancel
    }
    // Export normals
    MeshFacetIterator clIter(_kernel), clEnd(_kernel);
    const MeshGeomFacet* pclFacet {};

    clIter.Begin();
    clEnd.End();

    while (clIter < clEnd) {
        pclFacet = &(*clIter);
        out << "vn " << pclFacet->GetNormal().x << " " << pclFacet->GetNormal().y << " "
            << pclFacet->GetNormal().z << '\n';
        ++clIter;
        seq.next(true);  // allow to cancel
    }

    if (_groups.empty()) {
        if (exportColorPerFace) {
            // facet indices (no texture and normal indices)

            // make sure to use the 'usemtl' statement as less often as possible
            std::vector<App::Color> colors = _material->diffuseColor;
            std::sort(colors.begin(), colors.end(), Color_Less());
            colors.erase(std::unique(colors.begin(), colors.end()), colors.end());

            std::size_t index = 0;
            App::Color prev;
            int faceIdx = 1;
            const std::vector<App::Color>& Kd = _material->diffuseColor;
            for (MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end();
                 ++it, index++) {
                if (index == 0 || prev != Kd[index]) {
                    prev = Kd[index];
                    std::vector<App::Color>::iterator c_it =
                        std::find(colors.begin(), colors.end(), prev);
                    if (c_it != colors.end()) {
                        out << "usemtl material_" << (c_it - colors.begin()) << '\n';
                    }
                }
                out << "f " << it->_aulPoints[0] + 1 << "//" << faceIdx << " "
                    << it->_aulPoints[1] + 1 << "//" << faceIdx << " " << it->_aulPoints[2] + 1
                    << "//" << faceIdx << '\n';
                seq.next(true);  // allow to cancel
                faceIdx++;
            }
        }
        else {
            // facet indices (no texture and normal indices)
            std::size_t faceIdx = 1;
            for (const auto& it : rFacets) {
                out << "f " << it._aulPoints[0] + 1 << "//" << faceIdx << " "
                    << it._aulPoints[1] + 1 << "//" << faceIdx << " " << it._aulPoints[2] + 1
                    << "//" << faceIdx << '\n';
                seq.next(true);  // allow to cancel
                faceIdx++;
            }
        }
    }
    else {
        if (exportColorPerFace) {
            // make sure to use the 'usemtl' statement as less often as possible
            std::vector<App::Color> colors = _material->diffuseColor;
            std::sort(colors.begin(), colors.end(), Color_Less());
            colors.erase(std::unique(colors.begin(), colors.end()), colors.end());

            bool first = true;
            App::Color prev;
            const std::vector<App::Color>& Kd = _material->diffuseColor;

            for (const auto& gt : _groups) {
                out << "g " << Base::Tools::escapedUnicodeFromUtf8(gt.name.c_str()) << '\n';
                for (FacetIndex it : gt.indices) {
                    const MeshFacet& f = rFacets[it];
                    if (first || prev != Kd[it]) {
                        first = false;
                        prev = Kd[it];
                        std::vector<App::Color>::iterator c_it =
                            std::find(colors.begin(), colors.end(), prev);
                        if (c_it != colors.end()) {
                            out << "usemtl material_" << (c_it - colors.begin()) << '\n';
                        }
                    }

                    out << "f " << f._aulPoints[0] + 1 << "//" << it + 1 << " "
                        << f._aulPoints[1] + 1 << "//" << it + 1 << " " << f._aulPoints[2] + 1
                        << "//" << it + 1 << '\n';
                    seq.next(true);  // allow to cancel
                }
            }
        }
        else {
            for (const auto& gt : _groups) {
                out << "g " << Base::Tools::escapedUnicodeFromUtf8(gt.name.c_str()) << '\n';
                for (FacetIndex it : gt.indices) {
                    const MeshFacet& f = rFacets[it];
                    out << "f " << f._aulPoints[0] + 1 << "//" << it + 1 << " "
                        << f._aulPoints[1] + 1 << "//" << it + 1 << " " << f._aulPoints[2] + 1
                        << "//" << it + 1 << '\n';
                    seq.next(true);  // allow to cancel
                }
            }
        }
    }

    return true;
}

bool WriterOBJ::SaveMaterial(std::ostream& out)
{
    if (!out || out.bad()) {
        return false;
    }

    if (_material) {
        if (_material->binding == MeshIO::PER_FACE) {

            std::vector<App::Color> Kd = _material->diffuseColor;
            std::sort(Kd.begin(), Kd.end(), Color_Less());
            Kd.erase(std::unique(Kd.begin(), Kd.end()), Kd.end());

            out.precision(6);
            out.setf(std::ios::fixed | std::ios::showpoint);
            out << "# Created by FreeCAD <http://www.freecad.org>: 'None'\n";
            out << "# Material Count: " << Kd.size() << '\n';

            for (std::size_t i = 0; i < Kd.size(); i++) {
                out << '\n';
                out << "newmtl material_" << i << '\n';
                out << "    Ka 0.200000 0.200000 0.200000\n";
                out << "    Kd " << Kd[i].r << " " << Kd[i].g << " " << Kd[i].b << '\n';
                out << "    Ks 1.000000 1.000000 1.000000\n";
                out << "    d 1.000000" << '\n';
                out << "    illum 2" << '\n';
                out << "    Ns 0.000000" << '\n';
            }

            return true;
        }
    }

    return false;
}
