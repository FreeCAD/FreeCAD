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

#include <QPainterPath>
#include <qmath.h>
#endif// #ifndef _PreComp_

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>
#include <Mod/TechDraw/App/CenterLine.h>
#include <Mod/TechDraw/App/Cosmetic.h>
#include <Mod/TechDraw/App/DrawComplexSection.h>
#include <Mod/TechDraw/App/DrawGeomHatch.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawUtil.h>
#include <Mod/TechDraw/App/DrawViewDetail.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewSection.h>
#include <Mod/TechDraw/App/Geometry.h>

#include "MDIViewPage.h"
#include "PreferencesGui.h"
#include "QGICMark.h"
#include "QGICenterLine.h"
#include "QGIEdge.h"
#include "QGIFace.h"
#include "QGIHighlight.h"
#include "QGIMatting.h"
#include "QGISectionLine.h"
#include "QGIVertex.h"
#include "QGIViewPart.h"
#include "Rez.h"
#include "ViewProviderGeomHatch.h"
#include "ViewProviderHatch.h"
#include "ViewProviderViewPart.h"
#include "ZVALUE.h"
#include "PathBuilder.h"


using namespace TechDraw;
using namespace TechDrawGui;
using namespace std;
using DU = DrawUtil;

#define GEOMETRYEDGE 0
#define COSMETICEDGE 1
#define CENTERLINE 2


const float lineScaleFactor = Rez::guiX(1.);// temp fiddle for devel

QGIViewPart::QGIViewPart() : m_isExporting(false)
{
    setCacheMode(QGraphicsItem::NoCache);
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);

    showSection = false;
    m_pathBuilder = new PathBuilder(this);
}

QGIViewPart::~QGIViewPart()
{
    tidy();
    delete m_pathBuilder;
}

QVariant QGIViewPart::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        //There's nothing special for QGIVP to do when selection changes!
    }
    else if (change == ItemSceneChange && scene()) {
        tidy();
    }
    return QGIView::itemChange(change, value);
}

//obs?
void QGIViewPart::tidy()
{
    //Delete any leftover items
    for (QList<QGraphicsItem*>::iterator it = deleteItems.begin(); it != deleteItems.end(); ++it) {
        delete *it;
    }
    deleteItems.clear();
}

void QGIViewPart::setViewPartFeature(TechDraw::DrawViewPart* obj)
{
    if (!obj)
        return;

    setViewFeature(static_cast<TechDraw::DrawView*>(obj));
}

QPainterPath QGIViewPart::drawPainterPath(TechDraw::BaseGeomPtr baseGeom) const
{
    double rot = getViewObject()->Rotation.getValue();
    return m_pathBuilder->geomToPainterPath(baseGeom, rot);
}

void QGIViewPart::updateView(bool update)
{
    //    Base::Console().Message("QGIVP::updateView() - %s\n", getViewObject()->getNameInDocument());
    auto viewPart(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    if (!viewPart)
        return;
    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp)
        return;

    if (update)
        draw();
    QGIView::updateView(update);
}

void QGIViewPart::draw()
{
    if (!isVisible())
        return;

    drawViewPart();
    drawAllHighlights();
    drawMatting();
    //this is old C/L
    drawCenterLines(true);//have to draw centerlines after border to get size correct.
    drawAllSectionLines();//same for section lines
}

void QGIViewPart::drawViewPart()
{
    auto viewPart(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    if (!viewPart)
        return;
    //    Base::Console().Message("QGIVP::DVP() - %s / %s\n", viewPart->getNameInDocument(), viewPart->Label.getValue());
    if (!viewPart->hasGeometry()) {
        removePrimitives();//clean the slate
        removeDecorations();
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp)
        return;

    prepareGeometryChange();
    removePrimitives();//clean the slate
    removeDecorations();

    if (viewPart->handleFaces() && !viewPart->CoarseView.getValue()) {
        drawAllFaces();
    }

    drawAllEdges();

    drawAllVertexes();
}

void QGIViewPart::drawAllFaces(void)
{
    // dvp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));

    std::vector<TechDraw::DrawHatch*> regularHatches = dvp->getHatches();
    std::vector<TechDraw::DrawGeomHatch*> geomHatches = dvp->getGeomHatches();
    const std::vector<TechDraw::FacePtr>& faceGeoms = dvp->getFaceGeometry();
    int iFace(0);
    for (auto& face : faceGeoms) {
        QGIFace* newFace = drawFace(face, iFace);
        newFace->isHatched(false);
        newFace->setFillMode(QGIFace::PlainFill);

        TechDraw::DrawHatch* fHatch = faceIsHatched(iFace, regularHatches);
        TechDraw::DrawGeomHatch* fGeom = faceIsGeomHatched(iFace, geomHatches);
        if (fGeom) {
            // geometric hatch (from PAT hatch specification)
            newFace->isHatched(true);
            newFace->setFillMode(QGIFace::GeomHatchFill);
            std::vector<LineSet> lineSets = fGeom->getTrimmedLines(iFace);
            if (!lineSets.empty()) {
                // this face has geometric hatch lines
                newFace->clearLineSets();
                for (auto& ls : lineSets) {
                    newFace->addLineSet(ls);
                }
            }
            double hatchScale = fGeom->ScalePattern.getValue();
            if (hatchScale > 0.0) {
                newFace->setHatchScale(fGeom->ScalePattern.getValue());
            }
            newFace->setHatchRotation(fGeom->PatternRotation.getValue());
            newFace->setHatchOffset(fGeom->PatternOffset.getValue());
            newFace->setHatchFile(fGeom->PatIncluded.getValue());
            Gui::ViewProvider* gvp = QGIView::getViewProvider(fGeom);
            ViewProviderGeomHatch* geomVp = dynamic_cast<ViewProviderGeomHatch*>(gvp);
            if (geomVp) {
                newFace->setHatchColor(geomVp->ColorPattern.getValue());
                newFace->setLineWeight(geomVp->WeightPattern.getValue());
            }
        } else if (fHatch) {
            // svg or bitmap hatch
            newFace->isHatched(true);
            if (!fHatch->SvgIncluded.isEmpty()) {
                newFace->setHatchFile(fHatch->SvgIncluded.getValue());
            }
            if (fHatch->isSvgHatch()) {
                // svg tile hatch
                newFace->setFillMode(QGIFace::SvgFill);
                if (getExporting()) {
                    // SVG hatches don't work correctly when exported to PDF, so we need
                    // to tell the face to use a bitmap substitution for the SVG
                    newFace->hideSvg(true);
                } else {
                    newFace->hideSvg(false);
                }
            } else {
                //bitmap hatch
                newFace->setFillMode(QGIFace::BitmapFill);
            }

            // get the properties from the hatch viewprovider
            Gui::ViewProvider* gvp = QGIView::getViewProvider(fHatch);
            ViewProviderHatch* hatchVp = dynamic_cast<ViewProviderHatch*>(gvp);
            if (hatchVp) {
                if (hatchVp->HatchScale.getValue() > 0.0) {
                    newFace->setHatchScale(hatchVp->HatchScale.getValue());
                }
                newFace->setHatchColor(hatchVp->HatchColor.getValue());
                newFace->setHatchRotation(hatchVp->HatchRotation.getValue());
                newFace->setHatchOffset(hatchVp->HatchOffset.getValue());
            }
        }

        newFace->setDrawEdges(prefFaceEdges());
        newFace->setZValue(ZVALUE::FACE);
        newFace->setPrettyNormal();
        newFace->draw();
        iFace++;
    }
}

void QGIViewPart::drawAllEdges()
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));
    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));

    //    float lineWidthExtra = dvp->ExtraWidth.getValue() * lineScaleFactor;  //extra lines not used here

    const TechDraw::BaseGeomPtrVector& geoms = dvp->getEdgeGeometry();
    TechDraw::BaseGeomPtrVector::const_iterator itGeom = geoms.begin();
    QGIEdge* item;
    for (int iEdge = 0; itGeom != geoms.end(); itGeom++, iEdge++) {
        bool showItem = true;
        if (!showThisEdge(*itGeom)) {
            continue;
        }

        item = new QGIEdge(iEdge);
        addToGroup(item);      //item is created at scene(0, 0), not group(0, 0)
        item->setPath(drawPainterPath(*itGeom));

        item->setWidth(vp->LineWidth.getValue() * lineScaleFactor);     //thick
        item->setNormalColor(PreferencesGui::getAccessibleQColor(PreferencesGui::normalQColor()));
        item->setStyle(Qt::SolidLine);
        if ((*itGeom)->getCosmetic()) {
            // cosmetic edge - format appropriately
            int source = (*itGeom)->source();
            if (source == COSMETICEDGE) {
                std::string cTag = (*itGeom)->getCosmeticTag();
                showItem = formatGeomFromCosmetic(cTag, item);
            }
            else if (source == CENTERLINE) {
                std::string cTag = (*itGeom)->getCosmeticTag();
                showItem = formatGeomFromCenterLine(cTag, item);
            }
            else {
                Base::Console().Message("QGIVP::drawVP - cosmetic edge: %d is confused - source: %d\n",
                                        iEdge, source);
            }
        } else {
            // geometry edge - apply format if applicable
            TechDraw::GeomFormat* gf = dvp->getGeomFormatBySelection(iEdge);
            if (gf) {
                App::Color  color = Preferences::getAccessibleColor(gf->m_format.m_color);
                item->setNormalColor(color.asValue<QColor>());
                item->setWidth(gf->m_format.m_weight * lineScaleFactor);
                item->setStyle(gf->m_format.m_style);
                showItem = gf->m_format.m_visible;
            }
        }

        if (!(*itGeom)->getHlrVisible()) {
            // TODO: item->setISOLineNumber(getISOLineNumber(iEdge));
            item->setWidth(vp->HiddenWidth.getValue() * lineScaleFactor);   //thin
            item->setHiddenEdge(true);
            item->setZValue(ZVALUE::HIDEDGE);
        }

        if ((*itGeom)->getClassOfEdge()  == ecUVISO) {
            item->setWidth(vp->IsoWidth.getValue() * lineScaleFactor);   //graphic
        }

        item->setPos(0.0, 0.0);//now at group(0, 0)
        item->setZValue(ZVALUE::EDGE);
        item->setPrettyNormal();

        if (!vp->ShowAllEdges.getValue() && !showItem) {
             //view level "show" status  && individual edge "show" status
             item->hide();
        }

        //debug a path
        //            QPainterPath edgePath=drawPainterPath(*itGeom);
        //            std::stringstream edgeId;
        //            edgeId << "QGIVP.edgePath" << i;
        //            dumpPath(edgeId.str().c_str(), edgePath);
    }
}

void QGIViewPart::drawAllVertexes()
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));
    auto vp(static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject())));

    bool showVertices = true;
    bool showCenterMarks = true;
    if (getFrameState()) {
        //frames are on
        if (dvp->CoarseView.getValue()) {
            // don't show vertexes in CoarseView as there are too many
            showVertices = false;
        }
    } else {
        //frames are off
        showVertices = false;
    }

    if (!vp->ArcCenterMarks.getValue()) {
        showCenterMarks = false;
    }

    float lineWidth = vp->LineWidth.getValue() * lineScaleFactor;     //thick
    double vertexScaleFactor = Preferences::getPreferenceGroup("General")->GetFloat("VertexScale", 3.0);
    QColor vertexColor = PreferencesGui::getAccessibleQColor(PreferencesGui::vertexQColor());

    const std::vector<TechDraw::VertexPtr>& verts = dvp->getVertexGeometry();
    std::vector<TechDraw::VertexPtr>::const_iterator vert = verts.begin();
    for (int i = 0; vert != verts.end(); ++vert, i++) {
        if ((*vert)->isCenter()) {
            if (showCenterMarks) {
                QGICMark* cmItem = new QGICMark(i);
                addToGroup(cmItem);
                cmItem->setPos(Rez::guiX((*vert)->x()), Rez::guiX((*vert)->y()));
                cmItem->setThick(0.5 * lineWidth);//need minimum?
                cmItem->setSize(lineWidth * vertexScaleFactor * vp->CenterScale.getValue());
                cmItem->setPrettyNormal();
                cmItem->setZValue(ZVALUE::VERTEX);
            }
        } else {
            //regular Vertex
            if (showVertices) {
                QGIVertex* item = new QGIVertex(i);
                addToGroup(item);
                item->setPos(Rez::guiX((*vert)->x()), Rez::guiX((*vert)->y()));
                item->setNormalColor(vertexColor);
                item->setFillColor(vertexColor);
                item->setRadius(lineWidth * vertexScaleFactor);
                item->setPrettyNormal();
                item->setZValue(ZVALUE::VERTEX);
            }
        }
    }
}

bool QGIViewPart::showThisEdge(BaseGeomPtr geom)
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));

    if (geom->getHlrVisible()) {
        if ((geom->getClassOfEdge()  == ecHARD) || (geom->getClassOfEdge()  == ecOUTLINE)
            || ((geom->getClassOfEdge()  == ecSMOOTH) && dvp->SmoothVisible.getValue())
            || ((geom->getClassOfEdge()  == ecSEAM) && dvp->SeamVisible.getValue())
            || ((geom->getClassOfEdge()  == ecUVISO) && dvp->IsoVisible.getValue())) {
            return true;
        }
    } else {
        if (((geom->getClassOfEdge()  == ecHARD) && (dvp->HardHidden.getValue()))
            || ((geom->getClassOfEdge()  == ecOUTLINE) && (dvp->HardHidden.getValue()))
            || ((geom->getClassOfEdge()  == ecSMOOTH) && (dvp->SmoothHidden.getValue()))
            || ((geom->getClassOfEdge()  == ecSEAM) && (dvp->SeamHidden.getValue()))
            || ((geom->getClassOfEdge()  == ecUVISO) && (dvp->IsoHidden.getValue()))) {
            return true;
        }
    }

    return false;
}

bool QGIViewPart::formatGeomFromCosmetic(std::string cTag, QGIEdge* item)
{
    //    Base::Console().Message("QGIVP::formatGeomFromCosmetic(%s)\n", cTag.c_str());
    bool result = true;
    auto partFeat(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    TechDraw::CosmeticEdge* ce = partFeat ? partFeat->getCosmeticEdge(cTag) : nullptr;
    if (ce) {
        App::Color color = Preferences::getAccessibleColor(ce->m_format.m_color);
        item->setNormalColor(color.asValue<QColor>());
        item->setWidth(ce->m_format.m_weight * lineScaleFactor);
        item->setStyle(ce->m_format.m_style);
        result = ce->m_format.m_visible;
    }
    return result;
}


bool QGIViewPart::formatGeomFromCenterLine(std::string cTag, QGIEdge* item)
{
    //    Base::Console().Message("QGIVP::formatGeomFromCenterLine(%d)\n", sourceIndex);
    bool result = true;
    auto partFeat(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    TechDraw::CenterLine* cl = partFeat ? partFeat->getCenterLine(cTag) : nullptr;
    if (cl) {
        App::Color color = Preferences::getAccessibleColor(cl->m_format.m_color);
        item->setNormalColor(color.asValue<QColor>());
        item->setWidth(cl->m_format.m_weight * lineScaleFactor);
        item->setStyle(cl->m_format.m_style);
        result = cl->m_format.m_visible;
    }
    return result;
}

QGIFace* QGIViewPart::drawFace(TechDraw::FacePtr f, int idx)
{
    //    Base::Console().Message("QGIVP::drawFace - %d\n", idx);
    std::vector<TechDraw::Wire*> fWires = f->wires;
    QPainterPath facePath;
    for (std::vector<TechDraw::Wire*>::iterator wire = fWires.begin(); wire != fWires.end();
         ++wire) {
        TechDraw::BaseGeomPtrVector geoms = (*wire)->geoms;
        if (geoms.empty())
            continue;

        TechDraw::BaseGeomPtr firstGeom = geoms.front();
        QPainterPath wirePath;
        //QPointF startPoint(firstGeom->getStartPoint().x, firstGeom->getStartPoint().y);
        //wirePath.moveTo(startPoint);
        QPainterPath firstSeg = drawPainterPath(firstGeom);
        wirePath.connectPath(firstSeg);
        for (TechDraw::BaseGeomPtrVector::iterator edge = ((*wire)->geoms.begin()) + 1;
             edge != (*wire)->geoms.end(); ++edge) {
            QPainterPath edgePath = drawPainterPath(*edge);
            //handle section faces differently
            if (idx == -1) {
                QPointF wEnd = wirePath.currentPosition();
                auto element = edgePath.elementAt(0);
                QPointF eStart(element.x, element.y);
                QPointF eEnd = edgePath.currentPosition();
                QPointF sVec = wEnd - eStart;
                QPointF eVec = wEnd - eEnd;
                double sDist2 = sVec.x() * sVec.x() + sVec.y() * sVec.y();
                double eDist2 = eVec.x() * eVec.x() + eVec.y() * eVec.y();
                if (sDist2 > eDist2) {
                    edgePath = edgePath.toReversed();
                }
            }
            wirePath.connectPath(edgePath);
        }
        //        dumpPath("wirePath:", wirePath);
        facePath.addPath(wirePath);
    }
    facePath.setFillRule(Qt::OddEvenFill);

    QGIFace* gFace = new QGIFace(idx);
    addToGroup(gFace);
    gFace->setPos(0.0, 0.0);
    gFace->setOutline(facePath);
    //debug a path
    //std::stringstream faceId;
    //faceId << "facePath " << idx;
    //dumpPath(faceId.str().c_str(), facePath);

    return gFace;
}

//! Remove all existing QGIPrimPath items(Vertex, Edge, Face)
//note this triggers scene selectionChanged signal if vertex/edge/face is selected
void QGIViewPart::removePrimitives()
{
    QList<QGraphicsItem*> children = childItems();
    MDIViewPage* mdi = getMDIViewPage();
    if (mdi) {
        getMDIViewPage()->blockSceneSelection(true);
    }
    for (auto& c : children) {
        QGIPrimPath* prim = dynamic_cast<QGIPrimPath*>(c);
        if (prim) {
            prim->hide();
            scene()->removeItem(prim);
            delete prim;
        }
    }
    if (mdi) {
        getMDIViewPage()->blockSceneSelection(false);
    }
}

//! Remove all existing QGIDecoration items(SectionLine, SectionMark, ...)
void QGIViewPart::removeDecorations()
{
    QList<QGraphicsItem*> children = childItems();
    for (auto& c : children) {
        QGIDecoration* decor = dynamic_cast<QGIDecoration*>(c);
        QGIMatting* mat = dynamic_cast<QGIMatting*>(c);
        if (decor) {
            decor->hide();
            scene()->removeItem(decor);
            delete decor;
        }
        else if (mat) {
            mat->hide();
            scene()->removeItem(mat);
            delete mat;
        }
    }
}

void QGIViewPart::drawAllSectionLines()
{
    TechDraw::DrawViewPart* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart)
        return;

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp)
        return;
    if (vp->ShowSectionLine.getValue()) {
        auto refs = viewPart->getSectionRefs();
        for (auto& r : refs) {
            if (r->isDerivedFrom(DrawComplexSection::getClassTypeId())) {
                drawComplexSectionLine(r, true);
            }
            else {
                drawSectionLine(r, true);
            }
        }
    }
}

void QGIViewPart::drawSectionLine(TechDraw::DrawViewSection* viewSection, bool b)
{
    TechDraw::DrawViewPart* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart)
        return;
    if (!viewSection)
        return;

    if (!viewSection->hasGeometry())
        return;

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    float lineWidthThin = vp->HiddenWidth.getValue() * lineScaleFactor;//thin

    if (b) {
        QGISectionLine* sectionLine = new QGISectionLine();
        addToGroup(sectionLine);
        sectionLine->setSymbol(const_cast<char*>(viewSection->SectionSymbol.getValue()));
        sectionLine->setSectionStyle(vp->SectionLineStyle.getValue());
        App::Color color = Preferences::getAccessibleColor(vp->SectionLineColor.getValue());
        sectionLine->setSectionColor(color.asValue<QColor>());
        sectionLine->setPathMode(false);

        //find the ends of the section line
        double scale = viewPart->getScale();
        std::pair<Base::Vector3d, Base::Vector3d> sLineEnds = viewSection->sectionLineEnds();
        Base::Vector3d l1 = Rez::guiX(sLineEnds.first) * scale;
        Base::Vector3d l2 = Rez::guiX(sLineEnds.second) * scale;
        //make the section line a little longer
        double fudge = 2.0 * Preferences::dimFontSizeMM();
        Base::Vector3d lineDir = l2 - l1;
        lineDir.Normalize();
        sectionLine->setEnds(l1 - lineDir * Rez::guiX(fudge), l2 + lineDir * Rez::guiX(fudge));

        //which way do the arrows point?
        Base::Vector3d arrowDir = viewSection->SectionNormal.getValue();
        arrowDir = -viewPart->projectPoint(arrowDir);      //arrows point reverse of sectionNormal
        sectionLine->setDirection(arrowDir.x, -arrowDir.y);//3d direction needs Y inversion

        if (vp->SectionLineMarks.getValue()) {
            ChangePointVector points = viewSection->getChangePointsFromSectionLine();
            //extend the changePoint locations to match the fudged section line ends
            QPointF location0 = points.front().getLocation() * scale;
            location0 = location0 - DU::toQPointF(lineDir) * fudge;
            QPointF location1 = points.back().getLocation() * scale;
            location1 = location1 + DU::toQPointF(lineDir) * fudge;
            //change points have Rez::guiX applied in sectionLine
            points.front().setLocation(location0);
            points.back().setLocation(location1);
            sectionLine->setChangePoints(points);
        }
        else {
            sectionLine->clearChangePoints();
        }

        //set the general parameters
        sectionLine->setPos(0.0, 0.0);
        sectionLine->setWidth(lineWidthThin);
        double fontSize = Preferences::dimFontSizeMM();
        sectionLine->setFont(getFont(), fontSize);
        sectionLine->setZValue(ZVALUE::SECTIONLINE);
        sectionLine->setRotation(-viewPart->Rotation.getValue());
        sectionLine->draw();
    }
}

void QGIViewPart::drawComplexSectionLine(TechDraw::DrawViewSection* viewSection, bool b)
{
    Q_UNUSED(b);
    TechDraw::DrawViewPart* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart)
        return;
    if (!viewSection)
        return;
    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    float lineWidthThin = vp->HiddenWidth.getValue() * lineScaleFactor;//thin

    auto dcs = static_cast<DrawComplexSection*>(viewSection);
    BaseGeomPtrVector edges = dcs->makeSectionLineGeometry();
    QPainterPath wirePath;
    QPainterPath firstSeg = drawPainterPath(edges.front());
    wirePath.connectPath(firstSeg);
    int edgeCount = edges.size();
    //NOTE: if the edges are not in nose to tail order, Qt will insert extra segments
    //that will overlap the segments we add. for interrupted line styles, this
    //will make the line look continuous.  This is prevented in
    //DrawComplexSection::makeSectionLineGeometry by calling makeNoseToTailWire
    for (int i = 1; i < edgeCount; i++) {
        QPainterPath edgePath = drawPainterPath(edges.at(i));
        wirePath.connectPath(edgePath);
    }

    std::pair<Base::Vector3d, Base::Vector3d> ends = dcs->sectionLineEnds();
    Base::Vector3d vStart = Rez::guiX(ends.first);//already scaled by dcs
    Base::Vector3d vEnd = Rez::guiX(ends.second);

    QGISectionLine* sectionLine = new QGISectionLine();
    addToGroup(sectionLine);
    sectionLine->setSymbol(const_cast<char*>(viewSection->SectionSymbol.getValue()));
    sectionLine->setSectionStyle(vp->SectionLineStyle.getValue());
    App::Color color = Preferences::getAccessibleColor(vp->SectionLineColor.getValue());
    sectionLine->setSectionColor(color.asValue<QColor>());
    sectionLine->setPathMode(true);
    sectionLine->setPath(wirePath);
    sectionLine->setEnds(vStart, vEnd);
    if (vp->SectionLineMarks.getValue()) {
        sectionLine->setChangePoints(dcs->getChangePointsFromSectionLine());
    }
    else {
        sectionLine->clearChangePoints();
    }
    if (dcs->ProjectionStrategy.isValue("Offset")) {
        Base::Vector3d arrowDirOffset = viewSection->SectionNormal.getValue();
        arrowDirOffset =
            -viewPart->projectPoint(arrowDirOffset);//arrows are opposite section normal
        sectionLine->setDirection(arrowDirOffset.x, -arrowDirOffset.y);//invert y for Qt
    }
    else {
        std::pair<Base::Vector3d, Base::Vector3d> dirsAligned = dcs->sectionArrowDirs();
        dirsAligned.first = DrawUtil::invertY(dirsAligned.first);
        dirsAligned.second = DrawUtil::invertY(dirsAligned.second);
        sectionLine->setArrowDirections(dirsAligned.first, dirsAligned.second);
    }

    //set the general parameters
    sectionLine->setPos(0.0, 0.0);
    sectionLine->setWidth(lineWidthThin);
    double fontSize = Preferences::dimFontSizeMM();
    sectionLine->setFont(getFont(), fontSize);
    sectionLine->setZValue(ZVALUE::SECTIONLINE);
    sectionLine->setRotation(-viewPart->Rotation.getValue());
    sectionLine->draw();
}

//TODO: use Cosmetic::CenterLine object for this to make it usable for dims.
void QGIViewPart::drawCenterLines(bool b)
{
    TechDraw::DrawViewPart* viewPart = dynamic_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart)
        return;

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp)
        return;

    if (b) {
        bool horiz = vp->HorizCenterLine.getValue();
        bool vert = vp->VertCenterLine.getValue();

        QGICenterLine* centerLine;
        double sectionSpan;
        double sectionFudge = Rez::guiX(10.0);
        double xVal, yVal;
        if (horiz) {
            centerLine = new QGICenterLine();
            addToGroup(centerLine);
            centerLine->setPos(0.0, 0.0);
            double width = Rez::guiX(viewPart->getBoxX());
            sectionSpan = width + sectionFudge;
            xVal = sectionSpan / 2.0;
            yVal = 0.0;
            centerLine->setIntersection(horiz && vert);
            centerLine->setBounds(-xVal, -yVal, xVal, yVal);
            centerLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
            centerLine->setZValue(ZVALUE::SECTIONLINE);
            centerLine->draw();
        }
        if (vert) {
            centerLine = new QGICenterLine();
            addToGroup(centerLine);
            centerLine->setPos(0.0, 0.0);
            double height = Rez::guiX(viewPart->getBoxY());
            sectionSpan = height + sectionFudge;
            xVal = 0.0;
            yVal = sectionSpan / 2.0;
            centerLine->setIntersection(horiz && vert);
            centerLine->setBounds(-xVal, -yVal, xVal, yVal);
            centerLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
            centerLine->setZValue(ZVALUE::SECTIONLINE);
            centerLine->draw();
        }
    }
}

void QGIViewPart::drawAllHighlights()
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));

    auto drefs = dvp->getDetailRefs();
    for (auto& r : drefs) {
        drawHighlight(r, true);
    }
}

void QGIViewPart::drawHighlight(TechDraw::DrawViewDetail* viewDetail, bool b)
{
    TechDraw::DrawViewPart* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
    if (!viewPart || !viewDetail) {
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }
    auto vpDetail = static_cast<ViewProviderViewPart*>(getViewProvider(viewDetail));
    if (!vpDetail) {
        return;
    }
    if (b) {
        //        double fontSize = getPrefFontSize();
        double fontSize = Preferences::labelFontSizeMM();
        QGIHighlight* highlight = new QGIHighlight();
        scene()->addItem(highlight);
        highlight->setReference(viewDetail->Reference.getValue());
        highlight->setStyle((Qt::PenStyle)vp->HighlightLineStyle.getValue());
        App::Color color = Preferences::getAccessibleColor(vp->HighlightLineColor.getValue());
        highlight->setColor(color.asValue<QColor>());
        highlight->setFeatureName(viewDetail->getNameInDocument());
        highlight->setInteractive(true);

        addToGroup(highlight);
        highlight->setPos(0.0, 0.0);//sb setPos(center.x, center.y)?

        Base::Vector3d center = viewDetail->AnchorPoint.getValue() * viewPart->getScale();
        double rotationRad = viewPart->Rotation.getValue() * M_PI / 180.0;
        center.RotateZ(rotationRad);

        double radius = viewDetail->Radius.getValue() * viewPart->getScale();
        highlight->setBounds(center.x - radius, center.y + radius, center.x + radius,
                             center.y - radius);
        highlight->setWidth(Rez::guiX(vp->IsoWidth.getValue()));
        highlight->setFont(getFont(), fontSize);
        highlight->setZValue(ZVALUE::HIGHLIGHT);
        highlight->setReferenceAngle(vpDetail->HighlightAdjust.getValue());

        //handle conversion of apparent X,Y to rotated
        QPointF rotCenter = highlight->mapFromParent(transformOriginPoint());
        highlight->setTransformOriginPoint(rotCenter);

        double rotation = viewPart->Rotation.getValue();
        highlight->setRotation(rotation);
        highlight->draw();
    }
}

void QGIViewPart::highlightMoved(QGIHighlight* highlight, QPointF newPos)
{
    std::string highlightName = highlight->getFeatureName();
    App::Document* doc = getViewObject()->getDocument();
    App::DocumentObject* docObj = doc->getObject(highlightName.c_str());
    auto detail = dynamic_cast<DrawViewDetail*>(docObj);
    auto oldAnchor = detail->AnchorPoint.getValue();
    if (detail) {
        Base::Vector3d delta = Rez::appX(DrawUtil::toVector3d(newPos)) / getViewObject()->getScale();
        delta = DrawUtil::invertY(delta);
        detail->AnchorPoint.setValue(oldAnchor + delta);
    }
}

void QGIViewPart::drawMatting()
{
    auto viewPart(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    TechDraw::DrawViewDetail* dvd = nullptr;
    if (viewPart && viewPart->isDerivedFrom(TechDraw::DrawViewDetail::getClassTypeId())) {
        dvd = static_cast<TechDraw::DrawViewDetail*>(viewPart);
    }
    else {
        return;
    }

    double scale = dvd->getScale();
    double radius = dvd->Radius.getValue() * scale;
    QGIMatting* mat = new QGIMatting();
    addToGroup(mat);
    mat->setRadius(Rez::guiX(radius));
    mat->setPos(0.0, 0.0);
    mat->draw();
    mat->show();
}

void QGIViewPart::toggleCache(bool state)
{
    QList<QGraphicsItem*> items = childItems();
    for (QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
        //(*it)->setCacheMode((state)? DeviceCoordinateCache : NoCache);        //TODO: fiddle cache settings if req'd for performance
        Q_UNUSED(state);
        (*it)->setCacheMode(NoCache);
        (*it)->update();
    }
}

void QGIViewPart::toggleCosmeticLines(bool state)
{
    QList<QGraphicsItem*> items = childItems();
    for (QList<QGraphicsItem*>::iterator it = items.begin(); it != items.end(); it++) {
        QGIEdge* edge = dynamic_cast<QGIEdge*>(*it);
        if (edge) {
            edge->setCosmetic(state);
        }
    }
}

//get hatchObj for face i if it exists
TechDraw::DrawHatch* QGIViewPart::faceIsHatched(int i,
                                                std::vector<TechDraw::DrawHatch*> hatchObjs) const
{
    TechDraw::DrawHatch* result = nullptr;
    bool found = false;
    for (auto& h : hatchObjs) {
        const std::vector<std::string>& sourceNames = h->Source.getSubValues();
        for (auto& s : sourceNames) {
            int fdx = TechDraw::DrawUtil::getIndexFromName(s);
            if (fdx == i) {
                result = h;
                found = true;
                break;
            }
        }
        if (found) {
            break;
        }
    }
    return result;
}

TechDraw::DrawGeomHatch*
QGIViewPart::faceIsGeomHatched(int i, std::vector<TechDraw::DrawGeomHatch*> geomObjs) const
{
    TechDraw::DrawGeomHatch* result = nullptr;
    bool found = false;
    for (auto& h : geomObjs) {
        const std::vector<std::string>& sourceNames = h->Source.getSubValues();
        for (auto& sn : sourceNames) {
            int fdx = TechDraw::DrawUtil::getIndexFromName(sn);
            if (fdx == i) {
                result = h;
                found = true;
                break;
            }
            if (found) {
                break;
            }
        }
    }
    return result;
}


void QGIViewPart::dumpPath(const char* text, QPainterPath path)
{
    QPainterPath::Element elem;
    Base::Console().Message(">>>%s has %d elements\n", text, path.elementCount());
    char* typeName;
    for (int iElem = 0; iElem < path.elementCount(); iElem++) {
        elem = path.elementAt(iElem);
        if (elem.isMoveTo()) {
            typeName = "MoveTo";
        }
        else if (elem.isLineTo()) {
            typeName = "LineTo";
        }
        else if (elem.isCurveTo()) {
            typeName = "CurveTo";
        }
        else {
            typeName = "CurveData";
        }
        Base::Console().Message(">>>>> element %d: type:%d/%s pos(%.3f, %.3f) M:%d L:%d C:%d\n",
                                iElem, static_cast<int>(elem.type), typeName, elem.x, elem.y, static_cast<int>(elem.isMoveTo()),
                                static_cast<int>(elem.isLineTo()), static_cast<int>(elem.isCurveTo()));
    }
}

QRectF QGIViewPart::boundingRect() const
{
    //    return childrenBoundingRect();
    //    return customChildrenBoundingRect();
    return QGIView::boundingRect();
}
void QGIViewPart::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    QStyleOptionGraphicsItem myOption(*option);
    myOption.state &= ~QStyle::State_Selected;

    //    painter->drawRect(boundingRect());          //good for debugging

    QGIView::paint(painter, &myOption, widget);
}

//QGIViewPart derived classes do not need a rotate view method as rotation is handled on App side.
void QGIViewPart::rotateView() {}

bool QGIViewPart::prefFaceEdges()
{
    bool result = false;
    result = Preferences::getPreferenceGroup("General")->GetBool("DrawFaceEdges", false);
    return result;
}

bool QGIViewPart::prefPrintCenters()
{
    bool printCenters = Preferences::getPreferenceGroup("Decorations")->GetBool("PrintCenterMarks", false);//true matches v0.18 behaviour
    return printCenters;
}
