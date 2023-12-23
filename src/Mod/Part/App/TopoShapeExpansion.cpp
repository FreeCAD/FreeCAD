// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2022 Zheng, Lei <realthunder.dev@gmail.com>              *
 *   Copyright (c) 2023 FreeCAD Project Association                         *
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

#include <BRep_Tool.hxx>
#include "PreCompiled.h"

#include "TopoShape.h"
#include "TopoShapeCache.h"

FC_LOG_LEVEL_INIT("TopoShape", true, true)  // NOLINT

namespace Part
{

void TopoShape::initCache(int reset) const
{
    if (reset > 0 || !_cache || _cache->isTouched(_Shape)) {
        if (_parentCache) {
            _parentCache.reset();
            _subLocation.Identity();
        }
        _cache = std::make_shared<TopoShapeCache>(_Shape);
    }
}

void TopoShape::setShape(const TopoDS_Shape& shape, bool resetElementMap)
{
    if (resetElementMap) {
        this->resetElementMap();
    }
    else if (_cache && _cache->isTouched(shape)) {
        this->flushElementMap();
    }
    //_Shape._Shape = shape; // TODO: Replace the next line with this once ShapeProtector is
    // available.
    _Shape = shape;
    if (_cache) {
        initCache();
    }
}


TopoDS_Shape& TopoShape::move(TopoDS_Shape& tds, const TopLoc_Location& location)
{
#if OCC_VERSION_HEX < 0x070600
    tds.Move(location);
#else
    tds.Move(location, false);
#endif
    return tds;
}

TopoDS_Shape TopoShape::moved(const TopoDS_Shape& tds, const TopLoc_Location& location)
{
#if OCC_VERSION_HEX < 0x070600
    return tds.Moved(location);
#else
    return tds.Moved(location, false);
#endif
}

TopoDS_Shape& TopoShape::move(TopoDS_Shape& tds, const gp_Trsf& transfer)
{
#if OCC_VERSION_HEX < 0x070600
    static constexpr double scalePrecision {1e-14};
    if (std::abs(transfer.ScaleFactor()) > scalePrecision)
#else
    if (std::abs(transfer.ScaleFactor()) > TopLoc_Location::ScalePrec())
#endif
    {
        auto transferCopy(transfer);
        transferCopy.SetScaleFactor(1.0);
        tds.Move(transferCopy);
    }
    else {
        tds.Move(transfer);
    }
    return tds;
}

TopoDS_Shape TopoShape::moved(const TopoDS_Shape& tds, const gp_Trsf& transfer)
{
    TopoDS_Shape sCopy(tds);
    return move(sCopy, transfer);
}

TopoDS_Shape& TopoShape::locate(TopoDS_Shape& tds, const TopLoc_Location& loc)
{
    tds.Location(TopLoc_Location());
    return move(tds, loc);
}

TopoDS_Shape TopoShape::located(const TopoDS_Shape& tds, const TopLoc_Location& loc)
{
    auto sCopy(tds);
    sCopy.Location(TopLoc_Location());
    return moved(sCopy, loc);
}

TopoDS_Shape& TopoShape::locate(TopoDS_Shape& tds, const gp_Trsf& transfer)
{
    tds.Location(TopLoc_Location());
    return move(tds, transfer);
}

TopoDS_Shape TopoShape::located(const TopoDS_Shape& tds, const gp_Trsf& transfer)
{
    auto sCopy(tds);
    sCopy.Location(TopLoc_Location());
    return moved(sCopy, transfer);
}


int TopoShape::findShape(const TopoDS_Shape& subshape) const
{
    initCache();
    return _cache->findShape(_Shape, subshape);
}


TopoDS_Shape TopoShape::findShape(const char* name) const
{
    if (!name) {
        return {};
    }

    Data::MappedElement res = getElementName(name);
    if (!res.index) {
        return {};
    }

    auto idx = shapeTypeAndIndex(name);
    if (idx.second == 0) {
        return {};
    }
    initCache();
    return _cache->findShape(_Shape, idx.first, idx.second);
}

TopoDS_Shape TopoShape::findShape(TopAbs_ShapeEnum type, int idx) const
{
    initCache();
    return _cache->findShape(_Shape, type, idx);
}

int TopoShape::findAncestor(const TopoDS_Shape& subshape, TopAbs_ShapeEnum type) const
{
    initCache();
    return _cache->findShape(_Shape, _cache->findAncestor(_Shape, subshape, type));
}

TopoDS_Shape TopoShape::findAncestorShape(const TopoDS_Shape& subshape, TopAbs_ShapeEnum type) const
{
    initCache();
    return _cache->findAncestor(_Shape, subshape, type);
}

std::vector<int> TopoShape::findAncestors(const TopoDS_Shape& subshape, TopAbs_ShapeEnum type) const
{
    const auto& shapes = findAncestorsShapes(subshape, type);
    std::vector<int> ret;
    ret.reserve(shapes.size());
    for (const auto& shape : shapes) {
        ret.push_back(findShape(shape));
    }
    return ret;
}

std::vector<TopoDS_Shape> TopoShape::findAncestorsShapes(const TopoDS_Shape& subshape,
                                                         TopAbs_ShapeEnum type) const
{
    initCache();
    std::vector<TopoDS_Shape> shapes;
    _cache->findAncestor(_Shape, subshape, type, &shapes);
    return shapes;
}

}  // namespace Part
