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


#ifndef PARTGUI_ViewProvider_H
#define PARTGUI_ViewProvider_H

#include <Mod/Part/Gui/ViewProvider.h>


namespace PartDesignGui {

class PartDesignGuiExport ViewProvider : public PartGui::ViewProviderPart {
    typedef PartGui::ViewProviderPart inherited;
    PROPERTY_HEADER(PartDesignGui::ViewProvider);

public:
    /// constructor
    ViewProvider();
    /// destructor
    virtual ~ViewProvider();

    virtual bool doubleClicked(void);
    void updateData(const App::Property*);
};



} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderHole_H
