// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 Andrew Shkolik <shkolik@gmail.com>                  *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRepAdaptor_Curve.hxx>
# include <BRepBuilderAPI_MakeEdge.hxx>
# include <Standard_Version.hxx>
# include <TopoDS.hxx>
# include <gp_Pnt.hxx>
#endif

#include "occ_gordon_single.hpp"

#include <GeomConvert_CompCurveToBSplineCurve.hxx>

#if OCC_VERSION_HEX >= 0x080000
# include <GeomFill_Gordon.hxx>
#endif

#include "FeatureGordonSurface.h"

using namespace Surface;

PROPERTY_SOURCE(Surface::GordonSurface, Part::Spline)

GordonSurface::GordonSurface()
{
    ADD_PROPERTY_TYPE(ProfileEdges, (nullptr, ""), "GordonSurface", App::Prop_None, "Profiles edges.");
    ADD_PROPERTY_TYPE(GuideEdges, (nullptr, ""), "GordonSurface", App::Prop_None, "Guide edges.");
    ADD_PROPERTY_TYPE(ProfileDirections, (false), "GordonSurface", App::Prop_None, "Profile directions.");
    ADD_PROPERTY_TYPE(GuideDirections, (false), "GordonSurface", App::Prop_None, "Guide directions.");
    ADD_PROPERTY_TYPE(Tolerance, (1.e-4), "GordonSurface", App::Prop_None, "Tolerance");

    ADD_PROPERTY_TYPE(
        UseNativeAlgorithm,
        (false),
        "Native",
        App::Prop_None,
        QT_TR_NOOP("Use native OCCT Gordon algorithm")
    );
    ADD_PROPERTY_TYPE(
        ParallelMode,
        (true),
        "Native",
        App::Prop_None,
        QT_TR_NOOP("Enable parallel processing")
    );
    ADD_PROPERTY_TYPE(
        ApproximationMode,
        (0L),
        "Native",
        App::Prop_None,
        QT_TR_NOOP("Fallback behavior when exact construction fails")
    );

    static const char* approxEnums[] = {"Exact construction", "Allow approximation", nullptr};
    ApproximationMode.setEnums(approxEnums);
    ParallelMode.setStatus(App::Property::Status::Hidden, true);
    ApproximationMode.setStatus(App::Property::Status::Hidden, true);

    ProfileEdges.setScope(App::LinkScope::Global);
    GuideEdges.setScope(App::LinkScope::Global);

    ProfileEdges.setSize(0);
    GuideEdges.setSize(0);
    ProfileDirections.setSize(0);
    GuideDirections.setSize(0);
    Tolerance.setValue(1.e-4);
}

short GordonSurface::mustExecute() const
{
    if (ProfileEdges.isTouched() || GuideEdges.isTouched() || ProfileDirections.isTouched()
        || GuideDirections.isTouched() || Tolerance.isTouched() || UseNativeAlgorithm.isTouched()
        || ParallelMode.isTouched() || ApproximationMode.isTouched()) {
        return 1;
    }
    return 0;
}


#if OCC_VERSION_HEX >= 0x080000
static const char* gordonStatusToString(GeomFill_Gordon::ResultStatus status)
{
    switch (status) {
        case GeomFill_Gordon::ResultStatus::NotStarted:
            return "Not started";
        case GeomFill_Gordon::ResultStatus::Done:
            return "Success";
        case GeomFill_Gordon::ResultStatus::InvalidInput:
            return "Too few profile or guide curves (need at least 2 each)";
        case GeomFill_Gordon::ResultStatus::ConversionFailed:
            return "Curve conversion to B-spline failed";
        case GeomFill_Gordon::ResultStatus::IntersectionFailed:
            return "Could not build full profile/guide intersection table";
        case GeomFill_Gordon::ResultStatus::OrderingFailed:
            return "Network curves could not be ordered consistently";
        case GeomFill_Gordon::ResultStatus::ReparametrizationFailed:
            return "Intersection parameters could not be equalized";
        case GeomFill_Gordon::ResultStatus::CompatibilityFailed:
            return "Network failed geometric compatibility checks";
        case GeomFill_Gordon::ResultStatus::CurveCompatibilityFailed:
            return "Curve families are not B-spline compatible";
        case GeomFill_Gordon::ResultStatus::RationalReparametrizationFailed:
            return "Rational curves require unsupported exact reparametrization";
        case GeomFill_Gordon::ResultStatus::SkinningFailed:
            return "Intermediate profile/guide skinning failed";
        case GeomFill_Gordon::ResultStatus::ReferenceSurfaceFailed:
            return "Intersection-grid reference surface failed";
        case GeomFill_Gordon::ResultStatus::KnotAlignmentFailed:
            return "Intermediate surfaces could not be aligned";
        case GeomFill_Gordon::ResultStatus::RationalDegreeOverflow:
            return "Rational product degree exceeds OCCT B-spline limit";
        case GeomFill_Gordon::ResultStatus::RationalConstructionFailed:
            return "Rational numerator/denominator construction failed";
        case GeomFill_Gordon::ResultStatus::PeriodicityFailed:
            return "Closed seam could not be converted to periodic form";
        case GeomFill_Gordon::ResultStatus::ApproximationFailed:
            return "Approximate fallback failed";
        case GeomFill_Gordon::ResultStatus::ConstructionFailed:
            return "Final B-spline surface construction failed";
        default:
            return "Unknown error";
    }
}
#endif

void GordonSurface::onChanged(const App::Property* prop)
{
    if (prop == &UseNativeAlgorithm) {
        bool hidden = !UseNativeAlgorithm.getValue();
        ParallelMode.setStatus(App::Property::Status::Hidden, hidden);
        ApproximationMode.setStatus(App::Property::Status::Hidden, hidden);
    }
    Part::Spline::onChanged(prop);
}

std::vector<Handle(Geom_BSplineCurve)> GordonSurface::getCurves(
    const App::PropertyLinkSubList& edges,
    const App::PropertyBoolList& directions
)
{
    std::vector<Handle(Geom_BSplineCurve)> curves;

    auto objects = edges.getValues();
    auto subNames = edges.getSubValues();
    auto dirs = directions.getValues();

    if (objects.size() != dirs.size() || objects.size() != subNames.size()
        || dirs.size() != subNames.size()) {
        throw Standard_Failure(
            std::format(
                "Inconsistent number of edges ({}), sub-shapes ({}), and directions ({}).",
                objects.size(),
                subNames.size(),
                dirs.size()
            )
                .c_str()
        );
    }

    for (std::size_t i = 0; i < objects.size(); i++) {
        App::DocumentObject* obj = objects[i];
        std::string sub = subNames[i];
        if (obj && obj->isDerivedFrom<Part::Feature>()) {
            // get the sub-edge of the part's shape and copy it to nat make changes to original geometry
            const Part::TopoShape& shape
                = static_cast<Part::Feature*>(obj)->Shape.getShape().makeElementCopy();
            TopoDS_Shape edgeShape = shape.getSubShape(sub.c_str());
            if (edgeShape.IsNull() && shape.shapeType() == TopAbs_EDGE) {
                // Single-edge shape — no sub-element naming, use directly
                edgeShape = shape.getShape();
            }
            if (!edgeShape.IsNull() && edgeShape.ShapeType() == TopAbs_EDGE) {
                double u1, u2;
                const TopoDS_Edge& edge = TopoDS::Edge(edgeShape);
                TopLoc_Location heloc;  // this will be output curve location
                Handle(Geom_Curve) c_geom = BRep_Tool::Curve(edge, heloc, u1, u2);  // The geometric
                                                                                    // curve

                ShapeConstruct_Curve scc;
                Handle(Geom_BSplineCurve)
                    bspline = scc.ConvertToBSpline(c_geom, u1, u2, Precision::Confusion());

                if (bspline.IsNull()) {
                    throw Standard_Failure(
                        "A curve was not a B-spline and could not be converted into one."
                    );
                }
                if (dirs[i]) {
                    bspline->Reverse();
                }

                bspline->Transform(heloc.Transformation());  // apply original transformation to
                                                             // control points

                curves.emplace_back(bspline);
            }
            else {
                throw Standard_Failure("Sub-shape is not an edge");
            }
        }
    }

    return curves;
}


Handle(
    Geom_BSplineCurve
) GordonSurface::moveCurveSeam(const Handle(Geom_BSplineCurve) & curve, double targetParameter) const
{
    if (curve.IsNull() || !curve->IsClosed()) {
        throw Standard_Failure("Curve must be closed.");
    }

    Handle(Geom_BSplineCurve) result = Handle(Geom_BSplineCurve)::DownCast(curve->Copy());

    // Verify that the current seam is at least G1 continuous.
    double first = result->FirstParameter();
    double last = result->LastParameter();

    gp_Pnt pFirst, pLast;
    gp_Vec dFirst, dLast;

    result->D1(first, pFirst, dFirst);
    result->D1(last, pLast, dLast);

    // redundand check for closedness
    if (pFirst.Distance(pLast) > Precision::Intersection()) {
        throw Standard_Failure("Closed BSpline endpoints do not coincide.");
    }

    // Check for G1 continuity at the seam.
    if (dFirst.SquareMagnitude() <= Precision::SquareConfusion()
        || dLast.SquareMagnitude() <= Precision::SquareConfusion()) {
        throw Standard_Failure("Cannot determine tangent direction at BSpline seam.");
    }

    dFirst.Normalize();
    dLast.Normalize();

    if (dFirst.Dot(dLast) < 1.0 - Precision::Intersection()) {
        throw Standard_Failure("Cannot move seam of BSpline with tangent discontinuity.");
    }

    // Store target point before converting to periodic curve.
    // SetPeriodic() changes the parameterization.
    gp_Pnt targetPoint;

    bool isPeriodic = result->IsPeriodic();
    // Convert clamped closed representation to periodic.
    if (!isPeriodic) {
        curve->D0(targetParameter, targetPoint);
        result->SetPeriodic();

        // Find the corresponding parameter in the new representation.
        GeomAPI_ProjectPointOnCurve projector(targetPoint, result);

        if (projector.NbPoints() == 0) {
            throw Standard_Failure("Cannot find target parameter after periodic conversion.");
        }

        targetParameter = projector.LowerDistanceParameter();
        // Normalize parameter into the periodic range.
        double newFirst = result->FirstParameter();
        double newLast = result->LastParameter();
        double period = newLast - newFirst;

        while (targetParameter < newFirst) {
            targetParameter += period;
        }
        while (targetParameter > newLast) {
            targetParameter -= period;
        }
    }

    // move seam
    result->SetOrigin(targetParameter, Precision::Confusion());

    return result;
}


void GordonSurface::prepareCurvesNetwork(
    std::vector<Handle(Geom_BSplineCurve)>& profiles,
    std::vector<Handle(Geom_BSplineCurve)>& guides,
    bool addSeamCopy
) const
{
    bool profilesClosed = !profiles.empty()
        && std::all_of(profiles.begin(), profiles.end(), [](auto& c) { return c->IsClosed(); });

    bool guidesClosed = !guides.empty()
        && std::all_of(guides.begin(), guides.end(), [](auto& c) { return c->IsClosed(); });

    if (std::any_of(profiles.begin(), profiles.end(), [](auto& c) { return c->IsClosed(); })
        != profilesClosed) {
        throw Standard_Failure("Inconsistent closed state in profiles.");
    }
    if (std::any_of(guides.begin(), guides.end(), [](auto& c) { return c->IsClosed(); })
        != guidesClosed) {
        throw Standard_Failure("Inconsistent closed state in guides.");
    }

    if (profilesClosed && guidesClosed) {
        throw Standard_Failure("Gordon surface closed in both directions is not supported.");
    }

    // Verify curve network intersections
    double tol = Tolerance.getValue();
    for (size_t i = 0; i < profiles.size(); ++i) {
        for (size_t j = 0; j < guides.size(); ++j) {
            auto hits
                = occ_gordon_internal::BSplineAlgorithms::intersections(profiles[i], guides[j], tol);
            if (hits.empty()) {
                throw Standard_Failure(
                    std::format("Profile {} does not intersect Guide {}", i + 1, j + 1).c_str()
                );
            }
        }
    }

    // Nothing to do more if network is open both directions
    if (!profilesClosed && !guidesClosed) {
        return;
    }

    // check if the seam of the closed curves aligns with any curve of the open cuves
    std::vector<Handle(Geom_BSplineCurve)>& closedCurves = profilesClosed ? profiles : guides;
    std::vector<Handle(Geom_BSplineCurve)>& openCurves = profilesClosed ? guides : profiles;

    // find the best seam curve from the open curves that aligns with the most closed curves
    size_t bestSeamIdx = 0;
    int bestCount = -1;

    for (size_t j = 0; j < openCurves.size(); ++j) {
        int count = 0;
        for (size_t i = 0; i < closedCurves.size(); ++i) {
            gp_Pnt closedSeam = closedCurves[i]->Value(closedCurves[i]->FirstParameter());
            auto hits = occ_gordon_internal::BSplineAlgorithms::intersections(
                closedCurves[i],
                openCurves[j],
                tol
            );
            for (const auto& hit : hits) {
                gp_Pnt ptOnOpen = openCurves[j]->Value(hit.second);
                if (ptOnOpen.Distance(closedSeam) < tol) {
                    ++count;
                    break;
                }
            }
        }
        if (count > bestCount) {
            bestCount = count;
            bestSeamIdx = j;
        }
    }

    Handle(Geom_BSplineCurve) seamCurve = openCurves[bestSeamIdx];

    // shift closed curves whose seam doesn't align with the seam curve
    for (size_t i = 0; i < closedCurves.size(); ++i) {
        gp_Pnt closedSeam = closedCurves[i]->Value(closedCurves[i]->FirstParameter());
        auto hits
            = occ_gordon_internal::BSplineAlgorithms::intersections(closedCurves[i], seamCurve, tol);

        if (hits.empty()) {
            throw Standard_Failure("Closed curve does not intersect selected seam curve.");
        }

        bool aligned = false;
        for (const auto& hit : hits) {
            gp_Pnt ptOnOpen = seamCurve->Value(hit.second);
            if (ptOnOpen.Distance(closedSeam) < tol) {
                aligned = true;  // already aligned, no need to shift
                break;
            }
        }

        if (!aligned) {
            // target parameter on the closed curve to move the seam to
            double t = profilesClosed ? hits[0].first : hits[0].second;

            closedCurves[i] = moveCurveSeam(closedCurves[i], t);
        }
    }

    // duplicate seam curve at the end - OCCT cannot handle it internally
    if (addSeamCopy) {
        auto copy = Handle(Geom_BSplineCurve)::DownCast(seamCurve->Copy());
        openCurves.push_back(copy);
    }
}

App::DocumentObjectExecReturn* GordonSurface::execute()
{
    try {
        if ((ProfileEdges.getSize()) < 2) {
            throw Standard_Failure("Provide at least 2 profiles.");
        }
        if ((GuideEdges.getSize()) < 2) {
            throw Standard_Failure("Provide at least 2 guides.");
        }

        std::vector<Handle(Geom_BSplineCurve)> vcurves, ucurves;

        ucurves = getCurves(ProfileEdges, ProfileDirections);
        vcurves = getCurves(GuideEdges, GuideDirections);

        // there is no reason to go under 1e-7 precision
        double tol = Tolerance.getValue() < Precision::Confusion() ? Precision::Confusion()
                                                                   : Tolerance.getValue();
        Handle(Geom_BSplineSurface) surface;

        // Prepare curves for both algorithms: validate intersections, align seam for closed curves
        prepareCurvesNetwork(ucurves, vcurves, UseNativeAlgorithm.getValue());

#if OCC_VERSION_HEX >= 0x080000
        if (UseNativeAlgorithm.getValue()) {

            NCollection_Array1<Handle(Geom_Curve)> uarray(
                1,
                static_cast<Standard_Integer>(ucurves.size())
            );
            NCollection_Array1<Handle(Geom_Curve)> varray(
                1,
                static_cast<Standard_Integer>(vcurves.size())
            );
            for (std::size_t i = 0; i < ucurves.size(); ++i) {
                uarray.SetValue(static_cast<Standard_Integer>(i) + 1, ucurves[i]);
            }
            for (std::size_t i = 0; i < vcurves.size(); ++i) {
                varray.SetValue(static_cast<Standard_Integer>(i) + 1, vcurves[i]);
            }

            GeomFill_Gordon gordon;
            gordon.Init(uarray, varray, tol);
            gordon.SetParallelMode(ParallelMode.getValue());

            const char* modeStr = ApproximationMode.getValueAsString();
            if (strcmp(modeStr, "Allow approximation") == 0) {
                gordon.SetApproximationMode(
                    GeomFill_Gordon::ApproximationMode::AllowApproximateFallback
                );
            }
            else {
                gordon.SetApproximationMode(GeomFill_Gordon::ApproximationMode::ExactOnly);
            }

            gordon.Perform();
            if (!gordon.IsDone()) {
                throw Standard_Failure(
                    std::format(
                        "Failed to build Gordon surface: {}",
                        gordonStatusToString(gordon.Status())
                    )
                        .c_str()
                );
            }
            surface = gordon.Surface();
        }
        else
#endif
        {
            surface = occ_gordon::interpolate_curve_network(ucurves, vcurves, tol);
        }

        if (surface.IsNull()) {
            throw Standard_Failure("Failed to create a Gordon surface.");
        }

        // Create a face from the BSpline surface
        BRepBuilderAPI_MakeFace faceMaker(surface, Precision::Confusion());
        if (!faceMaker.IsDone()) {
            throw Standard_Failure("Failed to create a face from the BSpline surface.");
        }

        this->Shape.setValue(faceMaker.Face());
        return App::DocumentObject::StdReturn;
    }
    catch (const Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

void GordonSurface::onDocumentRestored()
{
    // init ProfileDirections and GuideDirections if not exists
    if (ProfileDirections.getSize() != ProfileEdges.getSize()) {
        ProfileDirections.setSize(ProfileEdges.getSize());
        for (std::size_t i = 0; i < ProfileDirections.getSize(); ++i) {
            ProfileDirections.set1Value(i, false);
        }
    }
    if (GuideDirections.getSize() != GuideEdges.getSize()) {
        GuideDirections.setSize(GuideEdges.getSize());
        for (std::size_t i = 0; i < GuideDirections.getSize(); ++i) {
            GuideDirections.set1Value(i, false);
        }
    }

    Part::Spline::onDocumentRestored();
}
