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

#include <memory>
#include <vector>
#include <list>
#include <TopoDS.hxx>
#include <gp_Pln.hxx>
#include <gp_Circ.hxx>
#include <gp_GTrsf.hxx>

#include "Path.h"
#include "AreaParams.h"

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
        PARAM_FOREACH(AREA_COMPARE,AREA_PARAMS_CONF)
        return true;
    }
    bool operator!=(const AreaParams &other) const {
        return !(*this == other);
    }

    AreaParams();
};

/** libarea configurator
 *
 * It is kind of troublesome with the fact that libarea uses static variables to
 * config its algorithm. CAreaConfig makes it easy to safely customize libarea.
 */
struct PathExport CAreaConfig {

    /** Stores current libarea settings */
    PARAM_DECLARE(PARAM_FNAME,AREA_PARAMS_CAREA)

    /** Stores user defined setting */
    CAreaParams params;

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

protected:

    struct Shape {
        short op;
        TopoDS_Shape shape;

        Shape(short opCode, const TopoDS_Shape &s)
            :op(opCode)
            ,shape(s)
        {}
    };

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

public:
    /** Declare all parameters defined in #AREA_PARAMS_ALL as member variable */
    PARAM_ENUM_DECLARE(AREA_PARAMS_ALL)

    Area(const AreaParams *params = NULL);
    Area(const Area &other, bool deep_copy=true);
    virtual ~Area();

    /** Set a working plane 
     *
     * If no working plane are set, Area will try to find a working plane from
     * individual children faces, wires or edges. By right, we should create a
     * compound of all shapes and then findplane on it. However, because we
     * supports solid, and also because OCC may hang for a long time if
     * something goes a bit off, we opt to find plane on each individual shape.
     * If you intend to pass individual edges, you must supply a workplane shape
     * manually
     *
     * \arg \c shape: a shape defining a working plane
     */
    void setPlane(const TopoDS_Shape &shape);

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


    /** Config this Area object */
    void setParams(const AreaParams &params);


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
     *
     * See #AREA_PARAMS_MIN_DIST for other arguments
     *
     * \return sorted wires
     * */
    std::list<TopoDS_Shape> sortWires(int index=-1, int count=0, 
            const gp_Pnt *pstart=NULL, gp_Pnt *pend=NULL, 
            PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_MIN_DIST));

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
     * maybe broken if the algorithm see fits.
     *
     * See #AREA_PARAMS_SORT_WIRES for other arguments
     *
     * \return sorted wires
     */
    static std::list<TopoDS_Shape> sortWires(const std::list<TopoDS_Shape> &shapes,
            const AreaParams *params = NULL, const gp_Pnt *pstart=NULL, 
            gp_Pnt *pend=NULL, PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_SORT_WIRES));

    /** Convert a list of wires to gcode
     *
     * \arg \c path: output toolpath
     * \arg \c shapes: input list of shapes
     * \arg \c pstart: output start point,
     * 
     * See #AREA_PARAMS_PATH for other arguments
     */
    static void toPath(Toolpath &path, const std::list<TopoDS_Shape> &shapes,
            const gp_Pnt *pstart=NULL, PARAM_ARGS_DEF(PARAM_FARG,AREA_PARAMS_PATH));

};

} //namespace Path

#endif //PATH_AREA_H
