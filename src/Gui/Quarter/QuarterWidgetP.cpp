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

#ifdef _MSC_VER
#pragma warning(disable : 4267)
#endif

#include "QuarterWidgetP.h"
#include "QuarterWidget.h"
#include "eventhandlers/EventFilter.h"

#include <QActionGroup>
#include <QApplication>
#include <QCursor>
#include <QMenu>

#include <Inventor/SoEventManager.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/misc/SoContextHandler.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>
#include <Inventor/C/glue/gl.h>

#include "ContextMenu.h"
#include "NativeEvent.h"
#include "QuarterP.h"


using namespace SIM::Coin3D::Quarter;

class QuarterWidgetP_cachecontext {
public:
  uint32_t id;
  SbList <const QtGLWidget *> widgetlist;
};

static SbList <QuarterWidgetP_cachecontext *> * cachecontext_list = nullptr;

QuarterWidgetP::QuarterWidgetP(QuarterWidget * masterptr, const QtGLWidget * sharewidget)
: master(masterptr),
  scene(nullptr),
  eventfilter(nullptr),
  interactionmode(nullptr),
  sorendermanager(nullptr),
  soeventmanager(nullptr),
  initialsorendermanager(false),
  initialsoeventmanager(false),
  headlight(nullptr),
  cachecontext(nullptr),
  contextmenuenabled(true),
  autoredrawenabled(true),
  interactionmodeenabled(false),
  clearzbuffer(true),
  clearwindow(true),
  addactions(true),
  processdelayqueue(true),
  currentStateMachine(nullptr),
  device_pixel_ratio(1.0),
  transparencytypegroup(nullptr),
  stereomodegroup(nullptr),
  rendermodegroup(nullptr),
  contextmenu(nullptr)
{
  this->cachecontext = findCacheContext(masterptr, sharewidget);

  // FIXME: Centralize this as only one custom event filter can be
  // added to an application. (20101019 handegar)
#ifdef HAVE_SPACENAV_LIB // fixes #0001970
  qApp->setEventFilter(QuarterWidgetP::nativeEventFilter);
#endif
}

QuarterWidgetP::~QuarterWidgetP()
{
  QtGLWidget* glMaster = static_cast<QtGLWidget*>(this->master->viewport());
  removeFromCacheContext(this->cachecontext, glMaster);
  if (this->contextmenu) {
    delete this->contextmenu;
  }
}

SoCamera *
QuarterWidgetP::searchForCamera(SoNode * root)
{
  SoSearchAction sa;
  sa.setInterest(SoSearchAction::FIRST);
  sa.setType(SoCamera::getClassTypeId());
  sa.apply(root);

  if (sa.getPath()) {
    SoNode * node = sa.getPath()->getTail();
    if (node && node->isOfType(SoCamera::getClassTypeId())) {
      return (SoCamera *) node;
    }
  }
  return nullptr;
}

uint32_t
QuarterWidgetP::getCacheContextId() const
{
  return this->cachecontext->id;
}

QuarterWidgetP_cachecontext *
QuarterWidgetP::findCacheContext(QuarterWidget * widget, const QtGLWidget * sharewidget)
{
  if (!cachecontext_list) {
    // FIXME: static memory leak
    cachecontext_list = new SbList <QuarterWidgetP_cachecontext*>;
  }
  for (int i = 0; i < cachecontext_list->getLength(); i++) {
    QuarterWidgetP_cachecontext * cachecontext = (*cachecontext_list)[i];

    for (int j = 0; j < cachecontext->widgetlist.getLength(); j++) {
      if (cachecontext->widgetlist[j] == sharewidget) {
        cachecontext->widgetlist.append(static_cast<const QtGLWidget*>(widget->viewport()));
        return cachecontext;
      }
    }
  }
  QuarterWidgetP_cachecontext * cachecontext = new QuarterWidgetP_cachecontext;
  cachecontext->id = SoGLCacheContextElement::getUniqueCacheContext();
  cachecontext->widgetlist.append(static_cast<const QtGLWidget*>(widget->viewport()));
  cachecontext_list->append(cachecontext);

  return cachecontext;
}

void
QuarterWidgetP::removeFromCacheContext(QuarterWidgetP_cachecontext * context, const QtGLWidget * widget)
{
  context->widgetlist.removeItem((const QtGLWidget*) widget);

  if (context->widgetlist.getLength() == 0) { // last context in this share group?
    assert(cachecontext_list);

    for (int i = 0; i < cachecontext_list->getLength(); i++) {
      if ((*cachecontext_list)[i] == context) {
        // set the context while calling destructingContext() (might trigger OpenGL calls)
        const_cast<QtGLWidget*> (widget)->makeCurrent();
        // fetch the cc_glglue context instance as a workaround for a bug fixed in Coin r12818
        (void) cc_glglue_instance(context->id);
        cachecontext_list->removeFast(i);
        SoContextHandler::destructingContext(context->id);
        const_cast<QtGLWidget*> (widget)->doneCurrent();
        delete context;
        return;
      }
    }
  }
}

void
QuarterWidgetP::replaceGLWidget(const QtGLWidget * newviewport)
{
  QtGLWidget* oldviewport = static_cast<QtGLWidget*>(this->master->viewport());
  cachecontext->widgetlist.removeItem(oldviewport);
  cachecontext->widgetlist.append(newviewport);
}

/*!

 */
void
QuarterWidgetP::rendercb(void * userdata, SoRenderManager *)
{
  QuarterWidget * thisp = static_cast<QuarterWidget *>(userdata);

  if (thisp->pimpl->autoredrawenabled) {
    thisp->redraw();
  }
}

void
QuarterWidgetP::prerendercb(void * userdata, SoRenderManager * manager)
{
  Q_UNUSED(manager); 
  QuarterWidgetP * thisp = static_cast<QuarterWidgetP *>(userdata);
  SoEventManager * evman = thisp->soeventmanager;
  assert(evman);
  for (int c = 0; c < evman->getNumSoScXMLStateMachines(); ++c) {
    SoScXMLStateMachine * statemachine = evman->getSoScXMLStateMachine(c);
    statemachine->preGLRender();
  }
}

void
QuarterWidgetP::postrendercb(void * userdata, SoRenderManager * manager)
{
  Q_UNUSED(manager); 
  QuarterWidgetP * thisp = static_cast<QuarterWidgetP *>(userdata);
  SoEventManager * evman = thisp->soeventmanager;
  assert(evman);
  for (int c = 0; c < evman->getNumSoScXMLStateMachines(); ++c) {
    SoScXMLStateMachine * statemachine = evman->getSoScXMLStateMachine(c);
    statemachine->postGLRender();
  }
}

void
QuarterWidgetP::statechangecb(void * userdata, ScXMLStateMachine * statemachine, const char * stateid, SbBool enter, SbBool)
{
  Q_UNUSED(statemachine); 
  static const SbName contextmenurequest("contextmenurequest");
  QuarterWidgetP * thisp = static_cast<QuarterWidgetP *>(userdata);
  assert(thisp && thisp->master);
  if (enter) {
    SbName state(stateid);
    if (thisp->contextmenuenabled && state == contextmenurequest) {
      thisp->contextMenu()->exec(thisp->eventfilter->globalMousePosition());
    }
    if (QuarterP::statecursormap->contains(state)) {
      QCursor cursor = QuarterP::statecursormap->value(state);
      thisp->master->setCursor(cursor);
    }
  }
}

#define ADD_ACTION(enum, text, group, parent, list)     \
  do {                                                  \
    QAction * action = new QAction(text, parent);       \
    action->setCheckable(true);                         \
    action->setData(enum);                              \
    action->setObjectName(text);                        \
    action->setActionGroup(group);                      \
    list.append(action);                                \
  } while (0)


QList<QAction *>
QuarterWidgetP::transparencyTypeActions() const
{
  if (this->transparencytypeactions.isEmpty()) {
    this->transparencytypegroup = new QActionGroup(this->master);
    ADD_ACTION(QuarterWidget::NONE, "none", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::SCREEN_DOOR, "screen door", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::ADD, "add", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::DELAYED_ADD, "delayed add", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::SORTED_OBJECT_ADD, "sorted object add", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::BLEND, "blend", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::DELAYED_BLEND, "delayed blend", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::SORTED_OBJECT_BLEND, "sorted object blend", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::SORTED_OBJECT_SORTED_TRIANGLE_ADD, "sorted object sorted triangle add", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::SORTED_OBJECT_SORTED_TRIANGLE_BLEND, "sorted object sorted triangle blend", transparencytypegroup, this->master, this->transparencytypeactions);
    ADD_ACTION(QuarterWidget::SORTED_LAYERS_BLEND, "sorted layers blend", transparencytypegroup, this->master, this->transparencytypeactions);
  }
  return this->transparencytypeactions;
}

QList<QAction *>
QuarterWidgetP::stereoModeActions() const
{
  if (this->stereomodeactions.isEmpty()) {
    this->stereomodegroup = new QActionGroup(this->master);
    ADD_ACTION(QuarterWidget::MONO, "mono", stereomodegroup, this->master, stereomodeactions);
    ADD_ACTION(QuarterWidget::ANAGLYPH, "anaglyph", stereomodegroup, this->master, stereomodeactions);
    ADD_ACTION(QuarterWidget::QUAD_BUFFER, "quad buffer", stereomodegroup, this->master, stereomodeactions);
    ADD_ACTION(QuarterWidget::INTERLEAVED_ROWS, "interleaved rows", stereomodegroup, this->master, stereomodeactions);
    ADD_ACTION(QuarterWidget::INTERLEAVED_COLUMNS, "interleaved columns", stereomodegroup, this->master, stereomodeactions);
  }
  return this->stereomodeactions;
}

QList<QAction *>
QuarterWidgetP::renderModeActions() const
{
  if (this->rendermodeactions.isEmpty()) {
    this->rendermodegroup = new QActionGroup(this->master);
    ADD_ACTION(QuarterWidget::AS_IS, "as is", rendermodegroup, this->master, rendermodeactions);
    ADD_ACTION(QuarterWidget::WIREFRAME, "wireframe", rendermodegroup, this->master, rendermodeactions);
    ADD_ACTION(QuarterWidget::WIREFRAME_OVERLAY, "wireframe overlay", rendermodegroup, this->master, rendermodeactions);
    ADD_ACTION(QuarterWidget::POINTS, "points", rendermodegroup, this->master, rendermodeactions);
    ADD_ACTION(QuarterWidget::HIDDEN_LINE, "hidden line", rendermodegroup, this->master, rendermodeactions);
    ADD_ACTION(QuarterWidget::BOUNDING_BOX, "bounding box", rendermodegroup, this->master, rendermodeactions);
  }
  return this->rendermodeactions;
}

#undef ADD_ACTION

QMenu *
QuarterWidgetP::contextMenu()
{
  if (!this->contextmenu) {
    this->contextmenu = new ContextMenu(this->master);
  }

  return this->contextmenu->getMenu();
}


bool 
QuarterWidgetP::nativeEventFilter(void * message, long * result)
{
#ifdef HAVE_SPACENAV_LIB
  XEvent * event = (XEvent *) message;
  if (event->type == ClientMessage) {
    // FIXME: I don't really like this, but the original XEvent will
    // die before reaching the destination within the Qt system. To
    // avoid this, we'll have to make a copy. We should try to find a
    // workaround for this. (20101020 handegar)

    // The copy will automatically be deleted when the NativeEvent dies.
    XEvent * copy = (XEvent *) malloc(sizeof(XEvent));
    memcpy(copy, event, sizeof(XEvent));
    NativeEvent * ne = new NativeEvent(copy);

    qApp->postEvent(QApplication::focusWidget(), ne);
    return true;
  }
#else
  Q_UNUSED(message); 
  Q_UNUSED(result); 
#endif // HAVE_SPACENAV_LIB

  return false;
}

