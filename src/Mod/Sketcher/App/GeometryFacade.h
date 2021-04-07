/***************************************************************************
 *   Copyright (c) 2020 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef SKETCHER_GEOMETRYFACADE_H
#define SKETCHER_GEOMETRYFACADE_H

#include <Base/BaseClass.h>

#include <Base/Console.h> // Only for Debug - To be removed
#include <boost/uuid/uuid_io.hpp>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchGeometryExtension.h>

namespace Sketcher
{

class GeometryFacadePy;
// This class is a Facade to handle geometry and sketcher geometry extensions with a single sketcher specific interface
//
// The facade privately inherits from a common interface it shares with the extension thereby implementing a compiler enforced
// same interface as the extension. It does not inherit from Part::Geometry and thus is intended to provide, in part a convenience
// subset of the interface of Part::Geometry, in part a different interface.
//
// GeometryFacade has private constructors and objects may only be created using the getFacade factory methods.
//
// There is a version of getFacade taking a const Part::Geometry and producing a const GeometryFacade, and a non-const
// version producing a non-const GeometryFacade. So constness of the Part::Geometry object is preserved by the GeometryFacade
// container.
//
// The const factory method will throw if the geometry does not have a SketchGeometryExtension (being const, it commits not to
// create one and modify the const Part::Geometry object). The non-const factory method will create the extension if not existing.
//
// There are some static convenience utility functions to simplify common operations such as ID copy or to ensure that a geometry
// object has the extension (creating the extension if not existing).
//
// A simple usage example:
//
// const std::vector< Part::Geometry * > &vals = getInternalGeometry();
// auto gf = GeometryFacade::getFacade(vals[GeoId]);
// id = gf->getId();
//
// An example of static Id utility function
//
// const Part::Geometry *geo = getGeometry(GeoId);
// ...
// std::unique_ptr<Part::GeomBSplineCurve> bspline(new Part::GeomBSplineCurve(curve));
// ...
//
// Part::GeomBSplineCurve * gbsc = bspline.release();
// GeometryFacade::copyId(geo, gbsc);
//
// Examples getting and setting the construction stations without creating a Facade:
//
//  if ((*geo) && GeometryFacade::getConstruction(*geo) &&
//      (*geo)->getTypeId() == Part::GeomLineSegment::getClassTypeId())
//            count++;
//
//  Part::Geometry* copy = v->copy();
//
//  if(construction && copy->getTypeId() != Part::GeomPoint::getClassTypeId()) {
//      GeometryFacade::setConstruction(copy, construction);
//  }
//
//
// Note: The standard GeometryFacade stores Part::Geometry derived classes as a Part::Geometry *, while
// it has the ability to return a dynamic_cast-ed version to a provided type as follows:
//
// HLine->getGeometry<Part::GeomLineSegment>();
//
// If for seamless operation it is convenient to have a given derived class of Part::Geometry, it is possible
// to use GeometryTypedFacade (see below).
//
// Summary Remarks:
// It is intended to have a separate type (not being a Geometry type).
// it is intended to have the relevant interface in full for the sketcher extension only
// It is intended to work on borrowed memory allocation.
class SketcherExport GeometryFacade : public Base::BaseClass, private ISketchGeometryExtension
{
TYPESYSTEM_HEADER_WITH_OVERRIDE();

protected:
    GeometryFacade(const Part::Geometry * geometry);
    GeometryFacade(); // As TYPESYSTEM requirement

    friend class GeometryFacadePy;

public: // Factory methods
    static std::unique_ptr<GeometryFacade> getFacade(Part::Geometry * geometry);
    static std::unique_ptr<const GeometryFacade> getFacade(const Part::Geometry * geometry);

public: // Utility methods
    static void ensureSketchGeometryExtension(Part::Geometry * geometry);
    static void copyId(const Part::Geometry * src, Part::Geometry * dst);
    static bool getConstruction(const Part::Geometry * geometry);
    static void setConstruction(Part::Geometry * geometry, bool construction);
    static bool isInternalType(const Part::Geometry * geometry, InternalType::InternalType type);
    static bool getBlocked(const Part::Geometry * geometry);

public:
    void setGeometry(Part::Geometry *geometry);

    // Geometry Extension Interface
    inline virtual long getId() const override {return getGeoExt()->getId();}
    virtual void setId(long id) override {getGeoExt()->setId(id);}

    virtual InternalType::InternalType getInternalType() const override {return getGeoExt()->getInternalType();}
    virtual void setInternalType(InternalType::InternalType type) override {getGeoExt()->setInternalType(type);}

    virtual bool testGeometryMode(int flag) const override { return getGeoExt()->testGeometryMode(flag); }
    virtual void setGeometryMode(int flag, bool v=true) override { getGeoExt()->setGeometryMode(flag, v); }

    // Convenience accessor
    bool getBlocked() const { return this->testGeometryMode(GeometryMode::Blocked);}
    void setBlocked(bool status = true) {this->setGeometryMode(GeometryMode::Blocked, status);}

    inline bool getConstruction(void) const {return this->testGeometryMode(GeometryMode::Construction);};
    inline void setConstruction(bool construction) {this->setGeometryMode(GeometryMode::Construction, construction);};

    bool isInternalAligned() const { return this->getInternalType() != InternalType::None; }

    // Geometry Extension Information
    inline const std::string &getExtensionName () const {return SketchGeoExtension->getName();}

    // Geometry Element
    template <  typename GeometryT = Part::Geometry,
                typename = typename std::enable_if<
                    std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value
             >::type
    >
    GeometryT * getGeometry() {return dynamic_cast<GeometryT *>(const_cast<Part::Geometry *>(Geo));}

    // Geometry Element
    template <  typename GeometryT = Part::Geometry,
                typename = typename std::enable_if<
                    std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value
             >::type
    >
    GeometryT * getGeometry() const {return dynamic_cast<GeometryT *>(Geo);}

    virtual PyObject *getPyObject(void) override;

    // Geometry Interface
    TopoDS_Shape toShape() const {return getGeo()->toShape();};
    const Handle(Geom_Geometry)& handle() const {return getGeo()->handle();};
    Part::Geometry *copy(void) const {return getGeo()->copy();};
    Part::Geometry *clone(void) const {return getGeo()->clone();};
    boost::uuids::uuid getTag() const {return getGeo()->getTag();};

    std::vector<std::weak_ptr<const Part::GeometryExtension>> getExtensions() const {return getGeo()->getExtensions();};
    bool hasExtension(Base::Type type) const {return getGeo()->hasExtension(type);};
    bool hasExtension(std::string name) const {return getGeo()->hasExtension(name);};
    std::weak_ptr<const Part::GeometryExtension> getExtension(Base::Type type) const {return getGeo()->getExtension(type);};
    std::weak_ptr<const Part::GeometryExtension> getExtension(std::string name) const {return getGeo()->getExtension(name);};
    void setExtension(std::unique_ptr<Part::GeometryExtension> &&geo) {return getGeo()->setExtension(std::move(geo));};
    void deleteExtension(Base::Type type) {return getGeo()->deleteExtension(type);};
    void deleteExtension(std::string name) {return getGeo()->deleteExtension(name);};

    void mirror(Base::Vector3d point) {return getGeo()->mirror(point);};
    void mirror(Base::Vector3d point, Base::Vector3d dir) {return getGeo()->mirror(point, dir);};
    void rotate(Base::Placement plm) {return getGeo()->rotate(plm);};
    void scale(Base::Vector3d vec, double scale) {return getGeo()->scale(vec, scale);};
    void transform(Base::Matrix4D mat) {return getGeo()->transform(mat);};
    void translate(Base::Vector3d vec) {return getGeo()->translate(vec);};

    // convenience GeometryFunctions
    bool isGeoType(const Base::Type &type) const { return getGeo()->getTypeId() == type;}

private:
    void initExtension(void);
    void initExtension(void) const;

    const Part::Geometry * getGeo(void) const {return Geo;}
    Part::Geometry * getGeo(void) {return const_cast<Part::Geometry *>(Geo);}

    std::shared_ptr<const SketchGeometryExtension> getGeoExt(void) const {return SketchGeoExtension;}
    std::shared_ptr<SketchGeometryExtension> getGeoExt (void) {return std::const_pointer_cast<SketchGeometryExtension>(SketchGeoExtension);}

private:
    const Part::Geometry * Geo;
    std::shared_ptr<const SketchGeometryExtension> SketchGeoExtension;
};



///////////////////////////////////////////////////////////////////////////////////////
//
// GeometryTypedFacade
//
// It provides all the funcionality of GeometryFacade (derives from it), but in addition
// allows to indicate the type of a Part::Geometry derived class.
//
// auto HLineF = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade(HLine);
//
// Then it is possible to get the typed geometry directly via:
//
// HLine->getTypedGeometry()->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0));
//
// If a facade is requested without passing an Part::Geometry derived object, the constructor
// of the indicated geometry type is called with any parameter passed as argument (emplace style)
//
//  Example of seamless operation with a GeomLineSegment:
//
//    auto HLine = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade();
//    HLine->getTypedGeometry()->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0));
//    HLine->setConstruction(true);
//    ExternalGeo.push_back(HLine->getGeometry());

template < typename GeometryT >
class SketcherExport GeometryTypedFacade : public GeometryFacade
{
    static_assert(  std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value &&
                    !std::is_same<Part::Geometry, typename std::decay<GeometryT>::type>::value, "Only for classes derived from Geometry!");
    private:
    GeometryTypedFacade(const Part::Geometry * geometry):GeometryFacade(geometry) {};
    GeometryTypedFacade():GeometryFacade() {};

public: // Factory methods
    static std::unique_ptr<GeometryTypedFacade<GeometryT>> getTypedFacade(GeometryT * geometry) {
        if(geometry != nullptr)
            return std::unique_ptr<GeometryTypedFacade<GeometryT>>(new GeometryTypedFacade(geometry));
        else
            return std::unique_ptr<GeometryTypedFacade<GeometryT>>(nullptr);
    }
    static std::unique_ptr<const GeometryTypedFacade<GeometryT>> getTypedFacade(const GeometryT * geometry) {
        if(geometry != nullptr)
            return std::unique_ptr<const GeometryTypedFacade<GeometryT>>(new GeometryTypedFacade(geometry));
        else
            return std::unique_ptr<const GeometryTypedFacade<GeometryT>>(nullptr);
    }

    template < typename... Args >
    static std::unique_ptr<GeometryTypedFacade<GeometryT>> getTypedFacade(Args&&... args) {
        return GeometryTypedFacade::getTypedFacade(new GeometryT(std::forward<Args>(args)...));
    }

    // Geometry Element
    GeometryT * getTypedGeometry() {return GeometryFacade::getGeometry<GeometryT>();}

    // Geometry Element
    GeometryT * getTypedGeometry() const {return GeometryFacade::getGeometry<GeometryT>();}
};





} //namespace Sketcher


#endif // SKETCHER_GEOMETRYFACADE_H
