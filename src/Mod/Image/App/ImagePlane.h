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

#ifndef Image_ImagePlane_H
#define Image_ImagePlane_H

#include <App/GeoFeature.h>
#include <App/PropertyFile.h>
#include <App/PropertyUnits.h>
#include <Mod/Image/ImageGlobal.h>

namespace Image
{

class ImageExport ImagePlane : public App::GeoFeature
{
    PROPERTY_HEADER(Image::ImagePlane);

public:
    /// Constructor
    ImagePlane();
    virtual ~ImagePlane();

    App::PropertyFileIncluded ImageFile;
    App::PropertyLength       XSize;
    App::PropertyLength       YSize;

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName() const {
        return "ImageGui::ViewProviderImagePlane";
    }
};

} //namespace Image


#endif // Image_ImagePlane_H
