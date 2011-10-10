/***************************************************************************
 *                                                                         *
 *   Copyright: http://www.ii-system.com                                   *
 *   License:   LGPL                                                       *
 *                                                                         *
 ***************************************************************************/

#ifndef IISTASKPANEL_GLOBAL_H
#define IISTASKPANEL_GLOBAL_H

#include <qglobal.h>

#ifdef QIIS_STATICLIB
#  undef QIIS_SHAREDLIB
#  define IISTASKPANEL_EXPORT
#else
#  ifdef QIIS_MAKEDLL
#   define IISTASKPANEL_EXPORT Q_DECL_EXPORT
#  else
#   define IISTASKPANEL_EXPORT Q_DECL_IMPORT
#  endif
#endif

#endif // IISTASKPANEL_GLOBAL_H
