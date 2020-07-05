add_subdirectory(examples)

if(INSTALL_DOCS)
INSTALL(
    FILES
        License.txt
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}
)
endif()
