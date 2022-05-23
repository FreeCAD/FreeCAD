/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
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


#ifndef MESH_IO_H
#define MESH_IO_H

#include "MeshKernel.h"
#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <App/Material.h>

namespace Base {
class XMLReader;
class Writer;
}

namespace MeshCore {

class MeshKernel;

namespace MeshIO {
    enum Format {
        Undefined,
        BMS,
        ASTL,
        BSTL,
        STL,
        OBJ,
        OFF,
        IDTF,
        MGL,
        IV,
        X3D,
        X3DZ,
        X3DOM,
        VRML,
        WRZ,
        NAS,
        PLY,
        APLY,
        PY,
        AMF,
        SMF,
        ASY,
        ThreeMF
    };
    enum Binding {
        OVERALL,
        PER_VERTEX,
        PER_FACE
    };
}

struct MeshExport Material
{
    Material() : binding(MeshIO::OVERALL) {}
    MeshIO::Binding binding;
    mutable std::string library;
    std::vector<App::Color> diffuseColor;
};

struct MeshExport Group
{
    std::vector<FacetIndex> indices;
    std::string name;
};

/**
 * The MeshInput class is able to read a mesh object from an input stream
 * in various formats.
 */
class MeshExport MeshInput
{
public:
    MeshInput (MeshKernel &rclM)
        : _rclMesh(rclM), _material(nullptr){}
    MeshInput (MeshKernel &rclM, Material* m)
        : _rclMesh(rclM), _material(m){}
    virtual ~MeshInput () { }
    const std::vector<std::string>& GetGroupNames() const {
        return _groupNames;
    }

    /// Loads the file, decided by extension
    bool LoadAny(const char* FileName);
    /// Loads from a stream and the given format
    bool LoadFormat(std::istream &str, MeshIO::Format fmt);
    /** Loads an STL file either in binary or ASCII format. 
     * Therefore the file header gets checked to decide if the file is binary or not.
     */
    bool LoadSTL (std::istream &rstrIn);
    /** Loads an ASCII STL file. */
    bool LoadAsciiSTL (std::istream &rstrIn);
    /** Loads a binary STL file. */
    bool LoadBinarySTL (std::istream &rstrIn);
    /** Loads an OBJ Mesh file. */
    bool LoadOBJ (std::istream &rstrIn);
    /** Loads the materials of an OBJ file. */
    bool LoadMTL (std::istream &rstrIn);
    /** Loads an SMF Mesh file. */
    bool LoadSMF (std::istream &rstrIn);
    /** Loads an OFF Mesh file. */
    bool LoadOFF (std::istream &rstrIn);
    /** Loads a PLY Mesh file. */
    bool LoadPLY (std::istream &rstrIn);
    /** Loads the mesh object from an XML file. */
    void LoadXML (Base::XMLReader &reader);
    /** Loads a node from an OpenInventor file. */
    bool LoadMeshNode (std::istream &rstrIn);
    /** Loads an OpenInventor file. */
    bool LoadInventor (std::istream &rstrIn);
    /** Loads a Nastran file. */
    bool LoadNastran (std::istream &rstrIn);
    /** Loads a Cadmould FE file. */
    bool LoadCadmouldFE (std::ifstream &rstrIn);

    static std::vector<std::string> supportedMeshFormats();
    static MeshIO::Format getFormat(const char* FileName);

protected:
    MeshKernel &_rclMesh;   /**< reference to mesh data structure */
    Material* _material;
    std::vector<std::string> _groupNames;
    std::vector<std::pair<std::string, unsigned long> > _materialNames;
};

/**
 * The MeshOutput class is able to write a mesh object to an output stream
 * on various formats.
 */
class MeshExport MeshOutput
{
public:
    MeshOutput (const MeshKernel &rclM)
        : _rclMesh(rclM), _material(nullptr), apply_transform(false){}
    MeshOutput (const MeshKernel &rclM, const Material* m)
        : _rclMesh(rclM), _material(m), apply_transform(false){}
    virtual ~MeshOutput () { }
    void SetObjectName(const std::string& n)
    { objectName = n; }
    void SetGroups(const std::vector<Group>& g) {
        _groups = g;
    }

    void Transform(const Base::Matrix4D&);
    /** Set custom data to the header of a binary STL.
     * If the data exceeds 80 characters then the characters too much
     * are ignored. If the data has less than 80 characters they are
     * automatically filled up with spaces.
     */
    static void SetSTLHeaderData(const std::string&);
    /**
     * Change the image size of the asymptote output.
     */
    static void SetAsymptoteSize(const std::string&, const std::string&);
    /// Determine the mesh format by file extension
    static MeshIO::Format GetFormat(const char* FileName);
    /// Saves the file, decided by extension if not explicitly given
    bool SaveAny(const char* FileName, MeshIO::Format f=MeshIO::Undefined) const;
    /// Saves to a stream and the given format
    bool SaveFormat(std::ostream &str, MeshIO::Format fmt) const;

    /** Saves the mesh object into an ASCII STL file. */
    bool SaveAsciiSTL (std::ostream &rstrOut) const;
    /** Saves the mesh object into a binary STL file. */
    bool SaveBinarySTL (std::ostream &rstrOut) const;
    /** Saves the mesh object into an OBJ file. */
    bool SaveOBJ (std::ostream &rstrOut) const;
    /** Saves the materials of an OBJ file. */
    bool SaveMTL(std::ostream &rstrOut) const;
    /** Saves the mesh object into an SMF file. */
    bool SaveSMF (std::ostream &rstrOut) const;
    /** Saves the mesh object into an OFF file. */
    bool SaveOFF (std::ostream &rstrOut) const;
    /** Saves the mesh object into a binary PLY file. */
    bool SaveBinaryPLY (std::ostream &rstrOut) const;
    /** Saves the mesh object into an ASCII PLY file. */
    bool SaveAsciiPLY (std::ostream &rstrOut) const;
    /** Saves the mesh object into an asymptote file. */
    bool SaveAsymptote (std::ostream &rstrOut) const;
    /** Saves the mesh object into an XML file. */
    void SaveXML (Base::Writer &writer) const;
    /** Saves the mesh object into a 3MF file. */
    bool Save3MF (std::ostream &str) const;
    /** Saves a node to an OpenInventor file. */
    bool SaveMeshNode (std::ostream &rstrIn);
    /** Writes an IDTF file. */
    bool SaveIDTF (std::ostream &rstrOut) const;
    /** Writes an MGL file. */
    bool SaveMGL (std::ostream &rstrOut) const;
    /** Writes an OpenInventor file. */
    bool SaveInventor (std::ostream &rstrOut) const;
    /** Writes an X3D file. */
    bool SaveX3D (std::ostream &rstrOut) const;
    /** Writes an X3dom file. */
    bool SaveX3DOM (std::ostream &rstrOut) const;
    /** Writes a VRML file. */
    bool SaveVRML (std::ostream &rstrOut) const;
    /** Writes a Nastran file. */
    bool SaveNastran (std::ostream &rstrOut) const;
    /** Writes a Cadmould FE file. */
    bool SaveCadmouldFE (std::ostream &rstrOut) const;
    /** Writes a python module which creates a mesh */
    bool SavePython (std::ostream &rstrOut) const;

    static std::vector<std::string> supportedMeshFormats();

protected:
    /** Writes an X3D file. */
    bool SaveX3DContent (std::ostream &rstrOut, bool exportViewpoints) const;
    bool Save3MFModel(std::ostream &str) const;
    bool Save3MFRels(std::ostream &str) const;
    bool Save3MFContent(std::ostream &str) const;

protected:
    const MeshKernel &_rclMesh;   /**< reference to mesh data structure */
    const Material* _material;
    Base::Matrix4D _transform;
    bool apply_transform;
    std::string objectName;
    std::vector<Group> _groups;
    static std::string stl_header;
    static std::string asyWidth;
    static std::string asyHeight;
};

/*!
  The MeshCleanup class is a helper class to remove points from the point array that are not
  referenced by any facet. It also removes facet with point indices that are out of range.
 */
class MeshCleanup
{
public:
    /*!
      \brief Construction.
      \param p -- the point array
      \param f -- the facet array
     */
    MeshCleanup(MeshPointArray& p, MeshFacetArray& f);
    ~MeshCleanup();

    /*!
      \brief Set the material array.
      In case the material array sets the colors per vertex and
      \ref RemoveInvalids() removes points from the array the
      material array will be adjusted.
     */
    void SetMaterial(Material* mat);

    /*!
      \brief Remove unreferenced and invalid facets.
     */
    void RemoveInvalids();

private:
    /*!
      \brief Remove invalid facets.
     */
    void RemoveInvalidFacets();
    /*!
      \brief Remove invalid points.
     */
    void RemoveInvalidPoints();

private:
    MeshPointArray& pointArray;
    MeshFacetArray& facetArray;
    Material* materialArray;
};

/*!
   The MeshPointFacetAdjacency class is a helper class to get all facets per vertex
   and set the neighbourhood of the facets.
   At this point the MeshFacetArray only references the points but does not have set
   the neighbourhood of two adjacent facets.
 */
class MeshPointFacetAdjacency
{
public:
    /*!
      \brief Construction.
      \param p -- the number of points
      \param f -- the facet array
     */
    MeshPointFacetAdjacency(std::size_t p, MeshFacetArray& f);
    ~MeshPointFacetAdjacency();

    /*!
      \brief Set the neighbourhood of two adjacent facets.
     */
    void SetFacetNeighbourhood();

private:
    /*!
      \brief Build up the adjacency information.
     */
    void Build();

private:
    std::size_t numPoints;
    MeshFacetArray& facets;
    std::vector< std::vector<std::size_t> > pointFacetAdjacency;
};


} // namespace MeshCore

#endif // MESH_IO_H
