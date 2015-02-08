set(PYTHON_DIR_BASENAME python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})

if(PYTHON_LIBRARY MATCHES "(.*Python\\.framework).*")
    #framework
    set(PYTHON_DIR ${CMAKE_MATCH_1}/Versions/Current/lib/${PYTHON_DIR_BASENAME})
else()
    #unix
    get_filename_component(PYTHON_DIR ${PYTHON_LIBRARY} PATH)
    set(PYTHON_DIR ${PYTHON_DIR}/${PYTHON_DIR_BASENAME})
endif()

install(CODE "execute_process(COMMAND
    ${CMAKE_COMMAND} -E copy_directory ${PYTHON_DIR} ${CMAKE_INSTALL_LIBDIR}/${PYTHON_DIR_BASENAME}
)")
install(DIRECTORY ${QT_PLUGINS_DIR}/ DESTINATION ${CMAKE_INSTALL_LIBDIR}/qtplugins)

#files installed by homebrew do not have write permission for regular user
install(CODE "execute_process(COMMAND chmod -R a+w ${CMAKE_INSTALL_LIBDIR})")

get_filename_component(APP_PATH ${CMAKE_INSTALL_PREFIX} PATH)

install(CODE 
    "message(STATUS \"Making bundle relocatable...\")
    execute_process(
        COMMAND ${PYTHON_EXECUTABLE} 
        ${CMAKE_SOURCE_DIR}/src/Tools/MakeMacBundleRelocatable.py ${APP_PATH}
    )"
)
