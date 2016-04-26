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
# include <QGraphicsScene>
# include <QGraphicsEffect>
# include <QMouseEvent>
# include <QPainter>
# include <QPaintEvent>
# include <QSvgGenerator>
# include <QWheelEvent>
# include <strstream>
# include <cmath>
#endif

#include <Base/Console.h>
#include <Base/Stream.h>
#include <Gui/FileDialog.h>
#include <Gui/WaitCursor.h>

#include <Mod/TechDraw/App/Geometry.h>
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawTemplate.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>
#include <Mod/TechDraw/App/DrawParametricTemplate.h>
#include <Mod/TechDraw/App/DrawViewCollection.h>
#include <Mod/TechDraw/App/DrawViewDimension.h>
#include <Mod/TechDraw/App/DrawProjGroup.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/TechDraw/App/DrawViewAnnotation.h>
#include <Mod/TechDraw/App/DrawViewSymbol.h>
#include <Mod/TechDraw/App/DrawViewClip.h>
#include "../App/DrawHatch.h"


#include "QGIDrawingTemplate.h"
#include "QGISVGTemplate.h"
#include "QGIViewCollection.h"
#include "QGIViewDimension.h"
#include "QGIProjGroup.h"
#include "QGIViewPart.h"
#include "QGIViewSection.h"
#include "QGIViewAnnotation.h"
#include "QGIViewSymbol.h"
#include "QGIViewClip.h"

#include "ZVALUE.h"
#include "QGVPage.h"

using namespace TechDrawGui;

QGVPage::QGVPage(ViewProviderPage *vp, QWidget *parent)
    : QGraphicsView(parent)
    , pageTemplate(0)
    , m_renderer(Native)
    , drawBkg(true)
    , m_backgroundItem(0)
    , m_outlineItem(0)
    , pageGui(0)
{
    assert(vp);
    pageGui = vp;
    const char* name = vp->getPageObject()->getNameInDocument();
    setObjectName(QString::fromLocal8Bit(name));

    setScene(new QGraphicsScene(this));
    //setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);
    setTransformationAnchor(AnchorUnderMouse);

    setDragMode(ScrollHandDrag);
    setCursor(QCursor(Qt::ArrowCursor));
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    m_backgroundItem = new QGraphicsRectItem();
    m_backgroundItem->setCacheMode(QGraphicsItem::NoCache);
    m_backgroundItem->setZValue(ZVALUE::BACKGROUND);
//     scene()->addItem(m_backgroundItem); // TODO IF SEGFAULTS WITH DRAW ENABLE THIS (REDRAWS ARE SLOWER :s)

    // Prepare background check-board pattern
    QLinearGradient gradient;
    gradient.setStart(0, 0);
    gradient.setFinalStop(0, height());
    gradient.setColorAt(0., QColor(72, 72, 72));
    gradient.setColorAt(1., QColor(150, 150, 150));
    bkgBrush = new QBrush(QColor::fromRgb(70,70,70));

    resetCachedContent();
}
QGVPage::~QGVPage()
{
    delete bkgBrush;
    delete m_backgroundItem;
}

void QGVPage::drawBackground(QPainter *p, const QRectF &)
{
    if(!drawBkg)
        return;

    p->save();
    p->resetTransform();


    p->setBrush(*bkgBrush);
    p->drawRect(viewport()->rect());

    if(!pageGui) {
        return;
    }

    // Default to A3 landscape, though this is currently relevant
    // only for opening corrupt docs, etc.
    float pageWidth = 420,
          pageHeight = 297;

    if ( pageGui->getPageObject()->hasValidTemplate() ) {
        pageWidth = pageGui->getPageObject()->getPageWidth();
        pageHeight = pageGui->getPageObject()->getPageHeight();
    }

    // Draw the white page
    QRectF paperRect(0, -pageHeight, pageWidth, pageHeight);
    QPolygon poly = mapFromScene(paperRect);

    QBrush pageBrush(Qt::white);
    p->setBrush(pageBrush);

    p->drawRect(poly.boundingRect());

    p->restore();

}

int QGVPage::addView(QGIView * view) {
    views.push_back(view);

    // Find if it belongs to a parent
    QGIView *parent = 0;
    parent = findParent(view);

    QPointF viewPos(view->getViewObject()->X.getValue(),
                    view->getViewObject()->Y.getValue() * -1);

    if(parent) {
        // Transfer the child vierw to the parent
        QPointF posRef(0.,0.);

        QPointF mapPos = view->mapToItem(parent, posRef);              //setPos is called later.  this doesn't do anything?
        view->moveBy(-mapPos.x(), -mapPos.y());

        parent->addToGroup(view);
    }

    view->setPos(viewPos);

    return views.size();
}

QGIView * QGVPage::addViewPart(TechDraw::DrawViewPart *part)
{
    QGIViewPart *viewPart = new QGIViewPart(QPoint(0,0), scene());
    viewPart->setViewPartFeature(part);

    addView(viewPart);
    return viewPart;
}

QGIView * QGVPage::addViewSection(TechDraw::DrawViewPart *part)
{
    QGIViewSection *viewSection = new QGIViewSection(QPoint(0,0), scene());
    viewSection->setViewPartFeature(part);

    addView(viewSection);
    return viewSection;
}

QGIView * QGVPage::addProjectionGroup(TechDraw::DrawProjGroup *view) {
    QGIViewCollection *qview = new  QGIProjGroup(QPoint(0,0), scene());
    qview->setViewFeature(view);
    addView(qview);
    return qview;
}

QGIView * QGVPage::addDrawView(TechDraw::DrawView *view)
{
    QGIView *qview = new  QGIView(QPoint(0,0), scene());
    qview->setViewFeature(view);
    addView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewCollection(TechDraw::DrawViewCollection *view)
{
    QGIViewCollection *qview = new  QGIViewCollection(QPoint(0,0), scene());
    qview->setViewFeature(view);
    addView(qview);
    return qview;
}

// TODO change to (App?) annotation object  ??
QGIView * QGVPage::addDrawViewAnnotation(TechDraw::DrawViewAnnotation *view)
{
    // This essentially adds a null view feature to ensure view size is consistent
    QGIViewAnnotation *qview = new  QGIViewAnnotation(QPoint(0,0), this->scene());
    qview->setViewAnnoFeature(view);

    addView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewSymbol(TechDraw::DrawViewSymbol *view)
{
    QPoint qp(view->X.getValue(),view->Y.getValue());
    // This essentially adds a null view feature to ensure view size is consistent
    QGIViewSymbol *qview = new  QGIViewSymbol(qp, scene());
    qview->setViewFeature(view);

    addView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewClip(TechDraw::DrawViewClip *view)
{
    QPoint qp(view->X.getValue(),view->Y.getValue());
    QGIViewClip *qview = new  QGIViewClip(qp, scene());
    qview->setViewFeature(view);

    addView(qview);
    return qview;
}

QGIView * QGVPage::addViewDimension(TechDraw::DrawViewDimension *dim)
{
    QGIViewDimension *dimGroup = new QGIViewDimension(QPoint(0,0), scene());
    dimGroup->setViewPartFeature(dim);

    // TODO consider changing dimension feature to use another property for label position
    // Instead of calling addView - the view must for now be added manually

    //Note dimension X,Y is different from other views -> can't use addView
    views.push_back(dimGroup);

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
    assert(dim);
    assert(parent);          //blow up if we don't have Dimension or Parent
    QPointF posRef(0.,0.);
    QPointF mapPos = dim->mapToItem(parent, posRef);
    dim->moveBy(-mapPos.x(), -mapPos.y());
    parent->addToGroup(dim);
    dim->setZValue(ZVALUE::DIMENSION);
}

QGIView * QGVPage::findView(App::DocumentObject *obj) const
{
  if(scene()) {
    const std::vector<QGIView *> qviews = views;
    for(std::vector<QGIView *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
          TechDraw::DrawView *fview = (*it)->getViewObject();
          if(fview && strcmp(obj->getNameInDocument(), fview->getNameInDocument()) == 0)
              return *it;
      }
  }
    return 0;
}

QGIView * QGVPage::findParent(QGIView *view) const
{
    const std::vector<QGIView *> qviews = views;
    TechDraw::DrawView *myView = view->getViewObject();

    //If type is dimension we check references first
    TechDraw::DrawViewDimension *dim = 0;
    dim = dynamic_cast<TechDraw::DrawViewDimension *>(myView);

    if(dim) {
        std::vector<App::DocumentObject *> objs = dim->References2D.getValues();

        if(objs.size() > 0) {
            std::vector<App::DocumentObject *> objs = dim->References2D.getValues();
            // Attach the dimension to the first object's group
            for(std::vector<QGIView *>::const_iterator it = qviews.begin(); it != qviews.end(); ++it) {
                TechDraw::DrawView *viewObj = (*it)->getViewObject();
                if(strcmp(viewObj->getNameInDocument(), objs.at(0)->getNameInDocument()) == 0) {
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
                    if(strcmp(myView->getNameInDocument(), (*it)->getNameInDocument()) == 0)

                        return grp;
                }
            }
        }
     }

    // Not found a parent
    return 0;
}

void QGVPage::setPageFeature(TechDraw::DrawPage *page)
{
    //redundant
#if 0
    // TODO verify if the pointer should even be used. Not really safe
    pageFeat = page;

    float pageWidth  = pageGui->getPageObject()->getPageWidth();
    float pageHeight = pageGui->getPageObject()->getPageHeight();

    QRectF paperRect(0, -pageHeight, pageWidth, pageHeight);

    QBrush brush(Qt::white);

    m_backgroundItem->setBrush(brush);
    m_backgroundItem->setRect(paperRect);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(10.0);
    shadow->setColor(Qt::white);
    shadow->setOffset(0,0);
    m_backgroundItem->setGraphicsEffect(shadow);

    QRectF myRect = paperRect;
    myRect.adjust(-20,-20,20,20);
    setSceneRect(myRect);
#endif
}

void QGVPage::setPageTemplate(TechDraw::DrawTemplate *obj)
{
    // Remove currently set background template
    // Assign a base template class and create object dependent on
    removeTemplate();

    if(obj->isDerivedFrom(TechDraw::DrawParametricTemplate::getClassTypeId())) {
        //TechDraw::DrawParametricTemplate *dwgTemplate = static_cast<TechDraw::DrawParametricTemplate *>(obj);
        QGIDrawingTemplate *qTempItem = new QGIDrawingTemplate(scene());
        pageTemplate = qTempItem;
    } else if(obj->isDerivedFrom(TechDraw::DrawSVGTemplate::getClassTypeId())) {
        //TechDraw::DrawSVGTemplate *dwgTemplate = static_cast<TechDraw::DrawSVGTemplate *>(obj);
        QGISVGTemplate *qTempItem = new QGISVGTemplate(scene());
        pageTemplate = qTempItem;
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

void QGVPage::setViewBackground(bool enable)
{
    if (!m_backgroundItem)
        return;

    m_backgroundItem->setVisible(enable);
}

void QGVPage::setViewOutline(bool enable)
{
    if (!m_outlineItem)
        return;

    m_outlineItem->setVisible(enable);
}

void QGVPage::toggleEdit(bool enable)
{
// TODO: needs fiddling to handle items that don't inherit QGIViewPart: Annotation, Symbol, Templates, Edges, Faces, Vertices,...
    QList<QGraphicsItem*> list = scene()->items();

    for (QList<QGraphicsItem*>::iterator it = list.begin(); it != list.end(); ++it) {
        QGIView *itemView = dynamic_cast<QGIView *>(*it);
        if(itemView) {
            QGIViewPart *viewPart = dynamic_cast<QGIViewPart *>(*it);
            itemView->setSelected(false);
            if(viewPart) {
                viewPart->toggleCache(enable);
                viewPart->toggleCosmeticLines(enable);
                viewPart->toggleVertices(enable);
                viewPart->toggleBorder(enable);
                setViewBackground(enable);
            } else {
                itemView->toggleBorder(enable);
            }
            //itemView->updateView(true);
        }

        int textItemType = QGraphicsItem::UserType + 160;
        QGraphicsItem*item = dynamic_cast<QGraphicsItem*>(*it);
        if(item) {
            //item->setCacheMode((enable) ? QGraphicsItem::DeviceCoordinateCache : QGraphicsItem::NoCache);
            item->setCacheMode((enable) ? QGraphicsItem::NoCache : QGraphicsItem::NoCache);
            item->update();
            if (item->type() == textItemType) {    //TODO: move this into SVGTemplate or TemplateTextField
                if (enable) {
                    item->show();
                } else {
                    item->hide();
                }
            }
        }
    }
    scene()->update();
    update();
    viewport()->repaint();
}

void QGVPage::saveSvg(QString filename)
{
    // TODO: We only have pageGui because constructor gets passed a view provider...
    TechDraw::DrawPage *page( pageGui->getPageObject() );

    const QString docName( QString::fromUtf8(page->getDocument()->getName()) );
    const QString pageName( QString::fromUtf8(page->getNameInDocument()) );
    QString svgDescription = tr("Drawing page: ") +
                             pageName +
                             tr(" exported from FreeCAD document: ") +
                             docName;

    //Base::Console().Message("TRACE - saveSVG - page width: %d height: %d\n",width,height);    //A4 297x210
    QSvgGenerator svgGen;
    svgGen.setFileName(filename);
    svgGen.setSize(QSize((int) page->getPageWidth(), (int)page->getPageHeight()));
    svgGen.setViewBox(QRect(0, 0, page->getPageWidth(), page->getPageHeight()));
    //TODO: Exported Svg file is not quite right. <svg width="301.752mm" height="213.36mm" viewBox="0 0 297 210"... A4: 297x210
    //      Page too small (A4 vs Letter? margins?)
    //TODO: text in Qt is in mm (actually scene units).  text in SVG is points(?). fontsize in export file is too small by 1/2.835.
    //      resize all textItem before export?
    //      postprocess generated file to mult all font-size attrib by 2.835 to get pts?
    //      duplicate all textItems and only show the appropriate one for screen/print vs export?
    svgGen.setResolution(25.4000508);    // mm/inch??  docs say this is DPI
    //svgGen.setResolution(600);    // resulting page is ~12.5x9mm
    //svgGen.setResolution(96);     // page is ~78x55mm
    svgGen.setTitle(QObject::tr("FreeCAD SVG Export"));
    svgGen.setDescription(svgDescription);

    Gui::Selection().clearSelection();

    toggleEdit(false);             //fiddle cache, cosmetic lines, vertices, etc
    scene()->update();

    Gui::Selection().clearSelection();
    QPainter p;

    p.begin(&svgGen);
    scene()->render(&p);
    p.end();

    toggleEdit(true);
    scene()->update();
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
    qreal factor = std::pow(1.2, -event->delta() / 240.0);
    scale(factor, factor);
    event->accept();
}
void QGVPage::enterEvent(QEvent *event)
{
    QGraphicsView::enterEvent(event);
    viewport()->setCursor(Qt::ArrowCursor);
}

void QGVPage::mousePressEvent(QMouseEvent *event)
{
    QGraphicsView::mousePressEvent(event);
    viewport()->setCursor(Qt::ClosedHandCursor);
}

void QGVPage::mouseReleaseEvent(QMouseEvent *event)
{
    QGraphicsView::mouseReleaseEvent(event);
    viewport()->setCursor(Qt::ArrowCursor);
}

#include "moc_QGVPage.cpp"
