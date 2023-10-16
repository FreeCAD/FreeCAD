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


#ifndef MESH_IO_WRITER_OBJ_H
#define MESH_IO_WRITER_OBJ_H

#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/MeshGlobal.h>

namespace MeshCore
{

/** Saves the mesh object into OBJ format. */
class MeshExport WriterOBJ
{
public:
    /*!
     * \brief WriterOBJ
     */
    explicit WriterOBJ(const MeshKernel& kernel, const Material*);
    /*!
     * \brief Set the mesh groups for the OBJ export.
     * A mesh group is a list of facet indices, i.e. a mesh segment.
     * This function must be called before calling \ref Save()
     * \param g
     */
    void SetGroups(const std::vector<Group>& g);
    /*!
     * \brief Apply a transformation for the exported mesh.
     */
    void SetTransform(const Base::Matrix4D&);
    /*!
     * \brief Save the mesh to an OBJ file.
     * \return true if the data could be written successfully, false otherwise.
     */
    bool Save(std::ostream&);
    /*!
     * \brief Save material file.
     * \return true on success and false otherwise.
     */
    bool SaveMaterial(std::ostream&);

private:
    struct Color_Less;

    const MeshKernel& _kernel;
    const Material* _material;
    Base::Matrix4D _transform;
    bool apply_transform {false};
    std::vector<Group> _groups;
};

}  // namespace MeshCore


#endif  // MESH_IO_WRITER_OBJ_H
