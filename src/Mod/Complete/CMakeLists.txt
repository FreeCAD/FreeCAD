if(BUILD_GUI)
    add_subdirectory(Gui)
endif(BUILD_GUI)

SET(Complete_SRCS
    Init.py
    InitGui.py
)

SOURCE_GROUP("" FILES ${Complete_SRCS})

ADD_CUSTOM_TARGET(Complete ALL
    SOURCES ${Complete_SRCS}
)

fc_copy_sources(Complete "${CMAKE_BINARY_DIR}/Mod/Complete" ${Complete_SRCS})

INSTALL(
    FILES
        ${Complete_SRCS}
    DESTINATION
        Mod/Complete
)
