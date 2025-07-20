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

class PartDesignExport Revolution : public Revolved
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Revolution);

public:
    Revolution();

    /** @name methods override feature */
    //@{
    App::DocumentObjectExecReturn *execute() override;
    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderRevolution";
    }
    //@}

protected:
    TopoShape makeShape(const TopoShape& base, const TopoShape& revolve) const override;
    bool suggestReversedAngle(double angle) const override;

private:
    static constexpr const std::size_t numMethods = 6;
    using MethodsArray = std::array<const char*, numMethods>;
    static MethodsArray TypeEnums;
};

} //namespace PartDesign


#endif // PART_Revolution_H
