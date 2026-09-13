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

#ifndef PART_SHAPELIST_H
#define PART_SHAPELIST_H

#include <functional>
#include <memory>
#include <vector>

#include <TopAbs_ShapeEnum.hxx>

#include "TopoShape.h"

namespace Part
{

/** The sub elements of one shape, materialised one at a time
 *
 * What Shape.Faces and its siblings answer with. A ShapeList is normally a
 * VIEW: it holds the parent shape, the element type and the type to avoid,
 * and asks the parent's cache for an element only when one is wanted. Its
 * length, and any single element of it, therefore cost what one cache
 * lookup costs -- not a shape object per element, which is what building
 * the whole list up front used to charge for a call that wanted one face.
 *
 * Writing to a list turns it into a value: the first mutating call
 * materialises every element into storage of its own, and from then on the
 * list shares that storage copy-on-write with every copy of itself. Reading
 * never materialises anything.
 *
 * @section shapelist_snapshot What a view is a view of
 *
 * The parent is held BY VALUE, so a list keeps answering with the elements
 * the shape had when it was asked -- reassigning the shape the list came
 * from does not change the list. Sharing the parent's cache is what makes
 * that free.
 */
class PartExport ShapeList
{
public:
    ShapeList() = default;

    /// A view of \a parent's sub shapes of \a type, skipping those that
    /// belong to a sub shape of type \a avoid
    ShapeList(const TopoShape& parent, TopAbs_ShapeEnum type, TopAbs_ShapeEnum avoid = TopAbs_SHAPE);

    /// A value: the given shapes, in the given order
    explicit ShapeList(std::vector<TopoShape> shapes);

    /// Whether this is still a view of a parent shape, i.e. whether nothing
    /// has been written to it
    bool isView() const
    {
        return _isView;
    }

    TopAbs_ShapeEnum getType() const
    {
        return _type;
    }

    /// Mark the list as mixed when a written element has another type.
    /// Once mixed, keep the conservative Shape type without scanning the
    /// whole value on every subsequent append or assignment.
    void noteType(const TopoShape& shape)
    {
        if (_type != TopAbs_SHAPE && (shape.isNull() || shape.getShape().ShapeType() != _type)) {
            _type = TopAbs_SHAPE;
        }
    }

    TopAbs_ShapeEnum getAvoid() const
    {
        return _avoid;
    }

    /// The shape a view came from; a null shape once the list is a value
    const TopoShape& getParent() const
    {
        return _parent;
    }

    int size() const;

    bool empty() const
    {
        return size() == 0;
    }

    /// One element, by a zero based index. An index outside the list
    /// answers with a null shape rather than throwing, so a caller that
    /// wants an exception raises its own.
    TopoShape get(int index) const;

    /// One element after the caller has validated the zero based index.
    TopoShape getUnchecked(int index) const;

    /// Every element. This is the call the whole class exists to avoid, so
    /// it is here for the caller that genuinely wants them all.
    std::vector<TopoShape> values() const;

    /// The index of the first element that is the same shape as \a shape,
    /// or -1. A view answers from the parent's cache, which knows where a
    /// sub shape sits without walking anything.
    int find(const TopoShape& shape) const;

    /// Run a write over the elements, materialising a view first
    void edit(const std::function<void(std::vector<TopoShape>&)>& op);

    /// Stop being a view: take a copy of every element into storage of this
    /// list's own. A no-op on a list that is already a value.
    void materialise();

private:
    /// Work out which of the parent's indices this view answers with, and
    /// set _hasFilter if that is not simply all of them in order
    const std::vector<int>& indices() const;

    TopoShape _parent;
    TopAbs_ShapeEnum _type {TopAbs_SHAPE};
    TopAbs_ShapeEnum _avoid {TopAbs_SHAPE};
    bool _isView {false};
    mutable bool _filtered {false};
    mutable bool _hasFilter {false};
    mutable std::vector<int> _indices;
    mutable int _cachedSize {-1};
    std::shared_ptr<std::vector<TopoShape>> _shapes;
};

}  // namespace Part

#endif  // PART_SHAPELIST_H
