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

#include <TopoDS.hxx>
#include <gp_Pln.hxx>
#include <gp_Circ.hxx>
#include <gp_GTrsf.hxx>

#include "AreaParams.h"

class CArea;

namespace Path
{

/** Store libarea algorithm configuration */
struct PathExport CAreaParams {
    PARAM_DECLARE(NAME,AREA_PARAMS_CAREA)
    CAreaParams();
};

/** Store all Area configurations */
struct PathExport AreaParams: CAreaParams {

    PARAM_DECLARE(NAME,AREA_PARAMS_BASE)
    PARAM_DECLARE(NAME,AREA_PARAMS_OFFSET_CONF)

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
    PARAM_DECLARE(NAME,AREA_PARAMS_CAREA)

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
    CArea *myArea;
    CArea *myAreaOpen;
    gp_Trsf myTrsf;
    AreaParams myParams;
    TopoDS_Shape myShapePlane;
    TopoDS_Shape myWorkPlane;
    TopoDS_Shape myShape;
    bool myHaveFace;
    int mySkippedShapes;

    /** Called internally to combine children shapes for further processing */
    void build();

    /** Called by build() to add children shape
     *
     * Mainly for checking if there is any faces for auto fill*/
    void addToBuild(CArea &area, const TopoDS_Shape &shape);

    /** Called internally to obtain the combained children shapes */
    TopoDS_Shape toShape(CArea &area, short fill);

public:
    /** Declare all parameters defined in #AREA_PARAMS_ALL as member variable */
    PARAM_ENUM_DECLARE(AREA_PARAMS_ALL)

public:
    Area(const AreaParams *params = NULL);
    virtual ~Area();

    /** Set a working plane 
     *
     * If no working plane are set, Area will try to find a working plane from
     * all the added children shapes. The purpose of this function is in case
     * the child shapes are all colinear edges
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
    void add(const TopoDS_Shape &shape,PARAM_ARGS_DEF(ARG,AREA_PARAMS_OPCODE));

    /** Generate an offset of the combined shape
     *
     * See #AREA_PARAMS_OFFSET for description of the arguments.
     * If more than one offset is requested, a compound shape is return
     * containing all offset shapes as wires regardless of \c Fill setting.
     */
    TopoDS_Shape makeOffset(PARAM_ARGS_DEF(ARG,AREA_PARAMS_OFFSET));

    /** Obtain a list of offset shapes of the combined shape,
     *
     * See #AREA_PARAMS_OFFSET for description of the arguments.
     */
    void makeOffset(std::list<TopoDS_Shape> &shapes,
                    PARAM_ARGS_DEF(ARG,AREA_PARAMS_OFFSET));

    /** Make a pocket of the combined shape
     *
     * See #AREA_PARAMS_POCKET for description of the arguments.
     */
    TopoDS_Shape makePocket(PARAM_ARGS_DEF(ARG,AREA_PARAMS_POCKET));


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

    /** Get the combined shape */
    const TopoDS_Shape &getShape();

    /** Add a OCC wire shape to CArea 
     *
     * \arg \c area: output converted curved object to here
     * \arg \c wire: input wire object
     * \arg \c trsf: optional transform matrix to transform the wire shape into
     * XY0 plane.
     * \arg \c deflection: for defecting non circular curves
     * */
    static void add(CArea &area, const TopoDS_Wire &wire, 
            const gp_Trsf *trsf=NULL, double deflection=0.01);

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
     * \arg \c reorder: reorder closed wires for wire only shape
     *
     * \return Returns the number of non coplaner. Planar testing only happens
     * if \c plane is supplied
     * */
    static int add(CArea &area, const TopoDS_Shape &shape, const gp_Trsf *trsf=NULL,
            double deflection=0.01,const TopoDS_Shape *plane = NULL,
            bool force_coplanar=true, CArea *areaOpen=NULL, bool to_edges=false, 
            bool reorder=true);

    /** Convert curves in CArea into an OCC shape
     *
     * \arg \c area: input area object
     * \arg \c fill: if true, create a face object from the wires
     * \arg \c trsf: optional transform matrix to transform the shape back into
     * its original position.
     * */
    static TopoDS_Shape toShape(const CArea &area, bool fill, 
            const gp_Trsf *trsf=NULL);

    static bool isCoplanar(const TopoDS_Shape &s1, const TopoDS_Shape &s2);
};

} //namespace Path

#endif //PATH_AREA_H
