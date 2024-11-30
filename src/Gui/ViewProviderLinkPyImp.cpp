/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <sstream>
#endif

#include <Base/PlacementPy.h>

// inclusion of the generated files (generated out of ViewProviderLinkPy.xml)
#include "ViewProviderLinkPy.h"
#include "ViewProviderLinkPy.cpp"


using namespace Gui;

// returns a string which represents the object e.g. when printed in python
std::string ViewProviderLinkPy::representation() const
{
    std::stringstream str;
    str << "<ViewProviderLink at " << getViewProviderLinkPtr() << ">";

    return str.str();
}

Py::Object ViewProviderLinkPy::getDraggingPlacement() const {
    return Py::asObject(new Base::PlacementPy(new Base::Placement(
                    getViewProviderLinkPtr()->currentDraggingPlacement())));
}

void ViewProviderLinkPy::setDraggingPlacement(Py::Object arg) {
    if(!PyObject_TypeCheck(arg.ptr(),&Base::PlacementPy::Type))
        throw Py::TypeError("expects a placement");
    getViewProviderLinkPtr()->updateDraggingPlacement(
            *static_cast<Base::PlacementPy*>(arg.ptr())->getPlacementPtr());
}

Py::Boolean ViewProviderLinkPy::getUseCenterballDragger() const {
    return {getViewProviderLinkPtr()->isUsingCenterballDragger()};
}

void ViewProviderLinkPy::setUseCenterballDragger(Py::Boolean arg) {
    try {
        getViewProviderLinkPtr()->enableCenterballDragger(arg);
    }catch(const Base::Exception &e){
        e.setPyException();
        throw Py::Exception();
    }
}

Py::Object ViewProviderLinkPy::getLinkView() const {
    return Py::Object(getViewProviderLinkPtr()->getPyLinkView(),true);
}

PyObject *ViewProviderLinkPy::getCustomAttributes(const char* /*attr*/) const
{
    return nullptr;
}

int ViewProviderLinkPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
