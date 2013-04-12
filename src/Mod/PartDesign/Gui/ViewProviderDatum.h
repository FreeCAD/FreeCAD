/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>        *
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


#ifndef PARTGUI_ViewProviderDatum_H
#define PARTGUI_ViewProviderDatum_H

#include "ViewProvider.h"

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderDatum : public ViewProvider
{
    PROPERTY_HEADER(PartDesignGui::ViewProviderDatum);

public:
    /// constructor
    ViewProviderDatum();
    /// destructor
    virtual ~ViewProviderDatum();

    /// grouping handling 
    void setupContextMenu(QMenu*, QObject*, const char*);

    virtual void attach(App::DocumentObject *);

    /// The datum type (Plane, Line or Point)
    QString datumType;

protected:
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);

};


} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderPad_H
