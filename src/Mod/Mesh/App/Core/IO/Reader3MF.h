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


#ifndef MESH_IO_READER_3MF_H
#define MESH_IO_READER_3MF_H

#include <Mod/Mesh/App/Core/MeshKernel.h>
#include <Mod/Mesh/MeshGlobal.h>
#include <iosfwd>
#include <memory>
#include <unordered_map>
#include <xercesc/util/XercesDefs.hpp>

XERCES_CPP_NAMESPACE_BEGIN
class DOMDocument;
class DOMNodeList;
XERCES_CPP_NAMESPACE_END

namespace MeshCore
{

/** Loads the mesh object from data in 3MF format. */
class MeshExport Reader3MF
{
public:
    /*!
     * \brief Reader3MF
     * \param str
     *
     * Passes an input stream to the constructor.
     */
    explicit Reader3MF(std::istream& str);

    /*!
     * \brief Reader3MF
     * \param filename
     *
     * Passes a file name to the constructor
     */
    explicit Reader3MF(const std::string& filename);
    /*!
     * \brief Load the mesh from the input stream or file
     * \return true on success and false otherwise
     */
    bool Load();
    std::vector<int> GetMeshIds() const;
    const MeshKernel& GetMesh(int id) const
    {
        return meshes.at(id).first;
    }
    const Base::Matrix4D& GetTransform(int id) const
    {
        return meshes.at(id).second;
    }

private:
    bool LoadModel(std::istream&);
    bool LoadModel(XERCES_CPP_NAMESPACE_QUALIFIER DOMDocument&);
    bool LoadResources(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList*);
    bool LoadBuild(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList*);
    bool LoadItems(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList*);
    bool LoadObjects(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList*);
    void LoadMesh(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList*, int id);
    void LoadVertices(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList*, MeshPointArray&);
    void LoadTriangles(XERCES_CPP_NAMESPACE_QUALIFIER DOMNodeList*, MeshFacetArray&);

private:
    using MeshKernelAndTransform = std::pair<MeshKernel, Base::Matrix4D>;
    std::unordered_map<int, MeshKernelAndTransform> meshes;
    std::unique_ptr<std::istream> zip;
};

}  // namespace MeshCore


#endif  // MESH_IO_READER_3MF_H
