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

#include <Mod/Sketcher/App/SketchAnalysis.h>

#include "Analyse.h"

#include "Sketch.h"

#include "SketchGeometryExtension.h"

namespace Sketcher
{

struct SketcherExport GeoEnum
{
    static const int RtPnt;
    static const int HAxis;
    static const int VAxis;
    static const int RefExt;
};

class SketchAnalysis;

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
    /** @name methods override Feature */
    //@{
    short mustExecute() const;
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

    /*!
     \brief Returns true if the sketcher supports the given geometry
     \param geo - the geometry
     \retval bool - true if the geometry is supported
     */
    bool isSupportedGeometry(const Part::Geometry *geo) const;
    /// add unspecified geometry
    int addGeometry(const Part::Geometry *geo, bool construction=false);
    /// add unspecified geometry
    int addGeometry(const std::vector<Part::Geometry *> &geoList, bool construction=false);
    /*!
     \brief Deletes indicated geometry (by geoid).
     \param GeoId - the geometry to delete
     \param deleteinternalgeo - if true deletes the associated and unconstraint internal geometry, otherwise deletes only the GeoId
     \retval int - 0 if successful
     */
    int delGeometry(int GeoId, bool deleteinternalgeo = true);
    /// deletes all the elements/constraints of the sketch except for external geometry
    int deleteAllGeometry();
    /// deletes all the constraints of the sketch
    int deleteAllConstraints();
    /// add all constraints in the list
    int addConstraints(const std::vector<Constraint *> &ConstraintList);
    /// Copy the constraints instead of cloning them and copying the expressions if any
    int addCopyOfConstraints(const SketchObject &orig);
    /// add constraint
    int addConstraint(const Constraint *constraint);
    /// delete constraint
    int delConstraint(int ConstrId);
    int delConstraints(std::vector<int> ConstrIds, bool updategeometry=true);
    int delConstraintOnPoint(int GeoId, PointPos PosId, bool onlyCoincident=true);
    int delConstraintOnPoint(int VertexId, bool onlyCoincident=true);
    /// Deletes all constraints referencing an external geometry
    int delConstraintsToExternal();
    /// transfers all constraints of a point to a new point
    int transferConstraints(int fromGeoId, PointPos fromPosId, int toGeoId, PointPos toPosId);
    /// Carbon copy another sketch geometry and constraints
    int carbonCopy(App::DocumentObject * pObj, bool construction = true);
    /// add an external geometry reference
    int addExternal(App::DocumentObject *Obj, const char* SubName);
    /** delete external
     *  ExtGeoId >= 0 with 0 corresponding to the first user defined
     *  external geometry
     */
    int delExternal(int ExtGeoId);

    /** deletes all external geometry */
    int delAllExternal();

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
    /**
     * sets the geometry of sketchObject as the solvedsketch geometry
     * returns the DoF of such a geometry.
     */
    int setUpSketch();

    /** solves the sketch and updates the geometry, but not all the dependent features (does not recompute)
        When a recompute is necessary, recompute triggers execute() which solves the sketch and updates all dependent features
        When a solve only is necessary (e.g. DoF changed), solve() solves the sketch and
        updates the geometry (if updateGeoAfterSolving==true), but does not trigger any recompute.
        @return 0 if no error, if error, the following codes in this order of priority: -4 if overconstrained,
                -3 if conflicting, -1 if solver error, -2 if redundant constraints
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

    /// set the driving status of this constraint and solve
    int setActive(int ConstrId, bool isactive);
    /// get the driving status of this constraint
    int getActive(int ConstrId, bool &isactive);
    /// toggle the driving status of this constraint
    int toggleActive(int ConstrId);

    /// Make all dimensionals Driving/non-Driving
    int setDatumsDriving(bool isdriving);
    /// Move Dimensional constraints at the end of the properties array
    int moveDatumsToEnd(void);

    /// set the driving status of this constraint and solve
    int setVirtualSpace(int ConstrId, bool isinvirtualspace);
    /// get the driving status of this constraint
    int getVirtualSpace(int ConstrId, bool &isinvirtualspace) const;
    /// toggle the driving status of this constraint
    int toggleVirtualSpace(int ConstrId);
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
    /// extend a curve
    int extend(int geoId, double increment, int endPoint);

    /// adds symmetric geometric elements with respect to the refGeoId (line or point)
    int addSymmetric(const std::vector<int> &geoIdList, int refGeoId, Sketcher::PointPos refPosId=Sketcher::none);
    /// with default parameters adds a copy of the geometric elements displaced by the displacement vector.
    /// It creates an array of csize elements in the direction of the displacement vector by rsize elements in the
    /// direction perpendicular to the displacement vector, wherein the modulus of this perpendicular vector is scaled by perpscale.
    int addCopy(const std::vector<int> &geoIdList, const Base::Vector3d& displacement, bool moveonly = false, bool clone=false, int csize=2, int rsize=1, bool constraindisplacement = false, double perpscale = 1.0);
    /// Exposes all internal geometry of an object supporting internal geometry
    /*!
     * \return -1 on error
     */
    int exposeInternalGeometry(int GeoId);
    /*!
     \brief Deletes all unused (not further constrained) internal geometry
     \param GeoId - the geometry having the internal geometry to delete
     \param delgeoid - if true in addition to the unused internal geometry also deletes the GeoId geometry
     \retval int - returns -1 on error, otherwise the number of deleted elements
     */
    int deleteUnusedInternalGeometry(int GeoId, bool delgeoid=false);
    /*!
     \brief Approximates the given geometry with a B-spline
     \param GeoId - the geometry to approximate
     \param delgeoid - if true in addition to the unused internal geometry also deletes the GeoId geometry
     \retval bool - returns true if the approximation succeeded, or false if it did not succeed.
     */
    bool convertToNURBS(int GeoId);

    /*!
     \brief Increases the degree of a BSpline by degreeincrement, which defaults to 1
     \param GeoId - the geometry of type bspline to increase the degree
     \param degreeincrement - the increment in number of degrees to effect
     \retval bool - returns true if the increase in degree succeeded, or false if it did not succeed.
     */
    bool increaseBSplineDegree(int GeoId, int degreeincrement = 1);

    /*!
     \brief Increases or Decreases the multiplicity of a BSpline knot by the multiplicityincr param, which defaults to 1, if the result is multiplicity zero, the knot is removed
     \param GeoId - the geometry of type bspline to increase the degree
     \param knotIndex - the index of the knot to modify (note that index is OCC consistent, so 1<=knotindex<=knots)
     \param multiplicityincr - the increment (positive value) or decrement (negative value) of multiplicity of the knot
     \retval bool - returns true if the operation succeeded, or false if it did not succeed.
     */
    bool modifyBSplineKnotMultiplicity(int GeoId, int knotIndex, int multiplicityincr = 1);

    /// retrieves for a Vertex number the corresponding GeoId and PosId
    void getGeoVertexIndex(int VertexId, int &GeoId, PointPos &PosId) const;
    int getHighestVertexIndex(void) const { return VertexId2GeoId.size() - 1; } // Most recently created
    int getHighestCurveIndex(void) const { return Geometry.getSize() - 1; }
    void rebuildVertexIndex(void);

    /// retrieves for a GeoId and PosId the Vertex number
    int getVertexIndexGeoPos(int GeoId, PointPos PosId) const;

    // retrieves an array of maps, each map containing the points that are coincidence by virtue of
    // any number of direct or indirect coincidence constraints
    const std::vector< std::map<int, Sketcher::PointPos> > getCoincidenceGroups();
    // returns if the given geoId is fixed (coincident) with external geometry on any of the possible relevant points
    void isCoincidentWithExternalGeometry(int GeoId, bool &start_external, bool &mid_external, bool &end_external);
    // returns a map containing all the GeoIds that are coincident with the given point as keys, and the PosIds as values associated
    // with the keys.
    const std::map<int, Sketcher::PointPos> getAllCoincidentPoints(int GeoId, PointPos PosId);

    /// retrieves for a Vertex number a list with all coincident points (sharing a single coincidence constraint)
    void getDirectlyCoincidentPoints(int GeoId, PointPos PosId, std::vector<int> &GeoIdList,
                             std::vector<PointPos> &PosIdList);
    void getDirectlyCoincidentPoints(int VertexId, std::vector<int> &GeoIdList, std::vector<PointPos> &PosIdList);
    bool arePointsCoincident(int GeoId1, PointPos PosId1, int GeoId2, PointPos PosId2);

    /// generates a warning message about constraint conflicts and appends it to the given message
    static void appendConflictMsg(const std::vector<int> &conflicting, std::string &msg);
    /// generates a warning message about redundant constraints and appends it to the given message
    static void appendRedundantMsg(const std::vector<int> &redundant, std::string &msg);

    double calculateAngleViaPoint(int geoId1, int geoId2, double px, double py);
    bool isPointOnCurve(int geoIdCurve, double px, double py);
    double calculateConstraintError(int ConstrId);
    int changeConstraintsLocking(bool bLock);
    /// returns whether a given constraint has an associated expression or not
    bool constraintHasExpression(int constrid) const;

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
    /// Checks if support is valid
    bool evaluateSupport(void);
    /// validate External Links (remove invalid external links)
    void validateExternalLinks(void);

    /// gets DoF of last solver execution
    inline int getLastDoF() const {return lastDoF;}
    /// gets HasConflicts status of last solver execution
    inline bool getLastHasConflicts() const {return lastHasConflict;}
    /// gets HasRedundancies status of last solver execution
    inline bool getLastHasRedundancies() const {return lastHasRedundancies;}
    /// gets solver status of last solver execution
    inline int getLastSolverStatus() const {return lastSolverStatus;}
    /// gets solver SolveTime of last solver execution
    inline float getLastSolveTime() const {return lastSolveTime;}
    /// gets the conflicting constraints of the last solver execution
    inline const std::vector<int> &getLastConflicting(void) const { return lastConflicting; }
    /// gets the redundant constraints of last solver execution
    inline const std::vector<int> &getLastRedundant(void) const { return lastRedundant; }
    /// gets the solved sketch as a reference
    inline Sketch &getSolvedSketch(void) {return solvedSketch;}

    /// returns the geometric elements/vertex which the solver detects as having dependent parameters.
    /// these parameters relate to not fully constraint edges/vertices.
    void getGeometryWithDependentParameters(std::vector<std::pair<int,PointPos>>& geometrymap);

    /// Flag to allow external geometry from other bodies than the one this sketch belongs to
    bool isAllowedOtherBody() const {
        return allowOtherBody;
    }
    void setAllowOtherBody(bool on) {
        allowOtherBody = on;
    }

    /// Flag to allow carbon copy from misaligned geometry
    bool isAllowedUnaligned() const {
        return allowUnaligned;
    }
    void setAllowUnaligned(bool on) {
        allowUnaligned = on;
    }

    enum eReasonList{
        rlAllowed,
        rlOtherDoc,
        rlCircularReference,
        rlOtherPart,
        rlOtherBody,
        rlOtherBodyWithLinks,   // for carbon copy
        rlNotASketch,           // for carbon copy
        rlNonParallel,          // for carbon copy
        rlAxesMisaligned,       // for carbon copy
        rlOriginsMisaligned     // for carbon copy
    };
    /// Return true if this object is allowed as external geometry for the
    /// sketch. rsn argument receives the reason for disallowing.
    bool isExternalAllowed(App::Document *pDoc, App::DocumentObject *pObj, eReasonList* rsn = 0) const;

    bool isCarbonCopyAllowed(App::Document *pDoc, App::DocumentObject *pObj, bool & xinv, bool & yinv, eReasonList* rsn = 0) const;
public:
    // Analyser functions
    int autoConstraint(double precision = Precision::Confusion() * 1000, double angleprecision = M_PI/20, bool includeconstruction = true);

    int detectMissingPointOnPointConstraints(double precision = Precision::Confusion() * 1000, bool includeconstruction = true);
    void analyseMissingPointOnPointCoincident(double angleprecision = M_PI/8);
    int detectMissingVerticalHorizontalConstraints(double angleprecision = M_PI/8);
    int detectMissingEqualityConstraints(double precision);

    std::vector<ConstraintIds> &getMissingPointOnPointConstraints(void);
    std::vector<ConstraintIds> &getMissingVerticalHorizontalConstraints(void);
    std::vector<ConstraintIds> &getMissingLineEqualityConstraints(void);
    std::vector<ConstraintIds> &getMissingRadiusConstraints(void);

    void setMissingRadiusConstraints(std::vector<ConstraintIds> &cl);
    void setMissingLineEqualityConstraints(std::vector<ConstraintIds>& cl);
    void setMissingVerticalHorizontalConstraints(std::vector<ConstraintIds>& cl);
    void setMissingPointOnPointConstraints(std::vector<ConstraintIds>& cl);

    void makeMissingPointOnPointCoincident(bool onebyone = false);
    void makeMissingVerticalHorizontal(bool onebyone = false);
    void makeMissingEquality(bool onebyone = true);

    // helper
    /// returns the number of redundant constraints detected
    int autoRemoveRedundants(bool updategeo = true);

    // Validation routines
    std::vector<Base::Vector3d> getOpenVertices(void) const;

protected:
    /// get called by the container when a property has changed
    virtual void onChanged(const App::Property* /*prop*/);
    virtual void onDocumentRestored();
    virtual void restoreFinished();

    virtual void setExpression(const App::ObjectIdentifier &path, boost::shared_ptr<App::Expression> expr, const char * comment = 0);

    std::string validateExpression(const App::ObjectIdentifier &path, boost::shared_ptr<const App::Expression> expr);

    void constraintsRenamed(const std::map<App::ObjectIdentifier, App::ObjectIdentifier> &renamed);
    void constraintsRemoved(const std::set<App::ObjectIdentifier> &removed);
    /*!
     \brief Returns a list of supported geometries from the input list
     \param geoList - the geometry list
     \retval list - the supported geometry list
     */
    std::vector<Part::Geometry *> supportedGeometry(const std::vector<Part::Geometry *> &geoList) const;


    // refactoring functions
    // check whether constraint may be changed driving status
    int testDrivingChange(int ConstrId, bool isdriving);

private:
    /// Flag to allow external geometry from other bodies than the one this sketch belongs to
    bool allowOtherBody;

    /// Flag to allow carbon copy from misaligned geometry
    bool allowUnaligned;

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

    boost::signals2::scoped_connection constraintsRenamedConn;
    boost::signals2::scoped_connection constraintsRemovedConn;

    bool AutoLockTangencyAndPerpty(Constraint* cstr, bool bForce = false, bool bLock = true);

    SketchAnalysis * analyser;
};

typedef App::FeaturePythonT<SketchObject> SketchObjectPython;

} //namespace Sketcher


#endif // SKETCHER_SKETCHOBJECT_H
