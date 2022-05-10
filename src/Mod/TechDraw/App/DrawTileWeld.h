/***************************************************************************
 *   Copyright (c) 2019 WandererFan <wandererfan@gmail.com>                *
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

#ifndef _TechDraw_DrawTileWeld_h_
#define _TechDraw_DrawTileWeld_h_

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyFile.h>
#include <App/PropertyStandard.h>

#include "DrawTile.h"


namespace TechDraw
{

class TechDrawExport DrawTileWeld : public TechDraw::DrawTile
{
    PROPERTY_HEADER(TechDraw::DrawTileWeld);

public:
    DrawTileWeld();
    virtual ~DrawTileWeld();

    App::PropertyString       LeftText;
    App::PropertyString       RightText;
    App::PropertyString       CenterText;
    App::PropertyFile         SymbolFile;
    App::PropertyFileIncluded SymbolIncluded;

    virtual short mustExecute() const;
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual void onDocumentRestored();
    virtual void setupObject();

    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderTile";
    }
    virtual PyObject *getPyObject(void);
    virtual QRectF getRect() const { return QRectF(0,0,1,1);}

    void replaceSymbolIncluded(std::string newSymbolFile);
    void setupSymbolIncluded(void);
//    void replaceSymbol(std::string newSymbolFile);

    std::string prefSymbol(void);

protected:
    virtual void onChanged(const App::Property* prop);

private:
};

typedef App::FeaturePythonT<DrawTileWeld> DrawTileWeldPython;

} //namespace TechDraw
#endif
