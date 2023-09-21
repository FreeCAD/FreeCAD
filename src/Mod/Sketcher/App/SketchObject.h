/***************************************************************************
 *   Copyright (c) 2008 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <App/FeaturePython.h>
#include <App/IndexedName.h>
#include <App/PropertyFile.h>
#include <Base/Axis.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/PropertyGeometryList.h>
#include <Mod/Sketcher/App/PropertyConstraintList.h>
#include <Mod/Sketcher/App/SketchAnalysis.h>

#include "Analyse.h"
#include "GeoEnum.h"
#include "GeoList.h"
#include "GeometryFacade.h"
#include "Sketch.h"


namespace Sketcher
{

class SketchAnalysis;

class SketcherExport SketchObject: public Part::Part2DObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Sketcher::SketchObject);

public:
    SketchObject();
    ~SketchObject() override;

    /// Property
    /**
     The Geometry list contains the non-external Part::Geometry objects in the sketch.  The list
     may be accessed directly, or indirectly via getInternalGeometry().

     Many of the methods in this class take geoId and posId parameters.  A GeoId is a unique
     identifier for geometry in the Sketch. geoId >= 0 means an index in the Geometry list. geoId <
     0 refers to sketch axes and external geometry.  posId is a PointPos enum, documented in
     Constraint.h.
    */
    Part ::PropertyGeometryList Geometry;
    Sketcher::PropertyConstraintList Constraints;
    App ::PropertyLinkSubList ExternalGeometry;
    App ::PropertyBool FullyConstrained;
    /** @name methods override Feature */
    //@{
    short mustExecute() const override;
    /// recalculate the Feature (if no recompute is needed see also solve() and solverNeedsUpdate
    /// boolean)
    App::DocumentObjectExecReturn* execute() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "SketcherGui::ViewProviderSketch";
    }
    //@}

    /** SketchObject can work in two modes: Recompute Mode and noRecomputes Mode
        - In Recompute Mode, a recompute is necessary after each geometry addition to update the
       solver DoF (default)
        - In NoRecomputes Mode, no recompute is necessary after a geometry addition. If a recompute
       is triggered it is just less efficient.

        This flag does not regulate whether this object will recompute or not if execute() or a
       recompute() is actually executed, it just regulates whether the solver is called or not (i.e.
       whether it relies on the solve of execute for the calculation)
    */
    bool noRecomputes;

    /*!
     \brief Returns true if the sketcher supports the given geometry
     \param geo - the geometry
     \retval bool - true if the geometry is supported
     */
    bool isSupportedGeometry(const Part::Geometry* geo) const;
    /*!
     \brief Add geometry to a sketch - It adds a copy with a different uuid (internally uses copy()
     instead of clone()) \param geo - geometry to add \param construction - true for construction
     lines \retval int - GeoId of added element
     */
    int addGeometry(const Part::Geometry* geo, bool construction = false);

    /*!
     \brief Add geometry to a sketch using up the provided newgeo. Caveat: It will use the provided
     newgeo with the uuid it has. This is different from the addGeometry method with a naked
     pointer, where a different uuid is ensured. The caller is responsible for provided a new or
     existing uuid, as necessary. \param geo - geometry to add \param construction - true for
     construction lines \retval int - GeoId of added element
     */
    int addGeometry(std::unique_ptr<Part::Geometry> newgeo, bool construction = false);

    /*!
     \brief Add multiple geometry elements to a sketch
     \param geoList - geometry to add
     \param construction - true for construction lines
     \retval int - GeoId of last added element
     */
    int addGeometry(const std::vector<Part::Geometry*>& geoList, bool construction = false);
    /*!
     \brief Deletes indicated geometry (by geoid).
     \param GeoId - the geometry to delete
     \param deleteinternalgeo - if true deletes the associated and unconstraint internal geometry,
     otherwise deletes only the GeoId \retval int - 0 if successful
     */
    int delGeometry(int GeoId, bool deleteinternalgeo = true);
    /// Deletes just the GeoIds indicated, it does not look for internal geometry
    int delGeometriesExclusiveList(const std::vector<int>& GeoIds);
    /// Does the same as \a delGeometry but allows to delete several geometries in one step
    int delGeometries(const std::vector<int>& GeoIds);
    /// deletes all the elements/constraints of the sketch except for external geometry
    int deleteAllGeometry();
    /// deletes all the constraints of the sketch
    int deleteAllConstraints();
    /// add all constraints in the list
    int addConstraints(const std::vector<Constraint*>& ConstraintList);
    /// Copy the constraints instead of cloning them and copying the expressions if any
    int addCopyOfConstraints(const SketchObject& orig);
    /// add constraint
    int addConstraint(const Constraint* constraint);
    /// add constraint
    int addConstraint(std::unique_ptr<Constraint> constraint);
    /// delete constraint
    int delConstraint(int ConstrId);
    /** deletes a group of constraints at once, if norecomputes is active, the default behaviour is
     * that it will solve the sketch.
     *
     * If updating the Geometry property as a consequence of a (successful) solve() is not wanted,
     * updategeometry=false, prevents the update. This allows to update the solve status (e.g. dof),
     * without updating the geometry (i.e. make it move to fulfil the constraints).
     */
    int delConstraints(std::vector<int> ConstrIds, bool updategeometry = true);
    int delConstraintOnPoint(int GeoId, PointPos PosId, bool onlyCoincident = true);
    int delConstraintOnPoint(int VertexId, bool onlyCoincident = true);
    /// Deletes all constraints referencing an external geometry
    int delConstraintsToExternal();
    /// transfers all constraints of a point to a new point
    int transferConstraints(int fromGeoId,
                            PointPos fromPosId,
                            int toGeoId,
                            PointPos toPosId,
                            bool doNotTransformTangencies = false);

    /// Carbon copy another sketch geometry and constraints
    int carbonCopy(App::DocumentObject* pObj, bool construction = true);
    /// add an external geometry reference
    int addExternal(App::DocumentObject* Obj, const char* SubName);
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
    template<
        typename GeometryT = Part::Geometry,
        typename = typename std::enable_if<
            std::is_base_of<Part::Geometry, typename std::decay<GeometryT>::type>::value>::type>
    const GeometryT* getGeometry(int GeoId) const;

    std::unique_ptr<const GeometryFacade> getGeometryFacade(int GeoId) const;

    /// returns a list of all internal geometries
    const std::vector<Part::Geometry*>& getInternalGeometry() const
    {
        return Geometry.getValues();
    }
    /// returns a list of projected external geometries
    const std::vector<Part::Geometry*>& getExternalGeometry() const
    {
        return ExternalGeo;
    }
    /// rebuilds external geometry (projection onto the sketch plane)
    void rebuildExternalGeometry();
    /// returns the number of external Geometry entities
    int getExternalGeometryCount() const
    {
        return ExternalGeo.size();
    }

    /// retrieves a vector containing both normal and external Geometry (including the sketch axes)
    std::vector<Part::Geometry*> getCompleteGeometry() const;

    GeoListFacade getGeoListFacade() const;

    /// converts a GeoId index into an index of the CompleteGeometry vector
    int getCompleteGeometryIndex(int GeoId) const;

    int getGeoIdFromCompleteGeometryIndex(int completeGeometryIndex) const;

    /// returns non zero if the sketch contains conflicting constraints
    int hasConflicts() const;
    /**
     * sets the geometry of sketchObject as the solvedsketch geometry
     * returns the DoF of such a geometry.
     */
    int setUpSketch();

    /** Performs a full analysis of the addition of additional constraints without adding them to
     * the sketch object */
    int diagnoseAdditionalConstraints(std::vector<Sketcher::Constraint*> additionalconstraints);

    /** solves the sketch and updates the geometry, but not all the dependent features (does not
       recompute) When a recompute is necessary, recompute triggers execute() which solves the
       sketch and updates all dependent features When a solve only is necessary (e.g. DoF changed),
       solve() solves the sketch and updates the geometry (if updateGeoAfterSolving==true), but does
       not trigger any recompute.
       @return 0 if no error, if error, the following codes in this order of priority:
       -4 if overconstrained,
       -3 if conflicting constraints,
       -5 if malformed constraints,
       -1 if solver error,
       -2 if redundant constraints
    */
    int solve(bool updateGeoAfterSolving = true);
    /// set the datum of a Distance or Angle constraint and solve
    int setDatum(int ConstrId, double Datum);
    /// set the driving status of this constraint and solve
    int setDriving(int ConstrId, bool isdriving);
    /// get the driving status of this constraint
    int getDriving(int ConstrId, bool& isdriving);
    /// toggle the driving status of this constraint
    int toggleDriving(int ConstrId)
    {
        return setDriving(ConstrId, !Constraints.getValues()[ConstrId]->isDriving);
    }

    /// set the driving status of this constraint and solve
    int setActive(int ConstrId, bool isactive);
    /// get the driving status of this constraint
    int getActive(int ConstrId, bool& isactive);
    /// toggle the driving status of this constraint
    int toggleActive(int ConstrId);

    /// Make all dimensionals Driving/non-Driving
    int setDatumsDriving(bool isdriving);
    /// Move Dimensional constraints at the end of the properties array
    int moveDatumsToEnd();

    /// Change an angle constraint to its supplementary angle.
    void reverseAngleConstraintToSupplementary(Constraint* constr, int constNum);
    /// Modify an angle constraint expression string to its supplementary angle
    static std::string reverseAngleConstraintExpression(std::string expression);

    // Check if a constraint has an expression associated.
    bool constraintHasExpression(int constNum) const;
    // Get a constraint associated expression
    std::string getConstraintExpression(int constNum) const;
    // Set a constraint associated expression
    void setConstraintExpression(int constNum, const std::string& newExpression);
    void setExpression(const App::ObjectIdentifier& path,
                       std::shared_ptr<App::Expression> expr) override;

    /// set the driving status of this constraint and solve
    int setVirtualSpace(int ConstrId, bool isinvirtualspace);
    /// set the driving status of a group of constraints at once
    int setVirtualSpace(std::vector<int> constrIds, bool isinvirtualspace);
    /// get the driving status of this constraint
    int getVirtualSpace(int ConstrId, bool& isinvirtualspace) const;
    /// toggle the driving status of this constraint
    int toggleVirtualSpace(int ConstrId);
    /// move this point to a new location and solve
    int movePoint(int GeoId,
                  PointPos PosId,
                  const Base::Vector3d& toPoint,
                  bool relative = false,
                  bool updateGeoBeforeMoving = false);
    /// retrieves the coordinates of a point
    static Base::Vector3d getPoint(const Part::Geometry* geo, PointPos PosId);
    Base::Vector3d getPoint(int GeoId, PointPos PosId) const;

    /// toggle geometry to draft line
    int toggleConstruction(int GeoId);
    int setConstruction(int GeoId, bool on);

    /*!
     \brief Create a sketch fillet from the point at the intersection of two lines
     \param geoId, pos - one of the (exactly) two coincident endpoints
     \param radius - fillet radius
     \param trim - if false, leaves the original lines untouched
     \param createCorner - keep geoId/pos as a Point and keep as many constraints as possible
     \retval - 0 on success, -1 on failure
     */
    int
    fillet(int geoId, PointPos pos, double radius, bool trim = true, bool preserveCorner = false);
    /*!
     \brief More general form of fillet
     \param geoId1, geoId2 - geoId for two lines (which don't necessarily have to coincide)
     \param refPnt1, refPnt2 - reference points on the input geometry, used to influence the free
     fillet variables \param radius - fillet radius \param trim - if false, leaves the original
     lines untouched \param preserveCorner - if the lines are coincident, place a Point where they
     meet and keep as many of the existing constraints as possible \retval - 0 on success, -1 on
     failure
     */
    int fillet(int geoId1,
               int geoId2,
               const Base::Vector3d& refPnt1,
               const Base::Vector3d& refPnt2,
               double radius,
               bool trim = true,
               bool createCorner = false);

    /// trim a curve
    int trim(int geoId, const Base::Vector3d& point);
    /// extend a curve
    int extend(int geoId, double increment, PointPos endPoint);
    /// split a curve
    int split(int geoId, const Base::Vector3d& point);
    /*!
      \brief Join one or two curves at the given end points
      \details The combined curve will be a b-spline
      \param geoId1, posId1, geoId2, posId2: the end points to join
      \retval - 0 on success, -1 on failure
    */
    int join(int geoId1, Sketcher::PointPos posId1, int geoId2, Sketcher::PointPos posId2);

    /// adds symmetric geometric elements with respect to the refGeoId (line or point)
    int addSymmetric(const std::vector<int>& geoIdList,
                     int refGeoId,
                     Sketcher::PointPos refPosId = Sketcher::PointPos::none);
    /// with default parameters adds a copy of the geometric elements displaced by the displacement
    /// vector. It creates an array of csize elements in the direction of the displacement vector by
    /// rsize elements in the direction perpendicular to the displacement vector, wherein the
    /// modulus of this perpendicular vector is scaled by perpscale.
    int addCopy(const std::vector<int>& geoIdList,
                const Base::Vector3d& displacement,
                bool moveonly = false,
                bool clone = false,
                int csize = 2,
                int rsize = 1,
                bool constraindisplacement = false,
                double perpscale = 1.0);

    int removeAxesAlignment(const std::vector<int>& geoIdList);
    /// Exposes all internal geometry of an object supporting internal geometry
    /*!
     * \return -1 on error
     */
    int exposeInternalGeometry(int GeoId);
    /*!
     \brief Deletes all unused (not further constrained) internal geometry
     \param GeoId - the geometry having the internal geometry to delete
     \param delgeoid - if true in addition to the unused internal geometry also deletes the GeoId
     geometry \retval int - returns -1 on error, otherwise the number of deleted elements
     */
    int deleteUnusedInternalGeometry(int GeoId, bool delgeoid = false);
    /*!
     \brief Approximates the given geometry with a B-spline
     \param GeoId - the geometry to approximate
     \param delgeoid - if true in addition to the unused internal geometry also deletes the GeoId
     geometry \retval bool - returns true if the approximation succeeded, or false if it did not
     succeed.
     */
    bool convertToNURBS(int GeoId);

    /*!
     \brief Increases the degree of a BSpline by degreeincrement, which defaults to 1
     \param GeoId - the geometry of type bspline to increase the degree
     \param degreeincrement - the increment in number of degrees to effect
     \retval bool - returns true if the increase in degree succeeded, or false if it did not
     succeed.
     */
    bool increaseBSplineDegree(int GeoId, int degreeincrement = 1);

    /*!
     \brief Decreases the degree of a BSpline by degreedecrement, which defaults to 1
     \param GeoId - the geometry of type bspline to increase the degree
     \param degreedecrement - the decrement in number of degrees to effect
     \retval bool - returns true if the decrease in degree succeeded, or false if it did not
     succeed.
     */
    bool decreaseBSplineDegree(int GeoId, int degreedecrement = 1);

    /*!
     \brief Increases or Decreases the multiplicity of a BSpline knot by the multiplicityincr param,
     which defaults to 1, if the result is multiplicity zero, the knot is removed \param GeoId - the
     geometry of type bspline to increase the degree \param knotIndex - the index of the knot to
     modify (note that index is OCC consistent, so 1<=knotindex<=knots) \param multiplicityincr -
     the increment (positive value) or decrement (negative value) of multiplicity of the knot
     \retval bool - returns true if the operation succeeded, or false if it did not succeed.
     */
    bool modifyBSplineKnotMultiplicity(int GeoId, int knotIndex, int multiplicityincr = 1);

    /*!
      \brief Inserts a knot in the BSpline at `param` with given `multiplicity`. If the knot already
      exists, its multiplicity is increased by `multiplicity`. \param GeoId - the geometry of type
      bspline to increase the degree \param param - the parameter value where the knot is to be
      placed \param multiplicity - multiplicity of the inserted knot \retval bool - returns true if
      the operation succeeded, or false if it did not succeed.
    */
    bool insertBSplineKnot(int GeoId, double param, int multiplicity = 1);

    /// retrieves for a Vertex number the corresponding GeoId and PosId
    void getGeoVertexIndex(int VertexId, int& GeoId, PointPos& PosId) const;
    int getHighestVertexIndex() const
    {
        return VertexId2GeoId.size() - 1;
    }  // Most recently created
    int getHighestCurveIndex() const
    {
        return Geometry.getSize() - 1;
    }
    void rebuildVertexIndex();

    /// retrieves for a GeoId and PosId the Vertex number
    int getVertexIndexGeoPos(int GeoId, PointPos PosId) const;

    // retrieves an array of maps, each map containing the points that are coincidence by virtue of
    // any number of direct or indirect coincidence constraints
    const std::vector<std::map<int, Sketcher::PointPos>> getCoincidenceGroups();
    // returns if the given geoId is fixed (coincident) with external geometry on any of the
    // possible relevant points
    void isCoincidentWithExternalGeometry(int GeoId,
                                          bool& start_external,
                                          bool& mid_external,
                                          bool& end_external);
    // returns a map containing all the GeoIds that are coincident with the given point as keys, and
    // the PosIds as values associated with the keys.
    const std::map<int, Sketcher::PointPos> getAllCoincidentPoints(int GeoId, PointPos PosId);

    /// retrieves for a Vertex number a list with all coincident points (sharing a single
    /// coincidence constraint)
    void getDirectlyCoincidentPoints(int GeoId,
                                     PointPos PosId,
                                     std::vector<int>& GeoIdList,
                                     std::vector<PointPos>& PosIdList);
    void getDirectlyCoincidentPoints(int VertexId,
                                     std::vector<int>& GeoIdList,
                                     std::vector<PointPos>& PosIdList);
    bool arePointsCoincident(int GeoId1, PointPos PosId1, int GeoId2, PointPos PosId2);

    /// returns a list of indices of all constraints involving given GeoId
    void getConstraintIndices(int GeoId, std::vector<int>& constraintList);

    /// generates a warning message about constraint conflicts and appends it to the given message
    static void appendConflictMsg(const std::vector<int>& conflicting, std::string& msg);
    /// generates a warning message about redundant constraints and appends it to the given message
    static void appendRedundantMsg(const std::vector<int>& redundant, std::string& msg);
    /// generates a warning message about malformed constraints and appends it to the given message
    static void appendMalformedConstraintsMsg(const std::vector<int>& malformed, std::string& msg);

    double calculateAngleViaPoint(int geoId1, int geoId2, double px, double py);
    bool isPointOnCurve(int geoIdCurve, double px, double py);
    double calculateConstraintError(int ConstrId);
    int changeConstraintsLocking(bool bLock);

    /// porting functions
    int port_reversedExternalArcs(bool justAnalyze);

    // from base class
    PyObject* getPyObject() override;
    unsigned int getMemSize() const override;
    void Save(Base::Writer& /*writer*/) const override;
    void Restore(Base::XMLReader& /*reader*/) override;

    /// returns the number of construction lines (to be used as axes)
    int getAxisCount() const override;
    /// retrieves an axis iterating through the construction lines of the sketch (indices start at
    /// 0)
    Base::Axis getAxis(int axId) const override;
    /// verify and accept the assigned geometry
    void acceptGeometry() override;
    /// Check if constraint has invalid indexes
    bool evaluateConstraint(const Constraint* constraint) const;
    /// Check for constraints with invalid indexes
    bool evaluateConstraints() const;
    /// Remove constraints with invalid indexes
    void validateConstraints();
    /// Checks if support is valid
    bool evaluateSupport();
    /// validate External Links (remove invalid external links)
    void validateExternalLinks();

    /// gets DoF of last solver execution
    inline int getLastDoF() const
    {
        return lastDoF;
    }
    /// gets HasConflicts status of last solver execution
    inline bool getLastHasConflicts() const
    {
        return lastHasConflict;
    }
    /// gets HasRedundancies status of last solver execution
    inline bool getLastHasRedundancies() const
    {
        return lastHasRedundancies;
    }
    /// gets HasRedundancies status of last solver execution
    inline bool getLastHasPartialRedundancies() const
    {
        return lastHasPartialRedundancies;
    }
    /// gets HasMalformedConstraints status of last solver execution
    inline bool getLastHasMalformedConstraints() const
    {
        return lastHasMalformedConstraints;
    }
    /// gets solver status of last solver execution
    inline int getLastSolverStatus() const
    {
        return lastSolverStatus;
    }
    /// gets solver SolveTime of last solver execution
    inline float getLastSolveTime() const
    {
        return lastSolveTime;
    }
    /// gets the conflicting constraints of the last solver execution
    inline const std::vector<int>& getLastConflicting() const
    {
        return lastConflicting;
    }
    /// gets the redundant constraints of last solver execution
    inline const std::vector<int>& getLastRedundant() const
    {
        return lastRedundant;
    }
    /// gets the redundant constraints of last solver execution
    inline const std::vector<int>& getLastPartiallyRedundant() const
    {
        return lastPartiallyRedundant;
    }
    /// gets the redundant constraints of last solver execution
    inline const std::vector<int>& getLastMalformedConstraints() const
    {
        return lastMalformedConstraints;
    }

public: /* Solver exposed interface */
    /// gets the solved sketch as a reference
    inline const Sketch& getSolvedSketch() const
    {
        return solvedSketch;
    }
    /// enables/disables solver initial solution recalculation when moving point mode (useful for
    /// dragging)
    inline void
    setRecalculateInitialSolutionWhileMovingPoint(bool recalculateInitialSolutionWhileMovingPoint)
    {
        solvedSketch.setRecalculateInitialSolutionWhileMovingPoint(
            recalculateInitialSolutionWhileMovingPoint);
    }
    /// Forwards a request for a temporary initMove to the solver using the current sketch state as
    /// a reference (enables dragging)
    inline int initTemporaryMove(int geoId, PointPos pos, bool fine = true);
    /// Forwards a request for a temporary initBSplinePieceMove to the solver using the current
    /// sketch state as a reference (enables dragging)
    inline int initTemporaryBSplinePieceMove(int geoId,
                                             PointPos pos,
                                             const Base::Vector3d& firstPoint,
                                             bool fine = true);
    /** Forwards a request for point or curve temporary movement to the solver using the current
     * state as a reference (enables dragging). NOTE: A temporary move operation must always be
     * preceded by a initTemporaryMove() operation.
     */
    inline int
    moveTemporaryPoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative = false);
    /// forwards a request to update an extension of a geometry of the solver to the solver.
    inline void updateSolverExtension(int geoId, std::unique_ptr<Part::GeometryExtension>&& ext)
    {
        return solvedSketch.updateExtension(geoId, std::move(ext));
    }

public:
    /// returns the geometric elements/vertex which the solver detects as having dependent
    /// parameters. these parameters relate to not fully constraint edges/vertices.
    void getGeometryWithDependentParameters(std::vector<std::pair<int, PointPos>>& geometrymap);

    /// Flag to allow external geometry from other bodies than the one this sketch belongs to
    bool isAllowedOtherBody() const
    {
        return allowOtherBody;
    }
    void setAllowOtherBody(bool on)
    {
        allowOtherBody = on;
    }

    /// Flag to allow carbon copy from misaligned geometry
    bool isAllowedUnaligned() const
    {
        return allowUnaligned;
    }
    void setAllowUnaligned(bool on)
    {
        allowUnaligned = on;
    }

    enum eReasonList
    {
        rlAllowed,
        rlOtherDoc,
        rlCircularReference,
        rlOtherPart,
        rlOtherBody,
        rlOtherBodyWithLinks,  // for carbon copy
        rlNotASketch,          // for carbon copy
        rlNonParallel,         // for carbon copy
        rlAxesMisaligned,      // for carbon copy
        rlOriginsMisaligned    // for carbon copy
    };
    /// Return true if this object is allowed as external geometry for the
    /// sketch. rsn argument receives the reason for disallowing.
    bool isExternalAllowed(App::Document* pDoc,
                           App::DocumentObject* pObj,
                           eReasonList* rsn = nullptr) const;

    bool isCarbonCopyAllowed(App::Document* pDoc,
                             App::DocumentObject* pObj,
                             bool& xinv,
                             bool& yinv,
                             eReasonList* rsn = nullptr) const;

    Part::TopoShape getEdge(const Part::Geometry* geo, const char* name) const;

    Data::IndexedName checkSubName(const char* sub) const;

    bool geoIdFromShapeType(const Data::IndexedName&, int& geoId, PointPos& posId) const;

    bool geoIdFromShapeType(const char* shapetype, int& geoId, PointPos& posId) const
    {
        return geoIdFromShapeType(checkSubName(shapetype), geoId, posId);
    }

    bool geoIdFromShapeType(const char* shapetype, int& geoId) const
    {
        PointPos posId;
        return geoIdFromShapeType(shapetype, geoId, posId);
    }

    std::string convertSubName(const char* subname, bool postfix = true) const
    {
        return convertSubName(checkSubName(subname), postfix);
    }

    std::string convertSubName(const std::string& subname, bool postfix = true) const
    {
        return convertSubName(subname.c_str(), postfix);
    }

    std::string convertSubName(const Data::IndexedName&, bool postfix = true) const;

    bool isPerformingInternalTransaction() const
    {
        return internaltransaction;
    };

    /** retrieves intersection points of this curve with the closest two curves around a point of
     * this curve.
     * - it includes internal and external intersecting geometry.
     * - it returns GeoEnum::GeoUndef if no intersection is found.
     */
    bool seekTrimPoints(int GeoId,
                        const Base::Vector3d& point,
                        int& GeoId1,
                        Base::Vector3d& intersect1,
                        int& GeoId2,
                        Base::Vector3d& intersect2);

public:
    // Analyser functions
    int autoConstraint(double precision = Precision::Confusion() * 1000,
                       double angleprecision = M_PI / 20,
                       bool includeconstruction = true);

    int detectMissingPointOnPointConstraints(double precision = Precision::Confusion() * 1000,
                                             bool includeconstruction = true);
    void analyseMissingPointOnPointCoincident(double angleprecision = M_PI / 8);
    int detectMissingVerticalHorizontalConstraints(double angleprecision = M_PI / 8);
    int detectMissingEqualityConstraints(double precision);

    std::vector<ConstraintIds>& getMissingPointOnPointConstraints();
    std::vector<ConstraintIds>& getMissingVerticalHorizontalConstraints();
    std::vector<ConstraintIds>& getMissingLineEqualityConstraints();
    std::vector<ConstraintIds>& getMissingRadiusConstraints();

    void setMissingRadiusConstraints(std::vector<ConstraintIds>& cl);
    void setMissingLineEqualityConstraints(std::vector<ConstraintIds>& cl);
    void setMissingVerticalHorizontalConstraints(std::vector<ConstraintIds>& cl);
    void setMissingPointOnPointConstraints(std::vector<ConstraintIds>& cl);

    void makeMissingPointOnPointCoincident(bool onebyone = false);
    void makeMissingVerticalHorizontal(bool onebyone = false);
    void makeMissingEquality(bool onebyone = true);

    // helper
    /// returns the number of redundant constraints detected
    int autoRemoveRedundants(bool updategeo = true);

    int renameConstraint(int GeoId, std::string name);

    // Validation routines
    std::vector<Base::Vector3d> getOpenVertices() const;

public:  // geometry extension functionalities for single element sketch object user convenience
    int setGeometryId(int GeoId, long id);
    int getGeometryId(int GeoId, long& id) const;

protected:
    /// get called by the container when a property has changed
    void onChanged(const App::Property* /*prop*/) override;
    void onDocumentRestored() override;
    void restoreFinished() override;

    void buildShape();

    std::string validateExpression(const App::ObjectIdentifier& path,
                                   std::shared_ptr<const App::Expression> expr);

    void constraintsRenamed(const std::map<App::ObjectIdentifier, App::ObjectIdentifier>& renamed);
    void constraintsRemoved(const std::set<App::ObjectIdentifier>& removed);
    /*!
     \brief Returns a list of supported geometries from the input list
     \param geoList - the geometry list
     \retval list - the supported geometry list
     */
    std::vector<Part::Geometry*>
    supportedGeometry(const std::vector<Part::Geometry*>& geoList) const;


    /*!
     \brief Transfer constraints on lines being filleted.

     Since filleting moves the endpoints of the input geometry, existing constraints may no longer
     be sensible. If fillet() was called with preserveCorner=false, the constraints are simply
     deleted. But if the lines are coincident and preserveCorner=true, we can preserve most
     constraints on the old end points by moving them to the preserved corner, or transforming
     distance constraints on straight lines into point-to-point distance constraints.

     \param geoId1, podId1, geoId2, posId2 - The two lines that have just been filleted
     */
    void transferFilletConstraints(int geoId1, PointPos posId1, int geoId2, PointPos posId2);

    // refactoring functions
    // check whether constraint may be changed driving status
    int testDrivingChange(int ConstrId, bool isdriving);

    void onUndoRedoFinished() override;

    // migration functions
    void migrateSketch();

    static void appendConstraintsMsg(const std::vector<int>& vector,
                                     const std::string& singularmsg,
                                     const std::string& pluralmsg,
                                     std::string& msg);

    // retrieves redundant, conflicting and malformed constraint information from the solver
    void retrieveSolverDiagnostics();

    // retrieves whether a geometry blocked state corresponds to this constraint
    // returns true of the constraint is of Block type, false otherwise
    bool getBlockedState(const Constraint* cstr, bool& blockedstate) const;

    // retrieves the geometry blocked state corresponding to this constraint
    // returns true of the constraint is of InternalAlignment type, false otherwise
    bool getInternalTypeState(const Constraint* cstr,
                              Sketcher::InternalType::InternalType& internaltypestate) const;

    // Checks whether the geometry state stored in the geometry extension matches the current
    // sketcher situation (e.g. constraints) and corrects the state if not matching.
    void synchroniseGeometryState();

    // helper function to create a new constraint and move it to the Constraint Property
    void addConstraint(Sketcher::ConstraintType constrType,
                       int firstGeoId,
                       Sketcher::PointPos firstPos,
                       int secondGeoId = GeoEnum::GeoUndef,
                       Sketcher::PointPos secondPos = Sketcher::PointPos::none,
                       int thirdGeoId = GeoEnum::GeoUndef,
                       Sketcher::PointPos thirdPos = Sketcher::PointPos::none);

    // creates a new constraint
    std::unique_ptr<Constraint>
    createConstraint(Sketcher::ConstraintType constrType,
                     int firstGeoId,
                     Sketcher::PointPos firstPos,
                     int secondGeoId = GeoEnum::GeoUndef,
                     Sketcher::PointPos secondPos = Sketcher::PointPos::none,
                     int thirdGeoId = GeoEnum::GeoUndef,
                     Sketcher::PointPos thirdPos = Sketcher::PointPos::none);

private:
    /// Flag to allow external geometry from other bodies than the one this sketch belongs to
    bool allowOtherBody;

    /// Flag to allow carbon copy from misaligned geometry
    bool allowUnaligned;

    std::vector<Part::Geometry*> ExternalGeo;

    std::vector<int> VertexId2GeoId;
    std::vector<PointPos> VertexId2PosId;

    Sketch solvedSketch;

    /** this internal flag indicate that an operation modifying the geometry, but not the DoF of the
       sketch took place (e.g. toggle construction), so if next action is a movement of a point
       (movePoint), the geometry must be updated first.
    */
    bool solverNeedsUpdate;

    int lastDoF;
    bool lastHasConflict;
    bool lastHasRedundancies;
    bool lastHasPartialRedundancies;
    bool lastHasMalformedConstraints;
    int lastSolverStatus;
    float lastSolveTime;

    std::vector<int> lastConflicting;
    std::vector<int> lastRedundant;
    std::vector<int> lastPartiallyRedundant;
    std::vector<int> lastMalformedConstraints;

    boost::signals2::scoped_connection constraintsRenamedConn;
    boost::signals2::scoped_connection constraintsRemovedConn;

    bool AutoLockTangencyAndPerpty(Constraint* cstr, bool bForce = false, bool bLock = true);

    // Geometry Extensions is used to store on geometry a state that is enforced by pre-existing
    // constraints Like Block constraint and InternalAlignment constraint. This enables (more)
    // convenient handling in ViewProviderSketch and solver.
    //
    // These functions are responsible for updating the Geometry State, currently Geometry Mode
    // (Blocked) and Geometry InternalType (BSplineKnot, BSplinePole).
    //
    // The data life model for handling this state is as follows:
    // 1. Upon restore, any migration is handled to set the status for legacy files (backwards
    // compatibility)
    // 2. Functionality adding constraints (of the relevant type) calls addGeometryState to set the
    // status
    // 3. Functionality removing constraints (of the relevant type) calls removeGeometryState to
    // remove the status
    // 4. Save mechanism will ensure persistence.
    void addGeometryState(const Constraint* cstr) const;
    void removeGeometryState(const Constraint* cstr) const;

    SketchAnalysis* analyser;

    bool internaltransaction;

    // indicates whether changes to properties are the deed of SketchObject or not (for input
    // validation)
    bool managedoperation;
};

inline int SketchObject::initTemporaryMove(int geoId, PointPos pos, bool fine /*=true*/)
{
    // if a previous operation did not update the geometry (including geometry extensions)
    // or constraints (including any deleted pointer, as in renameConstraint) of the solver,
    // here we update them before starting a temporary operation.
    if (solverNeedsUpdate) {
        solve();
    }

    return solvedSketch.initMove(geoId, pos, fine);
}

inline int SketchObject::initTemporaryBSplinePieceMove(int geoId,
                                                       PointPos pos,
                                                       const Base::Vector3d& firstPoint,
                                                       bool fine)
{
    // if a previous operation did not update the geometry (including geometry extensions)
    // or constraints (including any deleted pointer, as in renameConstraint) of the solver,
    // here we update them before starting a temporary operation.
    if (solverNeedsUpdate) {
        solve();
    }

    return solvedSketch.initBSplinePieceMove(geoId, pos, firstPoint, fine);
}

inline int SketchObject::moveTemporaryPoint(int geoId,
                                            PointPos pos,
                                            Base::Vector3d toPoint,
                                            bool relative /*=false*/)
{
    return solvedSketch.movePoint(geoId, pos, toPoint, relative);
}

template<typename GeometryT, typename>
const GeometryT* SketchObject::getGeometry(int GeoId) const
{
    if (GeoId >= 0) {
        const std::vector<Part::Geometry*>& geomlist = getInternalGeometry();
        if (GeoId < int(geomlist.size())) {
            return static_cast<GeometryT*>(geomlist[GeoId]);
        }
    }
    else if (-GeoId <= int(ExternalGeo.size())) {
        return static_cast<GeometryT*>(ExternalGeo[-GeoId - 1]);
    }

    return nullptr;
}

using SketchObjectPython = App::FeaturePythonT<SketchObject>;

}  // namespace Sketcher


#endif  // SKETCHER_SKETCHOBJECT_H
