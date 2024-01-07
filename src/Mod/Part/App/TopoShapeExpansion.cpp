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
#ifndef _PreComp_
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#endif

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

#define _HANDLE_NULL_SHAPE(_msg,_throw) do {\
    if(_throw) {\
        FC_THROWM(NullShapeException,_msg);\
    }\
    FC_WARN(_msg);\
}while(0)

#define HANDLE_NULL_SHAPE _HANDLE_NULL_SHAPE("Null shape",true)
#define HANDLE_NULL_INPUT _HANDLE_NULL_SHAPE("Null input shape",true)
#define WARN_NULL_INPUT _HANDLE_NULL_SHAPE("Null input shape",false)

bool TopoShape::hasPendingElementMap() const
{
    return !elementMap(false)
        && this->_cache
        && (this->_parentCache || this->_cache->cachedElementMap);
}

bool TopoShape::canMapElement(const TopoShape &other) const {
    if(isNull() || other.isNull() || this == &other || other.Tag == -1 || Tag == -1)
        return false;
    if(!other.Tag
        && !other.elementMap(false)
        && !other.hasPendingElementMap())
        return false;
    initCache();
    other.initCache();
    _cache->relations.clear();
    return true;
}

void TopoShape::mapSubElement(const TopoShape &other, const char *op, bool forceHasher) {
#ifdef FC_NO_ELEMENT_MAP
    return;
#endif

    if(!canMapElement(other))
        return;

    if (!getElementMapSize(false) && this->_Shape.IsPartner(other._Shape)) {
        if (!this->Hasher)
            this->Hasher = other.Hasher;
        copyElementMap(other, op);
        return;
    }

    bool warned = false;
    static const std::array<TopAbs_ShapeEnum, 3> types = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE};

    auto checkHasher = [this](const TopoShape &other) {
        if(Hasher) {
            if(other.Hasher!=Hasher) {
                if(!getElementMapSize(false)) {
                    if(FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
                        FC_WARN("hasher mismatch");
                }else {
                    // FC_THROWM(Base::RuntimeError, "hasher mismatch");
                    FC_ERR("hasher mismatch");
                }
                Hasher = other.Hasher;
            }
        }else
            Hasher = other.Hasher;
    };

    for(auto type : types) {
        auto &shapeMap = _cache->getAncestry(type);
        auto &otherMap = other._cache->getAncestry(type);
        if(!shapeMap.count() || !otherMap.count())
            continue;
        if(!forceHasher && other.Hasher) {
            forceHasher = true;
            checkHasher(other);
        }
        const char *shapetype = shapeName(type).c_str();
        std::ostringstream ss;

        bool forward;
        int count;
        if(otherMap.count()<=shapeMap.count()) {
            forward = true;
            count = otherMap.count();
        }else{
            forward = false;
            count = shapeMap.count();
        }
        for(int k=1;k<=count;++k) {
            int i,idx;
            if(forward) {
                i = k;
                idx = shapeMap.find(_Shape,otherMap.find(other._Shape,k));
                if(!idx) continue;
            } else {
                idx = k;
                i = otherMap.find(other._Shape,shapeMap.find(_Shape,k));
                if(!i) continue;
            }
            Data::IndexedName element = Data::IndexedName::fromConst(shapetype, idx);
            for(auto &v : other.getElementMappedNames(
                     Data::IndexedName::fromConst(shapetype,i),true))
            {
                auto &name = v.first;
                auto &sids = v.second;
                if(sids.size()) {
                    if (!Hasher)
                        Hasher = sids[0].getHasher();
                    else if (!sids[0].isFromSameHasher(Hasher)) {
                        if (!warned) {
                            warned = true;
                            FC_WARN("hasher mismatch");
                        }
                        sids.clear();
                    }
                }
                ss.str("");
                elementMap()->encodeElementName(shapetype[0],name,ss,&sids,Tag,op,other.Tag);
                elementMap()->setElementName(element,name,Tag, &sids);
            }
        }
    }
}

void TopoShape::mapSubElement(const std::vector<TopoShape> &shapes, const char *op) {
#ifdef FC_NO_ELEMENT_MAP
    return;
#endif

    if (shapes.empty())
        return;

    if (shapeType(true) == TopAbs_COMPOUND) {
        int count = 0;
        for (auto & s : shapes) {
            if (s.isNull())
                continue;
            if (!getSubShape(TopAbs_SHAPE, ++count, true).IsPartner(s._Shape)) {
                count = 0;
                break;
            }
        }
        if (count) {
            std::vector<Data::ElementMap::MappedChildElements> children;
            children.reserve(count*3);
            TopAbs_ShapeEnum types[] = {TopAbs_VERTEX, TopAbs_EDGE, TopAbs_FACE};
            for (unsigned i=0; i<sizeof(types)/sizeof(types[0]); ++i) {
                int offset = 0;
                for (auto & s : shapes) {
                    if (s.isNull())
                        continue;
                    int count = s.countSubShapes(types[i]);
                    if (!count)
                        continue;
                    children.emplace_back();
                    auto & child = children.back();
                    child.indexedName = Data::IndexedName::fromConst(shapeName(types[i]).c_str(), 1);
                    child.offset = offset;
                    offset += count;
                    child.count = count;
                    child.elementMap = s.elementMap();
                    child.tag = s.Tag;
                    if (op)
                        child.postfix = op;
                }
            }
            elementMap()->addChildElements(Tag, children);  // Replaces the original line below
            //setMappedChildElements(children);
            return;
        }
    }

    for(auto &shape : shapes)
        mapSubElement(shape,op);
}

TopoShape &TopoShape::makeElementCompound(const std::vector<TopoShape> &shapes, const char *op, bool force)
{
    if(!force && shapes.size()==1) {
        *this = shapes[0];
        return *this;
    }

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    if(shapes.empty()) {
        setShape(comp);
        return *this;
    }

    int count = 0;
    for(auto &s : shapes) {
        if(s.isNull()) {
            WARN_NULL_INPUT;
            continue;
        }
        builder.Add(comp,s.getShape());
        ++count;
    }
    if(!count)
        HANDLE_NULL_SHAPE;
    setShape(comp);
    initCache();

    mapSubElement(shapes,op);
    return *this;
}

}  // namespace Part
