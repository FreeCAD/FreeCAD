# SPDX-License-Identifier: LGPL-2.1-or-later

if(NOT DEFINED INPUTFILE OR NOT DEFINED OUTPUTFILE)
    message(FATAL_ERROR "LinkOrCopy.cmake requires INPUTFILE and OUTPUTFILE")
endif()

get_filename_component(_output_dir "${OUTPUTFILE}" DIRECTORY)
file(MAKE_DIRECTORY "${_output_dir}")
file(CREATE_LINK "${INPUTFILE}" "${OUTPUTFILE}" SYMBOLIC COPY_ON_ERROR RESULT _link_result)

if(NOT _link_result STREQUAL "0")
    message(FATAL_ERROR "Failed to mirror ${INPUTFILE} to ${OUTPUTFILE}: ${_link_result}")
endif()
