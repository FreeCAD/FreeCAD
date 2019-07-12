/***************************************************************************
 *   Copyright (c) 2017 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"
#include "PartPyCXX.h"
#include <CXX/Objects.hxx>
#include <Mod/Part/App/TopoShapeFacePy.h>
#include <Mod/Part/App/TopoShapeEdgePy.h>
#include <Mod/Part/App/TopoShapeWirePy.h>
#include <Mod/Part/App/TopoShapeVertexPy.h>
#include <Mod/Part/App/TopoShapeSolidPy.h>
#include <Mod/Part/App/TopoShapeShellPy.h>
#include <Mod/Part/App/TopoShapeCompSolidPy.h>
#include <Mod/Part/App/TopoShapeCompoundPy.h>

namespace Part {
PartExport Py::Object shape2pyshape(const TopoShape &shape)
{
    PyObject* ret = 0;
    if (!shape.isNull()) {
        TopAbs_ShapeEnum type = shape.getShape().ShapeType();
        switch (type)
        {
        case TopAbs_COMPOUND:
            ret = new TopoShapeCompoundPy(new TopoShape(shape));
            break;
        case TopAbs_COMPSOLID:
            ret = new TopoShapeCompSolidPy(new TopoShape(shape));
            break;
        case TopAbs_SOLID:
            ret = new TopoShapeSolidPy(new TopoShape(shape));
            break;
        case TopAbs_SHELL:
            ret = new TopoShapeShellPy(new TopoShape(shape));
            break;
        case TopAbs_FACE:
            ret = new TopoShapeFacePy(new TopoShape(shape));
            break;
        case TopAbs_WIRE:
            ret = new TopoShapeWirePy(new TopoShape(shape));
            break;
        case TopAbs_EDGE:
            ret = new TopoShapeEdgePy(new TopoShape(shape));
            break;
        case TopAbs_VERTEX:
            ret = new TopoShapeVertexPy(new TopoShape(shape));
            break;
        case TopAbs_SHAPE:
            ret = new TopoShapePy(new TopoShape(shape));
            break;
        default:
            //shouldn't happen
            ret = new TopoShapePy(new TopoShape(shape));
            break;
        }
    } else {
        ret = new TopoShapePy(new TopoShape(shape));
    }
    assert(ret);

    return Py::asObject(ret);
}

PartExport Py::Object shape2pyshape(const TopoDS_Shape &shape) {
    return shape2pyshape(TopoShape(shape));
}

} //namespace Part


namespace Py {
    template<>
    bool TopoShape::accepts (PyObject *pyob) const
    {
        return (pyob && PyObject_TypeCheck(pyob, &(Part::TopoShapePy::Type)));
    }

    // explicit template instantiation
    template class PartExport ExtensionObject<Part::TopoShapePy>;
}

