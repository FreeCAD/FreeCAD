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


#include <algorithm>
#include <string_view>
#include <unordered_set>

#include <App/Document.h>
#include <App/DocumentObjectPy.h>
#include <App/DocumentObject.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include "Selection.h"
#include "SelectionFilter.h"
#include "SelectionFilterParser.h"
#include "SelectionFilterPy.h"
#include "SelectionObject.h"


using namespace Gui;

SelectionFilterGate::SelectionFilterGate(const char* filter)
{
    Filter = new SelectionFilter(filter);
}

SelectionFilterGate::SelectionFilterGate(SelectionFilter* filter)
{
    Filter = filter;
}

SelectionFilterGate::SelectionFilterGate()
{
    Filter = nullptr;
}

SelectionFilterGate::~SelectionFilterGate()
{
    delete Filter;
}

bool SelectionFilterGate::allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* sSubName)
{
    return Filter->test(pObj, sSubName);
}

std::unordered_set<std::string> SelectionFilterGate::getGatedTypes(
    const std::vector<const char*>& allTypesForGeometry
) const
{
    std::unordered_set<std::string> allowedTypes;
    std::ranges::copy_if(
        allTypesForGeometry.begin(),
        allTypesForGeometry.end(),
        std::inserter(allowedTypes, allowedTypes.begin()),
        [&](const char* type) {
            return std::ranges::any_of(Filter->getAst()->Objects, [type](const Node_ObjectPtr& node) {
                if (node->SubName.empty()) {
                    return true;
                }
                if (std::string_view(type).starts_with(node->SubName)) {
                    return true;
                }
                return false;
            });
        }
    );
    return allowedTypes;
}

// ----------------------------------------------------------------------------

SelectionGatePython::SelectionGatePython(const Py::Object& obj)
    : gate(obj)
{}

SelectionGatePython::~SelectionGatePython() = default;

bool SelectionGatePython::allow(App::Document* doc, App::DocumentObject* obj, const char* sub)
{
    Base::PyGILStateLocker lock;
    try {
        if (this->gate.hasAttr(std::string("allow"))) {
            Py::Callable method(this->gate.getAttr(std::string("allow")));
            Py::Object pyDoc = Py::asObject(doc->getPyObject());
            Py::Object pyObj = Py::asObject(obj->getPyObject());
            Py::Object pySub = Py::None();
            if (sub) {
                pySub = Py::String(sub);
            }
            Py::Tuple args(3);
            args.setItem(0, pyDoc);
            args.setItem(1, pyObj);
            args.setItem(2, pySub);
            Py::Boolean ok(method.apply(args));
            return (bool)ok;
        }
    }
    catch (Py::Exception&) {
        Base::PyException e;  // extract the Python error text
        e.reportException();
    }

    return true;
}

// ----------------------------------------------------------------------------

SelectionFilterGatePython::SelectionFilterGatePython(SelectionFilterPy* obj)
    : filter(obj)
{
    Base::PyGILStateLocker lock;
    Py_INCREF(filter);
}

SelectionFilterGatePython::~SelectionFilterGatePython()
{
    Base::PyGILStateLocker lock;
    Py_DECREF(filter);
}

bool SelectionFilterGatePython::allow(App::Document*, App::DocumentObject* obj, const char* sub)
{
    return filter->filter.test(obj, sub);
}

// ----------------------------------------------------------------------------

SelectionFilter::SelectionFilter(const char* filter, App::DocumentObject* container)
    : container(container)
{
    setFilter(filter);
}

SelectionFilter::SelectionFilter(const std::string& filter, App::DocumentObject* container)
    : container(container)
{
    setFilter(filter.c_str());
}

void SelectionFilter::setFilter(const char* filter)
{
    if (Base::Tools::isNullOrEmpty(filter)) {
        Ast.reset();
        Filter.clear();
    }
    else {
        Filter = filter;
        if (!parse()) {
            throw Base::ParserError(Errors.c_str());
        }
    }
}

SelectionFilter::~SelectionFilter() = default;

bool SelectionFilter::match()
{
    if (!Ast) {
        return false;
    }
    Result.clear();

    for (const auto& it : Ast->Objects) {
        std::size_t min = 1;
        std::size_t max = 1;

        if (it->Slice) {
            min = it->Slice->Min;
            max = it->Slice->Max;
        }
        std::vector<Gui::SelectionObject> temp = container
            ? Gui::Selection().getSelectionIn(container, it->ObjectType)
            : Gui::Selection().getSelectionEx(nullptr, it->ObjectType);

        // test if subnames present
        if (it->SubName.empty()) {
            // if no subnames the count of the object get tested
            if (temp.size() < min || temp.size() > max) {
                return false;
            }
        }
        else {
            // if subnames present count all subs over the selected object of type
            std::size_t subCount = 0;
            for (const auto& it2 : temp) {
                const std::vector<std::string>& subNames = it2.getSubNames();
                if (subNames.empty()) {
                    return false;
                }
                for (const auto& subName : subNames) {
                    if (subName.find(it->SubName) != 0) {
                        return false;
                    }
                }
                subCount += subNames.size();
            }

            if (subCount < min || subCount > max) {
                return false;
            }
        }

        Result.push_back(temp);
    }
    return true;
}

bool SelectionFilter::test(App::DocumentObject* pObj, const char* sSubName)
{
    if (!Ast) {
        return false;
    }

    for (const auto& it : Ast->Objects) {
        if (pObj->isDerivedFrom(it->ObjectType)) {
            if (!sSubName) {
                return true;
            }
            if (it->SubName.empty()) {
                return true;
            }
            if (std::string(sSubName).find(it->SubName) == 0) {
                return true;
            }
        }
    }
    return false;
}

void SelectionFilter::addError(const char* e)
{
    Errors += e;
    Errors += '\n';
}

bool SelectionFilter::parse()
{
    Ast = parseSelectionFilter(Filter, Errors);
    return Errors.empty();
}
