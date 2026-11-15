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


#include "ImagePlane.h"


using namespace Image;

PROPERTY_SOURCE(Image::ImagePlane, App::GeoFeature)


ImagePlane::ImagePlane()
{
    ADD_PROPERTY_TYPE(ImageFile, (nullptr), "ImagePlane", App::Prop_None, "File of the image");
    ADD_PROPERTY_TYPE(XSize, (100), "ImagePlane", App::Prop_None, "Size of a pixel in X");
    ADD_PROPERTY_TYPE(YSize, (100), "ImagePlane", App::Prop_None, "Size of a pixel in Y");
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
