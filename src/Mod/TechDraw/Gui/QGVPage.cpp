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
#include <Mod/TechDraw/App/DrawHatch.h>
#include <Mod/TechDraw/App/DrawViewSpreadsheet.h>


#include "QGIDrawingTemplate.h"
#include "QGITemplate.h"
#include "QGISVGTemplate.h"
#include "TemplateTextField.h"
#include "QGIViewCollection.h"
#include "QGIViewDimension.h"
#include "QGIProjGroup.h"
#include "QGIViewPart.h"
#include "QGIViewSection.h"
#include "QGIViewAnnotation.h"
#include "QGIViewSymbol.h"
#include "QGIViewClip.h"
#include "QGIViewSpreadsheet.h"
#include "QGIFace.h"

#include "ZVALUE.h"
#include "ViewProviderPage.h"
#include "QGVPage.h"

using namespace TechDrawGui;

QGVPage::QGVPage(ViewProviderPage *vp, QGraphicsScene* s, QWidget *parent)
    : QGraphicsView(parent)
    , pageTemplate(0)
    , m_renderer(Native)
    , drawBkg(true)
    , pageGui(0)
{
    assert(vp);
    pageGui = vp;
    const char* name = vp->getPageObject()->getNameInDocument();
    setObjectName(QString::fromLocal8Bit(name));

    setScene(s);
    //setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);
    setTransformationAnchor(AnchorUnderMouse);

    setDragMode(ScrollHandDrag);
    setCursor(QCursor(Qt::ArrowCursor));
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    bkgBrush = new QBrush(QColor::fromRgb(70,70,70));

    resetCachedContent();
}
QGVPage::~QGVPage()
{
    delete bkgBrush;

}

void QGVPage::drawBackground(QPainter *p, const QRectF &)
{
//Note: Background is not part of scene()
    if(!drawBkg)
        return;

    if (!pageGui->getPageObject()) {
        //Base::Console().Log("TROUBLE - QGVP::drawBackground - no Page Object!\n");
        return;
    }

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

int QGVPage::addView(QGIView *view)
{
    auto ourScene( scene() );
    assert(ourScene);

    ourScene->addItem(view);

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
    auto viewPart( new QGIViewPart );

    viewPart->setViewPartFeature(part);

    addView(viewPart);
    return viewPart;
}

QGIView * QGVPage::addViewSection(TechDraw::DrawViewPart *part)
{
    auto viewSection( new QGIViewSection );

    viewSection->setViewPartFeature(part);

    addView(viewSection);
    return viewSection;
}

QGIView * QGVPage::addProjectionGroup(TechDraw::DrawProjGroup *view) {
    auto qview( new QGIProjGroup );

    qview->setViewFeature(view);
    addView(qview);
    return qview;
}

QGIView * QGVPage::addDrawView(TechDraw::DrawView *view)
{
    auto qview( new QGIView );

    qview->setViewFeature(view);
    addView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewCollection(TechDraw::DrawViewCollection *view)
{
    auto qview( new QGIViewCollection );

    qview->setViewFeature(view);
    addView(qview);
    return qview;
}

// TODO change to (App?) annotation object  ??
QGIView * QGVPage::addDrawViewAnnotation(TechDraw::DrawViewAnnotation *view)
{
    // This essentially adds a null view feature to ensure view size is consistent
    auto qview( new QGIViewAnnotation );

    qview->setViewAnnoFeature(view);

    addView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewSymbol(TechDraw::DrawViewSymbol *view)
{
    QPoint qp(view->X.getValue(),view->Y.getValue());
    // This essentially adds a null view feature to ensure view size is consistent
    auto qview( new QGIViewSymbol );

    qview->setViewFeature(view);

    addView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewClip(TechDraw::DrawViewClip *view)
{
    auto qview( new QGIViewClip );

    qview->setPosition(view->X.getValue(), view->Y.getValue());
    qview->setViewFeature(view);

    addView(qview);
    return qview;
}

QGIView * QGVPage::addDrawViewSpreadsheet(TechDraw::DrawViewSpreadsheet *view)
{
    auto qview( new QGIViewSpreadsheet );

    qview->setViewFeature(view);

    addView(qview);
    return qview;
}

QGIView * QGVPage::addViewDimension(TechDraw::DrawViewDimension *dim)
{
    auto dimGroup( new QGIViewDimension );

    auto ourScene( scene() );
    assert(ourScene);
    ourScene->addItem(dimGroup);

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

void QGVPage::setPageTemplate(TechDraw::DrawTemplate *obj)
{
    // Remove currently set background template
    // Assign a base template class and create object dependent on
    removeTemplate();

    if(obj->isDerivedFrom(TechDraw::DrawParametricTemplate::getClassTypeId())) {
        QGIDrawingTemplate *qTempItem = new QGIDrawingTemplate(scene());
        pageTemplate = qTempItem;
    } else if(obj->isDerivedFrom(TechDraw::DrawSVGTemplate::getClassTypeId())) {
        QGISVGTemplate *qTempItem = new QGISVGTemplate(scene(),this);
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

void QGVPage::toggleMarkers(bool enable)
{
    QList<QGraphicsItem*> list = scene()->items();
    for (QList<QGraphicsItem*>::iterator it = list.begin(); it != list.end(); ++it) {
        QGIView *itemView = dynamic_cast<QGIView *>(*it);
        if(itemView) {
            itemView->setSelected(false);
            itemView->toggleBorder(enable);
            QGIViewPart *viewPart = dynamic_cast<QGIViewPart *>(*it);
            if(viewPart) {
                viewPart->toggleVertices(enable);
            }
        }
        QGISVGTemplate* itemTemplate = dynamic_cast<QGISVGTemplate*> (*it);
        if (itemTemplate) {
            std::vector<TemplateTextField *> textFields = itemTemplate->getTestFields();
            for (auto& t:textFields) {
                if (enable) {
                    t->show();
                } else {
                    t->hide();
                }
            }
        }
    }
}

void QGVPage::toggleHatch(bool enable)
{
    QList<QGraphicsItem*> sceneItems = scene()->items();
    for (auto& qgi:sceneItems) {
        QGIViewPart* qgiPart = dynamic_cast<QGIViewPart *>(qgi);
        if(qgiPart) {
            QList<QGraphicsItem*> partChildren = qgiPart->childItems();
            int faceItemType = QGraphicsItem::UserType + 104;
            for (auto& c:partChildren) {
                if (c->type() == faceItemType) {
                    QGIFace* f = dynamic_cast<QGIFace*>(c);
                    f->toggleSvg(enable);
                }
            }
        }
    }
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

// TODO: Was    svgGen.setResolution(25.4000508);    // mm/inch??  docs say this is DPI  //really "user space units/inch"?
    svgGen.setResolution(25);    // mm/inch??  docs say this is DPI

    //svgGen.setResolution(600);    // resulting page is ~12.5x9mm
    //svgGen.setResolution(96);     // page is ~78x55mm
    svgGen.setTitle(QObject::tr("FreeCAD SVG Export"));
    svgGen.setDescription(svgDescription);

    Gui::Selection().clearSelection();

    toggleMarkers(false);             //fiddle cache, vertices, frames, etc
    toggleHatch(false);
    scene()->update();
    viewport()->repaint();

    Gui::Selection().clearSelection();
    QPainter p;

    p.begin(&svgGen);
    scene()->render(&p);
    p.end();

    toggleMarkers(true);
    toggleHatch(true);
    scene()->update();
    viewport()->repaint();
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

TechDraw::DrawPage* QGVPage::getDrawPage()
{
    return pageGui->getPageObject();
}


#include <Mod/TechDraw/Gui/moc_QGVPage.cpp>
