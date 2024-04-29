#ifndef __openglwrapper_h__
#define __openglwrapper_h__
#ifdef HAVE_OPENGL_EXT
#include <QtOpenGLExtensions/qopenglextensions.h>
#include <QtGui/qopenglfunctions.h>
#define OpenGlInherit : protected QOpenGLExtraFunctions
#include <QOpenGLExtraFunctions>
#else
#include "GL/glew.h"
#define OpenGlInherit
#endif // HAVE_OPENGL_EXT

#endif // !__openglwrapper_h__
