/***************************************************************************
 *   Copyright (c) 2014-2015 Nathan Miller    <Nathan.A.Mill[at]gmail.com> *
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

#ifndef FEATUREBSURF_H
#define FEATUREBSURF_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/PropertyLinks.h>
#include <GeomFill_BezierCurves.hxx>
#include <Geom_BoundedSurface.hxx>
#include "Mod/Part/App/PartFeature.h"
#include "../FillType.h"

namespace Surface
{



class BSurf : public Part::Feature
{
public:
    App::PropertyLinkSubList aBList; //curves to be turned into a face (2-4 curves allowed).
    App::PropertyInteger filltype;      //Fill method (1, 2, or 3 for Stretch, Coons, and Curved)

    short mustExecute() const;

    /// returns the type name of the view provider
   const char* getViewProviderName(void) const {
       return "SurfaceGui::ViewProviderBSurf";
   }

protected:
    GeomFill_FillingStyle getFillingStyle();
    void getWire(TopoDS_Wire& aWire);
    void createFace(const Handle_Geom_BoundedSurface aSurface);
};

}

#endif // FEATUREBSURF_H
