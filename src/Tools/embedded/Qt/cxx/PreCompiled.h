#ifndef __PRECOMPILED__
#define __PRECOMPILED__

#ifdef _MSC_VER
# pragma warning( disable : 4251 )
#endif

#if defined(signals) && defined(QOBJECTDEFS_H) && \
  !defined(QT_MOC_CPP)
#  undef signals
#  define signals signals
#endif

#include <boost/signal.hpp>
namespace boost
{
  namespace signalslib = signals;
}

#if defined(signals) && defined(QOBJECTDEFS_H) && \
  !defined(QT_MOC_CPP)
#  undef signals
// Restore the macro definition of "signals", as it was
// defined by Qt's <qobjectdefs.h>.
#  define signals public
#endif


#ifdef FC_OS_WIN32
// cmake generates this define
# if defined (FreeCADPlugin_EXPORTS)
#  define FC_PLUGIN_EXPORT __declspec(dllexport)
# else
#  define FC_PLUGIN_EXPORT __declspec(dllimport)
# endif
# define MeshExport        __declspec(dllimport)
#else // for Linux
# define FC_PLUGIN_EXPORT
# define MeshExport   
#endif


#endif