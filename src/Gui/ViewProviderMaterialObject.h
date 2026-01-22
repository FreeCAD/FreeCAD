/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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


#include "ViewProviderDocumentObject.h"
#include "ViewProviderFeaturePython.h"

namespace Gui
{

class GuiExport ViewProviderMaterialObject: public ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderMaterialObject);

public:
    /// constructor.
    ViewProviderMaterialObject();
    /// destructor.
    ~ViewProviderMaterialObject() override;

    QIcon getIcon() const override;

    bool doubleClicked() override;

    // shows solid in the tree
    bool isShow() const override
    {
        return true;
    }
};

using ViewProviderMaterialObjectPython = ViewProviderFeaturePythonT<ViewProviderMaterialObject>;

}  // namespace Gui
