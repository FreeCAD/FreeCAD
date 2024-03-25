/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#ifndef PATH_AREA_H
#define PATH_AREA_H

#include <chrono>
#include <list>
#include <memory>
#include <vector>

#include <TopoDS.hxx>

#include <Mod/CAM/PathGlobal.h>
#include <Mod/Part/App/PartPyCXX.h>
#include <Mod/Part/App/TopoShape.h>

#include "AreaParams.h"
#include "Path.h"


class CArea;
class CCurve;
class Bnd_Box;

namespace Path
{

/** Store libarea algorithm configuration */
struct PathExport CAreaParams {
    PARAM_DECLARE(PARAM_FNAME, AREA_PARAMS_CAREA)
    CAreaParams();
};

/** Store all Area configurations */
struct PathExport AreaParams: CAreaParams {

    PARAM_DECLARE(PARAM_FNAME, AREA_PARAMS_AREA)

    bool operator==(const AreaParams& other) const {
#define AREA_COMPARE(_param) \
         if(PARAM_FIELD(NAME,_param)!=other.PARAM_FIELD(NAME,_param)) return false;
        PARAM_FOREACH(AREA_COMPARE, AREA_PARAMS_CAREA);
        PARAM_FOREACH(AREA_COMPARE, AREA_PARAMS_AREA);
        return true;
    }
    bool operator!=(const AreaParams& other) const {
        return !(*this == other);
    }

    void dump(const char*) const;

    AreaParams();
};

struct PathExport AreaStaticParams: AreaParams {
    AreaStaticParams();
};

/** libarea configurator
 *
 * It is kind of troublesome with the fact that libarea uses static variables to
 * config its algorithm. CAreaConfig makes it easy to safely customize libarea.
 */
struct PathExport CAreaConfig {

    /** For saving current libarea settings */
    PARAM_DECLARE(PARAM_FNAME, AREA_PARAMS_CAREA)

    /** The constructor automatically saves current setting and apply user defined ones
     *
     * \arg \c p user defined configurations
     * \arg \c noFitArgs if true, will override and disable arc fitting. Because
     * arc unfiting and fitting is lossy. And repeatedly perform these operation
     * may cause shape deformation. So it is best to delay arc fitting until the
     * final step*/
    explicit CAreaConfig(const CAreaParams& p, bool noFitArcs = true);

    /** The destructor restores the setting, and thus exception safe.  */
    ~CAreaConfig();
};


/** Base class for FreeCAD wrapping of libarea */
class PathExport Area: public Base::BaseClass {

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    struct Shape {
        short op;
        TopoDS_Shape shape;

        Shape(short opCode, const TopoDS_Shape& s)
            :op(opCode)
            , shape(s)
        {}
    };

protected:
    std::list<Shape> myShapes;
    std::unique_ptr<CArea> myArea;
    std::unique_ptr<CArea> myAreaOpen;
    gp_Trsf myTrsf;
    AreaParams myParams;
    TopoDS_Shape myShapePlane;
    TopoDS_Shape myWorkPlane;
    TopoDS_Shape myShape;
    std::vector<std::shared_ptr<Area> > mySections;
    bool myHaveFace;
    bool myHaveSolid;
    bool myShapeDone;
    bool myProjecting;
    mutable int mySkippedShapes;

    static bool s_aborting;
    static AreaStaticParams s_params;

    /** Called internally to combine children shapes for further processing */
    void build();

    /** Called by build() to add children shape
     *
     * Mainly for checking if there is any faces for auto fill*/
    void addToBuild(CArea& area, const TopoDS_Shape& shape);

    /** Called internally to obtain the combined children shapes */
    TopoDS_Shape toShape(CArea& area, short fill, int reorient = 0);

    /** Obtain a list of offset areas
     *
     * See #AREA_PARAMS_OFFSET for description of the arguments.
     */
    void makeOffset(std::list<std::shared_ptr<CArea> >& areas,
        PARAM_ARGS_DEF(PARAM_FARG, AREA_PARAMS_OFFSET), bool from_center = false);

    /** Make a pocket of the combined shape
     *
     * User #AREA_PARAMS_POCKET setting in myParams.
     */
    TopoDS_Shape makePocket();

    void explode(const TopoDS_Shape& shape);

    TopoDS_Shape findPlane(const TopoDS_Shape& shape, gp_Trsf& trsf);

    std::list<Shape> getProjectedShapes(const gp_Trsf& trsf, bool inverse = true) const;

public:
    /** Declare all parameters defined in #AREA_PARAMS_ALL as member variable */
    PARAM_ENUM_DECLARE(AREA_PARAMS_ALL)

    explicit Area(const AreaParams* params = nullptr);
    Area(const Area& other, bool deep_copy = true);
    ~Area() override;

    bool isBuilt() const;

    /** Set a working plane
     *
     * \arg \c shape: a shape defining a working plane.
     *
     * The supplied shape does not need to be planar. Area will try to find planar
     * sub-shape (face, wire or edge). If more than one planar sub-shape is found,
     * it will prefer the top plane parallel to XY0 plane.
     *
     * If no working plane are set, Area will try to find a working plane from
     * the added children shape using the same algorithm
     */
    void setPlane(const TopoDS_Shape& shape);

    /** Return the current active workplane
     *
     * \arg \c trsf: optional return of a transformation matrix that will bring the
     * found plane to XY0 plane.
     *
     * If no workplane is set using setPlane(), the active workplane is derived from
     * the added children shapes using the same algorithm empolyed by setPlane().
     */
    TopoDS_Shape getPlane(gp_Trsf* trsf = nullptr);

    /** Add a child shape with given operation code
     *
     * No validation is done at this point. Exception will be thrown when asking
     * for output shape, if any of the children shapes is not valid or not
     * coplanar
     *
     * \arg \c shape: the child shape
     * \arg \c op: operation code, see #AREA_PARAMS_OPCODE
     */
    void add(const TopoDS_Shape& shape, PARAM_ARGS_DEF(PARAM_FARG, AREA_PARAMS_OPCODE));


    /** Generate an offset of the combined shape
     *
     * See #AREA_PARAMS_OFFSET for description of the arguments.
     * If more than one offset is requested, a compound shape is return
     * containing all offset shapes as wires regardless of \c Fill setting.
     */
    TopoDS_Shape makeOffset(int index = -1, PARAM_ARGS_DEF(PARAM_FARG, AREA_PARAMS_OFFSET),
        int reoirent = 0, bool from_center = false);

    /** Make a pocket of the combined shape
     *
     * See #AREA_PARAMS_POCKET for description of the arguments.
     */
    TopoDS_Shape makePocket(int index = -1, PARAM_ARGS_DEF(PARAM_FARG, AREA_PARAMS_POCKET));

    /** Make a pocket of the combined shape
     *
     * \arg \c heights: optional customized heights of each section. The
     * meaning of each height depends on section mode. If none is given,
     * the section heights is determined by the section settings in this
     * Area object (configured through setParams()).
     * \arg \c plane: the section plane if the section mode is
     * SectionModeWorkplane, otherwise ignored
     *
     * See #AREA_PARAMS_EXTRA for description of the arguments. Currently, there
     * is only one argument, namely \c mode for section mode.
     */
    std::vector<std::shared_ptr<Area> > makeSections(
        PARAM_ARGS_DEF(PARAM_FARG, AREA_PARAMS_SECTION_EXTRA),
        const std::vector<double>& heights = std::vector<double>(),
        const TopoDS_Shape& plane = TopoDS_Shape());

    std::shared_ptr<Area> getClearedArea(const Toolpath *path, double diameter, double zmax, Base::BoundBox3d bbox);
    std::shared_ptr<Area> getRestArea(std::vector<std::shared_ptr<Area>> clearedAreas, double diameter);
    TopoDS_Shape toTopoShape();

    /** Config this Area object */
    void setParams(const AreaParams& params);


    const std::list<Shape> getChildren() const {
        return myShapes;
    }

    /** Get the current configuration */
    const AreaParams& getParams() const {
        return myParams;
    }

    /** Clean internal caches
     *
     * The combined shapes is cached internally to make other operation more
     * efficient, such as makeOffset() and makePocket()
     *
     * \arg \c deleteShapes: if true, delete all children shapes.
     */
    void clean(bool deleteShapes = false);

    /** Get the combined shape
     * \arg \c index: index of the section, -1 for all sections. No effect on
     * non-sectioned area.
     */
    TopoDS_Shape getShape(int index = -1);

    /** Return the number of sections */
    std::size_t getSectionCount() {
        build();
        return mySections.size();
    }

    /** Add a OCC wire shape to CArea
     *
     * \arg \c area: output converted curved object to here
     * \arg \c wire: input wire object
     * \arg \c trsf: optional transform matrix to transform the wire shape into
     * XY0 plane.
     * \arg \c deflection: for discretizing non circular curves
     * \arg \c to_edges: if true, discretize all curves, and insert as open
     * line segments
     * */
    static void addWire(CArea& area, const TopoDS_Wire& wire, const gp_Trsf* trsf = nullptr,
        double deflection = 0.01, bool to_edges = false);

    /** Add a OCC generic shape to CArea
     *
     * \arg \c area: output converted curved object to here
     * \arg \c shape: input shape object
     * \arg \c trsf: optional transform matrix to transform the wire shape into
     * XY0 plane.
     * \arg \c deflection: for defecting non circular curves
     * \arg \c plane: a shape for testing coplanar
     * \arg \c force_coplaner: if true, discard non-coplanar shapes.
     * \arg \c areaOpen: for collecting open curves. If not supplied, open
     * curves are added to \c area
     * \arg \c to_edges: separate open wires to individual edges
     * \arg \c reorient: reorient closed wires for wire only shape
     *
     * \return Returns the number of non coplaner. Planar testing only happens
     * if \c plane is supplied
     * */
    static int addShape(CArea& area, const TopoDS_Shape& shape, const gp_Trsf* trsf = nullptr,
        double deflection = 0.01, const TopoDS_Shape* plane = nullptr,
        bool force_coplanar = true, CArea* areaOpen = nullptr, bool to_edges = false,
        bool reorient = true);

    /** Convert curves in CArea into an OCC shape
     *
     * \arg \c area: input area object
     * \arg \c fill: if true, create a face object from the wires
     * \arg \c trsf: optional transform matrix to transform the shape back into
     * its original position.
     * */
    static TopoDS_Shape toShape(const CArea& area, bool fill,
        const gp_Trsf* trsf = nullptr, int reoirent = 0);

    /** Convert a single curve into an OCC wire
     *
     * \arg \c curve: input curve object
     * \arg \c trsf: optional transform matrix to transform the shape back into
     * its original position.
     * */
    static TopoDS_Shape toShape(const CCurve& curve, const gp_Trsf* trsf = nullptr, int reorient = 0);

    /** Check if two OCC shape is coplanar */
    static bool isCoplanar(const TopoDS_Shape& s1, const TopoDS_Shape& s2);

    /** Group shapes by their plane, and return a list of sorted wires
     *
     * The output wires is ordered by its occupied plane, and sorted to
     * minimize traval distance
     *
     * \arg \c shapes: input list of shapes.
     * \arg \c has_start: if false or pstart is 0, then a start point will be
     * auto selected.
     * \arg \c pstart: optional start point. If has_start is false, then the
     * auto selected start point will be returned with this point if not NULL.
     * \arg \c pend: optional output containing the ending point of the returned
     * \arg \c stepdown_hint: optional output of a hint of step down as the max
     * distance between two sections.
     * \arg \c arc_plane: optional arc plane selection, if given the found plane
     * will be returned. See #AREA_PARAMS_ARC_PLANE for more details.
     *
     * See #AREA_PARAMS_SORT for other arguments
     *
     * \return sorted wires
     */
    static std::list<TopoDS_Shape> sortWires(const std::list<TopoDS_Shape>& shapes,
        bool has_start = false, gp_Pnt* pstart = nullptr, gp_Pnt* pend = nullptr, double* stepdown_hint = nullptr,
        short* arc_plane = nullptr, PARAM_ARGS_DEF(PARAM_FARG, AREA_PARAMS_SORT));

    /** Convert a list of wires to gcode
     *
     * \arg \c path: output toolpath
     * \arg \c shapes: input list of shapes
     * \arg \c pstart: output start point,
     * \arg \c pend: optional output containing the ending point of the returned
     *
     * See #AREA_PARAMS_PATH for other arguments
     */
    static void toPath(Toolpath& path, const std::list<TopoDS_Shape>& shapes,
        const gp_Pnt* pstart = nullptr, gp_Pnt* pend = nullptr,
        PARAM_ARGS_DEF(PARAM_FARG, AREA_PARAMS_PATH));

    static int project(TopoDS_Shape& out, const TopoDS_Shape& in,
        const AreaParams* params = nullptr,
        const TopoDS_Shape* work_plane = nullptr);

    static void setWireOrientation(TopoDS_Wire& wire, const gp_Dir& dir, bool ccw);

    PARAM_ENUM_DECLARE(AREA_PARAMS_PATH)

    static void abort(bool aborting);
    static bool aborting();

    static void setDefaultParams(const AreaStaticParams& params);
    static const AreaStaticParams& getDefaultParams();

    static void showShape(const TopoDS_Shape& shape, const char* name, const char* fmt = nullptr, ...);
};

} //namespace Path

#endif //PATH_AREA_H
