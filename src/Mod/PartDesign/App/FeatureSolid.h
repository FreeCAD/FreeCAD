/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_FeatureSolid_H
#define PARTDESIGN_FeatureSolid_H

#include "Feature.h"

namespace PartDesign
{

/**
 * Solid feature produced by Split
 */
class PartDesignExport Solid : public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Solid);
    typedef Part::Feature inherited;

public:
    App::PropertyLink Parent;
    App::PropertyBool Active;

    Solid();
    virtual void onChanged(const App::Property* prop) override;
    const char* getViewProviderName(void) const override {
        return "PartDesignGui::ViewProviderSolid";
    }
};

} //namespace PartDesign


#endif // PARTDESIGN_Feature_H
