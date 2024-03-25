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
#include <ostream>
#include <vector>

#include "MeshFeature.h"
#include "Core/MeshIO.h"
#include "Core/IO/Writer3MF.h"


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
class MeshExport Exporter
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
    int addObject(App::DocumentObject* obj, float tol);

    virtual bool addMesh(const char* name, const MeshObject& mesh) = 0;

    Exporter(const Exporter&) = delete;
    Exporter(Exporter&&) = delete;
    Exporter& operator=(const Exporter&) = delete;
    Exporter& operator=(Exporter&&) = delete;

protected:
    /// Does some simple escaping of characters for XML-type exports
    static std::string xmlEscape(const std::string& input);
    void throwIfNoPermission(const std::string&);

    std::map<const App::DocumentObject*, std::vector<std::string>> subObjectNameCache;
    std::map<const App::DocumentObject*, MeshObject> meshCache;
};

/// Creates a single mesh, in a file, from one or more objects
class MeshExport MergeExporter: public Exporter
{
public:
    MergeExporter(std::string fileName, MeshCore::MeshIO::Format fmt);
    ~MergeExporter() override;

    MergeExporter(const MergeExporter&) = delete;
    MergeExporter(MergeExporter&&) = delete;
    MergeExporter& operator=(const MergeExporter&) = delete;
    MergeExporter& operator=(MergeExporter&&) = delete;

    bool addMesh(const char* name, const MeshObject& mesh) override;

private:
    /// Write the meshes of the added objects to the output file
    void write();

protected:
    // NOLINTBEGIN
    MeshObject mergingMesh;
    std::string fName;
    // NOLINTEND
};

// ------------------------------------------------------------------------------------------------

/*!
 * \brief The AbstractExtension class
 * Abstract base class for file format extensions
 */
class MeshExport AbstractFormatExtension
{
protected:
    AbstractFormatExtension() = default;

public:
    virtual ~AbstractFormatExtension() = default;

    AbstractFormatExtension(const AbstractFormatExtension&) = delete;
    AbstractFormatExtension(AbstractFormatExtension&&) = delete;
    AbstractFormatExtension& operator=(const AbstractFormatExtension&) = delete;
    AbstractFormatExtension& operator=(AbstractFormatExtension&&) = delete;
};

using AbstractFormatExtensionPtr = std::shared_ptr<AbstractFormatExtension>;

/*!
 * \brief The Extension3MF class
 * Abstract base class for 3MF extensions
 */
class MeshExport Extension3MF: public AbstractFormatExtension
{
public:
    using Resource = MeshCore::Resource3MF;
    Extension3MF() = default;
    virtual Resource addMesh(const MeshObject& mesh) = 0;
};

using Extension3MFPtr = std::shared_ptr<Extension3MF>;

/*!
 * \brief The AbstractExtensionProducer class
 * Abstract base class to create an instance of an AbstractFormatExtension.
 */
class MeshExport AbstractExtensionProducer
{
public:
    AbstractExtensionProducer() = default;
    virtual ~AbstractExtensionProducer() = default;
    virtual AbstractFormatExtensionPtr create() const = 0;

    AbstractExtensionProducer(const AbstractExtensionProducer&) = delete;
    AbstractExtensionProducer(AbstractExtensionProducer&&) = delete;
    AbstractExtensionProducer& operator=(const AbstractExtensionProducer&) = delete;
    AbstractExtensionProducer& operator=(AbstractExtensionProducer&&) = delete;
};

using AbstractExtensionProducerPtr = std::shared_ptr<AbstractExtensionProducer>;

/*!
 * \brief The Extension3MFProducer class
 * Abstract base class to create an instance of an Extension3MF.
 */
class MeshExport Extension3MFProducer: public AbstractExtensionProducer
{
public:
    Extension3MFProducer() = default;
    virtual void initialize() = 0;
};

using Extension3MFProducerPtr = std::shared_ptr<Extension3MFProducer>;

/*!
 * \brief The GuiExtension3MFProducer class
 * This class tries to load the MeshGui module to register further 3MF extensions.
 */
class MeshExport GuiExtension3MFProducer: public Extension3MFProducer
{
public:
    GuiExtension3MFProducer() = default;
    void initialize() override;
    AbstractFormatExtensionPtr create() const override;
};

/*!
 * \brief The Extension3MFFactory class
 * Factor class to manage the producers of Extension3MF
 */
class MeshExport Extension3MFFactory
{
public:
    static void addProducer(Extension3MFProducer* ext);
    static void initialize();
    static std::vector<Extension3MFPtr> createExtensions();

private:
    static std::vector<Extension3MFProducerPtr> producer;
};

// ------------------------------------------------------------------------------------------------

/// Used for exporting to 3D Manufacturing Format (3MF)
/*!
 * The constructor and destructor write the beginning and end of the 3MF,
 * addObject() is used to add geometry
 */
class MeshExport Exporter3MF: public Exporter
{
public:
    Exporter3MF(std::string fileName, const std::vector<Extension3MFPtr>& = {});
    ~Exporter3MF() override;

    Exporter3MF(const Exporter3MF&) = delete;
    Exporter3MF(Exporter3MF&&) = delete;
    Exporter3MF& operator=(const Exporter3MF&) = delete;
    Exporter3MF& operator=(Exporter3MF&&) = delete;

    bool addMesh(const char* name, const MeshObject& mesh) override;
    /*!
     * \brief SetForceModel
     * Forcces to write the mesh as model even if itsn't a solid.
     * \param model
     */
    void setForceModel(bool model);

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
class MeshExport ExporterAMF: public Exporter
{
public:
    /// Writes AMF header
    /*!
     * meta information passed in is applied at the <amf> tag level
     */
    ExporterAMF(std::string fileName,
                const std::map<std::string, std::string>& meta,
                bool compress = true);

    /// Writes AMF footer
    ~ExporterAMF() override;

    ExporterAMF(const ExporterAMF&) = delete;
    ExporterAMF(ExporterAMF&&) = delete;
    ExporterAMF& operator=(const ExporterAMF&) = delete;
    ExporterAMF& operator=(ExporterAMF&&) = delete;

    bool addMesh(const char* name, const MeshObject& mesh) override;

private:
    /// Write the meshes of the added objects to the output file
    void write();

private:
    std::ostream* outputStreamPtr {nullptr};
    int nextObjectIndex {0};

    /// Helper for putting Base::Vector3f objects into a std::map in addMesh()
    class VertLess;
};  // class ExporterAMF

}  // namespace Mesh

#endif  // MESH_EXPORTER_H
