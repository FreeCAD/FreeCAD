/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include "PreCompiled.h"

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/PyObjectBase.h>

#include "CenterLine.h"
#include "CosmeticExtension.h"
#include "Cosmetic.h"
#include "DrawComplexSection.h"
#include "DrawGeomHatch.h"
#include "DrawHatch.h"
#include "DrawLeaderLine.h"
#include "DrawPage.h"
#include "DrawParametricTemplate.h"
#include "DrawProjGroup.h"
#include "DrawProjGroupItem.h"
#include "DrawRichAnno.h"
#include "DrawSVGTemplate.h"
#include "DrawTile.h"
#include "DrawTileWeld.h"
#include "DrawViewAnnotation.h"
#include "DrawViewArch.h"
#include "DrawViewBalloon.h"
#include "DrawViewClip.h"
#include "DrawViewCollection.h"
#include "DrawViewDetail.h"
#include "DrawViewDimension.h"
#include "DrawViewDimExtent.h"
#include "DrawViewDraft.h"
#include "DrawView.h"
#include "DrawViewImage.h"
#include "DrawViewPart.h"
#include "DrawViewSection.h"
#include "DrawViewSpreadsheet.h"
#include "DrawViewSymbol.h"
#include "DrawWeldSymbol.h"
#include "FeatureProjection.h"
#include "LandmarkDimension.h"
#include "PropertyCenterLineList.h"
#include "PropertyCosmeticEdgeList.h"
#include "PropertyCosmeticVertexList.h"
#include "PropertyGeomFormatList.h"



namespace TechDraw {
    extern PyObject* initModule();
}

/* Python entry */
PyMOD_INIT_FUNC(TechDraw)
{
    // load dependent module
    try {
        Base::Interpreter().loadModule("Part");
        Base::Interpreter().loadModule("Measure");
    }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        PyMOD_Return(nullptr);
    }
    PyObject* mod = TechDraw::initModule();
    Base::Console().Log("Loading TechDraw module... done\n");

    TechDraw::DrawPage            ::init();
    TechDraw::DrawView            ::init();
    TechDraw::DrawViewCollection  ::init();
    TechDraw::DrawViewPart        ::init();
    TechDraw::DrawViewAnnotation  ::init();
    TechDraw::DrawViewSymbol      ::init();
    TechDraw::DrawViewSpreadsheet ::init();

    TechDraw::DrawViewSection     ::init();
    TechDraw::DrawComplexSection     ::init();
    TechDraw::DrawViewDimension   ::init();
    TechDraw::DrawViewDimExtent   ::init();
    TechDraw::LandmarkDimension     ::init();
    TechDraw::DrawProjGroup       ::init();
    TechDraw::DrawProjGroupItem   ::init();
    TechDraw::DrawViewDetail      ::init();
    TechDraw::DrawViewBalloon     ::init();
    TechDraw::DrawLeaderLine      ::init();
    TechDraw::DrawRichAnno        ::init();

    TechDraw::DrawTemplate        ::init();
    TechDraw::DrawParametricTemplate::init();
    TechDraw::DrawSVGTemplate     ::init();

    TechDraw::DrawViewClip        ::init();
    TechDraw::DrawHatch           ::init();
    TechDraw::DrawGeomHatch       ::init();
    TechDraw::DrawViewDraft       ::init();
    TechDraw::DrawViewArch        ::init();
    TechDraw::DrawViewImage       ::init();
    TechDraw::DrawTile            ::init();
    TechDraw::DrawTileWeld        ::init();
    TechDraw::DrawWeldSymbol      ::init();

    TechDraw::PropertyGeomFormatList::init();
    TechDraw::GeomFormat            ::init();
    TechDraw::PropertyCenterLineList::init();
    TechDraw::CenterLine            ::init();
    TechDraw::PropertyCosmeticEdgeList::init();
    TechDraw::CosmeticEdge          ::init();
    TechDraw::PropertyCosmeticVertexList::init();
    TechDraw::CosmeticVertex        ::init();

    TechDraw::CosmeticExtension     ::init();
    TechDraw::CosmeticExtensionPython::init();

    TechDraw::FeatureProjection::init();

   // are these python init calls required?  some modules don't have them
   // Python Types
    TechDraw::DrawPagePython      ::init();
    TechDraw::DrawViewPython      ::init();
    TechDraw::DrawViewPartPython  ::init();
    TechDraw::DrawViewSectionPython::init();
    TechDraw::DrawComplexSectionPython ::init();
    TechDraw::DrawTemplatePython  ::init();
    TechDraw::DrawViewSymbolPython::init();
    TechDraw::DrawLeaderLinePython::init();
    TechDraw::DrawRichAnnoPython  ::init();
    TechDraw::DrawTilePython      ::init();
    TechDraw::DrawTileWeldPython  ::init();
    TechDraw::DrawWeldSymbolPython::init();

    PyMOD_Return(mod);
}
