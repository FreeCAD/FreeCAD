/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer <jrheinlaender[at]users.sourceforge.net>     *
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


#ifndef PART_DATUMFEATURE_H
#define PART_DATUMFEATURE_H

#include <QString>
#include <App/PropertyLinks.h>

#include "PartFeature.h"

namespace Part
{

// This generic class is defined here so that the Sketcher module can access datum features
// without creating a dependency on PartDesign

class PartExport Datum : public Part::Feature
{
    PROPERTY_HEADER(Part::Datum);

public:
    Datum();
    virtual ~Datum();
    //short mustExecute();

    /// The references defining the datum object, e.g. three planes for a point, two planes for a line
    App::PropertyLinkSubList References;
    /// Offsets and angle for defining planes
    App::PropertyFloat Offset;
    App::PropertyFloat Offset2;
    App::PropertyFloat Offset3;
    App::PropertyFloat Angle;

    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);

    /// returns the type name of the view provider
    virtual const char* getViewProviderName(void) const = 0;

    virtual const std::set<QString> getHint() const = 0;

    /// Return the number of offset values that make sense for the current reference combination
    virtual const int offsetsAllowed() const = 0;

    /// Return a shape including Placement representing the datum feature
    TopoDS_Shape getShape() const;

protected:
    void onChanged (const App::Property* prop);
    void onDocumentRestored();

protected:
    std::multiset<QString> refTypes;    

};

} //namespace Part


#endif // PART_DATUMFEATURE_H
