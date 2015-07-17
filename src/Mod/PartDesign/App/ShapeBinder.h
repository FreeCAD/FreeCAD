/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef PARTDESIGN_DATUMSHAPE_H
#define PARTDESIGN_DATUMSHAPE_H

#include <QString>
#include <App/PropertyLinks.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/Part2DObject.h>

namespace PartDesign
{

/*Those two feature are not realy a classical datum. They are fully defined shapes and not
 *infinit geometries likeplanes and lines. Also they are not calculated by references and hence
 *are not "attaced" to anything. Furthermore real shapes must be visualized. This makes it hard
 *to reuse the existing datum infrastructure and a special handling foor those two types is
 *created.
 */
// TODO Add a better documentation (2015-09-11, Fat-Zer)

class PartDesignExport ShapeBinder : public Part::Feature
{
    PROPERTY_HEADER(PartDesign::ShapeBinder);

public:
    ShapeBinder();
    virtual ~ShapeBinder();

    App::PropertyLinkSubList    Support;

    static TopoShape buildShapeFromReferences(std::vector<App::DocumentObject*> objects, std::vector<std::string> subobjects);

    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderShapeBinder";
    }

protected:
    virtual void onChanged(const App::Property* prop);
};

//this class is needed as long as sketch-based features can only work with Part2DObjects
class PartDesignExport ShapeBinder2D : public Part::Part2DObject
{
    PROPERTY_HEADER(PartDesign::ShapeBinder2D);

public:
    ShapeBinder2D();
    virtual ~ShapeBinder2D();

    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderShapeBinder";
    }

protected:
    virtual void onChanged(const App::Property* prop);
};

} //namespace PartDesign


#endif // PARTDESIGN_DATUMSHAPE_H
