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


#ifndef PARTDESIGN_FEATURETHICKNESS_H
#define PARTDESIGN_FEATURETHICKNESS_H

#include <App/PropertyStandard.h>
#include <App/PropertyLinks.h>
#include <App/PropertyUnits.h>
#include "FeatureDressUp.h"

namespace PartDesign
{

class PartDesignExport Thickness : public DressUp
{
    PROPERTY_HEADER(PartDesign::Thickness);

public:
    Thickness();

    App::PropertyLength         Value;
    App::PropertyBool           Reversed;
    App::PropertyEnumeration    Mode;
    App::PropertyEnumeration    Join;      
    App::PropertyBool           Refine;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    virtual void setupObject () override;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartDesignGui::ViewProviderThickness";
    }
    //@}
private:
    static const char* ModeEnums[];
    static const char* JoinEnums[];
};

} //namespace PartDesign


#endif // PARTDESIGN_FEATURETHICKNESS_H
