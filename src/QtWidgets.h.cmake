#pragma once

#include <QtGlobal>


#if QT_VERSION >= QT_VERSION_CHECK(6,0,0) && QT_VERSION < QT_VERSION_CHECK(6,8,1)
# define HAS_QTBUG_129596
#endif
