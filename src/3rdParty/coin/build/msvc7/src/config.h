#ifndef COIN_DEBUG
#error The define COIN_DEBUG needs to be defined to true or false
#endif

#ifndef COIN_INTERNAL
#error this is a private header file
#endif

#if COIN_DEBUG
#include "config-debug.h"
#else /* !COIN_DEBUG */
#include "config-release.h"
#endif /* !COIN_DEBUG */
