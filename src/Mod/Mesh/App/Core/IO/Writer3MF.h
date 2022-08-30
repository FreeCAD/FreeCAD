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


#ifndef MESH_IO_WRITER_3MF_H
#define MESH_IO_WRITER_3MF_H

#include <iosfwd>

#include "Core/Elements.h"

namespace MeshCore
{
class MeshKernel;

/** Saves the mesh object into 3MF format. */
class MeshExport Writer3MF
{
public:
    Writer3MF(const MeshKernel &mesh):
        kernel(mesh) {}

    void SetTransform(const Base::Matrix4D&);
    bool Save(std::ostream &str) const;

private:
    bool SaveModel(std::ostream &str) const;
    bool SaveRels(std::ostream &str) const;
    bool SaveContent(std::ostream &str) const;

private:
    bool applyTransform = false;
    Base::Matrix4D transform;
    const MeshKernel &kernel;
};

} // namespace MeshCore


#endif  // MESH_IO_WRITER_3MF_H
