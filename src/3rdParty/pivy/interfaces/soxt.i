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

%define SOXT_MODULE_DOCSTRING
"The soxt module is a wrapper for the SoXt library."
%enddef

%module(package="pivy.gui", docstring=SOXT_MODULE_DOCSTRING) soxt

%{

#include <Inventor/Xt/devices/SoXtLinuxJoystick.h>
#include <Inventor/Xt/devices/SoXtDevice.h>
#include <Inventor/Xt/devices/SoXtKeyboard.h>
#include <Inventor/Xt/devices/SoXtMouse.h>
#include <Inventor/Xt/editors/SoXtColorEditor.h>
#include <Inventor/Xt/editors/SoXtMaterialEditor.h>
#include <Inventor/Xt/nodes/SoGuiColorEditor.h>
#include <Inventor/Xt/nodes/SoGuiMaterialEditor.h>
#include <Inventor/Xt/viewers/SoXtViewer.h>
#include <Inventor/Xt/viewers/SoXtConstrainedViewer.h>
#include <Inventor/Xt/viewers/SoXtFullViewer.h>
#include <Inventor/Xt/viewers/SoXtExaminerViewer.h>
#include <Inventor/Xt/viewers/SoXtFlyViewer.h>
#include <Inventor/Xt/viewers/SoXtPlaneViewer.h>
#include <Inventor/Xt/widgets/SoXtPopupMenu.h>
#include <Inventor/Xt/SoXtResource.h>
#include <Inventor/Xt/SoXt.h>
#include <Inventor/Xt/SoXtBasic.h>
#include <Inventor/Xt/SoXtObject.h>
#include <Inventor/Xt/SoXtCursor.h>
#include <Inventor/Xt/SoXtComponent.h>
#include <Inventor/Xt/SoXtGLWidget.h>
#include <Inventor/Xt/SoXtRenderArea.h>
#include <Inventor/Xt/SoXtColorEditor.h>
#include <Inventor/Xt/SoXtMaterialEditor.h>

#include "coin_header_includes.h"

/* make CustomCursor in SoXtCursor known to SWIG */
typedef SoXtCursor::CustomCursor CustomCursor;

/* FIXME: there is a major pitfall reg. this solution, namely
 * thread safety! reconsider! 20030626 tamer.
 */
static void *
Pivy_PythonInteractiveLoop(void *data) {
  PyRun_InteractiveLoop(stdin, "<stdin>");
  return NULL;
}
%}

/* include the typemaps common to all pivy modules */
%include pivy_common_typemaps.i

/* import the pivy main interface file */
%import coin.i

%include Inventor/Xt/devices/SoXtLinuxJoystick.h
%include Inventor/Xt/devices/SoXtDevice.h
%include Inventor/Xt/devices/SoXtKeyboard.h
%include Inventor/Xt/devices/SoXtMouse.h
%include Inventor/Xt/editors/SoXtColorEditor.h
%include Inventor/Xt/editors/SoXtMaterialEditor.h
%include Inventor/Xt/nodes/SoGuiColorEditor.h
%include Inventor/Xt/nodes/SoGuiMaterialEditor.h
%include Inventor/Xt/viewers/SoXtViewer.h
%include Inventor/Xt/viewers/SoXtConstrainedViewer.h
%include Inventor/Xt/viewers/SoXtFullViewer.h
%include Inventor/Xt/viewers/SoXtExaminerViewer.h
%include Inventor/Xt/viewers/SoXtFlyViewer.h
%include Inventor/Xt/viewers/SoXtPlaneViewer.h
%include Inventor/Xt/widgets/SoXtPopupMenu.h
%include Inventor/Xt/SoXtResource.h
%include Inventor/Xt/SoXt.h
%include Inventor/Xt/SoXtBasic.h
%include Inventor/Xt/SoXtObject.h
%include Inventor/Xt/SoXtCursor.h
%include Inventor/Xt/SoXtComponent.h
%include Inventor/Xt/SoXtGLWidget.h
%include Inventor/Xt/SoXtRenderArea.h
%include Inventor/Xt/SoXtColorEditor.h
%include Inventor/Xt/SoXtMaterialEditor.h
