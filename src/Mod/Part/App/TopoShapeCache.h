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

#ifndef FREECAD_TOPOSHAPECACHE_H
#define FREECAD_TOPOSHAPECACHE_H


#include "PreCompiled.h"

#ifndef _PreComp_
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopoDS_Solid.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopTools_ListIteratorOfListOfShape.hxx>
#endif

#include <App/ElementMap.h>

#include "TopoShape.h"

namespace Part
{

struct ShapeRelationKey {
    Data::MappedName name;
    bool sameType;

    ShapeRelationKey(const Data::MappedName & name, bool sameType)
        :name(name), sameType(sameType)
    {}

    bool operator<(const ShapeRelationKey &other) const {
        if(sameType != other.sameType)
            return sameType;
        return name < other.name;
    }
};

class TopoShapeCache: public std::enable_shared_from_this<TopoShapeCache>
{
public:
    Data::ElementMapPtr cachedElementMap;
    TopLoc_Location subLocation;
    TopoDS_Shape shape;
    TopLoc_Location loc;
    TopLoc_Location locInv;

    std::size_t memsize = 0;

    struct AncestorInfo
    {
        bool inited = false;
        TopTools_IndexedDataMapOfShapeListOfShape shapes;
    };
    class Info
    {
    private:
        TopoShapeCache* owner = 0;
        TopTools_IndexedMapOfShape shapes;
        std::vector<TopoShape> topoShapes;
        std::array<AncestorInfo, TopAbs_SHAPE + 1> ancestors;

        TopoShape _getTopoShape(const TopoShape& parent, int index)
        {
            auto& s = topoShapes[index - 1];
            if (s.isNull()) {
                s.setShape(shapes.FindKey(index), true);
                s.initCache();
                s._cache->subLocation = s._Shape.Location();
            }

            if (s._Shape.IsEqual(parent._cache->shape))
                return parent;

            TopoShape res(s);
            res.Tag = parent.Tag;
            res.Hasher = parent.Hasher;

            if (!parent.getShape().Location().IsIdentity())
                res.setShape(TopoShape::moved(res._Shape, parent.getShape().Location()), false);

            if (s._cache->cachedElementMap)
                res.resetElementMap(s._cache->cachedElementMap);
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
            else
                res._parentCache = owner->shared_from_this();
            return res;
        }

    public:
        void clear()
        {
            topoShapes.clear();
        }

        TopoShape getTopoShape(const TopoShape& parent, int index)
        {
            TopoShape res;
            if (index <= 0 || index > shapes.Extent())
                return res;
            topoShapes.resize(shapes.Extent());
            return _getTopoShape(parent, index);
        }

        std::vector<TopoShape> getTopoShapes(const TopoShape& parent)
        {
            int count = shapes.Extent();
            std::vector<TopoShape> res;
            res.reserve(count);
            topoShapes.resize(count);
            for (int i = 1; i <= count; ++i)
                res.push_back(_getTopoShape(parent, i));
            return res;
        }

        TopoDS_Shape stripLocation(const TopoDS_Shape& parent, const TopoDS_Shape& child)
        {
            if (parent.Location() != owner->loc) {
                owner->loc = parent.Location();
                owner->locInv = parent.Location().Inverted();
            }
            return TopoShape::located(child, owner->locInv * child.Location());
        }

        int find(const TopoDS_Shape& parent, const TopoDS_Shape& subshape)
        {
            if (parent.Location().IsIdentity())
                return shapes.FindIndex(subshape);
            return shapes.FindIndex(stripLocation(parent, subshape));
        }

        TopoDS_Shape find(const TopoDS_Shape& parent, int index)
        {
            if (index <= 0 || index > shapes.Extent())
                return TopoDS_Shape();
            if (parent.Location().IsIdentity())
                return shapes.FindKey(index);
            else
                return TopoShape::moved(shapes.FindKey(index), parent.Location());
        }

        int count() const
        {
            return shapes.Extent();
        }

        friend TopoShapeCache;
    };

    std::array<Info, TopAbs_SHAPE + 1> infos;
    std::map<ShapeRelationKey, QVector<Data::MappedElement>> relations;

    TopoShapeCache(const TopoDS_Shape& s)
        : shape(s.Located(TopLoc_Location()))
    {}

    void insertRelation(const ShapeRelationKey& key, const QVector<Data::MappedElement>& value)
    {
        auto res = relations.insert(std::make_pair(key, value));
        if (res.second)
            res.first->first.name.compact();
        else
            res.first->second = value;
    }

    bool isTouched(const TopoDS_Shape& s)
    {
        return !this->shape.IsPartner(s) || this->shape.Orientation() != s.Orientation();
    }

    Info& getInfo(TopAbs_ShapeEnum type)
    {
        auto& info = infos[type];
        if (!info.owner) {
            info.owner = this;
            if (!shape.IsNull()) {
                if (type == TopAbs_SHAPE) {
                    for (TopoDS_Iterator it(shape); it.More(); it.Next())
                        info.shapes.Add(it.Value());
                }
                else
                    TopExp::MapShapes(shape, type, info.shapes);
            }
        }
        return info;
    }

    int countShape(TopAbs_ShapeEnum type)
    {
        if (shape.IsNull())
            return 0;
        return getInfo(type).count();
    }

    int findShape(const TopoDS_Shape& parent, const TopoDS_Shape& subshape)
    {
        if (shape.IsNull() || subshape.IsNull())
            return 0;
        return getInfo(subshape.ShapeType()).find(parent, subshape);
    }

    TopoDS_Shape findShape(const TopoDS_Shape& parent, TopAbs_ShapeEnum type, int index)
    {
        if (!shape.IsNull())
            return getInfo(type).find(parent, index);
        return TopoDS_Shape();
    }

    TopoDS_Shape findAncestor(const TopoDS_Shape& parent,
                              const TopoDS_Shape& subshape,
                              TopAbs_ShapeEnum type,
                              std::vector<TopoDS_Shape>* ancestors = 0)
    {
        TopoDS_Shape ret;
        if (shape.IsNull() || subshape.IsNull() || type == TopAbs_SHAPE)
            return ret;

        auto& info = getInfo(type);

        auto& ainfo = info.ancestors[subshape.ShapeType()];
        if (!ainfo.inited) {
            ainfo.inited = true;
            TopExp::MapShapesAndAncestors(shape, subshape.ShapeType(), type, ainfo.shapes);
        }
        int index;
        if (parent.Location().IsIdentity())
            index = ainfo.shapes.FindIndex(subshape);
        else
            index = ainfo.shapes.FindIndex(info.stripLocation(parent, subshape));
        if (!index)
            return ret;
        const auto& shapes = ainfo.shapes.FindFromIndex(index);
        if (!shapes.Extent())
            return ret;

        if (ancestors) {
            ancestors->reserve(ancestors->size() + shapes.Extent());
            for (TopTools_ListIteratorOfListOfShape it(shapes); it.More(); it.Next())
                ancestors->push_back(TopoShape::moved(it.Value(), parent.Location()));
        }
        return TopoShape::moved(shapes.First(), parent.Location());
    }

    std::size_t getMemSize();
};

}

#endif  // FREECAD_TOPOSHAPECACHE_H
