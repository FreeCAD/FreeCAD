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


#ifndef PARTDESIGN_DATUMPOINT_H
#define PARTDESIGN_DATUMPOINT_H

#include <QString>
#include <App/PropertyLinks.h>
#include <App/GeoFeature.h>
#include <Mod/Part/App/DatumFeature.h>

namespace PartDesign
{

class PartDesignExport Point : public Part::Datum
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Point);

public:
    Point();
    virtual ~Point();

    const char* getViewProviderName(void) const override {
        return "PartDesignGui::ViewProviderDatumPoint";
    }

    Base::Vector3d getPoint();

    typedef Part::Datum Superclass;

protected:
    virtual void onChanged(const App::Property* prop) override;
    virtual void onDocumentRestored() override;

private:
    void makeShape();

};

} //namespace PartDesign


#endif // PARTDESIGN_DATUMPOINT_H
