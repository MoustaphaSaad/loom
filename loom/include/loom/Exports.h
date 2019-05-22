
#ifndef LOOM_EXPORT_H
#define LOOM_EXPORT_H

#ifdef LOOM_STATIC_DEFINE
#  define LOOM_EXPORT
#  define LOOM_NO_EXPORT
#else
#  ifndef LOOM_EXPORT
#    ifdef loom_EXPORTS
        /* We are building this library */
#      define LOOM_EXPORT __declspec(dllexport)
#    else
        /* We are using this library */
#      define LOOM_EXPORT __declspec(dllimport)
#    endif
#  endif

#  ifndef LOOM_NO_EXPORT
#    define LOOM_NO_EXPORT 
#  endif
#endif

#ifndef LOOM_DEPRECATED
#  define LOOM_DEPRECATED __declspec(deprecated)
#endif

#ifndef LOOM_DEPRECATED_EXPORT
#  define LOOM_DEPRECATED_EXPORT LOOM_EXPORT LOOM_DEPRECATED
#endif

#ifndef LOOM_DEPRECATED_NO_EXPORT
#  define LOOM_DEPRECATED_NO_EXPORT LOOM_NO_EXPORT LOOM_DEPRECATED
#endif

#if 0 /* DEFINE_NO_DEPRECATED */
#  ifndef LOOM_NO_DEPRECATED
#    define LOOM_NO_DEPRECATED
#  endif
#endif

#endif /* LOOM_EXPORT_H */
