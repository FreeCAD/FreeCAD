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
#include <cstdio>
#ifndef _PreComp_

#include <QPainterPath>
#include <QKeyEvent>
#include <qmath.h>
#endif// #ifndef _PreComp_

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Base/Vector3D.h>
#include <Gui/Selection/Selection.h>
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
#include <Mod/TechDraw/App/DrawBrokenView.h>

#include "DrawGuiUtil.h"
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
#include "QGIBreakLine.h"

using namespace TechDraw;
using namespace TechDrawGui;
using namespace std;
using DU = DrawUtil;
using FillMode = QGIFace::FillMode;

const float lineScaleFactor = Rez::guiX(1.);// temp fiddle for devel

QGIViewPart::QGIViewPart()
{
    setCacheMode(QGraphicsItem::NoCache);
    setHandlesChildEvents(false);
    setAcceptHoverEvents(true);
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setFlag(QGraphicsItem::ItemIsFocusable, true);

    showSection = false;
    m_pathBuilder = new PathBuilder(this);
    m_dashedLineGenerator = new LineGenerator();
}

QGIViewPart::~QGIViewPart()
{
    tidy();
    delete m_pathBuilder;
    delete m_dashedLineGenerator;
}

QVariant QGIViewPart::itemChange(GraphicsItemChange change, const QVariant& value)
{
    if (change == ItemSelectedHasChanged && scene()) {
        //There's nothing special for QGIVP to do when selection changes!
    }
    else if (change == ItemSceneChange && scene()) {
        QObject::disconnect(m_selectionChangedConnection);
        tidy();
    }
    else if (change == QGraphicsItem::ItemSceneHasChanged) {
        if (scene()) {
            m_selectionChangedConnection = connect(scene(), &QGraphicsScene::selectionChanged, this, [this]() {
                // When selection changes, if the mouse is not over the view,
                // hide any non-selected vertices.
                if (!isUnderMouse()) {
                    for (QGraphicsItem* item : m_vertexItems) {
                        if (item && !item->isSelected()) {
                            item->setVisible(false);
                        }
                    }
                    update();
                }
            });
        }
    }

    return QGIView::itemChange(change, value);
}

bool QGIViewPart::sceneEventFilter(QGraphicsItem *watched, QEvent *event)
{
    // Base::Console().message("QGIVP::sceneEventFilter - event: %d watchedtype: %d\n",
    //                         event->type(), watched->type() - QGraphicsItem::UserType);
    if (event->type() == QEvent::ShortcutOverride) {
        // if we accept this event, we should get a regular keystroke event next
        // which will be processed by QGVPage/QGVNavStyle keypress logic, but not forwarded to
        // Std_Delete
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->matches(QKeySequence::Delete))  {
            bool success = removeSelectedCosmetic();
            if (success) {
                updateView(true);
                event->accept();
                return true;
            }
        }
    }

    return QGraphicsItem::sceneEventFilter(watched, event);
}

//! called when a DEL shortcut event is received.  If a cosmetic edge or vertex is
//! selected, remove it from the view.
bool QGIViewPart::removeSelectedCosmetic() const
{
    // Base::Console().message("QGIVP::removeSelectedCosmetic()\n");
    auto dvp(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    if (!dvp) {
        throw Base::RuntimeError("Graphic has no feature!");
    }
    char* defaultDocument{nullptr};
    std::vector<Gui::SelectionObject> selectionAll = Gui::Selection().getSelectionEx(
        defaultDocument, TechDraw::DrawViewPart::getClassTypeId(), Gui::ResolveMode::OldStyleElement);
    if (selectionAll.empty()) {
        return false;
    }
    std::vector<std::string> subElements = selectionAll.front().getSubNames();
    if (subElements.empty()) {
        return false;
    }

    dvp->deleteCosmeticElements(subElements);
    dvp->refreshCEGeoms();
    dvp->refreshCLGeoms();
    dvp->refreshCVGeoms();

    return true;
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
    // Base::Console().message("QGIVP::updateView() - %s\n", getViewObject()->getNameInDocument());
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
    auto viewPart(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    if (!viewPart) {
        return;
    }

    auto doc = viewPart->getDocument();
    if (!doc || doc->testStatus(App::Document::Status::Restoring)) {
        // if the document is still restoring, we may not have all the information
        // we need to draw the source objects, so we wait until restore is finished.
        // Base::Console().message("QGIVP::draw - document is restoring, do not draw\n");
        return;
    }

    if (!isVisible())
        return;

    drawViewPart();
    drawAllHighlights();
    drawBreakLines();
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
    //    Base::Console().message("QGIVP::DVP() - %s / %s\n", viewPart->getNameInDocument(), viewPart->Label.getValue());
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

    QColor faceColor;
    auto vpp = dynamic_cast<ViewProviderViewPart *>(getViewProvider(getViewObject()));
    if (vpp) {
        faceColor = vpp->FaceColor.getValue().asValue<QColor>();
        faceColor.setAlpha((100 - vpp->FaceTransparency.getValue())*255/100);
    }

    std::vector<TechDraw::DrawHatch*> regularHatches = dvp->getHatches();
    std::vector<TechDraw::DrawGeomHatch*> geomHatches = dvp->getGeomHatches();
    const std::vector<TechDraw::FacePtr>& faceGeoms = dvp->getFaceGeometry();
    int iFace(0);
    for (auto& face : faceGeoms) {
        QGIFace* newFace = drawFace(face, iFace);
        if (faceColor.isValid()) {
            newFace->setFillColor(faceColor);
            newFace->setFillMode(faceColor.alpha() ? FillMode::PlainFill : FillMode::NoFill);
        }

        TechDraw::DrawHatch* fHatch = faceIsHatched(iFace, regularHatches);
        TechDraw::DrawGeomHatch* fGeom = faceIsGeomHatched(iFace, geomHatches);
        if (fGeom) {
            // geometric hatch (from PAT hatch specification)
            newFace->isHatched(true);
            newFace->setFillMode(FillMode::GeomHatchFill);
            std::vector<LineSet> lineSets = fGeom->getTrimmedLines(iFace);
            if (!lineSets.empty()) {
                // this face has geometric hatch lines
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
            ViewProviderGeomHatch* geomVp = freecad_cast<ViewProviderGeomHatch*>(gvp);
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
                newFace->setFillMode(FillMode::SvgFill);
            } else {
                //bitmap hatch
                newFace->setFillMode(FillMode::BitmapFill);
            }

            // get the properties from the hatch viewprovider
            Gui::ViewProvider* gvp = QGIView::getViewProvider(fHatch);
            ViewProviderHatch* hatchVp = freecad_cast<ViewProviderHatch*>(gvp);
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
        item->setSource((*itGeom)->source());

        item->setNormalColor(PreferencesGui::getAccessibleQColor(PreferencesGui::normalQColor()));
        if ((*itGeom)->getCosmetic()) {
            // cosmetic edge - format appropriately
            TechDraw::SourceType source = (*itGeom)->source();
            if (source == TechDraw::SourceType::COSMETICEDGE) {
                std::string cTag = (*itGeom)->getCosmeticTag();
                showItem = formatGeomFromCosmetic(cTag, item);
            }
            else if (source == TechDraw::SourceType::CENTERLINE) {
                std::string cTag = (*itGeom)->getCosmeticTag();
                showItem = formatGeomFromCenterLine(cTag, item);
            }
            else {
                Base::Console().message("QGIVP::drawVP - cosmetic edge: %d is confused - source: %d\n",
                                        iEdge, static_cast<int>(source));
            }
        } else {
            // geometry edge - apply format if applicable
            TechDraw::GeomFormat* gf = dvp->getGeomFormatBySelection(iEdge);
            if (gf) {
                Base::Color  color = Preferences::getAccessibleColor(gf->m_format.getColor());
                item->setNormalColor(color.asValue<QColor>());
                int lineNumber = gf->m_format.getLineNumber();
                int qtStyle = gf->m_format.getStyle();
                item->setLinePen(m_dashedLineGenerator->getBestPen(lineNumber, (Qt::PenStyle)qtStyle,
                                                     gf->m_format.getWidth()));
                // but we need to actually draw the lines in QGScene coords (0.1 mm).
                item->setWidth(Rez::guiX(gf->m_format.getWidth()));
                showItem = gf->m_format.getVisible();
            } else {
                if (!(*itGeom)->getHlrVisible()) {
                    // hidden line without a format
                    item->setLinePen(m_dashedLineGenerator->getLinePen(Preferences::HiddenLineStyle(),
                                                                       vp->LineWidth.getValue()));
                    item->setHiddenEdge(true);
                    item->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));   //thin
                    item->setZValue(ZVALUE::HIDEDGE);
                } else {
                    // unformatted visible line, draw as continuous line
                    item->setLinePen(m_dashedLineGenerator->getLinePen(1, vp->LineWidth.getValue()));
                    item->setWidth(Rez::guiX(vp->LineWidth.getValue()));
                }
            }
        }

        if ((*itGeom)->getClassOfEdge() == EdgeClass::UVISO) {
            // we don't have a style option for iso-parametric lines so draw continuous
            item->setLinePen(m_dashedLineGenerator->getLinePen(1, vp->IsoWidth.getValue()));
            item->setWidth(Rez::guiX(vp->IsoWidth.getValue()));   //graphic
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
    m_vertexItems.clear();
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));
    auto vp(static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject())));

    QColor vertexColor = PreferencesGui::getAccessibleQColor(PreferencesGui::vertexQColor());

    const std::vector<TechDraw::VertexPtr>& verts = dvp->getVertexGeometry();
    std::vector<TechDraw::VertexPtr>::const_iterator vert = verts.begin();
    for (int i = 0; vert != verts.end(); ++vert, i++) {
        if ((*vert)->isCenter()) {
            if (showCenterMarks()) {
                QGICMark* cmItem = new QGICMark(i);
                addToGroup(cmItem);
                m_vertexItems.append(cmItem);
                cmItem->setPos(Rez::guiX((*vert)->x()), Rez::guiX((*vert)->y()));
                cmItem->setThick(0.5F * getLineWidth());//need minimum?
                cmItem->setSize(getVertexSize() * vp->CenterScale.getValue());
                cmItem->setPrettyNormal();
                cmItem->setZValue(ZVALUE::VERTEX);
                cmItem->setVisible(false);
            }
        } else {
            //regular Vertex
            if (showVertices()) {
                QGIVertex* item = new QGIVertex(i);
                addToGroup(item);
                m_vertexItems.append(item);
                item->setPos(Rez::guiX((*vert)->x()), Rez::guiX((*vert)->y()));
                item->setNormalColor(vertexColor);
                item->setFillColor(vertexColor);
                item->setRadius(getVertexSize());
                item->setPrettyNormal();
                item->setZValue(ZVALUE::VERTEX);
                item->setVisible(false);
            }
        }
    }
}

bool QGIViewPart::showThisEdge(BaseGeomPtr geom)
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));

    if (geom->getHlrVisible()) {
        if ((geom->getClassOfEdge() == EdgeClass::HARD) || (geom->getClassOfEdge() == EdgeClass::OUTLINE)
            || ((geom->getClassOfEdge() == EdgeClass::SMOOTH) && dvp->SmoothVisible.getValue())
            || ((geom->getClassOfEdge() == EdgeClass::SEAM) && dvp->SeamVisible.getValue())
            || ((geom->getClassOfEdge() == EdgeClass::UVISO) && dvp->IsoVisible.getValue())) {
            return true;
        }
    } else {
        if (((geom->getClassOfEdge() == EdgeClass::HARD) && (dvp->HardHidden.getValue()))
            || ((geom->getClassOfEdge() == EdgeClass::OUTLINE) && (dvp->HardHidden.getValue()))
            || ((geom->getClassOfEdge() == EdgeClass::SMOOTH) && (dvp->SmoothHidden.getValue()))
            || ((geom->getClassOfEdge() == EdgeClass::SEAM) && (dvp->SeamHidden.getValue()))
            || ((geom->getClassOfEdge() == EdgeClass::UVISO) && (dvp->IsoHidden.getValue()))) {
            return true;
        }
    }

    return false;
}

// returns true if vertex dots should be shown
bool QGIViewPart::showVertices()
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));

    if (dvp->CoarseView.getValue()) {
        // never show vertices in CoarseView
        return false;
    }
    return true;
}


// returns true if arc center marks should be shown
bool QGIViewPart::showCenterMarks()
{
    // dvp and vp already validated
    auto dvp(static_cast<TechDraw::DrawViewPart*>(getViewObject()));
    auto vp(static_cast<ViewProviderViewPart*>(getViewProvider(dvp)));

    if (!vp->ArcCenterMarks.getValue()) {
        // no center marks if view property is false
        return false;
    }
    if (prefPrintCenters()) {
        // frames are off, view property is true and Print Center Marks is true
        return true;
    }

    return true;
}


bool QGIViewPart::formatGeomFromCosmetic(std::string cTag, QGIEdge* item)
{
    //    Base::Console().message("QGIVP::formatGeomFromCosmetic(%s)\n", cTag.c_str());
    bool result = true;
    auto partFeat(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    TechDraw::CosmeticEdge* ce = partFeat ? partFeat->getCosmeticEdge(cTag) : nullptr;
    if (ce) {
        Base::Color color = Preferences::getAccessibleColor(ce->m_format.getColor());
        item->setNormalColor(color.asValue<QColor>());
        item->setLinePen(m_dashedLineGenerator->getBestPen(ce->m_format.getLineNumber(),
                                                     (Qt::PenStyle)ce->m_format.getStyle(),
                                                     ce->m_format.getWidth()));
        item->setWidth(Rez::guiX(ce->m_format.getWidth()));
        result = ce->m_format.getVisible();
    }
    return result;
}


bool QGIViewPart::formatGeomFromCenterLine(std::string cTag, QGIEdge* item)
{
//    Base::Console().message("QGIVP::formatGeomFromCenterLine()\n");
    bool result = true;
    auto partFeat(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    TechDraw::CenterLine* cl = partFeat ? partFeat->getCenterLine(cTag) : nullptr;
    if (cl) {
        Base::Color color = Preferences::getAccessibleColor(cl->m_format.getColor());
        item->setNormalColor(color.asValue<QColor>());
        item->setLinePen(m_dashedLineGenerator->getBestPen(cl->m_format.getLineNumber(),
                                                     (Qt::PenStyle)cl->m_format.getStyle(),
                                                     cl->m_format.getWidth()));
        item->setWidth(Rez::guiX(cl->m_format.getWidth()));
        result = cl->m_format.getVisible();
    }
    return result;
}

QGIFace* QGIViewPart::drawFace(TechDraw::FacePtr f, int idx)
{
    //    Base::Console().message("QGIVP::drawFace - %d\n", idx);
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
    if (!vp) {
        return;
    }

    if (vp->ShowSectionLine.getValue()) {
        auto refs = viewPart->getSectionRefs();
        for (auto& r : refs) {
            if (r->isDerivedFrom<DrawComplexSection>()) {
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
//    Base::Console().message("QGIVP::drawSectionLine()\n");
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

    if (b) {
        //find the ends of the section line
        double scale = viewPart->getScale();
        std::pair<Base::Vector3d, Base::Vector3d> sLineEnds = viewSection->sectionLineEnds();
        Base::Vector3d l1 = Rez::guiX(sLineEnds.first) * scale;
        Base::Vector3d l2 = Rez::guiX(sLineEnds.second) * scale;
        if (l1.IsEqual(l2, EWTOLERANCE) ) {
            Base::Console().message("QGIVP::drawSectionLine - line endpoints are equal. No section line created.\n");
            return;
        }

        QGISectionLine* sectionLine = new QGISectionLine();
        addToGroup(sectionLine);
        sectionLine->setSymbol(const_cast<char*>(viewSection->SectionSymbol.getValue()));
        Base::Color color = Preferences::getAccessibleColor(vp->SectionLineColor.getValue());
        sectionLine->setSectionColor(color.asValue<QColor>());
        sectionLine->setPathMode(false);

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

        if (vp->IncludeCutLine.getValue()) {
            sectionLine->setShowLine(true);
            // sectionLines are typically ISO 8 (long dash, short dash) or ISO 4 (long dash, dot)
            sectionLine->setLinePen(
                    m_dashedLineGenerator->getLinePen((size_t)vp->SectionLineStyle.getValue(),
                                                        vp->HiddenWidth.getValue()));
            sectionLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
        } else {
            sectionLine->setShowLine(false);
        }

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

    auto dcs = static_cast<DrawComplexSection*>(viewSection);
    std::pair<Base::Vector3d, Base::Vector3d> ends = dcs->sectionLineEnds();
    Base::Vector3d vStart = Rez::guiX(ends.first);//already scaled by dcs
    Base::Vector3d vEnd = Rez::guiX(ends.second);
    if (vStart.IsEqual(vEnd, EWTOLERANCE) ) {
        Base::Console().message("QGIVP::drawComplexSectionLine - line endpoints are equal. No section line created.\n");
        return;
    }


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


    QGISectionLine* sectionLine = new QGISectionLine();
    addToGroup(sectionLine);
    sectionLine->setSymbol(const_cast<char*>(viewSection->SectionSymbol.getValue()));
    Base::Color color = Preferences::getAccessibleColor(vp->SectionLineColor.getValue());
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
        sectionLine->setArrowDirections(dirsAligned.first, dirsAligned.second);
    }

    //set the general parameters
    sectionLine->setPos(0.0, 0.0);

    if (vp->IncludeCutLine.getValue()) {
        sectionLine->setShowLine(true);
        // sectionLines are typically ISO 8 (long dash, short dash) or ISO 4 (long dash, dot)
        sectionLine->setLinePen(
                m_dashedLineGenerator->getLinePen((size_t)vp->SectionLineStyle.getValue(),
                                                    vp->HiddenWidth.getValue()));
        sectionLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
    } else {
        sectionLine->setShowLine(false);
    }

    double fontSize = Preferences::dimFontSizeMM();
    sectionLine->setFont(getFont(), fontSize);
    sectionLine->setZValue(ZVALUE::SECTIONLINE);
    sectionLine->setRotation(-viewPart->Rotation.getValue());
    sectionLine->draw();
}

//TODO: use Cosmetic::CenterLine object for this to make it usable for dims.
// these are the view center lines (ie x,y axes)
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
            centerLine->setLinePen(m_dashedLineGenerator->getLinePen((size_t)Preferences::CenterLineStyle(),
                                  vp->HiddenWidth.getValue()));
            centerLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
            centerLine->setColor(Qt::green);
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
            centerLine->setLinePen(m_dashedLineGenerator->getLinePen((size_t)Preferences::CenterLineStyle(),
                                  vp->HiddenWidth.getValue()));
            centerLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
            centerLine->setColor(Qt::red);
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
    auto* viewPart = static_cast<TechDraw::DrawViewPart*>(getViewObject());
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

    if (!viewDetail->ShowHighlight.getValue()) {
        return;
    }

    if (b) {
        double fontSize = Preferences::labelFontSizeMM();
        auto* highlight = new QGIHighlight();

        scene()->addItem(highlight);
        highlight->setReference(viewDetail->Reference.getValue());

        Base::Color color = Preferences::getAccessibleColor(vp->HighlightLineColor.getValue());
        highlight->setColor(color.asValue<QColor>());
        highlight->setFeatureName(viewDetail->getNameInDocument());

        highlight->setInteractive(false);

        addToGroup(highlight);
        highlight->setPos(0.0, 0.0);//sb setPos(center.x, center.y)?

        Base::Vector3d center = viewDetail->AnchorPoint.getValue() * viewPart->getScale();
        double rotationRad = Base::toRadians(viewPart->Rotation.getValue());
        center.RotateZ(rotationRad);

        double radius = viewDetail->Radius.getValue() * viewPart->getScale();
        highlight->setBounds(center.x - radius, center.y + radius, center.x + radius,
                             center.y - radius);
        highlight->setLinePen(m_dashedLineGenerator->getLinePen((size_t)vp->HighlightLineStyle.getValue(),
                             vp->IsoWidth.getValue()));
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

//! this method is no longer used due to conflicts with TaskDetail dialog highlight drag
void QGIViewPart::highlightMoved(QGIHighlight* highlight, QPointF newPos)
{
    std::string highlightName = highlight->getFeatureName();
    App::Document* doc = getViewObject()->getDocument();
    App::DocumentObject* docObj = doc->getObject(highlightName.c_str());
    auto detail = freecad_cast<DrawViewDetail*>(docObj);
    auto baseView = freecad_cast<DrawViewPart*>(getViewObject());
    if (detail && baseView) {
        auto oldAnchor = detail->AnchorPoint.getValue();
        Base::Vector3d delta = Rez::appX(DrawUtil::toVector3d(newPos)) / getViewObject()->getScale();
        delta = DrawUtil::invertY(delta);
        Base::Vector3d newAnchorPoint = oldAnchor + delta;
                newAnchorPoint = baseView->snapHighlightToVertex(newAnchorPoint,
                                                                 detail->Radius.getValue());
        detail->AnchorPoint.setValue(newAnchorPoint);
    }
}


void QGIViewPart::drawMatting()
{
    auto viewPart(dynamic_cast<TechDraw::DrawViewPart*>(getViewObject()));
    TechDraw::DrawViewDetail* dvd = nullptr;
    if (viewPart && viewPart->isDerivedFrom<TechDraw::DrawViewDetail>()) {
        dvd = static_cast<TechDraw::DrawViewDetail*>(viewPart);
    }
    else {
        return;
    }

    if (!dvd->ShowMatting.getValue()) {
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


//! if this is a broken view, draw the break lines.
void QGIViewPart::drawBreakLines()
{
    // Base::Console().message("QGIVP::drawBreakLines()\n");

    auto dbv = dynamic_cast<TechDraw::DrawBrokenView*>(getViewObject());
    if (!dbv) {
        return;
    }

    auto vp = static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()));
    if (!vp) {
        return;
    }

    DrawBrokenView::BreakType breakType = static_cast<DrawBrokenView::BreakType>(vp->BreakLineType.getValue());
    auto breaks = dbv->Breaks.getValues();
    for (auto& breakObj : breaks) {
        QGIBreakLine* breakLine = new QGIBreakLine();
        addToGroup(breakLine);

        Base::Vector3d direction = dbv->guiDirectionFromObj(*breakObj);
        breakLine->setDirection(direction);
        // the bounds describe two corners of the removed area in the view
        std::pair<Base::Vector3d, Base::Vector3d> bounds = dbv->breakBoundsFromObj(*breakObj);
        // the bounds are in 3d form, so we need to invert & rez them
        Base::Vector3d topLeft     = Rez::guiX(DU::invertY(bounds.first));
        Base::Vector3d bottomRight = Rez::guiX(DU::invertY(bounds.second));
        breakLine->setBounds(topLeft, bottomRight);
        breakLine->setPos(0.0, 0.0);
        breakLine->setLinePen(
            m_dashedLineGenerator->getLinePen(vp->BreakLineStyle.getValue(), vp->HiddenWidth.getValue()));
        breakLine->setWidth(Rez::guiX(vp->HiddenWidth.getValue()));
        breakLine->setBreakType(breakType);
        breakLine->setZValue(ZVALUE::SECTIONLINE);
        Base::Color color = prefBreaklineColor();
        breakLine->setBreakColor(color.asValue<QColor>());
        breakLine->setRotation(-dbv->Rotation.getValue());
        breakLine->draw();
    }
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
    Base::Console().message(">>>%s has %d elements\n", text, path.elementCount());
    const char* typeName;
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
        Base::Console().message(">>>>> element %d: type:%d/%s pos(%.3f, %.3f) M:%d L:%d C:%d\n",
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

Base::Color QGIViewPart::prefBreaklineColor()
{
    return  Preferences::getAccessibleColor(PreferencesGui::breaklineColor());
}

QGraphicsItem *QGIViewPart::getQGISubItemByName(const std::string &subName) const
{
    int scanType = 0;
    try {
        const std::string &subType = TechDraw::DrawUtil::getGeomTypeFromName(subName);
        if (subType == "Vertex") {
            scanType = QGIVertex::Type;
        }
        else if (subType == "Edge") {
            scanType = QGIEdge::Type;
        }
        else if (subType == "Face") {
            scanType = QGIFace::Type;
        }
    }
    catch (Base::ValueError&) {
        // No action
    }
    if (!scanType) {
        return nullptr;
    }

    int scanIndex = -1;
    try {
        scanIndex = TechDraw::DrawUtil::getIndexFromName(subName);
    }
    catch (Base::ValueError&) {
        // No action
    }
    if (scanIndex < 0) {
        return nullptr;
    }

    for (auto child : childItems()) {
        if (child->type() != scanType) {
            continue;
        }

        int projIndex;
        switch (scanType) {
            case QGIVertex::Type:
                projIndex = static_cast<QGIVertex *>(child)->getProjIndex();
                break;
            case QGIEdge::Type:
                projIndex = static_cast<QGIEdge *>(child)->getProjIndex();
                break;
            case QGIFace::Type:
                projIndex = static_cast<QGIFace *>(child)->getProjIndex();
                break;
            default:
                projIndex = -1;
                break;
        }

        if (projIndex == scanIndex) {
            return child;
        }
    }

    return nullptr;
}

bool QGIViewPart::getGroupSelection() {
    return DrawGuiUtil::isSelectedInTree(this);
}

void QGIViewPart::setGroupSelection(bool isSelected) {
    DrawGuiUtil::setSelectedTree(this, isSelected);
}

void QGIViewPart::setGroupSelection(bool isSelected, const std::vector<std::string> &subNames)
{
    if (subNames.empty()) {
        setSelected(isSelected);
        return;
    }

    for (const std::string &subName : subNames) {
        if (subName.empty()) {
            setSelected(isSelected);
            continue;
        }

        QGraphicsItem *subItem = getQGISubItemByName(subName);
        if (subItem) {
            subItem->setSelected(isSelected);
        }
    }
}

double QGIViewPart::getLineWidth() {
    auto vp{static_cast<ViewProviderViewPart*>(getViewProvider(getViewObject()))};

    return vp->LineWidth.getValue() * lineScaleFactor; // Thick
}

double QGIViewPart::getVertexSize() {
    return getLineWidth() * Preferences::vertexScale();
}

void QGIViewPart::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView::hoverEnterEvent(event);

    for (QGraphicsItem* item : m_vertexItems) {
        if (item) {
            item->setVisible(true);
        }
    }
    update();
}

void QGIViewPart::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGIView::hoverLeaveEvent(event);

    for (QGraphicsItem* item : m_vertexItems) {
        if (item && !item->isSelected()) {
            item->setVisible(false);
        }
    }
    update();
}
