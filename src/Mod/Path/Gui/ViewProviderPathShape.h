/***************************************************************************
 *   Copyright (c) 2014 Yorik van Havre <yorik@uncreated.net>              *
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


#ifndef PATH_ViewProviderPathShape_H
#define PATH_ViewProviderPathShape_H

#include "ViewProviderPath.h"

namespace PathGui
{

class PathGuiExport ViewProviderPathShape: public ViewProviderPath
{
    PROPERTY_HEADER(PathGui::ViewProviderPathShape);

public:

    /// grouping handling
    virtual std::vector<App::DocumentObject*> claimChildren(void) const;
    virtual void updateData(const App::Property*);
    virtual bool onDelete(const std::vector<std::string> &);

    /// drag and drop
    virtual bool canDragObjects() const;
    virtual bool canDragObject(App::DocumentObject*) const;
    virtual void dragObject(App::DocumentObject*);
    virtual bool canDropObjects() const;
    virtual bool canDropObject(App::DocumentObject*) const;
    virtual void dropObject(App::DocumentObject*);

    QIcon getIcon(void) const;
};

} //namespace PathGui


#endif // PATH_ViewProviderPathShape_H
