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

#pragma once

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <memory>

#include <QGraphicsView>
#include <QLabel>
#include <QPainterPath>

#include <Base/Type.h>

namespace App
{
class DocumentObject;
}

namespace TechDraw
{
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
}// namespace TechDraw

namespace TechDrawGui
{
class MDIViewPage;
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
class TechDrawHandler;

class TechDrawGuiExport QGVPage: public QGraphicsView
{
    Q_OBJECT

public:
    enum class RendererType
    {
        Native,
        OpenGL,
        Image
    };

    QGVPage(ViewProviderPage* vpPage, QGSPage* scenePage, QWidget* parent = nullptr);
    ~QGVPage() override;

    void setRenderer(RendererType type = RendererType::Native);
    void drawBackground(QPainter* painter, const QRectF& rect) override;

    QGSPage* getScene() { return m_scene; }

    void startBalloonPlacing(TechDraw::DrawView* parent);
    void cancelBalloonPlacing();

    TechDraw::DrawPage* getDrawPage();

    void makeGrid(int width, int height, double step);
    void showGrid(bool state) { m_showGrid = state; }
    void updateViewport() { viewport()->repaint(); }

    void activateHandler(TechDrawHandler* newHandler);
    void deactivateHandler();
    bool isHandlerActive() { return toolHandler != nullptr; }

    bool isBalloonPlacing() const { return balloonPlacing; }
    void setBalloonPlacing(bool isPlacing) { balloonPlacing = isPlacing; }

    QLabel* getBalloonCursor() const { return balloonCursor; }
    void setBalloonCursor(QLabel* label) { balloonCursor = label; }

    void kbPanScroll(int xMove = 1, int yMove = 1);
    QPointF getBalloonCursorPos() const { return balloonCursorPos; }
    void setBalloonCursorPos(QPoint pos) { balloonCursorPos = pos; }

    void activateCursor(QCursor cursor);
    void resetCursor();
    void setPanCursor();
    void setZoomCursor();

    void pseudoContextEvent();

    void centerOnPage();

    TechDraw::DrawView* getBalloonParent() { return m_balloonParent; }

    void zoomIn();
    void zoomOut();

public Q_SLOTS:
    void setHighQualityAntialiasing(bool highQualityAntialiasing);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    void enterEvent(QEvent* event) override;
#else
    void enterEvent(QEnterEvent* event) override;
#endif
    void leaveEvent(QEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

    QColor getBackgroundColor();

    QPixmap prepareCursorPixmap(const char* iconName, QPoint& hotspot);

    void drawForeground(QPainter* painter, const QRectF& rect) override;

    std::string getNavStyleParameter();
    Base::Type getStyleType(std::string model);

    void initNavigationStyle();
    void setNavigationStyle(std::string navParm);

    void createStandardCursors();

private:
    RendererType m_renderer;

    bool drawBkg;
    QBrush* bkgBrush;
    QImage m_image;
    ViewProviderPage* m_vpPage;

    bool m_atCursor;
    bool m_invertZoom;
    double m_zoomIncrement;
    int m_reversePan;
    int m_reverseScroll;

    QGSPage* m_scene;
    bool balloonPlacing;
    QLabel* balloonCursor;
    QPoint balloonCursorPos;
    QPoint balloonHotspot;
    TechDraw::DrawView* m_balloonParent;//temp field. used during balloon placing.

    QPoint panOrigin;

    bool m_showGrid;
    QPainterPath m_gridPath;

    QGVNavStyle* m_navStyle;

    class Private;
    std::unique_ptr<Private> d;

    QCursor panCursor;
    QCursor zoomCursor;

    MDIViewPage* m_parentMDI;
    QContextMenuEvent* m_saveContextEvent;

    std::unique_ptr<TechDrawHandler> toolHandler;
};

}// namespace TechDrawGui