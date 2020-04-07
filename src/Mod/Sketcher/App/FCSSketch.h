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
    virtual bool hasConflicts(void) const override;
    virtual const std::vector<int> &getConflicting(void) const override;
    virtual bool hasRedundancies(void) const override;
    virtual const std::vector<int> &getRedundant(void) const override;
    
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
    /// add unspecified geometry, where each element's "fixed" status is given by the blockedGeometry array
    int addGeometry(const std::vector<Part::Geometry *> &geo,
                    const std::vector<bool> &blockedGeometry);
    
    /// add unspecified geometry
    int addGeometry(const Part::Geometry *geo, bool fixed=false);
    
    
private:
    FCS::HParameterStore parameterStore;
};

} //namespace Sketcher

#endif // SKETCHER_FCSSKETCH_H
