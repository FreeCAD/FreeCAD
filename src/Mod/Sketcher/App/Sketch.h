/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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

#ifndef SKETCHER_SKETCH_H
#define SKETCHER_SKETCH_H

#include <Base/Persistence.h>
#include <CXX/Objects.hxx>
#include <Mod/Part/App/TopoShape.h>

#include "Constraint.h"
#include "GeoList.h"
#include "planegcs/GCS.h"


namespace Sketcher
{
// Forward declarations
class SolverGeometryExtension;

class SketcherExport Sketch: public Base::Persistence
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    Sketch();
    ~Sketch() override;

    // from base class
    unsigned int getMemSize() const override;
    void Save(Base::Writer& /*writer*/) const override;
    void Restore(Base::XMLReader& /*reader*/) override;

    /// solve the actual set up sketch
    int solve();
    /// resets the solver
    int resetSolver();
    /// get standard (aka fine) solver precision
    double getSolverPrecision()
    {
        return GCSsys.getFinePrecision();
    }
    /// delete all geometry and constraints, leave an empty sketch
    void clear();
    /** set the sketch up with geoms and constraints
     *
     * returns the degree of freedom of a sketch and calculates a list of
     * conflicting constraints
     *
     * 0 degrees of freedom correspond to a fully constrained sketch
     * -1 degrees of freedom correspond to an over-constrained sketch
     * positive degrees of freedom correspond to an under-constrained sketch
     *
     * an over-constrained sketch will always contain conflicting constraints
     * a fully constrained or under-constrained sketch may contain conflicting
     * constraints or may not
     */
    int setUpSketch(const std::vector<Part::Geometry*>& GeoList,
                    const std::vector<Constraint*>& ConstraintList,
                    int extGeoCount = 0);
    /// return the actual geometry of the sketch a TopoShape
    Part::TopoShape toShape() const;
    /// add unspecified geometry
    int addGeometry(const Part::Geometry* geo, bool fixed = false);
    /// add unspecified geometry
    int addGeometry(const std::vector<Part::Geometry*>& geo, bool fixed = false);
    /// add unspecified geometry, where each element's "fixed" status is given by the
    /// blockedGeometry array
    int addGeometry(const std::vector<Part::Geometry*>& geo,
                    const std::vector<bool>& blockedGeometry);
    /// get boolean list indicating whether the geometry is to be blocked or not
    void getBlockedGeometry(std::vector<bool>& blockedGeometry,
                            std::vector<bool>& unenforceableConstraints,
                            const std::vector<Constraint*>& ConstraintList) const;
    /// returns the actual geometry
    std::vector<Part::Geometry*> extractGeometry(bool withConstructionElements = true,
                                                 bool withExternalElements = false) const;

    GeoListFacade extractGeoListFacade() const;

    void updateExtension(int geoId, std::unique_ptr<Part::GeometryExtension>&& ext);
    /// get the geometry as python objects
    Py::Tuple getPyGeometry() const;

    /// retrieves the index of a point
    int getPointId(int geoId, PointPos pos) const;
    /// retrieves a point
    Base::Vector3d getPoint(int geoId, PointPos pos) const;

    // Inline methods
    inline bool hasConflicts() const
    {
        return !Conflicting.empty();
    }
    inline const std::vector<int>& getConflicting() const
    {
        return Conflicting;
    }
    inline bool hasRedundancies() const
    {
        return !Redundant.empty();
    }
    inline const std::vector<int>& getRedundant() const
    {
        return Redundant;
    }
    inline bool hasPartialRedundancies() const
    {
        return !PartiallyRedundant.empty();
    }
    inline const std::vector<int>& getPartiallyRedundant() const
    {
        return PartiallyRedundant;
    }

    inline float getSolveTime() const
    {
        return SolveTime;
    }

    inline bool hasMalformedConstraints() const
    {
        return !MalformedConstraints.empty();
    }
    inline const std::vector<int>& getMalformedConstraints() const
    {
        return MalformedConstraints;
    }

public:
    std::set<std::pair<int, Sketcher::PointPos>> getDependencyGroup(int geoId, PointPos pos) const;

    std::shared_ptr<SolverGeometryExtension> getSolverExtension(int geoId) const;


public:
    /** set the datum of a distance or angle constraint to a certain value and solve
     * This can cause the solving to fail!
     */
    int setDatum(int constrId, double value);

    /** initializes a point (or curve) drag by setting the current
     * sketch status as a reference
     */
    int initMove(int geoId, PointPos pos, bool fine = true);

    /** Initializes a B-spline piece drag by setting the current
     * sketch status as a reference. Only moves piece around `firstPoint`.
     */
    int initBSplinePieceMove(int geoId,
                             PointPos pos,
                             const Base::Vector3d& firstPoint,
                             bool fine = true);

    /** Resets the initialization of a point or curve drag
     */
    void resetInitMove();

    /** Limits a b-spline drag to the segment around `firstPoint`.
     */
    int limitBSplineMove(int geoId, PointPos pos, const Base::Vector3d& firstPoint);

    /** move this point (or curve) to a new location and solve.
     * This will introduce some additional weak constraints expressing
     * a condition for satisfying the new point location!
     * The relative flag permits moving relatively to the current position
     */
    int movePoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative = false);

    /**
     * Sets whether the initial solution should be recalculated while dragging after a certain
     * distance from the previous drag point for smoother dragging operation.
     */
    bool getRecalculateInitialSolutionWhileMovingPoint() const
    {
        return RecalculateInitialSolutionWhileMovingPoint;
    }

    void
    setRecalculateInitialSolutionWhileMovingPoint(bool recalculateInitialSolutionWhileMovingPoint)
    {
        RecalculateInitialSolutionWhileMovingPoint = recalculateInitialSolutionWhileMovingPoint;
    }

    /// add dedicated geometry
    //@{
    /// add a point
    int addPoint(const Part::GeomPoint& point, bool fixed = false);
    /// add an infinite line
    int addLine(const Part::GeomLineSegment& line, bool fixed = false);
    /// add a line segment
    int addLineSegment(const Part::GeomLineSegment& lineSegment, bool fixed = false);
    /// add a arc (circle segment)
    int addArc(const Part::GeomArcOfCircle& circleSegment, bool fixed = false);
    /// add a circle
    int addCircle(const Part::GeomCircle& circle, bool fixed = false);
    /// add an ellipse
    int addEllipse(const Part::GeomEllipse& ellipse, bool fixed = false);
    /// add an arc of ellipse
    int addArcOfEllipse(const Part::GeomArcOfEllipse& ellipseSegment, bool fixed = false);
    /// add an arc of hyperbola
    int addArcOfHyperbola(const Part::GeomArcOfHyperbola& hyperbolaSegment, bool fixed = false);
    /// add an arc of parabola
    int addArcOfParabola(const Part::GeomArcOfParabola& parabolaSegment, bool fixed = false);
    /// add a BSpline
    int addBSpline(const Part::GeomBSplineCurve& spline, bool fixed = false);
    //@}


    /// constraints
    //@{
    /// add all constraints in the list
    int addConstraints(const std::vector<Constraint*>& ConstraintList);
    /// add all constraints in the list, provided that are enforceable
    int addConstraints(const std::vector<Constraint*>& ConstraintList,
                       const std::vector<bool>& unenforceableConstraints);
    /// add one constraint to the sketch
    int addConstraint(const Constraint* constraint);

    /**
     *   add a fixed X coordinate constraint to a point
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addCoordinateXConstraint(int geoId, PointPos pos, double* value, bool driving = true);
    /**
     *   add a fixed Y coordinate constraint to a point
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addCoordinateYConstraint(int geoId, PointPos pos, double* value, bool driving = true);
    /**
     *   add a horizontal distance constraint to two points or line ends
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addDistanceXConstraint(int geoId, double* value, bool driving = true);
    /**
     *   add a horizontal distance constraint to two points or line ends
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addDistanceXConstraint(int geoId1,
                               PointPos pos1,
                               int geoId2,
                               PointPos pos2,
                               double* value,
                               bool driving = true);
    /**
     *   add a vertical distance constraint to two points or line ends
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addDistanceYConstraint(int geoId, double* value, bool driving = true);
    /**
     *   add a vertical distance constraint to two points or line ends
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addDistanceYConstraint(int geoId1,
                               PointPos pos1,
                               int geoId2,
                               PointPos pos2,
                               double* value,
                               bool driving = true);
    /// add a horizontal constraint to a geometry
    int addHorizontalConstraint(int geoId);
    int addHorizontalConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2);
    /// add a vertical constraint to a geometry
    int addVerticalConstraint(int geoId);
    int addVerticalConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2);
    /// add a coincident constraint to two points of two geometries
    int addPointCoincidentConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2);
    /**
     *   add a length or distance constraint
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addDistanceConstraint(int geoId1, double* value, bool driving = true);
    /**
     *   add a length or distance constraint
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addDistanceConstraint(int geoId1,
                              PointPos pos1,
                              int geoId2,
                              double* value,
                              bool driving = true);
    /**
     *   add a length or distance constraint
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addDistanceConstraint(int geoId1,
                              PointPos pos1,
                              int geoId2,
                              PointPos pos2,
                              double* value,
                              bool driving = true);
    /**
     *   add a length or distance constraint
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addDistanceConstraint(int geoId1, int geoId2, double* value, bool driving = true);

    /// add a parallel constraint between two lines
    int addParallelConstraint(int geoId1, int geoId2);
    /// add a perpendicular constraint between two lines
    int addPerpendicularConstraint(int geoId1, int geoId2);
    /// add a tangency constraint between two geometries
    int addTangentConstraint(int geoId1, int geoId2);
    int addTangentLineAtBSplineKnotConstraint(int checkedlinegeoId,
                                              int checkedbsplinegeoId,
                                              int checkedknotgeoid);
    int addTangentLineEndpointAtBSplineKnotConstraint(int checkedlinegeoId,
                                                      PointPos endpointPos,
                                                      int checkedbsplinegeoId,
                                                      int checkedknotgeoid);
    int addAngleAtPointConstraint(int geoId1,
                                  PointPos pos1,
                                  int geoId2,
                                  PointPos pos2,
                                  int geoId3,
                                  PointPos pos3,
                                  double* value,
                                  ConstraintType cTyp,
                                  bool driving = true);
    /**
     *   add a radius constraint on a circle or an arc
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addRadiusConstraint(int geoId, double* value, bool driving = true);
    /**
     *   add a radius constraint on a circle or an arc
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addDiameterConstraint(int geoId, double* value, bool driving = true);
    /**
     *   add an angle constraint on a line or between two lines
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addAngleConstraint(int geoId, double* value, bool driving = true);
    /**
     *   add an angle constraint on a line or between two lines
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addAngleConstraint(int geoId1, int geoId2, double* value, bool driving = true);
    /**
     *   add an angle constraint on a line or between two lines
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addAngleConstraint(int geoId1,
                           PointPos pos1,
                           int geoId2,
                           PointPos pos2,
                           double* value,
                           bool driving = true);
    /**
     *   add angle-via-point constraint between any two curves
     *
     *   double * value is a pointer to double allocated in the heap, containing the
     *   constraint value and already inserted into either the FixParameters or
     *   Parameters array, as the case may be.
     */
    int addAngleViaPointConstraint(int geoId1,
                                   int geoId2,
                                   int geoId3,
                                   PointPos pos3,
                                   double value,
                                   bool driving = true);
    /// add an equal length or radius constraints between two lines or between circles and arcs
    int addEqualConstraint(int geoId1, int geoId2);
    /// add a point on line constraint
    int addPointOnObjectConstraint(int geoId1, PointPos pos1, int geoId2, bool driving = true);
    /// add a point on B-spline constraint: needs a parameter
    int addPointOnObjectConstraint(int geoId1,
                                   PointPos pos1,
                                   int geoId2,
                                   double* pointparam,
                                   bool driving = true);
    /// add a symmetric constraint between two points with respect to a line
    int addSymmetricConstraint(int geoId1, PointPos pos1, int geoId2, PointPos pos2, int geoId3);
    /// add a symmetric constraint between three points, the last point is in the middle of the
    /// first two
    int addSymmetricConstraint(int geoId1,
                               PointPos pos1,
                               int geoId2,
                               PointPos pos2,
                               int geoId3,
                               PointPos pos3);
    /**
     *   add a snell's law constraint
     *
     *   double * value and double * second are each a pointer to double
     *   allocated in the heap and already inserted into either the
     *   FixParameters or Parameters array, as the case may be.
     *
     *   value must contain the constraint value (the ratio of n2/n1)
     *   second may be initialized to any value, however the solver will
     *   provide n1 in value and n2 in second.
     */
    int addSnellsLawConstraint(int geoIdRay1,
                               PointPos posRay1,
                               int geoIdRay2,
                               PointPos posRay2,
                               int geoIdBnd,
                               double* value,
                               double* second,
                               bool driving = true);
    //@}

    /// Internal Alignment constraints
    //@{
    /// add InternalAlignmentEllipseMajorDiameter to a line and an ellipse
    int addInternalAlignmentEllipseMajorDiameter(int geoId1, int geoId2);
    int addInternalAlignmentEllipseMinorDiameter(int geoId1, int geoId2);
    int addInternalAlignmentEllipseFocus1(int geoId1, int geoId2);
    int addInternalAlignmentEllipseFocus2(int geoId1, int geoId2);
    /// add InternalAlignmentHyperbolaMajorRadius to a line and a hyperbola
    int addInternalAlignmentHyperbolaMajorDiameter(int geoId1, int geoId2);
    int addInternalAlignmentHyperbolaMinorDiameter(int geoId1, int geoId2);
    int addInternalAlignmentHyperbolaFocus(int geoId1, int geoId2);
    int addInternalAlignmentParabolaFocus(int geoId1, int geoId2);
    int addInternalAlignmentParabolaFocalDistance(int geoId1, int geoId2);
    int addInternalAlignmentBSplineControlPoint(int geoId1, int geoId2, int poleindex);
    int addInternalAlignmentKnotPoint(int geoId1, int geoId2, int knotindex);
    //@}
public:
    // This func is to be used during angle-via-point constraint creation. It calculates
    // the angle between geoId1,geoId2 at point px,py. The point should be on both curves,
    // otherwise the result will be systematically off (but smoothly approach the correct
    // value as the point approaches intersection of curves).
    double calculateAngleViaPoint(int geoId1, int geoId2, double px, double py);

    // This is to be used for rendering of angle-via-point constraint.
    Base::Vector3d calculateNormalAtPoint(int geoIdCurve, double px, double py) const;

    // icstr should be the value returned by addXXXXConstraint
    // see more info in respective function in GCS.
    double calculateConstraintError(int icstr)
    {
        return GCSsys.calculateConstraintErrorByTag(icstr);
    }

    /// Returns the size of the Geometry
    int getGeometrySize() const
    {
        return Geoms.size();
    }

    enum GeoType
    {
        None = 0,
        Point = 1,    // 1 Point(start), 2 Parameters(x,y)
        Line = 2,     // 2 Points(start,end), 4 Parameters(x1,y1,x2,y2)
        Arc = 3,      // 3 Points(start,end,mid), (4)+5 Parameters((x1,y1,x2,y2),x,y,r,a1,a2)
        Circle = 4,   // 1 Point(mid), 3 Parameters(x,y,r)
        Ellipse = 5,  // 1 Point(mid), 5 Parameters(x,y,r1,r2,phi)
                      // phi=angle xaxis of ellipse with respect of sketch xaxis
        ArcOfEllipse = 6,
        ArcOfHyperbola = 7,
        ArcOfParabola = 8,
        BSpline = 9
    };

protected:
    float SolveTime;
    bool RecalculateInitialSolutionWhileMovingPoint;

    // regulates a second solve for cases where there result of having update the geometry (e.g. via
    // OCCT) needs to be taken into account by the solver (for example to provide the right value of
    // non-driving constraints)
    bool resolveAfterGeometryUpdated;

protected:
    /// container element to store and work with the geometric elements of this sketch
    struct GeoDef
    {
        GeoDef()
            : geo(nullptr)
            , type(None)
            , external(false)
            , index(-1)
            , startPointId(-1)
            , midPointId(-1)
            , endPointId(-1)
        {}
        Part::Geometry* geo;  // pointer to the geometry
        GeoType type;         // type of the geometry
        bool external;        // flag for external geometries
        int index;         // index in the corresponding storage vector (Lines, Arcs, Circles, ...)
        int startPointId;  // index in Points of the start point of this geometry
        int midPointId;    // index in Points of the start point of this geometry
        int endPointId;    // index in Points of the end point of this geometry
    };
    /// container element to store and work with the constraints of this sketch
    struct ConstrDef
    {
        ConstrDef()
            : constr(nullptr)
            , driving(true)
            , value(nullptr)
            , secondvalue(nullptr)
        {}
        Constraint* constr;  // pointer to the constraint
        bool driving;
        double* value;
        double* secondvalue;  // this is needed for SnellsLaw
    };

    std::vector<GeoDef> Geoms;
    std::vector<ConstrDef> Constrs;
    GCS::System GCSsys;
    int ConstraintsCounter;
    std::vector<int> Conflicting;
    std::vector<int> Redundant;
    std::vector<int> PartiallyRedundant;
    std::vector<int> MalformedConstraints;

    std::vector<double*> pDependentParametersList;

    // map of geoIds to corresponding solverextensions. This is useful when solved geometry is NOT
    // to be assigned to the SketchObject
    std::vector<std::shared_ptr<SolverGeometryExtension>> solverExtensions;

    // maps a geoid corresponding to an internalgeometry (focus,knot,pole) to the geometry it
    // defines (ellipse, hyperbola, B-Spline)
    std::map<int, int> internalAlignmentGeometryMap;

    std::vector<std::set<std::pair<int, Sketcher::PointPos>>> pDependencyGroups;

    // this map is intended to convert a parameter (double *) into a GeoId/PointPos and parameter
    // number
    std::map<double*, std::tuple<int, Sketcher::PointPos, int>> param2geoelement;

    // solving parameters
    std::vector<double*> Parameters;        // with memory allocation
    std::vector<double*> DrivenParameters;  // with memory allocation
    std::vector<double*> FixParameters;     // with memory allocation
    std::vector<double> MoveParameters, InitParameters;
    std::vector<GCS::Point> Points;
    std::vector<GCS::Line> Lines;
    std::vector<GCS::Arc> Arcs;
    std::vector<GCS::Circle> Circles;
    std::vector<GCS::Ellipse> Ellipses;
    std::vector<GCS::ArcOfEllipse> ArcsOfEllipse;
    std::vector<GCS::ArcOfHyperbola> ArcsOfHyperbola;
    std::vector<GCS::ArcOfParabola> ArcsOfParabola;
    std::vector<GCS::BSpline> BSplines;

    bool isInitMove;
    bool isFine;
    Base::Vector3d initToPoint;
    double moveStep;

public:
    GCS::Algorithm defaultSolver;
    GCS::Algorithm defaultSolverRedundant;
    inline void setDogLegGaussStep(GCS::DogLegGaussStep mode)
    {
        GCSsys.dogLegGaussStep = mode;
    }
    inline void setDebugMode(GCS::DebugMode mode)
    {
        debugMode = mode;
        GCSsys.debugMode = mode;
    }
    inline GCS::DebugMode getDebugMode()
    {
        return debugMode;
    }
    inline void setMaxIter(int maxiter)
    {
        GCSsys.maxIter = maxiter;
    }
    inline void setMaxIterRedundant(int maxiter)
    {
        GCSsys.maxIterRedundant = maxiter;
    }
    inline void setSketchSizeMultiplier(bool mult)
    {
        GCSsys.sketchSizeMultiplier = mult;
    }
    inline void setSketchSizeMultiplierRedundant(bool mult)
    {
        GCSsys.sketchSizeMultiplierRedundant = mult;
    }
    inline void setConvergence(double conv)
    {
        GCSsys.convergence = conv;
    }
    inline void setConvergenceRedundant(double conv)
    {
        GCSsys.convergenceRedundant = conv;
    }
    inline void setQRAlgorithm(GCS::QRAlgorithm alg)
    {
        GCSsys.qrAlgorithm = alg;
    }
    inline GCS::QRAlgorithm getQRAlgorithm()
    {
        return GCSsys.qrAlgorithm;
    }
    inline void setQRPivotThreshold(double val)
    {
        GCSsys.qrpivotThreshold = val;
    }
    inline void setLM_eps(double val)
    {
        GCSsys.LM_eps = val;
    }
    inline void setLM_eps1(double val)
    {
        GCSsys.LM_eps1 = val;
    }
    inline void setLM_tau(double val)
    {
        GCSsys.LM_tau = val;
    }
    inline void setDL_tolg(double val)
    {
        GCSsys.DL_tolg = val;
    }
    inline void setDL_tolx(double val)
    {
        GCSsys.DL_tolx = val;
    }
    inline void setDL_tolf(double val)
    {
        GCSsys.DL_tolf = val;
    }
    inline void setLM_epsRedundant(double val)
    {
        GCSsys.LM_epsRedundant = val;
    }
    inline void setLM_eps1Redundant(double val)
    {
        GCSsys.LM_eps1Redundant = val;
    }
    inline void setLM_tauRedundant(double val)
    {
        GCSsys.LM_tauRedundant = val;
    }
    inline void setDL_tolgRedundant(double val)
    {
        GCSsys.DL_tolgRedundant = val;
    }
    inline void setDL_tolxRedundant(double val)
    {
        GCSsys.DL_tolxRedundant = val;
    }
    inline void setDL_tolfRedundant(double val)
    {
        GCSsys.DL_tolfRedundant = val;
    }

protected:
    GCS::DebugMode debugMode;

private:
    bool updateGeometry();
    bool updateNonDrivingConstraints();

    void calculateDependentParametersElements();

    void clearTemporaryConstraints();

    void buildInternalAlignmentGeometryMap(const std::vector<Constraint*>& constraintList);

    int internalSolve(std::string& solvername, int level = 0);

    /// checks if the index bounds and converts negative indices to positive
    int checkGeoId(int geoId) const;
    GCS::Curve* getGCSCurveByGeoId(int geoId);
    const GCS::Curve* getGCSCurveByGeoId(int geoId) const;

    // Block constraints

    /** This function performs a pre-analysis of blocked geometries, separating them into:
     *
     *  1) onlyblockedGeometry : Geometries affected exclusively by a block constraint.
     *
     *  2) blockedGeoIds       : Geometries affected notonly by a block constraint.
     *
     * This is important because 1) can be pre-fixed when creating geometry and constraints
     * before GCS::diagnose() via initSolution(). This is important because if no other constraint
     * affect the geometry, the geometry parameters won't even appear in the Jacobian, and they
     * won't be reported as dependent parameters.
     *
     * On the contrary 2) cannot be pre-fixed because it would lead to redundant constraints and
     * requires a post-analysis, see analyseBlockedConstraintDependentParameters, to fix just the
     * parameters that fulfil the dependacy groups.
     */
    bool analyseBlockedGeometry(const std::vector<Part::Geometry*>& internalGeoList,
                                const std::vector<Constraint*>& constraintList,
                                std::vector<bool>& onlyblockedGeometry,
                                std::vector<int>& blockedGeoIds) const;

    /* This function performs a post-analysis of blocked geometries (see analyseBlockedGeometry for
     * more detail on the pre-analysis).
     *
     * Basically identifies which parameters shall be fixed to make geometries having blocking
     * constraints fixed, while not leading to redundant/conflicting constraints. These parameters
     * must belong to blocked geometry. This is, groups may comprise parameters belonging to blocked
     * geometry and parameters belonging to unconstrained geometry. It is licit that the latter
     * remain as dependent parameters. The former are referred to as "blockable parameters".
     *
     * Extending this concept, there may be unsatisfiable groups (because they do not comprise any
     * bloackable parameter), and it is the desired outcome NOT to satisfy such groups.
     *
     * There is not a single combination of fixed parameters from the blockable parameters that
     * satisfy all the dependency groups. However:
     *
     * 1) some combinations do not satisfy all the dependency groups that must be satisfied (e.g.
     * fixing one group containing two blockable parameters with a given one may result in another
     * group, fixable only by the former, not to be satisfied). This leads, in a subsequent
     * diagnosis, to satisfiable unsatisfied groups.
     *
     * 2) some combinations lead to partially redundant constraints, that the solver will silently
     * drop in a subsequent diagnosis, thereby reducing the rank of the system fixing less than it
     * should.
     *
     * Implementation rationale (at this time):
     *
     * The implementation is on the order of the groups provided by the QR decomposition used to
     * reveal the parameters (see System::identifyDependentParameters in GCS). Zeros are made over
     * the pilot of the full R matrix of the QR decomposition, which is a top triangular
     * matrix.This, together with the permutation matrix, allow to know groups of dependent
     * parameters (cols between rank and full size). Each group refers to a new parameter not
     * affected by the rank in combination with other free parameters intervening in the rank
     * (because of the triangular shape of the R matrix). This results in that each the first column
     * between the rank and the full size, may only depend on a number of parameters, while the last
     * full size column may be dependent on any amount of previously introduced parameters.
     *
     * Thus the rationale is: start from the last group (having **potentially** the larger amount of
     * parameters) and selecting as blocking for that group the latest blockable parameter. Because
     * previous groups do not have access to the last parameter, this can never interfere with
     * previous groups. However, because the last parameter may not be a blockable one, there is a
     * risk of selecting a parameter common with other group, albeit the probability is reduced and
     * probably (I have not demonstrated it though and I am not sure), it leads to the right
     * solution in one iteration.
     *
     */
    bool analyseBlockedConstraintDependentParameters(std::vector<int>& blockedGeoIds,
                                                     std::vector<double*>& params_to_block) const;

    /// utility function refactoring fixing the provided parameters and running a new diagnose
    void fixParametersAndDiagnose(std::vector<double*>& params_to_block);
};

}  // namespace Sketcher


#endif  // SKETCHER_SKETCH_H
