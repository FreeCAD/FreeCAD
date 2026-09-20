/***************************************************************************
 *   Copyright (c) 2026 Zheng, Lei <realthunder.dev@gmail.com>             *
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

#include "ShapeList.h"

using namespace Part;

ShapeList::ShapeList(const TopoShape& parent, TopAbs_ShapeEnum type, TopAbs_ShapeEnum avoid)
    : _parent(parent)
    , _type(type)
    , _avoid(avoid)
    , _isView(true)
{
    // A null parent has no elements, and nothing below needs to special
    // case it: countSubShapes() answers 0 and get() answers a null shape.
}

ShapeList::ShapeList(std::vector<TopoShape> shapes)
{
    _shapes = std::make_shared<std::vector<TopoShape>>(std::move(shapes));
}

const std::vector<int>& ShapeList::indices() const
{
    if (_filtered) {
        return _indices;
    }
    std::vector<int> indices;
    bool hasFilter = false;
    // Only an avoided type the parent actually has can drop anything, so
    // the common case walks nothing and allocates nothing.
    if (_avoid != TopAbs_SHAPE && !_parent.isNull() && _parent.hasSubShape(_avoid)) {
        int count = static_cast<int>(_parent.countSubShapes(_type));
        indices.reserve(static_cast<std::size_t>(count));
        for (int i = 1; i <= count; ++i) {
            TopoShape shape = _parent.getSubTopoShape(_type, i, true);
            if (shape.isNull() || _parent.findAncestorShape(shape.getShape(), _avoid).IsNull()) {
                indices.push_back(i);
            }
        }
        hasFilter = true;
    }
    // Publish the cache only after traversal succeeds. If an OCCT lookup
    // throws, the next call must retry rather than observe partial indices.
    _indices = std::move(indices);
    _hasFilter = hasFilter;
    _filtered = true;
    return _indices;
}

int ShapeList::size() const
{
    if (!_isView) {
        return _shapes ? static_cast<int>(_shapes->size()) : 0;
    }
    if (_cachedSize >= 0) {
        return _cachedSize;
    }
    indices();
    if (_hasFilter) {
        _cachedSize = static_cast<int>(_indices.size());
    }
    else {
        _cachedSize = static_cast<int>(_parent.countSubShapes(_type));
    }
    return _cachedSize;
}

TopoShape ShapeList::get(int index) const
{
    if (index < 0 || index >= size()) {
        return TopoShape();
    }
    return getUnchecked(index);
}

TopoShape ShapeList::getUnchecked(int index) const
{
    if (!_isView) {
        return (*_shapes)[static_cast<std::size_t>(index)];
    }
    indices();
    int one = _hasFilter ? _indices[index] : index + 1;
    return _parent.getSubTopoShape(_type, one, true);
}

std::vector<TopoShape> ShapeList::values() const
{
    if (!_isView) {
        return _shapes ? *_shapes : std::vector<TopoShape>();
    }
    // The parent's own bulk call is cheaper than one lookup per element,
    // because it fills the cache's element vector in one pass.
    indices();
    if (!_hasFilter && !_parent.isNull()) {
        return _parent.getSubTopoShapes(_type, _avoid);
    }
    std::vector<TopoShape> res;
    int count = size();
    res.reserve(count);
    for (int i = 0; i < count; ++i) {
        res.push_back(getUnchecked(i));
    }
    return res;
}

int ShapeList::find(const TopoShape& shape) const
{
    if (!shape.isNull() && _isView && _type != TopAbs_SHAPE && shape.getShape().ShapeType() == _type
        && !_parent.isNull()) {
        // The parent's cache knows where one of its sub shapes sits
        int one = _parent.findShape(shape.getShape());
        if (one <= 0) {
            return -1;
        }
        indices();
        if (!_hasFilter) {
            return one - 1;
        }
        for (std::size_t i = 0; i < _indices.size(); ++i) {
            if (_indices[i] == one) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    int count = size();
    for (int i = 0; i < count; ++i) {
        TopoShape element = get(i);
        if ((element.isNull() && shape.isNull())
            || (!element.isNull() && !shape.isNull() && element.getShape().IsSame(shape.getShape()))) {
            return i;
        }
    }
    return -1;
}

void ShapeList::materialise()
{
    if (!_isView) {
        return;
    }
    std::vector<TopoShape> shapes = values();
    _shapes = std::make_shared<std::vector<TopoShape>>(std::move(shapes));
    _isView = false;
    _parent = TopoShape();
    _indices.clear();
    _filtered = false;
    _hasFilter = false;
    _cachedSize = -1;
}

void ShapeList::edit(const std::function<void(std::vector<TopoShape>&)>& op)
{
    materialise();
    if (!_shapes) {
        _shapes = std::make_shared<std::vector<TopoShape>>();
    }
    else if (!_shapes.unique()) {
        _shapes = std::make_shared<std::vector<TopoShape>>(*_shapes);
    }
    op(*_shapes);
}
