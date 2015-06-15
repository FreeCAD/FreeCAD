/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2008    *
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

#ifndef SKETCHER_SKETCHOBJECT_H
#define SKETCHER_SKETCHOBJECT_H

#include <App/PropertyStandard.h>
#include <App/PropertyFile.h>
#include <App/FeaturePython.h>
#include <Base/Axis.h>

#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/PropertyGeometryList.h>
#include <Mod/Sketcher/App/PropertyConstraintList.h>

#include "Sketch.h"

namespace Sketcher
{

class SketcherExport SketchObject : public Part::Part2DObject
{
    PROPERTY_HEADER(Sketcher::SketchObject);

public:
    SketchObject();
    ~SketchObject();

    /// Property
    Part    ::PropertyGeometryList   Geometry;
    Sketcher::PropertyConstraintList Constraints;
    App     ::PropertyLinkSubList    ExternalGeometry;
    /** @name methods overide Feature */
    //@{
    /// recalculate the Feature (if no recompute is needed see also solve() and solverNeedsUpdate boolean)
    App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "SketcherGui::ViewProviderSketch";
    }
    //@}
    
    /** SketchObject can work in two modes: Recompute Mode and noRecomputes Mode
        - In Recompute Mode, a recompute is necessary after each geometry addition to update the solver DoF (default)
        - In NoRecomputes Mode, no recompute is necessary after a geometry addition. If a recompute is triggered
          it is just less efficient.
          
        This flag does not regulate whether this object will recompute or not if execute() or a recompute() is actually executed,
        it just regulates whether the solver is called or not (i.e. whether it relies on 
        the solve of execute for the calculation)
    */
    bool noRecomputes;

    /// add unspecified geometry
    int addGeometry(const Part::Geometry *geo, bool construction=false);
    /// add unspecified geometry
    int addGeometry(const std::vector<Part::Geometry *> &geoList, bool construction=false);
    /// delete geometry
    int delGeometry(int GeoId);
    /// add all constraints in the list
    int addConstraints(const std::vector<Constraint *> &ConstraintList);
    /// add constraint
    int addConstraint(const Constraint *constraint);
    /// delete constraint
    int delConstraint(int ConstrId);
    int delConstraintOnPoint(int GeoId, PointPos PosId, bool onlyCoincident=true);
    int delConstraintOnPoint(int VertexId, bool onlyCoincident=true);
    /// Deletes all constraints referencing an external geometry
    int delConstraintsToExternal();
    /// transfers all contraints of a point to a new point
    int transferConstraints(int fromGeoId, PointPos fromPosId, int toGeoId, PointPos toPosId);
    /// add an external geometry reference
    int addExternal(App::DocumentObject *Obj, const char* SubName);
    /** delete external
     *  ExtGeoId >= 0 with 0 corresponding to the first user defined
     *  external geometry
     */
    int delExternal(int ExtGeoId);

    /** returns a pointer to a given Geometry index, possible indexes are:
     *  id>=0 for user defined geometries,
     *  id==-1 for the horizontal sketch axis,
     *  id==-2 for the vertical sketch axis
     *  id<=-3 for user defined projected external geometries,
     */
    const Part::Geometry* getGeometry(int GeoId) const;
    /// returns a list of all internal geometries
    const std::vector<Part::Geometry *> &getInternalGeometry(void) const { return Geometry.getValues(); }
    /// returns a list of projected external geometries
    const std::vector<Part::Geometry *> &getExternalGeometry(void) const { return ExternalGeo; }
    /// rebuilds external geometry (projection onto the sketch plane)
    void rebuildExternalGeometry(void);
    /// returns the number of external Geometry entities
    int getExternalGeometryCount(void) const { return ExternalGeo.size(); }

    /// retrieves a vector containing both normal and external Geometry (including the sketch axes)
    std::vector<Part::Geometry*> getCompleteGeometry(void) const;

    /// returns non zero if the sketch contains conflicting constraints
    int hasConflicts(void) const;

    /** solves the sketch and updates the geometry, but not all the dependent features (does not recompute)
        When a recompute is necessary, recompute triggers execute() which solves the sketch and updates all dependent features
        When a solve only is necessary (e.g. DoF changed), solve() solves the sketch and 
        updates the geometry (if updateGeoAfterSolving==true), but does not trigger any updates
    */
    int solve(bool updateGeoAfterSolving=true);   
    /// set the datum of a Distance or Angle constraint and solve
    int setDatum(int ConstrId, double Datum);
    /// set the driving status of this constraint and solve
    int setDriving(int ConstrId, bool isdriving);
    /// get the driving status of this constraint
    int getDriving(int ConstrId, bool &isdriving);
    /// toggle the driving status of this constraint
    int toggleDriving(int ConstrId);
    /// move this point to a new location and solve
    int movePoint(int GeoId, PointPos PosId, const Base::Vector3d& toPoint, bool relative=false, bool updateGeoBeforeMoving=false);
    /// retrieves the coordinates of a point
    Base::Vector3d getPoint(int GeoId, PointPos PosId) const;

    /// toggle geometry to draft line
    int toggleConstruction(int GeoId);
    int setConstruction(int GeoId, bool on);

    /// create a fillet
    int fillet(int geoId, PointPos pos, double radius, bool trim=true);
    int fillet(int geoId1, int geoId2,
               const Base::Vector3d& refPnt1, const Base::Vector3d& refPnt2,
               double radius, bool trim=true);

    /// trim a curve
    int trim(int geoId, const Base::Vector3d& point);
    /// Exposes all internal geometry of an object supporting internal geometry
    /*!
     * \return -1 on error
     */
    int ExposeInternalGeometry(int GeoId);
    /// Deletes all unused (not further constrained) internal geometry
    /*!
     * \return -1 on error
     */
    int DeleteUnusedInternalGeometry(int GeoId);

    /// retrieves for a Vertex number the corresponding GeoId and PosId
    void getGeoVertexIndex(int VertexId, int &GeoId, PointPos &PosId) const;
    int getHighestVertexIndex(void) const { return VertexId2GeoId.size() - 1; } // Most recently created
    int getHighestCurveIndex(void) const { return Geometry.getSize() - 1; }
    void rebuildVertexIndex(void);
    
    /// retrieves for a GeoId and PosId the Vertex number 
    int getVertexIndexGeoPos(int GeoId, PointPos PosId) const;

    /// retrieves for a Vertex number a list with all coincident points
    void getCoincidentPoints(int GeoId, PointPos PosId, std::vector<int> &GeoIdList,
                             std::vector<PointPos> &PosIdList);
    void getCoincidentPoints(int VertexId, std::vector<int> &GeoIdList, std::vector<PointPos> &PosIdList);
    bool arePointsCoincident(int GeoId1, PointPos PosId1, int GeoId2, PointPos PosId2);

    /// generates a warning message about constraint conflicts and appends it to the given message
    static void appendConflictMsg(const std::vector<int> &conflicting, std::string &msg);
    /// generates a warning message about redundant constraints and appends it to the given message
    static void appendRedundantMsg(const std::vector<int> &redundant, std::string &msg);

    double calculateAngleViaPoint(int geoId1, int geoId2, double px, double py);
    bool isPointOnCurve(int geoIdCurve, double px, double py);
    double calculateConstraintError(int ConstrId);
    int changeConstraintsLocking(bool bLock);

    ///porting functions
    int port_reversedExternalArcs(bool justAnalyze);

    // from base class
    virtual PyObject *getPyObject(void);
    virtual unsigned int getMemSize(void) const;
    virtual void Save(Base::Writer &/*writer*/) const;
    virtual void Restore(Base::XMLReader &/*reader*/);

    /// returns the number of construction lines (to be used as axes)
    virtual int getAxisCount(void) const;
    /// retrieves an axis iterating through the construction lines of the sketch (indices start at 0)
    virtual Base::Axis getAxis(int axId) const;
    /// verify and accept the assigned geometry
    virtual void acceptGeometry();
    /// Check if constraint has invalid indexes
    bool evaluateConstraint(const Constraint *constraint) const;
    /// Check for constraints with invalid indexes
    bool evaluateConstraints() const;
    /// Remove constraints with invalid indexes
    void validateConstraints();
    
    /// gets DoF of last solver execution
    int getLastDoF() const {return lastDoF;}
    /// gets HasConflicts status of last solver execution
    bool getLastHasConflicts() const {return lastHasConflict;}
    /// gets HasRedundancies status of last solver execution
    bool getLastHasRedundancies() const {return lastHasRedundancies;}
    /// gets solver status of last solver execution
    int getLastSolverStatus() const {return lastSolverStatus;}
    /// gets solver SolveTime of last solver execution
    float getLastSolveTime() const {return lastSolveTime;}
    /// gets the conflicting constraints of the last solver execution
    const std::vector<int> &getLastConflicting(void) const { return lastConflicting; }
    /// gets the redundant constraints of last solver execution
    const std::vector<int> &getLastRedundant(void) const { return lastRedundant; }

    Sketch &getSolvedSketch(void) {return solvedSketch;}

protected:
    /// get called by the container when a property has changed
    virtual void onChanged(const App::Property* /*prop*/);
    virtual void onDocumentRestored();

private:
    std::vector<Part::Geometry *> ExternalGeo;

    std::vector<int> VertexId2GeoId;
    std::vector<PointPos> VertexId2PosId;
    
    Sketch solvedSketch;
    
    /** this internal flag indicate that an operation modifying the geometry, but not the DoF of the sketch took place (e.g. toggle construction), 
        so if next action is a movement of a point (movePoint), the geometry must be updated first.
    */
    bool solverNeedsUpdate;
    
    int lastDoF;
    bool lastHasConflict;
    bool lastHasRedundancies;
    int lastSolverStatus;
    float lastSolveTime;

    std::vector<int> lastConflicting;
    std::vector<int> lastRedundant;

    bool AutoLockTangencyAndPerpty(Constraint* cstr, bool bForce = false, bool bLock = true);
};

typedef App::FeaturePythonT<SketchObject> SketchObjectPython;

} //namespace Sketcher


#endif // SKETCHER_SKETCHOBJECT_H
