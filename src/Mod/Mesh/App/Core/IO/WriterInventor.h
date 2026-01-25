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


#pragma once

#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/MeshGlobal.h>

namespace MeshCore
{

/** Saves the mesh object into OpenInventor v2.1 format. */
class MeshExport WriterInventor
{
public:
    /*!
     * \brief WriterInventor
     */
    explicit WriterInventor(const MeshKernel& kernel, const Material*);
    /*!
     * \brief Apply a transformation for the exported mesh.
     */
    void SetTransform(const Base::Matrix4D&);
    /*!
     * \brief Save the mesh to an OpenInventor file.
     * \return true if the data could be written successfully, false otherwise.
     */
    bool Save(std::ostream&);

private:
    const MeshKernel& _kernel;
    const Material* _material;
    Base::Matrix4D _transform;
    bool apply_transform {false};
};

}  // namespace MeshCore
