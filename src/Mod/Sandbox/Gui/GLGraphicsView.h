/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_GRAPHICSVIEW_H
#define GUI_GRAPHICSVIEW_H

#include <QGraphicsScene>
#include <QTime>
#include <Gui/MDIView.h>

class QGraphicsView;
class QDialog;
class QLabel;
class SoCamera;
class SoSeparator;

namespace Gui {

class /*GuiExport*/ GraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    GraphicsScene();
    virtual ~GraphicsScene();

    void drawBackground(QPainter *painter, const QRectF &rect);

    void enableWireframe(bool enabled);
    void enableNormals(bool enabled);
    void setModelColor();
    void setBackgroundColor();
    void loadModel();
    void loadModel(const QString &filePath);
    void modelLoaded();
    void viewAll();
    SoSeparator* getSceneGraph() const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent);

private:
    QDialog *createDialog(const QString &windowTitle) const;

    bool m_wireframeEnabled;
    bool m_normalsEnabled;

    QColor m_modelColor;
    QColor m_backgroundColor;

    QTime m_time;
    int m_lastTime;
    int m_mouseEventTime;

    float m_distance;

    QLabel *m_labels[4];
    QWidget *m_modelButton;

    SoSeparator* rootNode;
    mutable SoSeparator* sceneNode;
    SoCamera* sceneCamera;

    QGraphicsRectItem *m_lightItem;
};

class /*GuiExport*/ GraphicsView3D : public Gui::MDIView
{
    Q_OBJECT

public:
    GraphicsView3D(Gui::Document* doc, QWidget* parent = 0);
    virtual ~GraphicsView3D();
    GraphicsScene* getScene()
    { return m_Scene; }

private:
    GraphicsScene *m_Scene;
    QGraphicsView *m_view;
};

} // namespace Gui

#endif  // GUI_GRAPHICSVIEW_H

