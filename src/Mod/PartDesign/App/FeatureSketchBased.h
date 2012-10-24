/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_SketchBased_H
#define PARTDESIGN_SketchBased_H

#include <App/PropertyStandard.h>
#include "Feature.h"

class TopoDS_Face;
class TopoDS_Wire;

namespace PartDesign
{

class PartDesignExport SketchBased : public PartDesign::Feature
{
    PROPERTY_HEADER(PartDesign::SketchBased);

public:
    SketchBased();

    /// Common properties for all sketch based features
    App::PropertyLink   Sketch;
    /// Reverse extrusion direction
    App::PropertyBool       Reversed;
    /// Make extrusion symmetric to sketch plane
    App::PropertyBool       Midplane;

    short mustExecute() const;

    /** calculates and updates the Placement property based on the Sketch
     *  or its support if it has one
      */
    void positionBySketch(void);

    /// retrieves the number of axes in the linked sketch (defined as construction lines)
    int getSketchAxisCount(void) const;

protected:
    void onChanged(const App::Property* prop);
    TopoDS_Face validateFace(const TopoDS_Face&) const;
    TopoDS_Shape makeFace(const std::vector<TopoDS_Wire>&) const;
    TopoDS_Shape makeFace(std::list<TopoDS_Wire>&) const; // for internal use only
    bool isInside(const TopoDS_Wire&, const TopoDS_Wire&) const;
};

} //namespace PartDesign


#endif // PARTDESIGN_SketchBased_H
