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
        OBJ,
        OFF,
        IV,
        X3D,
        VRML,
        WRZ,
        NAS,
        PLY,
        APLY,
        PY
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
    std::vector<App::Color> diffuseColor;
};

/**
 * The MeshInput class is able to read a mesh object from a input stream
 * in various formats.
 */
class MeshExport MeshInput
{
public:
    MeshInput (MeshKernel &rclM)
        : _rclMesh(rclM), _material(0){}
    MeshInput (MeshKernel &rclM, Material* m)
        : _rclMesh(rclM), _material(m){}
    virtual ~MeshInput (void) { }

    /// Loads the file, decided by extension
    bool LoadAny(const char* FileName);
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

protected:
    MeshKernel &_rclMesh;   /**< reference to mesh data structure */
    Material* _material;
};

/**
 * The MeshOutput class is able to write a mesh object to an ouput stream
 * on various formats.
 */
class MeshExport MeshOutput
{
public:
    MeshOutput (const MeshKernel &rclM)
        : _rclMesh(rclM), _material(0), apply_transform(false){}
    MeshOutput (const MeshKernel &rclM, const Material* m)
        : _rclMesh(rclM), _material(m), apply_transform(false){}
    virtual ~MeshOutput (void) { }
    void Transform(const Base::Matrix4D&);
    /** Set custom data to the header of a binary STL.
     * If the data exceeds 80 characters the the characters too much
     * are ignored. If the data has less than 80 characters they are
     * automatically filled up with spaces.
     */
    static void SetSTLHeaderData(const std::string&);
    /// Saves the file, decided by extension if not explicitly given
    bool SaveAny(const char* FileName, MeshIO::Format f=MeshIO::Undefined) const;

    /** Saves the mesh object into an ASCII STL file. */
    bool SaveAsciiSTL (std::ostream &rstrOut) const;
    /** Saves the mesh object into a binary STL file. */
    bool SaveBinarySTL (std::ostream &rstrOut) const;
    /** Saves the mesh object into an OBJ file. */
    bool SaveOBJ (std::ostream &rstrOut) const;
    /** Saves the mesh object into an OFF file. */
    bool SaveOFF (std::ostream &rstrOut) const;
    /** Saves the mesh object into a binary PLY file. */
    bool SaveBinaryPLY (std::ostream &rstrOut) const;
    /** Saves the mesh object into an ASCII PLY file. */
    bool SaveAsciiPLY (std::ostream &rstrOut) const;
    /** Saves the mesh object into an XML file. */
    void SaveXML (Base::Writer &writer) const;
    /** Saves a node to an OpenInventor file. */
    bool SaveMeshNode (std::ostream &rstrIn);
    /** Writes an OpenInventor file. */
    bool SaveInventor (std::ostream &rstrOut) const;
    /** Writes an X3D file. */
    bool SaveX3D (std::ostream &rstrOut) const;
    /** Writes a VRML file. */
    bool SaveVRML (std::ostream &rstrOut, const App::Material &rclMat) const;
    /** Writes a Nastran file. */
    bool SaveNastran (std::ostream &rstrOut) const;
    /** Writes a Cadmould FE file. */
    bool SaveCadmouldFE (std::ostream &rstrOut) const;
    /** Writes a python module which creates a mesh */
    bool SavePython (std::ostream &rstrOut) const;

protected:
    const MeshKernel &_rclMesh;   /**< reference to mesh data structure */
    const Material* _material;
    Base::Matrix4D _transform;
    bool apply_transform;
    static std::string stl_header;
};

struct MeshExport VRMLViewpointData
{
    Base::Vector3f clVRefPln;
    Base::Vector3f clVRefUp;
    Base::Vector3f clVRefPt;
    Base::Vector3f clPRefPt;
    double    dVPlnDist;
    double    dUmin;
    double    dUmax;
    double    dVmin;
    double    dVmax;
    std::string  clName;
};

struct MeshExport VRMLInfo
{
    std::string _clFileName;
    std::string _clAuthor;
    std::string _clDate;
    std::string _clCompany;
    std::string _clAnnotation;
    std::string _clPicFileName;
    App::Color  _clColor;
    bool     _bSaveViewPoints;
    bool     _bSavePicture;
    std::vector<std::string> _clComments;
    std::vector<VRMLViewpointData> _clViewpoints;
};


} // namespace MeshCore

#endif // MESH_IO_H
