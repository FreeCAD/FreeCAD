/***************************************************************************
 *   Copyright (c) 2016 Victor Titov (DeepSOIC)      <vv.titov@gmail.com>  *
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

#ifndef PART_FEATUREOFFSET_H
#define PART_FEATUREOFFSET_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <Mod/Part/App/PartFeature.h>

namespace Part
{

class PartExport Offset : public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Offset);

public:
    Offset();
    ~Offset();

    App::PropertyLink  Source;
    App::PropertyFloat Value;
    App::PropertyEnumeration Mode;
    App::PropertyEnumeration Join;
    App::PropertyBool Intersection;
    App::PropertyBool SelfIntersection;
    App::PropertyBool Fill;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual short mustExecute() const override;
    virtual const char* getViewProviderName(void) const override {
        return "PartGui::ViewProviderOffset";
    }
    //@}

private:
    static const char* ModeEnums[];
    static const char* JoinEnums[];
};

class PartExport Offset2D : public Offset
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Offset2D);
public:
    Offset2D();
    ~Offset2D();

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    virtual App::DocumentObjectExecReturn *execute(void) override;
    virtual short mustExecute() const override;
    virtual const char* getViewProviderName(void) const override {
        return "PartGui::ViewProviderOffset2D";
    }
    //@}
};

}
#endif // PART_FEATUREOFFSET_H
