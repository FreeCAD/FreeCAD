/***************************************************************************
 *   Copyright (c) 2022 Uwe Stöhr <uwestoehr@lyx.org>                      *
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

#ifndef PART_EXTRUSIONHELPER_H
#define PART_EXTRUSIONHELPER_H

#include <list>
#include <vector>
#include <gp_Dir.hxx>
#include <TopoDS_Shape.hxx>

#include <Mod/Part/PartGlobal.h>


namespace Part
{

class PartExport ExtrusionHelper
{
public:
    ExtrusionHelper();

    /**
     * @brief makeDraft: creates a drafted extrusion shape out of the input 2D shape
     */
    static void makeDraft(const TopoDS_Shape& shape,
                          const gp_Dir& direction,
                          const double LengthFwd,
                          const double LengthRev,
                          const double AngleFwd,
                          const double AngleRev,
                          bool isSolid,
                          std::list<TopoDS_Shape>& drafts,
                          bool isPartDesign);
    /**
     * @brief checkInnerWires: Checks what wires are inner ones by taking a set of prisms created with every wire.
     * The prisms are cut from each other. If the moment of inertia thereby changes, the prism wire is an inner wire.
     * Inner wires can have nested inner wires that are then in fact outer wires.
     * Therefore checkInnerWires is called recursively until all wires are checked.
     */
    static void checkInnerWires(std::vector<bool>& isInnerWire, const gp_Dir direction,
                                std::vector<bool>& checklist, bool forInner, std::vector<TopoDS_Shape> prisms);
    /**
     * @brief createTaperedPrismOffset: creates an offset wire from the sourceWire in the specified
     * translation. isSecond determines if the wire is used for the 2nd extrusion direction.
     */
    static void createTaperedPrismOffset(TopoDS_Wire sourceWire,
                                         const gp_Vec& translation,
                                         double offset,
                                         bool isSecond,
                                         TopoDS_Wire& result);
};

} //namespace Part

#endif // PART_EXTRUSIONHELPER_H
