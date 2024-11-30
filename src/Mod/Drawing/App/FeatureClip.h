/***************************************************************************
 *   Copyright (c) Yorik van Havre <yorik@uncreated.net> 2012              *
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

#ifndef _FeatureClip_h_
#define _FeatureClip_h_

#include <App/DocumentObjectGroup.h>
#include <App/PropertyStandard.h>
#include <Mod/Drawing/DrawingGlobal.h>


namespace Drawing
{

/** Base class of all View Features in the drawing module
 */
class DrawingExport FeatureClip: public App::DocumentObjectGroup
{
    PROPERTY_HEADER(Drawing::FeatureClip);

public:
    /// Constructor
    FeatureClip(void);
    virtual ~FeatureClip();

    App::PropertyFloat X;
    App::PropertyFloat Y;
    App::PropertyFloat Width;
    App::PropertyFloat Height;
    App::PropertyBool ShowFrame;
    App::PropertyString ViewResult;
    App::PropertyBool Visible;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn* execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const
    {
        return "DrawingGui::ViewProviderDrawingClip";
    }

protected:
    void onChanged(const App::Property* prop);
};


}  // namespace Drawing


#endif
