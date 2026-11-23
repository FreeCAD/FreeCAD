# SPDX-License-Identifier: BSL-1.0
# Copyright 2022 Andy Maloney <asmaloney@gmail.com>

target_sources( ${PROJECT_NAME}
    PRIVATE
        main.cpp
        Helpers.cpp
        RandomNum.cpp
        TestData.cpp
        test_ImageFile.cpp
        test_extension_DIST.cpp
        test_SimpleData.cpp
        test_SimpleReader.cpp
        test_SimpleWriter.cpp
)

# Include internal tests if not building shared lib.
# The functions are not exported.
if ( NOT E57_BUILD_SHARED )
    target_sources( ${PROJECT_NAME}
        PRIVATE
           test_StringFunctions.cpp
    )
endif()
