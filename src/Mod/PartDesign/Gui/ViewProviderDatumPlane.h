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


#ifndef PARTGUI_ViewProviderDatumPlane_H
#define PARTGUI_ViewProviderDatumPlane_H

#include "Gui/ViewProviderGeometryObject.h"
#include "ViewProviderDatum.h"

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderDatumPlane : public PartDesignGui::ViewProviderDatum
{
    PROPERTY_HEADER(PartDesignGui::ViewProviderDatumPlane);

public:
    /// Constructor
    ViewProviderDatumPlane();
    virtual ~ViewProviderDatumPlane();

    virtual void attach ( App::DocumentObject *obj );
    virtual void updateData(const App::Property*);

    void setExtents (Base::BoundBox3d bbox);
    void setExtents(double l, double w);

private:
    SoCoordinate3 *pCoords;
};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderDatumPlane_H
