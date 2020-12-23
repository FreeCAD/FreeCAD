/***************************************************************************
 *   Copyright (c) Yorik van Havre          (yorik@uncreated.net 2012)     *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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




#ifndef _DrawViewAnnotation_h_
#define _DrawViewAnnotation_h_

#include <App/DocumentObject.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/FeaturePython.h>

#include "DrawView.h"

namespace TechDraw
{


class TechDrawExport DrawViewAnnotation : public TechDraw::DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewAnnotation);

public:
    /// Constructor
    DrawViewAnnotation(void);
    virtual ~DrawViewAnnotation();

    App::PropertyStringList   Text;
    App::PropertyFont         Font;
    App::PropertyColor        TextColor;
    App::PropertyLength       TextSize;
    App::PropertyInteger      LineSpace;
    App::PropertyEnumeration  TextStyle; // Plain,Bold,Italic,Bold-Italic
    App::PropertyFloat        MaxWidth;

    virtual QRectF getRect() const;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderAnnotation";
    }

protected:
    virtual void onChanged(const App::Property* prop);

private:
    static const char* TextStyleEnums[];
};

typedef App::FeaturePythonT<DrawViewAnnotation> DrawViewAnnotationPython;


} //namespace TechDraw


#endif
