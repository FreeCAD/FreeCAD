/***************************************************************************
 *   Copyright (c) Ajinkya Dahale       (ajinkyadahale@gmail.com) 2017     *
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

#ifndef TOPOHISTORY_H
#define TOPOHISTORY_H

#include <iostream>
#include <memory>
#include <BRepBuilderAPI_MakeShape.hxx>

#include <Base/BaseClass.h>

namespace Part {

class TopoShape;

/**
 * @brief The TopoHistory class
 *
 * Describes how a newer shape is developed from an older shape. Specifically,
 * tells which sub-shape in the old shape is deleted, or which generated/was
 * modified to which sub-shape(s) in the new shape.
 *
 * TODO: This class or, one of it's subclasses, might also be used to store
 * a "para-history" between two shapes generated with slightly different
 * methods (like with a different pad length somewhere in their history).
 */
class PartExport TopoHistory : public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    TopoHistory();
    TopoHistory(const TopoHistory&);

    void operator = (const TopoHistory&);

    TopTools_ListOfShape modified(const TopoShape&);
    TopTools_ListOfShape generated(const TopoShape&);
    bool isDeleted(const TopoShape &);

    std::shared_ptr<BRepBuilderAPI_MakeShape> shapeMaker;

};

}

#endif // TOPOHISTORY_H
