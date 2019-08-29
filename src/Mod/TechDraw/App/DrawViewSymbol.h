/***************************************************************************
 *   Copyright (c) Yorik van Havre          (yorik@uncreated.net 2013)     *
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

#ifndef _DrawViewSymbol_h_
#define _DrawViewSymbol_h_

#include <App/DocumentObject.h>
#include "DrawView.h"
#include <App/FeaturePython.h>

#include <Base/BoundBox.h>

namespace TechDraw
{
class DrawPage;


class TechDrawExport DrawViewSymbol : public TechDraw::DrawView
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDraw::DrawViewSymbol);

public:
    /// Constructor
    DrawViewSymbol(void);
    virtual ~DrawViewSymbol();

    App::PropertyString       Symbol;
    App::PropertyStringList   EditableTexts;

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn *execute(void) override;
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const override {
        return "TechDrawGui::ViewProviderSymbol";
    }
    virtual QRectF getRect() const override;
    virtual bool checkFit(TechDraw::DrawPage* p) const override;

    //return PyObject as DrawViewSymbolPy
    virtual PyObject *getPyObject(void) override;

    virtual short mustExecute() const override;


protected:
    virtual void onChanged(const App::Property* prop) override;
    Base::BoundBox3d bbox;
};

typedef App::FeaturePythonT<DrawViewSymbol> DrawViewSymbolPython;


} //namespace TechDraw


#endif
