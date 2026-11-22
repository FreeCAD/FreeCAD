// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2011 Jürgen Riegel
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <App/GeoFeature.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>

namespace Image
{

class AppExport ImagePlane: public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Image::ImagePlane);

public:
    /// Constructor
    ImagePlane();
    ~ImagePlane() override = default;

    App::PropertyFileIncluded ImageFile;
    App::PropertyLength XSize;
    App::PropertyLength YSize;

    int getXSizeInPixel();
    int getYSizeInPixel();
    void setXSizeInPixel(int);
    void setYSizeInPixel(int);

    double XPixelsPerMeter {1000.0};
    double YPixelsPerMeter {1000.0};

    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "Gui::ViewProviderImagePlane";
    }
};

}  // namespace Image
