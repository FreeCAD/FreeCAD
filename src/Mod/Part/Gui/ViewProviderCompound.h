/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTGUI_VIEWPROVIDERCOMPOUND_H
#define PARTGUI_VIEWPROVIDERCOMPOUND_H

#include "ViewProvider.h"


namespace PartGui {

class PartGuiExport ViewProviderCompound : public ViewProviderPart
{
    PROPERTY_HEADER(PartGui::ViewProviderCompound);

public:
    /// constructor
    ViewProviderCompound();
    /// destructor
    virtual ~ViewProviderCompound();
    std::vector<App::DocumentObject*> claimChildren() const;
    bool onDelete(const std::vector<std::string> &);

    /// drag and drop
    bool canDragObjects() const;
    bool canDragObject(App::DocumentObject*) const;
    void dragObject(App::DocumentObject*);
    bool canDropObjects() const;
    bool canDropObject(App::DocumentObject*) const;
    void dropObject(App::DocumentObject*);

protected:
    void updateData(const App::Property*);
};

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERCOMPOUND_H
