// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <boost/uuid/uuid_io.hpp>

#include <Base/BaseClass.h>

#include "SketchGeometryExtension.h"


namespace Sketcher
{

class GeometryFacadePy;
/** @brief This class is a Facade to handle geometry and sketcher geometry extensions with a single
 * sketcher specific interface
 *
 * @details
 * The facade privately inherits from a common interface it shares with the extension thereby
 * implementing a compiler enforced same interface as the extension. It does not inherit from
 * Part::Geometry and thus is intended to provide, in part a convenience subset of the interface of
 * Part::Geometry, in part a different interface.
 *
 * GeometryFacade has private constructors and objects may only be created using the getFacade
 * factory methods.
 *
 * There is a version of getFacade taking a const Part::Geometry and producing a const
 * GeometryFacade, and a non-const version producing a non-const GeometryFacade. So constness of the
 * Part::Geometry object is preserved by the GeometryFacade container.
 *
 * There are some static convenience utility functions to simplify common operations such as ID copy
 * or to ensure that a geometry object has the extension (creating the extension if not existing).
 *
 * @warning
 * The const factory method will throw if the geometry does not have a SketchGeometryExtension
 * (being const, it commits not to create one and modify the const Part::Geometry object). The
 * non-const factory method will create the extension if not existing.
 *
 * @warning
 * If the Geometry Pointer fed into the factory method is a nullptr, a nullptr GeometryFacade is
 * created. It should not be possible to create a GeometryFacade having a Part::Geometry * being a
 * nullptr.
 *
 * A simple usage example:
 *
 * const std::vector< Part::Geometry * > &vals = getInternalGeometry();
 * auto gf = GeometryFacade::getFacade(vals[GeoId]);
 * id = gf->getId();
 *
 * An example of static Id utility function
 *
 * const Part::Geometry *geo = getGeometry(GeoId);
 * ...
 * std::unique_ptr<Part::GeomBSplineCurve> bspline(new Part::GeomBSplineCurve(curve));
 * ...
 *
 * Part::GeomBSplineCurve * gbsc = bspline.release();
 * GeometryFacade::copyId(geo, gbsc);
 *
 * Examples getting and setting the construction stations without creating a Facade:
 *
 *  if ((*geo) && GeometryFacade::getConstruction(*geo) &&
 *      (*geo)->is<Part::GeomLineSegment>())
 *            count++;
 *
 *  Part::Geometry* copy = v->copy();
 *
 *  if(construction && !copy->is<Part::GeomPoint>()) {
 *      GeometryFacade::setConstruction(copy, construction);
 *  }
 *
 * Note: The standard GeometryFacade stores Part::Geometry derived classes as a Part::Geometry *,
 * while it has the ability to return a dynamic_cast-ed version to a provided type as follows:
 *
 * HLine->getGeometry<Part::GeomLineSegment>();
 *
 * If for seamless operation it is convenient to have a given derived class of Part::Geometry, it is
 * possible to use GeometryTypedFacade (see below).
 *
 * @remarks
 * Summary Remarks:
 * It is intended to have a separate type (not being a Geometry type).
 * it is intended to have the relevant interface in full for the sketcher extension only
 * It is intended to work on borrowed memory allocation. But the getFacade has an owner parameter to
 * take ownership of the geometry pointer if that is intended (this can also be achieved via the
 * setOwner method once created).
 */
class SketcherExport GeometryFacade: public Base::BaseClass, private ISketchGeometryExtension
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

protected:
    explicit GeometryFacade(const Part::Geometry* geometry, bool owner = false);
    GeometryFacade();  // As TYPESYSTEM requirement

    friend class GeometryFacadePy;

public:  // Factory methods
    static std::unique_ptr<GeometryFacade> getFacade(const Part::Geometry* geometry, bool owner = false);

public:  // Utility methods
    static void ensureSketchGeometryExtension(Part::Geometry* geometry);
    static void copyId(const Part::Geometry* src, Part::Geometry* dst);
    static bool getConstruction(const Part::Geometry* geometry);
    static void setConstruction(Part::Geometry* geometry, bool construction);
    static bool isInternalType(const Part::Geometry* geometry, InternalType::InternalType type);
    static bool isInternalAligned(const Part::Geometry* geometry);
    static InternalType::InternalType getInternalType(const Part::Geometry* geometry);
    static void setInternalType(Part::Geometry* geometry, InternalType::InternalType type);
    static bool getBlocked(const Part::Geometry* geometry);
    static int getId(const Part::Geometry* geometry);
    static void setId(const Part::Geometry* geometry, int id);

public:
    // Explicit deletion to show intent (not that it is needed)
    GeometryFacade(const GeometryFacade&) = delete;
    GeometryFacade& operator=(const GeometryFacade&) = delete;

    GeometryFacade(GeometryFacade&&) = default;
    GeometryFacade& operator=(GeometryFacade&&) = default;

    ~GeometryFacade() override;
    void setGeometry(Part::Geometry* geometry);

    void setOwner(bool owner)
    {
        OwnerGeo = owner;
    }

    // returns if the facade is the owner of the geometry pointer.
    bool getOwner() const
    {
        return OwnerGeo;
    }

    // Geometry Extension Interface
    inline long getId() const override
    {
        return getGeoExt()->getId();
    }
    void setId(long id) override
    {
        getGeoExt()->setId(id);
    }

    InternalType::InternalType getInternalType() const override
    {
        return getGeoExt()->getInternalType();
    }
    void setInternalType(InternalType::InternalType type) override
    {
        getGeoExt()->setInternalType(type);
    }

    bool testGeometryMode(int flag) const override
    {
        return getGeoExt()->testGeometryMode(flag);
    }
    void setGeometryMode(int flag, bool v = true) override
    {
        getGeoExt()->setGeometryMode(flag, v);
    }

    int getGeometryLayerId() const override
    {
        return getGeoExt()->getGeometryLayerId();
    }
    void setGeometryLayerId(int geolayer) override
    {
        getGeoExt()->setGeometryLayerId(geolayer);
    }

    // Convenience accessor
    bool getBlocked() const
    {
        return this->testGeometryMode(GeometryMode::Blocked);
    }
    void setBlocked(bool status = true)
    {
        this->setGeometryMode(GeometryMode::Blocked, status);
    }

    inline bool getConstruction() const
    {
        return this->testGeometryMode(GeometryMode::Construction);
    }
    inline void setConstruction(bool construction)
    {
        this->setGeometryMode(GeometryMode::Construction, construction);
    }

    bool isInternalAligned() const
    {
        return this->getInternalType() != InternalType::None;
    }

    bool isInternalType(InternalType::InternalType type) const
    {
        return this->getInternalType() == type;
    }

    // Geometry Extension Information
    inline const std::string& getExtensionName() const
    {
        return SketchGeoExtension->getName();
    }

    // Geometry Element
    template<
        typename GeometryT = Part::Geometry,
        typename = typename std::enable_if<
            std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value>::type>
    GeometryT* getGeometry()
    {
        return freecad_cast<GeometryT*>(const_cast<Part::Geometry*>(Geo));
    }

    // Geometry Element
    template<
        typename GeometryT = Part::Geometry,
        typename = typename std::enable_if<
            std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value>::type>
    const GeometryT* getGeometry() const
    {
        return dynamic_cast<const GeometryT*>(Geo);
    }

    PyObject* getPyObject() override;

    // Geometry Interface
    TopoDS_Shape toShape() const
    {
        return getGeo()->toShape();
    }
    const Handle(Geom_Geometry) & handle() const
    {
        return getGeo()->handle();
    }
    Part::Geometry* copy() const
    {
        return getGeo()->copy();
    }
    Part::Geometry* clone() const
    {
        return getGeo()->clone();
    }
    boost::uuids::uuid getTag() const
    {
        return getGeo()->getTag();
    }

    std::vector<std::weak_ptr<const Part::GeometryExtension>> getExtensions() const
    {
        return getGeo()->getExtensions();
    }
    bool hasExtension(const Base::Type& type) const
    {
        return getGeo()->hasExtension(type);
    }
    bool hasExtension(const std::string& name) const
    {
        return getGeo()->hasExtension(name);
    }
    std::weak_ptr<const Part::GeometryExtension> getExtension(const Base::Type& type) const
    {
        return getGeo()->getExtension(type);
    }
    std::weak_ptr<const Part::GeometryExtension> getExtension(const std::string& name) const
    {
        return getGeo()->getExtension(name);
    }
    void setExtension(std::unique_ptr<Part::GeometryExtension>&& geo)
    {
        return getGeo()->setExtension(std::move(geo));
    }
    void deleteExtension(const Base::Type& type)
    {
        return getGeo()->deleteExtension(type);
    }
    void deleteExtension(const std::string& name)
    {
        return getGeo()->deleteExtension(name);
    }

    void mirror(const Base::Vector3d& point)
    {
        return getGeo()->mirror(point);
    }
    void mirror(const Base::Vector3d& point, Base::Vector3d dir)
    {
        return getGeo()->mirror(point, dir);
    }
    void rotate(const Base::Placement& plm)
    {
        return getGeo()->rotate(plm);
    }
    void scale(const Base::Vector3d& vec, double scale)
    {
        return getGeo()->scale(vec, scale);
    }
    void transform(const Base::Matrix4D& mat)
    {
        return getGeo()->transform(mat);
    }
    void translate(const Base::Vector3d& vec)
    {
        return getGeo()->translate(vec);
    }

    // convenience GeometryFunctions
    bool isGeoType(const Base::Type& type) const
    {
        return getGeo()->getTypeId() == type;
    }

private:
    void initExtension();
    void initExtension() const;

    const Part::Geometry* getGeo() const
    {
        return Geo;
    }
    Part::Geometry* getGeo()
    {
        return const_cast<Part::Geometry*>(Geo);
    }

    std::shared_ptr<const SketchGeometryExtension> getGeoExt() const
    {
        return SketchGeoExtension;
    }
    std::shared_ptr<SketchGeometryExtension> getGeoExt()
    {
        return std::const_pointer_cast<SketchGeometryExtension>(SketchGeoExtension);
    }

    static void throwOnNullPtr(const Part::Geometry* geo);

private:
    const Part::Geometry* Geo;
    bool OwnerGeo;
    std::shared_ptr<const SketchGeometryExtension> SketchGeoExtension;
};


///////////////////////////////////////////////////////////////////////////////////////
//
// GeometryTypedFacade

/** @brief  It provides all the functionality of GeometryFacade (derives from it), but in addition
 * allows one to indicate the type of a Part::Geometry derived class.
 *
 * @details
 *
 * auto HLineF = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade(HLine);
 *
 * Then it is possible to get the typed geometry directly via:
 *
 * HLine->getTypedGeometry()->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0));
 *
 * If a facade is requested without passing an Part::Geometry derived object, the constructor
 * of the indicated geometry type is called with any parameter passed as argument (emplace style).
 * In this case the facade takes ownership of the newly created Part::Geometry object.
 *
 *  Example of seamless operation with a GeomLineSegment:
 *
 *    auto HLine = GeometryTypedFacade<Part::GeomLineSegment>::getTypedFacade();
 *    HLine->getTypedGeometry()->setPoints(Base::Vector3d(0,0,0),Base::Vector3d(1,0,0));
 *    HLine->setConstruction(true);
 *    ExternalGeo.push_back(HLine->getGeometry());
 */
template<typename GeometryT>
class SketcherExport GeometryTypedFacade: public GeometryFacade
{
    static_assert(
        std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value
            && !std::is_same<Part::Geometry, typename std::decay<GeometryT>::type>::value,
        "Only for classes derived from Geometry!"
    );

private:
    explicit GeometryTypedFacade(const Part::Geometry* geometry, bool owner = false)
        : GeometryFacade(geometry, owner)
    {}
    GeometryTypedFacade()
        : GeometryFacade()
    {}

public:  // Factory methods
    static std::unique_ptr<GeometryTypedFacade<GeometryT>> getTypedFacade(
        GeometryT* geometry,
        bool owner = false
    )
    {
        if (geometry) {
            return std::unique_ptr<GeometryTypedFacade<GeometryT>>(
                new GeometryTypedFacade(geometry, owner)
            );
        }
        else {
            return std::unique_ptr<GeometryTypedFacade<GeometryT>>(nullptr);
        }
    }
    static std::unique_ptr<const GeometryTypedFacade<GeometryT>> getTypedFacade(
        const GeometryT* geometry
    )
    {
        if (geometry) {
            return std::unique_ptr<const GeometryTypedFacade<GeometryT>>(
                new GeometryTypedFacade(geometry)
            );
        }
        else {
            return std::unique_ptr<const GeometryTypedFacade<GeometryT>>(nullptr);
        }
    }

    // This function takes direct ownership of the object it creates.
    template<typename... Args>
    static std::unique_ptr<GeometryTypedFacade<GeometryT>> getTypedFacade(Args&&... args)
    {
        return GeometryTypedFacade::getTypedFacade(new GeometryT(std::forward<Args>(args)...), true);
    }

    // Geometry Element
    GeometryT* getTypedGeometry()
    {
        return GeometryFacade::getGeometry<GeometryT>();
    }

    // Geometry Element
    GeometryT* getTypedGeometry() const
    {
        return GeometryFacade::getGeometry<GeometryT>();
    }
};


}  // namespace Sketcher
