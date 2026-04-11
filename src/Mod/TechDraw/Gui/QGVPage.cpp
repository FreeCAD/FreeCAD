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

#include <cmath>

#include <QApplication>
#include <QBitmap>
#include <QContextMenuEvent>
#include <QLabel>
#include <QMouseEvent>
#include <QOpenGLWidget>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

#include <App/Application.h>
#include <App/Document.h>
#include <Base/Parameter.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/Navigation/NavigationStyle.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawSVGTemplate.h>

#include "MDIViewPage.h"
#include "PreferencesGui.h"
#include "QGSPage.h"
#include "QGVNavStyleBlender.h"
#include "QGVNavStyleCAD.h"
#include "QGVNavStyleGesture.h"
#include "QGVNavStyleInventor.h"
#include "QGVNavStyleMaya.h"
#include "QGVNavStyleOCC.h"
#include "QGVNavStyleOpenSCAD.h"
#include "QGVNavStyleRevit.h"
#include "QGVNavStyleSolidWorks.h"
#include "QGVNavStyleTinkerCAD.h"
#include "QGVNavStyleTouchpad.h"
#include "QGVPage.h"
#include "Rez.h"
#include "TechDrawHandler.h"
#include "ViewProviderPage.h"


// used SVG namespaces
#define CC_NS_URI "http://creativecommons.org/ns#"
#define DC_NS_URI "http://purl.org/dc/elements/1.1/"
#define RDF_NS_URI "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define INKSCAPE_NS_URI "http://www.inkscape.org/namespaces/inkscape"
#define SODIPODI_NS_URI "http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"

using namespace Gui;
using namespace TechDraw;
using namespace TechDrawGui;

class QGVPage::Private: public ParameterGrp::ObserverType
{
public:
    /// handle to the viewer parameter group
    ParameterGrp::handle hGrp;
    QGVPage* page;
    explicit Private(QGVPage* page) : page(page)
    {
        // attach parameter Observer
        hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        hGrp->Attach(this);
    }
    void init()
    {
        page->m_atCursor = hGrp->GetBool("ZoomAtCursor", true);
        page->m_invertZoom = hGrp->GetBool("InvertZoom", false);
        page->m_zoomIncrement = hGrp->GetFloat("ZoomStep", 0.02);

        page->m_reversePan = Preferences::getPreferenceGroup("General")->GetInt("KbPan", 1);
        page->m_reverseScroll = Preferences::getPreferenceGroup("General")->GetInt("KbScroll", 1);
    }
    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override
    {
        const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
        if (strcmp(Reason, "NavigationStyle") == 0) {
            std::string model =
                rGrp.GetASCII("NavigationStyle", CADNavigationStyle::getClassTypeId().getName());
            page->setNavigationStyle(model);
        }
        else if (strcmp(Reason, "InvertZoom") == 0) {
            page->m_invertZoom = rGrp.GetBool("InvertZoom", true);
        }
        else if (strcmp(Reason, "ZoomStep") == 0) {
            page->m_zoomIncrement = rGrp.GetFloat("ZoomStep", 0.0f);
        }
        else if (strcmp(Reason, "ZoomAtCursor") == 0) {
            page->m_atCursor = rGrp.GetBool("ZoomAtCursor", true);
            if (page->m_atCursor) {
                page->setResizeAnchor(QGVPage::AnchorUnderMouse);
                page->setTransformationAnchor(QGVPage::AnchorUnderMouse);
            }
            else {
                page->setResizeAnchor(QGVPage::AnchorViewCenter);
                page->setTransformationAnchor(QGVPage::AnchorViewCenter);
            }
        }
    }
    void detach()
    {
        hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        hGrp->Detach(this);
    }
};

QGVPage::QGVPage(ViewProviderPage* vpPage, QGSPage* scenePage, QWidget* parent)
    : QGraphicsView(parent), m_renderer(RendererType::Native), drawBkg(true), m_vpPage(nullptr),
      m_scene(scenePage), balloonPlacing(false), m_showGrid(false),
      m_navStyle(nullptr), d(new Private(this)), toolHandler(nullptr)
{
    assert(vpPage);
    m_vpPage = vpPage;
    const char* name = vpPage->getDrawPage()->getNameInDocument();
    setObjectName(QString::fromLocal8Bit(name));

    setScene(scenePage);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);

    m_parentMDI = static_cast<MDIViewPage*>(parent);
    m_saveContextEvent = nullptr;

    setCacheMode(QGraphicsView::CacheBackground);
    setRenderer(RendererType::Native);
    //    setRenderer(OpenGL);  //gives rotten quality, don't use this
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    d->init();
    if (m_atCursor) {
        setResizeAnchor(AnchorUnderMouse);
        setTransformationAnchor(AnchorUnderMouse);
    }
    else {
        setResizeAnchor(AnchorViewCenter);
        setTransformationAnchor(AnchorViewCenter);
    }
    setAlignment(Qt::AlignCenter);

    //    setDragMode(ScrollHandDrag);
    setDragMode(QGraphicsView::NoDrag);
    resetCursor();

    bkgBrush = new QBrush(getBackgroundColor());

    balloonCursor = new QLabel(this);
    balloonCursor->setPixmap(
        prepareCursorPixmap("TechDraw_Balloon.svg", balloonHotspot = QPoint(8, 59)));
    balloonCursor->hide();

    initNavigationStyle();

    createStandardCursors();
}

QGVPage::~QGVPage()
{
    delete bkgBrush;
    delete m_navStyle;
    d->detach();
}

void QGVPage::centerOnPage(void) { centerOn(m_vpPage->getQGSPage()->getTemplateCenter()); }

void QGVPage::initNavigationStyle()
{
    std::string navParm = getNavStyleParameter();
    setNavigationStyle(navParm);
}

void QGVPage::setNavigationStyle(std::string navParm)
{
    if (m_navStyle) {
        delete m_navStyle;
    }

    std::size_t foundBlender = navParm.find("Blender");
    std::size_t foundCAD = navParm.find("Gui::CAD");
    std::size_t foundTouchPad = navParm.find("Touchpad");
    std::size_t foundInventor = navParm.find("Inventor");
    std::size_t foundTinker = navParm.find("TinkerCAD");
    std::size_t foundGesture = navParm.find("Gui::Gesture");
    std::size_t foundMaya = navParm.find("Gui::Maya");
    std::size_t foundOCC = navParm.find("OpenCascade");
    std::size_t foundOpenSCAD = navParm.find("OpenSCAD");
    std::size_t foundRevit = navParm.find("Revit");
    std::size_t foundSolidWorks = navParm.find("SolidWorks");

    if (foundBlender != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleBlender(this));
    }
    else if (foundCAD != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleCAD(this));
    }
    else if (foundTouchPad != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleTouchpad(this));
    }
    else if (foundInventor != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleInventor(this));
    }
    else if (foundTinker != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleTinkerCAD(this));
    }
    else if (foundGesture != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleGesture(this));
    }
    else if (foundMaya != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleMaya(this));
    }
    else if (foundOCC != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleOCC(this));
    }
    else if (foundOpenSCAD != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleOpenSCAD(this));
    }
    else if (foundRevit != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleRevit(this));
    }
    else if (foundSolidWorks != std::string::npos) {
        m_navStyle = static_cast<QGVNavStyle*>(new QGVNavStyleSolidWorks(this));
    }
    else {
        m_navStyle = new QGVNavStyle(this);
    }
}


void QGVPage::activateHandler(TechDrawHandler* newHandler)
{
    if (toolHandler) {
        toolHandler->deactivate();
    }

    toolHandler = std::unique_ptr<TechDrawHandler>(newHandler);
    toolHandler->activate(this);
}

void QGVPage::deactivateHandler()
{
    if (toolHandler) {
        toolHandler->deactivate();
        toolHandler = nullptr;
    }
}

void QGVPage::startBalloonPlacing(DrawView* parent)
{
    balloonPlacing = true;
    m_balloonParent = parent;
    activateCursor(
        QCursor(balloonCursor->pixmap(Qt::ReturnByValue), balloonHotspot.x(), balloonHotspot.y()));
}

void QGVPage::cancelBalloonPlacing()
{
    balloonPlacing = false;
    m_balloonParent = nullptr;
    balloonCursor->hide();
    resetCursor();
}

void QGVPage::drawBackground(QPainter* painter, const QRectF&)
{
    //Note: Background is not part of scene()
    if (!drawBkg)
        return;

    if (!m_vpPage) {
        return;
    }

    if (!m_vpPage->getDrawPage()) {
        return;
    }

    painter->save();
    painter->resetTransform();

    painter->setBrush(*bkgBrush);
    painter->drawRect(
        viewport()->rect().adjusted(-2, -2, 2, 2));//just bigger than viewport to prevent artifacts

    painter->restore();
}

void QGVPage::setRenderer(RendererType type)
{
    m_renderer = type;

    if (m_renderer == RendererType::OpenGL) {
#ifndef QT_NO_OPENGL
        setViewport(new QOpenGLWidget);
        setViewportUpdateMode(QGraphicsView::SmartViewportUpdate);
#endif
    }
    else {
        setViewport(new QWidget);
        setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    }
}

void QGVPage::setHighQualityAntialiasing(bool highQualityAntialiasing)
{
#ifndef QT_NO_OPENGL
    setRenderHint(QPainter::Antialiasing, highQualityAntialiasing);
#else
    Q_UNUSED(highQualityAntialiasing);
#endif
}

void QGVPage::paintEvent(QPaintEvent* event)
{
    if (m_renderer == RendererType::Image) {
        if (m_image.size() != viewport()->size()) {
            m_image = QImage(viewport()->size(), QImage::Format_ARGB32_Premultiplied);
        }

        QPainter imagePainter(&m_image);
        QGraphicsView::render(&imagePainter);
        imagePainter.end();

        QPainter p(viewport());
        p.drawImage(0, 0, m_image);
    }
    else {
        QGraphicsView::paintEvent(event);
    }
}

void QGVPage::contextMenuEvent(QContextMenuEvent* event)
{
    if (m_navStyle->allowContextMenu(event)) {
        QGraphicsView::contextMenuEvent(
            event);//this eats the event. mouseReleaseEvent will not be called.
        return;
    }

    //delete the old saved event before creating a new one to avoid memory leak
    //NOTE: saving the actual event doesn't work as the event gets deleted somewhere in Qt
    if (m_saveContextEvent) {
        delete m_saveContextEvent;
    }
    m_saveContextEvent =
        new QContextMenuEvent(QContextMenuEvent::Mouse, event->pos(), event->globalPos());
}

void QGVPage::pseudoContextEvent()
{
    if (m_saveContextEvent) {
        m_parentMDI->contextMenuEvent(m_saveContextEvent);
    }
}

void QGVPage::wheelEvent(QWheelEvent* event)
{
    m_navStyle->handleWheelEvent(event);
    event->accept();
}

void QGVPage::keyPressEvent(QKeyEvent* event)
{
    if (toolHandler) {
        toolHandler->keyPressEvent(event);
    }
    else {
        if (scene() && scene()->focusItem() != nullptr) {
            // The event belongs to the focused item. The base QGraphicsView implementation
            // will handle forwarding it correctly.
            QGraphicsView::keyPressEvent(event);

            // We MUST return here to prevent the navigation style from also
            // processing (and likely consuming) the event.
            return;
        }
        m_navStyle->handleKeyPressEvent(event);
    }
    if (!event->isAccepted()) {
        QGraphicsView::keyPressEvent(event);
    }
}

void QGVPage::keyReleaseEvent(QKeyEvent* event)
{
    if (toolHandler) {
        toolHandler->keyReleaseEvent(event);
    }
    else {
        m_navStyle->handleKeyReleaseEvent(event);
    }
    if (!event->isAccepted()) {
        QGraphicsView::keyReleaseEvent(event);
    }
}

void QGVPage::focusOutEvent(QFocusEvent* event)
{
    Q_UNUSED(event);
    m_navStyle->handleFocusOutEvent(event);
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

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
void QGVPage::enterEvent(QEvent* event)
#else
void QGVPage::enterEvent(QEnterEvent* event)
#endif
{
    if (toolHandler) {
        // if the user interacted with another widget than the mdi, the cursor got unset.
        // So we reapply it.
        toolHandler->updateCursor();
    }
    QGraphicsView::enterEvent(event);
    m_navStyle->handleEnterEvent(event);
    QGraphicsView::enterEvent(event);
}

void QGVPage::leaveEvent(QEvent* event)
{
    m_navStyle->handleLeaveEvent(event);
    QGraphicsView::leaveEvent(event);
}

void QGVPage::mousePressEvent(QMouseEvent* event)
{
    if (toolHandler && (event->button() != Qt::MiddleButton)) {
        toolHandler->mousePressEvent(event);
    }
    else {
        m_navStyle->handleMousePressEvent(event);
    }
    QGraphicsView::mousePressEvent(event);
}

void QGVPage::mouseMoveEvent(QMouseEvent* event)
{
    if (toolHandler) {
        toolHandler->mouseMoveEvent(event);
    }
    m_navStyle->handleMouseMoveEvent(event);
    QGraphicsView::mouseMoveEvent(event);
}

void QGVPage::mouseReleaseEvent(QMouseEvent* event)
{
    if (toolHandler && (event->button() != Qt::MiddleButton)) {
        QGraphicsView::mouseReleaseEvent(event);
        toolHandler->mouseReleaseEvent(event);
    }
    else {
        m_navStyle->handleMouseReleaseEvent(event);
        QGraphicsView::mouseReleaseEvent(event);
        if (toolHandler) {
            toolHandler->updateCursor();
        }
        else {
            resetCursor();
        }
    }
}

TechDraw::DrawPage* QGVPage::getDrawPage() { return m_vpPage->getDrawPage(); }

QColor QGVPage::getBackgroundColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(Preferences::getPreferenceGroup("Colors")->GetUnsigned("Background", 0x70707000));
    return fcColor.asValue<QColor>();
}

QPixmap QGVPage::prepareCursorPixmap(const char* iconName, QPoint& hotspot)
{

    QPointF floatHotspot(hotspot);

    // Due to impossibility to query cursor size via Qt API, we stick to (32x32)
    // as FreeCAD Wiki suggests - see https://wiki.freecad.org/HiDPI_support#Custom_cursor_size
    QPixmap pixmap = Gui::BitmapFactory().pixmapFromSvg(iconName, QSizeF(32, 32));

    // The default (and here expected) SVG cursor graphics size is 64x64 pixels, thus we must adjust
    // the 64x64 based hotspot position for our 32x32 based cursor pixmaps accordingly
    floatHotspot *= 0.5;

#if !defined(Q_OS_WIN32) && !defined(Q_OS_MACOS)
    // On XCB platform, the pixmap device pixel ratio is not taken into account for cursor hot spot,
    // therefore we must take care of the transformation ourselves...
    // Refer to QTBUG-68571 - https://bugreports.qt.io/browse/QTBUG-68571
    if (qGuiApp->platformName() == QLatin1String("xcb")) {
        floatHotspot *= Gui::BitmapFactoryInst::getMaximumDPR();
    }
#endif

    hotspot = floatHotspot.toPoint();
    return pixmap;
}

void QGVPage::activateCursor(QCursor cursor)
{
    this->setCursor(cursor);
    viewport()->setCursor(cursor);
}

void QGVPage::resetCursor()
{
    activateCursor(Qt::ArrowCursor);
}

void QGVPage::setPanCursor() { activateCursor(panCursor); }

void QGVPage::setZoomCursor() { activateCursor(zoomCursor); }

void QGVPage::zoomIn()
{
    m_navStyle->zoomIn();
}

void QGVPage::zoomOut()
{
    m_navStyle->zoomOut();
}

void QGVPage::drawForeground(QPainter* painter, const QRectF& rect)
{
    Q_UNUSED(rect);
    if (m_showGrid) {
        QPen gridPen(PreferencesGui::gridQColor());
        QPen savePen = painter->pen();
        painter->setPen(gridPen);
        painter->drawPath(m_gridPath);
        painter->setPen(savePen);
    }
}

void QGVPage::makeGrid(int gridWidth, int gridHeight, double gridStep)
{
    QPainterPath grid;
    double width = Rez::guiX(gridWidth);
    double height = Rez::guiX(gridHeight);
    double step = Rez::guiX(gridStep);
    double horizStart = 0.0;
    double vPos = 0;
    int rows = (height / step) + 1;
    //draw horizontal lines
    for (int i = 0; i < rows; i++) {
        vPos = i * step;
        QPointF start(horizStart, -vPos);
        QPointF end(width, -vPos);
        grid.moveTo(start);
        grid.lineTo(end);
    }
    //draw vertical lines
    double vertStart = 0.0;
    double hPos = 0.0;
    int cols = (width / step) + 1;
    for (int i = 0; i < cols; i++) {
        hPos = i * step;
        QPointF start(hPos, -vertStart);
        QPointF end(hPos, -height);
        grid.moveTo(start);
        grid.lineTo(end);
    }
    m_gridPath = grid;
}


std::string QGVPage::getNavStyleParameter()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    std::string model =
        hGrp->GetASCII("NavigationStyle", NavigationStyle::getClassTypeId().getName());
    return model;
}

Base::Type QGVPage::getStyleType(std::string model)
{
    Base::Type type = Base::Type::fromName(model.c_str());
    return type;
}

void QGVPage::createStandardCursors()
{
    QPixmap panPixmap = BitmapFactory().pixmapFromSvg("cursor-pan", QSize(16,16));
    QPixmap zoomPixmap = BitmapFactory().pixmapFromSvg("cursor-zoom", QSize(16,16));

    this->panCursor = QCursor(panPixmap, 8, 8);
    this->zoomCursor = QCursor(zoomPixmap, 8, 8);
}

#include <Mod/TechDraw/Gui/moc_QGVPage.cpp>
