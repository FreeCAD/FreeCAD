/***************************************************************************
 *   Copyright (c) 2017 Ian Rees                  <ian.rees@gmail.com>     *
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
#include <ostream>

#include "Base/Type.h"

#include "App/Property.h"

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
 * Objects are added using the addMeshFeat(), addPartFeat(), etc.
 *
 * If objects are meant to be combined into a single file, then the file should
 * be saved from the derived class' destructor.
 */
class Exporter
{
    public:
        Exporter();

        /*!
         * \return true if \a is an object that can be exported as mesh.
         */
        static bool isSupported(App::DocumentObject *obj);

        virtual bool addMeshFeat(App::DocumentObject *obj) = 0;
        virtual bool addPartFeat(App::DocumentObject *obj, float tol) = 0;

        /// Recursively adds objects from App::Part & App::DocumentObjectGroup
        /*!
         * \return true if all applicable objects within the group were
         * added successfully.
         */
        bool addAppGroup(App::DocumentObject *obj, float tol);

        bool addObject(App::DocumentObject *obj, float tol);

        virtual ~Exporter() = default;

    protected:
        /// Does some simple escaping of characters for XML-type exports
        static std::string xmlEscape(const std::string &input);

        const Base::Type meshFeatId;
        const Base::Type appPartId;
        const Base::Type groupExtensionId;
};

/// Creates a single mesh, in a file, from one or more objects
class MergeExporter : public Exporter
{
    public:
        MergeExporter(std::string fileName, MeshCore::MeshIO::Format fmt);
        ~MergeExporter();

        /// Directly adds a mesh feature
        bool addMeshFeat(App::DocumentObject *obj) override;

        /// Converts the a Part::Feature to a mesh, adds that mesh
        bool addPartFeat(App::DocumentObject *obj, float tol) override;

    protected:
        MeshObject mergingMesh;
        std::string fName;
};

/// Used for exporting to Additive Manufacturing File (AMF) format
/*!
 * The constructor and destructor write the beginning and end of the AMF,
 * add____() is used to add geometry
 */
class AmfExporter : public Exporter
{
    public:
        /// Writes AMF header
        /*!
         * meta information passed in is applied at the <amf> tag level
         */
        AmfExporter(std::string fileName,
                    const std::map<std::string, std::string> &meta,
                    bool compress = true);

        /// Writes AMF footer
        ~AmfExporter();

        bool addMeshFeat(App::DocumentObject *obj) override;

        bool addPartFeat(App::DocumentObject *obj, float tol) override;

        /*!
         * meta is included for the AMF object created
         */
        bool addMesh(const MeshCore::MeshKernel &kernel,
                     const std::map<std::string, std::string> &meta);
        
    private:
        std::ostream *outputStreamPtr;
        int nextObjectIndex;

    /// Helper for putting Base::Vector3f objects into a std::map in addMesh()
    class VertLess
    {
        public:
        bool operator()(const Base::Vector3f &a, const Base::Vector3f &b) const
        {
            if (a.x == b.x) {
                if (a.y == b.y) {
                    if (a.z == b.z) {
                        return false;
                    } else {
                        return a.z < b.z;
                    }
                } else {
                    return a.y < b.y;
                }
            } else {
                return a.x < b.x;
            }
        }
    };
};  // class AmfExporter

} // namespace Mesh
#endif // MESH_EXPORTER_H
