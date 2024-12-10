
#ifndef E57_DLL_H
#define E57_DLL_H

#ifdef E57FORMAT_STATIC_DEFINE
#  define E57_DLL
#  define E57FORMAT_NO_EXPORT
#else
#  ifndef E57_DLL
#    ifdef E57Format_EXPORTS
        /* We are building this library */
#      define E57_DLL 
#    else
        /* We are using this library */
#      define E57_DLL 
#    endif
#  endif

#  ifndef E57FORMAT_NO_EXPORT
#    define E57FORMAT_NO_EXPORT 
#  endif
#endif

#ifndef E57FORMAT_DEPRECATED
#  define E57FORMAT_DEPRECATED 
#endif

#ifndef E57FORMAT_DEPRECATED_EXPORT
#  define E57FORMAT_DEPRECATED_EXPORT E57_DLL E57FORMAT_DEPRECATED
#endif

#ifndef E57FORMAT_DEPRECATED_NO_EXPORT
#  define E57FORMAT_DEPRECATED_NO_EXPORT E57FORMAT_NO_EXPORT E57FORMAT_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef E57FORMAT_NO_DEPRECATED
#    define E57FORMAT_NO_DEPRECATED
#  endif
#endif
// NOTE: This is a generated file. Any changes will be overwritten.
#endif /* E57_DLL_H */
