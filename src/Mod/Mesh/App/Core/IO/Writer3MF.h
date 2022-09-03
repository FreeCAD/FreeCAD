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
#include <zipios++/zipoutputstream.h>
#include <Mod/Mesh/MeshGlobal.h>

namespace Base {
class Matrix4D;
}

namespace MeshCore
{
class MeshKernel;

struct Resource3MF {
    std::string extension;
    std::string contentType;
    std::string relationshipTarget;
    std::string relationshipType;
    std::string fileNameInZip;
    std::string fileContent;
};

/** Saves the mesh object into 3MF format. */
class MeshExport Writer3MF
{
public:
    /*!
     * \brief Writer3MF
     * Passes an output stream to the constructor.
     * \param str
     */
    explicit Writer3MF(std::ostream &str);

    /*!
     * \brief Writer3MF
     * Passes a file name to the constructor
     * \param filename
     */
    explicit Writer3MF(const std::string &filename);

    /*!
     * \brief Add a mesh object resource to the 3MF file.
     * \param mesh The mesh object to be written
     * \param mat The placement of the mesh object
     * \return true if the added mesh could be written successfully, false otherwise.
     */
    bool AddMesh(const MeshKernel& mesh, const Base::Matrix4D& mat);
    /*!
     * \brief AddResource
     * Add an additional resource to the 3MF file.
     */
    void AddResource(const Resource3MF&);
    /*!
     * \brief After having added the mesh objects with \ref AddMesh save the meta-information
     * to the 3MF file.
     * \return true if the data could be written successfully, false otherwise.
     */
    bool Save();

private:
    void Initialize(std::ostream &str);
    void Finish(std::ostream &str);
    std::string GetType(const MeshKernel& mesh) const;
    void SaveBuildItem(int id, const Base::Matrix4D& mat);
    std::string DumpMatrix(const Base::Matrix4D& mat) const;
    bool SaveObject(std::ostream &str, int id, const MeshKernel& mesh) const;
    bool SaveRels(std::ostream &str) const;
    bool SaveContent(std::ostream &str) const;

private:
    zipios::ZipOutputStream zip;
    int objectIndex;
    std::vector<std::string> items;
    std::vector<Resource3MF> resources;
};

} // namespace MeshCore


#endif  // MESH_IO_WRITER_3MF_H
