// SPDX-License-Identifier: LGPL-2.1-or-later

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


#include "ImagePlane.h"


using namespace Image;

PROPERTY_SOURCE(Image::ImagePlane, App::GeoFeature)

const App::PropertyFloatConstraint::Constraints cropPercent = {0.0, 100.0, 0.01};

ImagePlane::ImagePlane()
{
    ADD_PROPERTY_TYPE(ImageFile, (nullptr), "ImagePlane", App::Prop_None, "File of the image");
    ADD_PROPERTY_TYPE(XSize, (100), "ImagePlane", App::Prop_None, "Size of a pixel in X");
    ADD_PROPERTY_TYPE(YSize, (100), "ImagePlane", App::Prop_None, "Size of a pixel in Y");
    ADD_PROPERTY_TYPE(
        CropLeft,
        (0.0),
        "ImagePlane",
        App::Prop_None,
        "Left crop, as a percentage of the image width"
    );
    CropLeft.setConstraints(&cropPercent);
    ADD_PROPERTY_TYPE(
        CropRight,
        (0.0),
        "ImagePlane",
        App::Prop_None,
        "Right crop, as a percentage of the image width"
    );
    CropRight.setConstraints(&cropPercent);
    ADD_PROPERTY_TYPE(
        CropTop,
        (0.0),
        "ImagePlane",
        App::Prop_None,
        "Top crop, as a percentage of the image height"
    );
    CropTop.setConstraints(&cropPercent);
    ADD_PROPERTY_TYPE(
        CropBottom,
        (0.0),
        "ImagePlane",
        App::Prop_None,
        "Bottom crop, as a percentage of the image height"
    );
    CropBottom.setConstraints(&cropPercent);
}

int ImagePlane::getXSizeInPixel()
{
    return int(XSize.getValue() * XPixelsPerMeter / 1000);
}

int ImagePlane::getYSizeInPixel()
{
    return int(YSize.getValue() * YPixelsPerMeter / 1000);
}

void ImagePlane::setXSizeInPixel(int value)
{
    XSize.setValue(double(value) * 1000.0 / XPixelsPerMeter);
}

void ImagePlane::setYSizeInPixel(int value)
{
    YSize.setValue(double(value) * 1000.0 / YPixelsPerMeter);
}
