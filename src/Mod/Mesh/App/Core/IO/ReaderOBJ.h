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


#ifndef MESH_IO_READER_OBJ_H
#define MESH_IO_READER_OBJ_H

#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/MeshGlobal.h>
#include <iosfwd>

namespace MeshCore
{

class MeshKernel;
struct Material;

/** Loads the mesh object from data in OBJ format. */
class MeshExport ReaderOBJ
{
public:
    /*!
     * \brief ReaderOBJ
     */
    explicit ReaderOBJ(MeshKernel& kernel, Material*);
    /*!
     * \brief Load the mesh from the input stream
     * \return true on success and false otherwise
     */
    bool Load(std::istream& str);
    /*!
     * \brief Load the material file to the corresponding OBJ file.
     * This function must be called after \ref Load().
     * \param str
     * \return  true on success and false otherwise
     */
    bool LoadMaterial(std::istream& str);

    const std::vector<std::string>& GetGroupNames() const
    {
        return _groupNames;
    }

private:
    MeshKernel& _kernel;
    Material* _material;
    std::vector<std::string> _groupNames;
    std::vector<std::pair<std::string, unsigned long>> _materialNames;
};

}  // namespace MeshCore


#endif  // MESH_IO_READER_OBJ_H
