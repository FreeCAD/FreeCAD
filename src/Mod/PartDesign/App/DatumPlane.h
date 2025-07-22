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


#ifndef PARTDESIGN_DATUMPLANE_H
#define PARTDESIGN_DATUMPLANE_H

#include <Mod/Part/App/DatumFeature.h>
#include <Mod/PartDesign/PartDesignGlobal.h>
#include <App/PropertyUnits.h>

namespace PartDesign
{

class PartDesignExport Plane : public Part::Datum
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Plane);

public:
    Plane();
    ~Plane() override;

    App::PropertyEnumeration ResizeMode;
    App::PropertyLength Length;
    App::PropertyLength Width;

    void onChanged(const App::Property *prop) override;
    const char* getViewProviderName() const override {
        return "PartDesignGui::ViewProviderDatumPlane";
    }

    Base::Vector3d getNormal();

protected:
    void Restore(Base::XMLReader& reader) override;

private:
    static const char* ResizeModeEnums[];
};

} //namespace PartDesign


#endif // PARTDESIGN_DATUMPLANE_H
