# SPDX-License-Identifier: LGPL-2.1-or-later

add_executable(Import_tests_run
        DxfImport.cpp
)

set(Import_tests_data_files
        TestData/Issue24314.dxf
)

ADD_CUSTOM_TARGET(Import_tests_data ALL
        SOURCES ${Import_tests_data_files}
)

fc_target_copy_resource(Import_tests_data
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}/tests
        ${Import_tests_data_files}
)
