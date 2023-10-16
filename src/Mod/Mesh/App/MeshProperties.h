/***************************************************************************
 *   Copyright (c) Jürgen Riegel <juergen.riegel@web.de>                   *
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

#ifndef MESH_MESHPROPERTIES_H
#define MESH_MESHPROPERTIES_H

#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <Base/Handle.h>
#include <Base/Matrix.h>

#include <Mod/Mesh/App/Core/MeshIO.h>
#include <Mod/Mesh/App/Core/MeshKernel.h>

#include "Mesh.h"


namespace Mesh
{

class MeshPy;

/** The normals property class.
 * Note: We need an own class for that to distinguish from the base vector list.
 * @author Werner Mayer
 */
class MeshExport PropertyNormalList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyNormalList();

    void setSize(int newSize) override;
    int getSize() const override;

    void setValue(const Base::Vector3f&);
    void setValue(float x, float y, float z);

    const Base::Vector3f& operator[](const int idx) const
    {
        return _lValueList[idx];
    }

    void set1Value(const int idx, const Base::Vector3f& value)
    {
        _lValueList[idx] = value;
    }

    void setValues(const std::vector<Base::Vector3f>& values);

    const std::vector<Base::Vector3f>& getValues() const
    {
        return _lValueList;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    App::Property* Copy() const override;
    void Paste(const App::Property& from) override;

    unsigned int getMemSize() const override;

    void transformGeometry(const Base::Matrix4D& rclMat);

private:
    std::vector<Base::Vector3f> _lValueList;
};

/** Curvature information. */
struct MeshExport CurvatureInfo
{
    float fMaxCurvature {0.0F};
    float fMinCurvature {0.0F};
    Base::Vector3f cMaxCurvDir;
    Base::Vector3f cMinCurvDir;
};

/** The Curvature property class.
 * @author Werner Mayer
 */
class MeshExport PropertyCurvatureList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum
    {
        MeanCurvature = 0,  /**< Mean curvature */
        GaussCurvature = 1, /**< Gaussian curvature */
        MaxCurvature = 2,   /**< Maximum curvature */
        MinCurvature = 3,   /**< Minimum curvature */
        AbsCurvature = 4    /**< Absolute curvature */
    };

public:
    PropertyCurvatureList();

    void setSize(int newSize) override
    {
        _lValueList.resize(newSize);
    }
    int getSize() const override
    {
        return _lValueList.size();
    }
    std::vector<float> getCurvature(int tMode) const;
    void setValue(const CurvatureInfo&);
    void setValues(const std::vector<CurvatureInfo>&);

    /// index operator
    const CurvatureInfo& operator[](const int idx) const
    {
        return _lValueList[idx];
    }
    void set1Value(const int idx, const CurvatureInfo& value)
    {
        _lValueList[idx] = value;
    }
    const std::vector<CurvatureInfo>& getValues() const
    {
        return _lValueList;
    }
    void transformGeometry(const Base::Matrix4D& rclMat);

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    /** @name Python interface */
    //@{
    PyObject* getPyObject() override;
    void setPyObject(PyObject* value) override;
    //@}

    App::Property* Copy() const override;
    void Paste(const App::Property& from) override;

    unsigned int getMemSize() const override
    {
        return _lValueList.size() * sizeof(CurvatureInfo);
    }

private:
    std::vector<CurvatureInfo> _lValueList;
};

/** Mesh material properties
 */
class MeshExport PropertyMaterial: public App::Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyMaterial() = default;

    /** Sets the property
     */
    void setValue(const MeshCore::Material& mat);
    void setAmbientColor(const std::vector<App::Color>& col);
    void setDiffuseColor(const std::vector<App::Color>& col);
    void setSpecularColor(const std::vector<App::Color>& col);
    void setEmissiveColor(const std::vector<App::Color>& col);
    void setShininess(const std::vector<float>&);
    void setTransparency(const std::vector<float>&);
    void setBinding(MeshCore::MeshIO::Binding);

    const MeshCore::Material& getValue() const;
    const std::vector<App::Color>& getAmbientColor() const;
    const std::vector<App::Color>& getDiffuseColor() const;
    const std::vector<App::Color>& getSpecularColor() const;
    const std::vector<App::Color>& getEmissiveColor() const;
    const std::vector<float>& getShininess() const;
    const std::vector<float>& getTransparency() const;
    MeshCore::MeshIO::Binding getBinding() const;

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    const char* getEditorName() const override;

    Property* Copy() const override;
    void Paste(const Property& from) override;

    unsigned int getMemSize() const override;
    bool isSame(const Property& other) const override;

private:
    MeshCore::Material _material;
};

/** The mesh kernel property class.
 * @author Werner Mayer
 */
class MeshExport PropertyMeshKernel: public App::PropertyComplexGeoData
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyMeshKernel();
    ~PropertyMeshKernel() override;

    PropertyMeshKernel(const PropertyMeshKernel&) = delete;
    PropertyMeshKernel(PropertyMeshKernel&&) = delete;
    PropertyMeshKernel& operator=(const PropertyMeshKernel&) = delete;
    PropertyMeshKernel& operator=(PropertyMeshKernel&&) = delete;

    /** @name Getter/setter */
    //@{
    /** This method references the passed mesh object and takes possession of it,
     * it does NOT create a copy.
     * The currently referenced mesh object gets de-referenced and possibly deleted
     * if its reference counter becomes zero.
     * However, the mesh gets saved before if a transaction is open at this time.
     * @note Make sure not to reference the internal mesh pointer pf this class in
     * client code. This could lead to crashes if not handled properly.
     */
    void setValuePtr(MeshObject* m);
    /** This method sets the mesh by copying the data. */
    void setValue(const MeshObject& m);
    /** This method sets the mesh by copying the data. */
    void setValue(const MeshCore::MeshKernel& m);
    /** Swaps the mesh data structure. */
    void swapMesh(MeshObject&);
    /** Swaps the mesh data structure. */
    void swapMesh(MeshCore::MeshKernel&);
    /** Returns a the attached mesh object by reference. It cannot be modified
     * from outside.
     */
    const MeshObject& getValue() const;
    const MeshObject* getValuePtr() const;
    unsigned int getMemSize() const override;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    const Data::ComplexGeoData* getComplexData() const override;
    /** Returns the bounding box around the underlying mesh kernel */
    Base::BoundBox3d getBoundingBox() const override;
    //@}

    /** @name Modification */
    //@{
    MeshObject* startEditing();
    void finishEditing();
    /// Transform the real mesh data
    void transformGeometry(const Base::Matrix4D& rclMat) override;
    void setPointIndices(const std::vector<std::pair<PointIndex, Base::Vector3f>>&);
    void setTransform(const Base::Matrix4D& rclTrf) override;
    Base::Matrix4D getTransform() const override;
    //@}

    /** @name Python interface */
    //@{
    /** Returns a Python wrapper for the referenced mesh object. It does NOT
     * create a copy. However, the Python wrapper is marked as \a immutable so
     * that the mesh object cannot be modified from outside.
     */
    PyObject* getPyObject() override;
    /** This method copies the content, hence creates an new mesh object
     * to copy the data. The passed argument can be an instance of the Python
     * wrapper for the mesh object or simply a list of triangles, i.e. a list
     * of lists of three floats.
     */
    void setPyObject(PyObject* value) override;
    //@}

    const char* getEditorName() const override
    {
        return "MeshGui::PropertyMeshKernelItem";
    }

    /** @name Save/restore */
    //@{
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    App::Property* Copy() const override;
    void Paste(const App::Property& from) override;
    //@}

private:
    Base::Reference<MeshObject> _meshObject;
    MeshPy* meshPyObject {nullptr};
};

}  // namespace Mesh

#endif  // MESH_MESHPROPERTIES_H
