/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef APP_MEASUREDISTANCE_H
#define APP_MEASUREDISTANCE_H

#include "DocumentObject.h"
#include "PropertyGeo.h"
#include "PropertyUnits.h"


namespace App
{

class AppExport MeasureDistance : public DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::MeasureDistance);

public:
    /// Constructor
    MeasureDistance();
    ~MeasureDistance() override;

    App::PropertyVector P1;
    App::PropertyVector P2;
    App::PropertyDistance Distance;

    /// recalculate the object
    DocumentObjectExecReturn *execute() override;

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "Gui::ViewProviderMeasureDistance";
    }

protected:
    void onChanged(const Property* prop) override;
};

} //namespace App


#endif // APP_MEASUREDISTANCE_H
