/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef Fem_PropertyFemMesh_H
#define Fem_PropertyFemMesh_H

#include "FemMesh.h"
#include <App/PropertyGeo.h>
#include <Base/BoundBox.h>

namespace Fem
{


/** The part shape property class.
 * @author Werner Mayer
 */
class FemExport PropertyFemMesh : public App::PropertyComplexGeoData
{
    TYPESYSTEM_HEADER();

public:
    PropertyFemMesh();
    ~PropertyFemMesh();

    /** @name Getter/setter */
    //@{
    void setValuePtr(FemMesh* mesh);
    /// set the FemMesh shape
    void setValue(const FemMesh&);
    /// does nothing, for add property macro
    void setValue(void){}
    /// get the FemMesh shape
    const FemMesh &getValue(void) const;
    const Data::ComplexGeoData* getComplexData() const;
    //@}


    /** @name Getting basic geometric entities */
    //@{
    /** Returns the bounding box around the underlying mesh kernel */
    Base::BoundBox3d getBoundingBox() const;
    /// Set the placement of the geometry
    void setTransform(const Base::Matrix4D& rclTrf);
    /// Get the placement of the geometry
    Base::Matrix4D getTransform() const;
    void transformGeometry(const Base::Matrix4D &rclMat);
    //@}

    /** @name Python interface */
    //@{
    PyObject* getPyObject(void);
    void setPyObject(PyObject *value);
    //@}

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);
    void SaveDocFile (Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);

    App::Property *Copy(void) const;
    void Paste(const App::Property &from);
    unsigned int getMemSize (void) const;
    const char* getEditorName(void) const { return "FemGui::PropertyFemMeshItem"; }
    //@}

private:
    Base::Reference<FemMesh> _FemMesh;
};


} //namespace Fem


#endif // PROPERTYTOPOSHAPE_H
