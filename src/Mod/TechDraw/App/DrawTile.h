/***************************************************************************
 *   Copyright (c) 2019 Wanderer Fan <wandererfan@gmail.com>               *
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

#ifndef _TechDraw_DrawTile_h_
#define _TechDraw_DrawTile_h_

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>

#include "DrawView.h"

namespace TechDraw
{

class TechDrawExport DrawTile : public App::DocumentObject
{
    PROPERTY_HEADER(TechDraw::DrawTile);

public:
    DrawTile();
    virtual ~DrawTile();

    App::PropertyLink         TileParent;           //eg DrawWeldSymbol
    App::PropertyIntegerConstraint TileRow;
    App::PropertyIntegerConstraint::Constraints  TileRowConstraints;
    App::PropertyInteger      TileColumn;

    virtual short mustExecute() const;
    virtual App::DocumentObjectExecReturn *execute(void);

    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderTile";
    }
    virtual PyObject *getPyObject(void);
    virtual DrawView* getParent(void) const;

protected:
    virtual void onChanged(const App::Property* prop);
    virtual void handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property * prop);

private:
};

typedef App::FeaturePythonT<DrawTile> DrawTilePython;

} //namespace TechDraw
#endif
