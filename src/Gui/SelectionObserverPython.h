/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_SELECTIONOBSERVERPYTHON_H
#define GUI_SELECTIONOBSERVERPYTHON_H

#include <CXX/Objects.hxx>
#include "Selection.h"


namespace Gui
{

/**
 * The SelectionObserverPython class implements a mechanism to register
 * a Python class instance implementing the required interface in order
 * to be notified on selection changes.
 *
 * @author Werner Mayer
 */
class GuiExport SelectionObserverPython : public SelectionObserver
{

public:
    /// Constructor
    explicit SelectionObserverPython(const Py::Object& obj, ResolveMode resolve = ResolveMode::OldStyleElement);
    ~SelectionObserverPython() override;

    static void addObserver(const Py::Object& obj, ResolveMode resolve = ResolveMode::OldStyleElement);
    static void removeObserver(const Py::Object& obj);

private:
    void onSelectionChanged(const SelectionChanges& msg) override;
    void addSelection(const SelectionChanges&);
    void removeSelection(const SelectionChanges&);
    void setSelection(const SelectionChanges&);
    void clearSelection(const SelectionChanges&);
    void setPreselection(const SelectionChanges&);
    void removePreselection(const SelectionChanges&);
    void pickedListChanged();

private:
    Py::Object inst;

#define FC_PY_SEL_OBSERVER \
    FC_PY_ELEMENT(onSelectionChanged) \
    FC_PY_ELEMENT(addSelection) \
    FC_PY_ELEMENT(removeSelection) \
    FC_PY_ELEMENT(setSelection) \
    FC_PY_ELEMENT(clearSelection) \
    FC_PY_ELEMENT(setPreselection) \
    FC_PY_ELEMENT(removePreselection) \
    FC_PY_ELEMENT(pickedListChanged)

#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) Py::Object py_##_name;

    FC_PY_SEL_OBSERVER

    static std::vector<SelectionObserverPython*> _instances;
};

} //namespace Gui

#endif // GUI_SELECTIONOBSERVERPYTHON_H
