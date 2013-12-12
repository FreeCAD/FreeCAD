# ================================================================================
# == Macros, mostly for special targets ==========================================

MACRO(COPY_IF_DIFFERENT FROM_DIR TO_DIR FILES TARGETS TAGS)
# Macro to implement copy_if_different for a list of files
# Arguments - 
#   FROM_DIR - this is the source directory
#   TO_DIR   - this is the destination directory
#   FILES    - names of the files to copy 
#              TODO: add globing. 
#   TARGETS  - List of targets
#   TAGS     - Since only the file name is used
#              to generate rules, pre-pend a user 
#              supplied tag to prevent duplicate rule errors. 
SET(AddTargets "")
FOREACH(SRC ${FILES})
    GET_FILENAME_COMPONENT(SRCFILE ${SRC} NAME) 
    # Command to copy the files to ${STEP1}/src, if changed.
    SET(TARGET  "${TAGS}/${SRCFILE}")
    IF("${FROM_DIR}" STREQUAL "")
        SET(FROM ${SRC})
    ELSE("${FROM_DIR}" STREQUAL "")
        SET(FROM ${FROM_DIR}/${SRC})
    ENDIF("${FROM_DIR}" STREQUAL "")        
    IF("${TO_DIR}" STREQUAL "")
        SET(TO ${SRCFILE})
    ELSE("${TO_DIR}" STREQUAL "")
        SET(TO   ${TO_DIR}/${SRCFILE})
    ENDIF("${TO_DIR}" STREQUAL "")
    ADD_CUSTOM_COMMAND(
        OUTPUT  ${TARGET}
        COMMAND ${CMAKE_COMMAND}
        ARGS    -E copy_if_different ${FROM} ${TO}
        COMMENT "Copying ${SRCFILE} ${TO_DIR}"
        )
    SET(AddTargets ${AddTargets} ${TARGET})
ENDFOREACH(SRC ${FILES})
SET(${TARGETS} ${AddTargets})
ENDMACRO(COPY_IF_DIFFERENT FROM_DIR TO_DIR FILES TARGETS TAGS)

MACRO (fc_copy_sources target_name outpath)
	foreach(it ${ARGN})
		file(TO_NATIVE_PATH "${outpath}/${it}" outfile)
		get_filename_component(infile ${it} ABSOLUTE)
		get_filename_component(outfile ${outfile} ABSOLUTE)
		add_file_dependencies(${infile} ${outfile})
		ADD_CUSTOM_COMMAND(
			SOURCE    ${infile}
			COMMAND   ${CMAKE_COMMAND}
			ARGS      -E copy ${infile} ${outfile}
			TARGET    ${target_name}
			OUTPUTS   ${outfile}
		)
	endforeach(it)
	ADD_CUSTOM_COMMAND(
		SOURCE    ${target_name}
		TARGET    ${target_name}
		DEPENDS   ${ARGN}
	)
ENDMACRO(fc_copy_sources)

MACRO (fc_target_copy_resource target_name inpath outpath)
	foreach(it ${ARGN})
		file(TO_NATIVE_PATH "${inpath}/${it}" infile)
		file(TO_NATIVE_PATH "${outpath}/${it}" outfile)
		get_filename_component(infile ${infile} ABSOLUTE)
		get_filename_component(outfile ${outfile} ABSOLUTE)
		add_file_dependencies(${infile} ${outfile})
		ADD_CUSTOM_COMMAND(
			SOURCE    ${infile}
			COMMAND   ${CMAKE_COMMAND}
			ARGS      -E copy ${infile} ${outfile}
			TARGET    ${target_name}
			OUTPUTS   ${outfile}
		)
	endforeach(it)
	ADD_CUSTOM_COMMAND(
		SOURCE    ${target_name}
		TARGET    ${target_name}
		DEPENDS   ${ARGN}
	)
ENDMACRO(fc_target_copy_resource)

macro(copy_to_local_output_paths SOURCE_PATHS)
		 if(CMAKE_CFG_INTDIR STREQUAL .)
		 		 # No Debug/Release output paths
		 		 set(DEBUG_LOCAL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
		 		 set(RELEASE_LOCAL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
		 else(CMAKE_CFG_INTDIR STREQUAL .)
		 		 #set(DEBUG_LOCAL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/Debug)
		 		 #set(RELEASE_LOCAL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/Release)
		 		 set(DEBUG_LOCAL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
		 		 set(RELEASE_LOCAL_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR})
		 endif(CMAKE_CFG_INTDIR STREQUAL .)
		 file(TO_NATIVE_PATH ${SOURCE_PATHS} NATIVE_SOURCE)
		 file(TO_NATIVE_PATH ${DEBUG_LOCAL_OUTPUT_PATH}/ NATIVE_DEBUG_DESTINATION)
		 file(TO_NATIVE_PATH ${RELEASE_LOCAL_OUTPUT_PATH}/ NATIVE_RELESE_DESTINATION)
		 message(STATUS "${PLATFORM_CP} ${NATIVE_SOURCE} ${NATIVE_DEBUG_DESTINATION}")
		 execute_process(
		 		 COMMAND ${PLATFORM_CP} ${NATIVE_SOURCE} ${NATIVE_DEBUG_DESTINATION}
		 		 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		 if(NOT ${DEBUG_LOCAL_OUTPUT_PATH} STREQUAL ${RELEASE_LOCAL_OUTPUT_PATH})
		 		 message(STATUS "${PLATFORM_CP} ${NATIVE_SOURCE} ${NATIVE_RELESE_DESTINATION}")
		 		 execute_process(
		 		 		 COMMAND ${PLATFORM_CP} ${NATIVE_SOURCE} ${NATIVE_RELESE_DESTINATION}
		 		 		 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		 endif(NOT ${DEBUG_LOCAL_OUTPUT_PATH} STREQUAL ${RELEASE_LOCAL_OUTPUT_PATH})
endmacro(copy_to_local_output_paths)

macro(copy_to_main_output_paths SOURCE_PATHS)
		 file(TO_NATIVE_PATH ${SOURCE_PATHS} NATIVE_SOURCE)
		 file(TO_NATIVE_PATH ${DEBUG_MAIN_OUTPUT_PATH}/ NATIVE_DEBUG_DESTINATION)
		 file(TO_NATIVE_PATH ${RELEASE_MAIN_OUTPUT_PATH}/ NATIVE_RELESE_DESTINATION)
		 message(STATUS "${PLATFORM_CP} ${NATIVE_SOURCE} ${NATIVE_DEBUG_DESTINATION}")
		 execute_process(
		 		 COMMAND ${PLATFORM_CP} ${NATIVE_SOURCE} ${NATIVE_DEBUG_DESTINATION}
		 		 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		 if(NOT ${DEBUG_MAIN_OUTPUT_PATH} STREQUAL ${RELEASE_MAIN_OUTPUT_PATH})
		 		 message(STATUS "${PLATFORM_CP} ${NATIVE_SOURCE} ${NATIVE_RELESE_DESTINATION}")
		 		 execute_process(
		 		 		 COMMAND ${PLATFORM_CP} ${NATIVE_SOURCE} ${NATIVE_RELESE_DESTINATION}
		 		 		 WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		 endif(NOT ${DEBUG_MAIN_OUTPUT_PATH} STREQUAL ${RELEASE_MAIN_OUTPUT_PATH})
endmacro(copy_to_main_output_paths)

# It would be a bit cleaner to generate these files in ${CMAKE_CURRENT_BINARY_DIR}

macro(generate_from_xml BASE_NAME)
    file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/src/Tools/generate.py TOOL_PATH)
    file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.xml SOURCE_PATH)

    file(TO_NATIVE_PATH ${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}.cpp SOURCE_CPP_PATH)
    # BASE_NAME may include also a path name
    GET_FILENAME_COMPONENT(OUTPUT_PATH ${SOURCE_CPP_PATH} PATH)
    if (NOT EXISTS ${SOURCE_CPP_PATH})
        # assures the source files are generated at least once
        message(STATUS "${SOURCE_CPP_PATH}")
        execute_process(COMMAND ${PYTHON_EXECUTABLE} ${TOOL_PATH} --outputPath ${OUTPUT_PATH} ${SOURCE_PATH}
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif (NOT EXISTS ${SOURCE_CPP_PATH})

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}.h ${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}.cpp
        COMMAND ${PYTHON_EXECUTABLE} ${TOOL_PATH} --outputPath ${OUTPUT_PATH} ${BASE_NAME}.xml
        MAIN_DEPENDENCY ${BASE_NAME}.xml
        DEPENDS ${CMAKE_SOURCE_DIR}/src/Tools/generateTemplates/templateClassPyExport.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT Building ${BASE_NAME}.h/.cpp out of ${BASE_NAME}.xml
    )
endmacro(generate_from_xml)

macro(generate_from_py BASE_NAME OUTPUT_FILE)
		 file(TO_NATIVE_PATH ${CMAKE_SOURCE_DIR}/src/Tools/PythonToCPP.py TOOL_PATH)
		 file(TO_NATIVE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.py SOURCE_PATH)
		 add_custom_command(
		 		 OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}
		 		 COMMAND ${PYTHON_EXECUTABLE} ${TOOL_PATH} ${SOURCE_PATH} ${OUTPUT_FILE}
		 		 MAIN_DEPENDENCY ${BASE_NAME}.py
		 		 WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		 		 COMMENT Building files out of ${BASE_NAME}.py)
endmacro(generate_from_py)


# generates the ui -> cpp h files
#macro(qt4_wrap_ui infiles )
#
#endmacro(qt4_wrap_ui)


MACRO(ADD_MSVC_PRECOMPILED_HEADER PrecompiledHeader PrecompiledSource SourcesVar)
  IF(MSVC)
    GET_FILENAME_COMPONENT(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    SET(PrecompiledBinary "$(IntDir)\\$(TargetName).pch")
    SET(Sources ${${SourcesVar}})

    SET_SOURCE_FILES_PROPERTIES(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")
    SET_SOURCE_FILES_PROPERTIES(${Sources}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /FI\"${PrecompiledBinary}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")  
    # Add precompiled header to SourcesVar
    LIST(APPEND ${SourcesVar} ${PrecompiledSource})
  ENDIF(MSVC)
ENDMACRO(ADD_MSVC_PRECOMPILED_HEADER)

MACRO(GET_MSVC_PRECOMPILED_SOURCE PrecompiledSource SourcesVar)
  IF(MSVC)
    FOREACH (it ${ARGN})
      GET_FILENAME_COMPONENT(file_ext ${it} EXT)
      GET_FILENAME_COMPONENT(file_name ${it} NAME)
	  STRING(COMPARE EQUAL ${it} ${PrecompiledSource} pch)
	  IF (NOT pch)
	    # get c++ source files
		STRING(REGEX MATCH "^(.cpp|.cc|.cxx)$" cpp_file ${file_ext})
		# ignore any generated source files from Qt
		STRING(REGEX MATCH "^(moc_|qrc_|ui_)" gen_file ${file_name})
		IF(cpp_file AND NOT gen_file)
			LIST(APPEND ${SourcesVar} ${it})
		ENDIF(cpp_file AND NOT gen_file)
	  ENDIF(NOT pch)
    ENDFOREACH (it)
  ENDIF(MSVC)
ENDMACRO(GET_MSVC_PRECOMPILED_SOURCE)
