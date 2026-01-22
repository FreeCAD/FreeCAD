// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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


#pragma once

#include <limits>
#include <memory>
#include <string>
#include <CXX/Extensions.hxx>
#include "Selection.h"

namespace App
{
class DocumentObject;
}

namespace Gui
{
struct Node_Block;
class SelectionFilterPy;

/** Selection filter definition
 *  This class builds up a type/count tree out of a string
 *  to test very fast a selection or object/subelement type
 *  against it.
 *
 *  Example strings are:
 *  "SELECT Part::Feature SUBELEMENT Edge",
 *  "SELECT Robot::RobotObject",
 *  "SELECT Robot::RobotObject COUNT 1..5"
 */
class GuiExport SelectionFilter
{

public:
    /** Constructs a SelectionFilter object. */
    explicit SelectionFilter(const char* filter, App::DocumentObject* container = nullptr);
    explicit SelectionFilter(const std::string& filter, App::DocumentObject* container = nullptr);
    virtual ~SelectionFilter();

    /// Set a new filter string
    void setFilter(const char* filter);
    const std::string& getFilter() const
    {
        return Filter;
    }
    /** Test to current selection
     *  This method tests the current selection set
     *  against the filter and returns true if the
     *  described object(s) are selected.
     */
    bool match();
    /** Test objects
     *  This method tests if a given object is described in the
     *  filter. If SubName is not NULL the Subelement gets also
     *  tested.
     */
    bool test(App::DocumentObject* pObj, const char* sSubName);

    void addError(const char* e);

    friend class SelectionSingleton;

    std::vector<std::vector<SelectionObject>> Result;

    /// true if a valid filter is set
    bool isValid() const
    {
        return Ast ? true : false;
    }

protected:
    std::string Filter;
    std::string Errors;
    bool parse();
    App::DocumentObject* container;

    std::shared_ptr<Node_Block> Ast;
};

/** Filter object for the SelectionSengleton
 * This object is a link between the selection
 * filter class and the selection singleton. Created with a
 * filter string and registered in the selection it will only
 * allow the described object types to be selected.
 * @see SelectionFilter
 * @see SelectionSingleton
 */
class GuiExport SelectionFilterGate: public SelectionGate
{
public:
    /// construct with the filter string
    explicit SelectionFilterGate(const char* filter);
    explicit SelectionFilterGate(SelectionFilter* filter);
    ~SelectionFilterGate() override;
    bool allow(App::Document*, App::DocumentObject*, const char*) override;

protected:
    static SelectionFilter* nullPointer()
    {
        return nullptr;
    }

    SelectionFilterGate();

protected:
    SelectionFilter* Filter;
};

/**
 * A wrapper around a Python class that implements the SelectionGate interface
 * @author Werner Mayer
 */
class SelectionGatePython: public SelectionGate
{
public:
    /// Constructor
    explicit SelectionGatePython(const Py::Object& obj);
    ~SelectionGatePython() override;

    bool allow(App::Document*, App::DocumentObject*, const char*) override;

private:
    Py::Object gate;
};

/**
 * A Python wrapper around SelectionFilterPy to implement the SelectionGate interface
 * \code
 * class SelectionGate(object):
 *   def allow(self, doc, obj, sub):
 *     if not obj.isDerivedFrom("Part::Feature"):
 *       return False
 *     if not str(sub).startswith("Edge"):
 *       return False
 *     return True
 *
 * gate=SelectionGate()
 * Gui.Selection.addSelectionGate(gate)
 * \endcode
 * @author Werner Mayer
 */
class SelectionFilterGatePython: public SelectionGate
{
public:
    /// Constructor
    explicit SelectionFilterGatePython(SelectionFilterPy* obj);
    ~SelectionFilterGatePython() override;

    bool allow(App::Document*, App::DocumentObject*, const char*) override;

private:
    SelectionFilterPy* filter;
};

// === Abstract syntax tree (AST) ===========================================

struct Node_Slice
{
    explicit Node_Slice(int min = 1, int max = std::numeric_limits<int>::max())
        : Min(min)
        , Max(max)
    {}
    int Min, Max;
};


struct Node_Object
{
    Node_Object(std::string* type, std::string* subname, Node_Slice* slc)
        : Slice(slc)
    {
        ObjectType = Base::Type::fromName(type->c_str());
        if (subname) {
            SubName = *subname;
        }
    }
    ~Node_Object()
    {
        delete Slice;
    }
    Base::Type ObjectType;
    Node_Slice* Slice;
    std::string SubName;
};
using Node_ObjectPtr = std::shared_ptr<Node_Object>;

struct Node_Block
{
    explicit Node_Block(Node_Object* obj)
    {
        Objects.emplace_back(obj);
    }
    std::vector<Node_ObjectPtr> Objects;
};


}  // namespace Gui
