/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef PARTGUI_ViewProviderPrimitive_H
#define PARTGUI_ViewProviderPrimitive_H

#include "ViewProvider.h"
#include "ViewProviderAddSub.h"
#include <Mod/Part/Gui/SoBrepFaceSet.h>

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderPrimitive : public ViewProviderAddSub
{
    PROPERTY_HEADER(PartDesignGui::ViewProviderPrimitive);

public:
    /// constructor
    ViewProviderPrimitive();
    /// destructor
    virtual ~ViewProviderPrimitive();
    
    virtual void attach(App::DocumentObject*);
    virtual void updateData(const App::Property*);
    
protected:
    virtual QIcon getIcon(void) const;
    virtual void setupContextMenu(QMenu* menu, QObject* receiver, const char* member);
    virtual bool  setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    
    void updateAddSubShapeIndicator();
    
    std::string                 displayMode;
};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderBoolean_H
