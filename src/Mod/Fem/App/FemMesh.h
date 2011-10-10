/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2009     *
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


#ifndef FEM_FEMMESH_H
#define FEM_FEMMESH_H

#include <App/ComplexGeoData.h>
#include <Base/Placement.h>

#include <vector>
#include <list>
#include <boost/shared_ptr.hpp>

class SMESH_Gen;
class SMESH_Mesh;
class SMESH_Hypothesis;
class TopoDS_Shape;

namespace Fem
{

typedef boost::shared_ptr<SMESH_Hypothesis> SMESH_HypothesisPtr;

/** The representation of a FemMesh
 */
class AppFemExport FemMesh : public Data::ComplexGeoData
{
    TYPESYSTEM_HEADER();

public:
    FemMesh();
    FemMesh(const FemMesh&);
    ~FemMesh();

    FemMesh &operator=(const FemMesh&);
    const SMESH_Mesh* getSMesh() const;
    SMESH_Mesh* getSMesh();
    SMESH_Gen * getGenerator();
    void addHypothesis(const TopoDS_Shape & aSubShape, SMESH_HypothesisPtr hyp);
    void setStanardHypotheses();
    void compute();

    // from base class
    virtual unsigned int getMemSize (void) const;
    virtual void Save (Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);
    void SaveDocFile (Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);

    /** @name Subelement management */
    //@{
    /** Sub type list
     *  List of different subelement types
     *  it is NOT a list of the subelements itself
     */
    virtual std::vector<const char*> getElementTypes(void) const;
    virtual unsigned long countSubElements(const char* Type) const;
    /// get the subelement by type and number
    virtual Data::Segment* getSubElement(const char* Type, unsigned long) const;
    //@}

    /** @name Placement control */
    //@{
    /// set the transformation 
    void setTransform(const Base::Matrix4D& rclTrf);
    /// get the transformation 
    Base::Matrix4D getTransform(void) const;
    /// Bound box from the shape
    Base::BoundBox3d getBoundBox(void)const;
    //@}

    /** @name Modification */
    //@{
    /// Applies a transformation on the real geometric data type
    void transformGeometry(const Base::Matrix4D &rclMat);
    //@}

    /// import from files
    void read(const char *FileName);
    void write(const char *FileName) const;
    void writeABAQUS(const std::string &Filename, Base::Placement* = 0) const;

private:
    void copyMeshData(const FemMesh&);
    void readNastran(const std::string &Filename);

private:
    SMESH_Gen  *myGen;
    SMESH_Mesh *myMesh;

    std::list<SMESH_HypothesisPtr> hypoth;
};

} //namespace Part


#endif // FEM_FEMMESH_H
