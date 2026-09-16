// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <optional>
#include <string_view>

#include <Base/Exception.h>
#include <Base/Tools.h>
#include <Base/Tools2D.h>
#include <Mod/Sketcher/App/GeoEnum.h>
#include <QListWidget>
#include <QMap>
#include <QString>

#include "AutoConstraint.h"
#include "ViewProviderSketchGeometryExtension.h"

namespace App
{
class DocumentObject;
}

namespace Gui
{
class DocumentObject;
class Document;
}  // namespace Gui

namespace Part
{
class Geometry;
}
namespace Sketcher
{
enum class PointPos : int;
class SketchObject;

bool isCircle(const Part::Geometry&);
bool isArcOfCircle(const Part::Geometry&);
bool isEllipse(const Part::Geometry&);
bool isArcOfEllipse(const Part::Geometry&);
bool isLineSegment(const Part::Geometry&);
bool isArcOfHyperbola(const Part::Geometry&);
bool isArcOfParabola(const Part::Geometry&);
bool isBSplineCurve(const Part::Geometry&);
bool isPeriodicBSplineCurve(const Part::Geometry&);
bool isPoint(const Part::Geometry&);

bool isCircleOrArc(const Part::Geometry& geo);

std::tuple<double, Base::Vector3d> getRadiusCenterCircleArc(const Part::Geometry* geo);

}  // namespace Sketcher

namespace SketcherGui
{
class DrawSketchHandler;
class ViewProviderSketch;

enum OffsetMode : bool
{
    NoOffset = false,
    OffsetConstraint = true
};

// to improve readability, expose the enum cases directly in the namespace
using enum OffsetMode;

/// This function tries to auto-recompute the active document if the option
/// is set in the user parameter. If the option is not set nothing will be done
/// @return true if a recompute was undertaken, false if not.
bool tryAutoRecompute(Sketcher::SketchObject* obj);
/// Same as the other overload, but also returns whether redundants shall be removed or not
bool tryAutoRecompute(Sketcher::SketchObject* obj, bool& autoremoveredundants);

/// This function tries to auto-recompute as tryAutoRecompute. If tryAutoRecompute
/// is not enabled, then it solves the SketchObject.
void tryAutoRecomputeIfNotSolve(Sketcher::SketchObject* obj);

// Recomputes and closes a transaction, then resets the transaction id
void closeAndRecompute(int& tid, bool abort, Sketcher::SketchObject* Obj);

/// Release any currently-active handler for the document.
/// Returns true if a handler was released, and false if not
bool ReleaseHandler(Gui::Document* doc);

std::string getStrippedPythonExceptionString(const Base::Exception&);

void getIdsFromName(
    std::string_view name,
    const Sketcher::SketchObject* obj,
    int& geoId,
    Sketcher::PointPos& posId
);

/// Returns ONLY the geometry elements when the "Edge" is selected (including GeomPoints)
std::vector<int> getGeoIdsOfEdgesFromNames(
    const Sketcher::SketchObject* Obj,
    const std::vector<std::string>& names
);

bool checkBothExternal(int GeoId1, int GeoId2);

bool isPointOrSegmentFixed(const Sketcher::SketchObject* Obj, int GeoId);

bool areBothPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2);

bool areAllPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2, int GeoId3);

bool isSimpleVertex(const Sketcher::SketchObject* Obj, int GeoId, Sketcher::PointPos PosId);

/// Checks if `GeoId` corresponds to a B-Spline knot
bool isBsplineKnot(const Sketcher::SketchObject* Obj, int GeoId);
/// Checks if the (`GeoId`, `PosId`) pair corresponds to a B-Spline knot, including first and last
/// knots
bool isBsplineKnotOrEndPoint(const Sketcher::SketchObject* Obj, int GeoId, Sketcher::PointPos PosId);

bool IsPointAlreadyOnCurve(
    int GeoIdCurve,
    int GeoIdPoint,
    Sketcher::PointPos PosIdPoint,
    Sketcher::SketchObject* Obj
);

bool isBsplinePole(const Part::Geometry* geo);

bool isBsplinePole(const Sketcher::SketchObject* Obj, int GeoId);

/// Checks whether there is a constraint of the given type with a First element geoid and a FirstPos
/// PosId
bool checkConstraint(
    const std::vector<Sketcher::Constraint*>& vals,
    Sketcher::ConstraintType type,
    int geoid,
    Sketcher::PointPos pos
);

inline namespace
{
using namespace std::string_view_literals;

namespace
{
inline std::optional<int> extractId(size_t prefixLength, std::string_view name, int add, bool neg = false)
{
    const auto digits = name.substr(prefixLength);
    int id = 0;
    const auto [ptr, ec] = std::from_chars(digits.begin(), digits.end(), id);
    if (ec == std::errc()) {
        return (neg ? -id : id) + add;
    }
    return std::nullopt;
}
}  // namespace

constexpr bool isVertex(int geoId, Sketcher::PointPos posId)
{
    return geoId != Sketcher::GeoEnum::GeoUndef && posId != Sketcher::PointPos::none;
}

/** Check if a sketcher name belongs to a vertex. */
constexpr bool isVertex(std::string_view name)
{
    return name.starts_with("Vertex"sv);
}

/**
 * @brief Get the ID of a vertex from its name.
 * @returns Vertex ID, or `std::nullopt` if the ID failed to parse or isn't from a vertex.
 */
inline std::optional<int> getVertexId(std::string_view name)
{
    return isVertex(name) ? extractId("Vertex"sv.length(), name, -1) : std::nullopt;
}

constexpr bool isEdge(int geoId, Sketcher::PointPos posId)
{
    return geoId != Sketcher::GeoEnum::GeoUndef && posId == Sketcher::PointPos::none;
}

/** Check if a sketcher name belongs to an edge. */
constexpr bool isEdge(std::string_view name)
{
    return name.starts_with("Edge"sv);
}

/**
 * @brief Get the ID of an edge from its name.
 * @returns Edge ID, or `std::nullopt` if the ID failed to parse or isn't from an edge.
 */
inline std::optional<int> getEdgeId(std::string_view name)
{
    return isEdge(name) ? extractId("Edge"sv.length(), name, -1) : std::nullopt;
}

/** Check if a sketcher name belongs to an external edge. */
constexpr bool isExternalEdge(std::string_view name)
{
    return name.starts_with("ExternalEdge"sv);
}

/**
 * @brief Get the ID of an external edge from its name.
 * @returns External edge ID, or `std::nullopt` if the ID failed to parse or isn't from
 *          an external edge.
 */
inline std::optional<int> getExternalEdgeId(std::string_view name)
{
    return isExternalEdge(name)
        ? extractId("ExternalEdge"sv.length(), name, Sketcher::GeoEnum::RefExt + 1, true)
        : std::nullopt;
}

/** Check if a sketcher name belongs to a constraint. */
constexpr bool isConstraint(std::string_view name)
{
    return name.starts_with("Constraint"sv);
}

/**
 * @brief Get the ID of a constraint from its name.
 * @returns Constraint ID, or `std::nullopt` if the ID failed to parse or isn't from a constraint.
 */
inline std::optional<int> getConstraintId(std::string_view name)
{
    return isConstraint(name) ? extractId("Constraint"sv.length(), name, -1) : std::nullopt;
}

}  // namespace

/* helper functions ======================================================*/

// Return counter-clockwise angle from horizontal out of p1 to p2 in radians.
double GetPointAngle(const Base::Vector2d& p1, const Base::Vector2d& p2);

// Set the two points on circles at minimal distance
void GetCirclesMinimalDistance(
    const Part::Geometry* geom1,
    const Part::Geometry* geom2,
    Base::Vector3d& point1,
    Base::Vector3d& point2
);

void ActivateHandler(Gui::Document* doc, std::unique_ptr<DrawSketchHandler> handler);

/// Returns if a sketch is in edit mode
bool isSketchInEdit(Gui::Document* doc);

/// Returns whether an edit mode command should be activated or not. It is only activated if the
/// sketcher is no special state or a sketchHandler is active.
bool isCommandActive(Gui::Document* doc);
bool isCommandNeedingConstraintActive(Gui::Document* doc);
bool isCommandNeedingGeometryActive(Gui::Document* doc);
bool isCommandNeedingBSplineActive(Gui::Document* doc);

SketcherGui::ViewProviderSketch* getInactiveHandlerEditModeSketchViewProvider(Gui::Document* doc);

SketcherGui::ViewProviderSketch* getInactiveHandlerEditModeSketchViewProvider();

void removeRedundantHorizontalVertical(
    Sketcher::SketchObject* psketch,
    std::vector<AutoConstraint>& sug1,
    std::vector<AutoConstraint>& sug2
);

void ConstraintToAttachment(
    Sketcher::GeoElementId element,
    Sketcher::GeoElementId attachment,
    double distance,
    App::DocumentObject* obj
);

void ConstraintLineByAngle(int geoId, double angle, App::DocumentObject* obj);
void Constraint2LinesByAngle(int geoId1, int geoId2, double angle, App::DocumentObject* obj);

// convenience functions for cursor coordinates
bool hideUnits();
bool showCursorCoords();
bool useSystemDecimals();
std::string lengthToDisplayFormat(double value, int digits);
std::string angleToDisplayFormat(double value, int digits);

bool areCollinear(const Base::Vector2d& p1, const Base::Vector2d& p2, const Base::Vector2d& p3);

// Returns the index of the element in the vector, GeoUndef if the element is GeoUndef and -1 if not
// found
int indexOfGeoId(const std::vector<int>& vec, int elem);

inline void scrollTo(QListWidget* list, int i, bool select)
{
    if (select && list->model()) {  // scrollTo only on select, not de-select
        QModelIndex index = list->model()->index(i, 0);
        list->scrollTo(index, QAbstractItemView::PositionAtCenter);
    }
}

QMap<QString, QString> findAvailableFontFiles();

}  // namespace SketcherGui

/// converts a 2D vector into a 3D vector in the XY plane
inline Base::Vector3d toVector3d(const Base::Vector2d& vector2d)
{
    return Base::Vector3d(vector2d.x, vector2d.y, 0.);
}

/// converts a 3D vector in the XY plane into a 2D vector
inline Base::Vector2d toVector2d(const Base::Vector3d& vector3d)
{
    return Base::Vector2d(vector3d.x, vector3d.y);
}


template<typename T>
auto toPointerVector(const std::vector<std::unique_ptr<T>>& vector)
{
    std::vector<T*> vp(vector.size());

    std::transform(vector.begin(), vector.end(), vp.begin(), [](auto& p) { return p.get(); });

    return vp;
}

/** returns the visual layer id (not the one of the GeometryFacade, but the index to
 * PropertyVisualLayerList) from a geometry or GeometryFacade. NOTE: If the geometry or
 * geometryfacade does not have a corresponding ViewProviderSketchGeometryExtension, the default
 * layer (layer 0) is returned.
 * */
template<typename T>
auto getSafeGeomLayerId(T geom)
{
    int layerId = 0;

    if (geom->hasExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId())) {
        auto vpext = std::static_pointer_cast<const SketcherGui::ViewProviderSketchGeometryExtension>(
            geom->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()).lock()
        );

        layerId = vpext->getVisualLayerId();
    }

    return layerId;
}

/** sets the visual layer id (not the one of the GeometryFacade, but the index to
 * PropertyVisualLayerList) for a geometry or GeometryFacade. NOTE: If no
 * ViewProviderSketchGeometryExtension is present, one is created.
 * */
template<typename T>
void setSafeGeomLayerId(T geom, int layerindex)
{
    // create extension if none existing
    if (!geom->hasExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId())) {
        geom->setExtension(std::make_unique<SketcherGui::ViewProviderSketchGeometryExtension>());
    }

    auto vpext = std::static_pointer_cast<SketcherGui::ViewProviderSketchGeometryExtension>(
        geom->getExtension(SketcherGui::ViewProviderSketchGeometryExtension::getClassTypeId()).lock()
    );

    vpext->setVisualLayerId(layerindex);
}
