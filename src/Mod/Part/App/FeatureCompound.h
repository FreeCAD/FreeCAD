/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PART_FEATURECOMPOUND_H
#define PART_FEATURECOMPOUND_H

#include <App/PropertyLinks.h>
#include "PartFeature.h"

namespace Part
{

class Compound : public Part::Feature
{
    PROPERTY_HEADER(Part::Compound);

public:
    Compound();
    virtual ~Compound();

    App::PropertyLinkList Links;

    /** @name methods override feature */
    //@{
    short mustExecute() const;
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderCompound";
    }
    //@}
};

} //namespace Part


#endif // PART_FEATURECOMPOUND_H

