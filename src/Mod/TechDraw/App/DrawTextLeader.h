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

#ifndef _TechDraw_DrawTextLeader_h_
#define _TechDraw_DrawTextLeader_h_

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>

#include "DrawLeaderLine.h"


namespace TechDraw
{

class TechDrawExport DrawTextLeader : public TechDraw::DrawLeaderLine
{
    PROPERTY_HEADER(TechDraw::DrawTextLeader);

public:
    DrawTextLeader();
    virtual ~DrawTextLeader();

    App::PropertyString       LeaderText;
    App::PropertyVector       TextPosition;

    virtual short mustExecute() const;
    virtual App::DocumentObjectExecReturn *execute(void);

    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderTextLeader";
    }
    virtual PyObject *getPyObject(void);
    virtual QRectF getRect() const { return QRectF(0,0,1,1);}

protected:
    virtual void onChanged(const App::Property* prop);

private:
};

typedef App::FeaturePythonT<DrawTextLeader> DrawTextLeaderPython;

} //namespace TechDraw
#endif
