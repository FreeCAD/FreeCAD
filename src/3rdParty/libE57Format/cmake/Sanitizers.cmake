# SPDX-License-Identifier: BSL-1.0
# Copyright 2023 Andy Maloney <asmaloney@gmail.com>

# Note: In theory address sanitization should work on MSVC, but I could not get it working.
# If you know how to fix it, please submit a PR:
#   https://github.com/asmaloney/libE57Format/pulls

string( TOUPPER ${PROJECT_NAME} PROJECT_NAME_UPPERCASE )

set( compiler_is_clang "$<OR:$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:Clang>>" )
set( compiler_is_gnu "$<CXX_COMPILER_ID:GNU>" )
set( compiler_is_msvc "$<CXX_COMPILER_ID:MSVC>" )
set( compiler_is_not_msvc "$<NOT:${compiler_is_msvc}>" )

if ( NOT MSVC )
    option( ${PROJECT_NAME_UPPERCASE}_SANITIZE_ALL "Enable all sanitizers if available." OFF )
    option( ${PROJECT_NAME_UPPERCASE}_SANITIZE_ADDRESS "Enable address sanitizer (ASan) if available." OFF )
    option( ${PROJECT_NAME_UPPERCASE}_SANITIZE_UNDEFINED "Enable memory sanitizer (UBSan) if available." OFF )
endif()

function( enable_address_sanitizer target )
    message( STATUS "[${target}] Enabling address sanitizer (ASan)" )

    target_compile_options( ${target}
        PRIVATE
            $<${compiler_is_not_msvc}:
                -fno-omit-frame-pointer
                -fsanitize=address
            >

            $<${compiler_is_msvc}:
                /fsanitize=address
                /INCREMENTAL:NO
                /Zi
            >
    )
    target_link_options( ${target}
        PUBLIC
            $<${compiler_is_not_msvc}:
                -fno-omit-frame-pointer
                -fsanitize=address
            >

            $<${compiler_is_msvc}:
                /DEBUG
                /INCREMENTAL:NO
            >
    )
endfunction()

function( enable_undefined_sanitizer target )
    if ( MSVC )
        message( WARNING "[${target}] Undefined behaviour sanitizer (UBSan) not available for MSVC" )
        return()
    endif()

    message( STATUS "[${target}] Enabling undefined behaviour sanitizer (UBSan)" )

    target_compile_options( ${target}
        PRIVATE
            -fsanitize=undefined
    )
    target_link_options( ${target}
        PUBLIC
            -fsanitize=undefined
    )
endfunction()

function( enable_all_sanitizers target )
    if ( MSVC )
        return()
    endif()

    unset( ${PROJECT_NAME_UPPERCASE}_SANITIZE_ADDRESS CACHE )
    unset( ${PROJECT_NAME_UPPERCASE}_SANITIZE_UNDEFINED CACHE )

    enable_address_sanitizer( ${target} )
    enable_undefined_sanitizer( ${target} )
endfunction()

if ( NOT MSVC )
    if ( ${PROJECT_NAME_UPPERCASE}_SANITIZE_ALL )
        enable_all_sanitizers( ${PROJECT_NAME} )
    else()
        if ( ${PROJECT_NAME_UPPERCASE}_SANITIZE_ADDRESS )
            enable_address_sanitizer( ${PROJECT_NAME} )
        endif()

        if ( ${PROJECT_NAME_UPPERCASE}_SANITIZE_UNDEFINED )
            enable_undefined_sanitizer( ${PROJECT_NAME} )
        endif()
    endif()
endif()
