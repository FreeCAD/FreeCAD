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


#ifndef PARTGUI_ViewProviderDatumPoint_H
#define PARTGUI_ViewProviderDatumPoint_H

#include "Gui/ViewProviderGeometryObject.h"
#include "ViewProviderDatum.h"

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderDatumPoint : public PartDesignGui::ViewProviderDatum
{
    PROPERTY_HEADER(PartDesignGui::ViewProviderDatumPoint);

public:
    /// Constructor
    ViewProviderDatumPoint();
    virtual ~ViewProviderDatumPoint();

    virtual void attach ( App::DocumentObject *obj );

    // Note: don't overload setExtents () here because point doesn't really depends on it

protected:
    virtual void onChanged(const App::Property* prop);
};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderDatumPoint_H
