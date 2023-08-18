#ifndef QUARTER_QUARTERWIDGET_H
#define QUARTER_QUARTERWIDGET_H

/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include <Inventor/SoRenderManager.h>
#include <Inventor/actions/SoGLRenderAction.h>

#include <QColor>
#include <QGraphicsView>
#include <QUrl>
#include <QtOpenGL.h>

#include "Basic.h"


class QMenu;
class SoNode;
class SoEvent;
class SoCamera;
class SoEventManager;
class SoRenderManager;
class SoDirectionalLight;
class SoScXMLStateMachine;

namespace SIM { namespace Coin3D { namespace Quarter {

class EventFilter;
const char DEFAULT_NAVIGATIONFILE []  = "coin:///scxml/navigation/examiner.xml";

class QUARTER_DLL_API QuarterWidget : public QGraphicsView {
  typedef QGraphicsView inherited;
  Q_OBJECT

public:
  enum TransparencyType {
    SCREEN_DOOR = SoGLRenderAction::SCREEN_DOOR,
    ADD = SoGLRenderAction::ADD,
    DELAYED_ADD = SoGLRenderAction::DELAYED_ADD,
    SORTED_OBJECT_ADD = SoGLRenderAction::SORTED_OBJECT_ADD,
    BLEND = SoGLRenderAction::BLEND,
    DELAYED_BLEND = SoGLRenderAction::DELAYED_BLEND,
    SORTED_OBJECT_BLEND = SoGLRenderAction::SORTED_OBJECT_BLEND,
    SORTED_OBJECT_SORTED_TRIANGLE_ADD = SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_ADD,
    SORTED_OBJECT_SORTED_TRIANGLE_BLEND = SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND,
    NONE = SoGLRenderAction::NONE,
    SORTED_LAYERS_BLEND = SoGLRenderAction::SORTED_LAYERS_BLEND
  };

  enum RenderMode {
    AS_IS = SoRenderManager::AS_IS,
    WIREFRAME = SoRenderManager::WIREFRAME,
    WIREFRAME_OVERLAY = SoRenderManager::WIREFRAME_OVERLAY,
    POINTS = SoRenderManager::POINTS,
    HIDDEN_LINE = SoRenderManager::HIDDEN_LINE,
    BOUNDING_BOX = SoRenderManager::BOUNDING_BOX
  };

  enum StereoMode {
    MONO = SoRenderManager::MONO,
    ANAGLYPH = SoRenderManager::ANAGLYPH,
    QUAD_BUFFER = SoRenderManager::QUAD_BUFFER,
    INTERLEAVED_ROWS = SoRenderManager::INTERLEAVED_ROWS,
    INTERLEAVED_COLUMNS = SoRenderManager::INTERLEAVED_COLUMNS
  };

  Q_PROPERTY(QUrl navigationModeFile READ navigationModeFile WRITE setNavigationModeFile RESET resetNavigationModeFile) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(bool contextMenuEnabled READ contextMenuEnabled WRITE setContextMenuEnabled) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(bool headlightEnabled READ headlightEnabled WRITE setHeadlightEnabled) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(bool clearZBuffer READ clearZBuffer WRITE setClearZBuffer) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(bool clearWindow READ clearWindow WRITE setClearWindow) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(bool interactionModeEnabled READ interactionModeEnabled WRITE setInteractionModeEnabled) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(bool interactionModeOn READ interactionModeOn WRITE setInteractionModeOn) // clazy:exclude=qproperty-without-notify

  Q_PROPERTY(TransparencyType transparencyType READ transparencyType WRITE setTransparencyType) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(RenderMode renderMode READ renderMode WRITE setRenderMode) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(StereoMode stereoMode READ stereoMode WRITE setStereoMode) // clazy:exclude=qproperty-without-notify
  Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged)

  Q_ENUM(TransparencyType)
  Q_ENUM(RenderMode)
  Q_ENUM(StereoMode)


public:
  explicit QuarterWidget(QWidget * parent = nullptr, const QtGLWidget * sharewidget = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  explicit QuarterWidget(QtGLContext * context, QWidget * parent = nullptr, const QtGLWidget * sharewidget = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  explicit QuarterWidget(const QtGLFormat & format, QWidget * parent = nullptr, const QtGLWidget * shareWidget = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
  ~QuarterWidget() override;

  TransparencyType transparencyType() const;
  RenderMode renderMode() const;
  StereoMode stereoMode() const;

  void setBackgroundColor(const QColor & color);
  QColor backgroundColor() const;

  qreal devicePixelRatio() const;

  void resetNavigationModeFile();
  void setNavigationModeFile(const QUrl & url = QUrl(QString::fromLatin1(DEFAULT_NAVIGATIONFILE)));
  const QUrl & navigationModeFile() const;

  void setContextMenuEnabled(bool yes);
  bool contextMenuEnabled() const;
  QMenu * getContextMenu() const;

  bool headlightEnabled() const;
  void setHeadlightEnabled(bool onoff);
  SoDirectionalLight * getHeadlight() const;

  bool clearZBuffer() const;
  void setClearZBuffer(bool onoff);

  bool clearWindow() const;
  void setClearWindow(bool onoff);

  bool interactionModeEnabled() const;
  void setInteractionModeEnabled(bool onoff);

  bool interactionModeOn() const;
  void setInteractionModeOn(bool onoff);

  void setStateCursor(const SbName & state, const QCursor & cursor);
  QCursor stateCursor(const SbName & state);

  uint32_t getCacheContextId() const;

  virtual void setSceneGraph(SoNode * root);
  virtual SoNode * getSceneGraph() const;

  void setSoEventManager(SoEventManager * manager);
  SoEventManager * getSoEventManager() const;

  void setSoRenderManager(SoRenderManager * manager);
  SoRenderManager * getSoRenderManager() const;

  EventFilter * getEventFilter() const;

  void addStateMachine(SoScXMLStateMachine * statemachine);
  void removeStateMachine(SoScXMLStateMachine * statemachine);

  virtual bool processSoEvent(const SoEvent * event);
  QSize minimumSizeHint() const override;

  QList<QAction *> transparencyTypeActions() const;
  QList<QAction *> stereoModeActions() const;
  QList<QAction *> renderModeActions() const;

public Q_SLOTS:
  virtual void viewAll();
  virtual void seek();

  void redraw();

  void setRenderMode(SIM::Coin3D::Quarter::QuarterWidget::RenderMode mode);
  void setStereoMode(SIM::Coin3D::Quarter::QuarterWidget::StereoMode mode);
  void setTransparencyType(SIM::Coin3D::Quarter::QuarterWidget::TransparencyType type);

Q_SIGNALS:
  void devicePixelRatioChanged(qreal dev_pixel_ratio);

private Q_SLOTS:
  void replaceViewport();
  virtual void aboutToDestroyGLContext();

protected:
  void paintEvent(QPaintEvent*) override;
  void resizeEvent(QResizeEvent*) override;
  bool viewportEvent(QEvent* event) override;
  virtual void actualRedraw();
  virtual bool updateDevicePixelRatio();

private:
  void constructor(const QtGLFormat& format, const QtGLWidget* sharewidget);
  friend class QuarterWidgetP;
  class QuarterWidgetP * pimpl;
  bool initialized;
};

}}} // namespace

#endif // QUARTER_QUARTERWIDGET_H
