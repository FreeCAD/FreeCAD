add_subdirectory(examples)
add_subdirectory(settings)

INSTALL(
    FILES
        License.txt
    DESTINATION
        ${CMAKE_INSTALL_DATADIR}
)

