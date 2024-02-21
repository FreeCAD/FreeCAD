# SPDX-License-Identifier: MIT
# Copyright 2020 Andy Maloney <asmaloney@gmail.com>

target_sources( ${PROJECT_NAME}
	PRIVATE
		E57Exception.h
		E57Format.h
		E57SimpleData.h
		E57SimpleReader.h
		E57SimpleWriter.h
		E57Version.h
)

install(
	FILES
		E57Format.h
		E57Exception.h
		E57SimpleData.h
		E57SimpleReader.h
		E57SimpleWriter.h
		E57Version.h
	DESTINATION
		include/E57Format
)

target_include_directories( ${PROJECT_NAME}
	PUBLIC
		$<INSTALL_INTERFACE:include/E57Format>
		$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
