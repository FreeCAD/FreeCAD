// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include "Edge.h"


namespace Mesh
{
// forward declaration
class MeshObject;

/** The Facet helper class
 * The MeshFacet class provides an interface for the MeshFacetPy class for
 * convenient access to the Mesh data structure. This class should not be used
 * for programming algorithms in C++. Use Mesh Core classes instead!
 */
class MeshExport Facet: public MeshCore::MeshGeomFacet
{
public:
    explicit Facet(
        const MeshCore::MeshFacet& face = MeshCore::MeshFacet(),
        const MeshObject* obj = nullptr,
        MeshCore::FacetIndex index = MeshCore::FACET_INDEX_MAX
    );
    Facet(const Facet& f);
    Facet(Facet&& f);
    ~Facet();

    bool isBound() const
    {
        return Index != MeshCore::FACET_INDEX_MAX;
    }
    Facet& operator=(const Facet& f);
    Facet& operator=(Facet&& f);
    Edge getEdge(int) const;

    MeshCore::FacetIndex Index;
    MeshCore::PointIndex PIndex[3];
    MeshCore::FacetIndex NIndex[3];
    Base::Reference<const MeshObject> Mesh;
};

}  // namespace Mesh
