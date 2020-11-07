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
# include <QAction>
# include <QApplication>
# include <QContextMenuEvent>
# include <QFileInfo>
# include <QFileDialog>
# include <QGLWidget>
# include <QGraphicsEffect>
# include <QMouseEvent>
# include <QPainter>
# include <QPaintEvent>
# include <QSvgGenerator>
#include <QScrollBar>
# include <QWheelEvent>
#include <QTemporaryFile>
#include <QDomDocument>
#include <QTextStream>
#include <QFile>
#include <QLabel>
#include <QTextCodec>
#include <cmath>
#endif

#include <QXmlQuery>
#include <QXmlResultItems>

#include <App/Application.h>
#include <App/Document.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Stream.h>
#include <Gui/FileDialog.h>
#include <Gui/Selection.h>
#include <Gui/WaitCursor.h>
#include <Gui/Command.h>

#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>
#include <Mod/TechDraw/App/DrawParametricTemplate.h>
#include <Mod/TechDraw/App/DrawViewCollection.h>
#include <Mod/TechDraw/App/DrawViewBalloon.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
//#include <Mod/TechDraw/App/LandmarkDimension.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewAnnotation.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>
#include <Mod/TechDraw/App/DrawViewImage.h>
#include <Mod/TechDraw/App/DrawLeaderLine.h>
#include <Mod/TechDraw/App/DrawRichAnno.h>
#include <Mod/TechDraw/App/DrawWeldSymbol.h>
#include <Mod/TechDraw/App/DrawTile.h>
#include <Mod/TechDraw/App/DrawTileWeld.h>
#include <Mod/TechDraw/App/QDomNodeModel.h>
#include <Mod/TechDraw/App/DrawUtil.h>

#include "Rez.h"
#include "QGIDrawingTemplate.h"
#include "QGITemplate.h"
#include "QGISVGTemplate.h"
#include "TemplateTextField.h"
#include "QGIViewCollection.h"
#include "QGIViewDimension.h"
#include "QGIViewBalloon.h"
#include "QGIProjGroup.h"
#include "QGIViewPart.h"
#include "QGIViewSection.h"
#include "QGIViewAnnotation.h"
#include "QGIViewSymbol.h"
#include "QGIViewClip.h"
#include "QGIViewSpreadsheet.h"
#include "QGIViewImage.h"
#include "QGIFace.h"
#include "QGILeaderLine.h"
#include "QGIRichAnno.h"
#include "QGIWeldSymbol.h"
#include "QGITile.h"

#include "ZVALUE.h"
#include "ViewProviderPage.h"
#include "QGVPage.h"
#include "MDIViewPage.h"

// used SVG namespaces
#define CC_NS_URI "http://creativecommons.org/ns#"
#define DC_NS_URI "http://purl.org/dc/elements/1.1/"
#define RDF_NS_URI "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

QGVPage::QGVPage(ViewProviderPage *vp, QGraphicsScene* s, QWidget *parent)
    : QGraphicsView(parent),
      pageTemplate(0),
      m_renderer(Native),
      drawBkg(true),
      m_vpPage(0),
      panningActive(false)
{
    assert(vp);
    m_vpPage = vp;
    const char* name = vp->getDrawPage()->getNameInDocument();
    setObjectName(QString::fromLocal8Bit(name));
    m_vpPage->setGraphicsView(this);

    setScene(s);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);

//    setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setViewportUpdateMode(QGraphicsView::FullViewportUpdate); //this prevents crash when deleting dims.
                                                          //scene(view?) indices of dirty regions gets
                                                          //out of sync.  missing prepareGeometryChange
                                                          //somewhere???? QTBUG-18021????
    setCacheMode(QGraphicsView::CacheBackground);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("View");
    m_atCursor = hGrp->GetBool("ZoomAtCursor", 1l);
    m_invertZoom = hGrp->GetBool("InvertZoom", 0l);
    m_zoomIncrement = hGrp->GetFloat("ZoomStep",0.02);
    hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/General");
    m_reversePan = hGrp->GetInt("KbPan",1);
    m_reverseScroll = hGrp->GetInt("KbScroll",1);


    if (m_atCursor) {
        setResizeAnchor(AnchorUnderMouse);
        setTransformationAnchor(AnchorUnderMouse);
    } else {
        setResizeAnchor(AnchorViewCenter);
        setTransformationAnchor(AnchorViewCenter);
    }
    setAlignment(Qt::AlignCenter);

    setDragMode(ScrollHandDrag);
    setCursor(QCursor(Qt::ArrowCursor));
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    bkgBrush = new QBrush(getBackgroundColor());

    balloonCursor = new QLabel(this);
    balloonCursor->setPixmap(QPixmap(QString::fromUtf8(":/icons/cursor-balloon.png")));
    balloonCursor->hide();

    resetCachedContent();
}

QGVPage::~QGVPage()
{
    delete bkgBrush;

}

void QGVPage::cancelBalloonPlacing(void)
{
        getDrawPage()->balloonPlacing = false;
        balloonCursor->hide();
        QApplication::restoreOverrideCursor();
}

void QGVPage::drawBackground(QPainter *p, const QRectF &)
{
//Note: Background is not part of scene()
    if(!drawBkg)
        return;

    if(!m_vpPage) {
        return;
    }

    if (!m_vpPage->getDrawPage()) {
        //Base::Console().Log("TROUBLE - QGVP::drawBackground - no Page Object!\n");
        return;
    }

    p->save();
    p->resetTransform();


    p->setBrush(*bkgBrush);
    p->drawRect(viewport()->rect().adjusted(-2,-2,2,2));   //just bigger than viewport to prevent artifacts

    // Default to A3 landscape, though this is currently relevant
    // only for opening corrupt docs, etc.
    float pageWidth = 420,
          pageHeight = 297;

    if ( m_vpPage->getDrawPage()->hasValidTemplate() ) {
        pageWidth = Rez::guiX(m_vpPage->getDrawPage()->getPageWidth());
        pageHeight = Rez::guiX(m_vpPage->getDrawPage()->getPageHeight());
    }

    // Draw the white page
    QRectF paperRect(0, -pageHeight, pageWidth, pageHeight);
    QPolygon poly = mapFromScene(paperRect);

    QBrush pageBrush(Qt::white);
    p->setBrush(pageBrush);

    p->drawRect(poly.boundingRect());

    p->restore();
}

//! retrieve the QGIView objects currently in the scene
std::vector<QGIView *> QGVPage::getViews() const
{
    std::vector<QGIView*> result;
    QList<QGraphicsItem*> items = scene()->items();
    for (auto& v:items) {
        QGIView* qv = dynamic_cast<QGIView*>(v);
        if (qv != nullptr) {
            result.push_back(qv);
        }
    }
    return result;
}

int QGVPage::addQView(QGIView *view)
{
    //don't add twice!
    QGIView* existing = getQGIVByName(view->getViewName());
    if (existing == nullptr) {
        auto ourScene( scene() );
        assert(ourScene);

        ourScene->addItem(view);

        // Find if it belongs to a parent
        QGIView *parent = 0;
        parent = findParent(view);

        QPointF viewPos(Rez::guiX(view->getViewObject()->X.getValue()),
                        Rez::guiX(view->getViewObject()->Y.getValue() * -1));

        if(parent) {
            // move child view to center of parent
            QPointF posRef(0.,0.);
            QPointF mapPos = view->mapToItem(parent, posRef);
            view->moveBy(-mapPos.x(), -mapPos.y());

            parent->addToGroup(view);
        }

        view->setPos(viewPos);
        view->updateView(true);
    }
    return 0;
}

int QGVPage::removeQView(QGIView *view)
{
    if (view != nullptr) {
        removeQViewFromScene(view);
        delete view;
    }
    return 0;
}

int QGVPage::removeQViewByName(const char* name)
{
    std::vector<QGIView*> items = getViews();
    QString qsName = QString::fromUtf8(name);
    bool found = false;
    QGIView* ourItem = nullptr;
    for (auto& i:items) {
        if (qsName == i->data(1).toString()) {          //is there a QGIV with this name in scene?
            found = true;
            ourItem = i;
            break;
        }
    }

    if (found) {
        int balloonItemType = QGraphicsItem::UserType + 140;
        if (ourItem->type() == balloonItemType) {
            QGIViewBalloon* balloon = dynamic_cast<QGIViewBalloon*>(ourItem);
            balloon->disconnect();
        }
        removeQViewFromScene(ourItem);
        delete ourItem;              //commenting this prevents crash but means a small memory waste.
                                     //alternate fix(?) is to change indexing/caching option in scene/view
    }

    return 0;
}

void QGVPage::removeQViewFromScene(QGIView *view)
{
    if (view->scene() != nullptr) {
        QGIView* qgParent = dynamic_cast<QGIView*>(view->parentItem());
        if (qgParent != nullptr) {
            qgParent->removeChild(view);
        } else {
            view->scene()->removeItem(view);
        }
    }
}


QGIView * QGVPage::addViewPart(TechDraw::DrawViewPart *part)
{
    QGIView* existing = findQViewForDocObj(part);
    if (existing != nullptr) {
       return existing;
    }

    auto viewPart( new QGIViewPart );

    viewPart->setViewPartFeature(part);

    addQView(viewPart);
    return viewPart;
}

QGIView * QGVPage::addViewSection(TechDraw::DrawViewPart *part)
{
    auto viewSection( new QGIViewSection );

    viewSection->setViewPartFeature(part);

    addQView(viewSection);
    return viewSection;
}

QGIView * QGVPage::addProjectionGroup(TechDraw::DrawProjGroup *view) {
    auto qview( new QGIProjGroup );

    qview->setViewFeature(view);
    addQView(qview);
    return qview;
}

QGIView * QGVPage::addDrawView(TechDraw::DrawView *view)
{
    auto qview( new QGIView );

    qview->setViewFeature(view);
    addQView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewCollection(TechDraw::DrawViewCollection *view)
{
    auto qview( new QGIViewCollection );

    qview->setViewFeature(view);
    addQView(qview);
    return qview;
}

// TODO change to (App?) annotation object  ??
QGIView * QGVPage::addDrawViewAnnotation(TechDraw::DrawViewAnnotation *view)
{
    auto qview( new QGIViewAnnotation );

    qview->setViewAnnoFeature(view);

    addQView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewSymbol(TechDraw::DrawViewSymbol *view)
{
    auto qview( new QGIViewSymbol );

    qview->setViewFeature(view);

    addQView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewClip(TechDraw::DrawViewClip *view)
{
    auto qview( new QGIViewClip );

    qview->setPosition(Rez::guiX(view->X.getValue()), Rez::guiX(view->Y.getValue()));
    qview->setViewFeature(view);

    addQView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewSpreadsheet(TechDraw::DrawViewSpreadsheet *view)
{
    auto qview( new QGIViewSpreadsheet );

    qview->setViewFeature(view);

    addQView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewImage(TechDraw::DrawViewImage *view)
{
    auto qview( new QGIViewImage );

    qview->setViewFeature(view);

    addQView(qview);
    return qview;
}

QGIView * QGVPage::addViewBalloon(TechDraw::DrawViewBalloon *balloon)
{
    auto vBalloon( new QGIViewBalloon );

    auto ourScene( scene() );
    assert(ourScene);
    ourScene->addItem(vBalloon);

    vBalloon->setViewPartFeature(balloon);
    vBalloon->dvBalloon = balloon;

    QGIView *parent = 0;
    parent = findParent(vBalloon);

    if(parent)
        addBalloonToParent(vBalloon,parent);

    if (getDrawPage()->balloonPlacing) {
            vBalloon->placeBalloon(balloon->origin);
            cancelBalloonPlacing();
    }

    return vBalloon;
}

void QGVPage::addBalloonToParent(QGIViewBalloon* balloon, QGIView* parent)
{
    assert(balloon);
    assert(parent);          //blow up if we don't have Dimension or Parent
    QPointF posRef(0.,0.);
    QPointF mapPos = balloon->mapToItem(parent, posRef);
    balloon->moveBy(-mapPos.x(), -mapPos.y());
    parent->addToGroup(balloon);
    balloon->setZValue(ZVALUE::DIMENSION);
}

QGIView * QGVPage::addViewDimension(TechDraw::DrawViewDimension *dim)
{
    auto dimGroup( new QGIViewDimension );

    auto ourScene( scene() );
    assert(ourScene);
    ourScene->addItem(dimGroup);

    dimGroup->setViewPartFeature(dim);

    // Find if it belongs to a parent
    QGIView *parent = 0;
    parent = findParent(dimGroup);

    if(parent) {
        addDimToParent(dimGroup,parent);
    }

    return dimGroup;
}

void QGVPage::addDimToParent(QGIViewDimension* dim, QGIView* parent)
{
//    Base::Console().Message("QGVP::addDimToParent()\n");
    assert(dim);
    assert(parent);          //blow up if we don't have Dimension or Parent
    QPointF posRef(0.,0.);
    QPointF mapPos = dim->mapToItem(parent, posRef);
    dim->moveBy(-mapPos.x(), -mapPos.y());
    parent->addToGroup(dim);
    dim->setZValue(ZVALUE::DIMENSION);
}

QGIView * QGVPage::addViewLeader(TechDraw::DrawLeaderLine *leader)
{
//    Base::Console().Message("QGVP::addViewLeader(%s)\n",leader->getNameInDocument());
    QGILeaderLine* leaderGroup = new QGILeaderLine();

    auto ourScene( scene() );
    ourScene->addItem(leaderGroup);

    leaderGroup->setLeaderFeature(leader);

    QGIView *parent = 0;
    parent = findParent(leaderGroup);

    if(parent) {
        addLeaderToParent(leaderGroup,parent);
    }

    leaderGroup->updateView(true);

    return leaderGroup;
}

void QGVPage::addLeaderToParent(QGILeaderLine* lead, QGIView* parent)
{
//    Base::Console().Message("QGVP::addLeaderToParent()\n");
    parent->addToGroup(lead);
    lead->setZValue(ZVALUE::DIMENSION);
}

QGIView * QGVPage::addRichAnno(TechDraw::DrawRichAnno* anno)
{
    QGIRichAnno* annoGroup = nullptr;
    TechDraw::DrawView*  parentDV = nullptr;
    
    App::DocumentObject* parentObj = anno->AnnoParent.getValue();
    if (parentObj != nullptr) {
        parentDV  = dynamic_cast<TechDraw::DrawView*>(parentObj);
    }
    if (parentDV != nullptr) {
        QGIView* parentQV = findQViewForDocObj(parentObj);
        annoGroup = new QGIRichAnno(parentQV, anno);
        annoGroup->updateView(true);
    } else {
        annoGroup = new QGIRichAnno(nullptr, anno);
        if (annoGroup->scene() == nullptr) {
            scene()->addItem(annoGroup);
        }
        annoGroup->updateView(true);
    }
    return annoGroup;
}

QGIView * QGVPage::addWeldSymbol(TechDraw::DrawWeldSymbol* weld)
{
//    Base::Console().Message("QGVP::addWeldSymbol()\n");
    QGIWeldSymbol* weldGroup = nullptr;
    TechDraw::DrawView*  parentDV = nullptr;
    
    App::DocumentObject* parentObj = weld->Leader.getValue();
    if (parentObj != nullptr) {
        parentDV  = dynamic_cast<TechDraw::DrawView*>(parentObj);
    } else {
//        Base::Console().Message("QGVP::addWeldSymbol - no parent doc obj\n");
    }
    if (parentDV != nullptr) {
        QGIView* parentQV = findQViewForDocObj(parentObj);
        QGILeaderLine* leadParent = dynamic_cast<QGILeaderLine*>(parentQV);
        if (leadParent != nullptr) {
            weldGroup = new QGIWeldSymbol(leadParent);
            weldGroup->setFeature(weld);       //for QGIWS
            weldGroup->setViewFeature(weld);   //for QGIV
            weldGroup->updateView(true);
        } else {
            Base::Console().Error("QGVP::addWeldSymbol - no parent QGILL\n");
        }
    } else {
        Base::Console().Error("QGVP::addWeldSymbol - parent is not DV!\n");
    }
    return weldGroup;
}


//! find the graphic for a DocumentObject
QGIView * QGVPage::findQViewForDocObj(App::DocumentObject *obj) const
{
  if(obj) {
    const std::vector<QGIView *> qviews = getViews();
    for(std::vector<QGIView *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
          if(strcmp(obj->getNameInDocument(), (*it)->getViewName()) == 0)
              return *it;
      }
  }
    return 0;
}

//! find the graphic for DocumentObject with name
QGIView* QGVPage::getQGIVByName(std::string name)
{
    QList<QGraphicsItem*> qgItems = scene()->items();
    QList<QGraphicsItem*>::iterator it = qgItems.begin();
    for (; it != qgItems.end(); it++) {
        QGIView* qv = dynamic_cast<QGIView*>((*it));
        if (qv) {
            const char* qvName = qv->getViewName();
            if(name.compare(qvName) == 0) {
                return (qv);
            }
        }
    }
    return nullptr;
}

//find the parent of a QGIV based on the corresponding feature's parentage
QGIView * QGVPage::findParent(QGIView *view) const
{
    const std::vector<QGIView *> qviews = getViews();
    TechDraw::DrawView *myFeat = view->getViewObject();

//LandmarkDimension shouldn't require special handling
//    TechDraw::LandmarkDimension *robust = nullptr;
//    robust = dynamic_cast<TechDraw::LandmarkDimension*>(myFeat);
//    if (robust != nullptr) {
//        App::DocumentObject* robustParent = robust->ParentView.getValue();
//        for (auto& qv: qviews) {
//            if(strcmp(qv->getViewName(), robustParent->getNameInDocument()) == 0) {
//                return qv;
//            }
//        }
//    }

    //If type is dimension we check references first
    TechDraw::DrawViewDimension *dim = 0;
    dim = dynamic_cast<TechDraw::DrawViewDimension *>(myFeat);
    if(dim) {
        std::vector<App::DocumentObject *> objs = dim->References2D.getValues();

        if(objs.size() > 0) {
            std::vector<App::DocumentObject *> objs = dim->References2D.getValues();
            // Attach the dimension to the first object's group
            for(std::vector<QGIView *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
                if(strcmp((*it)->getViewName(), objs.at(0)->getNameInDocument()) == 0) {
                    return *it;
                }
            }
        }
    }

    //If type is balloon we check references first
    TechDraw::DrawViewBalloon *balloon = 0;
    balloon = dynamic_cast<TechDraw::DrawViewBalloon *>(myFeat);

    if(balloon) {
        App::DocumentObject* obj = balloon->SourceView.getValue();

        if(obj) {
            // Attach the dimension to the first object's group
            for(std::vector<QGIView *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
                if(strcmp((*it)->getViewName(), obj->getNameInDocument()) == 0) {
                    return *it;
                }
            }
        }
    }

    // Check if part of view collection
    for(std::vector<QGIView *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
        QGIViewCollection *grp = 0;
        grp = dynamic_cast<QGIViewCollection *>(*it);
        if(grp) {
            TechDraw::DrawViewCollection *collection = 0;
            collection = dynamic_cast<TechDraw::DrawViewCollection *>(grp->getViewObject());
            if(collection) {
                std::vector<App::DocumentObject *> objs = collection->Views.getValues();
                for( std::vector<App::DocumentObject *>::iterator it = objs.begin(); it != objs.end(); ++it) {
                    if(strcmp(myFeat->getNameInDocument(), (*it)->getNameInDocument()) == 0)

                        return grp;
                }
            }
        }
    }

     //If type is LeaderLine we check LeaderParent
    TechDraw::DrawLeaderLine *lead = 0;
    lead = dynamic_cast<TechDraw::DrawLeaderLine *>(myFeat);

    if(lead) {
        App::DocumentObject* obj = lead->LeaderParent.getValue();
        if(obj != nullptr) {
            std::string parentName = obj->getNameInDocument();
            for(std::vector<QGIView *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
                if(strcmp((*it)->getViewName(), parentName.c_str()) == 0) {
                    return *it;
                }
            }
        }
    }
    // Not found a parent
    return nullptr;
}

void QGVPage::setPageTemplate(TechDraw::DrawTemplate *obj)
{
    removeTemplate();

    if(obj->isDerivedFrom(TechDraw::DrawParametricTemplate::getClassTypeId())) {
        pageTemplate = new QGIDrawingTemplate(scene());
    } else if(obj->isDerivedFrom(TechDraw::DrawSVGTemplate::getClassTypeId())) {
        pageTemplate = new QGISVGTemplate(scene());
    }
    pageTemplate->setTemplate(obj);
    pageTemplate->updateView();
}

QGITemplate* QGVPage::getTemplate() const
{
    return pageTemplate;
}

void QGVPage::removeTemplate()
{
    if(pageTemplate) {
        scene()->removeItem(pageTemplate);
        pageTemplate->deleteLater();
        pageTemplate = 0;
    }
}
void QGVPage::setRenderer(RendererType type)
{
    m_renderer = type;

    if (m_renderer == OpenGL) {
#ifndef QT_NO_OPENGL
        setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
#endif
    } else {
        setViewport(new QWidget);
    }
}

void QGVPage::setHighQualityAntialiasing(bool highQualityAntialiasing)
{
#ifndef QT_NO_OPENGL
    setRenderHint(QPainter::HighQualityAntialiasing, highQualityAntialiasing);
#else
    Q_UNUSED(highQualityAntialiasing);
#endif
}

void QGVPage::refreshViews(void)
{
//    Base::Console().Message("QGVP::refreshViews()\n");
    QList<QGraphicsItem*> list = scene()->items();
    QList<QGraphicsItem*> qgiv;
    //find only QGIV's 
    for (auto q: list) {
        QString viewFamily = QString::fromUtf8("QGIV");
        if (viewFamily == q->data(0).toString()) {
            qgiv.push_back(q);
        }
    }
    for (auto q: qgiv) {
        QGIView *itemView = dynamic_cast<QGIView *>(q);
        if(itemView) {
            itemView->updateView(true);
        }
    }
}

void QGVPage::setExporting(bool enable)
{
    QList<QGraphicsItem*> sceneItems = scene()->items();
    std::vector<QGIViewPart*> dvps;
    for (auto& qgi:sceneItems) {
        QGIViewPart* qgiPart = dynamic_cast<QGIViewPart *>(qgi);
        QGIRichAnno* qgiRTA  = dynamic_cast<QGIRichAnno *>(qgi);
        if(qgiPart) {
            qgiPart->setExporting(enable);
            dvps.push_back(qgiPart);
        }
        if (qgiRTA) {
            qgiRTA->setExporting(enable);
        }
    }
    for (auto& v: dvps) {
        v->draw();
    }
}

void QGVPage::saveSvg(QString filename)
{
    // TODO: We only have m_vpPage because constructor gets passed a view provider...
    //NOTE: this makes wrong size pages in low-Rez
    TechDraw::DrawPage *page( m_vpPage->getDrawPage() );

    const QString docName( QString::fromUtf8(page->getDocument()->getName()) );
    const QString pageName( QString::fromUtf8(page->getNameInDocument()) );
    QString svgDescription = QString::fromUtf8("Drawing page: ") +
                             pageName +
                             QString::fromUtf8(" exported from FreeCAD document: ") +
                             docName;

    QSvgGenerator svgGen;
    QTemporaryFile temporaryFile;
    svgGen.setOutputDevice(&temporaryFile);

    // Set resolution in DPI. Use the actual one, i.e. Rez::guiX(inch)
    svgGen.setResolution(Rez::guiX(25.4));

    // Set size in pixels, which Qt recomputes using DPI to mm.
    int pixelWidth = Rez::guiX(page->getPageWidth());
    int pixelHeight = Rez::guiX(page->getPageHeight());
    svgGen.setSize(QSize(pixelWidth, pixelHeight));

    //"By default this property is set to QSize(-1, -1), which indicates that the generator should not output
    // the width and height attributes of the <svg> element."  >> but Inkscape won't read it without size info??
    svgGen.setViewBox(QRect(0, 0, pixelWidth, pixelHeight));

    svgGen.setTitle(QString::fromUtf8("FreeCAD SVG Export"));
    svgGen.setDescription(svgDescription);

    Gui::Selection().clearSelection();

    bool saveState = m_vpPage->getFrameState();
    m_vpPage->setFrameState(false);
    m_vpPage->setTemplateMarkers(false);
    setExporting(true);

    // Here we temporarily hide the page template, because Qt would otherwise convert the SVG template
    // texts into series of paths, making the later document edits practically unfeasible.
    // We will insert the SVG template ourselves in the final XML postprocessing operation.
    QGISVGTemplate *svgTemplate = dynamic_cast<QGISVGTemplate *>(pageTemplate);
    bool templateVisible = false;
    if (svgTemplate) {
        templateVisible = svgTemplate->isVisible();
        svgTemplate->hide();
    }

    refreshViews();
    viewport()->repaint();

    double width  =  Rez::guiX(page->getPageWidth());
    double height =  Rez::guiX(page->getPageHeight());
    QRectF sourceRect(0.0,-height,width,height);
    QRectF targetRect(0.0,0.0,width,height);

    Gui::Selection().clearSelection();
    QPainter p;

    p.begin(&svgGen);
    scene()->render(&p, targetRect,sourceRect);    //note: scene render, not item render!
    p.end();

    m_vpPage->setFrameState(saveState);
    m_vpPage->setTemplateMarkers(saveState);
    setExporting(false);
    if (templateVisible && svgTemplate) {
        svgTemplate->show();
    }

    refreshViews();
    viewport()->repaint();

    temporaryFile.close();
    postProcessXml(temporaryFile, filename, pageName);
}

void QGVPage::postProcessXml(QTemporaryFile& temporaryFile, QString fileName, QString pageName)
{
    QDomDocument exportDoc(QString::fromUtf8("SvgDoc"));
    QFile file(temporaryFile.fileName());
    if (!file.open(QIODevice::ReadOnly)) {
        Base::Console().Message("QGVPage::ppsvg - tempfile open error\n");
        return;
    }
    if (!exportDoc.setContent(&file)) {
        Base::Console().Message("QGVPage::ppsvg - xml error\n");
        file.close();
        return;
    }
    file.close();

    QDomElement exportDocElem = exportDoc.documentElement();          //root <svg>

    // Insert Freecad SVG namespace into namespace declarations
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:freecad"),
                               QString::fromUtf8(FREECAD_SVG_NS_URI));
    // Insert all namespaces used by TechDraw's page template SVGs
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:svg"),
        QString::fromUtf8(SVG_NS_URI));
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:cc"),
        QString::fromUtf8(CC_NS_URI));
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:dc"),
        QString::fromUtf8(DC_NS_URI));
    exportDocElem.setAttribute(QString::fromUtf8("xmlns:rdf"),
        QString::fromUtf8(RDF_NS_URI));

    // Create the root group which will host the drawing group and the template group
    QDomElement rootGroup = exportDoc.createElement(QString::fromUtf8("g"));
    rootGroup.setAttribute(QString::fromUtf8("id"), pageName);

    // Now insert our template
    QGISVGTemplate *svgTemplate = dynamic_cast<QGISVGTemplate *>(pageTemplate);
    if (svgTemplate) {
        DrawSVGTemplate *drawTemplate = svgTemplate->getSVGTemplate();
        if (drawTemplate) {
            QFile templateResultFile(QString::fromUtf8(drawTemplate->PageResult.getValue()));
            if (templateResultFile.open(QIODevice::ReadOnly)) {
                QDomDocument templateResultDoc(QString::fromUtf8("SvgDoc"));
                if (templateResultDoc.setContent(&templateResultFile)) {
                    QDomElement templateDocElem = templateResultDoc.documentElement();

                    // Insert the template into a new group with id set to template name
                    QDomElement templateGroup = exportDoc.createElement(QString::fromUtf8("g"));
                    Base::FileInfo fi(drawTemplate->Template.getValue());
                    templateGroup.setAttribute(QString::fromUtf8("id"),
                                               QString::fromUtf8(fi.fileName().c_str()));
                    templateGroup.setAttribute(QString::fromUtf8("style"),
                                               QString::fromUtf8("stroke: none;"));

                    // Scale the template group correctly
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
                    templateGroup.setAttribute(QString::fromUtf8("transform"),
                        QString().sprintf("scale(%f, %f)", Rez::guiX(1.0), Rez::guiX(1.0)));
#else
                    templateGroup.setAttribute(QString::fromUtf8("transform"),
                        QString::fromLatin1("scale(%1, %2)").arg(Rez::guiX(1.0), 0, 'f').arg(Rez::guiX(1.0), 0, 'f'));
#endif

                    // Finally, transfer all template document child nodes under the template group
                    while (!templateDocElem.firstChild().isNull()) {
                        templateGroup.appendChild(templateDocElem.firstChild());
                    }

                    rootGroup.appendChild(templateGroup);
                }
            }
        }
    }

    QXmlQuery query(QXmlQuery::XQuery10);
    QDomNodeModel model(query.namePool(), exportDoc);
    query.setFocus(QXmlItem(model.fromDomNode(exportDocElem)));

    // XPath query to select first <g> node as direct <svg> element descendant
    query.setQuery(QString::fromUtf8(
        "declare default element namespace \"" SVG_NS_URI "\"; "
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "/svg/g[1]"));

    QXmlResultItems queryResult;
    query.evaluateTo(&queryResult);

    // Obtain the drawing group element, move it under root node and set its id to "DrawingContent"
    QDomElement drawingGroup;
    if (!queryResult.next().isNull()) {
        drawingGroup = model.toDomNode(queryResult.current().toNodeModelIndex()).toElement();
        drawingGroup.setAttribute(QString::fromUtf8("id"), QString::fromUtf8("DrawingContent"));
        rootGroup.appendChild(drawingGroup);
    }

    exportDocElem.appendChild(rootGroup);

    // As icing on the cake, get rid of the empty <g>'s Qt SVG generator painting inserts.
    // XPath query to select any <g> element anywhere with no child nodes whatsoever
    query.setQuery(QString::fromUtf8(
        "declare default element namespace \"" SVG_NS_URI "\"; "
        "declare namespace freecad=\"" FREECAD_SVG_NS_URI "\"; "
        "//g[not(*)]"));

    query.evaluateTo(&queryResult);
    while (!queryResult.next().isNull()) {
        QDomElement g(model.toDomNode(queryResult.current().toNodeModelIndex()).toElement());
        g.parentNode().removeChild(g);
    }

    // Time to save our product
    QFile outFile( fileName );
    if( !outFile.open( QIODevice::WriteOnly | QIODevice::Text ) ) {
        Base::Console().Message("QGVP::ppxml - failed to open file for writing: %s\n",qPrintable(fileName) );
    }

    QTextStream stream( &outFile );
    stream.setGenerateByteOrderMark(false);
    stream.setCodec("UTF-8");

    stream << exportDoc.toByteArray();
    outFile.close();
}

void QGVPage::paintEvent(QPaintEvent *event)
{
    if (m_renderer == Image) {
        if (m_image.size() != viewport()->size()) {
            m_image = QImage(viewport()->size(), QImage::Format_ARGB32_Premultiplied);
        }

        QPainter imagePainter(&m_image);
        QGraphicsView::render(&imagePainter);
        imagePainter.end();

        QPainter p(viewport());
        p.drawImage(0, 0, m_image);

    } else {
        QGraphicsView::paintEvent(event);
    }
}

void QGVPage::wheelEvent(QWheelEvent *event)
{
//Delta is the distance that the wheel is rotated, in eighths of a degree.
//positive indicates rotation forwards away from the user; negative backwards toward the user.
//Most mouse types work in steps of 15 degrees, in which case the delta value is a multiple of 120; i.e., 120 units * 1/8 = 15 degrees.
//1 click = 15 degrees.  15 degrees = 120 deltas.  delta/240 -> 1 click = 0.5 ==> factor = 1.2^0.5 = 1.095
//                                                              1 click = -0.5 ==> factor = 1.2^-0.5 = 0.91
//so to change wheel direction, multiply (event->delta() / 240.0) by +/-1
    double mouseBase = 1.2;        //magic numbers. change for different mice?
    double mouseAdjust = -240.0;
    if (m_invertZoom) {
        mouseAdjust = -mouseAdjust;
    }

    QPointF center = mapToScene(viewport()->rect().center());
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    int delta = event->angleDelta().y();
#else
    int delta = event->delta();
#endif
    qreal factor = std::pow(mouseBase, delta / mouseAdjust);
    scale(factor, factor);

    QPointF newCenter = mapToScene(viewport()->rect().center());
    QPointF change = newCenter - center;
    translate(change.x(), change.y());

    event->accept();
}

void QGVPage::keyPressEvent(QKeyEvent *event)
{
    if(event->modifiers().testFlag(Qt::ControlModifier)) {
        switch(event->key()) {
            case Qt::Key_Plus: { 
                scale(1.0 + m_zoomIncrement, 1.0 + m_zoomIncrement);
                break; 
            }
            case Qt::Key_Minus: {
                scale(1.0 - m_zoomIncrement, 1.0 - m_zoomIncrement);
                break;
            }
            default: {
                break;
            }
        }
    }

    if(event->modifiers().testFlag( Qt::NoModifier)) {
        switch(event->key()) {
            case Qt::Key_Left: {
                kbPanScroll(1, 0);
                break;
            }
            case Qt::Key_Up: {
                kbPanScroll(0, 1);
                break;
            }
            case Qt::Key_Right: {
                kbPanScroll(-1, 0);
                break;
            }
            case Qt::Key_Down: {
                kbPanScroll(0, -1);
                break;
            }
            case Qt::Key_Escape: {
                cancelBalloonPlacing();
                break;
            }
            default: {
                break;
            }
        }
    }
    QGraphicsView::keyPressEvent(event);
}

void QGVPage::focusOutEvent(QFocusEvent *event) {
        Q_UNUSED(event);
        cancelBalloonPlacing();
}

void QGVPage::kbPanScroll(int xMove, int yMove) 
{
    if (xMove != 0) {
        QScrollBar* hsb = horizontalScrollBar();
//        int hRange = hsb->maximum() - hsb->minimum();     //default here is 100?
//        int hDelta = xMove/hRange
        int hStep = hsb->singleStep() * xMove * m_reversePan;
        int hNow = hsb->value();
        hsb->setValue(hNow + hStep);
    }
    if (yMove != 0) {
        QScrollBar* vsb = verticalScrollBar();
        int vStep = vsb->singleStep() * yMove * m_reverseScroll;
        int vNow = vsb->value();
        vsb->setValue(vNow + vStep);
    }
}

void QGVPage::enterEvent(QEvent *event)
{
    QGraphicsView::enterEvent(event);
    if(getDrawPage()->balloonPlacing) {
        balloonCursor->hide();
        QApplication::setOverrideCursor(QCursor(QPixmap(QString::fromUtf8(":/icons/cursor-balloon.png")),0,32));
      } else {
        QApplication::restoreOverrideCursor();
        viewport()->setCursor(Qt::ArrowCursor);
    }
}

void QGVPage::leaveEvent(QEvent * event)
{
    QApplication::restoreOverrideCursor();
    if(getDrawPage()->balloonPlacing) {


        int left_x;
        if (balloonCursorPos.x() < 32)
            left_x = 0;
        else if (balloonCursorPos.x() > (this->contentsRect().right() - 32))
            left_x = this->contentsRect().right() - 32;
        else
            left_x = balloonCursorPos.x();

        int left_y;
        if (balloonCursorPos.y() < 32)
            left_y = 0;
        else if (balloonCursorPos.y() > (this->contentsRect().bottom() - 32))
            left_y = this->contentsRect().bottom() - 32;
        else
            left_y = balloonCursorPos.y();

        /* When cursor leave the page, display balloonCursor where it left */
        balloonCursor->setGeometry(left_x ,left_y, 32, 32);
        balloonCursor->show();
    }
    
    QGraphicsView::leaveEvent(event);
}

void QGVPage::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::MiddleButton) {
        panOrigin = event->pos();
        panningActive = true;
        event->accept();

        QApplication::setOverrideCursor(Qt::ClosedHandCursor);
    }

    QGraphicsView::mousePressEvent(event);
}

void QGVPage::mouseMoveEvent(QMouseEvent *event)
{
    balloonCursorPos = event->pos();

    if (panningActive) {
        QScrollBar *horizontalScrollbar = horizontalScrollBar();
        QScrollBar *verticalScrollbar = verticalScrollBar();
        QPoint direction = event->pos() - panOrigin;

        horizontalScrollbar->setValue(horizontalScrollbar->value() - m_reversePan*direction.x());
        verticalScrollbar->setValue(verticalScrollbar->value() - m_reverseScroll*direction.y());

        panOrigin = event->pos();
        event->accept();
    }

    QGraphicsView::mouseMoveEvent(event);
}

void QGVPage::mouseReleaseEvent(QMouseEvent *event)
{
    if(getDrawPage()->balloonPlacing) {
        QApplication::restoreOverrideCursor();
        balloonCursor->hide();

        std::string FeatName = getDrawPage()->getDocument()->getUniqueObjectName("Balloon");
        std::string PageName = getDrawPage()->getNameInDocument();
        Gui::Command::openCommand("Create Balloon");
        TechDraw::DrawViewBalloon *balloon = 0;

        Gui::Command::openCommand("Create Balloon");
        Command::doCommand(Command::Doc,"App.activeDocument().addObject('TechDraw::DrawViewBalloon','%s')",FeatName.c_str());
        Command::doCommand(Command::Doc,"App.activeDocument().%s.addView(App.activeDocument().%s)",PageName.c_str(),FeatName.c_str());

        balloon = dynamic_cast<TechDraw::DrawViewBalloon *>(getDrawPage()->getDocument()->getObject(FeatName.c_str()));
        if (!balloon) {
            throw Base::TypeError("CmdTechDrawNewBalloon - balloon not found\n");
        }

        balloon->SourceView.setValue(getDrawPage()->balloonParent);
        balloon->origin = mapToScene(event->pos());

        Gui::Command::commitCommand();
        balloon->recomputeFeature();

        //Horrible hack to force Tree update
        double x = getDrawPage()->balloonParent->X.getValue();
        getDrawPage()->balloonParent->X.setValue(x);
    }

    if (event->button()&Qt::MiddleButton) {
        QApplication::restoreOverrideCursor();
        panningActive = false;
    }

    QGraphicsView::mouseReleaseEvent(event);
    viewport()->setCursor(Qt::ArrowCursor);
}

TechDraw::DrawPage* QGVPage::getDrawPage()
{
    return m_vpPage->getDrawPage();
}

QColor QGVPage::getBackgroundColor()
{
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                                        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Colors");
    App::Color fcColor;
    fcColor.setPackedValue(hGrp->GetUnsigned("Background", 0x70707000));
    return fcColor.asValue<QColor>();
}

#include <Mod/TechDraw/Gui/moc_QGVPage.cpp>
