/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTDESIGN_FEATURECHAMFER_H
#define PARTDESIGN_FEATURECHAMFER_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/PropertyLinks.h>
#include "FeatureDressUp.h"

namespace PartDesign
{

class PartDesignExport Chamfer : public DressUp
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Chamfer);

public:
    Chamfer();

    App::PropertyEnumeration ChamferType;
    App::PropertyQuantityConstraint Size;
    App::PropertyQuantityConstraint Size2;
    App::PropertyAngle Angle;
    App::PropertyBool FlipDirection;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void) override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const override {
        return "PartDesignGui::ViewProviderChamfer";
    }
    //@}

    virtual void onChanged(const App::Property* /*prop*/) override;

    void updateProperties();

protected:
    void Restore(Base::XMLReader &reader) override;
    static const App::PropertyQuantityConstraint::Constraints floatSize;
    static const App::PropertyAngle::Constraints floatAngle;
};

} //namespace Part


#endif // PARTDESIGN_FEATURECHAMFER_H
