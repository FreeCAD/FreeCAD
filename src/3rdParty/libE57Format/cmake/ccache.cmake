# SPDX-License-Identifier: BSL-1.0
# Copyright 2022 Andy Maloney <asmaloney@gmail.com>

# See: https://crascit.com/2016/04/09/using-ccache-with-cmake/
find_program( CCACHE_PROGRAM ccache )

if ( CCACHE_PROGRAM )
    message( STATUS "[${PROJECT_NAME}] Using ccache: ${CCACHE_PROGRAM}" )

    set_target_properties( ${PROJECT_NAME}
        PROPERTIES
            CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}"
            C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}"
    )
endif()
