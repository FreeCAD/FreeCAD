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
#include <Base/Parameter.h>

class QUrl;
class QGraphicsView;
class QDialog;
class QLabel;
class SoCamera;
class SoSeparator;
class SoRenderManager;

namespace SIM { namespace Coin3D { namespace Quarter {
class InputDevice;
}}}

namespace Gui {

class SceneEventFilter : public QObject
{
    Q_OBJECT

public:
    SceneEventFilter(QObject * parent);
    ~SceneEventFilter();

    void registerInputDevice(SIM::Coin3D::Quarter::InputDevice * device);
    void unregisterInputDevice(SIM::Coin3D::Quarter::InputDevice * device);

    const QPoint & globalMousePosition(void) const;

protected:
    bool eventFilter(QObject * obj, QEvent * event);

private:
    class Private;
    Private* pimpl;
};

class /*GuiExport*/ GraphicsScene : public QGraphicsScene
{
    Q_OBJECT

public:
    GraphicsScene();
    virtual ~GraphicsScene();

    void drawBackground(QPainter *painter, const QRectF &rect);

    void setBackgroundColor(const QColor&);
    void viewAll();

    void setSceneGraph(SoNode * node);
    SoNode* getSceneGraph() const;

    SceneEventFilter *
    getEventFilter(void) const;

    void addStateMachine(SoScXMLStateMachine * statemachine);
    void removeStateMachine(SoScXMLStateMachine * statemachine);
    void setNavigationModeFile(const QUrl&);

    bool processSoEvent(const SoEvent * event);

    SoRenderManager *
    getSoRenderManager(void) const;

    SoEventManager *
    getSoEventManager(void) const;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void wheelEvent(QGraphicsSceneWheelEvent * wheelEvent);

private Q_SLOTS:
    void onSceneRectChanged(const QRectF & rect);

private:
    QDialog *createDialog(const QString &windowTitle) const;
    SoCamera* searchForCamera(SoNode * root);

private:
    QColor m_backgroundColor;

    QTime m_time;
    int m_lastTime;
    int m_mouseEventTime;

    float m_distance;

    QLabel *m_labels[4];
    QWidget *m_modelButton;

    mutable SoNode* sceneNode;
    SoNode* headlight;

    QGraphicsRectItem *m_lightItem;
    SoRenderManager* sorendermanager;
    SoEventManager* soeventmanager;
    SceneEventFilter* eventfilter;
};

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView();
    ~GraphicsView();

protected:
    void resizeEvent(QResizeEvent *event);
    bool viewportEvent(QEvent* event);
};

class /*GuiExport*/ GraphicsView3D : public Gui::MDIView
                                   , public ParameterGrp::ObserverType
{
    Q_OBJECT

public:
    GraphicsView3D(Gui::Document* doc, QWidget* parent = 0);
    virtual ~GraphicsView3D();
    GraphicsScene* getScene()
    { return m_scene; }

    virtual void OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason);

private:
    GraphicsScene *m_scene;
    QGraphicsView *m_view;
    ParameterGrp::handle hGrp;
};

} // namespace Gui

#endif  // GUI_GRAPHICSVIEW_H

