/***************************************************************************
 *                                                                         *
 *   Copyright: https://code.google.com/p/qsint/                           *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef QSINT_GLOBAL_H
#define QSINT_GLOBAL_H

#include <qglobal.h>

#ifdef QSINT_STATICLIB
#  undef QSINT_SHAREDLIB
#  define QSINT_EXPORT
#else
#  ifdef QSINT_MAKEDLL
#   define QSINT_EXPORT Q_DECL_EXPORT
#  else
#   define QSINT_EXPORT Q_DECL_IMPORT
#  endif
#endif

#endif // QSINT_GLOBAL_H
