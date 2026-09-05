# This file defines the variables
# ${PROJECT_NAME}_BUILD_TAG

set( T_ ${CMAKE_SYSTEM_PROCESSOR} )

string( TOLOWER ${CMAKE_SYSTEM_NAME} T1_ )

function( add_compiler_version )
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} ${CMAKE_CXX_COMPILER_ARG1} -dumpversion
        OUTPUT_VARIABLE T2_
    )
    string( REGEX REPLACE "([0-9])\\.([0-9])(\\.[0-9])?" "\\1\\2" T2_ ${T2_} )
    string( STRIP ${T2_} T2_ )
    set( T1_ ${T1_}${T2_} PARENT_SCOPE )
endfunction()

# default to just the CMake compiler ID
set( T1_ ${CMAKE_CXX_COMPILER_ID} )

# special cases to add versions and other info
if ( MSVC )
    if ( CMAKE_CL_64 )
        set( T_ ${T_}_64 )
    endif()

    set( T1_ "vc${MSVC_VERSION}" )
elseif ( MINGW )
    set( T1_ "MinGW" )
    add_compiler_version()
elseif ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    set( T1_ "gcc" )
    add_compiler_version()
elseif ( CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compiler_version()
endif()

set( T_ ${T_}-${T1_} )
set( ${PROJECT_NAME}_BUILD_TAG ${T_} )
