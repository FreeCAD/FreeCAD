/***************************************************************************
 *   Copyright (c) 2007 Jürgen Riegel <juergen.riegel@web.de>              *
 *                                                                         *
 *   This file is Drawing of the FreeCAD CAx development system.           *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A DrawingICULAR PURPOSE.  See the      *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef DRAWINGGUI_DRAWINGVIEW_H
#define DRAWINGGUI_DRAWINGVIEW_H

#include <QGraphicsView>
#include <QPrinter>

#include <Gui/MDIView.h>
#include <Mod/Drawing/DrawingGlobal.h>


QT_BEGIN_NAMESPACE
class QSlider;
class QAction;
class QActionGroup;
class QFile;
class QPopupMenu;
class QToolBar;
class QSvgWidget;
class QScrollArea;
class QPrinter;
QT_END_NAMESPACE

namespace DrawingGui
{

class DrawingGuiExport SvgView: public QGraphicsView
{
    Q_OBJECT

public:
    enum RendererType
    {
        Native,
        OpenGL,
        Image
    };

    SvgView(QWidget* parent = nullptr);

    void openFile(const QFile& file);
    void setRenderer(RendererType type = Native);
    void drawBackground(QPainter* p, const QRectF& rect);
    void setZoomInverted(bool on)
    {
        m_invertZoom = on;
    }

public Q_SLOTS:
    void setHighQualityAntialiasing(bool highQualityAntialiasing);
    void setViewBackground(bool enable);
    void setViewOutline(bool enable);

protected:
    void wheelEvent(QWheelEvent* event);
    void paintEvent(QPaintEvent* event);

private:
    RendererType m_renderer;

    QGraphicsItem* m_svgItem;
    QGraphicsRectItem* m_backgroundItem;
    QGraphicsRectItem* m_outlineItem;

    QImage m_image;
    bool m_invertZoom;
};

class DrawingGuiExport DrawingView: public Gui::MDIView
{
    Q_OBJECT

public:
    DrawingView(Gui::Document* doc, QWidget* parent = nullptr);
    virtual ~DrawingView();

public Q_SLOTS:
    void load(const QString& path = QString());
    void setRenderer(QAction* action);
    void viewAll();

public:
    bool onMsg(const char* pMsg, const char** ppReturn);
    bool onHasMsg(const char* pMsg) const;
    void onRelabel(Gui::Document* pDoc);
    void print();
    void printPdf();
    void printPreview();
    void print(QPrinter* printer);
    void setDocumentObject(const std::string&);
    PyObject* getPyObject();

protected:
    void contextMenuEvent(QContextMenuEvent* event);
    void closeEvent(QCloseEvent*);
    void findPrinterSettings(const QString&);
    QPageSize::PageSizeId getPageSize(int w, int h) const;

private:
    QAction* m_nativeAction;
    QAction* m_glAction;
    QAction* m_imageAction;
    QAction* m_highQualityAntialiasingAction;
    QAction* m_backgroundAction;
    QAction* m_outlineAction;

    SvgView* m_view;
    std::string m_objectName;

    QString m_currentPath;
    QPageLayout::Orientation m_orientation;
    QPageSize::PageSizeId m_pageSize;
};

}  // namespace DrawingGui

#endif  // DRAWINGGUI_DRAWINGVIEW_H
