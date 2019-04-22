/***************************************************************************
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

#ifndef _TechDraw_DrawViewBalloon_h_
#define _TechDraw_DrawViewBalloon_h_
#include <tuple>

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>

#include "DrawView.h"

class TopoDS_Shape;

namespace Measure {
class Measurement;
}
namespace TechDraw {

class DrawViewPart;

class TechDrawExport DrawViewBalloon : public TechDraw::DrawView
{
    PROPERTY_HEADER(TechDraw::DrawViewBalloon);

public:
    /// Constructor
    DrawViewBalloon();
    virtual ~DrawViewBalloon();

    App::PropertyLink        sourceView;
    App::PropertyString      Text;
    App::PropertyEnumeration EndType;
    App::PropertyEnumeration Symbol;
    App::PropertyFloat       SymbolScale;
    App::PropertyFloat       OriginX;
    App::PropertyFloat       OriginY;
    App::PropertyBool        OriginIsSet;
    App::PropertyFloat       TextWrapLen;

    short mustExecute() const;

    DrawViewPart* getViewPart() const;

    //virtual PyObject *getPyObject(void);

    virtual App::DocumentObjectExecReturn *execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderBalloon";
    }

    static const char* balloonTypeEnums[];
    static const char* endTypeEnums[];

protected:
    void onChanged(const App::Property* prop);
    virtual void onDocumentRestored();

private:
};

} //namespace TechDraw
#endif
