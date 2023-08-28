/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel (juergen.riegel@web.de)              *
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

#ifndef App_ImagePlane_H
#define App_ImagePlane_H

#include <App/GeoFeature.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>

namespace Image
{

class AppExport ImagePlane : public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Image::ImagePlane);

public:
    /// Constructor
    ImagePlane();
    ~ImagePlane() override = default;

    App::PropertyFileIncluded ImageFile;
    App::PropertyLength       XSize;
    App::PropertyLength       YSize;

    int getXSizeInPixel();
    int getYSizeInPixel();
    void setXSizeInPixel(int);
    void setYSizeInPixel(int);

    double XPixelsPerMeter{1000.0};
    double YPixelsPerMeter{1000.0};

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override {
        return "Gui::ViewProviderImagePlane";
    }
};

} //namespace Image


#endif // App_ImagePlane_H
