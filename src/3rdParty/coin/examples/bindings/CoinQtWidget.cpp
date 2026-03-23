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

#include <qtimer.h>
#include <qapplication.h>
#include <qhbox.h>
#include <qvbox.h>
#include <qdialog.h>
#include <qpushbutton.h>

#include <Inventor/SoDB.h>
#include <Inventor/SbTime.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoCone.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/SoSceneManager.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/events/SoEvents.h>
#include <Inventor/errors/SoDebugError.h>

#include <Inventor/system/gl.h>

#include <CoinQtWidget.h>

// *************************************************************************
// TODO
// - unique cachecontexts for each widget
// - interaction camera control, lighting, etc. - superscene management
// - improve idle-sensor handling / framerate control
// - documentation

// *************************************************************************
// CoinQtManager
// *************************************************************************

class CoinQtManager {
public:
  static CoinQtManager * getManager(void);

  void registerWidget(CoinQtWidget * widget);
  void unregisterWidget(CoinQtWidget * widget);

  void update(void);

protected:
  CoinQtManager(void);

private:
  static CoinQtManager * singleton;

  QTimer * timer;

}; // CoinQtManager

CoinQtManager * CoinQtManager::singleton = NULL;

CoinQtManager::CoinQtManager(void)
{
  CoinQtManager::singleton = this;
  this->timer = new QTimer;
  this->timer->start(100);
}

CoinQtManager *
CoinQtManager::getManager(void)
{
  if ( !CoinQtManager::singleton ) {
    // first widget is set up
    CoinQtManager::singleton = new CoinQtManager;
    assert(CoinQtManager::singleton);
    SoDB::setRealTimeInterval(SbTime(1.0/50.0));
  }
  return CoinQtManager::singleton;
}

void
CoinQtManager::registerWidget(CoinQtWidget * widget)
{
  QObject::connect(this->timer, SIGNAL(timeout()), widget, SLOT(tick()));
}

void
CoinQtManager::unregisterWidget(CoinQtWidget * widget)
{
  QObject::disconnect(this->timer, SIGNAL(timeout()), widget, SLOT(tick()));
}

void
CoinQtManager::update(void)
{
  SoSensorManager * sensormanager = SoDB::getSensorManager();
  sensormanager->processDelayQueue(TRUE);
  sensormanager->processTimerQueue();
}

// *************************************************************************
// CoinQtWidget code
// *************************************************************************

class CoinQtWidgetP {
protected:
  friend class CoinQtWidget;

  CoinQtWidgetP(CoinQtWidget * master);
  ~CoinQtWidgetP(void);

  CoinQtWidget * master;
  SoSeparator * root;
  SoNode * publicroot;

  SoSceneManager * scenemanager;

  static void renderCB(void * closure, SoSceneManager * manager);
  void render(SoSceneManager * manager);

  static const char * superscene[];
  static const char * aboutscene[];

  SoLocation2Event * location2;
  SoMouseButtonEvent * mousebutton;
  SoKeyboardEvent * keyboard;

}; // CoinQtWidgetP

const char *
CoinQtWidgetP::superscene[] = {
  "#Inventor V2.1 ascii\n",
  "\n",
  "DEF root Separator {\n",
  "  DEF light DirectionalLight {\n",
  "    direction 0 0 -1\n",
  "    intensity 1.0\n",
  "  }\n",
  "  DEF camera PerspectiveCamera {\n",
  "  }\n",
  "}\n",
  NULL
};

const char *
CoinQtWidgetP::aboutscene[] = {
  "#Inventor V2.1 ascii\n",
  "\n",
  "Separator {\n",
  "  RotationXYZ {\n",
  "    axis X\n",
  "    angle -1.56\n",
  "  }\n",
  "  Rotor {\n",
  "    speed 0.10\n",
  "  }\n",
  "  RotationXYZ {\n",
  "    axis X\n",
  "    angle 1.56\n",
  "  }\n",
  "  Cone {\n",
  "  }\n",
  "}\n",
  NULL
};

CoinQtWidgetP::CoinQtWidgetP(CoinQtWidget * master)
{
  this->master = master;
  this->root = NULL;
  this->publicroot = NULL;
  this->scenemanager = new SoSceneManager;
  this->scenemanager->setRenderCallback(CoinQtWidgetP::renderCB, this);
  this->scenemanager->activate();
  this->location2 = new SoLocation2Event;
  this->keyboard = new SoKeyboardEvent;
  this->mousebutton = new SoMouseButtonEvent;
}

CoinQtWidgetP::~CoinQtWidgetP(void)
{
  if ( this->root ) {
    this->root->unref();
    this->root = NULL;
  }
  delete this->scenemanager;
  this->scenemanager = NULL;
  delete this->location2;
  delete this->keyboard;
  delete this->mousebutton;
}

void
CoinQtWidgetP::render(SoSceneManager * manager)
{
  assert(manager == this->scenemanager);
  this->master->makeCurrent();
  this->scenemanager->render(TRUE, TRUE);
  if ( this->master->doubleBuffer() ) {
    this->master->swapBuffers();
  }
}

void
CoinQtWidgetP::renderCB(void * closure, SoSceneManager * manager)
{
  assert(closure);
  CoinQtWidgetP * thisp = (CoinQtWidgetP *) closure;
  thisp->render(manager);
}

// *************************************************************************

#define PRIVATE(obj) ((obj)->internals)

CoinQtWidget::CoinQtWidget(QWidget * parent, const char * name,
                           const QGLWidget * shareWidget, WFlags f)
: inherited(parent, name, shareWidget, f),
  internals(NULL)
{
  PRIVATE(this) = new CoinQtWidgetP(this);
  CoinQtManager * manager = CoinQtManager::getManager();
  manager->registerWidget(this);
  this->setMouseTracking(true);
  this->setFocusPolicy(QWidget::StrongFocus);
}

CoinQtWidget::CoinQtWidget(const QGLFormat & format, QWidget * parent,
                           const char * name,
                           const QGLWidget * shareWidget, WFlags f)
: inherited(format, parent, name, shareWidget, f), internals(NULL)
{
  PRIVATE(this) = new CoinQtWidgetP(this);
  CoinQtManager * manager = CoinQtManager::getManager();
  manager->registerWidget(this);
  this->setMouseTracking(true);
  this->setFocusPolicy(QWidget::StrongFocus);
}

CoinQtWidget::~CoinQtWidget(void)
{
  CoinQtManager * manager = CoinQtManager::getManager();
  manager->unregisterWidget(this);
  delete PRIVATE(this);
  PRIVATE(this) = NULL;
}

SbBool
CoinQtWidget::setSceneGraph(SoNode * node, SbBool managed)
{
  if ( node == NULL ) {
    if ( PRIVATE(this)->publicroot ) {
      PRIVATE(this)->publicroot->unref();
      PRIVATE(this)->publicroot = NULL;
      PRIVATE(this)->root->unref();
      PRIVATE(this)->root = NULL;
      PRIVATE(this)->scenemanager->setSceneGraph(NULL);
    }
    return TRUE;
  } else {
    this->setSceneGraph(NULL);
  }
  if ( managed ) {
    printf("managed scene graph\n");
    // no PRIVATE(this)->root node
    PRIVATE(this)->publicroot = node;
    PRIVATE(this)->publicroot->ref();
  } else {
    SoInput in;
    in.setStringArray(CoinQtWidgetP::superscene);
    SoNode * scene = NULL;
    SbBool status = SoDB::read(&in, scene);
    if ( !status ) {
      assert(0);
      return FALSE;
    }
    scene->ref(); // actions are applied later

    SoSearchAction sa;

    // get desired root node
    sa.setInterest(SoSearchAction::FIRST);
    sa.setName(SbName("root"));
    sa.apply(scene);
    if ( !sa.getPath() ) {
      assert(0 && "no root");
      return FALSE;
    }

    SoNode * root = sa.getPath()->getTail();
    if ( !root->isOfType(SoSeparator::getClassTypeId()) ) {
      assert(0 && "invalid root type");
      return FALSE;
    }
    PRIVATE(this)->root = (SoSeparator *) root;
    PRIVATE(this)->root->ref();
    root = NULL;

    scene->unref();
    scene = NULL;

    sa.reset();
    sa.setInterest(SoSearchAction::FIRST);
    sa.setType(SoCamera::getClassTypeId());
    sa.apply(PRIVATE(this)->root);
    if ( !sa.getPath() ) {
      assert(0 && "no camera");
      return FALSE;
    }

    assert(sa.getPath()->getTail()->isOfType(SoCamera::getClassTypeId()));
    SoCamera * camera = (SoCamera *) sa.getPath()->getTail();

    PRIVATE(this)->publicroot = node;
    PRIVATE(this)->publicroot->ref();
    PRIVATE(this)->root->addChild(node);

    camera->viewAll(PRIVATE(this)->publicroot, PRIVATE(this)->scenemanager->getViewportRegion());

  }
  if ( PRIVATE(this)->root ) {
    PRIVATE(this)->scenemanager->setSceneGraph(PRIVATE(this)->root);
  } else {
    PRIVATE(this)->scenemanager->setSceneGraph(PRIVATE(this)->publicroot);
  }
  return TRUE;
}

SoNode *
CoinQtWidget::getSceneGraph(void) const
{
  return PRIVATE(this)->publicroot;
}

void
CoinQtWidget::updateGL(void)
{
  // FIXME: when is this one invoked?
  printf("updateGL()\n");
  inherited::updateGL();
}

void
CoinQtWidget::resizeGL(int width, int height)
{
  inherited::resizeGL(width, height);
  PRIVATE(this)->scenemanager->setViewportRegion(SbViewportRegion(width, height));
}

void
CoinQtWidget::initializeGL(void)
{
  // FIXME: assert context is active
  inherited::initializeGL();
  glEnable(GL_DEPTH_TEST);
}

void
CoinQtWidget::paintGL(void)
{
  inherited::paintGL();
  PRIVATE(this)->scenemanager->render(TRUE, TRUE);
}

void
CoinQtWidget::tick(void)
{
  CoinQtManager * manager = CoinQtManager::getManager();
  manager->update();
}

void
CoinQtWidget::glInit(void)
{
  // printf("glInit()\n");
  inherited::glInit();
}

void
CoinQtWidget::glDraw(void)
{
  // printf("glDraw()\n");
  inherited::glDraw();
}

// *************************************************************************

// shift, control, alt

void
CoinQtWidget::mousePressEvent(QMouseEvent * event)
{
  PRIVATE(this)->mousebutton->setTime(SbTime());
  PRIVATE(this)->mousebutton->setPosition(PRIVATE(this)->location2->getPosition());
  PRIVATE(this)->mousebutton->setShiftDown(PRIVATE(this)->keyboard->wasShiftDown());
  PRIVATE(this)->mousebutton->setCtrlDown(PRIVATE(this)->keyboard->wasCtrlDown());
  PRIVATE(this)->mousebutton->setAltDown(PRIVATE(this)->keyboard->wasAltDown());

  PRIVATE(this)->mousebutton->setState(SoButtonEvent::DOWN);
  PRIVATE(this)->mousebutton->setButton(SoMouseButtonEvent::ANY);
  switch ( event->button() ) {
  case Qt::LeftButton:
    PRIVATE(this)->mousebutton->setButton(SoMouseButtonEvent::BUTTON1);
    break;
  case Qt::RightButton:
    PRIVATE(this)->mousebutton->setButton(SoMouseButtonEvent::BUTTON2);
    break;
  case Qt::MidButton:
    PRIVATE(this)->mousebutton->setButton(SoMouseButtonEvent::BUTTON3);
    break;
  default:
    SoDebugError::postInfo("CoinQtWidget::mousePressEvent",
                           "Unhandled ButtonState = %x", event->button());
    break;
  }
  PRIVATE(this)->scenemanager->processEvent(PRIVATE(this)->mousebutton);
}

void
CoinQtWidget::mouseReleaseEvent(QMouseEvent * event)
{
  PRIVATE(this)->mousebutton->setTime(SbTime());
  PRIVATE(this)->mousebutton->setPosition(PRIVATE(this)->location2->getPosition());
  PRIVATE(this)->mousebutton->setShiftDown(PRIVATE(this)->keyboard->wasShiftDown());
  PRIVATE(this)->mousebutton->setCtrlDown(PRIVATE(this)->keyboard->wasCtrlDown());
  PRIVATE(this)->mousebutton->setAltDown(PRIVATE(this)->keyboard->wasAltDown());

  PRIVATE(this)->mousebutton->setState(SoButtonEvent::UP);
  PRIVATE(this)->mousebutton->setButton(SoMouseButtonEvent::ANY);
  switch ( event->button() ) {
  case Qt::LeftButton:
    PRIVATE(this)->mousebutton->setButton(SoMouseButtonEvent::BUTTON1); break;
  case Qt::RightButton:
    PRIVATE(this)->mousebutton->setButton(SoMouseButtonEvent::BUTTON2); break;
  case Qt::MidButton:
    PRIVATE(this)->mousebutton->setButton(SoMouseButtonEvent::BUTTON3); break;
  default:
    SoDebugError::postInfo("CoinQtWidget::mouseReleaseEvent",
                           "Unhandled ButtonState = %x", event->button());
    break;
  }
  PRIVATE(this)->scenemanager->processEvent(PRIVATE(this)->mousebutton);
}

void
CoinQtWidget::mouseMoveEvent(QMouseEvent * event)
{
  PRIVATE(this)->location2->setTime(SbTime());
  PRIVATE(this)->location2->setShiftDown(PRIVATE(this)->keyboard->wasShiftDown());
  PRIVATE(this)->location2->setCtrlDown(PRIVATE(this)->keyboard->wasCtrlDown());
  PRIVATE(this)->location2->setAltDown(PRIVATE(this)->keyboard->wasAltDown());

  SbVec2s pos(event->pos().x(), event->pos().y());
  pos[1] = PRIVATE(this)->scenemanager->getViewportRegion().getWindowSize()[1]
           - pos[1] - 1;
  PRIVATE(this)->location2->setPosition(pos);
  PRIVATE(this)->scenemanager->processEvent(PRIVATE(this)->location2);

  // FIXME: control focusin/focusout based on location?
}

static
void
setKey(SoKeyboardEvent * coinevent, QKeyEvent * qtevent)
{
  // FIXME: complete the key translation table
  switch ( qtevent->key() ) {
  case Qt::Key_Escape: coinevent->setKey(SoKeyboardEvent::ESCAPE); break;
  case Qt::Key_Space: coinevent->setKey(SoKeyboardEvent::SPACE); break;
  case Qt::Key_Tab: coinevent->setKey(SoKeyboardEvent::TAB); break;
  case Qt::Key_Backspace: coinevent->setKey(SoKeyboardEvent::BACKSPACE); break;
  case Qt::Key_Delete: coinevent->setKey(SoKeyboardEvent::KEY_DELETE); break;

  case Qt::Key_A: coinevent->setKey(SoKeyboardEvent::A); break;
  case Qt::Key_B: coinevent->setKey(SoKeyboardEvent::B); break;
  case Qt::Key_C: coinevent->setKey(SoKeyboardEvent::C); break;
  case Qt::Key_D: coinevent->setKey(SoKeyboardEvent::D); break;
  case Qt::Key_E: coinevent->setKey(SoKeyboardEvent::E); break;
  case Qt::Key_F: coinevent->setKey(SoKeyboardEvent::F); break;
  case Qt::Key_G: coinevent->setKey(SoKeyboardEvent::G); break;
  case Qt::Key_H: coinevent->setKey(SoKeyboardEvent::H); break;
  case Qt::Key_I: coinevent->setKey(SoKeyboardEvent::I); break;
  case Qt::Key_J: coinevent->setKey(SoKeyboardEvent::J); break;
  case Qt::Key_K: coinevent->setKey(SoKeyboardEvent::K); break;
  case Qt::Key_L: coinevent->setKey(SoKeyboardEvent::L); break;
  case Qt::Key_M: coinevent->setKey(SoKeyboardEvent::M); break;
  case Qt::Key_N: coinevent->setKey(SoKeyboardEvent::N); break;
  case Qt::Key_O: coinevent->setKey(SoKeyboardEvent::O); break;
  case Qt::Key_P: coinevent->setKey(SoKeyboardEvent::P); break;
  case Qt::Key_Q: coinevent->setKey(SoKeyboardEvent::Q); break;
  case Qt::Key_R: coinevent->setKey(SoKeyboardEvent::R); break;
  case Qt::Key_S: coinevent->setKey(SoKeyboardEvent::S); break;
  case Qt::Key_T: coinevent->setKey(SoKeyboardEvent::T); break;
  case Qt::Key_U: coinevent->setKey(SoKeyboardEvent::U); break;
  case Qt::Key_V: coinevent->setKey(SoKeyboardEvent::V); break;
  case Qt::Key_W: coinevent->setKey(SoKeyboardEvent::W); break;
  case Qt::Key_X: coinevent->setKey(SoKeyboardEvent::X); break;
  case Qt::Key_Y: coinevent->setKey(SoKeyboardEvent::Y); break;
  case Qt::Key_Z: coinevent->setKey(SoKeyboardEvent::Z); break;

  case Qt::Key_0: coinevent->setKey(SoKeyboardEvent::NUMBER_0); break;
  case Qt::Key_1: coinevent->setKey(SoKeyboardEvent::NUMBER_1); break;
  case Qt::Key_2: coinevent->setKey(SoKeyboardEvent::NUMBER_2); break;
  case Qt::Key_3: coinevent->setKey(SoKeyboardEvent::NUMBER_3); break;
  case Qt::Key_4: coinevent->setKey(SoKeyboardEvent::NUMBER_4); break;
  case Qt::Key_5: coinevent->setKey(SoKeyboardEvent::NUMBER_5); break;
  case Qt::Key_6: coinevent->setKey(SoKeyboardEvent::NUMBER_6); break;
  case Qt::Key_7: coinevent->setKey(SoKeyboardEvent::NUMBER_7); break;
  case Qt::Key_8: coinevent->setKey(SoKeyboardEvent::NUMBER_8); break;
  case Qt::Key_9: coinevent->setKey(SoKeyboardEvent::NUMBER_9); break;

  // no way to distinguish between left and right
  case Qt::Key_Shift: coinevent->setKey(SoKeyboardEvent::LEFT_SHIFT); break;
  case Qt::Key_Control: coinevent->setKey(SoKeyboardEvent::LEFT_CONTROL); break;
  case Qt::Key_Alt: coinevent->setKey(SoKeyboardEvent::LEFT_ALT); break;

  // can't handle them all...
  default: break;
  }

  // and the modifiers...
  coinevent->setShiftDown(qtevent->stateAfter() & Qt::ShiftButton);
  coinevent->setCtrlDown(qtevent->stateAfter() & Qt::ControlButton);
  coinevent->setAltDown(qtevent->stateAfter() & Qt::AltButton);
}

void
CoinQtWidget::keyPressEvent(QKeyEvent * event)
{
  PRIVATE(this)->keyboard->setTime(SbTime());
  PRIVATE(this)->keyboard->setPosition(PRIVATE(this)->location2->getPosition());

  PRIVATE(this)->keyboard->setState(SoButtonEvent::DOWN);
  PRIVATE(this)->keyboard->setKey(SoKeyboardEvent::ANY);
  setKey(PRIVATE(this)->keyboard, event);

  PRIVATE(this)->scenemanager->processEvent(PRIVATE(this)->keyboard);
}

void
CoinQtWidget::keyReleaseEvent(QKeyEvent * event)
{
  PRIVATE(this)->keyboard->setTime(SbTime());
  PRIVATE(this)->keyboard->setPosition(PRIVATE(this)->location2->getPosition());

  PRIVATE(this)->keyboard->setState(SoButtonEvent::UP);
  PRIVATE(this)->keyboard->setKey(SoKeyboardEvent::ANY);
  setKey(PRIVATE(this)->keyboard, event);

  PRIVATE(this)->scenemanager->processEvent(PRIVATE(this)->keyboard);
}

// *************************************************************************

void
CoinQtWidget::about(void)
{
  QDialog * dialog = new QDialog(NULL, "About");
  dialog->setFixedSize(420, 240);

  QPushButton * close = new QPushButton(dialog, "About::Close");
  close->setText("Close");
  QRect geom = close->rect();
  close->move(210 - (geom.width() / 2), 230 - geom.height());

  CoinQtWidget * logo = new CoinQtWidget(dialog);
  logo->setGeometry(10, 10, 400, 210 - geom.height() - 1);

  QObject::connect(close, SIGNAL(clicked()), dialog, SLOT(close()));

  SoInput in;
  in.setStringArray(CoinQtWidgetP::aboutscene);
  SoNode * scene = NULL;
  SbBool status = SoDB::read(&in, scene);
  if ( !status ) {
    assert(0);
    return;
  }
  logo->setSceneGraph(scene);

  dialog->show();
  dialog->exec();
  dialog->hide();
  delete dialog;
}

// *************************************************************************

#undef PRIVATE
