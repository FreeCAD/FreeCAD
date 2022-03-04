/***************************************************************************
 *   Copyright (c) 2016 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef GUI_VIEWPROVIDEREXTENSIONPYTHON_H
#define GUI_VIEWPROVIDEREXTENSIONPYTHON_H

#include "ViewProviderExtension.h"
#include <App/PropertyPythonObject.h>

namespace Gui {

/**
 * Generic Python extension class which allows to behave every extension
 * derived class as Python extension -- simply by subclassing.
 */
template <class ExtensionT>
class ViewProviderExtensionPythonT : public ExtensionT
{
    EXTENSION_PROPERTY_HEADER(Gui::ViewProviderExtensionPythonT<ExtensionT>);

public:
    typedef ExtensionT Inherited;

    ViewProviderExtensionPythonT() {
        ExtensionT::m_isPythonExtension = true;
        ExtensionT::initExtensionType(ViewProviderExtensionPythonT::getExtensionClassTypeId());
    }
    virtual ~ViewProviderExtensionPythonT() {
    }
};

typedef ViewProviderExtensionPythonT<Gui::ViewProviderExtension> ViewProviderExtensionPython;

} //Gui

#endif // GUI_VIEWPROVIDEREXTENSIONPYTHON_H
