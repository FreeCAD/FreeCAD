/*
 * Copyright (c) 2002-2007 Systems in Motion
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

%define SOGTK_MODULE_DOCSTRING
"The sogtk module is a wrapper for the SoGtk library."
%enddef

%module(package="pivy.gui", docstring=SOGTK_MODULE_DOCSTRING) sogtk

#include <Inventor/Gtk/devices/SoGtkDevice.h>
#include <Inventor/Gtk/devices/SoGtkKeyboard.h>
#include <Inventor/Gtk/devices/SoGtkMouse.h>
#include <Inventor/Gtk/widgets/SoGtkPopupMenu.h>
#include <Inventor/Gtk/viewers/SoGtkViewer.h>
#include <Inventor/Gtk/viewers/SoGtkConstrainedViewer.h>
#include <Inventor/Gtk/viewers/SoGtkFullViewer.h>
#include <Inventor/Gtk/viewers/SoGtkExaminerViewer.h>
#include <Inventor/Gtk/viewers/SoGtkFlyViewer.h>
#include <Inventor/Gtk/viewers/SoGtkPlaneViewer.h>
#include <Inventor/Gtk/SoGtkGraphEditor.h>
#include <Inventor/Gtk/SoGtkRoster.h>
#include <Inventor/Gtk/SoGtk.h>
#include <Inventor/Gtk/SoGtkBasic.h>
#include <Inventor/Gtk/SoGtkObject.h>
#include <Inventor/Gtk/SoGtkCursor.h>
#include <Inventor/Gtk/SoGtkComponent.h>
#include <Inventor/Gtk/SoGtkGLWidget.h>
#include <Inventor/Gtk/SoGtkRenderArea.h>

#include "coin_header_includes.h"

/* FIXME: there is a major pitfall reg. this solution, namely
 * thread safety! reconsider! 20030626 tamer.
 */
static void *Pivy_PythonInteractiveLoop(void *data) {
  PyRun_InteractiveLoop(stdin, "<stdin>");
  return NULL;
}

%}

/* include the typemaps common to all pivy modules */
%include pivy_common_typemaps.i

/* import the pivy main interface file */
%import coin.i

%include Inventor/Gtk/devices/SoGtkDevice.h
%include Inventor/Gtk/devices/SoGtkKeyboard.h
%include Inventor/Gtk/devices/SoGtkMouse.h
%include Inventor/Gtk/widgets/SoGtkPopupMenu.h
%include Inventor/Gtk/viewers/SoGtkViewer.h
%include Inventor/Gtk/viewers/SoGtkConstrainedViewer.h
%include Inventor/Gtk/viewers/SoGtkFullViewer.h
%include Inventor/Gtk/viewers/SoGtkExaminerViewer.h
%include Inventor/Gtk/viewers/SoGtkFlyViewer.h
%include Inventor/Gtk/viewers/SoGtkPlaneViewer.h
%include Inventor/Gtk/SoGtkGraphEditor.h
%include Inventor/Gtk/SoGtkRoster.h
%include Inventor/Gtk/SoGtk.h
%include Inventor/Gtk/SoGtkBasic.h
%include Inventor/Gtk/SoGtkObject.h
%include Inventor/Gtk/SoGtkCursor.h
%include Inventor/Gtk/SoGtkComponent.h
%include Inventor/Gtk/SoGtkGLWidget.h
%include Inventor/Gtk/SoGtkRenderArea.h
