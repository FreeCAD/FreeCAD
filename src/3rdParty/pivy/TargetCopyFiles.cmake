# The MIT License (MIT)

# Copyright (c) 2013 University College London

# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

# Adds a target which simply copies files from one place to another.
# See https://github.com/UCL/GreatCMakeCookOff/wiki for information

include(CMakeParseArguments)

function(add_copy_files FILECOPIER_TARGET)
    cmake_parse_arguments(
        FILECOPIER
        "VERBOSE"
        "DESTINATION;GLOB"
        "REPLACE;FILES"
        ${ARGN}
    )

    if(NOT TARGET "${FILECOPIER_TARGET}")
        add_custom_target(${FILECOPIER_TARGET})
    endif()
    get_target_property(result ${FILECOPIER_TARGET} TYPE)
    if(NOT FILECOPIER_DESTINATION)
        set(destination ${CMAKE_CURRENT_BINARY_DIR})
    else()
        get_filename_component(destination "${FILECOPIER_DESTINATION}" ABSOLUTE)
    endif()
    if(NOT FILECOPIER_GLOB AND NOT FILECOPIER_FILES)
        set(input_sources ${FILECOPIER_UNPARSED_ARGUMENTS})
    elseif(FILECOPIER_GLOB AND FILECOPIER_FILES)
        message(FATAL_ERROR "copy_files takes one of GLOB or FILES, not both")
    elseif(FILECOPIER_FILES)
        set(input_sources ${FILECOPIER_FILES})
    else()
        file(GLOB input_sources ${FILECOPIER_GLOB})
    endif()

    if(FILECOPIER_REPLACE)
        list(LENGTH FILECOPIER_REPLACE replace_length)
        if(NOT ${replace_length} EQUAL 2)
            message(FATAL_ERROR "copy_files argument REPLACE takes two inputs")
        endif()
        list(GET FILECOPIER_REPLACE 0 PATTERN)
        list(GET FILECOPIER_REPLACE 1 REPLACEMENT)
    endif()

    foreach(input ${input_sources})
        get_filename_component(output ${input} NAME)
        if(NOT "${FILECOPIER_REPLACE}" STREQUAL "")
            string(REGEX REPLACE "${PATTERN}" "${REPLACEMENT}" output ${output})
        endif()
        set(output ${destination}/${output})
        get_filename_component(input_abs "${input}" ABSOLUTE)
        get_filename_component(output_abs "${output}" ABSOLUTE)
        set(verbosity COMMENT "Copying ${input} to ${destination}")
        if(NOT ${FILECOPIER_VERBOSE})
            unset(verbosity)
        endif()
        if(NOT "${input_abs}" STREQUAL "${output_abs}")
            add_custom_command(
                TARGET ${FILECOPIER_TARGET}
                PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${input_abs} ${output_abs}
                ${verbosity}
                DEPENDS "${input}"
            )
       endif()
    endforeach()
endfunction()

function(add_copy_directory dircopy_TARGET directory)
    cmake_parse_arguments(dircopy
        "VERBOSE" "DESTINATION;RELATIVE" "EXCLUDE;GLOB" ${ARGN})

    get_filename_component(directory "${directory}" ABSOLUTE)
    if(NOT TARGET ${dircopy_TARGET})
        add_custom_target(${dircopy_TARGET})
    endif()
    if(NOT dircopy_GLOB)
        set(dircopy_GLOB "*")
    endif()
    if(NOT dircopy_EXCLUDE)
        unset(dircopy_EXCLUDE)
    endif()
    if(NOT dircopy_DESTINATION)
        set(dircopy_DESTINATION "${CMAKE_CURRENT_BINARY_DIR}")
    else()
        get_filename_component(dircopy_DESTINATION "${dircopy_DESTINATION}" ABSOLUTE)
    endif()
    if(NOT dircopy_RELATIVE)
        set(dircopy_RELATIVE "${directory}")
    else()
        get_filename_component(dircopy_RELATIVE "${dircopy_RELATIVE}" ABSOLUTE)
    endif()

    # Figure out globs for files that could be copied
    unset(in_globs)
    foreach(pattern ${dircopy_GLOB})
        list(APPEND in_globs "${directory}/${pattern}")
    endforeach()
    # Figure out globs for files that won't be copied
    unset(exclude_globs)
    foreach(pattern ${dircopy_EXCLUDE})
        list(APPEND exclude_globs "${directory}/${pattern}")
    endforeach()

    # Figure out files to copy
    file(GLOB_RECURSE in_files RELATIVE "${dircopy_RELATIVE}" ${in_globs})
    if(NOT "${exclude_globs}" STREQUAL "")
        file(GLOB_RECURSE exclude_files RELATIVE "${dircopy_RELATIVE}" ${exclude_globs})
        if(exclude_files)
          list(REMOVE_ITEM in_files ${exclude_files})
        endif()
    endif()

    # And do the copying
    foreach(infile ${in_files})
        set(output "${dircopy_DESTINATION}/${infile}")
        set(input "${dircopy_RELATIVE}/${infile}")
        get_filename_component(output_abs "${output}" ABSOLUTE)
        get_filename_component(input_abs "${input}" ABSOLUTE)
        set(verbosity COMMENT "Copying ${infile} to ${dircopy_DESTINATION}")
        if(NOT ${dircopy_VERBOSE})
            unset(verbosity)
        endif()
        if(NOT "${input_abs}" STREQUAL "${output_abs}")
            add_custom_command(
                TARGET ${dircopy_TARGET}
                PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${input_abs} ${output_abs}
                ${verbosity}
            )
        endif()
    endforeach()
endfunction()

