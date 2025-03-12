// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>               *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#include <BRep_Tool.hxx>
#include <TopoDS_Edge.hxx>
#endif

#include "TopoShapeMapper.h"
#include "Geometry.h"

namespace Part
{

void ShapeMapper::expand(const TopoDS_Shape& d, std::vector<TopoDS_Shape>& shapes)
{
    if (d.IsNull()) {
        return;
    }
    for (TopExp_Explorer xp(d, TopAbs_FACE); xp.More(); xp.Next()) {
        shapes.push_back(xp.Current());
    }
    for (TopExp_Explorer xp(d, TopAbs_EDGE, TopAbs_FACE); xp.More(); xp.Next()) {
        shapes.push_back(xp.Current());
    }
    for (TopExp_Explorer xp(d, TopAbs_VERTEX, TopAbs_EDGE); xp.More(); xp.Next()) {
        shapes.push_back(xp.Current());
    }
}

void ShapeMapper::populate(MappingStatus status,
                           const TopTools_ListOfShape& src,
                           const TopTools_ListOfShape& dst)
{
    for (TopTools_ListIteratorOfListOfShape it(src); it.More(); it.Next()) {
        populate(status, it.Value(), dst);
    }
}

void ShapeMapper::populate(MappingStatus status,
                           const TopoShape& src,
                           const TopTools_ListOfShape& dst)
{
    if (src.isNull()) {
        return;
    }
    std::vector<TopoDS_Shape> dstShapes;
    for (TopTools_ListIteratorOfListOfShape it(dst); it.More(); it.Next()) {
        expand(it.Value(), dstShapes);
    }
    insert(status, src.getShape(), dstShapes);
    if (shapeSet.insert(src.getShape()).second) {
        shapes.push_back(src);
    }
}

void ShapeMapper::insert(MappingStatus status, const TopoDS_Shape& s, const TopoDS_Shape& d)
{
    if (s.IsNull() || d.IsNull()) {
        return;
    }
    // Prevent an element shape from being both generated and modified
    if (status == MappingStatus::Generated) {
        if (_modifiedShapes.count(d)) {
            return;
        }
        _generatedShapes.insert(d);
    }
    else {
        if (_generatedShapes.count(d)) {
            return;
        }
        _modifiedShapes.insert(d);
    }
    auto& entry = (status == MappingStatus::Generated) ? _generated[s] : _modified[s];
    if (entry.shapeSet.insert(d).second) {
        entry.shapes.push_back(d);
    }
};

void ShapeMapper::insert(MappingStatus status,
                         const TopoDS_Shape& s,
                         const std::vector<TopoDS_Shape>& d)
{
    if (s.IsNull() || d.empty()) {
        return;
    }
    auto& entry = (status == MappingStatus::Generated) ? _generated[s] : _modified[s];
    for (auto& shape : d) {
        // Prevent an element shape from being both generated and modified
        if (status == MappingStatus::Generated) {
            if (_modifiedShapes.count(shape)) {
                continue;
            }
            _generatedShapes.insert(shape);
        }
        else {
            if (_generatedShapes.count(shape)) {
                continue;
            }
            _modifiedShapes.insert(shape);
        }
        if (entry.shapeSet.insert(shape).second) {
            entry.shapes.push_back(shape);
        }
    }
};

void GenericShapeMapper::init(const TopoShape& src, const TopoDS_Shape& dst)
{
    for (TopExp_Explorer exp(dst, TopAbs_FACE); exp.More(); exp.Next()) {
        const TopoDS_Shape& dstFace = exp.Current();
        if (src.findShape(dstFace)) {
            continue;
        }
#if OCC_VERSION_HEX < 0x070800
        struct TopoDS_ShapeHasher
        {
            std::size_t operator()(const TopoDS_Shape& key) const
            {
                return key.HashCode(IntegerLast());
            }
        };
        std::unordered_map<TopoDS_Shape, int, TopoDS_ShapeHasher> map;
#else
        std::unordered_map<TopoDS_Shape, int> map;
#endif
        bool found = false;

        // Try to find a face in the src that shares at least two edges (or one
        // closed edge) with dstFace.
        // TODO: consider degenerative cases of two or more edges on the same line.
        for (TopExp_Explorer it(dstFace, TopAbs_EDGE); it.More(); it.Next()) {
            int idx = src.findShape(it.Current());
            if (!idx) {
                continue;
            }
            TopoDS_Edge e = TopoDS::Edge(it.Current());
            if (BRep_Tool::IsClosed(e)) {
                // closed edge, one face is enough
                TopoDS_Shape face =
                    src.findAncestorShape(src.getSubShape(TopAbs_EDGE, idx), TopAbs_FACE);
                if (!face.IsNull()) {
                    this->insert(MappingStatus::Generated, face, dstFace);
                    found = true;
                    break;
                }
                continue;
            }
            for (auto& face :
                 src.findAncestorsShapes(src.getSubShape(TopAbs_EDGE, idx), TopAbs_FACE)) {
                int& cnt = map[face];
                if (++cnt == 2) {
                    this->insert(MappingStatus::Generated, face, dstFace);
                    found = true;
                    break;
                }
                if (found) {
                    break;
                }
            }
        }

        if (found) {
            continue;
        }

        // if no face matches, try search by geometry surface
        std::unique_ptr<Geometry> g(Geometry::fromShape(dstFace));
        if (!g) {
            continue;
        }

        for (auto& v : map) {
            std::unique_ptr<Geometry> g2(Geometry::fromShape(v.first));
            if (g2 && g2->isSame(*g, 1e-7, 1e-12)) {
                this->insert(MappingStatus::Generated, v.first, dstFace);
                break;
            }
        }
    }
}

}  // namespace Part
