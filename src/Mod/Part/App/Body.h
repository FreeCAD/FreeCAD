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


#ifndef PART_Body_H
#define PART_Body_H

#include <App/PropertyStandard.h>
#include <Mod/Part/App/PartFeature.h>


namespace Part
{
/** Base class of all body objects in FreeCAD
  * A body is used, e.g. in PartDesign, to agregate
  * some modeling features to one shape. As long as not
  * in edit or active on a workbench, the body shows only the
  * resulting shape to the outside (Tip link).
  */
class PartExport Body : public Part::Feature
{
    PROPERTY_HEADER(PartDesign::Body);

public:
    Body();

    App::PropertyLinkList   Model;
    App::PropertyLink       Tip;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    //const char* getViewProviderName(void) const {
    //    return "PartDesignGui::ViewProviderBody";
    //}
    //@}
};

} //namespace Part


#endif // PART_Body_H
