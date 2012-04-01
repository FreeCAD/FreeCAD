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


#ifndef ASSEMBLYGUI_ViewProviderAssembly_H
#define ASSEMBLYGUI_ViewProviderAssembly_H

#include "ViewProvider.h"


namespace AssemblyGui {

class AssemblyGuiExport ViewProviderItemAssembly : public AssemblyGui::ViewProviderItem
{
    PROPERTY_HEADER(PartGui::ViewProviderItemAssembly);

public:
    /// constructor
    ViewProviderItemAssembly();
    /// destructor
    virtual ~ViewProviderItemAssembly();

    virtual bool doubleClicked(void);
    virtual void attach(App::DocumentObject *);
    virtual void setDisplayMode(const char* ModeName);
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const;

    virtual std::vector<App::DocumentObject*> claimChildren(void)const;

    virtual std::vector<App::DocumentObject*> claimChildren3D(void)const;

};



} // namespace AssemblyGui


#endif // ASSEMBLYGUI_ViewProviderAssembly_H
