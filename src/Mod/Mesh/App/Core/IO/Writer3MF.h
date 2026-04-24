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

#include <Mod/Mesh/MeshGlobal.h>
#include <iosfwd>
#include <zipios++/zipoutputstream.h>

namespace Base
{
class Matrix4D;
}

namespace MeshCore
{
class MeshKernel;

struct Resource3MF
{
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
    explicit Writer3MF(std::ostream& str);

    /*!
     * \brief Writer3MF
     * Passes a file name to the constructor
     * \param filename
     */
    explicit Writer3MF(const std::string& filename);
    /*!
     * \brief SetForceModel
     * Forcces to write the mesh as model even if itsn't a solid.
     * \param model
     */
    void SetForceModel(bool model);
    /*!
     * \brief Add a mesh object resource to the 3MF file.
     * \param mesh The mesh object to be written
     * \param mat The placement of the mesh object
     * \param name The name of the mesh object which can later be displayed by downstream applications
     * \return true if the added mesh could be written successfully, false otherwise.
     */
    bool AddMesh(const MeshKernel& mesh, const Base::Matrix4D& mat, const std::string& name = "");
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
    static void Initialize(std::ostream& str);
    void Finish(std::ostream& str);
    std::string GetType(const MeshKernel& mesh) const;
    void SaveBuildItem(int id, const Base::Matrix4D& mat);
    static std::string DumpMatrix(const Base::Matrix4D& mat);
    bool SaveObject(std::ostream& str, int id, const MeshKernel& mesh, const std::string& name) const;
    bool SaveRels(std::ostream& str) const;
    bool SaveContent(std::ostream& str) const;

private:
    zipios::ZipOutputStream zip;
    int objectIndex = 0;
    std::vector<std::string> items;
    std::vector<Resource3MF> resources;
    bool forceModel = true;
};

}  // namespace MeshCore
