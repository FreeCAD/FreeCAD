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

#include <Inventor/SbBasic.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/actions/SoGLRenderAction.h>

#include <QtGui/QColor>
#include <QGraphicsView>
#include <QtCore/QUrl>
#include <QtOpenGL.h>
#include <Gui/Quarter/Basic.h>

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

  Q_PROPERTY(QUrl navigationModeFile READ navigationModeFile WRITE setNavigationModeFile RESET resetNavigationModeFile)
  Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor)
  Q_PROPERTY(bool contextMenuEnabled READ contextMenuEnabled WRITE setContextMenuEnabled)
  Q_PROPERTY(bool headlightEnabled READ headlightEnabled WRITE setHeadlightEnabled)
  Q_PROPERTY(bool clearZBuffer READ clearZBuffer WRITE setClearZBuffer)
  Q_PROPERTY(bool clearWindow READ clearWindow WRITE setClearWindow)
  Q_PROPERTY(bool interactionModeEnabled READ interactionModeEnabled WRITE setInteractionModeEnabled)
  Q_PROPERTY(bool interactionModeOn READ interactionModeOn WRITE setInteractionModeOn)

  Q_PROPERTY(TransparencyType transparencyType READ transparencyType WRITE setTransparencyType)
  Q_PROPERTY(RenderMode renderMode READ renderMode WRITE setRenderMode)
  Q_PROPERTY(StereoMode stereoMode READ stereoMode WRITE setStereoMode)
  Q_PROPERTY(qreal devicePixelRatio READ devicePixelRatio NOTIFY devicePixelRatioChanged)

  Q_ENUMS(TransparencyType)
  Q_ENUMS(RenderMode)
  Q_ENUMS(StereoMode)


public:
  explicit QuarterWidget(QWidget * parent = 0, const QtGLWidget * sharewidget = 0, Qt::WindowFlags f = Qt::WindowFlags());
  explicit QuarterWidget(QtGLContext * context, QWidget * parent = 0, const QtGLWidget * sharewidget = 0, Qt::WindowFlags f = Qt::WindowFlags());
  explicit QuarterWidget(const QtGLFormat & format, QWidget * parent = 0, const QtGLWidget * shareWidget = 0, Qt::WindowFlags f = Qt::WindowFlags());
  virtual ~QuarterWidget();

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

  TransparencyType transparencyType(void) const;
  RenderMode renderMode(void) const;
  StereoMode stereoMode(void) const;

  void setBackgroundColor(const QColor & color);
  QColor backgroundColor(void) const;

  qreal devicePixelRatio(void) const;

  void resetNavigationModeFile(void);
  void setNavigationModeFile(const QUrl & url = QUrl(QString::fromLatin1(DEFAULT_NAVIGATIONFILE)));
  const QUrl & navigationModeFile(void) const;

  void setContextMenuEnabled(bool yes);
  bool contextMenuEnabled(void) const;
  QMenu * getContextMenu(void) const;

  bool headlightEnabled(void) const;
  void setHeadlightEnabled(bool onoff);
  SoDirectionalLight * getHeadlight(void) const;

  bool clearZBuffer(void) const;
  void setClearZBuffer(bool onoff);

  bool clearWindow(void) const;
  void setClearWindow(bool onoff);

  bool interactionModeEnabled(void) const;
  void setInteractionModeEnabled(bool onoff);

  bool interactionModeOn(void) const;
  void setInteractionModeOn(bool onoff);

  void setStateCursor(const SbName & state, const QCursor & cursor);
  QCursor stateCursor(const SbName & state);

  uint32_t getCacheContextId(void) const;

  virtual void setSceneGraph(SoNode * root);
  virtual SoNode * getSceneGraph(void) const;

  void setSoEventManager(SoEventManager * manager);
  SoEventManager * getSoEventManager(void) const;

  void setSoRenderManager(SoRenderManager * manager);
  SoRenderManager * getSoRenderManager(void) const;

  EventFilter * getEventFilter(void) const;

  void addStateMachine(SoScXMLStateMachine * statemachine);
  void removeStateMachine(SoScXMLStateMachine * statemachine);

  virtual bool processSoEvent(const SoEvent * event);
  virtual QSize minimumSizeHint(void) const;

  QList<QAction *> transparencyTypeActions(void) const;
  QList<QAction *> stereoModeActions(void) const;
  QList<QAction *> renderModeActions(void) const;

public Q_SLOTS:
  virtual void viewAll(void);
  virtual void seek(void);

  void redraw(void);

  void setRenderMode(RenderMode mode);
  void setStereoMode(StereoMode mode);
  void setTransparencyType(TransparencyType type);

Q_SIGNALS:
  void devicePixelRatioChanged(qreal dev_pixel_ratio);

private Q_SLOTS:
  void replaceViewport();
  virtual void aboutToDestroyGLContext();

protected:
  virtual void paintEvent(QPaintEvent*);
  virtual void resizeEvent(QResizeEvent*);
  virtual bool viewportEvent(QEvent* event);
  virtual void actualRedraw(void);
  virtual bool updateDevicePixelRatio(void);

private:
  void constructor(const QtGLFormat& format, const QtGLWidget* sharewidget);
  friend class QuarterWidgetP;
  class QuarterWidgetP * pimpl;
  bool initialized;
};

}}} // namespace

#endif // QUARTER_QUARTERWIDGET_H
