/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PART_CUSTOMFEATURE_H
#define PART_CUSTOMFEATURE_H

#include <Mod/Part/App/PartFeature.h>

namespace Part
{

/** Base class of all custom feature classes which are almost used as base
 * for python feature classes.
 */
class PartExport CustomFeature : public Part::Feature
{
    PROPERTY_HEADER(Part::UserFeature);

public:
    /// Constructor
    CustomFeature(void);
    virtual ~CustomFeature();

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual short mustExecute(void) const;
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "PartGui::ViewProviderCustom";
    }
};

typedef App::FeaturePythonT<CustomFeature> CustomFeaturePython;

} //namespace Part


#endif // PART_CUSTOMFEATURE_H
