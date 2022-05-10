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

#ifndef TECHDRAWGUI_QGVIEW_H
#define TECHDRAWGUI_QGVIEW_H

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <QGraphicsScene>
#include <QGraphicsView>
#include <QLabel>
#include <QPainterPath>

class QTemporaryFile;

namespace App {
class DocumentObject;
}

namespace TechDraw {
class DrawView;
class DrawViewPart;
class DrawProjGroup;
class DrawViewDimension;
class DrawPage;
class DrawTemplate;
class DrawViewAnnotation;
class DrawViewSymbol;
class DrawViewClip;
class DrawViewCollection;
class DrawViewSpreadsheet;
class DrawViewImage;
class DrawLeaderLine;
class DrawViewBalloon;
class DrawRichAnno;
class DrawWeldSymbol;
}

namespace TechDrawGui
{
class QGIView;
class QGIViewDimension;
class QGITemplate;
class ViewProviderPage;
class QGIViewBalloon;
class QGILeaderLine;
class QGIRichAnno;
class QGITile;

class TechDrawGuiExport QGVPage : public QGraphicsView
{
    Q_OBJECT

public:
    enum RendererType { Native, OpenGL, Image };

    QGVPage(ViewProviderPage *vp, QGraphicsScene* s, QWidget *parent = nullptr);
    virtual ~QGVPage();

    void setRenderer(RendererType type = Native);
    void drawBackground(QPainter *p, const QRectF &rect) override;

    QGIView * addViewDimension(TechDraw::DrawViewDimension *dim);
    QGIView * addViewBalloon(TechDraw::DrawViewBalloon *balloon);
    QGIView * addProjectionGroup(TechDraw::DrawProjGroup *view);
    QGIView * addViewPart(TechDraw::DrawViewPart *part);
    QGIView * addViewSection(TechDraw::DrawViewPart *part);
    QGIView * addDrawView(TechDraw::DrawView *view);
    QGIView * addDrawViewCollection(TechDraw::DrawViewCollection *view);
    QGIView * addDrawViewAnnotation(TechDraw::DrawViewAnnotation *view);
    QGIView * addDrawViewSymbol(TechDraw::DrawViewSymbol *view);
    QGIView * addDrawViewClip(TechDraw::DrawViewClip *view);
    QGIView * addDrawViewSpreadsheet(TechDraw::DrawViewSpreadsheet *view);
    QGIView * addDrawViewImage(TechDraw::DrawViewImage *view);
    QGIView * addViewLeader(TechDraw::DrawLeaderLine* view);
    QGIView * addRichAnno(TechDraw::DrawRichAnno* anno);
    QGIView * addWeldSymbol(TechDraw::DrawWeldSymbol* weld);

    QGIView* findQViewForDocObj(App::DocumentObject *obj) const;
    QGIView* getQGIVByName(std::string name);
    QGIView* findParent(QGIView *) const;

    void addBalloonToParent(QGIViewBalloon* balloon, QGIView* parent);
    void createBalloon(QPointF origin, TechDraw::DrawViewPart *parent);
    void startBalloonPlacing(void);
    void cancelBalloonPlacing(void);

    void addDimToParent(QGIViewDimension* dim, QGIView* parent);
    void addLeaderToParent(QGILeaderLine* lead, QGIView* parent);

    std::vector<QGIView *> getViews() const;

    int addQView(QGIView * view);
    int removeQView(QGIView *view);
    int removeQViewByName(const char* name);
    void removeQViewFromScene(QGIView *view);

    void setPageTemplate(TechDraw::DrawTemplate *pageTemplate);

    QGITemplate * getTemplate() const;
    void removeTemplate();

    TechDraw::DrawPage * getDrawPage();

    void setExporting(bool enable);
    virtual void refreshViews(void);


    /// Renders the page to SVG with filename.
    void saveSvg(QString filename);
    void postProcessXml(QTemporaryFile& tempFile, QString filename, QString pagename);

    void makeGrid(int width, int height, double step);
    void showGrid(bool state) {m_showGrid = state;}
    void updateViewport(void) {viewport()->repaint();}

public Q_SLOTS:
    void setHighQualityAntialiasing(bool highQualityAntialiasing);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void kbPanScroll(int xMove = 1, int yMove = 1); 

    static QColor SelectColor;
    static QColor PreselectColor;
    QColor getBackgroundColor();
    

    QGITemplate *pageTemplate;

    double getDevicePixelRatio() const;
    QPixmap prepareCursorPixmap(const char *iconName, QPoint &hotspot);

    void activateCursor(QCursor cursor);
    void resetCursor();
    virtual void drawForeground(QPainter *painter, const QRectF &rect) override;

private:
    RendererType m_renderer;

    bool drawBkg;
    QBrush* bkgBrush;
    QImage m_image;
    ViewProviderPage *m_vpPage;
    
    bool m_atCursor;
    bool m_invertZoom;
    double m_zoomIncrement;
    int m_reversePan;
    int m_reverseScroll;

    bool balloonPlacing;
    QLabel *balloonCursor;
    QPoint balloonCursorPos;
    QPoint balloonHotspot;

    QPoint panOrigin;
    bool panningActive;

    bool m_showGrid;
    QPainterPath m_gridPath;
};

} // namespace 

#endif // TECHDRAWGUI_QGVIEW_H
