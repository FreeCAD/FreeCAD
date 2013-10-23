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



#ifndef PARTGUI_VIEWPROVIDERPARTSPLINE_H
#define PARTGUI_VIEWPROVIDERPARTSPLINE_H

#include "ViewProviderExt.h"

namespace PartGui
{

class PartGuiExport ViewProviderSpline : public ViewProviderPartExt
{
    PROPERTY_HEADER(PartGui::ViewProviderSpline);

public:
    /// constructor
    ViewProviderSpline();
    /// destructor
    virtual ~ViewProviderSpline();

    // Display properties
    App::PropertyBool ControlPoints;

    void updateData(const App::Property* prop);

protected:
    void onChanged(const App::Property* prop);
    void showControlPoints(bool, const App::Property* prop);
    void showControlPointsOfEdge(const TopoDS_Edge&);
    void showControlPointsOfFace(const TopoDS_Face&);

    SoSwitch     *pcControlPoints;
};

} //namespace PartGui


#endif // PARTGUI_VIEWPROVIDERPARTSPLINE_H

