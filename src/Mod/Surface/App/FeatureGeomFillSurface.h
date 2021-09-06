/***************************************************************************
 *   Copyright (c) 2014-2015 Nathan Miller <Nathan.A.Mill[at]gmail.com>    *
 *                           Balázs Bámer                                  *
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

#ifndef FEATUREGEOMFILLSURFACE_H
#define FEATUREGEOMFILLSURFACE_H

#include <Geom_BoundedSurface.hxx>
#include <GeomFill_FillingStyle.hxx>
#include <ShapeExtend_WireData.hxx>

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/PropertyLinks.h>
#include <Mod/Part/App/FeaturePartSpline.h>

namespace Surface
{

class SurfaceExport ShapeValidator
{
protected:
    bool willBezier;
    int edgeCount;

public:
    ShapeValidator();
    void initValidator(void);
    void checkEdge(const TopoDS_Shape& shape);
    void checkAndAdd(const TopoDS_Shape &shape, Handle(ShapeExtend_WireData) *aWD = NULL);
    void checkAndAdd(const Part::TopoShape &ts, const char *subName, Handle(ShapeExtend_WireData) *aWire = NULL);

    bool isBezier() const {
        return willBezier;
    }
    int numEdges() const {
        return edgeCount;
    }
};

class GeomFillSurface : public Part::Spline
{
  PROPERTY_HEADER(Surface::GeomFillSurface);

public:
    GeomFillSurface();
    App::PropertyLinkSubList BoundaryList;  // Curves to be turned into a face (2-4 curves allowed).
    App::PropertyBoolList ReversedList;     // Booleans to handle orientation of the curves
    App::PropertyEnumeration FillType;      // Fill method (1, 2, or 3 for Stretch, Coons, and Curved)

    short mustExecute() const;
    void onChanged(const App::Property*);
    App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "SurfaceGui::ViewProviderGeomFillSurface";
    }

protected:
    GeomFill_FillingStyle getFillingStyle();
    /// True means that all edges have Bezier curves
    bool getWire(TopoDS_Wire& aWire);
    void createFace(const Handle(Geom_BoundedSurface) &aSurface);
    void createBezierSurface(TopoDS_Wire& aWire);
    void createBSplineSurface(TopoDS_Wire& aWire);

private:
    static const char* FillTypeEnums[];
};

}

#endif // FEATUREGEOMFILLSURFACE_H
