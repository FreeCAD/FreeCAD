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

#ifndef SKETCHER_FCSSKETCH_H
#define SKETCHER_FCSSKETCH_H

#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>

#include <Mod/ConstraintSolver/App/ParameterStore.h>

#include <Mod/ConstraintSolver/App/G2D/ParaPoint.h>
#include <Mod/ConstraintSolver/App/G2D/ParaLine.h>

#include "Constraint.h"

#include "SketchSolver.h"

namespace Sketcher
{

class SketcherExport FCSSketch : public SketchSolver
{
    TYPESYSTEM_HEADER();

public:
    FCSSketch();
    virtual ~FCSSketch() override = default;

    // from base class
    virtual unsigned int getMemSize(void) const override;
    virtual void Save(Base::Writer &/*writer*/) const override;
    virtual void Restore(Base::XMLReader &/*reader*/) override;

    // from SketchSolver
            
    /// solve the actual set up sketch
    virtual int solve(void) override;
    
    virtual int setUpSketch(const std::vector<Part::Geometry *> &GeoList, const std::vector<Constraint *> &ConstraintList,
                    int extGeoCount=0) override;
                    
    /// return the actual geometry of the sketch a TopoShape
    virtual Part::TopoShape toShape(void) const override;
    
    /// returns the actual geometry
    virtual std::vector<Part::Geometry *> extractGeometry(bool withConstructionElements=true,
                                                  bool withExternalElements=false) const override;
    
    /// retrieves the index of a point
    virtual int getPointId(int geoId, PointPos pos) const override;
    /// retrieves a point
    virtual Base::Vector3d getPoint(int geoId, PointPos pos) const override;

    // Inline methods
    virtual inline bool hasConflicts(void) const override { return !Conflicting.empty(); }
    virtual inline const std::vector<int> &getConflicting(void) const override { return Conflicting; }
    virtual inline bool hasRedundancies(void) const override { return !Redundant.empty(); }
    virtual inline const std::vector<int> &getRedundant(void) const override { return Redundant; }
    
    /** initializes a point (or curve) drag by setting the current
      * sketch status as a reference
      */
    virtual int initMove(int geoId, PointPos pos, bool fine=true) override;
    
    /** Resets the initialization of a point or curve drag
     */
    virtual void resetInitMove() override;

    /** move this point (or curve) to a new location and solve.
      * This will introduce some additional weak constraints expressing
      * a condition for satisfying the new point location!
      * The relative flag permits moving relatively to the current position
      */
    virtual int movePoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative=false) override;
    
    //This is to be used for rendering of angle-via-point constraint.
    virtual Base::Vector3d calculateNormalAtPoint(int geoIdCurve, double px, double py) override;

    //icstr should be the value returned by addXXXXConstraint
    //see more info in respective function in GCS.
    //double calculateConstraintError(int icstr) { return GCSsys.calculateConstraintErrorByTag(icstr);}

    /// Returns the size of the Geometry
    virtual int getGeometrySize(void) const override;
    
    virtual float getSolveTime() override;
    virtual void setRecalculateInitialSolutionWhileMovingPoint(bool on) override;
    
private:
    
    enum class GeoType {
        None    = 0,
        Point   = 1, // 1 Point(start), 2 Parameters(x,y)
        Line    = 2, // 2 Points(start,end), 4 Parameters(x1,y1,x2,y2)
        Arc     = 3, // 3 Points(start,end,mid), (4)+5 Parameters((x1,y1,x2,y2),x,y,r,a1,a2)
        Circle  = 4, // 1 Point(mid), 3 Parameters(x,y,r)
        Ellipse = 5,  // 1 Point(mid), 5 Parameters(x,y,r1,r2,phi)  phi=angle xaxis of ellipse with respect of sketch xaxis
        ArcOfEllipse = 6,
        ArcOfHyperbola = 7,
        ArcOfParabola = 8,
        BSpline = 9
    };
    
    /// container element to store and work with the geometric elements of this sketch
    struct GeoDef {
        GeoDef() : geo(nullptr),type(GeoType::None),external(false),index(-1),
                   startPointId(-1),midPointId(-1),endPointId(-1) {}
        std::shared_ptr<Part::Geometry>     geo;            // pointer to the geometry
        GeoType                             type;           // type of the geometry
        bool                                external;       // flag for external geometries
        int                                 index;          // index in the corresponding storage vector (Lines, Arcs, Circles, ...)
        int                                 startPointId;   // index in Points of the start point of this geometry
        int                                 midPointId;     // index in Points of the start point of this geometry
        int                                 endPointId;     // index in Points of the end point of this geometry
    };
    
    
private:
    /// add unspecified geometry, where each element's "fixed" status is given by the blockedGeometry array
    int addGeometry(const std::vector<Part::Geometry *> &geo,
                    const std::vector<bool> &blockedGeometry);
    
    int addGeometry(const std::vector<Part::Geometry *> &geo, bool fixed=false);
    
    /// add unspecified geometry
    int addGeometry(const Part::Geometry *geo, bool fixed=false);
    
    int addPoint(const Part::GeomPoint &point, bool fixed=false);
    int addLineSegment(const Part::GeomLineSegment &lineSegment, bool fixed=false);
    
    
    
    int checkGeoId(int geoId) const;
    
private:
    // Solver
    FCS::HParameterStore parameterStore;
    
    // Interface classes
    std::vector<GeoDef>                         Geoms;
    std::vector<FCS::G2D::HParaPoint>           Points;
    std::vector<FCS::G2D::HParaLine>            LineSegments;
    
    // Equation system diagnosis
    std::vector<int> Conflicting;
    std::vector<int> Redundant;
};

} //namespace Sketcher

#endif // SKETCHER_FCSSKETCH_H
