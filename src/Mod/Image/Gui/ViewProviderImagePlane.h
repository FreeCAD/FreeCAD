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


#ifndef IMAGE_ViewProviderImagePlane_H
#define IMAGE_ViewProviderImagePlane_H

#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/SoFCSelection.h>

class SoDragger;
class SoJackDragger;
class SoCoordinate3;
class SoDrawStyle;  
class SoTexture2; 

namespace ImageGui
{

class ImageGuiExport ViewProviderImagePlane : public Gui::ViewProviderGeometryObject
{
    PROPERTY_HEADER(RobotGui::ViewProviderImagePlane);

public:
    /// constructor.
    ViewProviderImagePlane();

    /// destructor.
    ~ViewProviderImagePlane();

    void attach(App::DocumentObject *pcObject);
    void setDisplayMode(const char* ModeName);
    std::vector<std::string> getDisplayModes() const;
    void updateData(const App::Property*);

protected:
    Gui::SoFCSelection    * pcImagePlaneRoot;
    SoCoordinate3         * pcCoords;
    SoDrawStyle           * pcDrawStyle;
    SoTexture2            * texture;
 };

} //namespace RobotGui


#endif // IMAGE_ViewProviderImagePlane_H
