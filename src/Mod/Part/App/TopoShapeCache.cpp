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

#include "PreCompiled.h"
#include "TopoShapeCache.h"

using namespace Part;

ShapeRelationKey::ShapeRelationKey(Data::MappedName name, HistoryTraceType historyTraceType)
    : name(std::move(name))
    , historyTraceType(historyTraceType)
{}

bool ShapeRelationKey::operator<(const ShapeRelationKey& other) const
{
    if (historyTraceType != other.historyTraceType) {
        return historyTraceType < other.historyTraceType;
    }
    return name < other.name;
}

TopoShape TopoShapeCache::Ancestry::_getTopoShape(const TopoShape& parent, int index)
{
    auto& ts = topoShapes[index - 1];
    if (ts.isNull()) {
        ts.setShape(shapes.FindKey(index), true);
        ts.initCache();
        ts._cache->subLocation = ts._Shape.Location();
    }

    if (ts._Shape.IsEqual(parent._cache->shape)) {
        return parent;
    }

    TopoShape res(ts);
    res.Tag = parent.Tag;
    res.Hasher = parent.Hasher;

    if (!parent.getShape().Location().IsIdentity()) {
        res.setShape(TopoShape::moved(res._Shape, parent.getShape().Location()), false);
    }

    if (ts._cache->cachedElementMap) {
        res.resetElementMap(ts._cache->cachedElementMap);
    }
    else if (parent._parentCache) {
        // If no cachedElementMap exists, we use _parentCache for
        // delayed generation of sub element map so that we don't need
        // to always generate a full map whenever we return a sub
        // shape.  To simplify the mapping and avoid circular
        // dependency, we do not chain parent and grandparent.
        // Instead, we always use the cache from the top parent. And to
        // make it work, we must accumulate the TopLoc_Location along
        // the lineage, which is required for OCCT shape mapping to
        // work.
        //
        // Cache::subLocation is shared and only contains the location
        // in the direct parent shape, while TopoShape::_subLocation is
        // used to accumulate locations in higher ancestors. We
        // separate these two to avoid invalidating cache.

        res._subLocation = parent._subLocation * parent._cache->subLocation;
        res._parentCache = parent._parentCache;
    }
    else {
        res._parentCache = owner->shared_from_this();
    }
    return res;
}


void TopoShapeCache::Ancestry::clear()
{
    topoShapes.clear();
}

TopoShape TopoShapeCache::Ancestry::getTopoShape(const TopoShape& parent, int index)
{
    TopoShape res;
    if (index <= 0 || index > shapes.Extent()) {
        return res;
    }
    topoShapes.resize(shapes.Extent());
    return _getTopoShape(parent, index);
}

std::vector<TopoShape> TopoShapeCache::Ancestry::getTopoShapes(const TopoShape& parent)
{
    int count = shapes.Extent();
    std::vector<TopoShape> res;
    res.reserve(count);
    topoShapes.resize(count);
    for (int i = 1; i <= count; ++i) {
        res.push_back(_getTopoShape(parent, i));
    }
    return res;
}

TopoDS_Shape TopoShapeCache::Ancestry::stripLocation(const TopoDS_Shape& parent,
                                                     const TopoDS_Shape& child)
{
    if (parent.Location() != owner->location) {
        owner->location = parent.Location();
        owner->locationInverse = parent.Location().Inverted();
    }
    return TopoShape::located(child, owner->locationInverse * child.Location());
}

int TopoShapeCache::Ancestry::find(const TopoDS_Shape& parent, const TopoDS_Shape& subShape)
{
    if (parent.Location().IsIdentity()) {
        return shapes.FindIndex(subShape);
    }
    return shapes.FindIndex(stripLocation(parent, subShape));
}

TopoDS_Shape TopoShapeCache::Ancestry::find(const TopoDS_Shape& parent, int index)
{
    if (index <= 0 || index > shapes.Extent()) {
        return {};
    }
    if (parent.Location().IsIdentity()) {
        return shapes.FindKey(index);
    }
    return TopoShape::moved(shapes.FindKey(index), parent.Location());
}

int TopoShapeCache::Ancestry::count() const
{
    return shapes.Extent();
}

bool TopoShapeCache::Ancestry::empty() const
{
    return shapes.IsEmpty();
}

TopoShapeCache::TopoShapeCache(const TopoDS_Shape& tds)
    : shape(tds.Located(TopLoc_Location()))
{}

void TopoShapeCache::insertRelation(const ShapeRelationKey& key,
                                    const QVector<Data::MappedElement>& value)
{
    auto [insertedItr, newKeyInserted] = relations.insert({key, value});
    if (newKeyInserted) {
        insertedItr->first.name.compact();
    }
    else {
        insertedItr->second = value;
    }
}

bool TopoShapeCache::isTouched(const TopoDS_Shape& tds) const
{
    return !this->shape.IsPartner(tds) || this->shape.Orientation() != tds.Orientation();
}

TopoShapeCache::Ancestry& TopoShapeCache::getAncestry(TopAbs_ShapeEnum type)
{
    auto& ancestry = shapeAncestryCache.at(type);
    if (!ancestry.owner) {
        ancestry.owner = this;
        if (!shape.IsNull()) {
            if (type == TopAbs_SHAPE) {
                for (TopoDS_Iterator it(shape); it.More(); it.Next()) {
                    ancestry.shapes.Add(it.Value());
                }
            }
            else {
                TopExp::MapShapes(shape, type, ancestry.shapes);
            }
        }
    }
    return ancestry;
}

int TopoShapeCache::countShape(TopAbs_ShapeEnum type)
{
    if (shape.IsNull()) {
        return 0;
    }
    return getAncestry(type).count();
}

int TopoShapeCache::findShape(const TopoDS_Shape& parent, const TopoDS_Shape& subShape)
{
    if (shape.IsNull() || subShape.IsNull()) {
        return 0;
    }
    return getAncestry(subShape.ShapeType()).find(parent, subShape);
}

TopoDS_Shape TopoShapeCache::findShape(const TopoDS_Shape& parent, TopAbs_ShapeEnum type, int index)
{
    if (!shape.IsNull()) {
        return getAncestry(type).find(parent, index);
    }
    return {};
}

TopoDS_Shape TopoShapeCache::findAncestor(const TopoDS_Shape& parent,
                                          const TopoDS_Shape& subShape,
                                          TopAbs_ShapeEnum type,
                                          std::vector<TopoDS_Shape>* ancestors)
{
    TopoDS_Shape nullShape;
    if (shape.IsNull() || subShape.IsNull() || type == TopAbs_SHAPE) {
        return nullShape;
    }

    auto& info = getAncestry(type);

    auto& ancestorInfo = info.ancestors.at(subShape.ShapeType());
    if (!ancestorInfo.initialized) {
        ancestorInfo.initialized = true;
        // ancestorInfo.shapes is the output variable here, storing (and caching) the actual map
        TopExp::MapShapesAndAncestors(shape, subShape.ShapeType(), type, ancestorInfo.shapes);
    }
    int index = parent.Location().IsIdentity()
        ? ancestorInfo.shapes.FindIndex(subShape)
        : ancestorInfo.shapes.FindIndex(info.stripLocation(parent, subShape));
    if (index == 0) {
        return nullShape;
    }
    const auto& shapes = ancestorInfo.shapes.FindFromIndex(index);
    if (shapes.Extent() == 0) {
        return nullShape;
    }

    if (ancestors) {
        ancestors->reserve(ancestors->size() + shapes.Extent());
        for (TopTools_ListIteratorOfListOfShape it(shapes); it.More(); it.Next()) {
            ancestors->push_back(TopoShape::moved(it.Value(), parent.Location()));
        }
    }
    return TopoShape::moved(shapes.First(), parent.Location());
}
