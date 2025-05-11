/***************************************************************************
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
 *   Copyright (c) 2014 WandererFan <wandererfan@gmail.com>                *
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
# include <cmath>

# include <QGraphicsColorizeEffect>
# include <QGraphicsItem>
# include <QRectF>
# include <QRegularExpression>
# include <QRegularExpressionMatch>
#endif

#include <Base/Console.h>
#include <Mod/TechDraw/App/DrawViewArch.h>
#include <Mod/TechDraw/App/DrawViewDraft.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>

#include "QGIViewSymbol.h"
#include "PreferencesGui.h"
#include "QGCustomSvg.h"
#include "QGDisplayArea.h"
#include "Rez.h"
#include "ViewProviderSymbol.h"


using namespace TechDrawGui;
using namespace TechDraw;

QGIViewSymbol::QGIViewSymbol()
{
    setHandlesChildEvents(false);
    setCacheMode(QGraphicsItem::NoCache);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    m_displayArea = new QGDisplayArea();
    addToGroup(m_displayArea);
    m_displayArea->centerAt(0., 0.);

    m_svgItem = new QGCustomSvg();
    m_displayArea->addToGroup(m_svgItem);
    m_svgItem->centerAt(0., 0.);
}

QGIViewSymbol::~QGIViewSymbol()
{
    // m_svgItem belongs to this group and will be deleted by Qt
}

void QGIViewSymbol::setViewSymbolFeature(TechDraw::DrawViewSymbol* obj)
{
    // called from QGVPage. (once)
    setViewFeature(static_cast<TechDraw::DrawView*>(obj));
}

void QGIViewSymbol::updateView(bool update)
{
    auto viewSymbol(dynamic_cast<TechDraw::DrawViewSymbol*>(getViewObject()));
    if (!viewSymbol)
        return;

    if (update || viewSymbol->isTouched() || viewSymbol->Symbol.isTouched()) {
        draw();
    }

    if (viewSymbol->Scale.isTouched()) {
        draw();
    }

    QGIView::updateView(update);
}

void QGIViewSymbol::draw()
{
    if (!isVisible()) {
        return;
    }

    drawSvg();
    QGIView::draw();
}

void QGIViewSymbol::drawSvg()
{
    auto viewSymbol(dynamic_cast<TechDraw::DrawViewSymbol*>(getViewObject()));
    if (!viewSymbol) {
        return;
    }

    auto vp = getViewProvider(viewSymbol);
    auto vps = freecad_cast<ViewProviderSymbol*>(vp);
    if (!vp || !vps) {
        return;
    }

    double scaling{1};
    if (vps->LegacyScaling.getValue()) {
        scaling = legacyScaler(viewSymbol);
    } else {
        scaling = symbolScaler(viewSymbol);
    }

    m_svgItem->setScale(scaling);

    QByteArray qba(viewSymbol->Symbol.getValue(), strlen(viewSymbol->Symbol.getValue()));
    symbolToSvg(qba);
    rotateView();
}

void QGIViewSymbol::symbolToSvg(QByteArray qba)
{
    if (qba.isEmpty()) {
        return;
    }

    prepareGeometryChange();
    if (!m_svgItem->load(&qba)) {
        Base::Console().error("Error - Could not load Symbol into SVG renderer for %s\n",
                              getViewName());
    }
    m_svgItem->centerAt(0., 0.);

    if (Preferences::lightOnDark()) {
        QColor color = PreferencesGui::getAccessibleQColor(QColor(Qt::black));
        QGraphicsColorizeEffect* colorizeEffect = new QGraphicsColorizeEffect();
        colorizeEffect->setColor(color);
        m_svgItem->setGraphicsEffect(colorizeEffect);
    }
    else {
        //remove and delete any existing graphics effect
        if (m_svgItem->graphicsEffect()) {
            m_svgItem->setGraphicsEffect(nullptr);
        }
    }
}

void QGIViewSymbol::rotateView()
{
    QRectF r = m_displayArea->boundingRect();
    m_displayArea->setTransformOriginPoint(r.center());
    double rot = getViewObject()->Rotation.getValue();
    m_displayArea->setRotation(-rot);
}

//! this is the original scaling logic as used in versions <= 1.0
//! it does not scale correctly for svg files that use mm units, but is available for
//! backwards compatibility.  Set General/LegacySvgScaling to true to use this method.
double QGIViewSymbol::legacyScaler(TechDraw::DrawViewSymbol* feature) const
{
    double rezfactor = Rez::getRezFactor();
    double scaling = feature->getScale();
    double pxMm = 3.78;//96px/25.4mm ( CSS/SVG defined value of 96 pixels per inch)
    //    double pxMm = 3.54;                 //90px/25.4mm ( inkscape value version <= 0.91)
    //some software uses different px/in, so symbol will need Scale adjusted.
    //Arch/Draft views are in px and need to be scaled @ rezfactor px/mm to ensure proper representation
    if (feature->isDerivedFrom<TechDraw::DrawViewArch>()
        || feature->isDerivedFrom<TechDraw::DrawViewDraft>()) {
        scaling = scaling * rezfactor;
    }
    else {
        scaling = scaling * rezfactor / pxMm;
    }

    return scaling;
}

//! new symbol scaling logic as of v1.1
//! svg in mm scales correctly.  svg in px will be drawn using scene units (0.1 mm)
//! as pixels.
double QGIViewSymbol::symbolScaler(TechDraw::DrawViewSymbol* feature) const
{
    double scaling = feature->getScale();
    double rezfactor = Rez::getRezFactor();

    QByteArray qba(feature->Symbol.getValue(), strlen(feature->Symbol.getValue()));
    QString qSymbolString = QString::fromUtf8(qba);

    const QString pxToken{QStringLiteral("px")};
    const QString mmToken{QStringLiteral("mm")};

    // heightRegex finds (height="51.8309mm") in the svg text and returns the mm if present
    QString heightRegex = QStringLiteral(R"(height=\"\d*\.?\d+([a-zA-Z]+)\")");
    QRegularExpression reHeight(heightRegex);
    QRegularExpressionMatch matchHeight = reHeight.match(qSymbolString);

    QString matchUnits;
    if (matchHeight.hasMatch()) {
        auto capture0 = matchHeight.captured(0);
        matchUnits = matchHeight.captured(1);
    }

    // if there are no units specified, or the units are px, we just draw the symbol

    if (matchUnits == mmToken) {
        auto svgSize = m_svgItem->renderer()->defaultSize();
        auto vportSize = m_svgItem->renderer()->viewBox();
        // wf: this calculation works, but I don't know why. :(
        // hints here: https://stackoverflow.com/questions/49866474/get-svg-size-from-qsvgrenderer
        // and here: https://stackoverflow.com/questions/7544921/qt-qgraphicssvgitem-renders-too-big-0-5-unit-on-each-side
        scaling *= rezfactor * vportSize.width() / svgSize.width();
    }

    return scaling;
}


