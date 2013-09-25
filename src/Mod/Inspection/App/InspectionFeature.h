/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef INSPECTION_FEATURE_H
#define INSPECTION_FEATURE_H

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>
#include <App/DocumentObjectGroup.h>

#include <Mod/Mesh/App/Core/Iterator.h>
#include <Mod/Points/App/Points.h>

class TopoDS_Shape;
class BRepExtrema_DistShapeShape;

namespace MeshCore {
class MeshKernel;
class MeshGrid;
}

namespace Mesh   { class MeshObject; }
namespace Points { class PointsGrid; }
namespace Part   { class TopoShape;  }

namespace Inspection
{

/** Delivers the number of points to be checked and returns the appropriate point to an index. */
class InspectionExport InspectActualGeometry
{
public:
    InspectActualGeometry() {}
    virtual ~InspectActualGeometry() {}
    /// Number of points to be checked
    virtual unsigned long countPoints() const = 0;
    virtual Base::Vector3f getPoint(unsigned long) = 0;
};

class InspectionExport InspectActualMesh : public InspectActualGeometry
{
public:
    InspectActualMesh(const Mesh::MeshObject& rMesh);
    ~InspectActualMesh();
    virtual unsigned long countPoints() const;
    virtual Base::Vector3f getPoint(unsigned long);

private:
    MeshCore::MeshPointIterator _iter;
    unsigned long _count;
};

class InspectionExport InspectActualPoints : public InspectActualGeometry
{
public:
    InspectActualPoints(const Points::PointKernel&);
    virtual unsigned long countPoints() const;
    virtual Base::Vector3f getPoint(unsigned long);

private:
    const Points::PointKernel& _rKernel;
};

class InspectionExport InspectActualShape : public InspectActualGeometry
{
public:
    InspectActualShape(const Part::TopoShape&);
    virtual unsigned long countPoints() const;
    virtual Base::Vector3f getPoint(unsigned long);

private:
    const Part::TopoShape& _rShape;
    std::vector<Base::Vector3d> points;
};

/** Calculates the shortest distance of the underlying geometry to a given point. */
class InspectionExport InspectNominalGeometry
{
public:
    InspectNominalGeometry() {}
    virtual ~InspectNominalGeometry() {}
    virtual float getDistance(const Base::Vector3f&) = 0;
};

class InspectionExport InspectNominalMesh : public InspectNominalGeometry
{
public:
    InspectNominalMesh(const Mesh::MeshObject& rMesh, float offset);
    ~InspectNominalMesh();
    virtual float getDistance(const Base::Vector3f&);

private:
    MeshCore::MeshFacetIterator _iter;
    MeshCore::MeshGrid* _pGrid;
    Base::BoundBox3f _box;
};

class InspectionExport InspectNominalFastMesh : public InspectNominalGeometry
{
public:
    InspectNominalFastMesh(const Mesh::MeshObject& rMesh, float offset);
    ~InspectNominalFastMesh();
    virtual float getDistance(const Base::Vector3f&);

protected:
    MeshCore::MeshFacetIterator _iter;
    MeshCore::MeshGrid* _pGrid;
    Base::BoundBox3f _box;
    unsigned long max_level;
};

class InspectionExport InspectNominalPoints : public InspectNominalGeometry
{
public:
    InspectNominalPoints(const Points::PointKernel&, float offset);
    ~InspectNominalPoints();
    virtual float getDistance(const Base::Vector3f&);

private:
    const Points::PointKernel& _rKernel;
    Points::PointsGrid* _pGrid;
};

class InspectionExport InspectNominalShape : public InspectNominalGeometry
{
public:
    InspectNominalShape(const TopoDS_Shape&, float offset);
    ~InspectNominalShape();
    virtual float getDistance(const Base::Vector3f&);

private:
    BRepExtrema_DistShapeShape* distss;
    const TopoDS_Shape& _rShape;
};

class InspectionExport PropertyDistanceList: public App::PropertyLists
{
    TYPESYSTEM_HEADER();

public:

    /**
     * A constructor.
     * A more elaborate description of the constructor.
     */
    PropertyDistanceList();
    
    /**
     * A destructor.
     * A more elaborate description of the destructor.
     */
    virtual ~PropertyDistanceList();
    
    virtual void setSize(int newSize);
    virtual int getSize(void) const;

    /** Sets the property 
     */
    void setValue(float);
    
    /// index operator
    float operator[] (const int idx) const {return _lValueList.operator[] (idx);} 
    
    void set1Value (const int idx, float value){_lValueList.operator[] (idx) = value;}
    void setValues (const std::vector<float>& values);
    
    const std::vector<float> &getValues(void) const{return _lValueList;}
    
    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);
    
    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);
    
    virtual void SaveDocFile (Base::Writer &writer) const;
    virtual void RestoreDocFile(Base::Reader &reader);
    
    virtual Property *Copy(void) const;
    virtual void Paste(const Property &from);
    virtual unsigned int getMemSize (void) const;

private:
    std::vector<float> _lValueList;
};

// ----------------------------------------------------------------

/** The inspection feature.
 * \author Werner Mayer
 */
class InspectionExport Feature : public App::DocumentObject
{
    PROPERTY_HEADER(Inspection::Feature);

public:
    /// Constructor
    Feature(void);
    virtual ~Feature();

    /** @name Properties */
    //@{
    App::PropertyFloat     SearchRadius;
    App::PropertyFloat     Thickness;
    App::PropertyLink      Actual;
    App::PropertyLinkList  Nominals;
    PropertyDistanceList   Distances;
    //@}

    /** @name Actions */
    //@{
    short mustExecute() const;
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute(void);
    //@}

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const 
    { return "InspectionGui::ViewProviderInspection"; }
};

class InspectionExport Group : public App::DocumentObjectGroup
{
    PROPERTY_HEADER(Inspection::Group);

public:
    /// Constructor
    Group(void);
    virtual ~Group();

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const 
    { return "InspectionGui::ViewProviderInspectionGroup"; }
};

} //namespace Inspection


#endif // INSPECTION_FEATURE_H
