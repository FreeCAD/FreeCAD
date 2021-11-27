/****************************************************************************
 *   Copyright (c) 2020 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QToolButton>
# include <QMenu>
# include <QToolTip>
# include <QPainter>
# include <QTimer>
# include <QElapsedTimer>
#endif

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QWidgetAction>
#include <cmath>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <App/Document.h>
#include <App/DocumentObserver.h>
#include "PieMenu.h"
#include "Application.h"
#include "Document.h"
#include "BitmapFactory.h"
#include "MetaTypes.h"
#include "MainWindow.h"
#include "Widgets.h"
#include "Action.h"
#include "ViewParams.h"

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace Gui;

FC_LOG_LEVEL_INIT("Pie", true, true)

/* TRANSLATOR Gui::PieMenu */

class PieMenu::Private
{
public:
    PieMenu &master;
    std::vector<PieButton*> buttons;
    std::vector<QGraphicsOpacityEffect*> effects;
    int hoverIndex = -1;

    bool forwardKeyPress = false;

    qreal offset = 0.;
    int iconSize;
    int fontSize;
    int radius;
    int triggerRadius;
    int duration;

    QTimer timer;
    QMenu *menu = nullptr;
    QAction *action = nullptr;
    std::string param;

    QPropertyAnimation *animator=nullptr;
    bool animating = false;

    QEventLoop *eventLoop = nullptr;
    QPointer<QMenu> activeMenu;

    bool dragging = false;
    QPoint lastPressPos;
    qreal dragOffset = 0;

    Private(PieMenu &master, QMenu *menu, const char *_param)
        :master(master), menu(menu), param(_param?_param:"")
    {
        fontSize = ViewParams::getPieMenuFontSize();
        iconSize = ViewParams::getPieMenuIconSize();
        radius = ViewParams::getPieMenuRadius();
        duration = ViewParams::getPieMenuAnimationDuration();
        triggerRadius = radius - ViewParams::getPieMenuTriggerRadius();
        if (triggerRadius < 0)
            triggerRadius = 0;
        else
            triggerRadius *= triggerRadius;
    }

    static ParameterGrp::handle getParameterGroup()
    {
        static ParameterGrp::handle hGrp;
        if (!hGrp)
            hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/PieMenu");
        return hGrp;
    }

    void init()
    {
#if QT_VERSION  >= 0x050000
        menu->aboutToShow();
#else
        // work around qt4 protected signal
        connect(&master, SIGNAL(initMenu()), menu, SIGNAL(aboutToShow()));
        master.initMenu();
#endif
        QAction *first = nullptr;
        for(QAction *action : menu->actions()) {
            if (action->isSeparator())
                continue;
            if (qobject_cast<QWidgetAction*>(action))
                continue;
            if (!first)
                first = action;
            auto button = addButton(action->text(), action->icon());
            if (action->menu())
                button->setMenu(action->menu());
            else
                button->setDefaultAction(action);
            if (action->isCheckable()) {
                button->setCheckable(true);
                button->setChecked(action->isChecked());
                connect(button, SIGNAL(triggered(QAction*)), &master, SLOT(onTriggered(QAction*)));
            }
            connect(button, SIGNAL(triggered(QAction*)), &master, SLOT(onTriggered(QAction*)));
        }

        animator = new QPropertyAnimation(&master, "offset", &master);
        connect(animator, SIGNAL(stateChanged(QAbstractAnimation::State, 
                                              QAbstractAnimation::State)),
                &master, SLOT(onStateChanged()));

        if (param.size()) {
            offset = checkOffset(getParameterGroup()->GetInt(param.c_str(), 0));
        }

        updateGeometries();

        timer.setSingleShot(true);
        connect(&timer, SIGNAL(timeout()), &master, SLOT(onTimer()));
    }

    virtual ~Private()
    {
        if (eventLoop) {
            eventLoop->exit();
            eventLoop = nullptr;
        }
    }

    qreal checkOffset(qreal ofs)
    {
        if (ofs < 0)
            return 0;
        int lastPage = buttons.size() / 8 * 8;
        if (lastPage && buttons.size() % 8 == 0)
            lastPage -= 8;
        if (ofs > lastPage)
            ofs = lastPage;
        return ofs;
    }

    void onTimer()
    {
        if (hoverIndex < 0 || hoverIndex >= (int)buttons.size())
            return;

        activeMenu = buttons[hoverIndex]->menu();
        if (activeMenu) {
            activeMenu->installEventFilter(&master);
            auto btn = buttons[hoverIndex];
            setHoverIndex(-1);
            btn->click();
            if (activeMenu)
                activeMenu->removeEventFilter(&master);
            activeMenu = nullptr;
        } else if (ViewParams::PieMenuTriggerAction())
            buttons[hoverIndex]->animateClick();
    }

    bool eventFilter(QObject *, QEvent *ev)
    {
        if (ev->type() == QEvent::MouseMove && activeMenu) {
            auto me = static_cast<QMouseEvent*>(ev);
            auto pos = me->globalPos();
            int index = hitTest(master.mapFromGlobal(pos));
            if (index != hoverIndex) {
                QPoint p = activeMenu->mapFromGlobal(pos);
                if (!activeMenu->rect().contains(p)) {
                    activeMenu->hide();
                    ToolTip::hideText();
                }
            }
        }
        return false;
    }

    void enablePieMenu(bool enable=true)
    {
        if (param.size()) {
            if (!enable)
                getParameterGroup()->SetInt(param.c_str(), -1);
            else
                getParameterGroup()->SetInt(param.c_str(), (int)offset);
        }
    }

    PieButton *addButton(const QString &title, const QIcon &icon)
    {
        static QPixmap generic;
        auto button = new PieButton(&master);
        button->setText(title);
        button->setIconSize(QSize(iconSize, iconSize));
        if (icon.isNull()) {
            if (generic.isNull()) {
                generic = QPixmap(1, 32);
                generic.fill(Qt::transparent);
            }
            button->setIcon(generic);
        } else
            button->setIcon(icon);
        button->setPopupMode(QToolButton::InstantPopup);
        button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

        int fs = this->fontSize;
        if (fs <= 0)
            fs = getMainWindow()->font().pointSize();
        QFont font = master.font();
        font.setPointSize(fs);
        button->setFont(font);

        auto effect = new QGraphicsOpacityEffect(&master);
        button->setGraphicsEffect(effect);
        effects.push_back(effect);

        this->buttons.push_back(button);
        return button;
    }

    void updateVisual(qreal ofs, qreal t, const QPoint &center, int index)
    {
        (void)ofs;
        auto button = buttons[index];

        if (index < offset || (int)std::trunc(index-offset) > 7) {
            button->hide();
            return;
        }

        button->show();

        int findex = (int)std::ceil(index - offset);
        if (findex < 0) {
            findex = 0;
            effects[index]->setOpacity(1.0 - t);
        } else if (findex > 7) {
            findex = 0;
            effects[index]->setOpacity(t);
        } else
            effects[index]->setOpacity(1.0);

        QPointF pt;
        qreal angle = M_PI * 0.25 * (findex - t);
        pt.ry() = -radius * std::cos(angle);
        pt.rx() = radius * std::sin(angle);

        qreal w = button->width();
        qreal h = button->height();

        switch(findex) {
        case 0:
            pt.rx() += -w * 0.5 * (1.0 + t);
            pt.ry() += -h * 0.5 * (2.0 - t);
            break;
        case 1:
            pt.rx() += -w * 0.5 * t;
            pt.ry() += -h * 0.5 * (1.0 + t);
            break;
        case 2:
        case 3:
            pt.ry() += -h * 0.5;
            break;
        case 4:
            pt.rx() += -w * 0.5 * (1.0 - t);
            pt.ry() += -h * 0.5 * t;
            break;
        case 5:
            pt.rx() += -w * 0.5 * (2.0 - t);
            pt.ry() += -h * 0.5 * (1.0 - t);
            break;
        case 6:
        case 7:
            pt.rx() += -w;
            pt.ry() += -h * 0.5;
            break;
        }
        button->move(center + pt.toPoint());
    }

    void updateGeometries()
    {
        int w = 0, h = 0;
        for (auto btn : buttons) {
            QSize size = btn->sizeHint();
            w = std::max(w, size.width());
            h = std::max(h, size.height());
        }
        w = w*2 + 2*radius + 2;
        h = h*2 + 2*radius + 2;
        if (w != master.width() || h != master.height()) {
            QRect rect = master.geometry();
            rect.setLeft((rect.width() - w)/2);
            rect.setTop((rect.height() - h)/2);
            rect.setWidth(w);
            rect.setHeight(h);
            master.setGeometry(rect);
        }
        updateVisuals();
    }

    void updateVisuals()
    {
        ToolTip::hideText();
        QPoint center(master.width()/2, master.height()/2);
        qreal ofs, t;
        t = std::modf(offset, &ofs);
        for (std::size_t i=0; i<buttons.size(); ++i)
            updateVisual(ofs, t, center, i);
        master.repaint();
    }

    void wheelEvent(QWheelEvent *ev)
    {
        if (ev->modifiers() == Qt::ControlModifier)
            animate(ev->delta()<0 ? -1 : 1);
        else
            animate(ev->delta()<0 ? -8 : 8);
    }

    void animate(qreal step)
    {
        if (step <= 1)
            animator->setDuration(duration);
        else
            animator->setDuration(duration * 3 / 2);

        animator->stop();
        qreal endOffset = offset;
        endOffset += step;
        endOffset = checkOffset(endOffset);

        if (endOffset == offset)
            return;

        if (param.size())
            getParameterGroup()->SetInt(param.c_str(), (int)endOffset);

        if (std::abs(offset-endOffset) < 0.01 ||  duration <= 0) {
            offset = endOffset;
            updateVisuals();
        }
        animator->setEasingCurve((QEasingCurve::Type)ViewParams::getPieMenuAnimationCurve());
        animator->setStartValue(offset);
        animator->setEndValue(endOffset);
        animator->start();
    }

    void setHoverIndex(int index)
    {
        if (index == hoverIndex || activeMenu)
            return;

        if (index == -2 && buttons.size() > 8)
            master.setCursor(Qt::OpenHandCursor);
        else
            master.setCursor(QCursor());

        hoverIndex = index;
        master.update(master.width()/2-radius, master.height()/2-radius, radius*2, radius*2);

        if (ViewParams::getPieMenuTriggerDelay() > 0) {
            if (hoverIndex < 0)
                timer.stop();
            else
                timer.start(ViewParams::getPieMenuTriggerDelay());
        }
    }

    bool isAnimating()
    {
        return animator->state() == QAbstractAnimation::Running;
    }

    bool hitAngle(QPoint pos, int hitRadius, qreal &angle)
    {
        QPoint center(master.width()/2, master.height()/2);
        pos -= center;
        int length = pos.x()*pos.x() + pos.y()*pos.y();
        if (!length || length < hitRadius)
            return false;

        angle = std::acos(pos.x()/std::sqrt(length));
        if (pos.y() > 0)
            angle += M_PI * 0.5;
        else if (angle < M_PI * 0.5)
            angle = M_PI * 0.5 - angle;
        else
            angle = M_PI * 2.5 - angle;
        angle += M_PI * 0.125;
        if (angle > M_PI * 2.0)
            angle = M_PI * 2.0 - angle;

        return true;
    }

    int hitTest(QPoint pos)
    {
        if (isAnimating()
                || pos.x() < 0
                || pos.y() < 0
                || pos.x() > master.width()
                || pos.y() > master.height())
            return -1;

        qreal angle;
        if (!hitAngle(pos, triggerRadius, angle))
            return -2;

        int index = (int)(angle / (M_PI * 0.25) + offset);
        if (index >= (int)buttons.size())
            return -1;

        return index;
    }

    int squareDistance(const QPoint &p1, const QPoint &p2)
    {
        int x = p1.x()-p2.x();
        int y = p1.y()-p2.y();
        return x*x + y*y;
    }

    void mouseMoveEvent(QMouseEvent *ev)
    {
        if (!dragging) {
            if (ev->buttons().testFlag(Qt::LeftButton) && 
                    squareDistance(ev->pos(), lastPressPos) > 25)
                startDragging();
        } else if (!ev->buttons().testFlag(Qt::LeftButton)) {
            endDragging();
        }

        if (dragging) {
            qreal angle;
            if (!hitAngle(ev->pos(), 1, angle))
                return;
            angle -= M_PI * (2.0 - 0.25 * dragOffset);
            qreal ofs = checkOffset(dragOffset - angle*4/M_PI);
            if (fabs(ofs-offset) > 0.01) {
                offset = ofs;
                updateVisuals();
            }
            return;
        }

        setHoverIndex(hitTest(ev->pos()));
    }

    void mousePressEvent(QMouseEvent *ev)
    {
        if (ev->button() == Qt::RightButton) {
            this->action = menu->exec(ev->globalPos());
            if (this->action) {
                enablePieMenu(false);
                master.hide();
            }
            return;
        }

        if (ev->button() != Qt::LeftButton) {
            master.hide();
            return;
        }

        lastPressPos = ev->pos();
        setHoverIndex(hitTest(ev->pos()));
    }

    void mouseReleaseEvent(QMouseEvent *ev)
    {
        if (ev->button() == Qt::LeftButton) {
            if (dragging)
                endDragging();
            else {
                int index = hitTest(ev->pos());
                setHoverIndex(index);
                if (index >= 0 && index < (int)buttons.size())
                    buttons[index]->animateClick();
                else 
                    master.hide();
            }
        }
    }

    void startDragging()
    {
        if (dragging)
            return;
        master.setCursor(Qt::ClosedHandCursor);
        timer.stop();
        dragOffset = offset;
        dragging = true;
    }

    void endDragging()
    {
        if (!dragging)
            return;

        dragging = false;
        master.setCursor(QCursor());

        animate(std::round(offset) - offset);
    }

    void keyPressEvent(QKeyEvent *ev)
    {
        int trigger = 0;
        int step = 0;
        switch(ev->key()) {
        case Qt::Key_PageDown:
        case Qt::Key_Q:
            step = 8;
            break;
        case Qt::Key_PageUp:
        case Qt::Key_W:
            step = -8;
            break;
        case Qt::Key_1:
            trigger = 1;
            break;
        case Qt::Key_2:
            trigger = 2;
            break;
        case Qt::Key_3:
            trigger = 3;
            break;
        case Qt::Key_4:
            trigger = 4;
            break;
        case Qt::Key_5:
            trigger = 5;
            break;
        case Qt::Key_6:
            trigger = 6;
            break;
        case Qt::Key_7:
            trigger = 7;
            break;
        case Qt::Key_8:
            trigger = 8;
            break;
        default:
            if (forwardKeyPress) {
                QKeyEvent ke(ev->type(),ev->key(),ev->modifiers(),
                        ev->text(),ev->isAutoRepeat(),ev->count());
                qApp->sendEvent(menu, &ke);
                return;
            } else if (ev->key() == Qt::Key_Space) {
                action = menu->exec(QCursor::pos());
                if (action) {
                    enablePieMenu(false);
                    master.hide();
                }
                return;
            }
        }
        if (trigger && !isAnimating()) {
            trigger += (int)std::round(offset) - 1;
            if (trigger < (int)buttons.size())
                buttons[trigger]->animateClick();
        }
        if (step)
            animate(step);
    }

    void drawIndicator(QPainter &painter, qreal index, bool reverse)
    {
        qreal findex = std::fmod(index, 8.0);
        qreal angle = M_PI * 0.25 * findex;
        QPointF pt;
        pt.ry() = -(radius-10) * std::cos(angle) + master.height() * 0.5;
        pt.rx() = (reverse?-1:1)*(radius-10) * std::sin(angle) + master.width() * 0.5;
        qreal size = 20.0;
        QRectF rect(pt.x()-size*0.5, pt.y()-size*0.5, size, size);

        angle = ((reverse?1:-1)*std::fmod(45.0 * findex, 360.0) -135) * 16.0;
        painter.drawPie(rect, (int)angle, 90*16);
    }

    void paint(QPaintEvent *)
    {
        QPainter painter(&master);
        painter.setOpacity(0.6);
        painter.setRenderHint(QPainter::Antialiasing, true);

        if (ViewParams::getPieMenuCenterRadius()) {
            painter.setPen(QPen(Qt::black, 4));
            qreal r = ViewParams::getPieMenuCenterRadius();
            painter.drawEllipse(QPointF(master.width()*0.5, master.height()*0.5), r, r);
        }

        if (buttons.size() > 8) {
            painter.setPen(Qt::transparent);
            painter.setBrush(Qt::black);
            drawIndicator(painter, offset, true);
        }

        if (!isAnimating() && hoverIndex >= 0) {
            painter.setPen(QPen(QColor(0x5e, 0x90, 0xfa), 4));
            painter.setBrush(QBrush());
            drawIndicator(painter, hoverIndex - offset, false);
            return;
        }
    }
};

PieMenu::PieMenu(QMenu *menu, const char *param, QWidget *parent)
  : QWidget(parent)
{
    this->setMouseTracking(true);
    this->setAttribute(Qt::WA_NoSystemBackground, true);
    this->setAttribute(Qt::WA_TranslucentBackground, true);

#if QT_VERSION  >= 0x050000
    if (ViewParams::PieMenuPopup())
        this->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    else {
        this->setFocusPolicy(Qt::StrongFocus);
        this->setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        connect(qApp, SIGNAL(focusChanged(QWidget*,QWidget*)),
                this, SLOT(onFocusChanged(QWidget*,QWidget*)));
    }

    QString stylesheet = qApp->styleSheet();
    if (stylesheet.isEmpty() || stylesheet.indexOf(QLatin1String("Gui--PieButton")) < 0) {
        QLatin1String key("background-color: #");
        int index = stylesheet.indexOf(key);
        QString color;
        if (index >= 0) {
            bool ok = false;
            ulong c = stylesheet.midRef(index+key.size()).toUInt(&ok, 16);
            if (ok) color = QString::fromLatin1("#%1").arg(c, 0, 16);
        }
        if (color.isEmpty()) {
            auto hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/MainWindow");
            if (hGrp->GetASCII("StyleSheet", "").find("Dark") != std::string::npos)
                color = QString::fromLatin1("#6e6e6e");
            else
                color = QString::fromLatin1("palette(window)");
        }
        stylesheet = QString::fromLatin1(
                "Gui--PieButton {"
                    "color: %3;"
                    "background-color: %1;"
                    "border: 1px solid %2;"
                    "border-radius: 4px; }"
                "Gui--PieButton:pressed {"
                    "border: 1px inset %2; }"
                "Gui--PieButton:focus {"
                    "color: palette(highlighted-text);"
                    "background-color: palette(highlight); }"
                "Gui--PieButton:disabled {"
                    "color: palette(mid);}")
            .arg(color)
            .arg(QLatin1String(color>0xa0a0a0?"palette(mid)":"palette(shadow)"))
            .arg(QLatin1String(color>0xa0a0a0?"palette(bright-text)":"palette(text)"));

        setStyleSheet(stylesheet);
    }
#else // qt4
    this->setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);
#endif
    pimpl.reset(new Private(*this, menu, param));
    pimpl->init();
}
    
PieMenu::~PieMenu()
{
}

void PieMenu::leaveEvent(QEvent *)
{
    pimpl->setHoverIndex(-1);
}

void PieMenu::mousePressEvent(QMouseEvent *ev)
{
    pimpl->mousePressEvent(ev);
}

void PieMenu::mouseReleaseEvent(QMouseEvent *ev)
{
    pimpl->mouseReleaseEvent(ev);
}

void PieMenu::hideEvent(QHideEvent *)
{
    if (pimpl->eventLoop)
        pimpl->eventLoop->exit();
}

void PieMenu::mouseMoveEvent(QMouseEvent *ev)
{
    pimpl->mouseMoveEvent(ev);
}

void PieMenu::wheelEvent(QWheelEvent *ev)
{
    pimpl->wheelEvent(ev);
}

void PieMenu::onTriggered(QAction *action)
{
    pimpl->action = action;

    // The following commented line is originally used to not dismiss pie menu
    // if a checkable action is toggled so the user can continually check more
    // actions. This seems to be an undesired behavior.
    //
    // TODO: The optimal way, at least for pie menu is to group all checkable
    // actions under some pie button for selection
    //
    // if (action && !action->isCheckable())
        hide();
}

bool PieMenu::isEnabled(const char *name)
{
    if (!name || !name[0])
        return false;
    return Private::getParameterGroup()->GetInt(name, 0) >= 0;
}

void PieMenu::setEnabled(const char *name, bool enabled)
{
    if (!name || !name[0])
        return;
    int val = Private::getParameterGroup()->GetInt(name, 0);
    if (enabled && val < 0)
        Private::getParameterGroup()->RemoveInt(name);
    else if (!enabled && val >= 0)
        Private::getParameterGroup()->SetInt(name, -1);
}

PieMenu *_ActivePieMenu;
struct PieMenuGuard {
    PieMenuGuard(PieMenu *menu)
        :_menu(_ActivePieMenu)
    {
        _ActivePieMenu = menu;
    }
    ~PieMenuGuard()
    {
        _ActivePieMenu = _menu;
    }
    PieMenu *_menu;
};

void PieMenu::deactivate(bool all)
{
    if (_ActivePieMenu) {
        if (all) {
            for (auto child : _ActivePieMenu->findChildren<QMenu*>())
                child->hide();
        }
        _ActivePieMenu->hide();
        _ActivePieMenu = nullptr;
    }
}

QAction *PieMenu::exec(QMenu *menu,
                       const QPoint &pt,
                       const char *name,
                       bool forwardKeyPress,
                       bool resetOffset)
{
    PieMenu pmenu(menu, name, getMainWindow());
    PieMenuGuard guard(&pmenu);

    pmenu.pimpl->forwardKeyPress = forwardKeyPress;
    if (pmenu.pimpl->buttons.empty())
        return nullptr;

    if (resetOffset)
        pmenu.pimpl->offset = 0.;

    if (!isEnabled(pmenu.pimpl->param.c_str())) {
        const auto &actions = menu->actions();
        if (actions.isEmpty())
            return nullptr;
        QAction *sep = menu->insertSeparator(actions.front());
        QAction *actionPie = new QAction(tr("Show pie menu"), menu);
        menu->insertAction(sep, actionPie);

        QAction *action = menu->exec(pt);
        menu->removeAction(actionPie);
        delete actionPie;
        if (action && action == actionPie)
            pmenu.pimpl->enablePieMenu();
        else
            return action;
    }

    pmenu.createWinId();
    QPoint point;
    if (ViewParams::PieMenuPopup())
        point = pt;
    else
        point = getMainWindow()->mapFromGlobal(pt);
    pmenu.move(point - QPoint(pmenu.width()/2, pmenu.height()/2));
    pmenu.show();
    pmenu.setFocus();
    QEventLoop eventLoop;
    pmenu.pimpl->eventLoop = &eventLoop;
    eventLoop.exec();
    pmenu.pimpl->eventLoop = nullptr;
    return pmenu.pimpl->action;
}

void PieMenu::onFocusChanged(QWidget *, QWidget *now)
{
    for (auto w=now; w; w=w->parentWidget()) {
        if (w == this || qobject_cast<QMenu*>(w))
            return;
    }
    hide();
}

int PieMenu::radius() const
{
    return pimpl->radius;
}

void PieMenu::setRadius(int radius) const
{
    pimpl->radius = radius;
    pimpl->updateGeometries();
}

int PieMenu::animateDuration() const
{
    return pimpl->duration;
}

void PieMenu::setAnimateDuration(int duration)
{
    pimpl->duration = duration;
}

int PieMenu::fontSize() const
{
    return pimpl->fontSize;
}

void PieMenu::setFontSize(int size)
{
    if (size <= 0)
        size = getMainWindow()->font().pointSize();
    if (size == pimpl->fontSize)
        return;
    pimpl->fontSize = size;
    QFont font = this->font();
    font.setPointSize(size);
    for (auto btn : pimpl->buttons)
        btn->setFont(font);
    pimpl->updateGeometries();
}

qreal PieMenu::offset() const
{
    return pimpl->offset;
}

void PieMenu::setOffset(qreal offset)
{
    offset = pimpl->checkOffset(offset);
    if (pimpl->offset != offset) {
        pimpl->offset = offset;
        pimpl->updateVisuals();
    }
}

void PieMenu::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Escape) {
        if (ViewParams::PieMenuPopup())
            QWidget::keyPressEvent(ev);
        else
            hide();
    } else
        pimpl->keyPressEvent(ev);
}

void PieMenu::onStateChanged()
{
    if (!pimpl->isAnimating())
        setOffset(pimpl->animator->endValue().toReal());
}

void PieMenu::paintEvent(QPaintEvent *ev)
{
    pimpl->paint(ev);
}

void PieMenu::onTimer()
{
    pimpl->onTimer();
}

bool PieMenu::eventFilter(QObject *o, QEvent *ev)
{
    return pimpl->eventFilter(o, ev);
}

// ----------------------------------------------------------------------------

PieButton::PieButton(QWidget *parent)
    : QToolButton(parent)
{
}

PieButton::~PieButton()
{
}

bool PieButton::event(QEvent *ev)
{
    return QToolButton::event(ev);
        
}

void PieButton::keyPressEvent(QKeyEvent *ev)
{
    if (ev->key() == Qt::Key_Escape)
        PieMenu::deactivate();
    else
        QToolButton::keyPressEvent(ev);
}

#include "moc_PieMenu.cpp"
