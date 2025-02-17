// SPDX-License-Identifier: LGPL-2.1-or-later

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

#ifndef PARTDESIGN_Revolution_H
#define PARTDESIGN_Revolution_H

#include <array>
#include <App/PropertyUnits.h>
#include "FeatureRevolved.h"

namespace PartDesign
{

class PartDesignExport Revolution: public Revolved
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Revolution);

public:
    Revolution();

    /**
     * Compatibility property that is required to preserve behavior from 1.0,
     * that while incorrect may have an impact over user files.
     */
    App::PropertyEnumeration FuseOrder;

    /** @name methods override feature */
    //@{
    App::DocumentObjectExecReturn* execute() override;
    short mustExecute() const override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderRevolution";
    }
    //@}

    enum FuseOrder : std::uint8_t
    {
        BaseFirst,
        FeatureFirst
    };

protected:
    TopoShape makeShape(const TopoShape& base, const TopoShape& revolve) const override;
    bool suggestReversedAngle(double angle) const override;
    void Restore(Base::XMLReader& reader) override;

private:
    static const char* TypeEnums[];
    static const char* FuseOrderEnums[];
};

}  // namespace PartDesign

#endif  // PARTDESIGN_Revolution_H
