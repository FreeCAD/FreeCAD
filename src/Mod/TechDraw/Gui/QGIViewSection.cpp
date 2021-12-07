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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <cmath>
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QFile>
#include <QFileInfo>
#include <QGraphicsScene>
#include <QMenu>
#include <QMouseEvent>
#include <QGraphicsSceneHoverEvent>
#include <QPainterPathStroker>
#include <QPainter>
#include <QPainterPath>
#include <QTextOption>
#endif

#include <qmath.h>

#include <Base/Console.h>
#include <App/Material.h>

#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawViewSection.h>


#include "ZVALUE.h"
#include "ViewProviderViewSection.h"
#include "QGIFace.h"
#include "QGIViewSection.h"

using namespace TechDrawGui;

void QGIViewSection::draw()
{
    if (!isVisible()) {
        return;
    }

    QGIViewPart::draw();
    drawSectionFace();
}

void QGIViewSection::drawSectionFace()
{
    auto section( dynamic_cast<TechDraw::DrawViewSection *>(getViewObject()) );
    if( section == nullptr ) {
        return;
    }

    if ( !section->hasGeometry()) {
        return;
    }
    Gui::ViewProvider* gvp = QGIView::getViewProvider(section);
    ViewProviderViewSection* sectionVp = dynamic_cast<ViewProviderViewSection*>(gvp);
    if ((sectionVp == nullptr)  ||
        (!sectionVp->ShowCutSurface.getValue())) {
        return;
    }

    float lineWidth    = sectionVp->LineWidth.getValue();

    auto sectionFaces( section->getTDFaceGeometry() );
    if (sectionFaces.empty()) {
        Base::Console().
             Log("INFO - QGIViewSection::drawSectionFace - No sectionFaces available. Check Section plane.\n");
        return;
    }

    std::vector<TechDraw::FacePtr>::iterator fit = sectionFaces.begin();
    int i = 0;
    for(; fit != sectionFaces.end(); fit++, i++) {
        QGIFace* newFace = drawFace(*fit,-1);
        newFace->setZValue(ZVALUE::SECTIONFACE);
        if (section->showSectionEdges()) {
            newFace->setDrawEdges(true);
            newFace->setStyle(Qt::SolidLine);
            newFace->setWidth(lineWidth);
        } else {
            newFace->setDrawEdges(false);
        }

        if (section->CutSurfaceDisplay.isValue("Hide")) {
            return;
        } else if (section->CutSurfaceDisplay.isValue("Color")) {
            newFace->isHatched(false);
            newFace->setFillMode(QGIFace::PlainFill);
            QColor faceColor = (sectionVp->CutSurfaceColor.getValue()).asValue<QColor>();
            newFace->setFillColor(faceColor);
            newFace->setFillStyle(Qt::SolidPattern);
        } else if (section->CutSurfaceDisplay.isValue("SvgHatch")) {
            if (getExporting()) {
                newFace->hideSvg(true);
            } else {
                newFace->hideSvg(false);
            }
            newFace->setFillMode(QGIFace::SvgFill);
            newFace->setHatchColor(sectionVp->HatchColor.getValue());
            newFace->setHatchScale(section->HatchScale.getValue());
//                std::string hatchSpec = section->FileHatchPattern.getValue();
            std::string hatchSpec = section->SvgIncluded.getValue();
            newFace->setHatchFile(hatchSpec);
        } else if (section->CutSurfaceDisplay.isValue("PatHatch")) {
            newFace->isHatched(true);
            newFace->setFillMode(QGIFace::GeomHatchFill);
            newFace->setHatchColor(sectionVp->GeomHatchColor.getValue());
            newFace->setHatchScale(section->HatchScale.getValue());
            newFace->setLineWeight(sectionVp->WeightPattern.getValue());
            std::vector<TechDraw::LineSet> lineSets = section->getDrawableLines(i);
            if (!lineSets.empty()) {
                newFace->clearLineSets();
                for (auto& ls: lineSets) {
                    newFace->addLineSet(ls);
                }
            }
        } else {
            Base::Console().Warning("QGIVS::draw - unknown CutSurfaceDisplay: %d\n", 
                                    section->CutSurfaceDisplay.getValue());
        }

        newFace->draw();
        newFace->setPrettyNormal();
        newFace->setAcceptHoverEvents(false);
        newFace->setFlag(QGraphicsItem::ItemIsSelectable, false);
    }
}

void QGIViewSection::updateView(bool update)
{
    Q_UNUSED(update);
    auto viewPart( dynamic_cast<TechDraw::DrawViewSection *>(getViewObject()) );
    if( viewPart == nullptr ) {
        return;
    }
    draw();
    QGIView::updateView(update);
}

void QGIViewSection::drawSectionLine(TechDraw::DrawViewSection* s, bool b)
{
    Q_UNUSED(b);
    Q_UNUSED(s);
   //override QGIVP::drawSectionLine
}

