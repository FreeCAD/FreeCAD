/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef _TechDraw_DrawViewDimExtent_h_
#define _TechDraw_DrawViewDimExtent_h_

#include <tuple>

#include <App/DocumentObject.h>
#include <App/PropertyLinks.h>

#include "DrawViewDimension.h"


namespace TechDraw {

class TechDrawExport DrawViewDimExtent : public TechDraw::DrawViewDimension
{
    PROPERTY_HEADER(TechDraw::DrawViewDimExtent);

public:
    /// Constructor
    DrawViewDimExtent();
    virtual ~DrawViewDimExtent();

    App::PropertyLinkSubList       Source;                       //DrawViewPart & SubElements(Edges)
                                                                 //Cosmetic End points are stored in DVD::References2d
    App::PropertyLinkSubList       Source3d;                     //Part::Feature & SubElements  TBI
    App::PropertyInteger           DirExtent;                    //Horizontal, Vertical, TBD
    App::PropertyStringList        CosmeticTags;                 //id of cosmetic end points.

    virtual App::DocumentObjectExecReturn *execute(void);
    virtual short mustExecute() const;
    virtual void unsetupObject();

    virtual bool checkReferences2D(void) const;

    //return PyObject as DrawViewDimExtentPy
    virtual PyObject *getPyObject(void);

protected:
    virtual void onChanged(const App::Property* prop);
    std::vector<std::string> getSubNames(void);
    virtual pointPair getPointsTwoVerts();

private:
};

} //namespace TechDraw
#endif
