/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinländer                                    *
 *                                   <jrheinlaender@users.sourceforge.net> *
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

#include "AttachExtension.h"


namespace Part
{

// This generic class is defined here so that the Sketcher module can access datum features
// without creating a dependency on PartDesign

class PartExport Datum : public Part::Feature, public Part::AttachExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::Datum);

public:
    Datum();
    ~Datum() override;
    //short mustExecute();

    /// returns the type name of the view provider
    const char* getViewProviderName() const override = 0;

    /// Return a shape including Placement representing the datum feature
    virtual TopoDS_Shape getShape() const;

    /// Returns a point of the feature it counts as it's base
    virtual Base::Vector3d getBasePoint () const;

    App::DocumentObject *getSubObject(const char *subname, PyObject **pyObj,
            Base::Matrix4D *mat, bool transform, int depth) const override;
protected:
    void onDocumentRestored() override;
    void handleChangedPropertyName(Base::XMLReader &reader, const char* TypeName, const char* PropName) override;
};

} //namespace Part


#endif // PART_DATUMFEATURE_H
