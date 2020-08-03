/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QPointer>
# include <QPainter>
# include <QDockWidget>
# include <QMdiArea>
# include <QTabBar>
# include <QTreeView>
# include <QHeaderView>
# include <QToolTip>
# include <QAction>
# include <QKeyEvent>
# include <QMap>
# include <QTextStream>
# include <QComboBox>
# include <QBoxLayout>
# include <QSpacerItem>
# include <QSplitter>
# include <QStackedWidget>
#endif

#if QT_VERSION >= 0x050000
# include <QWindow>
#endif

#include <QPropertyAnimation>

#include <array>

#include <Base/Tools.h>
#include <Base/Console.h>
#include "DockWindowManager.h"
#include "MainWindow.h"
#include "ViewParams.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "SplitView3DInventor.h"
#include "Application.h"
#include "Control.h"
#include "TaskView/TaskView.h"
#include "Tree.h"
#include <App/Application.h>
#include "propertyeditor/PropertyEditor.h"

FC_LOG_LEVEL_INIT("Dock", true, true);

using namespace Gui;

DockWindowItems::DockWindowItems()
{
}

DockWindowItems::~DockWindowItems()
{
}

void DockWindowItems::addDockWidget(const char* name, Qt::DockWidgetArea pos, bool visibility, bool tabbed)
{
    DockWindowItem item;
    item.name = QString::fromLatin1(name);
    item.pos = pos;
    item.visibility = visibility;
    item.tabbed = tabbed;
    _items << item;
}

void DockWindowItems::setDockingArea(const char* name, Qt::DockWidgetArea pos)
{
    for (QList<DockWindowItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        if (it->name == QLatin1String(name)) {
            it->pos = pos;
            break;
        }
    }
}

void DockWindowItems::setVisibility(const char* name, bool v)
{
    for (QList<DockWindowItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        if (it->name == QLatin1String(name)) {
            it->visibility = v;
            break;
        }
    }
}

void DockWindowItems::setVisibility(bool v)
{
    for (QList<DockWindowItem>::iterator it = _items.begin(); it != _items.end(); ++it) {
        it->visibility = v;
    }
}

const QList<DockWindowItem>& DockWindowItems::dockWidgets() const
{
    return this->_items;
}

// -----------------------------------------------------------

#ifdef FC_HAS_DOCK_OVERLAY

static OverlayTabWidget *_LeftOverlay;
static OverlayTabWidget *_RightOverlay;
static OverlayTabWidget *_TopOverlay;
static OverlayTabWidget *_BottomOverlay;

static const int _TitleButtonSize = 12;

#define TITLE_BUTTON_COLOR "# c #202020"

static const char *_PixmapOverlay[]={
    "10 10 2 1",
    ". c None",
    TITLE_BUTTON_COLOR,
    "##########",
    "#........#",
    "#........#",
    "##########",
    "#........#",
    "#........#",
    "#........#",
    "#........#",
    "#........#",
    "##########",
};

// -----------------------------------------------------------

OverlayProxyWidget::OverlayProxyWidget(OverlayTabWidget *tabOverlay)
    :QWidget(tabOverlay->parentWidget()), owner(tabOverlay), _hintColor(QColor(50,50,50,150))
{
    dockArea = owner->getDockArea();
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(onTimer()));
}

bool OverlayProxyWidget::isActivated() const
{
    return drawLine && isVisible();
}

bool OverlayProxyWidget::hitTest(QPoint pt, bool delay)
{
    if (!isVisible())
        return false;

    QTabBar *tabbar = owner->tabBar();
    if (tabbar->isVisible() && tabbar->tabAt(tabbar->mapFromGlobal(pt))>=0)
        return true;

    pt = mapFromGlobal(pt);
    int hit = 0;
    QSize s = this->size();
    int hintSize = ViewParams::getDockOverlayHintTriggerSize();
    switch(dockArea) {
    case Qt::LeftDockWidgetArea:
        hit = (pt.y() >= 0 && pt.y() <= s.height() && pt.x() > 0 && pt.x() < hintSize);
        if (hit && pt.x() <= s.width())
            hit = 2;
        break;
    case Qt::RightDockWidgetArea:
        hit = (pt.y() >= 0 && pt.y() <= s.height() && pt.x() < s.width() && pt.x() > -hintSize);
        if (hit && pt.x() >= 0)
            hit = 2;
        break;
    case Qt::TopDockWidgetArea:
        hit = (pt.x() >= 0 && pt.x() <= s.width() && pt.y() > 0 && pt.y() < hintSize);
        if (hit && pt.y() <= s.height())
            hit = 2;
        break;
    case Qt::BottomDockWidgetArea:
        hit = (pt.x() >= 0 && pt.x() <= s.width() && pt.y() < s.height() && pt.y() > -hintSize);
        if (hit && pt.y() >= 0)
            hit = 2;
        break;
    }
    if (hit) {
        if (drawLine)
            timer.stop();
        else if (delay) {
            if (!timer.isActive())
                timer.start(ViewParams::getDockOverlayHintDelay());
        } else {
            timer.stop();
            owner->setState(OverlayTabWidget::State_Hint);
            drawLine = true;
            update();
        }
        if(hit > 1 && ViewParams::getDockOverlayActivateOnHover()) {
            if (owner->isVisible() && owner->tabBar()->isVisible()) {
                QSize size = owner->tabBar()->size();
                QPoint pt = owner->tabBar()->mapToGlobal(
                                QPoint(size.width(), size.height()));
                QPoint pos = QCursor::pos();
                switch(this->dockArea) {
                case Qt::LeftDockWidgetArea:
                case Qt::RightDockWidgetArea:
                    if (pos.y() < pt.y())
                        return false;
                    break;
                case Qt::TopDockWidgetArea:
                case Qt::BottomDockWidgetArea:
                    if (pos.x() < pt.x())
                        return false;
                    break;
                default:
                    break;
                }
            }
            owner->setState(OverlayTabWidget::State_Normal);
            DockWindowManager::instance()->refreshOverlay();
        }

    } else if (!drawLine)
        timer.stop();
    else if (delay) {
        if (!timer.isActive())
            timer.start(ViewParams::getDockOverlayHintDelay());
    } else {
        timer.stop();
        owner->setState(OverlayTabWidget::State_Normal);
        drawLine = false;
        update();
    }
    return hit;
}

void OverlayProxyWidget::onTimer()
{
    hitTest(QCursor::pos(), false);
}

void OverlayProxyWidget::enterEvent(QEvent *)
{
    if(!owner->count())
        return;

    if (!drawLine) {
        if (!timer.isActive())
            timer.start(ViewParams::getDockOverlayHintDelay());
    }
}

void OverlayProxyWidget::leaveEvent(QEvent *)
{
    // drawLine = false;
    // update();
}

void OverlayProxyWidget::hideEvent(QHideEvent *)
{
    drawLine = false;
}

void OverlayProxyWidget::mousePressEvent(QMouseEvent *ev)
{
    if(!owner->count() || ev->button() != Qt::LeftButton)
        return;

    owner->setState(OverlayTabWidget::State_Normal);
    DockWindowManager::instance()->refreshOverlay(this);
}

QBrush OverlayProxyWidget::hintColor() const
{
    return _hintColor;
}

void OverlayProxyWidget::setHintColor(const QBrush &brush)
{
    _hintColor = brush;
}

void OverlayProxyWidget::paintEvent(QPaintEvent *)
{
    if(!drawLine)
        return;
    QPainter painter(this);
    painter.setOpacity(_hintColor.color().alphaF());
    painter.setPen(Qt::transparent);
    painter.setBrush(_hintColor);

    QRect rect = this->rect();
    if (owner->isVisible() && owner->tabBar()->isVisible()) {
        QSize size = owner->tabBar()->size();
        QPoint pt = owner->tabBar()->mapToGlobal(
                        QPoint(size.width(), size.height()));
        pt = this->mapFromGlobal(pt);
        switch(this->dockArea) {
            case Qt::LeftDockWidgetArea:
            case Qt::RightDockWidgetArea:
                rect.setTop(pt.y());
                break;
            case Qt::TopDockWidgetArea:
            case Qt::BottomDockWidgetArea:
                rect.setLeft(pt.x());
                break;
            default:
                break;
        }
    }
    painter.drawRect(rect);
}

OverlayToolButton::OverlayToolButton(QWidget *parent)
    :QToolButton(parent)
{}

OverlayTabWidget::OverlayTabWidget(QWidget *parent, Qt::DockWidgetArea pos)
    :QTabWidget(parent), dockArea(pos)
{
    // This is necessary to capture any focus lost from switching the tab,
    // otherwise the lost focus will leak to the parent, i.e. MdiArea, which may
    // cause unexpected Mdi sub window switching.
    setFocusPolicy(Qt::StrongFocus);

    splitter = new QSplitter(this);

    _graphicsEffect = new OverlayGraphicsEffect(splitter);
    splitter->setGraphicsEffect(_graphicsEffect);

    _graphicsEffectTab = new OverlayGraphicsEffect(this);
    _graphicsEffectTab->setEnabled(false);
    tabBar()->setGraphicsEffect(_graphicsEffectTab);

    switch(pos) {
    case Qt::LeftDockWidgetArea:
        _LeftOverlay = this;
        setTabPosition(QTabWidget::West);
        splitter->setOrientation(Qt::Vertical);
        break;
    case Qt::RightDockWidgetArea:
        _RightOverlay = this;
        setTabPosition(QTabWidget::East);
        splitter->setOrientation(Qt::Vertical);
        break;
    case Qt::TopDockWidgetArea:
        _TopOverlay = this;
        setTabPosition(QTabWidget::North);
        splitter->setOrientation(Qt::Horizontal);
        break;
    case Qt::BottomDockWidgetArea:
        _BottomOverlay = this;
        setTabPosition(QTabWidget::South);
        splitter->setOrientation(Qt::Horizontal);
        break;
    default:
        break;
    }

    proxyWidget = new OverlayProxyWidget(this);
    proxyWidget->hide();
    _setOverlayMode(proxyWidget,true);

    setOverlayMode(true);
    hide();

    static QIcon pxTransparent;
    if(pxTransparent.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "..........",
            "...####...",
            ".##....##.",
            "##..##..##",
            "#..####..#",
            "#..####..#",
            "##..##..##",
            ".##....##.",
            "...####...",
            "..........",
        };
        pxTransparent = QIcon(QPixmap(bytes));
    }
    actTransparent.setIcon(pxTransparent);
    actTransparent.setCheckable(true);
    actTransparent.setData(QString::fromLatin1("OBTN Transparent"));
    actTransparent.setParent(this);
    addAction(&actTransparent);

    QPixmap pxAutoHide;
    if(pxAutoHide.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "...#######",
            ".........#",
            "..##.....#",
            ".##......#",
            "#######..#",
            "#######..#",
            ".##......#",
            "..##.....#",
            ".........#",
            "...#######",
        };
        pxAutoHide = QPixmap(bytes);
    }
    switch(dockArea) {
    case Qt::LeftDockWidgetArea:
        actAutoHide.setIcon(pxAutoHide);
        break;
    case Qt::RightDockWidgetArea:
        actAutoHide.setIcon(pxAutoHide.transformed(QTransform().scale(-1,1)));
        break;
    case Qt::TopDockWidgetArea:
        actAutoHide.setIcon(pxAutoHide.transformed(QTransform().rotate(90)));
        break;
    case Qt::BottomDockWidgetArea:
        actAutoHide.setIcon(pxAutoHide.transformed(QTransform().rotate(-90)));
        break;
    default:
        break;
    }
    actAutoHide.setCheckable(true);
    actAutoHide.setData(QString::fromLatin1("OBTN AutoHide"));
    actAutoHide.setParent(this);
    addAction(&actAutoHide);

    static QIcon pxEditHide;
    if(pxEditHide.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "##....##..",
            "###..#.##.",
            ".####..###",
            "..###.#..#",
            "..####..#.",
            ".#..####..",
            "##...###..",
            "##...####.",
            "#####..###",
            "####....##",
        };
        pxEditHide = QIcon(QPixmap(bytes));
    }
    actEditHide.setIcon(pxEditHide);
    actEditHide.setCheckable(true);
    actEditHide.setData(QString::fromLatin1("OBTN EditHide"));
    actEditHide.setParent(this);
    addAction(&actEditHide);

    static QIcon pxEditShow;
    if(pxEditShow.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "......##..",
            ".....#.##.",
            "....#..###",
            "...#..#..#",
            "..##.#..#.",
            ".#.##..#..",
            "##..###...",
            "##...#....",
            "#####.....",
            "####......",
        };
        pxEditShow = QIcon(QPixmap(bytes));
    }
    actEditShow.setIcon(pxEditShow);
    actEditShow.setCheckable(true);
    actEditShow.setData(QString::fromLatin1("OBTN EditShow"));
    actEditShow.setParent(this);
    addAction(&actEditShow);

    static QIcon pxIncrease;
    if(pxIncrease.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "....##....",
            "....##....",
            "....##....",
            "....##....",
            "##########",
            "##########",
            "....##....",
            "....##....",
            "....##....",
            "....##....",
        };
        pxIncrease = QIcon(QPixmap(bytes));
    }
    actIncrease.setIcon(pxIncrease);
    actIncrease.setData(QString::fromLatin1("OBTN Increase"));
    actIncrease.setParent(this);
    addAction(&actIncrease);

    static QIcon pxDecrease;
    if(pxDecrease.isNull()) {
        const char * const bytes[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "..........",
            "..........",
            "..........",
            "..........",
            "##########",
            "##########",
            "..........",
            "..........",
            "..........",
            "..........",
        };
        pxDecrease = QIcon(QPixmap(bytes));
    }
    actDecrease.setIcon(pxDecrease);
    actDecrease.setData(QString::fromLatin1("OBTN Decrease"));
    actDecrease.setParent(this);
    addAction(&actDecrease);

    actOverlay.setIcon(QPixmap(_PixmapOverlay));
    actOverlay.setData(QString::fromLatin1("OBTN Overlay"));
    actOverlay.setParent(this);
    addAction(&actOverlay);

    retranslate();

    connect(tabBar(), SIGNAL(tabBarClicked(int)), this, SLOT(onCurrentChanged(int)));
    connect(tabBar(), SIGNAL(tabMoved(int,int)), this, SLOT(onTabMoved(int,int)));
    tabBar()->installEventFilter(this);
    connect(splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(onSplitterMoved()));

    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), this, SLOT(setupLayout()));

    repaintTimer.setSingleShot(true);
    connect(&repaintTimer, SIGNAL(timeout()), this, SLOT(onRepaint()));

    _animator = new QPropertyAnimation(this, "animation", this);
    _animator->setStartValue(0.0);
    _animator->setEndValue(1.0);
    connect(_animator, SIGNAL(stateChanged(QAbstractAnimation::State, 
                                           QAbstractAnimation::State)),
            this, SLOT(onAnimationStateChanged()));
}

void OverlayTabWidget::onAnimationStateChanged()
{
    if (_animator->state() != QAbstractAnimation::Running) {
        setAnimation(0);
        if (_animator->startValue().toReal() == 0.0) {
            hide();
            DockWindowManager::instance()->refreshOverlay();
        }
    }
}

void OverlayTabWidget::setAnimation(qreal t)
{
    if (t != _animation) {
        _animation = t;
        setupLayout();
    }
}

void OverlayTabWidget::startShow()
{
    if (isVisible() || _state != State_Normal)
        return;
    int duration = ViewParams::getDockOverlayAnimationDuration();
    if (duration) {
        _animator->setStartValue(1.0);
        _animator->setEndValue(0.0);
        _animator->setDuration(duration);
        _animator->setEasingCurve((QEasingCurve::Type)ViewParams::getDockOverlayAnimationCurve());
        _animator->start();
    }
    proxyWidget->hide();
    show();
}

void OverlayTabWidget::startHide()
{
    if (!isVisible()
            || _state != State_Normal
            || (_animator->state() == QAbstractAnimation::Running
                && _animator->startValue().toReal() == 0.0))
        return;
    int duration = ViewParams::getDockOverlayAnimationDuration();
    if (!duration)
        hide();
    else {
        _animator->setStartValue(0.0);
        _animator->setEndValue(1.0);
        _animator->setDuration(duration);
        _animator->setEasingCurve((QEasingCurve::Type)ViewParams::getDockOverlayAnimationCurve());
        _animator->start();
    }
}

bool OverlayTabWidget::event(QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::MouseButtonRelease:
        if(mouseGrabber() == this) {
            releaseMouse();
            ev->accept();
            return true;
        }
        break;
    case QEvent::MouseMove:
    case QEvent::ContextMenu:
        if(QApplication::mouseButtons() == Qt::NoButton && mouseGrabber() == this) {
            releaseMouse();
            ev->accept();
            return true;
        }
        break;
    case QEvent::MouseButtonPress:
        ev->accept();
        return true;
    default:
        break;
    }
    return QTabWidget::event(ev);
}

int OverlayTabWidget::testAlpha(const QPoint &_pos)
{
    if (!count() || (!isOverlayed() && !isTransparent()) || !isVisible())
        return -1;

    if (tabBar()->isVisible() && tabBar()->tabAt(tabBar()->mapFromGlobal(_pos))>=0)
        return -1;

    if (titleBar->isVisible() && titleBar->rect().contains(titleBar->mapFromGlobal(_pos)))
        return -1;

    if (!splitter->isVisible())
        return 0;

    auto pos = splitter->mapFromGlobal(_pos);
    QSize size = splitter->size();
    if (pos.x() < 0 || pos.y() < 0
            || pos.x() >= size.width()
            || pos.y() >= size.height())
    {
        if (this->rect().contains(this->mapFromGlobal(_pos)))
            return 0;
        return -1;
    }

    if (_image.isNull()) {
        auto pixmap = splitter->grab();
        _imageScale = pixmap.devicePixelRatio();
        _image = pixmap.toImage();
    }

    int res = qAlpha(_image.pixel(pos*_imageScale));
    int radius = ViewParams::getDockOverlayAlphaRadius();
    if (res || radius<=0 )
        return res;

    radius *= _imageScale;
    for (int i=-radius; i<radius; ++i) {
        for (int j=-radius; j<radius; ++j) {
            if (pos.x()+i < 0 || pos.y()+j < 0
                    || pos.x()+i >= size.width()
                    || pos.y()+j >= size.height())
                continue;
            res = qAlpha(_image.pixel(pos*_imageScale + QPoint(i,j)));
            if (res)
                return res;
        }
    }
    return 0;
}

void OverlayTabWidget::paintEvent(QPaintEvent *ev)
{
    Base::StateLocker guard(repainting);
    repaintTimer.stop();
    if (!_image.isNull())
        _image = QImage();
    QTabWidget::paintEvent(ev);
}

void OverlayTabWidget::onRepaint()
{
    Base::StateLocker guard(repainting);
    repaintTimer.stop();
    if (!_image.isNull())
        _image = QImage();
    splitter->repaint();
}

void OverlayTabWidget::scheduleRepaint()
{
    if(!repainting
            && isVisible() 
            && _graphicsEffect
            && _graphicsEffect->enabled())
    {
        repaintTimer.start(100);
    }
}

QColor OverlayTabWidget::effectColor() const
{
    return _graphicsEffect->color();
}

void OverlayTabWidget::setEffectColor(const QColor &color)
{
    _graphicsEffect->setColor(color);
    _graphicsEffectTab->setColor(color);
}

int OverlayTabWidget::effectWidth() const
{
    return _graphicsEffect->size().width();
}

void OverlayTabWidget::setEffectWidth(int s)
{
    auto size = _graphicsEffect->size();
    size.setWidth(s);
    _graphicsEffect->setSize(size);
    _graphicsEffectTab->setSize(size);
}

int OverlayTabWidget::effectHeight() const
{
    return _graphicsEffect->size().height();
}

void OverlayTabWidget::setEffectHeight(int s)
{
    auto size = _graphicsEffect->size();
    size.setHeight(s);
    _graphicsEffect->setSize(size);
    _graphicsEffectTab->setSize(size);
}

qreal OverlayTabWidget::effectOffsetX() const
{
    return _graphicsEffect->offset().x();
}

void OverlayTabWidget::setEffectOffsetX(qreal d)
{
    auto offset = _graphicsEffect->offset();
    offset.setX(d);
    _graphicsEffect->setOffset(offset);
    _graphicsEffectTab->setOffset(offset);
}

qreal OverlayTabWidget::effectOffsetY() const
{
    return _graphicsEffect->offset().y();
}

void OverlayTabWidget::setEffectOffsetY(qreal d)
{
    auto offset = _graphicsEffect->offset();
    offset.setY(d);
    _graphicsEffect->setOffset(offset);
    _graphicsEffectTab->setOffset(offset);
}

qreal OverlayTabWidget::effectBlurRadius() const
{
    return _graphicsEffect->blurRadius();
}

void OverlayTabWidget::setEffectBlurRadius(qreal r)
{
    _graphicsEffect->setBlurRadius(r);
    _graphicsEffectTab->setBlurRadius(r);
}

bool OverlayTabWidget::effectEnabled() const
{
    return _effectEnabled;
}

void OverlayTabWidget::setEffectEnabled(bool enable)
{
    _effectEnabled = enable;
}

bool OverlayTabWidget::eventFilter(QObject *o, QEvent *ev)
{
    if(ev->type() == QEvent::Resize && o == tabBar()) {
        if (_state == State_Normal)
            timer.start(10);
    }
    return QTabWidget::eventFilter(o, ev);
}

void OverlayTabWidget::restore(ParameterGrp::handle handle)
{
    std::string widgets = handle->GetASCII("Widgets","");
    for(auto &name : QString::fromLatin1(widgets.c_str()).split(QLatin1Char(','))) {
        if(name.isEmpty())
            continue;
        auto dock = getMainWindow()->findChild<QDockWidget*>(name);
        if(dock)
            addWidget(dock, dock->windowTitle());
    }
    int width = handle->GetInt("Width", 0);
    int height = handle->GetInt("Height", 0);
    int offset1 = handle->GetInt("Offset1", 0);
    int offset2 = handle->GetInt("Offset3", 0);
    setOffset(QSize(offset1,offset2));
    setSizeDelta(handle->GetInt("Offset2", 0));
    if(width && height) {
        QRect rect = geometry();
        setRect(QRect(rect.left(),rect.top(),width,height));
    }
    setAutoHide(handle->GetBool("AutoHide", false));
    setTransparent(handle->GetBool("Transparent", false));
    setEditHide(handle->GetBool("EditHide", false));
    setEditShow(handle->GetBool("EditShow", false));

    std::string savedSizes = handle->GetASCII("Sizes","");
    QList<int> sizes;
    for(auto &size : QString::fromLatin1(savedSizes.c_str()).split(QLatin1Char(',')))
        sizes.append(size.toInt());

    getSplitter()->setSizes(sizes);
    hGrp = handle;
}

void OverlayTabWidget::saveTabs()
{
    if(!hGrp)
        return;

    std::ostringstream os;
    for(int i=0,c=count(); i<c; ++i) {
        auto dock = dockWidget(i);
        if(dock && dock->objectName().size())
            os << dock->objectName().toLatin1().constData() << ",";
    }
    hGrp->SetASCII("Widgets", os.str().c_str());

    if(splitter->isVisible()) {
        os.str("");
        for(int size : splitter->sizes())
            os << size << ",";
        hGrp->SetASCII("Sizes", os.str().c_str());
    }
}

void OverlayTabWidget::onTabMoved(int from, int to)
{
    QWidget *w = splitter->widget(from);
    splitter->insertWidget(to,w);
    saveTabs();
}

void OverlayTabWidget::setTitleBar(QWidget *w)
{
    titleBar = w;
}

void OverlayTabWidget::changeEvent(QEvent *e)
{
    QTabWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange)
        retranslate();
}

void OverlayTabWidget::retranslate()
{
    actTransparent.setToolTip(tr("Toggle transparent mode"));
    actAutoHide.setToolTip(tr("Toggle auto hide mode"));
    actEditHide.setToolTip(tr("Toggle auto hide on edit mode"));
    actEditShow.setToolTip(tr("Toggle auto show on edit mode"));
    actIncrease.setToolTip(tr("Increase window size, either width or height depending on the docking site.\n"
                              "Hold CTRL key while pressing the button to change size in the other dimension.\n"
                              "Hold SHIFT key while pressing the button to move the window.\n"
                              "Hold CTRL + SHIFT key to move the window in the other direction."));
    actDecrease.setToolTip(tr("Decrease window size, either width or height depending on the docking site.\n"
                              "Hold CTRL key while pressing to change size in the other dimension.\n"
                              "Hold SHIFT key while pressing the button to move the window.\n"
                              "Hold CTRL + SHIFT key to move the window in the other direction."));
    actOverlay.setToolTip(tr("Toggle overlay"));
}

void OverlayTabWidget::onAction(QAction *action)
{
    if(action == &actEditHide) {
        if(hGrp)
            hGrp->SetBool("EditHide", actEditHide.isChecked());
        if(action->isChecked()) {
            setAutoHide(false);
            setEditShow(false);
        }
    } else if(action == &actAutoHide) {
        if(hGrp)
            hGrp->SetBool("AutoHide", actAutoHide.isChecked());
        if(action->isChecked()) {
            setEditHide(false);
            setEditShow(false);
        }
    } else if(action == &actEditShow) {
        if(hGrp)
            hGrp->SetBool("EditShow", actEditShow.isChecked());
        if(action->isChecked()) {
            setEditHide(false);
            setAutoHide(false);
        }
    } else if(action == &actIncrease)
        changeSize(5);
    else if(action == &actDecrease)
        changeSize(-5);
    else if(action == &actOverlay) {
        DockWindowManager::instance()->setOverlayMode(DockWindowManager::ToggleActive);
        return;
    } else if(action == &actTransparent) {
        if(hGrp)
            hGrp->SetBool("Transparent", actTransparent.isChecked());
    }
    DockWindowManager::instance()->refreshOverlay(this);
}

int OverlayTabWidget::adjustSize(int size) const
{
    if (_state != State_Normal || !isVisible())
        return size;
    if (dockArea == Qt::LeftDockWidgetArea
            || dockArea == Qt::RightDockWidgetArea)
        return std::max(size, rectOverlay.width()+offset.width());
    else
        return std::max(size, rectOverlay.height()+offset.width());
}

void OverlayTabWidget::setState(State state)
{
    if (_state == state)
        return;
    switch(state) {
    case State_Normal:
        _state = state;
        hide();
        if (dockArea == Qt::RightDockWidgetArea)
            setTabPosition(East);
        else if (dockArea == Qt::BottomDockWidgetArea)
            setTabPosition(South);
        if (count() == 1)
            tabBar()->hide();
        _graphicsEffectTab->setEnabled(false);
        titleBar->show();
        splitter->show();
        break;
    case State_Hint:
        if (_state == State_HintHidden)
            break;
        _state = state;
        if (ViewParams::getDockOverlayHintTabBar()) {
            tabBar()->show();
            titleBar->hide();
            splitter->hide();
            _graphicsEffectTab->setEnabled(true);
            show();
            raise();
            proxyWidget->raise();
            if (dockArea == Qt::RightDockWidgetArea)
                setTabPosition(West);
            else if (dockArea == Qt::BottomDockWidgetArea)
                setTabPosition(North);
            DockWindowManager::instance()->refreshOverlay(this);
        }
        break;
    case State_HintHidden:
        _state = state;
        hide();
        _graphicsEffectTab->setEnabled(true);
        break;
    }
}

bool OverlayTabWidget::checkAutoHide() const
{
    if(isAutoHide())
        return true;

    if(ViewParams::getDockOverlayAutoView()) {
        auto view = getMainWindow()->activeWindow();
        if(!view || (!view->isDerivedFrom(View3DInventor::getClassTypeId())
                        && !view->isDerivedFrom(SplitView3DInventor::getClassTypeId())))
            return true;
    }

    if(isEditShow()) {
        return !Application::Instance->editDocument() 
            && (!Control().taskPanel() || Control().taskPanel()->isEmpty());
    }

    if(isEditHide() && Application::Instance->editDocument())
        return true;

    return false;
}

static inline OverlayTabWidget *findTabWidget(QWidget *widget=nullptr, bool filterDialog=false)
{
    if(!widget)
        widget = qApp->focusWidget();
    for(auto w=widget; w; w=w->parentWidget()) {
        auto tabWidget = qobject_cast<OverlayTabWidget*>(w);
        if(tabWidget) 
            return tabWidget;
        auto proxy = qobject_cast<OverlayProxyWidget*>(w);
        if(proxy)
            return proxy->getOwner();
        if(filterDialog && w->windowType() != Qt::Widget)
            break;
    }
    return nullptr;
}

void OverlayTabWidget::leaveEvent(QEvent*)
{
    DockWindowManager::instance()->refreshOverlay();
}

void OverlayTabWidget::enterEvent(QEvent*)
{
    revealTime = QTime();
    DockWindowManager::instance()->refreshOverlay();
}

void OverlayTabWidget::setRevealTime(const QTime &time)
{
    revealTime = time;
}

class OverlayStyleSheet: public ParameterGrp::ObserverType {
public:

    OverlayStyleSheet() {
        handle = App::GetApplication().GetParameterGroupByPath(
                "User parameter:BaseApp/Preferences/MainWindow");
        update();
        handle->Attach(this);
    }

    static OverlayStyleSheet *instance() {
        static OverlayStyleSheet *inst;
        if(!inst)
            inst = new OverlayStyleSheet;
        return inst;
    }

    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        if(strcmp(sReason, "StyleSheet")==0
                || strcmp(sReason, "OverlayActiveStyleSheet")==0
                || strcmp(sReason, "OverlayOnStyleSheet")==0
                || strcmp(sReason, "OverlayOffStyleSheet")==0)
        {
            DockWindowManager::instance()->refreshOverlay(nullptr, true);
        }
    }

    void update() {
        QString mainstyle = QString::fromLatin1(handle->GetASCII("StyleSheet").c_str());

        QString prefix;
       
        if(!mainstyle.isEmpty()) {
            int dark = mainstyle.indexOf(QLatin1String("dark"),0,Qt::CaseInsensitive);
            prefix = QString::fromLatin1("overlay:%1").arg(
                    dark<0 ? QLatin1String("Light") : QLatin1String("Dark"));
        }

        QString name;

        onStyleSheet.clear();
        if(ViewParams::getDockOverlayExtraState()) {
            name = QString::fromUtf8(handle->GetASCII("OverlayOnStyleSheet").c_str());
            if(name.isEmpty() && !prefix.isEmpty())
                name = prefix + QLatin1String("-on.qss");
            else if (!QFile::exists(name))
                name = QString::fromLatin1("overlay:%1").arg(name);
            if(QFile::exists(name)) {
                QFile f(name);
                if(f.open(QFile::ReadOnly)) {
                    QTextStream str(&f);
                    onStyleSheet = str.readAll();
                }
            }
            if(onStyleSheet.isEmpty()) {
                static QLatin1String _default(
                    "* { background-color: transparent;"
                        "border: 1px solid palette(dark);"
                        "alternate-background-color: rgba(255,255,255,100)}"
                    "QTreeView, QListView { background: rgba(255,255,255,50) }"
                    "QToolTip { background-color: palette(base) }"
                    // Both background and border are necessary to make this work.
                    // And this spare us to have to call QTabWidget::setDocumentMode(true).
                    "QTabWidget:pane { background-color: rgba(255,255,255,50); border: transparent }"
                );
                onStyleSheet = _default;
            }
        }

        name = QString::fromUtf8(handle->GetASCII("OverlayOffStyleSheet").c_str());
        if(name.isEmpty() && !prefix.isEmpty())
            name = prefix + QLatin1String("-off.qss");
        else if (!QFile::exists(name))
            name = QString::fromLatin1("overlay:%1").arg(name);
        offStyleSheet.clear();
        if(QFile::exists(name)) {
            QFile f(name);
            if(f.open(QFile::ReadOnly)) {
                QTextStream str(&f);
                offStyleSheet = str.readAll();
            }
        }
        if(offStyleSheet.isEmpty()) {
            static QLatin1String _default(
                "Gui--OverlayToolButton { background: transparent; padding: 0px; border: none }"
                "Gui--OverlayToolButton:hover { background: palette(light); border: 1px solid palette(dark) }"
                "Gui--OverlayToolButton:focus { background: palette(dark); border: 1px solid palette(dark) }"
                "Gui--OverlayToolButton:pressed { background: palette(dark); border: 1px inset palette(dark) }"
                "Gui--OverlayToolButton:checked { background: palette(dark); border: 1px inset palette(dark) }"
                "Gui--OverlayToolButton:checked:hover { background: palette(light); border: 1px inset palette(dark) }"
            );
            offStyleSheet = _default;
        }

        name = QString::fromUtf8(handle->GetASCII("OverlayActiveStyleSheet").c_str());
        if(name.isEmpty() && !prefix.isEmpty())
            name = prefix + QLatin1String(".qss");
        else if (!QFile::exists(name))
            name = QString::fromLatin1("overlay:%1").arg(name);
        activeStyleSheet.clear();
        if(QFile::exists(name)) {
            QFile f(name);
            if(f.open(QFile::ReadOnly)) {
                QTextStream str(&f);
                activeStyleSheet = str.readAll();
            }
        }
        if(activeStyleSheet.isEmpty()) {
            static QLatin1String _default(
                "* {alternate-background-color: rgba(250,250,250,120);}"

                "QComboBox, QComboBox:editable, QComboBox:!editable, QLineEdit,"
                "QTextEdit, QPlainTextEdit, QAbstractSpinBox, QDateEdit, QDateTimeEdit,"
                "Gui--PropertyEditor--PropertyEditor QLabel "
                    "{background : palette(base);}"

                "QScrollBar { background: transparent;}"
                "QTabWidget::pane { background-color: transparent; border: transparent }"
                "Gui--OverlayTabWidget { qproperty-effectColor: rgba(0,0,0,0) }"
                "Gui--OverlayTabWidget::pane { background-color: rgba(250,250,250,80) }"

                "QTabBar {border : none;}"
                "QTabBar::tab {color: palette(text);"
                              "background-color: rgba(100,100,100,50);"
                              "padding: 5px}"
                "QTabBar::tab:selected {background-color: rgba(250,250,250,80);}"
                "QTabBar::tab:hover {background-color: rgba(250,250,250,200);}"

                "QHeaderView { background:transparent }"
                "QHeaderView::section {color: palette(text);"
                                      "background-color: rgba(250,250,250,50);"
                                      "border: 1px solid palette(dark);"
                                      "padding: 2px}"

                "QTreeView, QListView, QTableView {"
                            "background: rgb(250,250,250);"
                            "selection-background-color: rgba(94, 144, 250, 0.7);}"
                "QListView::item:selected, QTreeView::item:selected {"
                            "background-color: rgba(94, 144, 250, 0.7);}"

                "Gui--PropertyEditor--PropertyEditor {"
                            "border: 1px solid palette(dark);"
                            "qproperty-groupTextColor: rgb(100, 100, 100);"
                            "qproperty-groupBackground: rgba(180, 180, 180, 0.7);}"

                "QToolTip {background-color: rgba(250,250,250,180);}"

                "Gui--CallTipsList::item { background-color: rgba(200,200,200,200);}"
                "Gui--CallTipsList::item::selected { background-color: palette(highlight);}"

                "QAbstractButton { background: rgba(250,250,250,80);"
                                  "padding: 2px 4px;}"
                "QAbstractButton::hover { background: rgba(250,250,250,200);}"
                "QAbstractButton::focus { background: rgba(250,250,250,255);}"
                "QAbstractButton::pressed { background: rgba(100,100,100,100);"
                                           "border: 1px inset palette(dark) }"
                "QAbstractButton::checked { background: rgba(100,100,100,100);"
                                           "border: 1px inset palette(dark) }"
                "QAbstractButton::checked:hover { background: rgba(150,150,150,200);"
                                                 "border: 1px inset palette(dark) }"
                "Gui--OverlayToolButton { background: transparent; padding: 0px; border: none }"
                );
            activeStyleSheet = _default;
        }

        if(onStyleSheet.isEmpty()) {
            onStyleSheet = activeStyleSheet;
            hideTab = false;
        } else {
            hideTab = (onStyleSheet.indexOf(QLatin1String("QTabBar")) < 0);
        }
    }

    ParameterGrp::handle handle;
    QString onStyleSheet;
    QString offStyleSheet;
    QString activeStyleSheet;
    bool hideTab = false;
};

void OverlayTabWidget::_setOverlayMode(QWidget *widget, int enable)
{
    if(!widget)
        return;

#if QT_VERSION>QT_VERSION_CHECK(5,12,2) && QT_VERSION < QT_VERSION_CHECK(5,12,6)
    // Work around Qt bug https://bugreports.qt.io/browse/QTBUG-77006
    if(enable < 0)
        widget->setStyleSheet(OverlayStyleSheet::instance()->activeStyleSheet);
    else if(enable)
        widget->setStyleSheet(OverlayStyleSheet::instance()->onStyleSheet);
    else
        widget->setStyleSheet(OverlayStyleSheet::instance()->offStyleSheet);
#endif

    auto tabbar = qobject_cast<QTabBar*>(widget);
    if(tabbar) {
        // Stylesheet QTabWidget::pane make the following two calls unnecessary
        //
        // tabbar->setDrawBase(enable>0);
        // tabbar->setDocumentMode(enable!=0);

        if(!tabbar->autoHide() || tabbar->count()>1) {
            if(!OverlayStyleSheet::instance()->hideTab)
                tabbar->setVisible(true);
            else
                tabbar->setVisible(enable==0 || (enable<0 && tabbar->count()>1));
            return;
        }
    }
    if(enable!=0) {
        widget->setWindowFlags(widget->windowFlags() | Qt::FramelessWindowHint);
    } else {
        widget->setWindowFlags(widget->windowFlags() & ~Qt::FramelessWindowHint);
    }
    widget->setAttribute(Qt::WA_NoSystemBackground, enable!=0);
    widget->setAttribute(Qt::WA_TranslucentBackground, enable!=0);
}

void OverlayTabWidget::setOverlayMode(QWidget *widget, int enable)
{
    if(!widget || qobject_cast<QDialog*>(widget))
        return;

    if(widget != tabBar()) {
        if((ViewParams::getDockOverlayMouseThrough()
                    || ViewParams::getDockOverlayAutoMouseThrough())
                && enable == -1)
        {
            widget->setMouseTracking(true);
        }
    }

    _setOverlayMode(widget, enable);

    if(qobject_cast<QComboBox*>(widget)) {
        // do not set child QAbstractItemView of QComboBox, otherwise the drop down box
        // won't be shown
        return;
    }
    for(auto child : widget->children())
        setOverlayMode(qobject_cast<QWidget*>(child), enable);
}

void OverlayTabWidget::setAutoHide(bool enable)
{
    if(actAutoHide.isChecked() == enable)
        return;
    if(hGrp)
        hGrp->SetBool("AutoHide", enable);
    actAutoHide.setChecked(enable);
    if(enable) {
        setEditHide(false);
        setEditShow(false);
    }
    DockWindowManager::instance()->refreshOverlay(this);
}

void OverlayTabWidget::setTransparent(bool enable)
{
    if(actTransparent.isChecked() == enable)
        return;
    if(hGrp)
        hGrp->SetBool("Transparent", enable);
    actTransparent.setChecked(enable);
    DockWindowManager::instance()->refreshOverlay(this);
}

void OverlayTabWidget::setEditHide(bool enable)
{
    if(actEditHide.isChecked() == enable)
        return;
    if(hGrp)
        hGrp->SetBool("EditHide", enable);
    actEditHide.setChecked(enable);
    if(enable) {
        setAutoHide(false);
        setEditShow(false);
    }
    DockWindowManager::instance()->refreshOverlay(this);
}

void OverlayTabWidget::setEditShow(bool enable)
{
    if(actEditShow.isChecked() == enable)
        return;
    if(hGrp)
        hGrp->SetBool("EditShow", enable);
    actEditShow.setChecked(enable);
    if(enable) {
        setAutoHide(false);
        setEditHide(false);
    }
    DockWindowManager::instance()->refreshOverlay(this);
}

QDockWidget *OverlayTabWidget::currentDockWidget() const
{
    int index = -1;
    for(int size : splitter->sizes()) {
        ++index;
        if(size>0)
            return dockWidget(index);
    }
    return dockWidget(currentIndex());
}

QDockWidget *OverlayTabWidget::dockWidget(int index) const
{
    if(index < 0 || index >= splitter->count())
        return nullptr;
    return qobject_cast<QDockWidget*>(splitter->widget(index));
}

void OverlayTabWidget::setOverlayMode(bool enable)
{
    overlayed = enable;

    if(!isVisible() || !count())
        return;

    touched = false;

    if (_state == State_Normal)
        titleBar->setVisible(!enable);

    if(!enable && isTransparent())
    {
        proxyWidget->setStyleSheet(OverlayStyleSheet::instance()->activeStyleSheet);
        setStyleSheet(OverlayStyleSheet::instance()->activeStyleSheet);
        setOverlayMode(this, -1);
    } else if (enable && !isTransparent() && (isEditShow() || isAutoHide())) {
        proxyWidget->setStyleSheet(OverlayStyleSheet::instance()->offStyleSheet);
        setStyleSheet(OverlayStyleSheet::instance()->offStyleSheet);
        setOverlayMode(this, 0);
    } else {
        if(enable) {
            proxyWidget->setStyleSheet(OverlayStyleSheet::instance()->onStyleSheet);
            setStyleSheet(OverlayStyleSheet::instance()->onStyleSheet);
        } else {
            proxyWidget->setStyleSheet(OverlayStyleSheet::instance()->offStyleSheet);
            setStyleSheet(OverlayStyleSheet::instance()->offStyleSheet);
        }

        setOverlayMode(this, enable?1:0);
    }

    _graphicsEffect->setEnabled(effectEnabled() && (enable || isTransparent()));

    if (_state == State_Hint && ViewParams::getDockOverlayHintTabBar()) {
        tabBar()->show();
    } else if(count() == 1) {
        tabBar()->hide();
    } else
        tabBar()->setVisible(!enable || !OverlayStyleSheet::instance()->hideTab);

    setRect(rectOverlay);
}

const QRect &OverlayTabWidget::getRect()
{
    return rectOverlay;
}

bool OverlayTabWidget::getAutoHideRect(QRect &rect) const
{
    rect = rectOverlay;
    int hintWidth = ViewParams::getDockOverlayHintSize();
    switch(dockArea) {
    case Qt::RightDockWidgetArea:
        rect.setLeft(rect.left() + std::max(rect.width()-hintWidth,0));
        rect.setTop(_TopOverlay->adjustSize(rect.top()));
        break;
    case Qt::LeftDockWidgetArea:
        rect.setRight(rect.right() - std::max(rect.width()-hintWidth,0));
        rect.setTop(_TopOverlay->adjustSize(rect.top()));
        break;
    case Qt::TopDockWidgetArea:
        rect.setBottom(rect.bottom() - std::max(rect.height()-hintWidth,0));
        rect.setLeft(_LeftOverlay->adjustSize(rect.left()));
        break;
    case Qt::BottomDockWidgetArea:
        rect.setTop(rect.top() + std::max(rect.height()-hintWidth,0));
        rect.setLeft(_LeftOverlay->adjustSize(rect.left()));
        rect.setRight(rect.right() - _RightOverlay->adjustSize(0));
        break;
    default:
        break;
    }
    return overlayed && checkAutoHide();
}

void OverlayTabWidget::setOffset(const QSize &ofs)
{
    if(offset != ofs) {
        offset = ofs;
        if(hGrp) {
            hGrp->SetInt("Offset1", ofs.width());
            hGrp->SetInt("Offset3", ofs.height());
        }
    }
}

void OverlayTabWidget::setSizeDelta(int delta)
{
    if(sizeDelta != delta) {
        if(hGrp)
            hGrp->SetInt("Offset2", delta);
        sizeDelta = delta;
    }
}

void OverlayTabWidget::setRect(QRect rect)
{
    if(rect.width()<=0 || rect.height()<=0)
        return;

    if(hGrp && rect.size() != rectOverlay.size()) {
        hGrp->SetInt("Width", rect.width());
        hGrp->SetInt("Height", rect.height());
    }
    rectOverlay = rect;

    if(getAutoHideRect(rect) || _state == State_Hint) {
        QRect rectHint = rect;
        if (_state != State_Hint)
            startHide();
        else if (count() && ViewParams::getDockOverlayHintTabBar()) {
            switch(dockArea) {
            case Qt::LeftDockWidgetArea: 
            case Qt::RightDockWidgetArea: 
                rectHint.setBottom(rect.bottom());
                if (dockArea == Qt::LeftDockWidgetArea)
                    rect.setWidth(tabBar()->width());
                else
                    rect.setLeft(rect.left() + rect.width() - tabBar()->width());
                rect.setHeight(std::min(rect.height(), 
                            tabBar()->y() + tabBar()->sizeHint().height() + 5));
                break;
            case Qt::BottomDockWidgetArea: 
            case Qt::TopDockWidgetArea: 
                rectHint.setRight(rect.right());
                if (dockArea == Qt::TopDockWidgetArea)
                    rect.setHeight(tabBar()->height());
                else
                    rect.setTop(rect.top() + rect.height() - tabBar()->height());
                rect.setWidth(std::min(rect.width(),
                            tabBar()->x() + tabBar()->sizeHint().width() + 5));
                break;
            default:
                break;
            }

            setGeometry(rect);
        }
        proxyWidget->setGeometry(rectHint);
        proxyWidget->show();
        proxyWidget->raise();

    } else {
        setGeometry(rectOverlay);

        for(int i=0, count=splitter->count(); i<count; ++i)
            splitter->widget(i)->show();

        if(!isVisible() && count()) {
            proxyWidget->hide();
            startShow();
            setOverlayMode(overlayed);
        }
    }
}

void OverlayTabWidget::addWidget(QDockWidget *dock, const QString &title)
{
    QRect rect = dock->geometry();

    getMainWindow()->removeDockWidget(dock);

    auto titleWidget = dock->titleBarWidget();
    if(titleWidget && titleWidget->objectName()==QLatin1String("OverlayTitle")) {
        auto w = new QWidget();
        w->setObjectName(QLatin1String("OverlayTitle"));
        dock->setTitleBarWidget(w);
        w->hide();
        delete titleWidget;
    }

    dock->show();
    splitter->addWidget(dock);
    addTab(new QWidget(this), title);

    dock->setFeatures(dock->features() & ~QDockWidget::DockWidgetFloatable);
    if(count() == 1)
        setRect(rect);

    saveTabs();
}

int OverlayTabWidget::dockWidgetIndex(QDockWidget *dock) const
{
    return splitter->indexOf(dock);
}

void OverlayTabWidget::removeWidget(QDockWidget *dock)
{
    int index = dockWidgetIndex(dock);
    if(index < 0)
        return;

    dock->setParent(nullptr);

    auto w = this->widget(index);
    removeTab(index);
    delete w;

    if(!count())
        hide();

    w = dock->titleBarWidget();
    if(w && w->objectName() == QLatin1String("OverlayTitle")) {
        dock->setTitleBarWidget(nullptr);
        delete w;
    }
    DockWindowManager::instance()->setupTitleBar(dock);

    dock->setFeatures(dock->features() | QDockWidget::DockWidgetFloatable);

    setOverlayMode(dock, 0);

    saveTabs();
}

void OverlayTabWidget::resizeEvent(QResizeEvent *ev)
{
    QTabWidget::resizeEvent(ev);
    if (_state == State_Normal)
        timer.start(10);
}

void OverlayTabWidget::setupLayout()
{
    if (_state != State_Normal)
        return;

    if(count() == 1)
        tabSize = 0;
    else {
        int tsize;
        if(dockArea==Qt::LeftDockWidgetArea || dockArea==Qt::RightDockWidgetArea)
            tsize = tabBar()->width();
        else
            tsize = tabBar()->height();
        tabSize = tsize;
    }
    int titleBarSize = _TitleButtonSize + 1;
    QRect rect, rectTitle;
    switch(tabPosition()) {
    case West:
        rectTitle = QRect(tabSize, 0, this->width()-tabSize, titleBarSize);
        rect = QRect(rectTitle.left(), rectTitle.bottom(),
                     rectTitle.width(), this->height()-rectTitle.height());
        break;
    case East:
        rectTitle = QRect(0, 0, this->width()-tabSize, titleBarSize);
        rect = QRect(rectTitle.left(), rectTitle.bottom(),
                     rectTitle.width(), this->height()-rectTitle.height());
        break;
    case North:
        rectTitle = QRect(0, tabSize, titleBarSize, this->height()-tabSize);
        rect = QRect(rectTitle.right(), rectTitle.top(),
                     this->width()-rectTitle.width(), rectTitle.height());
        break;
    case South:
        rectTitle = QRect(0, 0, titleBarSize, this->height()-tabSize);
        rect = QRect(rectTitle.right(), rectTitle.top(),
                     this->width()-rectTitle.width(), rectTitle.height());
        break;
    }
    if (_animation != 0.0) {
        switch(dockArea) {
        case Qt::LeftDockWidgetArea:
            rect.moveLeft(rect.left() - _animation * rect.width());
            break;
        case Qt::RightDockWidgetArea:
            rect.moveLeft(rect.left() + _animation * rect.width());
            break;
        case Qt::TopDockWidgetArea:
            rect.moveTop(rect.top() - _animation * rect.height());
            break;
        case Qt::BottomDockWidgetArea:
            rect.moveTop(rect.top() + _animation * rect.height());
            break;
        default:
            break;
        }
    }
    splitter->setGeometry(rect);
    titleBar->setGeometry(rectTitle);
}

void OverlayTabWidget::setCurrent(QDockWidget *widget)
{
    int index = dockWidgetIndex(widget);
    if(index >= 0) 
        setCurrentIndex(index);
}

void OverlayTabWidget::onSplitterMoved()
{
    int index = -1;
    for(int size : splitter->sizes()) {
        ++index;
        if(size) {
            if (index != currentIndex()) {
                QSignalBlocker guard(this);
                setCurrentIndex(index);
            }
            break;
        }
    }
    saveTabs();
}

void OverlayTabWidget::onCurrentChanged(int index)
{
    setState(State_Normal);
    startShow();

    auto sizes = splitter->sizes();
    int i=0;
    int size = splitter->orientation()==Qt::Vertical ? 
                    height()-tabBar()->height() : width()-tabBar()->width();
    for(auto &s : sizes) {
        if(i++ == index)
            s = size;
        else
            s = 0;
    }
    splitter->setSizes(sizes);
    saveTabs();
}

void OverlayTabWidget::changeSize(int changes, bool checkModify)
{
    auto modifier = checkModify ? QApplication::queryKeyboardModifiers() : Qt::NoModifier;
    if(modifier== Qt::ShiftModifier) {
        setOffset(QSize(std::max(offset.width()+changes, 0), offset.height()));
        return;
    } else if ((modifier == (Qt::ShiftModifier | Qt::AltModifier))
            || (modifier == (Qt::ShiftModifier | Qt::ControlModifier))) {
        setOffset(QSize(offset.width(), std::max(offset.height()+changes, 0)));
        return;
    } else if (modifier == Qt::ControlModifier || modifier == Qt::AltModifier) {
        setSizeDelta(sizeDelta - changes);
        return;
    }

    QRect rect = rectOverlay;
    switch(dockArea) {
    case Qt::LeftDockWidgetArea:
        rect.setRight(rect.right() + changes);
        break;
    case Qt::RightDockWidgetArea:
        rect.setLeft(rect.left() - changes);
        break;
    case Qt::TopDockWidgetArea:
        rect.setBottom(rect.bottom() + changes);
        break;
    case Qt::BottomDockWidgetArea:
        rect.setTop(rect.top() - changes);
        break;
    default:
        break;
    }
    setRect(rect);
}

// -----------------------------------------------------------

OverlayGraphicsEffect::OverlayGraphicsEffect(QObject *parent) :
    QGraphicsEffect(parent),
    _enabled(false),
    _size(1,1),
    _blurRadius(2.0f),
    _color(0, 0, 0, 80)
{
}

QT_BEGIN_NAMESPACE
  extern Q_WIDGETS_EXPORT void qt_blurImage(QPainter *p, QImage &blurImage, qreal radius, bool quality, bool alphaOnly, int transposed = 0 );
QT_END_NAMESPACE

void OverlayGraphicsEffect::draw(QPainter* painter)
{
    // if nothing to show outside the item, just draw source
    if (!_enabled || _blurRadius + _size.height() <= 0 || _blurRadius + _size.width() <= 0) {
        drawSource(painter);
        return;
    }

    PixmapPadMode mode = QGraphicsEffect::PadToEffectiveBoundingRect;
    QPoint offset;
    QPixmap px = sourcePixmap(Qt::DeviceCoordinates, &offset, mode);

    // return if no source
    if (px.isNull())
        return;

    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        static int count;
        getMainWindow()->showMessage(
                QString::fromLatin1("dock overlay redraw %1").arg(count++));
    }

    QTransform restoreTransform = painter->worldTransform();
    painter->setWorldTransform(QTransform());

    // Calculate size for the background image
    QImage tmp(px.size(), QImage::Format_ARGB32_Premultiplied);
    tmp.setDevicePixelRatio(px.devicePixelRatioF());
    tmp.fill(0);
    QPainter tmpPainter(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_Source);
    if(_size.width() == 0 && _size.height() == 0)
        tmpPainter.drawPixmap(QPoint(0, 0), px);
    else {
        for (int x=-_size.width();x<=_size.width();++x) {
            for (int y=-_size.height();y<=_size.height();++y) {
                if (x || y) {
                    tmpPainter.drawPixmap(QPoint(x, y), px);
                    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
                }
            }
        }
    }
    tmpPainter.end();

    // blur the alpha channel
    QImage blurred(tmp.size(), QImage::Format_ARGB32_Premultiplied);
    blurred.setDevicePixelRatio(px.devicePixelRatioF());
    blurred.fill(0);
    QPainter blurPainter(&blurred);
    qt_blurImage(&blurPainter, tmp, blurRadius(), false, true);
    blurPainter.end();

    tmp = blurred;

    // blacken the image...
    tmpPainter.begin(&tmp);
    tmpPainter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    tmpPainter.fillRect(tmp.rect(), color());
    tmpPainter.end();

    // draw the blurred shadow...
    painter->drawImage(QPointF(offset.x()+_offset.x(), offset.y()+_offset.y()), tmp);

    // draw the actual pixmap...
    painter->drawPixmap(offset, px, QRectF());

#if 0
    QWidget *focus = qApp->focusWidget();
    if (focus) {
        QWidget *widget = qobject_cast<QWidget*>(this->parent());
        if (auto *edit = qobject_cast<QPlainTextEdit*>(focus)) {
            if (!edit->isReadOnly() && edit->isEnabled()) {
                for(auto w=edit->parentWidget(); w; w=w->parentWidget()) {
                    if (w == widget) {
                        QRect r = edit->cursorRect();
                        QRect rect(edit->viewport()->mapTo(widget, r.topLeft()), 
                                edit->viewport()->mapTo(widget, r.bottomRight()));
                        // painter->fillRect(rect, edit->textColor());
                        // painter->fillRect(rect, edit->currentCharFormat().foreground());
                        painter->fillRect(rect.translated(offset), Qt::white);
                    }
                }
            }
        }
    }
#endif

    // restore world transform
    painter->setWorldTransform(restoreTransform);
}

QRectF OverlayGraphicsEffect::boundingRectFor(const QRectF& rect) const
{
    if (!_enabled)
        return rect;
    return rect.united(rect.adjusted(-_blurRadius - _size.width() + _offset.x(), 
                                     -_blurRadius - _size.height()+ _offset.y(), 
                                     _blurRadius + _size.width() + _offset.x(),
                                     _blurRadius + _size.height() + _offset.y()));
}

// -----------------------------------------------------------

struct OverlayInfo {
    const char *name;
    OverlayTabWidget *tabWidget;
    Qt::DockWidgetArea dockArea;
    QMap<QDockWidget*, OverlayInfo*> &overlayMap;
    ParameterGrp::handle hGrp;

    OverlayInfo(QWidget *parent, const char *name, Qt::DockWidgetArea pos, QMap<QDockWidget*, OverlayInfo*> &map)
        : name(name), dockArea(pos), overlayMap(map)
    {
        tabWidget = new OverlayTabWidget(parent, dockArea);
        tabWidget->setObjectName(QString::fromLatin1(name));
        tabWidget->getProxyWidget()->setObjectName(tabWidget->objectName() + QString::fromLatin1("Proxy"));
        tabWidget->setMovable(true);
        hGrp = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                            ->GetGroup("MainWindow")->GetGroup("DockWindows")->GetGroup(name);
    }

    bool addWidget(QDockWidget *dock, bool forced=true) {
        if(!dock)
            return false;
        if(tabWidget->dockWidgetIndex(dock) >= 0)
            return false;
        overlayMap[dock] = this;
        bool visible = dock->isVisible();

        auto focus = qApp->focusWidget();
        if(focus && findTabWidget(focus) != tabWidget)
            focus = nullptr;

        tabWidget->addWidget(dock, dock->windowTitle());

        if(focus) {
            tabWidget->setCurrent(dock);
            focus = qApp->focusWidget();
            if(focus)
                focus->clearFocus();
        }

        if(forced) {
            auto mw = getMainWindow();
            for(auto d : mw->findChildren<QDockWidget*>()) {
                if(mw->dockWidgetArea(d) == dockArea
                        && d->toggleViewAction()->isChecked())
                {
                    addWidget(d, false);
                }
            }
            if(visible) {
                dock->show();
                tabWidget->setCurrent(dock);
            }
        } else
            tabWidget->saveTabs();
        return true;
    }

    void removeWidget() {
        if(!tabWidget->count())
            return;

        tabWidget->hide();

        QPointer<QWidget> focus = qApp->focusWidget();

        MainWindow *mw = getMainWindow();
        QDockWidget *lastDock = tabWidget->currentDockWidget();
        if(lastDock) {
            tabWidget->removeWidget(lastDock);
            lastDock->show();
            mw->addDockWidget(dockArea, lastDock);
        }
        while(tabWidget->count()) {
            QDockWidget *dock = tabWidget->dockWidget(0);
            if(!dock) {
                tabWidget->removeTab(0);
                continue;
            }
            tabWidget->removeWidget(dock);
            dock->show();
            if(lastDock)
                mw->tabifyDockWidget(lastDock, dock);
            else
                mw->addDockWidget(dockArea, dock);
            lastDock = dock;
        }

        if(focus)
            focus->setFocus();

        tabWidget->saveTabs();
    }

    bool geometry(QRect &rect, QRect &rectHint) {
        if(!tabWidget->count()) {
            rect = rectHint = QRect(0,0,0,0);
            return false;
        }
        rect = tabWidget->getRect();
        if (!tabWidget->isVisible() 
                || tabWidget->getState() != OverlayTabWidget::State_Normal)
            rectHint = QRect(0,0,0,0);
        else
            rectHint = rect;
        return true;
    }

    void setGeometry(int x, int y, int w, int h)
    {
        if(!tabWidget->count())
            return;
        tabWidget->setRect(QRect(x,y,w,h));
    }

    void save()
    {
    }

    void restore()
    {
        tabWidget->restore(hGrp);
        for(int i=0,c=tabWidget->count();i<c;++i) {
            auto dock = tabWidget->dockWidget(i);
            if(dock)
                overlayMap[dock] = this;
        }
    }

};

#endif // FC_HAS_DOCK_OVERLAY

enum OverlayToggleMode {
    OverlayUnset,
    OverlaySet,
    OverlayToggle,
    OverlayToggleAutoHide,
    OverlayToggleTransparent,
    OverlayCheck,
};

namespace Gui {
struct DockWindowManagerP
{
    QList<QDockWidget*> _dockedWindows;
    QMap<QString, QPointer<QWidget> > _dockWindows;
    DockWindowItems _dockWindowItems;
    QTimer _timer;

#ifdef FC_HAS_DOCK_OVERLAY
    QMap<QDockWidget*, OverlayInfo*> _overlays;
    OverlayInfo _left;
    OverlayInfo _right;
    OverlayInfo _top;
    OverlayInfo _bottom;
    std::array<OverlayInfo*,4> _overlayInfos;

    QPoint _lastPos;

    QAction _actClose;
    QAction _actFloat;
    QAction _actOverlay;
    std::array<QAction*, 3> _actions;

    QList<QPointer<View3DInventorViewer> > _3dviews;
    int _trackingView = -1;
    OverlayTabWidget *_trackingOverlay = nullptr;

    bool updateStyle = false;

    DockWindowManagerP(DockWindowManager *host, QWidget *parent)
        :_left(parent,"OverlayLeft", Qt::LeftDockWidgetArea,_overlays)
        ,_right(parent,"OverlayRight", Qt::RightDockWidgetArea,_overlays)
        ,_top(parent,"OverlayTop", Qt::TopDockWidgetArea,_overlays)
        ,_bottom(parent,"OverlayBottom",Qt::BottomDockWidgetArea,_overlays)
        ,_overlayInfos({&_left,&_right,&_top,&_bottom})
        ,_actions({&_actOverlay,&_actFloat,&_actClose})
    {
        Application::Instance->signalActivateView.connect([this](const MDIView *) {
            refreshOverlay();
        });
        Application::Instance->signalInEdit.connect([this](const ViewProviderDocumentObject &) {
            refreshOverlay();
        });
        Application::Instance->signalResetEdit.connect([this](const ViewProviderDocumentObject &) {
            refreshOverlay();
        });

        _actOverlay.setIcon(QPixmap(_PixmapOverlay));
        _actOverlay.setData(QString::fromLatin1("OBTN Overlay"));

        const char * const pixmapFloat[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "...#######",
            "...#.....#",
            "...#.....#",
            "#######..#",
            "#.....#..#",
            "#.....#..#",
            "#.....####",
            "#.....#...",
            "#.....#...",
            "#######...",
        };
        _actFloat.setIcon(QPixmap(pixmapFloat));
        _actFloat.setData(QString::fromLatin1("OBTN Float"));

        const char * const pixmapClose[]={
            "10 10 2 1",
            ". c None",
            TITLE_BUTTON_COLOR,
            "##......##",
            "###....###",
            ".###..###.",
            "..######..",
            "...####...",
            "...####...",
            "..######..",
            ".###..###.",
            "###....###",
            "##......##",
        };
        _actClose.setIcon(QPixmap(pixmapClose));
        _actClose.setData(QString::fromLatin1("OBTN Close"));

        retranslate();

        for(auto action : _actions) {
            QObject::connect(action, SIGNAL(triggered(bool)), host, SLOT(onAction()));
        }
        for(auto o : _overlayInfos) {
            for(auto action : o->tabWidget->actions()) {
                QObject::connect(action, SIGNAL(triggered(bool)), host, SLOT(onAction()));
            }
            o->tabWidget->setTitleBar(createTitleBar(o->tabWidget));
        }
    }

    bool toggleOverlay(QDockWidget *dock, OverlayToggleMode toggle,
            Qt::DockWidgetArea dockPos=Qt::NoDockWidgetArea)
    {
        if(!dock)
            return false;

        auto it = _overlays.find(dock);
        if(it != _overlays.end()) {
            auto o = it.value();
            switch(toggle) {
            case OverlayToggleAutoHide:
                o->tabWidget->setAutoHide(!o->tabWidget->isAutoHide());
                break;
            case OverlayToggleTransparent:
                o->tabWidget->setTransparent(!o->tabWidget->isTransparent());
                break;
            case OverlayUnset:
            case OverlayToggle:
                _overlays.erase(it);
                o->removeWidget();
                return false;
            default:
                break;
            }
            return true;
        }

        if(toggle == OverlayUnset)
            return false;

        if(dockPos == Qt::NoDockWidgetArea)
            dockPos = getMainWindow()->dockWidgetArea(dock);
        OverlayInfo *o;
        switch(dockPos) {
        case Qt::LeftDockWidgetArea:
            o = &_left;
            break;
        case Qt::RightDockWidgetArea:
            o = &_right;
            break;
        case Qt::TopDockWidgetArea:
            o = &_top;
            break;
        case Qt::BottomDockWidgetArea:
            o = &_bottom;
            break;
        default:
            return false;
        }
        if(toggle == OverlayCheck && !o->tabWidget->count())
            return false;
        if(o->addWidget(dock)) {
            if(toggle == OverlayToggleAutoHide)
                o->tabWidget->setAutoHide(true);
            else if(toggle == OverlayToggleTransparent)
                o->tabWidget->setTransparent(true);
        }
        return true;
    }

    void refreshOverlay(QWidget *widget=nullptr, bool refreshStyle=false)
    {
        if(refreshStyle) {
            OverlayStyleSheet::instance()->update();
            updateStyle = true;
        }

        if(widget) {
            auto tabWidget = findTabWidget(widget);
            if(tabWidget && tabWidget->count()) {
                for(auto o : _overlayInfos) {
                    if(tabWidget == o->tabWidget) {
                        tabWidget->touch();
                        onTimer();
                        return;
                    }
                }
            }
        }
        _timer.start(ViewParams::getDockOverlayDelay());
    }

    void saveOverlay()
    {
        _left.save();
        _right.save();
        _top.save();
        _bottom.save();
    }

    void restoreOverlay()
    {
        _left.restore();
        _right.restore();
        _top.restore();
        _bottom.restore();
        refreshOverlay();
    }

    void onTimer()
    {
        auto mdi = getMainWindow() ? getMainWindow()->getMdiArea() : nullptr;
        if(!mdi)
            return;

        auto focus = findTabWidget(qApp->focusWidget());
        auto active = findTabWidget(qApp->widgetAt(QCursor::pos()));
        OverlayTabWidget *reveal = nullptr;

        bool updateFocus = false;
        bool updateActive = false;

        for(auto o : _overlayInfos) {
            if(o->tabWidget->isTouched() || updateStyle) {
                if(o->tabWidget == focus)
                    updateFocus = true;
                else if(o->tabWidget == active)
                    updateActive = true;
                else 
                    o->tabWidget->setOverlayMode(true);
            }
            if(!o->tabWidget->getRevealTime().isNull()) {
                if(o->tabWidget->getRevealTime()<= QTime::currentTime())
                    o->tabWidget->setRevealTime(QTime());
                else
                    reveal = o->tabWidget;
            }
        }
        updateStyle = false;

        if(focus && (focus->isOverlayed() || updateFocus)) {
            focus->setOverlayMode(false);
            focus->raise();
            if(reveal == focus)
                reveal = nullptr;
        }

        if(active) {
            if(active != focus && (active->isOverlayed() || updateActive)) 
                active->setOverlayMode(false);
            active->raise();
            if(reveal == active)
                reveal = nullptr;
        }

        if(reveal) {
            reveal->setOverlayMode(false);
            reveal->raise();
        }

        for(auto o : _overlayInfos) {
            if(o->tabWidget != focus 
                    && o->tabWidget != active
                    && o->tabWidget != reveal
                    && o->tabWidget->count()
                    && !o->tabWidget->isOverlayed())
            {
                o->tabWidget->setOverlayMode(true);
            }
        }

        int w = mdi->geometry().width();
        int h = mdi->geometry().height();
        auto tabbar = mdi->findChild<QTabBar*>();
        if(tabbar)
            h -= tabbar->height();

        int naviCubeSize = ViewParams::getNaviWidgetSize()+10;
        int naviCorner = ViewParams::getDockOverlayCheckNaviCube() ?
            ViewParams::getCornerNaviCube() : -1;

        QRect rect;
        QRect rectBottom;
        if(_bottom.geometry(rect, rectBottom)) {
            QSize ofs = _bottom.tabWidget->getOffset();
            int delta = _bottom.tabWidget->getSizeDelta();
            h -= ofs.height();
            if(naviCorner == 2)
                ofs.setWidth(ofs.width()+naviCubeSize);
            int bw = w-10-ofs.width()-delta;
            if(naviCorner == 3)
                bw -= naviCubeSize;
            if(bw < 10)
                bw = 10;
            // Bottom width is maintain the same to reduce QTextEdit re-layout
            // which may be expensive if there are lots of text, e.g. for
            // ReportView or PythonConsole.
            _bottom.setGeometry(ofs.width(),h-rect.height(),bw,rect.height());
        }
        QRect rectLeft;
        if(_left.geometry(rect, rectLeft)) {
            auto ofs = _left.tabWidget->getOffset();
            if(naviCorner == 0)
                ofs.setWidth(ofs.width()+naviCubeSize);
            int delta = _left.tabWidget->getSizeDelta()+rectBottom.height();
            if(naviCorner == 2 && naviCubeSize > rectBottom.height())
                delta += naviCubeSize - rectBottom.height();
            int lh = std::max(h-ofs.width()-delta, 10);
            _left.setGeometry(ofs.height(),ofs.width(),rect.width(),lh);
        }

        QRect rectRight(0,0,0,0);
        if(_right.geometry(rect, rectRight)) {
            auto ofs = _right.tabWidget->getOffset();
            if(naviCorner == 1)
                ofs.setWidth(ofs.width()+naviCubeSize);
            int delta = _right.tabWidget->getSizeDelta()+rectBottom.height();
            if(naviCorner == 3 && naviCubeSize > rectBottom.height())
                delta += naviCubeSize - rectBottom.height();
            int rh = std::max(h-ofs.width()-delta, 10);
            w -= ofs.height();
            _right.setGeometry(w-rect.width(),ofs.width(),rect.width(),rh);
        }
        QRect rectTop;
        if(_top.geometry(rect, rectTop)) {
            auto ofs = _top.tabWidget->getOffset();
            int delta = _top.tabWidget->getSizeDelta();
            if(naviCorner == 0)
                rectLeft.setWidth(std::max(rectLeft.width(), naviCubeSize));
            else if(naviCorner == 1)
                rectRight.setWidth(std::max(rectRight.width(), naviCubeSize));
            int tw = w-rectLeft.width()-rectRight.width()-ofs.width()-delta;
            _top.setGeometry(rectLeft.width()-ofs.width(),ofs.height(),tw,rect.height());
        }
    }

    void setOverlayMode(DockWindowManager::OverlayMode mode)
    {
        switch(mode) {
        case DockWindowManager::DisableAll:
        case DockWindowManager::EnableAll: {
            auto docks = getMainWindow()->findChildren<QDockWidget*>();
            // put visible dock widget first
            std::sort(docks.begin(),docks.end(),
                [](const QDockWidget *a, const QDockWidget *) {
                    return !a->visibleRegion().isEmpty();
                });
            for(auto dock : docks) {
                if(mode == DockWindowManager::DisableAll)
                    toggleOverlay(dock, OverlayUnset);
                else
                    toggleOverlay(dock, OverlaySet);
            }
            return;
        }
        case DockWindowManager::ToggleAll:
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count()) {
                    setOverlayMode(DockWindowManager::DisableAll);
                    return;
                }
            }
            setOverlayMode(DockWindowManager::EnableAll);
            return;
        case DockWindowManager::AutoHideAll: {
            bool found = false;
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count())
                    found = true;
            }
            if(!found)
                setOverlayMode(DockWindowManager::EnableAll);
        }
        // fall through
        case DockWindowManager::AutoHideNone:
            for(auto o : _overlayInfos)
                o->tabWidget->setAutoHide(mode == DockWindowManager::AutoHideAll);
            refreshOverlay();
            return;
        case DockWindowManager::ToggleAutoHideAll:
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count() && o->tabWidget->isAutoHide()) {
                    setOverlayMode(DockWindowManager::AutoHideNone);
                    return;
                }
            }
            setOverlayMode(DockWindowManager::AutoHideAll);
            return;
        case DockWindowManager::TransparentAll: {
            bool found = false;
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count())
                    found = true;
            }
            if(!found)
                setOverlayMode(DockWindowManager::EnableAll);
        }
        // fall through
        case DockWindowManager::TransparentNone:
            for(auto o : _overlayInfos)
                o->tabWidget->setTransparent(mode == DockWindowManager::TransparentAll);
            refreshOverlay();
            return;
        case DockWindowManager::ToggleTransparentAll:
            for(auto o : _overlayInfos) {
                if(o->tabWidget->count() && o->tabWidget->isTransparent()) {
                    setOverlayMode(DockWindowManager::TransparentNone);
                    return;
                }
            }
            setOverlayMode(DockWindowManager::TransparentAll);
            return;
        default:
            break;
        }

        OverlayToggleMode m;
        QDockWidget *dock = nullptr;
        for(auto w=qApp->widgetAt(QCursor::pos()); w; w=w->parentWidget()) {
            dock = qobject_cast<QDockWidget*>(w);
            if(dock)
                break;
            auto tabWidget = qobject_cast<OverlayTabWidget*>(w);
            if(tabWidget) {
                dock = tabWidget->currentDockWidget();
                if(dock)
                    break;
            }
        }
        if(!dock) {
            for(auto w=qApp->focusWidget(); w; w=w->parentWidget()) {
                dock = qobject_cast<QDockWidget*>(w);
                if(dock)
                    break;
            }
        }

        switch(mode) {
        case DockWindowManager::ToggleActive:
            m = OverlayToggle;
            break;
        case DockWindowManager::ToggleAutoHide:
            m = OverlayToggleAutoHide;
            break;
        case DockWindowManager::ToggleTransparent:
            m = OverlayToggleTransparent;
            break;
        case DockWindowManager::EnableActive:
            m = OverlaySet;
            break;
        case DockWindowManager::DisableActive:
            m = OverlayUnset;
            break;
        default:
            return;
        }
        toggleOverlay(dock, m);
    }

    void onToggleDockWidget(QDockWidget *dock, bool checked)
    {
        if(!dock)
            return;

        auto it = _overlays.find(dock);
        if(it == _overlays.end()) {
            if(!checked)
                return;
            toggleOverlay(dock, OverlayCheck);
            it = _overlays.find(dock);
            if(it == _overlays.end())
                return;
        }
        if(checked) {
            int index = it.value()->tabWidget->dockWidgetIndex(dock);
            if(index >= 0) {
                auto sizes = it.value()->tabWidget->getSplitter()->sizes();
                if(index >= sizes.size() || sizes[index]==0) 
                    it.value()->tabWidget->setCurrent(dock);
                else {
                    QSignalBlocker guard(it.value()->tabWidget);
                    it.value()->tabWidget->setCurrent(dock);
                }
            }
            it.value()->tabWidget->setRevealTime(QTime::currentTime().addMSecs(
                    ViewParams::getDockOverlayRevealDelay()));
            refreshOverlay();
        } else {
            it.value()->tabWidget->removeWidget(dock);
            getMainWindow()->addDockWidget(it.value()->dockArea, dock);
            _overlays.erase(it);
        }
    }

    void changeOverlaySize(int changes)
    {
        auto tabWidget = findTabWidget(qApp->widgetAt(QCursor::pos()));
        if(tabWidget) {
            tabWidget->changeSize(changes, false);
            refreshOverlay();
        }
    }

    void onFocusChanged(QWidget *, QWidget *) {
        refreshOverlay();
    }

    void setupTitleBar(QDockWidget *dock)
    {
        if(!dock->titleBarWidget())
            dock->setTitleBarWidget(createTitleBar(nullptr));
    }

    QWidget *createTitleBar(QWidget *parent)
    {
        auto widget = new QWidget(parent);
        widget->setObjectName(QLatin1String("OverlayTitle"));

        bool vertical = false;
        QBoxLayout *layout = nullptr;
        auto tabWidget = qobject_cast<OverlayTabWidget*>(parent);
        if(!tabWidget) {
           layout = new QBoxLayout(QBoxLayout::LeftToRight, widget); 
        } else {
            switch(tabWidget->getDockArea()) {
            case Qt::LeftDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::LeftToRight, widget); 
                break;
            case Qt::RightDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::RightToLeft, widget); 
                break;
            case Qt::TopDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::TopToBottom, widget); 
                vertical = true;
                break;
            case Qt::BottomDockWidgetArea:
                layout = new QBoxLayout(QBoxLayout::BottomToTop, widget); 
                vertical = true;
                break;
            default:
                break;
            }
        }
        layout->addSpacing(5);
        layout->setContentsMargins(1,1,1,1);
        if(parent) {
            for(auto action : parent->actions())
                layout->addWidget(createTitleButton(action));
        } else {
            for(auto action : _actions)
                layout->addWidget(createTitleButton(action));
        }
        // layout->addStretch(2);
        layout->addSpacerItem(new QSpacerItem(_TitleButtonSize,_TitleButtonSize,
                    vertical?QSizePolicy::Minimum:QSizePolicy::Expanding,
                    vertical?QSizePolicy::Expanding:QSizePolicy::Minimum));
        return widget;
    }

    QWidget *createTitleButton(QAction *action)
    {
        auto button = new OverlayToolButton(nullptr);
        button->setObjectName(action->data().toString());
        button->setDefaultAction(action);
        button->setAutoRaise(true);
        button->setContentsMargins(0,0,0,0);
        button->setFixedSize(_TitleButtonSize,_TitleButtonSize);
        return button;
    }

    void onAction(QAction *action) {
        if(action == &_actOverlay) {
            DockWindowManager::instance()->setOverlayMode(DockWindowManager::ToggleActive);
        } else if(action == &_actFloat || action == &_actClose) {
            for(auto w=qApp->widgetAt(QCursor::pos());w;w=w->parentWidget()) {
                auto dock = qobject_cast<QDockWidget*>(w);
                if(!dock)
                    continue;
                if(action == &_actClose) {
                    dock->toggleViewAction()->activate(QAction::Trigger);
                } else {
                    auto it = _overlays.find(dock);
                    if(it != _overlays.end()) {
                        it.value()->tabWidget->removeWidget(dock);
                        getMainWindow()->addDockWidget(it.value()->dockArea, dock);
                        _overlays.erase(it);
                        dock->show();
                        dock->setFloating(true);
                        refreshOverlay();
                    } else 
                        dock->setFloating(!dock->isFloating());
                }
                return;
            }
        } else {
            auto tabWidget = qobject_cast<OverlayTabWidget*>(action->parent());
            if(tabWidget)
                tabWidget->onAction(action);
        }
    }

    void retranslate()
    {
        _actOverlay.setToolTip(QObject::tr("Toggle overlay"));
        _actFloat.setToolTip(QObject::tr("Toggle floating window"));
        _actClose.setToolTip(QObject::tr("Close dock window"));
    }

#else // FC_HAS_DOCK_OVERLAY

    DockWindowManagerP(DockWindowManager *, QWidget *) {}
    void refreshOverlay(QWidget *, bool) {}
    void saveOverlay() {}
    void restoreOverlay() {}
    void onTimer() {}
    void setOverlayMode(DockWindowManager::OverlayMode) {}
    void onToggleDockWidget(QDockWidget *, bool) {}
    void changeOverlaySize(int) {}
    void onFocusChanged(QWidget *, QWidget *) {}
    void onAction(QAction *) {}
    void setupTitleBar(QDockWidget *) {}
    void retranslate() {}

    bool toggleOverlay(QDockWidget *, OverlayToggleMode,
            Qt::DockWidgetArea dockPos=Qt::NoDockWidgetArea)
    {
        (void)dockPos;
        return false;
    }

#endif // FC_HAS_DOCK_OVERLAY
};
} // namespace Gui

DockWindowManager* DockWindowManager::_instance = 0;

DockWindowManager* DockWindowManager::instance()
{
    if ( _instance == 0 )
        _instance = new DockWindowManager;
    return _instance;
}

void DockWindowManager::destruct()
{
    delete _instance;
    _instance = 0;
}

DockWindowManager::DockWindowManager()
{
    auto mdi = getMainWindow()->getMdiArea();
    assert(mdi);
    d = new DockWindowManagerP(this,mdi);
    connect(&d->_timer, SIGNAL(timeout()), this, SLOT(onTimer()));
    d->_timer.setSingleShot(true);

    connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
            this, SLOT(onFocusChanged(QWidget*,QWidget*)));

    qApp->installEventFilter(this);
}

DockWindowManager::~DockWindowManager()
{
    d->_dockedWindows.clear();
    delete d;
}

void DockWindowManager::setOverlayMode(OverlayMode mode)
{
    d->setOverlayMode(mode);
}

/**
 * Adds a new dock window to the main window and embeds the given \a widget.
 */
QDockWidget* DockWindowManager::addDockWindow(const char* name, QWidget* widget, Qt::DockWidgetArea pos)
{
    if(!widget)
        return nullptr;
    QDockWidget *dw = qobject_cast<QDockWidget*>(widget->parentWidget());
    if(dw)
        return dw;

    // creates the dock widget as container to embed this widget
    MainWindow* mw = getMainWindow();
    dw = new QDockWidget(mw);
    d->setupTitleBar(dw);

    // Note: By default all dock widgets are hidden but the user can show them manually in the view menu.
    // First, hide immediately the dock widget to avoid flickering, after setting up the dock widgets
    // MainWindow::loadLayoutSettings() is called to restore the layout.
    dw->hide();
    switch (pos) {
    case Qt::LeftDockWidgetArea:
    case Qt::RightDockWidgetArea:
    case Qt::TopDockWidgetArea:
    case Qt::BottomDockWidgetArea:
        mw->addDockWidget(pos, dw);
    default:
        break;
    }
    connect(dw, SIGNAL(destroyed(QObject*)),
            this, SLOT(onDockWidgetDestroyed(QObject*)));
    connect(widget, SIGNAL(destroyed(QObject*)),
            this, SLOT(onWidgetDestroyed(QObject*)));

    // add the widget to the dock widget
    widget->setParent(dw);
    dw->setWidget(widget);

    // set object name and window title needed for i18n stuff
    dw->setObjectName(QLatin1String(name));
    dw->setWindowTitle(QDockWidget::tr(name));
    dw->setFeatures(QDockWidget::AllDockWidgetFeatures);

    d->_dockedWindows.push_back(dw);

    connect(dw->toggleViewAction(), SIGNAL(triggered(bool)), this, SLOT(onToggleDockWidget(bool)));
    return dw;
}

void DockWindowManager::onToggleDockWidget(bool checked)
{
    auto action = qobject_cast<QAction*>(sender());
    if(!action)
        return;
    d->onToggleDockWidget(qobject_cast<QDockWidget*>(action->parent()), checked);
}

/**
 * Returns the widget inside the dock window by name.
 * If it does not exist 0 is returned.
 */
QWidget* DockWindowManager::getDockWindow(const char* name) const
{
    for (QList<QDockWidget*>::ConstIterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        if ((*it)->objectName() == QLatin1String(name))
            return (*it)->widget();
    }

    return 0;
}

/**
 * Returns a list of all widgets inside the dock windows.
 */
QList<QWidget*> DockWindowManager::getDockWindows() const
{
    QList<QWidget*> docked;
    for (QList<QDockWidget*>::ConstIterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it)
        docked.push_back((*it)->widget());
    return docked;
}

/**
 * Removes the specified dock window with name \name without deleting it.
 */
QWidget* DockWindowManager::removeDockWindow(const char* name)
{
    QWidget* widget=0;
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        if ((*it)->objectName() == QLatin1String(name)) {
            QDockWidget* dw = *it;
            d->_dockedWindows.erase(it);
            d->toggleOverlay(dw, OverlayUnset);
            getMainWindow()->removeDockWidget(dw);
            // avoid to destruct the embedded widget
            widget = dw->widget();
            widget->setParent(0);
            dw->setWidget(0);
            disconnect(dw, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onDockWidgetDestroyed(QObject*)));
            disconnect(widget, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onWidgetDestroyed(QObject*)));
            delete dw; // destruct the QDockWidget, i.e. the parent of the widget
            break;
        }
    }

    return widget;
}

/**
 * Method provided for convenience. Does basically the same as the method above unless that
 * it accepts a pointer.
 */
void DockWindowManager::removeDockWindow(QWidget* widget)
{
    if (!widget)
        return;
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        if ((*it)->widget() == widget) {
            QDockWidget* dw = *it;
            d->_dockedWindows.erase(it);
            d->toggleOverlay(dw, OverlayUnset);
            getMainWindow()->removeDockWidget(dw);
            // avoid to destruct the embedded widget
            widget->setParent(0);
            dw->setWidget(0);
            disconnect(dw, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onDockWidgetDestroyed(QObject*)));
            disconnect(widget, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onWidgetDestroyed(QObject*)));
            delete dw; // destruct the QDockWidget, i.e. the parent of the widget
            break;
        }
    }
}

/**
 * Sets the window title for the dockable windows.
 */
void DockWindowManager::retranslate()
{
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        (*it)->setWindowTitle(QDockWidget::tr((*it)->objectName().toLatin1()));
    }
    d->retranslate();
}

/**
 * Appends a new \a widget with \a name to the list of available dock widgets. The caller must make sure that
 * the name is unique. If a widget with this name is already registered nothing is done but false is returned,
 * otherwise it is appended and true is returned.
 *
 * As default the following widgets are already registered:
 * \li Std_TreeView
 * \li Std_PropertyView
 * \li Std_ReportView
 * \li Std_ToolBox
 * \li Std_ComboView
 * \li Std_SelectionView
 *
 * To avoid name clashes the caller should use names of the form \a module_widgettype, i. e. if a analyse dialog for
 * the mesh module is added the name must then be Mesh_AnalyzeDialog. 
 *
 * To make use of dock windows when a workbench gets loaded the method setupDockWindows() must reimplemented in a 
 * subclass of Gui::Workbench. 
 */
bool DockWindowManager::registerDockWindow(const char* name, QWidget* widget)
{
    QMap<QString, QPointer<QWidget> >::Iterator it = d->_dockWindows.find(QLatin1String(name));
    if (it != d->_dockWindows.end() || !widget)
        return false;
    d->_dockWindows[QLatin1String(name)] = widget;
    widget->hide(); // hide the widget if not used
    return true;
}

QWidget* DockWindowManager::unregisterDockWindow(const char* name)
{
    QWidget* widget = 0;
    QMap<QString, QPointer<QWidget> >::Iterator it = d->_dockWindows.find(QLatin1String(name));
    if (it != d->_dockWindows.end()) {
        widget = d->_dockWindows.take(QLatin1String(name));
    }

    return widget;
}

QWidget* DockWindowManager::findRegisteredDockWindow(const char* name)
{
    QMap<QString, QPointer<QWidget> >::Iterator it = d->_dockWindows.find(QLatin1String(name));
    if (it != d->_dockWindows.end())
        return it.value();
    return nullptr;
}

/** Sets up the dock windows of the activated workbench. */
void DockWindowManager::setup(DockWindowItems* items)
{
    // save state of current dock windows
    saveState();
    d->_dockWindowItems = *items;

    ParameterGrp::handle hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("MainWindow")->GetGroup("DockWindows");
    QList<QDockWidget*> docked = d->_dockedWindows;
    const QList<DockWindowItem>& dws = items->dockWidgets();

    for (QList<DockWindowItem>::ConstIterator it = dws.begin(); it != dws.end(); ++it) {
        QDockWidget* dw = findDockWidget(docked, it->name);
        QByteArray dockName = it->name.toLatin1();
        bool visible = hPref->GetBool(dockName.constData(), it->visibility);

        if (!dw) {
            QMap<QString, QPointer<QWidget> >::ConstIterator jt = d->_dockWindows.find(it->name);
            if (jt != d->_dockWindows.end()) {
                dw = addDockWindow(jt.value()->objectName().toUtf8(), jt.value(), it->pos);
                jt.value()->show();
                dw->toggleViewAction()->setData(it->name);
                dw->setVisible(visible);
                if(!visible)
                    continue;
            }
        }
        else {
            dw->setVisible(visible);
            dw->toggleViewAction()->setVisible(true);
            int index = docked.indexOf(dw);
            docked.removeAt(index);
        }

        if(dw)
            d->toggleOverlay(dw, OverlayCheck);
    }
}

void DockWindowManager::saveState()
{
    ParameterGrp::handle hPref = App::GetApplication().GetUserParameter().GetGroup("BaseApp")
                               ->GetGroup("MainWindow")->GetGroup("DockWindows");

    const QList<DockWindowItem>& dockItems = d->_dockWindowItems.dockWidgets();
    for (QList<DockWindowItem>::ConstIterator it = dockItems.begin(); it != dockItems.end(); ++it) {
        QDockWidget* dw = findDockWidget(d->_dockedWindows, it->name);
        if (dw) {
            QByteArray dockName = dw->toggleViewAction()->data().toByteArray();
            hPref->SetBool(dockName.constData(), dw->isVisible());
        }
    }
}

QDockWidget* DockWindowManager::findDockWidget(const QList<QDockWidget*>& dw, const QString& name) const
{
    for (QList<QDockWidget*>::ConstIterator it = dw.begin(); it != dw.end(); ++it) {
        if ((*it)->toggleViewAction()->data().toString() == name)
            return *it;
    }

    return 0;
}

void DockWindowManager::onDockWidgetDestroyed(QObject* dw)
{
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        if (*it == dw) {
            d->_dockedWindows.erase(it);
            break;
        }
    }
}

void DockWindowManager::onWidgetDestroyed(QObject* widget)
{
    for (QList<QDockWidget*>::Iterator it = d->_dockedWindows.begin(); it != d->_dockedWindows.end(); ++it) {
        // make sure that the dock widget is not about to being deleted
        if ((*it)->metaObject() != &QDockWidget::staticMetaObject) {
            disconnect(*it, SIGNAL(destroyed(QObject*)),
                       this, SLOT(onDockWidgetDestroyed(QObject*)));
            d->_dockedWindows.erase(it);
            break;
        }

        if ((*it)->widget() == widget) {
            // Delete the widget if not used anymore
            QDockWidget* dw = *it;
            dw->deleteLater();
            break;
        }
    }
}

void DockWindowManager::onTimer()
{
    d->onTimer();
}

bool DockWindowManager::eventFilter(QObject *o, QEvent *ev)
{
    switch(ev->type()) {
    case QEvent::Resize: {
        if(getMainWindow() && o == getMainWindow()->getMdiArea())
            refreshOverlay();
        return false;
    }
#ifdef FC_HAS_DOCK_OVERLAY
    case QEvent::KeyPress: {
        QKeyEvent *ke = static_cast<QKeyEvent*>(ev);
        bool accepted = false;
        if (ke->modifiers() == Qt::NoModifier && ke->key() == Qt::Key_Escape) {
            for (auto &o : d->_overlays) {
                if (o->tabWidget->getState() == OverlayTabWidget::State_Hint) {
                    o->tabWidget->setState(OverlayTabWidget::State_HintHidden);
                    accepted = true;
                }
            }
        }
        if (accepted) {
            ke->accept();
            return true;
        }
        break;
    }
    case QEvent::Paint:
        if (auto widget = qobject_cast<QWidget*>(o)) {
            // QAbstractItemView optimize redraw using its item delegate's
            // visualRect(). However, if we are using QGraphicsEffects, the
            // effect may touch areas outside of visualRect(), so
            // OverlayTabWidget offers a timer for a delayed redraw.
            widget = qobject_cast<QAbstractItemView*>(widget->parentWidget());
            if(widget) {
                auto tabWidget = findTabWidget(widget, true);
                if (tabWidget)
                    tabWidget->scheduleRepaint();
            }
        }
        break;
    // case QEvent::MouseButtonDblClick:
    // case QEvent::NativeGesture:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::Wheel:
    case QEvent::ContextMenu: {
        if (d->_trackingView >= 0) {
            View3DInventorViewer *view = nullptr;
            if(!TreeWidget::isDragging() && d->_trackingView < d->_3dviews.size())
                view = d->_3dviews[d->_trackingView];
            if(view)
                view->callEventFilter(ev);
            if(!view || ev->type() == QEvent::MouseButtonRelease
                     || QApplication::mouseButtons() == Qt::NoButton)
            {
                d->_trackingView = -1;
                if (d->_trackingOverlay == QWidget::mouseGrabber()
                        && ev->type() == QEvent::MouseButtonRelease)
                {
                    d->_trackingOverlay = nullptr;
                    // Must not release mouse here, because otherwise the event
                    // will find its way to the actual widget under cursor.
                    // Instead, return false here to let OverlayTabWidget::event()
                    // release the mouse.
                    return false;
                }
                if(d->_trackingOverlay && QWidget::mouseGrabber() == d->_trackingOverlay)
                    d->_trackingOverlay->releaseMouse();
                d->_trackingOverlay = nullptr;
            }
            // Must return true here to filter the event, otherwise ContextMenu
            // event may be routed to the actual widget. Other types of event
            // probably do not matter.
            return true;
        } else if (ev->type() != QEvent::MouseButtonPress 
                && QApplication::mouseButtons()!=Qt::NoButton)
            return false;

        if(TreeWidget::isDragging())
            return false;

        int hit = 0;
        QPoint pos = QCursor::pos();
        if ((ViewParams::getDockOverlayAutoMouseThrough()
                    && ev->type() != QEvent::Wheel
                    && pos == d->_lastPos)
                || (ViewParams::getDockOverlayMouseThrough()
                    && (QApplication::queryKeyboardModifiers() & Qt::AltModifier)))
        {
            hit = 1;
        } else if (ev->type() != QEvent::Wheel) {
            for(auto widget=qApp->widgetAt(pos); widget ; widget=widget->parentWidget()) {
                int type = widget->windowType();
                if (type != Qt::Widget && type != Qt::Window) {
                    if (type != Qt::SubWindow)
                        hit = -1;
                    break;
                }
                if (qobject_cast<QAbstractButton*>(widget))
                    break;
                auto tabWidget = qobject_cast<OverlayTabWidget*>(widget);
                if (tabWidget) {
                    if (tabWidget->testAlpha(pos) == 0) {
                        hit = ViewParams::getDockOverlayAutoMouseThrough();
                        d->_lastPos = pos;
                    }
                    break;
                }
            }
        }
        if (hit == 0) {
            for (auto &o : d->_overlays)
                o->tabWidget->getProxyWidget()->hitTest(pos);
        }
        if (hit <= 0) {
            d->_lastPos.setX(INT_MAX);
            d->_3dviews.clear();
            return false;
        }
        if(d->_3dviews.isEmpty()) {
            for(auto w : getMainWindow()->windows(QMdiArea::StackingOrder)) {
                if(!w->isVisible())
                    continue;
                // It is possible to support mouse through for all MDIView.
                // But then we would have to copy all types of intercepted
                // event and manually map the local position inside. For
                // View3DInventorViewer, we use its backdoor function
                // callEventFilter() to pass event directly to
                // Quarter::EventFilter, and subsequently to
                // Quarter::Mouse, which we have modified to use the global
                // position of the event instead of local one.
                for(auto view : w->findChildren<View3DInventorViewer*>()) {
                    if(view->isVisible())
                        d->_3dviews.insert(0,view);
                }
            }
            if(d->_3dviews.isEmpty())
                return false;
        }

        auto widget = qobject_cast<QWidget*>(o);
        if(!widget) {
            QWindow* window = qobject_cast<QWindow*>(o);
            if (window) {
                widget = QWidget::find(window->winId());
                if (!widget)
                    return false;
            }
        }
        auto tabWidget = findTabWidget(widget, true);
        if(!tabWidget || tabWidget->isOverlayed() || !tabWidget->isTransparent())
            return false;
        if(o != tabWidget) {
            ev->ignore();
            return true;
        }
        ev->accept();
        int i = -1;
        for(auto &view : d->_3dviews) {
            ++i;
            if(!view || !view->isVisible())
                continue;
            auto p = view->mapFromGlobal(pos);
            if(p.x()<0 || p.y()<0 || p.x()>view->width() || p.y()>view->height())
                continue;

            // We could have used sendEvent() here, but it won't work for Wheel
            // event. It is (probably) filtered out by some unknown Qt event
            // filter if the target widget is not under focus.  Calling
            // setFocus() here does not seem to have any effect. So we have to
            // use some kind of backdoor here.
            view->callEventFilter(ev);

            if (ev->type() == QEvent::MouseButtonPress) {
                d->_trackingView = i;
                d->_trackingOverlay = tabWidget;
                d->_trackingOverlay->grabMouse();
            }
            break;
        }
        break;
    }
#endif
    default:
        break;
    }
    return false;
}

void DockWindowManager::refreshOverlay(QWidget *widget, bool refreshStyle)
{
    d->refreshOverlay(widget, refreshStyle);
}

bool DockWindowManager::isUnderOverlay() const
{
#ifdef FC_HAS_DOCK_OVERLAY
    return ViewParams::getDockOverlayAutoMouseThrough()
        && findTabWidget(qApp->widgetAt(QCursor::pos()), true);
#else
    return false;
#endif
}

void DockWindowManager::saveOverlay()
{
    d->saveOverlay();
}

void DockWindowManager::restoreOverlay()
{
    d->restoreOverlay();
}

void DockWindowManager::changeOverlaySize(int changes)
{
    d->changeOverlaySize(changes);
}

void DockWindowManager::onFocusChanged(QWidget *old, QWidget *now)
{
    d->onFocusChanged(old, now);
}

void DockWindowManager::setupTitleBar(QDockWidget *dock)
{
    d->setupTitleBar(dock);
}

void DockWindowManager::onAction()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if(action)
        d->onAction(action);
}

#include "moc_DockWindowManager.cpp"
