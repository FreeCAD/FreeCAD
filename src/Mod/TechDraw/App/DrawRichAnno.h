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

#ifndef _TechDraw_DrawRichAnno_h_
#define _TechDraw_DrawRichAnno_h_

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>

#include "DrawView.h"


namespace TechDraw
{

class TechDrawExport DrawRichAnno : public TechDraw::DrawView
{
    PROPERTY_HEADER(TechDraw::DrawRichAnno);

public:
    DrawRichAnno();
    virtual ~DrawRichAnno();

    App::PropertyLink         AnnoParent;
    App::PropertyString       AnnoText;
//    App::PropertyVector       TextPosition;
    App::PropertyBool         ShowFrame;
    App::PropertyFloat        MaxWidth;
    App::PropertyVector       AttachPoint;

    virtual short mustExecute() const;
    virtual App::DocumentObjectExecReturn *execute(void);

    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderRichAnno";
    }
    virtual PyObject *getPyObject(void);
    virtual QRectF getRect() const { return QRectF(0,0,1,1);}
    DrawView* getBaseView(void) const;

protected:
    virtual void onChanged(const App::Property* prop);

private:
};

typedef App::FeaturePythonT<DrawRichAnno> DrawRichAnnoPython;

} //namespace TechDraw
#endif
