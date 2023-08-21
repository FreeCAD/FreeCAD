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
# include <QMenu>
# include <QMouseEvent>
# include <Inventor/nodes/SoCamera.h>
#endif
#include <Inventor/SbVec2s.h>

#include "Flag.h"
#include "View3DInventorViewer.h"


using namespace Gui;


/* TRANSLATOR Gui::Flag */

// TODO: Rename to Annotation
//       Support transparency
//       Embed complete widgets

Flag::Flag(QWidget* parent)
  : QtGLWidget(parent), coord(0.0f, 0.0f, 0.0f)
{
    this->setFixedHeight(20);
    setAutoFillBackground(true);
}

Flag::~Flag() = default;

void Flag::initializeGL()
{
    const QPalette& p = this->palette();
    QColor c(p.color(QPalette::Window));
    glClearColor(c.redF(), c.greenF(), c.blueF(), c.alphaF());
}

void Flag::paintGL()
{
    QOpenGLWidget::paintGL();
}

void Flag::paintEvent(QPaintEvent* e)
{
    const QPalette& p = this->palette();
    QColor c(p.color(QPalette::Text));

    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(e->rect(), p.color(QPalette::Window));
    painter.setPen(c);
    painter.drawText(10, 15, this->text);
    painter.end();
}

void Flag::resizeGL(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
}

void Flag::setOrigin(const SbVec3f& v)
{
    this->coord = v;
}

const SbVec3f& Flag::getOrigin() const
{
    return this->coord;
}

void Flag::drawLine (View3DInventorViewer* v, int tox, int toy)
{
    if (!isVisible())
        return;

    // Get position of line
    QSize s = parentWidget()->size();
    SbVec2s view(s.width(), s.height());
    int fromx = pos().x();
    int fromy = pos().y() + height()/2;
    if (false) fromx += width();

    GLPainter p;
    p.begin(v->getGLWidget());

    // the line
    p.setLineWidth(1.0f);
    p.drawLine(fromx, fromy, tox, toy);
    // the point
    p.setPointSize(3.0f);
    p.drawPoint(tox, toy);
    p.end();
}

void Flag::setText(const QString& t)
{
    this->text = t;
}

void Flag::resizeEvent(QResizeEvent* e)
{
    QtGLWidget::resizeEvent(e);
}

void Flag::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton) {
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        move(e->globalPos() - dragPosition);
#else
        move(e->globalPosition().toPoint() - dragPosition);
#endif
        e->accept();
        auto viewer = dynamic_cast<View3DInventorViewer*>(parentWidget());
        if (viewer)
            viewer->getSoRenderManager()->scheduleRedraw();
    }
}

void Flag::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
        dragPosition = e->globalPos() - frameGeometry().topLeft();
#else
        dragPosition = e->globalPosition().toPoint() - frameGeometry().topLeft();
#endif
        e->accept();
    }
}

void Flag::contextMenuEvent(QContextMenuEvent * e)
{
    QMenu menu(this);

    QAction* topleft = menu.addAction(tr("Top left"));
    topleft->setCheckable(true);
    QAction* botleft = menu.addAction(tr("Bottom left"));
    botleft->setCheckable(true);
    QAction* topright = menu.addAction(tr("Top right"));
    topright->setCheckable(true);
    QAction* botright = menu.addAction(tr("Bottom right"));
    botright->setCheckable(true);
    menu.addSeparator();
    QAction* remove = menu.addAction(tr("Remove"));
    QAction* select = menu.exec(e->globalPos());
    if (remove == select)
        this->deleteLater();
}

QSize Flag::sizeHint() const
{
    int w = 100;
    int h = 20;
    QFontMetrics metric(this->font());
    QRect r = metric.boundingRect(text);
    w = std::max<int>(w, r.width()+20);
    h = std::max<int>(h, r.height());
    return {w, h};
}

// ------------------------------------------------------------------------

FlagLayout::FlagLayout(QWidget *parent, int margin, int spacing)
    : QLayout(parent)
{
    setContentsMargins(margin, margin, margin, margin);
    setSpacing(spacing);
}

FlagLayout::FlagLayout(int spacing)
{
    setSpacing(spacing);
}

FlagLayout::~FlagLayout()
{
    QLayoutItem *l;
    while ((l = takeAt(0)))
        delete l;
}

void FlagLayout::addItem(QLayoutItem *item)
{
    add(item, TopLeft);
}

void FlagLayout::addWidget(QWidget *widget, Position position)
{
    add(new QWidgetItem(widget), position);
}

Qt::Orientations FlagLayout::expandingDirections() const
{
    return Qt::Horizontal | Qt::Vertical;
}

bool FlagLayout::hasHeightForWidth() const
{
    return false;
}

int FlagLayout::count() const
{
    return list.size();
}

QLayoutItem *FlagLayout::itemAt(int index) const
{
    ItemWrapper *wrapper = list.value(index);
    if (wrapper)
        return wrapper->item;
    else
        return nullptr;
}

QSize FlagLayout::minimumSize() const
{
    return calculateSize(MinimumSize);
}

void FlagLayout::setGeometry(const QRect &rect)
{
    int topHeight = 0;
    int bottomHeight = 0;

    QLayout::setGeometry(rect);

    // left side
    for (ItemWrapper *wrapper : list) {
        QLayoutItem *item = wrapper->item;
        Position position = wrapper->position;

        if (position == TopLeft) {
            topHeight += spacing();
            item->setGeometry(QRect(rect.x() + spacing(), topHeight,
                                    item->sizeHint().width(), item->sizeHint().height()));

            topHeight += item->geometry().height();
        } else if (position == BottomLeft) {
            bottomHeight += item->geometry().height() + spacing();
            item->setGeometry(QRect(rect.x() + spacing(), rect.height() - bottomHeight,
                                    item->sizeHint().width(), item->sizeHint().height()));
        }
    }

    // right side
    topHeight = 0;
    bottomHeight = 0;
    for (ItemWrapper *wrapper : list) {
        QLayoutItem *item = wrapper->item;
        Position position = wrapper->position;

        int rightpos = item->sizeHint().width() + spacing();
        if (position == TopRight) {
            topHeight += spacing();
            item->setGeometry(QRect(rect.x() + rect.width() - rightpos, topHeight,
                                    item->sizeHint().width(), item->sizeHint().height()));

            topHeight += item->geometry().height();
        } else if (position == BottomRight) {
            bottomHeight += item->geometry().height() + spacing();
            item->setGeometry(QRect(rect.x() + rect.width() - rightpos, rect.height() - bottomHeight,
                                    item->sizeHint().width(), item->sizeHint().height()));
        }
    }
}

QSize FlagLayout::sizeHint() const
{
    return calculateSize(SizeHint);
}

QLayoutItem *FlagLayout::takeAt(int index)
{
    if (index >= 0 && index < list.size()) {
        ItemWrapper *layoutStruct = list.takeAt(index);
        return layoutStruct->item;
    }
    return nullptr;
}

void FlagLayout::add(QLayoutItem *item, Position position)
{
    list.append(new ItemWrapper(item, position));
}

QSize FlagLayout::calculateSize(SizeType sizeType) const
{
    QSize totalSize;

    for (ItemWrapper *wrapper : list) {
        QSize itemSize;

        if (sizeType == MinimumSize)
            itemSize = wrapper->item->minimumSize();
        else // (sizeType == SizeHint)
            itemSize = wrapper->item->sizeHint();

        totalSize.rheight() += itemSize.height();
        totalSize.rwidth() = qMax<int>(totalSize.width(),itemSize.width());
    }
    return totalSize;
}


TYPESYSTEM_SOURCE_ABSTRACT(Gui::GLFlagWindow, Gui::GLGraphicsItem)

GLFlagWindow::GLFlagWindow(View3DInventorViewer* view) : _viewer(view), _flagLayout(nullptr)
{
}

GLFlagWindow::~GLFlagWindow()
{
    deleteFlags();
    if (_flagLayout)
        _flagLayout->deleteLater();
}

void GLFlagWindow::deleteFlags()
{
    if (_flagLayout) {
        int ct = _flagLayout->count();
        for (int i=0; i<ct;i++) {
            QWidget* flag = _flagLayout->itemAt(0)->widget();
            if (flag) {
                _flagLayout->removeWidget(flag);
                flag->deleteLater();
            }
        }
        if (ct > 0)
            _viewer->getSoRenderManager()->scheduleRedraw();
    }
}

void GLFlagWindow::addFlag(Flag* item, FlagLayout::Position pos)
{
    if (!_flagLayout) {
        _flagLayout = new FlagLayout(3);
        _viewer->setLayout(_flagLayout);
    }

    item->setParent(_viewer);
    _flagLayout->addWidget(item, pos);
    item->show();
    _viewer->getSoRenderManager()->scheduleRedraw();
}

void GLFlagWindow::removeFlag(Flag* item)
{
    if (_flagLayout) {
        _flagLayout->removeWidget(item);
        _viewer->getSoRenderManager()->scheduleRedraw();
    }
}

Flag* GLFlagWindow::getFlag(int index) const
{
    if (_flagLayout) {
        QWidget* flag = _flagLayout->itemAt(index)->widget();
        return qobject_cast<Flag*>(flag);
    }
    return nullptr;
}

int GLFlagWindow::countFlags() const
{
    if (_flagLayout) {
        return _flagLayout->count();
    }

    return 0;
}

void GLFlagWindow::paintGL()
{
    // draw lines for the flags
    if (_flagLayout) {
        // it can happen that the GL widget gets replaced internally (SoQt only, not with quarter) which
        // causes to destroy the FlagLayout instance
        int ct = _flagLayout->count();
        const SbViewportRegion vp = _viewer->getSoRenderManager()->getViewportRegion();
        SbVec2s size = vp.getViewportSizePixels();
        float aspectratio = float(size[0])/float(size[1]);
        SbViewVolume vv = _viewer->getSoRenderManager()->getCamera()->getViewVolume(aspectratio);
        for (int i=0; i<ct;i++) {
            Flag* flag = qobject_cast<Flag*>(_flagLayout->itemAt(i)->widget());
            if (flag) {
                SbVec3f pt = flag->getOrigin();
                vv.projectToScreen(pt, pt);
                int tox = (int)(pt[0] * size[0]);
                int toy = (int)((1.0f-pt[1]) * size[1]);
                flag->drawLine(_viewer, tox, toy);
            }
        }
    }
}

#include "moc_Flag.cpp"
