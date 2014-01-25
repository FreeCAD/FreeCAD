/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef ASSEMBLYGUI_ViewProvider_H
#define ASSEMBLYGUI_ViewProvider_H

#include <Gui/ViewProviderGeometryObject.h>

class SoGroup;

namespace AssemblyGui {

class AssemblyGuiExport ViewProviderItem : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(AssemblyGui::ViewProviderItem);

public:
    /// constructor
    ViewProviderItem();
    /// destructor
    virtual ~ViewProviderItem();

    virtual bool useNewSelectionModel(void) const {return false;}

    // returns the root node where the children gets collected(3D)
    virtual SoGroup* getChildRoot(void) const {return pcChildren;}

    virtual bool doubleClicked(void);
private:
    /// group node for all children collected through claimChildren3D(), reused by all Assembly ViewProviders
    SoGroup *pcChildren;

};



} // namespace AssemblyGui


#endif // ASSEMBLYGUI_ViewProvider_H
