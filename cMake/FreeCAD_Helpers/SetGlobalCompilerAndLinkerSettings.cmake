macro(SetGlobalCompilerAndLinkerSettings)
    # ================================================================================
    # == Global Compiler and Linker Settings =========================================

    include_directories(${CMAKE_BINARY_DIR}/src
                        ${CMAKE_SOURCE_DIR}/src)

    # check for 64-bit platform
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(STATUS "Platform is 64-bit, set -D_OCC64")
        add_definitions(-D_OCC64 )
    else(CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(STATUS "Platform is 32-bit")
    endif(CMAKE_SIZEOF_VOID_P EQUAL 8)

    # check for mips64 platform
    if("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "mips64")
        message(STATUS "Architecture: mips64")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mxgot")
    endif()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        # Only add -O0 if no -O* optimization flag exists
        if (NOT "${CMAKE_C_FLAGS_DEBUG}" MATCHES "-O[a-z0-9]+")
            set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0")
        endif()
        if (NOT "${CMAKE_CXX_FLAGS_DEBUG}" MATCHES "-O[a-z0-9]+")
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
        endif()
    endif()
    if(MSVC)
        # set default compiler settings
        add_definitions(-D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR -DNOMINMAX)
        add_compile_options(/Zm150 /bigobj)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DFC_DEBUG")
        if (MSVC_VERSION LESS 1930)  # Anything before VS 2022
            # set default libs
            set (CMAKE_C_STANDARD_LIBRARIES "kernel32.lib user32.lib gdi32.lib winspool.lib SHFolder.lib shell32.lib ole32.lib oleaut32.lib uuid.lib comdlg32.lib advapi32.lib winmm.lib comsupp.lib Ws2_32.lib dbghelp.lib ")
            set (CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_C_STANDARD_LIBRARIES}")
            # set linker flag /nodefaultlib
            set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB")
            set (CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /NODEFAULTLIB")
            set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB")
        endif()
        if(FREECAD_RELEASE_PDB)
            set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi")
            set (CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG")
            set (CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /DEBUG")
        endif(FREECAD_RELEASE_PDB)
        if(FREECAD_RELEASE_SEH)
            # remove /EHsc or /EHs flags because they are incompatible with /EHa
            if (${CMAKE_BUILD_TYPE} MATCHES "Release")
                string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
                string(REPLACE "/EHs" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
                set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /EHa")
            endif()
        endif(FREECAD_RELEASE_SEH)
        if(CCACHE_PROGRAM)
            # By default Visual Studio generators will use /Zi which is not compatible
            # with ccache, so tell Visual Studio to use /Z7 instead.
            string(REGEX REPLACE "/Z[iI]" "/Z7" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
            string(REGEX REPLACE "/Z[iI]" "/Z7" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
            string(REGEX REPLACE "/Z[iI]" "/Z7" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
        endif(CCACHE_PROGRAM)
        option(FREECAD_USE_MP_COMPILE_FLAG "Add /MP flag to the compiler definitions. Speeds up the compile on multi processor machines" ON)
        if(FREECAD_USE_MP_COMPILE_FLAG)
            # set "Build with Multiple Processes"
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
        endif()

        # Mark 32 bit executables large address aware so they can use > 2GB address space
        # NOTE: This setting only has an effect on machines with at least 3GB of RAM, although it sets the linker option it doesn't set the linker switch 'Enable Large Addresses'
        if(CMAKE_SIZEOF_VOID_P EQUAL 4)
            set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /LARGEADDRESSAWARE")
            set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /LARGEADDRESSAWARE")
        endif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    else(MSVC)
        set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DFC_DEBUG")
            #message(STATUS "DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
    endif(MSVC)

    if(MINGW)
        if(CMAKE_COMPILER_IS_CLANGXX)
            # clang for MSYS doesn't support -mthreads
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-all-symbols")
            #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols")
        else()
            # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=12477
            # Actually '-Wno-inline-dllimport' should work to suppress warnings of the form:
            # inline function 'foo' is declared as dllimport: attribute ignored
            # But it doesn't work with MinGW gcc 4.5.0 while using '-Wno-attributes' seems to
            # do the trick.
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes")
            set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes")
            set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--export-all-symbols")
            #set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-all-symbols")
            # http://stackoverflow.com/questions/8375310/warning-auto-importing-has-been-activated-without-enable-auto-import-specifie
            # set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -static-libgcc -static-libstdc++")
            link_libraries(-lgdi32)
        endif()
    endif(MINGW)

endmacro(SetGlobalCompilerAndLinkerSettings)
