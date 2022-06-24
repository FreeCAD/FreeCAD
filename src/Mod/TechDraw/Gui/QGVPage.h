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

#include <QGraphicsView>
#include <QLabel>
#include <QPainterPath>

#include <Base/Type.h>
#include <Base/Parameter.h>

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
class QGSPage;
class QGIView;
class QGIViewDimension;
class QGITemplate;
class ViewProviderPage;
class QGIViewBalloon;
class QGILeaderLine;
class QGIRichAnno;
class QGITile;
class QGVNavStyle;

class TechDrawGuiExport QGVPage : public QGraphicsView, public ParameterGrp::ObserverType
{
    Q_OBJECT

public:
    enum RendererType { Native, OpenGL, Image };

    QGVPage(ViewProviderPage *vp, QGSPage* s, QWidget *parent = nullptr);
    virtual ~QGVPage();

    /// Observer message from the ParameterGrp
    virtual void OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason) override;


    void setRenderer(RendererType type = Native);
    void drawBackground(QPainter *p, const QRectF &rect) override;

    QGSPage* getScene() {return m_scene; };

    void startBalloonPlacing(void);
    void cancelBalloonPlacing(void);

    TechDraw::DrawPage * getDrawPage();

    void setExporting(bool enable);

    /// Renders the page to SVG with filename.
//    void saveSvg(QString filename);
//    void postProcessXml(QTemporaryFile& tempFile, QString filename, QString pagename);

    void makeGrid(int width, int height, double step);
    void showGrid(bool state) {m_showGrid = state;}
    void updateViewport(void) {viewport()->repaint();}

    bool isBalloonPlacing() {return balloonPlacing; };
    void setBalloonPlacing(bool s) {balloonPlacing = s;};

    QLabel* getBalloonCursor() {return balloonCursor;};
    void setBalloonCursor(QLabel* l) {balloonCursor = l;};

    void kbPanScroll(int xMove = 1, int yMove = 1);
    QPointF getBalloonCursorPos() {return balloonCursorPos;};
    void setBalloonCursorPos(QPoint p) { balloonCursorPos = p;};

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
    void keyReleaseEvent(QKeyEvent *event) override;
    virtual void contextMenuEvent(QContextMenuEvent *event) override;

    static QColor SelectColor;
    static QColor PreselectColor;
    QColor getBackgroundColor();

    double getDevicePixelRatio() const;
    QPixmap prepareCursorPixmap(const char *iconName, QPoint &hotspot);

    void activateCursor(QCursor cursor);
    void resetCursor();
    virtual void drawForeground(QPainter *painter, const QRectF &rect) override;
    
    std::string getNavStyleParameter();
    Base::Type getStyleType(std::string model);

    void initNavigationStyle();
    void setNavigationStyle(std::string navParm);

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

    QGSPage* m_scene;
    bool balloonPlacing;
    QLabel *balloonCursor;
    QPoint balloonCursorPos;
    QPoint balloonHotspot;

    QPoint panOrigin;
    bool panningActive;

    bool m_showGrid;
    QPainterPath m_gridPath;

    QGVNavStyle* m_navStyle;

    /// handle to the viewer parameter group
    ParameterGrp::handle hGrp;

};

} // namespace 

#endif // TECHDRAWGUI_QGVIEW_H
