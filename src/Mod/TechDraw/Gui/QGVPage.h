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

#ifndef DRAWINGGUI_CANVASVIEW_H
#define DRAWINGGUI_CANVASVIEW_H

#include <QGraphicsView>

#include "ViewProviderPage.h"

namespace TechDraw {
class DrawViewPart;
class DrawProjGroup;
class DrawViewDimension;
class DrawPage;
class DrawTemplate;
class DrawViewAnnotation;
class DrawViewSymbol;
class DrawViewClip;
class DrawHatch;
class DrawViewCollection;
class DrawViewSpreadsheet;
}

namespace TechDrawGui
{
class QGIView;
class QGIViewDimension;
class QGITemplate;
class QGIHatch;

class TechDrawGuiExport QGVPage : public QGraphicsView
{
    Q_OBJECT

public:
    enum RendererType { Native, OpenGL, Image };

    QGVPage(ViewProviderPage *vp, QWidget *parent = 0);
    ~QGVPage();

    void setRenderer(RendererType type = Native);
    void drawBackground(QPainter *p, const QRectF &rect);

    QGIView * addViewDimension(TechDraw::DrawViewDimension *dim);
    QGIView * addProjectionGroup(TechDraw::DrawProjGroup *view);
    QGIView * addViewPart(TechDraw::DrawViewPart *part);
    QGIView * addViewSection(TechDraw::DrawViewPart *part);
    QGIView * addDrawView(TechDraw::DrawView *view);
    QGIView * addDrawViewCollection(TechDraw::DrawViewCollection *view);
    QGIView * addDrawViewAnnotation(TechDraw::DrawViewAnnotation *view);
    QGIView * addDrawViewSymbol(TechDraw::DrawViewSymbol *view);
    QGIView * addDrawViewClip(TechDraw::DrawViewClip *view);
    QGIView * addDrawViewSpreadsheet(TechDraw::DrawViewSpreadsheet *view);

    QGIView * findView(App::DocumentObject *obj) const;
    QGIView * findParent(QGIView *) const;

    void addDimToParent(QGIViewDimension* dim, QGIView* parent);
    const std::vector<QGIView *> & getViews() const { return views; }
    int addView(QGIView * view);
    void setViews(const std::vector<QGIView *> &view) {views = view; }
    void setPageFeature(TechDraw::DrawPage *page);
    void setPageTemplate(TechDraw::DrawTemplate *pageTemplate);

    QGITemplate * getTemplate() const;
    void removeTemplate();

    /// Getter for DrawPage feature
    TechDraw::DrawPage * getDrawPage() { return pageGui->getPageObject(); }

    void toggleEdit(bool enable);

    /// Renders the page to SVG with filename.
    void saveSvg(QString filename);

public Q_SLOTS:
    void setHighQualityAntialiasing(bool highQualityAntialiasing);
    void setViewBackground(bool enable);
    void setViewOutline(bool enable);

protected:
    void wheelEvent(QWheelEvent *event);
    void paintEvent(QPaintEvent *event);
    void enterEvent(QEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

    static QColor SelectColor;
    static QColor PreselectColor;

    QGITemplate *pageTemplate;
    std::vector<QGIView *> views;

private:
    RendererType m_renderer;

    bool drawBkg;
    QGraphicsRectItem *m_backgroundItem;
    QGraphicsRectItem *m_outlineItem;
    QBrush *bkgBrush;
    QImage m_image;
    ViewProviderPage *pageGui;
};

} // namespace MDIViewPageGui

#endif // DRAWINGGUI_CANVASVIEW_H
