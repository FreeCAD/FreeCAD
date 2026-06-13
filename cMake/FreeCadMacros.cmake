include (CheckCXXSourceRuns)

# ================================================================================
# == Macros, mostly for special targets ==========================================

MACRO (fc_copy_sources target_name outpath)
	if(BUILD_VERBOSE_GENERATION)
		set(fc_details " (fc_copy_sources called from ${CMAKE_CURRENT_SOURCE_DIR})")
	else()
		set(fc_details "")
	endif()
	if(INSTALL_PREFER_SYMLINKS)
		set(copy_command create_symlink)
	else()
		set(copy_command copy)
	endif()

	foreach(it ${ARGN})
		get_filename_component(infile ${it} ABSOLUTE)
		get_filename_component(outfile "${outpath}/${it}" ABSOLUTE)
		# Ensure parent directory exists when copying or creating symlinks
		get_filename_component(outfile_dir "${outfile}" PATH)
		add_file_dependencies("${infile}" "${outfile}")
		ADD_CUSTOM_COMMAND(
			# Make sure destination directory exists before copy/symlink
			COMMAND   "${CMAKE_COMMAND}" -E make_directory "${outfile_dir}"
			COMMAND   "${CMAKE_COMMAND}" -E ${copy_command} "${infile}" "${outfile}"
			OUTPUT   "${outfile}"
			COMMENT "Copying ${infile} to ${outfile}${fc_details}"
			MAIN_DEPENDENCY "${infile}"
		)
	endforeach(it)
ENDMACRO(fc_copy_sources)

MACRO (fc_copy_file_if_different inputfile outputfile)
    if (EXISTS ${inputfile})
        if (EXISTS ${outputfile})
            execute_process(
                COMMAND ${CMAKE_COMMAND} -E compare_files ${inputfile}
                                                          ${outputfile}
                RESULT_VARIABLE DIFFERENT_FILES
                OUTPUT_QUIET
                ERROR_QUIET
            )

            if (DIFFERENT_FILES)
                execute_process(COMMAND "${CMAKE_COMMAND}" -E copy "${inputfile}"
                                                                   "${outputfile}")
            endif()
        else()
            execute_process(COMMAND "${CMAKE_COMMAND}" -E copy "${inputfile}"
                                                               "${outputfile}")
        endif()
    endif()
ENDMACRO(fc_copy_file_if_different)

MACRO (fc_target_copy_resource target_name inpath outpath)
# Macro to copy a list of files into a nested directory structure
# Arguments -
#   target_name - name of the target the files will be added to
#   inpath      - name of the source directory
#   outpath     - name of the destination directory
#   ARGN        - a list of relative file names that will be copied
#
#   If a relative file name is foo/bar.txt then the foo directory
#   part will be kept so that the destination file name will be
#   ${outpath}/foo/bar.txt
#
	if(BUILD_VERBOSE_GENERATION)
		set(fc_details " (fc_target_copy_resource called from ${CMAKE_CURRENT_SOURCE_DIR})")
	else()
		set(fc_details "")
	endif()
	foreach(it ${ARGN})
		get_filename_component(infile "${inpath}/${it}" ABSOLUTE)
		get_filename_component(outfile "${outpath}/${it}" ABSOLUTE)
		add_file_dependencies("${infile}" "${outfile}")
		ADD_CUSTOM_COMMAND(
			COMMAND   "${CMAKE_COMMAND}" -E copy "${infile}" "${outfile}"
			OUTPUT   "${outfile}"
			COMMENT "Copying ${infile} to ${outfile}${fc_details}"
			MAIN_DEPENDENCY "${infile}"
		)
	endforeach(it)
ENDMACRO(fc_target_copy_resource)

MACRO (fc_target_copy_resource_flat target_name inpath outpath)
# Macro to copy a list of files into a flat directory structure
# Arguments -
#   target_name - name of the target the files will be added to
#   inpath      - name of the source directory
#   outpath     - name of the destination directory
#   ARGN        - a list of relative file names that will be copied
#
#   If a relative file name is foo/bar.txt then the foo directory
#   part will be removed so that the destination file name will be
#   ${outpath}/bar.txt
#
	if(BUILD_VERBOSE_GENERATION)
		set(fc_details " (fc_target_copy_resource_flat called from ${CMAKE_CURRENT_SOURCE_DIR})")
	else()
		set(fc_details "")
	endif()
	foreach(it ${ARGN})
		get_filename_component(infile "${inpath}/${it}" ABSOLUTE)
		get_filename_component(outfile "${it}" NAME)
		get_filename_component(outfile "${outpath}/${outfile}" ABSOLUTE)
		add_file_dependencies("${infile}" "${outfile}")
		ADD_CUSTOM_COMMAND(
			COMMAND   "${CMAKE_COMMAND}" -E copy "${infile}" "${outfile}"
			OUTPUT    "${outfile}"
			COMMENT "Copying ${infile} to ${outfile}${fc_details}"
			MAIN_DEPENDENCY "${infile}"
		)
	endforeach(it)
ENDMACRO(fc_target_copy_resource_flat)

# It would be a bit cleaner to generate these files in ${CMAKE_CURRENT_BINARY_DIR}

# To be removed once all instances are migrated to generate_from_py
macro(generate_from_xml BASE_NAME)
    set(TOOL_PATH "${CMAKE_SOURCE_DIR}/src/Tools/bindings/generate.py")
    file(TO_NATIVE_PATH "${TOOL_PATH}" TOOL_NATIVE_PATH)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.xml" SOURCE_NATIVE_PATH)

    set(SOURCE_CPP_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}.cpp" )
    
    # BASE_NAME may include also a path name
    GET_FILENAME_COMPONENT(OUTPUT_PATH "${SOURCE_CPP_PATH}" PATH)
    file(TO_NATIVE_PATH "${OUTPUT_PATH}" OUTPUT_NATIVE_PATH)
    if(NOT EXISTS "${SOURCE_CPP_PATH}")
        # assures the source files are generated at least once
        message(STATUS "${SOURCE_CPP_PATH}")
        execute_process(COMMAND "${Python3_EXECUTABLE}" "${TOOL_NATIVE_PATH}" --outputPath "${OUTPUT_NATIVE_PATH}" "${SOURCE_NATIVE_PATH}"
                        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )
    endif()
    add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}.h" "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}.cpp"
        COMMAND ${Python3_EXECUTABLE} "${TOOL_NATIVE_PATH}" --outputPath "${OUTPUT_NATIVE_PATH}" ${BASE_NAME}.xml
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.xml"
        DEPENDS
        "${CMAKE_SOURCE_DIR}/src/Tools/bindings/templates/templateClassPyExport.py"
        "${TOOL_PATH}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Building ${BASE_NAME}.h/.cpp out of ${BASE_NAME}.xml"
    )
endmacro(generate_from_xml)

function(_freecad_initialize_pyi_dependency_cache)
    get_property(PYI_DEPENDENCY_CACHE_READY GLOBAL PROPERTY FREECAD_PYI_DEPENDENCY_CACHE_READY SET)
    if(PYI_DEPENDENCY_CACHE_READY)
        return()
    endif()
    set_property(GLOBAL PROPERTY FREECAD_PYI_DEPENDENCY_CACHE_READY TRUE)

    set(TOOL_PATH "${CMAKE_SOURCE_DIR}/src/Tools/bindings/generate.py")
    file(TO_NATIVE_PATH "${TOOL_PATH}" TOOL_NATIVE_PATH)

    set(PYI_CMAKE_LISTS "${CMAKE_SOURCE_DIR}/CMakeLists.txt")
    file(GLOB_RECURSE PYI_SUBDIRECTORY_CMAKE_LISTS "${CMAKE_SOURCE_DIR}/*/CMakeLists.txt")
    list(APPEND PYI_CMAKE_LISTS ${PYI_SUBDIRECTORY_CMAKE_LISTS})

    set(PYI_SOURCE_PATHS)
    foreach(PYI_CMAKE_LIST ${PYI_CMAKE_LISTS})
        get_filename_component(PYI_CMAKE_LIST_DIR "${PYI_CMAKE_LIST}" DIRECTORY)
        file(READ "${PYI_CMAKE_LIST}" CURRENT_CMAKE_LISTS)
        string(REGEX MATCHALL "generate_from_py_?\\([ \t\r\n]*\"?([^\" \t\r\n\\)]+)" PYI_GENERATOR_CALLS "${CURRENT_CMAKE_LISTS}")

        foreach(PYI_GENERATOR_CALL ${PYI_GENERATOR_CALLS})
            string(REGEX REPLACE "^generate_from_py_?\\([ \t\r\n]*\"?([^\" \t\r\n\\)]+).*" "\\1" PYI_BASE_NAME "${PYI_GENERATOR_CALL}")
            set(PYI_SOURCE_PATH "${PYI_CMAKE_LIST_DIR}/${PYI_BASE_NAME}.pyi")
            if(EXISTS "${PYI_SOURCE_PATH}")
                file(TO_NATIVE_PATH "${PYI_SOURCE_PATH}" PYI_SOURCE_NATIVE_PATH)
                list(APPEND PYI_SOURCE_PATHS "${PYI_SOURCE_NATIVE_PATH}")
            endif()
        endforeach()
    endforeach()

    if(NOT PYI_SOURCE_PATHS)
        return()
    endif()
    list(REMOVE_DUPLICATES PYI_SOURCE_PATHS)

    execute_process(
        COMMAND "${Python3_EXECUTABLE}" "${TOOL_NATIVE_PATH}" --print-dependency-map ${PYI_SOURCE_PATHS}
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE PYI_DEPENDENCY_MAP
        OUTPUT_STRIP_TRAILING_WHITESPACE
        COMMAND_ERROR_IS_FATAL ANY
    )

    string(JSON PYI_DEPENDENCY_RECORD_COUNT LENGTH "${PYI_DEPENDENCY_MAP}")
    if(PYI_DEPENDENCY_RECORD_COUNT EQUAL 0)
        return()
    endif()

    math(EXPR PYI_DEPENDENCY_RECORD_LAST "${PYI_DEPENDENCY_RECORD_COUNT} - 1")
    foreach(PYI_DEPENDENCY_RECORD_INDEX RANGE 0 ${PYI_DEPENDENCY_RECORD_LAST})
        string(JSON PYI_SOURCE_PATH GET "${PYI_DEPENDENCY_MAP}" ${PYI_DEPENDENCY_RECORD_INDEX} source)
        string(JSON PYI_DEPENDENCY_COUNT LENGTH "${PYI_DEPENDENCY_MAP}" ${PYI_DEPENDENCY_RECORD_INDEX} dependencies)

        set(PYI_SOURCE_DEPENDENCIES)
        if(PYI_DEPENDENCY_COUNT GREATER 0)
            math(EXPR PYI_DEPENDENCY_LAST "${PYI_DEPENDENCY_COUNT} - 1")
            foreach(PYI_DEPENDENCY_INDEX RANGE 0 ${PYI_DEPENDENCY_LAST})
                string(JSON PYI_DEPENDENCY GET "${PYI_DEPENDENCY_MAP}" ${PYI_DEPENDENCY_RECORD_INDEX} dependencies ${PYI_DEPENDENCY_INDEX})
                list(APPEND PYI_SOURCE_DEPENDENCIES "${PYI_DEPENDENCY}")
            endforeach()
        endif()

        string(MD5 PYI_SOURCE_KEY "${PYI_SOURCE_PATH}")
        set_property(GLOBAL PROPERTY "FREECAD_PYI_SOURCE_DEPENDENCIES_${PYI_SOURCE_KEY}" "${PYI_SOURCE_DEPENDENCIES}")
    endforeach()
endfunction()

function(_freecad_get_pyi_source_dependencies BASE_NAME OUT_VAR)
    _freecad_initialize_pyi_dependency_cache()

    file(REAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.pyi" PYI_SOURCE_PATH)
    string(MD5 PYI_SOURCE_KEY "${PYI_SOURCE_PATH}")
    get_property(PYI_SOURCE_DEPENDENCIES GLOBAL PROPERTY "FREECAD_PYI_SOURCE_DEPENDENCIES_${PYI_SOURCE_KEY}")
    set(${OUT_VAR} "${PYI_SOURCE_DEPENDENCIES}" PARENT_SCOPE)
endfunction()

macro(generate_from_py_impl BASE_NAME SUFFIX)
    set(TOOL_PATH "${CMAKE_SOURCE_DIR}/src/Tools/bindings/generate.py")
    file(TO_NATIVE_PATH "${TOOL_PATH}" TOOL_NATIVE_PATH)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.pyi" SOURCE_NATIVE_PATH)

    set(SOURCE_CPP_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}Py${SUFFIX}.cpp")
    set(SOURCE_H_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}Py${SUFFIX}.h")
    set(SOURCE_DEPFILE_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}Py${SUFFIX}.d")

    # BASE_NAME may include also a path name
    GET_FILENAME_COMPONENT(OUTPUT_PATH "${SOURCE_CPP_PATH}" PATH)
    file(TO_NATIVE_PATH "${OUTPUT_PATH}" OUTPUT_NATIVE_PATH)
    file(TO_NATIVE_PATH "${SOURCE_DEPFILE_PATH}" SOURCE_DEPFILE_NATIVE_PATH)

    _freecad_get_pyi_source_dependencies("${BASE_NAME}" PYI_SOURCE_DEPENDENCIES)

    if(NOT EXISTS "${SOURCE_CPP_PATH}")
        # Ensure the source files are generated at least once.
        message(STATUS "${SOURCE_CPP_PATH}")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" "${TOOL_NATIVE_PATH}" --outputPath "${OUTPUT_NATIVE_PATH}" --depfile "${SOURCE_DEPFILE_NATIVE_PATH}" "${SOURCE_NATIVE_PATH}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    add_custom_command(
        OUTPUT "${SOURCE_H_PATH}" "${SOURCE_CPP_PATH}"
        COMMAND ${Python3_EXECUTABLE} "${TOOL_NATIVE_PATH}" --outputPath "${OUTPUT_NATIVE_PATH}" --depfile "${SOURCE_DEPFILE_NATIVE_PATH}" ${BASE_NAME}.pyi
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.pyi"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/src/Tools/bindings/templates/templateClassPyExport.py"
            "${TOOL_PATH}"
            ${PYI_SOURCE_DEPENDENCIES}
        DEPFILE "${SOURCE_DEPFILE_PATH}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Building ${BASE_NAME}Py${SUFFIX}.h/.cpp out of ${BASE_NAME}.pyi"
    )
endmacro(generate_from_py_impl)

macro(generate_from_py BASE_NAME)
    generate_from_py_impl(${BASE_NAME} "")
endmacro(generate_from_py)

macro(generate_from_py_ BASE_NAME)
    generate_from_py_impl(${BASE_NAME} "_")
endmacro(generate_from_py_)

macro(generate_module_from_py BASE_NAME)
    set(TOOL_PATH "${CMAKE_SOURCE_DIR}/src/Tools/bindings/generate.py")
    file(TO_NATIVE_PATH "${TOOL_PATH}" TOOL_NATIVE_PATH)
    file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.module.pyi" SOURCE_NATIVE_PATH)

    set(SOURCE_CPP_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}ModulePy.cpp")
    set(SOURCE_H_PATH "${CMAKE_CURRENT_BINARY_DIR}/${BASE_NAME}ModulePy.h")

    GET_FILENAME_COMPONENT(OUTPUT_PATH "${SOURCE_CPP_PATH}" PATH)
    file(TO_NATIVE_PATH "${OUTPUT_PATH}" OUTPUT_NATIVE_PATH)
    if(NOT EXISTS "${SOURCE_CPP_PATH}")
        message(STATUS "${SOURCE_CPP_PATH}")
        execute_process(
            COMMAND "${Python3_EXECUTABLE}" "${TOOL_NATIVE_PATH}" --outputPath "${OUTPUT_NATIVE_PATH}" "${SOURCE_NATIVE_PATH}"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            COMMAND_ERROR_IS_FATAL ANY
        )
    endif()

    add_custom_command(
        OUTPUT "${SOURCE_H_PATH}" "${SOURCE_CPP_PATH}"
        COMMAND ${Python3_EXECUTABLE} "${TOOL_NATIVE_PATH}" --outputPath "${OUTPUT_NATIVE_PATH}" ${BASE_NAME}.module.pyi
        MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.module.pyi"
        DEPENDS
            "${CMAKE_SOURCE_DIR}/src/Tools/bindings/templates/templateModulePyExport.py"
            "${TOOL_PATH}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Building ${BASE_NAME}ModulePy.h/.cpp out of ${BASE_NAME}.module.pyi"
    )
endmacro(generate_module_from_py)

macro(generate_embed_from_py BASE_NAME OUTPUT_FILE)
		set(TOOL_PATH "${CMAKE_SOURCE_DIR}/src/Tools/PythonToCPP.py")
		file(TO_NATIVE_PATH "${TOOL_PATH}" TOOL_NATIVE_PATH)
		file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.py" SOURCE_NATIVE_PATH)
		add_custom_command(
		 		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}"
		 		COMMAND "${Python3_EXECUTABLE}" "${TOOL_NATIVE_PATH}" "${SOURCE_NATIVE_PATH}" "${OUTPUT_FILE}"
				MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${BASE_NAME}.py"
				DEPENDS "${TOOL_PATH}"
				WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
				COMMENT "Building files out of ${BASE_NAME}.py")
endmacro(generate_embed_from_py)

macro(generate_from_any INPUT_FILE OUTPUT_FILE VARIABLE)
		set(TOOL_PATH "${CMAKE_SOURCE_DIR}/src/Tools/PythonToCPP.py")
		file(TO_NATIVE_PATH "${TOOL_PATH}" TOOL_NATIVE_PATH)
		file(TO_NATIVE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}" SOURCE_NATIVE_PATH)
		add_custom_command(
		 		OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${OUTPUT_FILE}"
		 		COMMAND "${Python3_EXECUTABLE}" "${TOOL_NATIVE_PATH}" "${SOURCE_NATIVE_PATH}" "${OUTPUT_FILE}" "${VARIABLE}"
				MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${INPUT_FILE}"
				DEPENDS "${TOOL_PATH}"
				WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
				COMMENT "Building files out of ${INPUT_FILE}")
endmacro(generate_from_any)


# Macro to replace all the binary output locations.  Takes 2 optional parameters.
# ${ARGVN} is zero based so the 3rd element is ${ARGV2}.  When the 3rd element is missing,
# Runtime and Lib directories default to /bin and /lib.  When present, the 3rd element
# specifies both Runtime and Lib directories.  4th specifies linux install path.
MACRO(SET_BIN_DIR ProjectName OutputName)
    set_target_properties(${ProjectName} PROPERTIES OUTPUT_NAME ${OutputName})
    if(${ARGC} GREATER 2)
        # VS_IDE (and perhaps others) make Release and Debug subfolders.  This removes them.
        set_target_properties(${ProjectName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}${ARGV2})
        set_target_properties(${ProjectName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}${ARGV2})
        set_target_properties(${ProjectName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}${ARGV2})
        set_target_properties(${ProjectName} PROPERTIES LIBRARY_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}${ARGV2})
        set_target_properties(${ProjectName} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}${ARGV2})
        set_target_properties(${ProjectName} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}${ARGV2})
    else(${ARGC} GREATER 2)
        set_target_properties(${ProjectName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/bin)
        set_target_properties(${ProjectName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
        set_target_properties(${ProjectName} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/bin)
        set_target_properties(${ProjectName} PROPERTIES LIBRARY_OUTPUT_DIRECTORY         ${CMAKE_BINARY_DIR}/lib)
        set_target_properties(${ProjectName} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib)
        set_target_properties(${ProjectName} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_BINARY_DIR}/lib)
    endif(${ARGC} GREATER 2)

    if(WIN32)
        set_target_properties(${ProjectName} PROPERTIES DEBUG_OUTPUT_NAME ${OutputName}_d)
    else(WIN32)
        # FreeCADBase, SMDS, Driver and MEFISTO2 libs don't depend on parts from CMAKE_INSTALL_LIBDIR
        if(NOT ${ProjectName} MATCHES "^(FreeCADBase|SMDS|Driver|MEFISTO2)$")
            if(${ARGC} STREQUAL 4)
                set_property(TARGET ${ProjectName} APPEND PROPERTY INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${ARGV3})
            elseif(NOT IS_ABSOLUTE ${CMAKE_INSTALL_LIBDIR})
                set_property(TARGET ${ProjectName} APPEND PROPERTY INSTALL_RPATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
            else()
                set_property(TARGET ${ProjectName} APPEND PROPERTY INSTALL_RPATH ${CMAKE_INSTALL_LIBDIR})
            endif()
        endif()
    endif(WIN32)
ENDMACRO(SET_BIN_DIR)

# Set python prefix & suffix together
MACRO(SET_PYTHON_PREFIX_SUFFIX ProjectName)
    if(NOT MSVC)
        set_target_properties(${ProjectName} PROPERTIES PREFIX "")
    endif(NOT MSVC)
    
    if(WIN32)
        set_target_properties(${ProjectName} PROPERTIES SUFFIX ".pyd")
    # 0000661: cmake build on Mac OS: dealing with dylib versus so
    elseif(APPLE)
        set_target_properties(${ProjectName} PROPERTIES SUFFIX ".so")
    endif(WIN32)
ENDMACRO(SET_PYTHON_PREFIX_SUFFIX)

# Locate the include directory for a pip-installed package -- uses pip show to find the base pip
# install directory, and then appends the package name and  "/include" to the end
macro(find_pip_package PACKAGE)
	unset(${PACKAGE}_FOUND)  # Remove from local scope
	unset(${PACKAGE}_FOUND CACHE)  # Remove from CMake cache (if it exists)
	execute_process(
			COMMAND ${Python3_EXECUTABLE} -m pip show ${PACKAGE}
			RESULT_VARIABLE FAILURE
			OUTPUT_VARIABLE PRINT_OUTPUT
	)
	if(NOT FAILURE)
		# Extract Name: and Location: lines and use them to construct the include directory
		string(REPLACE "\n" ";" PIP_OUTPUT_LINES ${PRINT_OUTPUT})
		foreach(LINE IN LISTS PIP_OUTPUT_LINES)
			STRING(FIND "${LINE}" "Name: " NAME_STRING_LOCATION)
			STRING(FIND "${LINE}" "Location: " LOCATION_STRING_LOCATION)
			if(${NAME_STRING_LOCATION} EQUAL 0)
				STRING(SUBSTRING "${LINE}" 6 -1 PIP_PACKAGE_NAME)
				STRING(STRIP "${PIP_PACKAGE_NAME}" PIP_PACKAGE_NAME)
				STRING(REPLACE "-" "_" PIP_PACKAGE_NAME "${PIP_PACKAGE_NAME}")
			elseif(${LOCATION_STRING_LOCATION} EQUAL 0)
				STRING(SUBSTRING "${LINE}" 9 -1 PIP_PACKAGE_LOCATION)
				STRING(STRIP "${PIP_PACKAGE_LOCATION}" PIP_PACKAGE_LOCATION)
			endif()
		endforeach()
		file(TO_CMAKE_PATH "${PIP_PACKAGE_LOCATION}" PIP_PACKAGE_LOCATION)
		if(EXISTS "${PIP_PACKAGE_LOCATION}/${PIP_PACKAGE_NAME}/include")
			set(INCLUDE_DIR "${PIP_PACKAGE_LOCATION}/${PIP_PACKAGE_NAME}/include")
		endif()
		# There are many different library naming schemes, but basically we are looking for things that look like
		#  PACKAGE*.lib/so or PACKAGE_d*.lib/so
		if(WIN32)
			file(GLOB OPT_LIBRARIES "${PIP_PACKAGE_LOCATION}/${PIP_PACKAGE_NAME}/${PIP_PACKAGE_NAME}.*.lib")
			file(GLOB DEBUG_LIBRARIES "${PIP_PACKAGE_LOCATION}/${PIP_PACKAGE_NAME}/${PIP_PACKAGE_NAME}_d.*.lib")
		else()
			string(TOLOWER ${PIP_PACKAGE_NAME} PIP_LIB_NAME)
			file(GLOB OPT_LIBRARIES "${PIP_PACKAGE_LOCATION}/${PIP_PACKAGE_NAME}/*${PIP_LIB_NAME}*.so.*")
		endif()
		if (OPT_LIBRARIES AND DEBUG_LIBRARIES)
			set(${PACKAGE}_LIBRARIES optimized ${OPT_LIBRARIES} debug ${DEBUG_LIBRARIES} CACHE PATH "Location of the ${PACKAGE} libraries")
		elseif(OPT_LIBRARIES)
			set(${PACKAGE}_LIBRARIES ${OPT_LIBRARIES} CACHE PATH "Location of the ${PACKAGE} optimized libraries")
		elseif(DEBUG_LIBRARIES)
			set(${PACKAGE}_LIBRARIES ${DEBUG_LIBRARIES} CACHE PATH "Location of the ${PACKAGE} debug libraries")
		endif()
		set(${PACKAGE}_INCLUDE_DIRS ${INCLUDE_DIR} CACHE PATH "Location of the ${PACKAGE} includes")
		set(${PACKAGE}_FOUND ON)
		message(STATUS "Found pip-installed ${PACKAGE} in ${PIP_PACKAGE_LOCATION}/${PIP_PACKAGE_NAME}")
		if(${PACKAGE}_INCLUDE_DIRS)
			message(STATUS "  --> with includes in ${${PACKAGE}_INCLUDE_DIRS}")
		endif()
	endif()
endmacro()

function(target_compile_warn_error ProjectName)
    if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
        target_compile_options(${ProjectName} PRIVATE -Werror)
    endif()
endfunction()

