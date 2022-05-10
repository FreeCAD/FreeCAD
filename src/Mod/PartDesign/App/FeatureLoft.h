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


#ifndef PARTDESIGN_Loft_H
#define PARTDESIGN_Loft_H

#include "FeatureSketchBased.h"

namespace PartDesign
{

class PartDesignExport Loft : public ProfileBased
{
    PROPERTY_HEADER(PartDesign::Loft);

public:
    Loft();

    App::PropertyLinkSubList Sections;
    App::PropertyBool Ruled;
    App::PropertyBool Closed;

    /** @name methods override feature */
    //@{
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderLoft";
    }
    //@}

protected:
    // handle changed property
    virtual void handleChangedPropertyType(Base::XMLReader& reader, const char* TypeName, App::Property* prop);

private:
    //static const char* TypeEnums[];
    //static const char* SideEnums[];
};

class PartDesignExport AdditiveLoft : public Loft {

    PROPERTY_HEADER(PartDesign::AdditiveLoft);
public:
    AdditiveLoft();
};

class PartDesignExport SubtractiveLoft : public Loft {

    PROPERTY_HEADER(PartDesign::SubtractiveLoft);
public:
    SubtractiveLoft();
};

} //namespace PartDesign


#endif // PART_Loft_H
