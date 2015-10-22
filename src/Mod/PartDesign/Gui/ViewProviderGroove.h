/******************************************************************************
 *   Copyright (c)2012 Jan Rheinlaender <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#ifndef PARTGUI_ViewProviderGroove_H
#define PARTGUI_ViewProviderGroove_H

#include "ViewProvider.h"


namespace PartDesignGui {

class PartDesignGuiExport ViewProviderGroove : public ViewProvider
{
    PROPERTY_HEADER(PartGui::ViewProviderGroove);

public:
    /// constructor
    ViewProviderGroove();
    /// destructor
    virtual ~ViewProviderGroove();

    /// grouping handling 
    std::vector<App::DocumentObject*> claimChildren(void)const;

    void setupContextMenu(QMenu*, QObject*, const char*);

    virtual bool onDelete(const std::vector<std::string> &);

protected:
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);

};


} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderGroove_H
