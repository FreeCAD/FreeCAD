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
#endif
#include <Inventor/SbVec2s.h>

#include "Flag.h"

using namespace Gui;

#if 0 // Test functions with transparency

#if 1
    QDialog* dlg = Gui::getMainWindow()->findChild<QDialog*>();
    QImage image;
    if (dlg) {
        QPixmap p = QPixmap::grabWidget(dlg);
        image = p.toImage();
    }
    else {
        QImage img(128,128, QImage::Format_ARGB32);
        img.fill(qRgba(255, 255, 255, 127));
        QPainter painter;
        painter.begin(&img);
        painter.setPen(Qt::black);
        painter.drawText(25, 50, QLatin1String("Hello, World!"));
        painter.end();
        image = img;
    }
#else
    QPixmap pm (128,128);
    QBitmap mask (128,128);
    mask.fill(Qt::color0);

    QPainter painter(&mask);
    painter.drawText(QPoint(0, 0), QLatin1String("Hello, World!"));
    pm.setMask(mask);

    QImage img = pm.toImage();
    img.load(QLatin1String("C:/Temp/tux.png"),"PNG");
#endif

#include "MainWindow.h"
void drawImage(QGLWidget* w,double x1, double y1, double x2, double y2, QImage pic)
{
    //pic.save(QLatin1String("C:/Temp/texture.png"),"PNG");
#if 0
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(0.0,0.0,1.0,0.2f);
    glBegin(GL_QUADS);
    glTexCoord2d(0,0); glVertex2f(x1,y1);
    glTexCoord2d(1,0); glVertex2f(x2,y1);
    glTexCoord2d(1,1); glVertex2f(x2,y2);
    glTexCoord2d(0,1); glVertex2f(x1,y2);
    glEnd();
    glPopAttrib();
#elif 0
    pic = QGLWidget::convertToGLFormat(pic);
    int texid = w->bindTexture(pic);
    glColor3f(1,1,1);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    glTexCoord2d(0,0); glVertex2f(x1,y1);
    glTexCoord2d(1,0); glVertex2f(x2,y1);
    glTexCoord2d(1,1); glVertex2f(x2,y2);
    glTexCoord2d(0,1); glVertex2f(x1,y2);
    glEnd();
    //    glEnable(GL_LIGHTING);
    w->deleteTexture(texid);
#elif 0
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(0.0,0.0,1.0,0.2f);
    int texid = w->bindTexture(pic);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glDisable(GL_LIGHTING);
    glBegin(GL_QUADS);
    glTexCoord2d(0,0); glVertex2f(x1,y1);
    glTexCoord2d(1,0); glVertex2f(x2,y1);
    glTexCoord2d(1,1); glVertex2f(x2,y2);
    glTexCoord2d(0,1); glVertex2f(x1,y2);
    glEnd();
    //    glEnable(GL_LIGHTING);
    w->deleteTexture(texid);
    glPopAttrib();
#elif 1
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_BLEND);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(0.0,0.0,1.0,0.2f);
    pic = QGLWidget::convertToGLFormat(pic);
    glRasterPos2d(x1,y1);
    glDrawPixels(pic.width(),pic.height(),GL_RGBA,GL_UNSIGNED_BYTE,pic.bits());
    glPopAttrib();
#endif
}

#endif

/* TRANSLATOR Gui::Flag */

#if 1

// TODO: Rename to Annotation
//       Support transparency
//       Embed complete widgets

Flag::Flag(QWidget* parent)
  : QGLWidget(parent), coord(0.0f, 0.0f, 0.0f)
{
    this->setFixedHeight(20);
}

void Flag::initializeGL()
{
    const QPalette& p = this->palette();
    qglClearColor(/*Qt::white*/p.color(QPalette::Window));
}

void Flag::paintGL()
{
    const QPalette& p = this->palette();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    qglColor(/*Qt::black*/p.color(QPalette::Text));
    renderText(10,15,this->text);
}

void Flag::resizeGL(int width, int height)
{
    return;
    //int side = qMin(width, height);
    //glViewport((width - side) / 2, (height - side) / 2, side, side);

    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
    //glFrustum(-1.0, +1.0, -1.0, 1.0, 5.0, 60.0);
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    //glTranslated(0.0, 0.0, -40.0);
}

#else

Flag::Flag(QWidget* parent)
  : QWidget(parent), coord(0.0f, 0.0f, 0.0f)
{
    this->setFixedHeight(20);
}
#endif

Flag::~Flag()
{
}

void Flag::setOrigin(const SbVec3f& v)
{
    this->coord = v;
}

const SbVec3f& Flag::getOrigin() const
{
    return this->coord;
}

void Flag::drawLine (int tox, int toy)
{
    if (!isVisible())
        return;

    // Get position of line
    QSize s = parentWidget()->size();
    SbVec2s view(s.width(), s.height());
    int fromx = pos().x();
    int fromy = pos().y() + height()/2;
    if (false) fromx += width();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, view[0], 0, view[1], -1, 1);

    // Store GL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    GLfloat depthrange[2];
    glGetFloatv(GL_DEPTH_RANGE, depthrange);
    GLdouble projectionmatrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projectionmatrix);

    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glDepthRange(0,0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND);

    glColor4f(1.0, 1.0, 1.0, 0.0);
    glViewport(0, 0, view[0], view[1]);

    // the line
    glLineWidth(1.0f);
    glBegin(GL_LINE_LOOP);
        glVertex3i(fromx, view[1]-fromy, 0);
        glVertex3i(tox  , view[1]-toy  , 0);
    glEnd();

    glPointSize(3.0f);
    glBegin(GL_POINTS);
        glVertex3i(tox  , view[1]-toy  , 0);
    glEnd();

    glFlush();

    // Reset original state
    glDepthRange(depthrange[0], depthrange[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projectionmatrix);

    glPopAttrib();
    glPopMatrix();
}

void Flag::setText(const QString& t)
{
    this->text = t;
}

void Flag::resizeEvent(QResizeEvent * e)
{
#if 0
    image = QImage(this->size(), QImage::Format_ARGB32);

    QPainter painter;
    painter.begin(&image);
    painter.fillRect(image.rect(), Qt::white);
    painter.setPen(Qt::black);
    painter.drawText(10, 15, this->text);
    painter.end();
#endif
}

void Flag::paintEvent(QPaintEvent* e)
{
#if 1
    QGLWidget::paintEvent(e);
#else
#if 1
    QPainter painter;
    painter.begin(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.drawImage((width() - image.width())/2, 0, image);
    painter.end();
#else
    // draw the overlayed text using QPainter
    QPainter p(this);
    p.fillRect(this->rect(), Qt::white);
    p.setPen(Qt::black);
    p.setBrush(Qt::NoBrush);
    QFontMetrics fm(p.font());
    p.drawText(10, 15, this->text);
#endif
#endif
}

void Flag::mouseMoveEvent(QMouseEvent *e)
{
    if (e->buttons() & Qt::LeftButton) {
        move(e->globalPos() - dragPosition);
        e->accept();
    }
}

void Flag::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        dragPosition = e->globalPos() - frameGeometry().topLeft();
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
    return QSize(w, h);
}

// ------------------------------------------------------------------------

FlagLayout::FlagLayout(QWidget *parent, int margin, int spacing)
    : QLayout(parent)
{
    setMargin(margin);
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
        return 0;
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
    for (int i = 0; i < list.size(); ++i) {
        ItemWrapper *wrapper = list.at(i);
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
    for (int i = 0; i < list.size(); ++i) {
        ItemWrapper *wrapper = list.at(i);
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
    return 0;
}

void FlagLayout::add(QLayoutItem *item, Position position)
{
    list.append(new ItemWrapper(item, position));
}

QSize FlagLayout::calculateSize(SizeType sizeType) const
{
    QSize totalSize;

    for (int i = 0; i < list.size(); ++i) {
        ItemWrapper *wrapper = list.at(i);
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

#include "moc_Flag.cpp"
