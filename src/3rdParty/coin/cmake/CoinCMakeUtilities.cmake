# this file contains all helper macros and functions of the Coin3D project.

# option controlled helper for cmake variable dumping during config
function(dump_variable)
  if (COIN_VERBOSE)
    foreach(f ${ARGN})
      if (DEFINED ${f})
        message("${f} = ${${f}}")
      else()
        message("${f} = ***UNDEF***")
      endif()
    endforeach()
  endif()
endfunction()

# modifies the install directory passed by postfix, either substituting 'Coin' 
# with 'Coin${COIN_MAJOR_VERSION}' or adding the same versioned name after the 
# postfix part.
#
# Example: 
# versionize(INCLUDEDIR DOCDIR) will modify
#
# CMAKE_INSTALL_DOCDIR          'share/doc/Coin'          --> 'share/doc/Coin4'
# CMAKE_INSTALL_FULL_DOCDIR     '<prefix>/share/doc/Coin' --> '<prefix>/share/doc/Coin4'
#
# CMAKE_INSTALL_INCLUDEDIR      'include'          --> 'include/Coin4'
# CMAKE_INSTALL_FULL_INCLUDEDIR '<prefix>/include' --> '<prefix>/include/Coin4'
function(versionize)
  foreach(dir ${ARGN})
    set(name      "CMAKE_INSTALL_${dir}")
    set(full-name "CMAKE_INSTALL_FULL_${dir}")
    if(${name} MATCHES ${PROJECT_NAME})
      string(REPLACE "${PROJECT_NAME}" "${PROJECT_NAME}${PROJECT_VERSION_MAJOR}" value      "${${name}}")
      string(REPLACE "${PROJECT_NAME}" "${PROJECT_NAME}${PROJECT_VERSION_MAJOR}" full-value "${${full-name}}")
    else()
      set(value      "${${name}}/${PROJECT_NAME}${PROJECT_VERSION_MAJOR}")
      set(full-value "${CMAKE_INSTALL_PREFIX}/${value}")
    endif()
    set(${name}      ${value}      PARENT_SCOPE)
    set(${full-name} ${full-value} PARENT_SCOPE)
  endforeach()
endfunction()


# Checks all specified types for existence and sets variable and sets a variable HAVE_<type_name>
# if so. Additionally a variable named <type_name> is set to the size of the type.
# Moreover, ${_type_variable} will be set to the first type matching the specified ${_type_size}.
macro(coin_find_int_type_with_size _type_variable _type_size)
  set(${_type_variable} "")
  foreach(_type ${ARGN})
    string(TOUPPER ${_type} _type_var)
    string(REPLACE " " "_" _type_var ${_type_var})
    check_type_size(${_type} ${_type_var})
    if((${_type_var} STREQUAL ${_type_size}) AND (NOT ${_type_variable}))
      set(${_type_variable} ${_type})
      break()
    endif()
  endforeach()
endmacro()


# Replace the link type of the MSVCRT libraries
# statically: /MT|/MTd
# dynamically: /MD|/MDd
# Since CMake 3.15 this can be set by the CMAKE_MSVC_RUNTIME_LIBRARY variable
# or by setting the MSVC_RUNTIME_LIBRARY property of a target. But for being
# backwards compatible with older CMake versions this is needed.
macro(coin_msvc_link_static_crt _enable_static_crt)
  set(_vars          CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
  set(_vars ${_vars} CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  if(${_enable_static_crt})
    message(STATUS "Build against static Microsoft Visual C runtime library")
    foreach(_flags ${_vars})
      if(${_flags} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${_flags} "${${_flags}}")
      endif()
    endforeach()
    set(PKG_CONFIG_MSVC_LIBC "multithread-static")
  else()
    message(STATUS "Build against dynamic Microsoft Visual C runtime library")
    foreach(_flags ${_vars})
      if(${_flags} MATCHES "/MT")
        string(REGEX REPLACE "/MT" "/MD" ${_flags} "${${_flags}}")
      endif()
    endforeach()
    set(PKG_CONFIG_MSVC_LIBC "multithread-dynamic")
  endif()
endmacro()


# Adds all library dependencies in _input list (with absolute or relative paths)
# to _output list for pkg-config .pc file.
# Handles absolute Unix and Windows paths.
# A list element prefixed with '-' is transferred as is.
function(coin_add_pkg_config_lib_dependencies _output _input)
  foreach(_lib ${_input})
    if((_lib MATCHES "^/") OR (_lib MATCHES "^[a-zA-Z]:"))
      get_filename_component(_lib ${_lib} NAME_WE)
      string(REGEX REPLACE "^lib" "" _lib ${_lib})
    endif()
    if(_lib MATCHES "^-")
      set(${_output} "${${_output}} ${_lib}")
    else()
      set(${_output} "${${_output}} -l${_lib}")
    endif()
  endforeach()
  set(${_output} ${${_output}} PARENT_SCOPE)
endfunction()


# Add target settings like lib name, include directories, compile options, and
# compile definitions, from _targets to pkg-config settings
macro(coin_get_pkg_config_target_properties _comp_flags _inc_deps _lib_deps _targets)
  foreach(_tgt ${_targets})
    if(TARGET ${_tgt})
      get_target_property(_tgt_type ${_tgt} TYPE)
      if (NOT ${_tgt_type} STREQUAL "INTERFACE_LIBRARY")
        get_property(_has_prop TARGET ${_tgt} PROPERTY IMPORTED_LOCATION SET)
        if(_has_prop)
          get_target_property(_imp_loc ${_tgt} IMPORTED_LOCATION)
          list(APPEND ${_lib_deps} ${_imp_loc})
        else()
          get_property(_has_prop TARGET ${_tgt} PROPERTY IMPORTED_LOCATION_RELEASE SET)
          if(_has_prop)
            get_target_property(_imp_loc ${_tgt} IMPORTED_LOCATION_RELEASE)
            list(APPEND ${_lib_deps} ${_imp_loc})
          endif()
        endif()
      else()
        get_property(_has_prop TARGET ${_tgt} PROPERTY IMPORTED_LIBNAME SET)
        if(_has_prop)
          get_target_property(_imp_lib ${_tgt} IMPORTED_LIBNAME)
          list(APPEND ${_lib_deps} ${_imp_lib})
        else()
          get_property(_has_prop TARGET ${_tgt} PROPERTY IMPORTED_LIBNAME_RELEASE SET)
          if(_has_prop)
            get_target_property(_imp_lib ${_tgt} IMPORTED_LIBNAME_RELEASE)
            list(APPEND ${_lib_deps} ${_imp_lib})
          endif()
        endif()
      endif()
      get_property(_has_prop TARGET ${_tgt} PROPERTY INTERFACE_INCLUDE_DIRECTORIES SET)
      if(_has_prop)
        get_target_property(_inc_dirs ${_tgt} INTERFACE_INCLUDE_DIRECTORIES)
        list(APPEND ${_inc_deps} ${_inc_dirs})
      endif()
      get_property(_has_prop TARGET ${_tgt} PROPERTY INTERFACE_COMPILE_DEFINITIONS SET)
      if(_has_prop)
        get_target_property(_compile_defs ${_tgt} INTERFACE_COMPILE_DEFINITIONS)
        foreach(_def ${_compile_defs})
          set(${_comp_flags} "${${_comp_flags}} -D${_def}")
        endforeach()
      endif()
      get_property(_has_prop TARGET ${_tgt} PROPERTY INTERFACE_COMPILE_OPTIONS SET)
      if(_has_prop)
        get_target_property(_compile_opts ${_tgt} INTERFACE_COMPILE_OPTIONS)
        foreach(_opt ${_compile_opts})
          set(${_comp_flags} "${${_comp_flags}} ${_opt}")
        endforeach()
      endif()
    endif()
  endforeach()
endmacro()


# Set MACOSX_PACKAGE_LOCATION property on files
# Determine which subdirectory this file (header, resource) should be installed into.
# As the PUBLIC_HEADER and RESOURCE options of install target do not support
# directory structure creation when building a framework we set the MACOSX_PACKAGE_LOCATION
# property on the source files and add them to the target. This does however not work
# for the generated documentation files.
function(coin_set_macosx_properties _removable_prefixes _install_prefix _source_files)
  foreach(_file ${_source_files})
    get_filename_component(_loc "${_file}" DIRECTORY)
    foreach(_prefix ${_removable_prefixes})
      string(REPLACE "${_prefix}" "" _loc "${_loc}")
    endforeach()
    set_source_files_properties(${_file} PROPERTIES MACOSX_PACKAGE_LOCATION ${_install_prefix}${_loc})
  endforeach()
endfunction()


function(report_prepare)
  set(multiValueArgs IF_APPLE IF_WIN32 IF_MSVC)
  cmake_parse_arguments(REPORT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  if(REPORT_IF_APPLE AND APPLE)
    list(APPEND res ${REPORT_IF_APPLE})
  endif()
  if(REPORT_IF_WIN32 AND WIN32)
    list(APPEND res ${REPORT_IF_WIN32})
  endif()
  if(REPORT_IF_MSVC AND MSVC)
    list(APPEND res ${REPORT_IF_MSVC})
  endif()
  list(APPEND res ${REPORT_UNPARSED_ARGUMENTS})
  list(APPEND PACKAGE_OPTIONS ${res})
  set(PACKAGE_OPTIONS "${PACKAGE_OPTIONS}" PARENT_SCOPE)
endfunction()
