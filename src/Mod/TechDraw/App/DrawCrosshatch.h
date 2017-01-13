/***************************************************************************
 *   Copyright (c) 2017 WandererFan <wandererfan@gmail.com>                *
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

#ifndef _TechDraw_DrawCrosshatch_h_
#define _TechDraw_DrawCrosshatch_h_

# include <App/DocumentObject.h>
# include <App/FeaturePython.h>
# include <App/PropertyLinks.h>
#include <App/PropertyFile.h>

#include "HatchLine.h"
#include "Geometry.h"

class TopoDS_Edge;
class Bnd_Box;

namespace TechDraw
{
class DrawViewPart;

class TechDrawExport DrawCrosshatch : public App::DocumentObject
{
    PROPERTY_HEADER(TechDraw::DrawCrosshatch);

public:
    DrawCrosshatch();
    virtual ~DrawCrosshatch();

    App::PropertyVector      DirProjection;                            //Source is only valid for original projection?
    App::PropertyLinkSub     Source;                                   //the dvp & face this crosshatch belongs to
    App::PropertyFile        FilePattern;
    App::PropertyString      NamePattern;
    App::PropertyFloat       ScalePattern;
//    App::PropertyFloat       WeightPattern;
//    App::PropertyColor       ColorPattern;
//    App::PropertyStringList  LineSpecs;

    virtual short mustExecute() const;
    virtual App::DocumentObjectExecReturn *execute(void);
    virtual void onChanged(const App::Property* prop);
    virtual const char* getViewProviderName(void) const {
        return "TechDrawGui::ViewProviderCrosshatch";
    }
    virtual PyObject *getPyObject(void);

    std::vector<LineSet> getDrawableLines();
    DrawViewPart* getSourceView(void) const;
    void adviseParent(void) const;               //don't like this!
 

protected:
    TopoDS_Edge makeLine(Base::Vector3d s, Base::Vector3d e);
    std::vector<HatchLine> getDecodedSpecsFromFile();
    std::vector<TopoDS_Edge> makeEdgeOverlay(HatchLine hl, Bnd_Box bBox);
    std::vector<LineSet> m_lineSets;

private:
};

typedef App::FeaturePythonT<DrawCrosshatch> DrawCrosshatchPython;



} //namespace TechDraw
#endif
