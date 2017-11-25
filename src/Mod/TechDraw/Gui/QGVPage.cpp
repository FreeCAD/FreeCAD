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
# include <cmath>
#endif

#include <App/Application.h>
#include <App/Material.h>
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
#include <Mod/TechDraw/App/DrawViewImage.h>

#include "Rez.h"
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
#include "QGIViewImage.h"
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
    , m_vpPage(0)
{
    assert(vp);
    m_vpPage = vp;
    const char* name = vp->getDrawPage()->getNameInDocument();
    setObjectName(QString::fromLocal8Bit(name));

    setScene(s);

    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    //setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    //setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
    setCacheMode(QGraphicsView::CacheBackground);
    //setTransformationAnchor(AnchorUnderMouse);
    //setTransformationAnchor(NoAnchor);
    setTransformationAnchor(AnchorViewCenter);
    setResizeAnchor(AnchorViewCenter);
    setAlignment(Qt::AlignCenter);

    setDragMode(ScrollHandDrag);
    setCursor(QCursor(Qt::ArrowCursor));
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    bkgBrush = new QBrush(getBackgroundColor());

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

    if (!m_vpPage->getDrawPage()) {
        //Base::Console().Log("TROUBLE - QGVP::drawBackground - no Page Object!\n");
        return;
    }

    p->save();
    p->resetTransform();


    p->setBrush(*bkgBrush);
    p->drawRect(viewport()->rect().adjusted(-2,-2,2,2));   //just bigger than viewport to prevent artifacts

    if(!m_vpPage) {
        return;
    }

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
        removeQViewFromScene(ourItem);
        delete ourItem;
    }

    return 0;
}

void QGVPage::removeQViewFromScene(QGIView *view)
{
    QGraphicsItemGroup* grp = view->group();
    if (grp) {
        grp->removeFromGroup(view);
    }

    if (view->parentItem()) {    //not top level
        view->setParentItem(0);
    }

    if (view->scene()) {
        view->scene()->removeItem(view);
    }
}


QGIView * QGVPage::addViewPart(TechDraw::DrawViewPart *part)
{
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
    assert(dim);
    assert(parent);          //blow up if we don't have Dimension or Parent
    QPointF posRef(0.,0.);
    QPointF mapPos = dim->mapToItem(parent, posRef);
    dim->moveBy(-mapPos.x(), -mapPos.y());
    parent->addToGroup(dim);
    dim->setZValue(ZVALUE::DIMENSION);
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


QGIView * QGVPage::findParent(QGIView *view) const
{
    const std::vector<QGIView *> qviews = getViews();
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
                if(strcmp((*it)->getViewName(), objs.at(0)->getNameInDocument()) == 0) {
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
            std::vector<TemplateTextField *> textFields = itemTemplate->getTextFields();
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
                    static_cast<QGIFace*>(c)->toggleSvg(enable);
                }
            }
        }
    }
}

void QGVPage::saveSvg(QString filename)
{
    // TODO: We only have m_vpPage because constructor gets passed a view provider...
    //NOTE: this makes wrong size pages in low-Rez
    TechDraw::DrawPage *page( m_vpPage->getDrawPage() );

    const QString docName( QString::fromUtf8(page->getDocument()->getName()) );
    const QString pageName( QString::fromUtf8(page->getNameInDocument()) );
    QString svgDescription = tr("Drawing page: ") +
                             pageName +
                             tr(" exported from FreeCAD document: ") +
                             docName;


    QSvgGenerator svgGen;
    svgGen.setFileName(filename);
    svgGen.setSize(QSize((int) Rez::guiX(page->getPageWidth()), (int) Rez::guiX(page->getPageHeight())));   //expects pixels, gets mm
    //"By default this property is set to QSize(-1, -1), which indicates that the generator should not output 
    // the width and height attributes of the <svg> element."  >> but Inkscape won't read it without size info??
    svgGen.setViewBox(QRect(0, 0, Rez::guiX(page->getPageWidth()), Rez::guiX(page->getPageHeight())));
    
    svgGen.setResolution(Rez::guiX(25.4));    // docs say this is DPI. 1dot/mm so 25.4dpi

    svgGen.setTitle(QObject::tr("FreeCAD SVG Export"));
    svgGen.setDescription(svgDescription);

    Gui::Selection().clearSelection();

    toggleMarkers(false);             //fiddle cache, vertices, frames, etc
    toggleHatch(false);
    scene()->update();
    viewport()->repaint();

    double width  =  Rez::guiX(page->getPageWidth());
    double height =  Rez::guiX(page->getPageHeight());
    QRectF sourceRect(0.0,-height,width,height);
    QRectF targetRect;

    Gui::Selection().clearSelection();
    QPainter p;

    p.begin(&svgGen);
    scene()->render(&p, targetRect,sourceRect);
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
//Delta is the distance that the wheel is rotated, in eighths of a degree.
//positive indicates rotation forwards away from the user; negative backwards toward the user.
//Most mouse types work in steps of 15 degrees, in which case the delta value is a multiple of 120; i.e., 120 units * 1/8 = 15 degrees.
//1 click = 15 degrees.  15 degrees = 120 deltas.  delta/240 -> 1 click = 0.5 ==> factor = 1.2^0.5 = 1.095
//                                                              1 click = -0.5 ==> factor = 1.2^-0.5 = 0.91
//so to change wheel direction, multiply (event->delta() / 240.0) by +/-1
    double mouseBase = 1.2;        //magic numbers. change for different mice?
    double mouseAdjust = 240.0;

    QPointF center = mapToScene(viewport()->rect().center());
    qreal factor = std::pow(mouseBase, event->delta() / mouseAdjust);
    scale(factor, factor);

    QPointF newCenter = mapToScene(viewport()->rect().center());
    QPointF change = newCenter - center;
    translate(change.x(), change.y());

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
