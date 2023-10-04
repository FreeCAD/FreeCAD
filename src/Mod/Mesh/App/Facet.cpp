/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#ifndef _PreComp_
#include <sstream>
#endif

#include "Facet.h"
#include "Mesh.h"


using namespace Mesh;

Facet::Facet(const MeshCore::MeshFacet& face,  // NOLINT
             const MeshObject* obj,
             MeshCore::FacetIndex index)
    : Index(index)
    , Mesh(obj)
{
    for (int i = 0; i < 3; i++) {
        PIndex[i] = face._aulPoints[i];
        NIndex[i] = face._aulNeighbours[i];
    }
    if (Mesh.isValid() && index != MeshCore::FACET_INDEX_MAX) {
        for (int i = 0; i < 3; i++) {
            Base::Vector3d vert = Mesh->getPoint(PIndex[i]);
            _aclPoints[i].Set((float)vert.x, (float)vert.y, (float)vert.z);
        }
    }
}

Facet::Facet(const Facet& f)  // NOLINT
    : MeshCore::MeshGeomFacet(f)
    , Index(f.Index)
    , Mesh(f.Mesh)
{
    for (int i = 0; i < 3; i++) {
        PIndex[i] = f.PIndex[i];
        NIndex[i] = f.NIndex[i];
    }
}

Facet::Facet(Facet&& f)  // NOLINT
    : MeshCore::MeshGeomFacet(f)
    , Index(f.Index)
    , Mesh(f.Mesh)
{
    for (int i = 0; i < 3; i++) {
        PIndex[i] = f.PIndex[i];
        NIndex[i] = f.NIndex[i];
    }
}

Facet::~Facet() = default;

Facet& Facet::operator=(const Facet& f)
{
    MeshCore::MeshGeomFacet::operator=(f);
    Mesh = f.Mesh;
    Index = f.Index;
    for (int i = 0; i < 3; i++) {
        PIndex[i] = f.PIndex[i];
        NIndex[i] = f.NIndex[i];
    }

    return *this;
}

Facet& Facet::operator=(Facet&& f)
{
    MeshCore::MeshGeomFacet::operator=(f);
    Mesh = f.Mesh;
    Index = f.Index;
    for (int i = 0; i < 3; i++) {
        PIndex[i] = f.PIndex[i];
        NIndex[i] = f.NIndex[i];
    }

    return *this;
}

Edge Facet::getEdge(int index) const
{
    index = index % 3;
    Edge edge;
    // geometric coordinates
    edge._aclPoints[0] = this->_aclPoints[index];
    edge._aclPoints[1] = this->_aclPoints[(index + 1) % 3];

    // indices
    edge.Index = index;
    edge.PIndex[0] = this->PIndex[index];
    edge.PIndex[1] = this->PIndex[(index + 1) % 3];
    edge.NIndex[0] = this->Index;
    edge.NIndex[1] = this->NIndex[index];
    edge._bBorder = (this->NIndex[index] == MeshCore::FACET_INDEX_MAX);

    edge.Mesh = this->Mesh;
    return edge;
}
