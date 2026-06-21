# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Findsentry
# ----------
#
# Locate the Sentry Native SDK and expose it through the imported target
# ``sentry::sentry``.
#
# The module first attempts to use an installed CMake config package
# (``sentry-config.cmake``) shipped by the SDK. When that is unavailable it
# falls back to locating the header and library directly, which covers libpack
# style installations.
#
# Result variables:
#   sentry_FOUND        - true when the SDK was located
#   sentry_INCLUDE_DIR  - directory containing ``sentry.h``
#   sentry_LIBRARY      - the resolved ``sentry`` library

if(TARGET sentry::sentry)
    set(sentry_FOUND TRUE)
    return()
endif()

find_package(sentry CONFIG QUIET)
if(sentry_FOUND AND TARGET sentry::sentry)
    return()
endif()

find_path(sentry_INCLUDE_DIR
    NAMES sentry.h
    PATH_SUFFIXES include sentry/include
)

find_library(sentry_LIBRARY
    NAMES sentry libsentry
    PATH_SUFFIXES lib lib64 bin
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(sentry
    REQUIRED_VARS sentry_LIBRARY sentry_INCLUDE_DIR
)

if(sentry_FOUND AND NOT TARGET sentry::sentry)
    add_library(sentry::sentry UNKNOWN IMPORTED)
    set_target_properties(sentry::sentry PROPERTIES
        IMPORTED_LOCATION "${sentry_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${sentry_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(sentry_INCLUDE_DIR sentry_LIBRARY)
