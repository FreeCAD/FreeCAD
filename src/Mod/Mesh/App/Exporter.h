/***************************************************************************
 *   Copyright (c) 2017 Ian Rees <ian.rees@gmail.com>                      *
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

#ifndef MESH_EXPORTER_H
#define MESH_EXPORTER_H

#include <map>
#include <vector>
#include <ostream>

#include <Base/Stream.h>
#include <Base/Type.h>

#include <App/Property.h>

#include "MeshFeature.h"
#include "Core/MeshIO.h"
#include "Core/MeshKernel.h"

namespace Mesh
{

/// Virtual base class for exporting meshes
/*!
 * Constructors of derived classes are expected to be required, for passing
 * in the name of output file.
 *
 * If objects are meant to be combined into a single file, then the file should
 * be saved from the derived class' destructor.
 */
class Exporter
{
public:
    Exporter();
    virtual ~Exporter() = default;

    /// Add object and all subobjects and links etc. Returns the number of stuff added.
    /*!
     * @param obj The object to export. If this is a group like object, its
     *            sub-objects will be added.
     * @param tol The tolerance/accuracy with which to generate the triangle mesh
     * @return The number of objects/subobjects that was exported from the document.
               See the parameter `accuracy` of ComplexGeoData::getFaces
     */
    int addObject(App::DocumentObject *obj, float tol);

    virtual bool addMesh(const char *name, const MeshObject & mesh) = 0;

protected:
    /// Does some simple escaping of characters for XML-type exports
    static std::string xmlEscape(const std::string &input);
    void throwIfNoPermission(const std::string&);

    std::map<const App::DocumentObject *, std::vector<std::string> > subObjectNameCache;
    std::map<const App::DocumentObject *, MeshObject> meshCache;
};

/// Creates a single mesh, in a file, from one or more objects
class MergeExporter : public Exporter
{
public:
    MergeExporter(std::string fileName, MeshCore::MeshIO::Format fmt);
    ~MergeExporter() override;

    bool addMesh(const char *name, const MeshObject & mesh) override;

private:
    /// Write the meshes of the added objects to the output file
    void write();

protected:
    MeshObject mergingMesh;
    std::string fName;
};

/// Used for exporting to 3D Manufacturing Format (3MF)
/*!
 * The constructor and destructor write the beginning and end of the 3MF,
 * addObject() is used to add geometry
 */
class Exporter3MF : public Exporter
{
public:
    Exporter3MF(std::string fileName);
    ~Exporter3MF() override;

    bool addMesh(const char *name, const MeshObject & mesh) override;

private:
    /// Write the meshes of the added objects to the output file
    void write();

private:
    class Private;
    std::unique_ptr<Private> d;
};

/// Used for exporting to Additive Manufacturing File (AMF) format
/*!
 * The constructor and destructor write the beginning and end of the AMF,
 * addObject() is used to add geometry
 */
class ExporterAMF : public Exporter
{
public:
    /// Writes AMF header
    /*!
     * meta information passed in is applied at the <amf> tag level
     */
    ExporterAMF(std::string fileName,
                const std::map<std::string, std::string> &meta,
                bool compress = true);

    /// Writes AMF footer
    ~ExporterAMF() override;

    bool addMesh(const char *name, const MeshObject & mesh) override;

private:
    /// Write the meshes of the added objects to the output file
    void write();

private:
    std::ostream *outputStreamPtr;
    int nextObjectIndex;

    /// Helper for putting Base::Vector3f objects into a std::map in addMesh()
    class VertLess;
};  // class ExporterAMF

} // namespace Mesh

#endif // MESH_EXPORTER_H
