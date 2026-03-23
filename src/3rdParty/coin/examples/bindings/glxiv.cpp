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

// *************************************************************************

// glxiv.cpp -- example demonstrating Coin (or Open Inventor) bound
// directly into GLX/X11. Useful e.g. as a starting point if one wants
// to make an X11 application that doesn't depend on SoXt.
//

// If you have Coin properly installed, you should be able to build by
// simply doing:
//
//   $ coin-config --build glxiv glxiv.cpp
//

// *************************************************************************

// FIXME: note that there are some limitations to this example:
//
//  * The most important one is that events are not translated from
//    native X11 events to Coin events (to be sent to the scene
//    graph). This means that for instance draggers and manipulators in
//    the scene graph will not respond to attempts at interaction.
//
//  * The sensor queue processing is just a hack. Don't use this
//    in production code.
//
// 20031113 mortene.

// *************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>

#include <Inventor/SbTime.h>
#include <Inventor/SoDB.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/SoSceneManager.h>
#include <Inventor/nodekits/SoNodeKit.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoRotor.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText3.h>

// *************************************************************************

typedef struct {
  Display * display;
  Window window;
  GLXContext context;

  SoSceneManager * scenemanager;
} WindowData;

// *************************************************************************

SbTime * starttime = NULL;
unsigned int rendercounter = 0;
static SoSeparator * root = NULL;

static void
draw_scene(void * userdata, SoSceneManager * scenemanager)
{
  if (starttime->getValue() == 0) { *starttime = SbTime::getTimeOfDay(); }

  // FIXME: should set near and far planes properly before
  // rendering. 20031113 mortene.

  scenemanager->render();

  WindowData * win = (WindowData *)userdata;
  glXSwapBuffers(win->display, win->window);

  rendercounter++;
  SbTime currenttime = SbTime::getTimeOfDay();
  SbTime interval = currenttime - *starttime;
  if (interval > 1.0) {
    (void)fprintf(stdout, "fps %f\n", rendercounter / interval.getValue());
    *starttime = currenttime;
    rendercounter = 0;
  }
}

// *************************************************************************

static void
make_glx_window(WindowData * win,
                int x, int y, unsigned int width, unsigned int height)
{
  int attrib[] = {
    GLX_RGBA,
    GLX_RED_SIZE, 1,
    GLX_GREEN_SIZE, 1,
    GLX_BLUE_SIZE, 1,
    GLX_DEPTH_SIZE, 1,
    GLX_DOUBLEBUFFER,
    None
  };


  const int scrnum = DefaultScreen(win->display);
  const Window rootwindow = RootWindow(win->display, scrnum);

  XVisualInfo * visinfo = glXChooseVisual(win->display, scrnum, attrib);
  if (!visinfo) {
    (void)fprintf(stderr, "Error: couldn't get an RGB, double-buffered visual.\n");
    exit(1);
  }

  XSetWindowAttributes attr;
  attr.background_pixel = 0;
  attr.border_pixel = 0;
  attr.colormap = XCreateColormap(win->display, rootwindow, visinfo->visual, AllocNone);
  attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
  unsigned long mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

  win->window = XCreateWindow(win->display, rootwindow, 0, 0, width, height,
                              0, visinfo->depth, InputOutput,
                              visinfo->visual, mask, &attr);
  
  {
    XSizeHints sizehints;
    sizehints.x = x;
    sizehints.y = y;
    sizehints.width  = width;
    sizehints.height = height;
    sizehints.flags = USSize | USPosition;
    XSetNormalHints(win->display, win->window, &sizehints);
    const char * name = "Coin in GLX";
    XSetStandardProperties(win->display, win->window, name, name,
                           None, (char **)NULL, 0, &sizehints);
  }

  win->context = glXCreateContext(win->display, visinfo, NULL, True);
  if (!win->context) {
    (void)fprintf(stderr, "Error: glXCreateContext() failed.\n");
    exit(1);
  }

  XFree(visinfo);
}

// *************************************************************************

static void
event_loop(WindowData * win)
{
  while (1) {
    static long mask = StructureNotifyMask | ExposureMask | KeyPressMask;
    XEvent event;
    while (XCheckWindowEvent(win->display, win->window, mask, &event)) {
      if (event.xany.window == win->window) {
        switch (event.type) {

        case Expose:
          draw_scene(win, win->scenemanager);
          break;
          
        case ConfigureNotify:
          {
            const int w = event.xconfigure.width;
            const int h = event.xconfigure.height;
            
            win->scenemanager->setWindowSize(SbVec2s(w, h));
            win->scenemanager->setSize(SbVec2s(w, h));
            win->scenemanager->setViewportRegion(SbViewportRegion(w, h));
            win->scenemanager->scheduleRedraw();
          }
          break;
          
        case KeyPress:
          {
            char buffer[1] = "";
            (void)XLookupString(&event.xkey, buffer, sizeof(buffer), NULL, NULL);
            if (buffer[0] == /* Esc: */ 27) { return; }
          }
          break;
        }
      }
    }
    
    // FIXME: should do this properly as in SoXt, to avoid using ~100%
    // CPU. 20031113 mortene.
    SoDB::getSensorManager()->processTimerQueue();
    SoDB::getSensorManager()->processDelayQueue(TRUE);
  }
}

// *************************************************************************

static void
make_coin_scenegraph(WindowData * win)
{
  root = new SoSeparator;
  root->ref();

  SoPerspectiveCamera * camera = new SoPerspectiveCamera;
  root->addChild(camera);

  SoDirectionalLight * light = new SoDirectionalLight;
  light->direction.setValue(-0.5, -0.5, 1);
  root->addChild(light);

  SoDirectionalLight * l = new SoDirectionalLight;
  l->direction = - light->direction.getValue();
  root->addChild(l);

  SoRotor * rotor = new SoRotor;
  rotor->rotation.setValue(SbVec3f(0, 1, 0), 0.1f);
  rotor->speed = 0.2;
  root->addChild(rotor);

  SoText3 * text3 = new SoText3;
  text3->string = "Coin 3D";
  text3->justification = SoText3::CENTER;
  text3->parts = SoText3::ALL;
  root->addChild(text3);

  win->scenemanager = new SoSceneManager;
  win->scenemanager->setRenderCallback(draw_scene, win);
  win->scenemanager->activate();
  win->scenemanager->setSceneGraph(root);

  camera->viewAll(root, win->scenemanager->getViewportRegion());
}

// *************************************************************************

int
main(int argc, char *argv[])
{
  // Initialize Coin.
  SoDB::init();
  SoDB::setRealTimeInterval(1/120.0);
  SoNodeKit::init();
  SoInteraction::init();

  Display * dpy = XOpenDisplay(NULL);
  if (!dpy) {
    (void)fprintf(stderr, "Error: couldn't open default display.\n");
    exit(1);
  }

  WindowData win = { dpy, 0, NULL, NULL };
  make_glx_window(&win, 100, 100, 400, 400);
  make_coin_scenegraph(&win);

  XMapWindow(win.display, win.window);
  glXMakeCurrent(win.display, win.window, win.context);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_LIGHTING);

  starttime = new SbTime(0.0);

  event_loop(&win);

  delete starttime;

  XDestroyWindow(win.display, win.window);
  glXDestroyContext(win.display, win.context);
  XCloseDisplay(win.display);

  win.scenemanager->setSceneGraph(NULL);
  root->unref();
  SoDB::finish();
  return 0;
}

// *************************************************************************
