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


#ifndef SKETCHER_GEOMETRYEXTERNALFACADE_H
#define SKETCHER_GEOMETRYEXTERNALFACADE_H

#include <Base/BaseClass.h>

#include <Base/Console.h> // Only for Debug - To be removed
#include <boost/uuid/uuid_io.hpp>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchGeometryExtension.h>
#include <Mod/Sketcher/App/ExternalGeometryExtension.h>

namespace Sketcher
{

class ExternalGeometryFacadePy;
// This class is a Facade to handle EXTERNAL sketcher geometry and sketcher geometry extensions with a single sketcher specific interface.
//
// Exactly the same considerations as for GeometryFacade apply (see documentation of GeometryFacade).
//
// It was not made publicly deriving from GeometryFacade because it is not possible to differentiate functions by return type, which is the
// case of getFacade() returning a unique_ptr to GeometryFacade in GeometryFacade, and one to ExternalGeometryFacade. I have not managed to
// find a good solution to this problem, thus the code duplication.
//
// Summary Remarks:
// It is intended to have a separate type (not being a Geometry type).
// it is intended to have the relevant interface in full for the sketcher extension only
// It is intended to work on borrowed memory allocation.
class SketcherExport ExternalGeometryFacade : public Base::BaseClass, private ISketchGeometryExtension, private ISketchExternalGeometryExtension
{
TYPESYSTEM_HEADER_WITH_OVERRIDE();

private:
    ExternalGeometryFacade(const Part::Geometry * geometry);
    ExternalGeometryFacade(); // As TYPESYSTEM requirement

    friend class ExternalGeometryFacadePy;

public: // Factory methods
    static std::unique_ptr<ExternalGeometryFacade> getFacade(Part::Geometry * geometry);
    static std::unique_ptr<const ExternalGeometryFacade> getFacade(const Part::Geometry * geometry);

public: // Utility methods
    static void ensureSketchGeometryExtensions(Part::Geometry * geometry);
    static void copyId(const Part::Geometry * src, Part::Geometry * dst);

public:
    void setGeometry(Part::Geometry *geometry);

    /** External GeometryExtension Interface **/
    virtual bool testFlag(int flag) const override { return getExternalGeoExt()->testFlag(flag); }
    virtual void setFlag(int flag, bool v=true) override { getExternalGeoExt()->setFlag(flag, v); }

    virtual bool isClear() const override {return getExternalGeoExt()->isClear();}
    virtual size_t flagSize() const override {return getExternalGeoExt()->flagSize();}

    virtual const std::string& getRef() const override {return getExternalGeoExt()->getRef();}
    virtual void setRef(const std::string & ref) override {getExternalGeoExt()->setRef(ref);}

    /** GeometryExtension Interface **/
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

    // Geometry Extension Information
    inline const std::string &getSketchExtensionName () const {return SketchGeoExtension->getName();}
    inline const std::string &getExternalExtensionName () const {return ExternalGeoExtension->getName();}

    // Geometry Element
    template <  typename GeometryT = Part::Geometry,
                typename = typename std::enable_if<
                    std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value
             >::type
    >
    GeometryT * getGeometry() {return dynamic_cast<GeometryT *>(const_cast<GeometryT *>(Geo));}

    // Geometry Element
    template <  typename GeometryT = Part::Geometry,
                typename = typename std::enable_if<
                    std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value
             >::type
    >
    GeometryT * getGeometry() const {return dynamic_cast<GeometryT *>(Geo);}

    virtual PyObject *getPyObject(void) override;

    /** Geometry Interface **/
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

private:
    void initExtensions(void);
    void initExtensions(void) const;

    const Part::Geometry * getGeo(void) const {return Geo;}
    Part::Geometry * getGeo(void) {return const_cast<Part::Geometry *>(Geo);}

    std::shared_ptr<const SketchGeometryExtension> getGeoExt(void) const {return SketchGeoExtension;}
    std::shared_ptr<SketchGeometryExtension> getGeoExt (void) {return std::const_pointer_cast<SketchGeometryExtension>(SketchGeoExtension);}

    std::shared_ptr<const ExternalGeometryExtension> getExternalGeoExt(void) const {return ExternalGeoExtension;}
    std::shared_ptr<ExternalGeometryExtension> getExternalGeoExt (void) {return std::const_pointer_cast<ExternalGeometryExtension>(ExternalGeoExtension);}

private:
    const Part::Geometry * Geo;
    std::shared_ptr<const SketchGeometryExtension> SketchGeoExtension;
    std::shared_ptr<const ExternalGeometryExtension> ExternalGeoExtension;
};




} //namespace Sketcher


#endif // SKETCHER_GEOMETRYEXTERNALFACADE_H
