/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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



#ifndef PART_FEATUREPARTBOX_H
#define PART_FEATUREPARTBOX_H

#include <App/PropertyStandard.h>

#include "PrimitiveFeature.h"

namespace Part
{

class PartExport Box :public Part::Primitive
{
    PROPERTY_HEADER(Part::Box);

public:
    Box();

    App::PropertyLength Length,Height,Width;


    /** @name methods override feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderBox";
    }
protected:
    void Restore(Base::XMLReader &reader);
    /// get called by the container when a property has changed
    virtual void onChanged (const App::Property* prop);
    //@}
};

} //namespace Part


#endif // PART_FEATUREPARTBOX_H
