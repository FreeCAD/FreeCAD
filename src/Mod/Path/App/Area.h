/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#include <QCoreApplication>
#include <chrono>
#include <memory>
#include <vector>
#include <list>
#include <TopoDS.hxx>
#include <gp_Pln.hxx>
#include <gp_Circ.hxx>
#include <gp_GTrsf.hxx>

#include <Base/Console.h>
#include "Path.h"
#include "AreaParams.h"

#define _AREA_LOG(_l,_msg) do {\
    if(Area::_l##Enabled()){\
        std::stringstream str;\
        str << "Path.Area: " << _msg;\
        Base::Console()._l("%s\n",str.str().c_str());\
    }\
    QCoreApplication::sendPostedEvents();\
    if(Area::aborting()) {\
        Area::abort(false);\
        throw Base::AbortException("operation aborted");\
    }\
}while(0)

#define AREA_LOG(_msg) _AREA_LOG(Log,_msg)
#define AREA_WARN(_msg) _AREA_LOG(Warning,_msg)
#define AREA_ERR(_msg) _AREA_LOG(Error,_msg)
#define AREA_PT(_pt) '('<<(_pt).X()<<", " << (_pt).Y()<<", " << (_pt).Z()<<')'
#define AREA_PT2(_pt) '('<<(_pt).x<<", " << (_pt).y<<')'

#define AREA_TRACE(_msg) do{\
    if(Area::TraceEnabled()) AREA_LOG('('<<__LINE__<<"): " <<_msg);\
}while(0)

#define AREA_TIME_ENABLE

#ifdef AREA_TIME_ENABLE
#define TIME_UNIT duration<double>
#define TIME_CLOCK high_resolution_clock
#define TIME_POINT std::chrono::TIME_CLOCK::time_point

#define TIME_INIT(_t) \
    auto _t=std::chrono::TIME_CLOCK::now()

#define TIME_INIT2(_t1,_t2) TIME_INIT(_t1),_t2=_t1
#define TIME_INIT3(_t1,_t2,_t3) TIME_INIT(_t1),_t2=_t1,_t3=_t1

#define _DURATION_PRINT(_l,_d,_msg) \
    AREA_##_l(_msg<< " time: " << _d.count()<<'s');

#define DURATION_PRINT(_d,_msg) _DURATION_PRINT(LOG,_d,_msg)

#define TIME_PRINT(_t,_msg) \
    DURATION_PRINT(Path::getDuration(_t),_msg);

#define TIME_TRACE(_t,_msg) \
    _DURATION_PRINT(TRACE,Path::getDuration(_t),_msg);

#define DURATION_INIT(_d) \
    std::chrono::TIME_UNIT _d(0)

#define DURATION_INIT2(_d1,_d2) DURATION_INIT(_d1),_d2(0)

namespace Path {
inline std::chrono::TIME_UNIT getDuration(TIME_POINT &t)
{
    auto tnow = std::chrono::TIME_CLOCK::now();
    auto d = std::chrono::duration_cast<std::chrono::TIME_UNIT>(tnow-t);
    t = tnow;
    return d;
}
}

#define DURATION_PLUS(_d,_t) _d += Path::getDuration(_t)

#else

#define TIME_INIT(...) do{}while(0)
#define TIME_INIT2(...) do{}while(0)
#define TIME_INIT3(...) do{}while(0)
#define TIME_PRINT(...) do{}while(0)
#define DURATION_PRINT(...) do{}while(0)
#define DURATION_INIT(...) do{}while(0)
#define DURATION_INIT2(...) do{}while(0)
#define DURATION_PLUS(...) do{}while(0)

#endif

class CArea;
class CCurve;

namespace Path
{

/** Store libarea algorithm configuration */
struct PathExport CAreaParams {
    PARAM_DECLARE(PARAM_FNAME,AREA_PARAMS_CAREA)
    CAreaParams();
};

/** Store all Area configurations */
struct PathExport AreaParams: CAreaParams {

    PARAM_DECLARE(PARAM_FNAME,AREA_PARAMS_AREA)

    bool operator==(const AreaParams &other) const {
#define AREA_COMPARE(_param) \
         if(PARAM_FIELD(NAME,_param)!=other.PARAM_FIELD(NAME,_param)) return false;
        PARAM_FOREACH(AREA_COMPARE,AREA_PARAMS_CAREA)
        PARAM_FOREACH(AREA_COMPARE,AREA_PARAMS_AREA)
        return true;
    }
    bool operator!=(const AreaParams &other) const {
        return !(*this == other);
    }

    AreaParams();
};

struct PathExport AreaStaticParams: AreaParams {
    PARAM_DECLARE(PARAM_FNAME,AREA_PARAMS_EXTRA_CONF);

    AreaStaticParams();
};

/** libarea configurator
 *
 * It is kind of troublesome with the fact that libarea uses static variables to
 * config its algorithm. CAreaConfig makes it easy to safely customize libarea.
 */
struct PathExport CAreaConfig {

    /** For saving current libarea settings */
    PARAM_DECLARE(PARAM_FNAME,AREA_PARAMS_CAREA)

    /** The constructor automatically saves current setting and apply user defined ones 
     *
     * \arg \c p user defined configurations
     * \arg \c noFitArgs if true, will override and disable arc fitting. Because
     * arc unfiting and fitting is lossy. And repeatedly perform these operation
     * may cause shape deformation. So it is best to delay arc fitting until the
     * final step*/
    CAreaConfig(const CAreaParams &p, bool noFitArcs=true);

    /** The destructor restores the setting, and thus exception safe.  */
    ~CAreaConfig();
};


/** Base class for FreeCAD wrapping of libarea */
class PathExport Area: public Base::BaseClass {

    TYPESYSTEM_HEADER();

public:
    struct Shape {
        short op;
        TopoDS_Shape shape;

        Shape(short opCode, const TopoDS_Shape &s)
            :op(opCode)
            ,shape(s)
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
    int mySkippedShapes;

    static bool s_aborting;
    static AreaStaticParams s_params;

    /** Called internally to combine children shapes for further processing */
    void build();

    /** Called by build() to add children shape
     *
     * Mainly for checking if there is any faces for auto fill*/
    void addToBuild(CArea &area, const TopoDS_Shape &shape);

    /** Called internally to obtain the combained children shapes */
    TopoDS_Shape toShape(CArea &area, short fill);

    /** Obtain a list of offseted areas
     *
     * See #AREA_PARAMS_OFFSET for description of the arguments.
     */
    void makeOffset(std::list<std::shared_ptr<CArea> > &areas,
                    PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_OFFSET));

    /** Make a pocket of the combined shape
     *
     * User #AREA_PARAMS_POCKET setting in myParams.
     */
    TopoDS_Shape makePocket();

    void explode(const TopoDS_Shape &shape);

    bool isBuilt() const;

    TopoDS_Shape findPlane(const TopoDS_Shape &shape, gp_Trsf &trsf);

public:
    /** Declare all parameters defined in #AREA_PARAMS_ALL as member variable */
    PARAM_ENUM_DECLARE(AREA_PARAMS_ALL)

    Area(const AreaParams *params = NULL);
    Area(const Area &other, bool deep_copy=true);
    virtual ~Area();

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
    void setPlane(const TopoDS_Shape &shape);

    /** Return the current active workplane
     *
     * \arg \c trsf: optional return of a transformation matrix that will bring the
     * found plane to XY0 plane.
     *
     * If no workplane is set using setPlane(), the active workplane is derived from
     * the added children shapes using the same algorithm empolyed by setPlane().
     */
    TopoDS_Shape getPlane(gp_Trsf *trsf = NULL);

    /** Add a child shape with given operation code 
     *
     * No validation is done at this point. Exception will be thrown when asking
     * for output shape, if any of the children shapes is not valid or not
     * coplanar
     *
     * \arg \c shape: the child shape
     * \arg \c op: operation code, see #AREA_PARAMS_OPCODE
     */
    void add(const TopoDS_Shape &shape,PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_OPCODE));


    /** Generate an offset of the combined shape
     *
     * See #AREA_PARAMS_OFFSET for description of the arguments.
     * If more than one offset is requested, a compound shape is return
     * containing all offset shapes as wires regardless of \c Fill setting.
     */
    TopoDS_Shape makeOffset(int index=-1, PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_OFFSET));

    /** Make a pocket of the combined shape
     *
     * See #AREA_PARAMS_POCKET for description of the arguments.
     */
    TopoDS_Shape makePocket(int index=-1, PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_POCKET));


    std::vector<std::shared_ptr<Area> > makeSections(
            PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_SECTION_EXTRA),
            const std::vector<double> &_heights = std::vector<double>(),
            const TopoDS_Shape &plane = TopoDS_Shape());

    /** Config this Area object */
    void setParams(const AreaParams &params);


    const std::list<Shape> getChildren() const {
        return myShapes;
    }

    /** Get the current configuration */
    const AreaParams &getParams() const {
        return myParams;
    }

    /** Clean internal caches
     *
     * The combained shapes is cached internally to make other operation more
     * efficient, such as makeOffset() and makePocket()
     *
     * \arg \c deleteShapes: if true, delete all children shapes.
     */
    void clean(bool deleteShapes=false);

    /** Get the combined shape
     * \arg \c index: index of the section, -1 for all sections. No effect on
     * non-sectioned area.
     */
    TopoDS_Shape getShape(int index=-1);

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
    static void add(CArea &area, const TopoDS_Wire &wire, const gp_Trsf *trsf=NULL, 
            double deflection=0.01, bool to_edges=false); 

    /** Output a list or sorted wire with minimize traval distance
     *
     * \arg \c index: index of the section, -1 for all sections. No effect on
     * non-sectioned area.
     * \arg \c count: number of the sections to return, <=0 for all sections
     * after \c index. No effect on non-sectioned area.
     * \arg \c pstart: optional start point
     * \arg \c pend: optional output containing the ending point of the returned
     * wires
     * \arg \c allow_Break: whether allow to break open wires
     *
     * See #AREA_PARAMS_SORT for other arguments
     *
     * \return sorted wires
     * */
    std::list<TopoDS_Shape> sortWires(int index=-1, int count=0, 
            const gp_Pnt *pstart=NULL, gp_Pnt *pend=NULL,
            PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_SORT));

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
    static int add(CArea &area, const TopoDS_Shape &shape, const gp_Trsf *trsf=NULL,
            double deflection=0.01,const TopoDS_Shape *plane = NULL,
            bool force_coplanar=true, CArea *areaOpen=NULL, bool to_edges=false, 
            bool reorient=true);

    /** Convert curves in CArea into an OCC shape
     *
     * \arg \c area: input area object
     * \arg \c fill: if true, create a face object from the wires
     * \arg \c trsf: optional transform matrix to transform the shape back into
     * its original position.
     * */
    static TopoDS_Shape toShape(const CArea &area, bool fill, 
            const gp_Trsf *trsf=NULL);

    /** Convert a single curve into an OCC wire
     *
     * \arg \c curve: input curve object
     * \arg \c trsf: optional transform matrix to transform the shape back into
     * its original position.
     * */
    static TopoDS_Wire toShape(const CCurve &curve, const gp_Trsf *trsf=NULL);

    /** Check if two OCC shape is coplanar */
    static bool isCoplanar(const TopoDS_Shape &s1, const TopoDS_Shape &s2);

    /** Group shapes by their plane, and return a list of sorted wires
     *
     * The output wires is ordered by its occupied plane, and sorted to
     * minimize traval distance
     *
     * \arg \c shapes: input list of shapes.
     * \arg \c params: optional Area parameters for the Area object internally
     * used for sorting
     * \arg \c pstart: optional start point
     * \arg \c pend: optional output containing the ending point of the returned
     *
     * See #AREA_PARAMS_SORT for other arguments
     *
     * \return sorted wires
     */
    static std::list<TopoDS_Shape> sortWires(const std::list<TopoDS_Shape> &shapes,
            const AreaParams *params = NULL, const gp_Pnt *pstart=NULL, 
            gp_Pnt *pend=NULL, PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_SORT));

    /** Convert a list of wires to gcode
     *
     * \arg \c path: output toolpath
     * \arg \c shapes: input list of shapes
     * \arg \c params: optional Area parameters for the Area object internally
     * used for sorting
     * \arg \c pstart: output start point,
     * \arg \c pend: optional output containing the ending point of the returned
     * 
     * See #AREA_PARAMS_PATH for other arguments
     */
    static void toPath(Toolpath &path, const std::list<TopoDS_Shape> &shapes,
            const AreaParams *params=NULL, const gp_Pnt *pstart=NULL, gp_Pnt *pend=NULL,
            PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_PATH));

    PARAM_ENUM_DECLARE(AREA_PARAMS_PATH)

    static void abort(bool aborting);
    static bool aborting();

    static void setDefaultParams(const AreaStaticParams &params);
    static const AreaStaticParams &getDefaultParams();

#define AREA_LOG_CHECK_DECLARE(_1,_2,_elem) \
    static bool BOOST_PP_CAT(_elem,Enabled)();
    BOOST_PP_SEQ_FOR_EACH(AREA_LOG_CHECK_DECLARE,_,AREA_PARAM_LOG_LEVEL)

    PARAM_ENUM_DECLARE(AREA_PARAMS_LOG_LEVEL)
};

} //namespace Path

#endif //PATH_AREA_H
