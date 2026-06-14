#pragma once

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
#include <QList>
#include <QUrl>

class QOpenGLWidget;

class SoNode;
class SoCamera;
class SoRenderManager;
class SoEventManager;
class SoDirectionalLight;
class QuarterWidgetP_cachecontext;
class QAction;
class QActionGroup;
class QMenu;
class ScXMLStateMachine;
class SoScXMLStateMachine;
template <class Key, class T> class QMap;

namespace SIM { namespace Coin3D { namespace Quarter {

class EventFilter;
class InteractionMode;
class ContextMenu;

class QuarterWidgetP {
public:

  QuarterWidgetP(class QuarterWidget * master, const QOpenGLWidget * sharewidget);
  ~QuarterWidgetP();

  SoCamera * searchForCamera(SoNode * root);
  uint32_t getCacheContextId() const;
  QMenu * contextMenu();

  QList<QAction *> transparencyTypeActions() const;
  QList<QAction *> renderModeActions() const;
  QList<QAction *> stereoModeActions() const;

  QuarterWidget * const master;
  SoNode * scene;
  EventFilter * eventfilter;
  InteractionMode * interactionmode;
  SoRenderManager * sorendermanager;
  SoEventManager * soeventmanager;
  bool initialsorendermanager;
  bool initialsoeventmanager;
  SoDirectionalLight * headlight;
  QuarterWidgetP_cachecontext * cachecontext;
  bool contextmenuenabled;
  bool autoredrawenabled;
  bool interactionmodeenabled;
  bool clearzbuffer;
  bool clearwindow;
  bool addactions;
  bool processdelayqueue;
  QUrl navigationModeFile;
  SoScXMLStateMachine * currentStateMachine;
  qreal device_pixel_ratio;

  static void rendercb(void * userdata, SoRenderManager *);
  static void prerendercb(void * userdata, SoRenderManager * manager);
  static void postrendercb(void * userdata, SoRenderManager * manager);
  static void statechangecb(void * userdata, ScXMLStateMachine * statemachine, const char * stateid, SbBool enter, SbBool success);

  mutable QList<QAction *> transparencytypeactions;
  mutable QList<QAction *> rendermodeactions;
  mutable QList<QAction *> stereomodeactions;

  mutable QActionGroup * transparencytypegroup;
  mutable QActionGroup * stereomodegroup;
  mutable QActionGroup * rendermodegroup;

  mutable ContextMenu * contextmenu;

  static bool nativeEventFilter(void * message, long * result);
  void replaceGLWidget(const QOpenGLWidget * newviewport);

 private:
  QuarterWidgetP_cachecontext * findCacheContext(QuarterWidget * widget, const QOpenGLWidget * sharewidget);
  static void removeFromCacheContext(QuarterWidgetP_cachecontext * context, const QOpenGLWidget * widget);
};

}}} // namespace
