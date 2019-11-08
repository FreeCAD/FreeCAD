#ifndef _BLSURFPlugin_DEFS_HXX_
#define _BLSURFPlugin_DEFS_HXX_

#ifdef WIN32
  #if defined BLSURFPlugin_EXPORTS
    #define BLSURFPLUGIN_EXPORT __declspec( dllexport )
  #else
    #define BLSURFPLUGIN_EXPORT __declspec( dllimport )
  #endif
#else
  #define BLSURFPLUGIN_EXPORT
#endif

#endif
