# SPDX-License-Identifier: LGPL-2.1-or-later

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

option(FREECAD_USE_SANITIZER_ASAN "Enable AddressSanitizer (ASan)" OFF)
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    option(FREECAD_USE_SANITIZER_LSAN "Enable LeakSanitizer (LSan)" OFF)
    option(FREECAD_USE_SANITIZER_TSAN "Enable ThreadSanitizer (TSan)" OFF)
    option(FREECAD_USE_SANITIZER_UBSAN "Enable UndefinedBehaviorSanitizer (UBSan)" OFF)
    option(FREECAD_USE_SANITIZER_MSAN "Enable MemorySanitizer (MSan)" OFF)
endif()

mark_as_advanced(
    FREECAD_USE_SANITIZER_ASAN
    FREECAD_USE_SANITIZER_LSAN
    FREECAD_USE_SANITIZER_TSAN
    FREECAD_USE_SANITIZER_UBSAN
    FREECAD_USE_SANITIZER_MSAN
)

macro(_check_sanitizer_flag flag result)
    set(CMAKE_REQUIRED_LINK_OPTIONS ${flag})
    check_c_compiler_flag(${flag} ${result})
    unset(CMAKE_REQUIRED_LINK_OPTIONS)
    if(NOT ${result})
        message(FATAL_ERROR "Compiler does not support ${flag}")
    endif()
endmacro()

macro(SetupSanitizers)
    if(MSVC)
        if(FREECAD_USE_SANITIZER_ASAN)
            message(STATUS "Building with AddressSanitizer (ASan)")
            string(REPLACE "/RTC1" "" CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}")
            string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
            add_compile_options(/fsanitize=address /fsanitize-address-use-after-return)
        endif()
    else()
        # Validate incompatible sanitizer combinations
        if(FREECAD_USE_SANITIZER_ASAN AND (FREECAD_USE_SANITIZER_MSAN OR FREECAD_USE_SANITIZER_TSAN))
            message(FATAL_ERROR "ASan cannot be used with MSan or TSan")
        endif()
        if(FREECAD_USE_SANITIZER_LSAN AND (FREECAD_USE_SANITIZER_MSAN OR FREECAD_USE_SANITIZER_TSAN))
            message(FATAL_ERROR "LSan cannot be used with MSan or TSan")
        endif()
        if(FREECAD_USE_SANITIZER_TSAN AND FREECAD_USE_SANITIZER_MSAN)
            message(FATAL_ERROR "TSan and MSan cannot be used together")
        endif()

        # Verify compiler supports requested sanitizers
        if(FREECAD_USE_SANITIZER_ASAN)
            _check_sanitizer_flag(-fsanitize=address _asan_supported)
        endif()
        if(FREECAD_USE_SANITIZER_LSAN)
            _check_sanitizer_flag(-fsanitize=leak _lsan_supported)
        endif()
        if(FREECAD_USE_SANITIZER_MSAN)
            _check_sanitizer_flag(-fsanitize=memory _msan_supported)
        endif()
        if(FREECAD_USE_SANITIZER_TSAN)
            _check_sanitizer_flag(-fsanitize=thread _tsan_supported)
        endif()
        if(FREECAD_USE_SANITIZER_UBSAN)
            _check_sanitizer_flag(-fsanitize=undefined _ubsan_supported)
        endif()

        if(FREECAD_USE_SANITIZER_ASAN OR FREECAD_USE_SANITIZER_LSAN OR FREECAD_USE_SANITIZER_TSAN
                OR FREECAD_USE_SANITIZER_UBSAN OR FREECAD_USE_SANITIZER_MSAN)
            if(NOT CMAKE_BUILD_TYPE MATCHES "Debug|RelWithDebInfo")
                message(WARNING "Sanitizers work best with Debug or RelWithDebInfo builds")
            endif()

            set(_sanitizer_flags
                $<$<BOOL:${FREECAD_USE_SANITIZER_ASAN}>:-fsanitize=address>
                $<$<BOOL:${FREECAD_USE_SANITIZER_LSAN}>:-fsanitize=leak>
                $<$<BOOL:${FREECAD_USE_SANITIZER_MSAN}>:-fsanitize=memory>
                $<$<BOOL:${FREECAD_USE_SANITIZER_TSAN}>:-fsanitize=thread>
                $<$<BOOL:${FREECAD_USE_SANITIZER_UBSAN}>:-fsanitize=undefined>
            )

            add_compile_options(-fno-omit-frame-pointer ${_sanitizer_flags})
            add_link_options(${_sanitizer_flags})
        endif()
    endif()
endmacro()
