# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2026 Céleste Wouters <foss@elementw.net>
# SPDX-FileNotice: Part of the FreeCAD project.

################################################################################
#                                                                              #
#   FreeCAD is free software: you can redistribute it and/or modify            #
#   it under the terms of the GNU Lesser General Public License as             #
#   published by the Free Software Foundation, either version 2.1              #
#   of the License, or (at your option) any later version.                     #
#                                                                              #
#   FreeCAD is distributed in the hope that it will be useful,                 #
#   but WITHOUT ANY WARRANTY; without even the implied warranty                #
#   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                    #
#   See the GNU Lesser General Public License for more details.                #
#                                                                              #
#   You should have received a copy of the GNU Lesser General Public           #
#   License along with FreeCAD. If not, see https://www.gnu.org/licenses       #
#                                                                              #
################################################################################

if(HDF5_FIND_VERSION)
    # This isn't implemented (yet), but can be at a later date.
    message(FATAL_ERROR "FreeCAD FindHDF5.cmake doesn't support version matching")
endif()
set(_args)
if(DEFINED HDF5_FIND_COMPONENTS)
    list(APPEND _args COMPONENTS ${HDF5_FIND_COMPONENTS})
endif()

# Use CMake's native FindHDF5.cmake first, as it is the one VTK might use,
# and the pkg-config version could clash with it.

set(_prefer_system_default ON)
if(FREECAD_LIBPACK_USE AND (FREECAD_LIBPACK_VERSION VERSION_LESS "3.5.0"))
    set(_prefer_system_default OFF)
    message(WARNING "HDF5 packaging is broken on LibPack versions before 3.5.0.")
endif()
if(((NOT DEFINED FREECAD_CMAKE_PREFER_SYSTEM_HDF5) AND _prefer_system_default) OR
        FREECAD_CMAKE_PREFER_SYSTEM_HDF5)
    set(_hdf5_find_mode "config")
    find_package(HDF5 ${_args} NO_MODULE)
    if(NOT HDF5_FOUND)
        set(_cmake_module_path "${CMAKE_MODULE_PATH}")
        list(REMOVE_ITEM CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cMake")
        set(_hdf5_find_mode "module")
        find_package(HDF5 ${_args})
        set(CMAKE_MODULE_PATH "${_cmake_module_path}")
    endif()

    if(HDF5_FOUND)
        if((NOT TARGET hdf5::hdf5) AND (DEFINED HDF5_INCLUDE_DIR))
            # FindHDF5 is old and doesn't create modern CMake target, do it
            # ourselves to avoid having to handle this different case down the line.
            set(_hdf5_lib_var "HDF5_C_SHARED_LIBRARY")
            if("static" IN_LIST HDF5_FIND_COMPONENTS)
                set(_hdf5_lib_var "HDF5_C_STATIC_LIBRARY")
            endif()
            if(DEFINED "${_hdf5_lib_var}")
                add_library(hdf5::hdf5 INTERFACE IMPORTED)
                target_include_directories(hdf5::hdf5 INTERFACE ${HDF5_INCLUDE_DIR})
                target_link_libraries(hdf5::hdf5 INTERFACE ${${_hdf5_lib_var}})
            endif()
        endif()
        if(NOT TARGET hdf5::hdf5)
            message(FATAL_ERROR "Imported HDF5 (${_hdf5_find_mode}) but no hdf5::hdf5 target exists")
        endif()

        # CMake found it, we're done here.
        hdf5_clean_fortify_source()
        message(STATUS "Passed find_package(HDF5) through (${_hdf5_find_mode} mode)")
        return()
    endif()
endif()

if(MSVC)
    message(FATAL_ERROR "find_package(HDF5) was requested but could not be found by either CMake \
        config or module, and pkg-config is not available when using MSVC.")
endif()

find_package(PkgConfig)
pkg_search_module(PCHDF5 REQUIRED ${HDF5_VARIANT})

add_library(hdf5::hdf5 INTERFACE IMPORTED)
target_include_directories(hdf5::hdf5 INTERFACE ${PCHDF5_LIBRARY_DIRS})
target_compile_options(hdf5::hdf5 INTERFACE ${PCHDF5_CFLAGS})
target_link_libraries(hdf5::hdf5 INTERFACE ${PCHDF5_LIBRARIES})

# workaround to define include dir from PCHDF5_CFLAGS (pkg-config PCHDF5_INCLUDEDIR is only filled since hdf5 1.14.6)
set(hdf5_include_path "")
foreach(flag IN LISTS PCHDF5_CFLAGS)
    if(flag MATCHES "^-I")
        string(REGEX REPLACE "^-I[ ]*" "" flag "${flag}")
        list(APPEND hdf5_include_path "${flag}")
    endif()
endforeach()

set(_save_INC CMAKE_REQUIRED_INCLUDES)
set(CMAKE_REQUIRED_INCLUDES ${hdf5_include_path})
check_include_file_cxx(hdf5.h HDF5_HEAD_FOUND)
set(CMAKE_REQUIRED_INCLUDES ${_save_INC})
if(NOT HDF5_HEAD_FOUND)
    message(FATAL_ERROR "hdf5.h was not found (tested pkg-config ${HDF5_VARIANT}, suggested header location was '${hdf5_include_path}').")
endif()

hdf5_clean_fortify_source()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HDF5)
