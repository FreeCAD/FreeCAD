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

#ifndef GUI_SELECTIONFILTERPY_H
#define GUI_SELECTIONFILTERPY_H

#include <CXX/Extensions.hxx>

#include "SelectionFilter.h"


namespace Gui {
/**
 * Python binding for SelectionFilter class.
 * \code
 * filter=Gui.Selection.Filter("SELECT Part::Feature SUBELEMENT Edge")
 * Gui.Selection.addSelectionGate(filter)
 * \endcode
 * @see SelectionFilter
 * @author Werner Mayer
 */
class SelectionFilterPy : public Py::PythonClass<SelectionFilterPy>
{
public:
    SelectionFilter filter;

public:
    static void init_type();    // announce properties and methods
    static SelectionFilterPy* cast(PyObject* py) {
        using SelectionFilterClass = Py::PythonClassObject<SelectionFilterPy>;
        return SelectionFilterClass(py).getCxxObject();
    }

    SelectionFilterPy(Py::PythonClassInstance* self, Py::Tuple& args, Py::Dict& kdws);

    ~SelectionFilterPy() override;

    Py::Object repr() override;
    Py::Object match();
    Py::Object result();
    Py::Object test(const Py::Tuple&);
    Py::Object setFilter(const Py::Tuple&);
    Py::Object getFilter();
};

} // namespace Gui

#endif // GUI_SELECTIONFILTERPY_H
