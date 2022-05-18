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

#include <vector>
#include <list>
#include <set>
#include <string>
#include <map>

#include <Base/Handle.h>
#include <Base/Matrix.h>
#include <Base/Vector3D.h>

#include <App/PropertyStandard.h>
#include <App/PropertyGeo.h>

#include "Core/MeshKernel.h"
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
    TYPESYSTEM_HEADER();

public:
    PropertyNormalList();
    ~PropertyNormalList();

    virtual void setSize(int newSize);
    virtual int getSize() const;

    void setValue(const Base::Vector3f&);
    void setValue(float x, float y, float z);

    const Base::Vector3f& operator[] (const int idx) const {
        return _lValueList[idx];
    }

    void set1Value (const int idx, const Base::Vector3f& value) {
        _lValueList[idx] = value;
    }

    void setValues (const std::vector<Base::Vector3f>& values);

    const std::vector<Base::Vector3f> &getValues() const {
        return _lValueList;
    }

    virtual PyObject *getPyObject();
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual void SaveDocFile (Base::Writer &writer) const;
    virtual void RestoreDocFile(Base::Reader &reader);

    virtual App::Property *Copy() const;
    virtual void Paste(const App::Property &from);

    virtual unsigned int getMemSize () const;

    void transformGeometry(const Base::Matrix4D &rclMat);

private:
    std::vector<Base::Vector3f> _lValueList;
};

/** Curvature information. */
struct MeshExport CurvatureInfo
{
    float fMaxCurvature, fMinCurvature;
    Base::Vector3f cMaxCurvDir, cMinCurvDir;
};

/** The Curvature property class.
 * @author Werner Mayer
 */
class MeshExport PropertyCurvatureList: public App::PropertyLists
{
    TYPESYSTEM_HEADER();

public:
    enum { 
        MeanCurvature  = 0,  /**< Mean curvature */
        GaussCurvature = 1,  /**< Gaussian curvature */
        MaxCurvature   = 2,  /**< Maximum curvature */ 
        MinCurvature   = 3,  /**< Minimum curvature */
        AbsCurvature   = 4   /**< Absolute curvature */
    };

public:
    PropertyCurvatureList();
    ~PropertyCurvatureList();

    void setSize(int newSize){_lValueList.resize(newSize);}
    int getSize() const {return _lValueList.size();}
    std::vector<float> getCurvature( int tMode) const;
    void setValue(const CurvatureInfo&);
    void setValues(const std::vector<CurvatureInfo>&);

    /// index operator
    const CurvatureInfo& operator[] (const int idx) const {
        return _lValueList[idx];
    }
    void  set1Value (const int idx, const CurvatureInfo& value) {
        _lValueList[idx] = value;
    }
    const std::vector<CurvatureInfo> &getValues() const {
        return _lValueList;
    }
    void transformGeometry(const Base::Matrix4D &rclMat);

    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);

    void SaveDocFile (Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);

    /** @name Python interface */
    //@{
    PyObject* getPyObject();
    void setPyObject(PyObject *value);
    //@}

    App::Property *Copy() const;
    void Paste(const App::Property &from);

    virtual unsigned int getMemSize () const{return _lValueList.size() * sizeof(CurvatureInfo);}

private:
    std::vector<CurvatureInfo> _lValueList;
};

/** The mesh kernel property class.
 * @author Werner Mayer
 */
class MeshExport PropertyMeshKernel : public App::PropertyComplexGeoData
{
    TYPESYSTEM_HEADER();

public:
    PropertyMeshKernel();
    ~PropertyMeshKernel();

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
    const MeshObject &getValue() const;
    const MeshObject *getValuePtr() const;
    virtual unsigned int getMemSize () const;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    const Data::ComplexGeoData* getComplexData() const;
    /** Returns the bounding box around the underlying mesh kernel */
    Base::BoundBox3d getBoundingBox() const;
    //@}

    /** @name Modification */
    //@{
    MeshObject* startEditing();
    void finishEditing();
    /// Transform the real mesh data
    void transformGeometry(const Base::Matrix4D &rclMat);
    void setPointIndices( const std::vector<std::pair<PointIndex, Base::Vector3f> >& );
    void setTransform(const Base::Matrix4D& rclTrf);
    Base::Matrix4D getTransform() const;
    //@}

    /** @name Python interface */
    //@{
    /** Returns a Python wrapper for the referenced mesh object. It does NOT 
     * create a copy. However, the Python wrapper is marked as \a immutable so
     * that the mesh object cannot be modified from outside.
     */
    PyObject* getPyObject();
    /** This method copies the content, hence creates an new mesh object 
     * to copy the data. The passed argument can be an instance of the Python
     * wrapper for the mesh object or simply a list of triangles, i.e. a list
     * of lists of three floats.
     */
    void setPyObject(PyObject *value);
    //@}

    const char* getEditorName() const {
        return "MeshGui::PropertyMeshKernelItem";
    }

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);

    void SaveDocFile (Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);

    App::Property *Copy() const;
    void Paste(const App::Property &from);
    //@}

private:
    Base::Reference<MeshObject> _meshObject;
    MeshPy* meshPyObject;
};

} // namespace Mesh

#endif // MESH_MESHPROPERTIES_H
