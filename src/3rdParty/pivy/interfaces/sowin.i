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
%module(package="pivy.gui") sowin

%{
#if defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#undef max
#undef ERROR
#undef DELETE
#undef ANY
#endif

#include <Inventor/Win/devices/SoWinDevice.h>
#include <Inventor/Win/devices/SoWinKeyboard.h>
#include <Inventor/Win/devices/SoWinMouse.h>
#include <Inventor/Win/viewers/SoWinViewer.h>
#include <Inventor/Win/viewers/SoWinFullViewer.h>
#include <Inventor/Win/viewers/SoWinExaminerViewer.h>
#include <Inventor/Win/viewers/SoWinPlaneViewer.h>
#include <Inventor/Win/viewers/SoWinConstrainedViewer.h>
#include <Inventor/Win/viewers/SoWinFlyViewer.h>
#include <Inventor/Win/widgets/SoWinPopupMenu.h>
#include <Inventor/Win/SoWin.h>
#include <Inventor/Win/SoWinBasic.h>
#include <Inventor/Win/SoWinObject.h>
#include <Inventor/Win/SoWinCursor.h>
#include <Inventor/Win/SoWinComponent.h>
#include <Inventor/Win/SoWinGLWidget.h>
#include <Inventor/Win/SoWinRenderArea.h>

#include "coin_header_includes.h"

/* make CustomCursor in SoWinCursor known to SWIG */
typedef SoWinCursor::CustomCursor CustomCursor;

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

%include Inventor/Win/devices/SoWinDevice.h
%include Inventor/Win/devices/SoWinKeyboard.h
%include Inventor/Win/devices/SoWinMouse.h
%include Inventor/Win/viewers/SoWinViewer.h
%include Inventor/Win/viewers/SoWinFullViewer.h
%include Inventor/Win/viewers/SoWinExaminerViewer.h
%include Inventor/Win/viewers/SoWinPlaneViewer.h
%include Inventor/Win/viewers/SoWinConstrainedViewer.h
%include Inventor/Win/viewers/SoWinFlyViewer.h
%include Inventor/Win/widgets/SoWinPopupMenu.h
%include Inventor/Win/SoWin.h
%include Inventor/Win/SoWinBasic.h
%include Inventor/Win/SoWinObject.h
%include Inventor/Win/SoWinCursor.h
%include Inventor/Win/SoWinComponent.h
%include Inventor/Win/SoWinGLWidget.h
%include Inventor/Win/SoWinRenderArea.h
