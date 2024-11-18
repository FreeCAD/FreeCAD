/***************************************************************************
 *   Copyright (c) 2021 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHER_GeoList_H
#define SKETCHER_GeoList_H

#include <memory>
#include <vector>

#include "GeoEnum.h"
#include "GeometryFacade.h"


namespace Base
{
template<typename T>
class Vector3;
}

namespace Part
{
class Geometry;
}

namespace Sketcher
{
}

namespace Sketcher
{

/** @brief      Class for managing internal and external geometry as a single object
 *  @details
 *  Internal and external geometries are present in a single geometry vector one after the other.
 *
 * This class follows the format used by solver facade (sketch.cpp) in:
 * getSolvedSketch().extractGeometry() and by SketchObject in getCompleteGeometry(). Care should be
 * taken that the former may provide the list with a deep copy of the geometry pointers, whereas the
 * second just provides a shallow copy of the pointers.
 *
 * This class is templated to allow instantiations with list elements being legacy naked pointers
 * (Part::Geometry *) and GeometryFacade smart pointer objects. Convenience typedefs are provided:
 *
 * using GeoList = GeoListModel<GeometryPtr>;
 * using GeoListFacade = GeoListModel<GeometryFacadeUniquePtr>;
 *
 * with:
 * using GeometryPtr = Part::Geometry *;
 * using GeometryFacadeUniquePtr = std::unique_ptr<const Sketcher::GeometryFacade>;
 *
 * N.B.: Note that the index of the geomlist (all layers) and the GeoId can be converted
 * from each other as needed using the member functions (and sometimes the static functions).
 */
template<typename T>
class GeoListModel
{
    using Vector3d = Base::Vector3<double>;


protected:
    /** @brief
     * Constructors are protected, use static methods getGeoListModel() to construct the objects
     * instead.
     *
     * Constructs the object from a list of geometry in geomlist format and the number of internal
     * geometries (non external) present in the list.
     *
     * @param geometrylist: the geometry in geomlist format (external after internal in a single
     * vector).
     * @param intgeocount: the number of internal geometries (non external) in the list.
     * @param ownerT: indicates whether the GeoListModel takes ownership of the elements of the
     * std::vector<T> (for pointers)
     */
    explicit GeoListModel(std::vector<T>&& geometrylist, int intgeocount, bool ownerT = false);

    explicit GeoListModel(const std::vector<T>& geometrylist, int intgeocount);

public:
    /** @brief Destructor having type dependent behaviour
     *
     * @warning
     * For GeoList, the destructor will destruct the Part::Geometry pointers * only * if it was
     * constructed with ownerT = true.
     *
     * For GeoListFacade, the smart pointers will be deleted. However, a GeometryFacade does * not *
     * delete the underlying naked pointers by default (which is mostly the desired behaviour as the
     * ownership of the pointers belongs to sketchObject). If GeometryFacade is to delete the
     * underlying naked pointers (because it is a temporal deep copy), then the GeometryFacade needs
     * to get ownership (see setOwner method).
     *
     */
    ~GeoListModel();

    // Explicit deletion to show intent (not that it is needed). This is a move only type.
    GeoListModel(const GeoListModel&) = delete;
    GeoListModel& operator=(const GeoListModel&) = delete;

    // enable move constructor and move assignment. This is a move only type.
    GeoListModel(GeoListModel&&) = default;
    GeoListModel& operator=(GeoListModel&&) = default;

    /** @brief
     * GeoListModel manages the lifetime of its internal std::vector. This means that while the
     * actual ownership of the T parameter needs to be specified or separately handled. In the
     * absence of that, a new vector will be created and the T elements shallow copied to the
     * internal vector.
     *
     * The constness of the GeoListModel is tied to the constness of the std::vector from which it
     * is constructed.
     *
     * @warning
     * For GeoListFacade ownership at GeoListModel level cannot be taken (ownerT cannot be true). An
     * assertion is raised if this happens. The ownership needs to be specified on the GeoListFacade
     * objects themselves (setOwner method).
     */
    static GeoListModel<T>
    getGeoListModel(std::vector<T>&& geometrylist, int intgeocount, bool ownerT = false);
    static const GeoListModel<T> getGeoListModel(const std::vector<T>& geometrylist,
                                                 int intgeocount);


    /** @brief
     * returns the geometry given by the GeoId
     */
    const Part::Geometry* getGeometryFromGeoId(int geoId) const;

    /** @brief returns a geometryfacade
     * @warning If the underlying model of the list is a naked pointed (Part::Geometry *), i.e. a
     * GeoList instantiation, the client (the user) bears responsibility for releasing the
     * GeometryFacade pointer!!
     *
     * This is not a problem when the model of the list is a
     * std::unique_ptr<Sketcher::GeometryFacade>, because the lifetime is tied to the
     * GeometryFacade. It will destruct the pointer if it is the owner.
     */
    const Sketcher::GeometryFacade* getGeometryFacadeFromGeoId(int geoId) const;

    /** @brief
     * returns the GeoId index from the index in the geometry in geomlist format with which it was
     * constructed.
     *
     * @param index: the index of the list of geometry in geomlist format.
     */
    int getGeoIdFromGeomListIndex(int index) const;

    /** @brief
     * returns the geometry given by the GeoId in the geometrylist in geomlist format provided as a
     * parameter.
     *
     * @param geometrylist: the geometry in geomlist format (external after internal in a single
     * vector).
     *
     * @param index: the index of the list of geometry in geomlist format.
     */
    static const Part::Geometry* getGeometryFromGeoId(const std::vector<T>& geometrylist,
                                                      int geoId);

    /** @brief returns a geometry facade
     * @warning If the underlying model of the list is a naked pointed (Part::Geometry *), the
     * client (the user) bears responsibility for releasing the GeometryFacade pointer!!
     *
     * This is not a problem when the model of the list is a
     * std::unique_ptr<Sketcher::GeometryFacade>, because the lifetime is tied to the model itself.
     */
    static const Sketcher::GeometryFacade*
    getGeometryFacadeFromGeoId(const std::vector<T>& geometrylist, int geoId);

    /** @brief
     *  Obtain a GeoElementId class {GeoId, Pos} given a VertexId.
     *
     * A vertexId is a positive index of the vertex, where indices of external geometry taken higher
     * positive values than normal geometry. It is the same format of vertex numbering used in the
     * Sketcher, Sketch.cpp, and ViewProviderSketch.
     *
     */
    Sketcher::GeoElementId getGeoElementIdFromVertexId(int vertexId);


    /** @brief
     *  Given an GeoElementId {GeoId, Pos}, it returns the index of the vertex in VertexId format.
     *
     * A vertexId is a positive index of the vertex, where indices of external geometry taken higher
     * positive values than normal geometry. It is the same format of vertex numbering used in the
     * Sketcher, Sketch.cpp, and ViewProviderSketch.
     *
     */
    int getVertexIdFromGeoElementId(const Sketcher::GeoElementId& geoelementId) const;

    /** @brief
     *  Returns a point coordinates given {GeoId, Pos}.
     */
    Vector3d getPoint(int geoId, Sketcher::PointPos pos) const;

    /** @brief
     *  Returns a point coordinates given GeoElementId {GeoId, Pos}.
     */
    Vector3d getPoint(const GeoElementId& geid) const;

    /** @brief
     * returns the amount of internal (normal, non-external) geometry objects.
     */
    int getInternalCount() const
    {
        return intGeoCount;
    }

    /** @brief
     * returns the amount of external geometry objects.
     */
    int getExternalCount() const
    {
        return int(geomlist.size()) - intGeoCount;
    }

    /** @brief
     * return a reference to the internal geometry list vector.
     *
     * @warning { It returns a reference to the internal list vector. The validity of the
     * reference depends on the lifetime of the GeoListModel object.}
     */
    std::vector<T>& geometryList()
    {
        return const_cast<std::vector<T>&>(geomlist);
    }

public:
    std::vector<T> geomlist;

private:
    Vector3d getPoint(const Part::Geometry* geo, Sketcher::PointPos pos) const;

    void rebuildVertexIndex() const;

private:
    int intGeoCount;
    bool OwnerT;
    mutable bool indexInit;
    mutable std::vector<Sketcher::GeoElementId>
        VertexId2GeoElementId;  // these maps a lazy initialised on first demand.
    mutable std::map<Sketcher::GeoElementId, int> GeoElementId2VertexId;
};

using GeometryPtr = Part::Geometry*;
using GeometryFacadeUniquePtr = std::unique_ptr<const Sketcher::GeometryFacade>;

using GeoList = GeoListModel<GeometryPtr>;
using GeoListFacade = GeoListModel<GeometryFacadeUniquePtr>;

GeoListFacade getGeoListFacade(const GeoList& geolist);

}  // namespace Sketcher


#endif  // SKETCHER_GeoList_H
