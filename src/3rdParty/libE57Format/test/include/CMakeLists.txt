# SPDX-License-Identifier: MIT
# Copyright 2022 Andy Maloney <asmaloney@gmail.com>

target_sources( ${PROJECT_NAME}
	PRIVATE
	    ${CMAKE_CURRENT_SOURCE_DIR}/Helpers.h
		${CMAKE_CURRENT_SOURCE_DIR}/RandomNum.h
		${CMAKE_CURRENT_SOURCE_DIR}/TestData.h
)

target_include_directories( ${PROJECT_NAME}
	PUBLIC
	    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
