function(set_swig_properties NAME)
    set_property(SOURCE ${NAME} PROPERTY CPLUSPLUS ON)
    set_property(SOURCE ${NAME} PROPERTY SWIG_FLAGS "-includeall")
    set_property(SOURCE ${NAME} APPEND PROPERTY SWIG_FLAGS "-py3")
    set_property(SOURCE ${NAME} APPEND PROPERTY SWIG_FLAGS "-D__PIVY__")
    if(DISABLE_SWIG_WARNINGS)
        set_property(SOURCE ${NAME} APPEND PROPERTY SWIG_FLAGS
            "-w302,306,307,312,314,325,361,362,467,389,503,509,510")
    endif(DISABLE_SWIG_WARNINGS)
endfunction()


set_swig_properties(coin.i)

# remember: the order of the includes is important!
# swig needs the generated headers first (these headers are replacing the origin Inventor headers)
set_property(SOURCE coin.i PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}")
set_property(SOURCE coin.i APPEND PROPERTY INCLUDE_DIRECTORIES "${PROJECT_SOURCE_DIR}/fake_headers")
set_property(SOURCE coin.i APPEND PROPERTY INCLUDE_DIRECTORIES "${Coin_INCLUDE_DIR}")
if (Coin_GENERATED_INCLUDE_DIR)
    set_property(SOURCE coin.i APPEND PROPERTY INCLUDE_DIRECTORIES "${Coin_GENERATED_INCLUDE_DIR}")
endif()

swig_add_library(coin
    LANGUAGE python
    OUTPUT_DIR ${CMAKE_BINARY_DIR}/pivy
    SOURCES coin.i)

if (APPLE)
    set_target_properties(coin PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
endif()

if (MSVC)
    set_target_properties(coin PROPERTIES COMPILE_FLAGS "/bigobj")
endif ()

target_include_directories(coin
    PUBLIC
    ${Coin_INCLUDE_DIR}
    ${Python_INCLUDE_DIRS}
    PRIVATE
    ${PROJECT_SOURCE_DIR}/interfaces
    )

target_link_libraries(coin PUBLIC Coin::Coin Python::Module)
install(TARGETS coin DESTINATION ${PIVY_Python_SITEARCH}/pivy)

cmake_policy(SET CMP0068 NEW)  # Use RUNPATH instead of RPATH, default from CMake 3.9
set_target_properties(coin PROPERTIES BUILD_RPATH "$ORIGIN" INSTALL_RPATH "$ORIGIN")

if (SoQt_FOUND)

    set_swig_properties(soqt.i)

    # remember: the order of the includes is important!
    # swig needs the generated headers first (these headers are replacing the origin Inventor headers)
    set_property(SOURCE soqt.i PROPERTY INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}")
    set_property(SOURCE soqt.i APPEND PROPERTY INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/fake_headers")
    set_property(SOURCE soqt.i APPEND PROPERTY INCLUDE_DIRECTORIES "${SoQt_INCLUDE_DIRS}")
    set_property(SOURCE soqt.i APPEND PROPERTY INCLUDE_DIRECTORIES "${Coin_INCLUDE_DIR}")


    swig_add_library(soqt
        LANGUAGE python
        OUTPUT_DIR ${CMAKE_BINARY_DIR}/pivy/gui
        SOURCES soqt.i)

    if (APPLE)
        set_target_properties(soqt PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
    endif()

    if (MSVC)
        set_target_properties(coin PROPERTIES COMPILE_FLAGS "/bigobj")
    endif ()

    if (PIVY_USE_QT6)
        set(QT_INC_DIRS ${Qt6Gui_INCLUDE_DIRS} ${Qt6Widgets_INCLUDE_DIRS})
    else ()
        set(QT_INC_DIRS ${Qt5Gui_INCLUDE_DIRS} ${Qt5Widgets_INCLUDE_DIRS})
    endif ()

    target_include_directories(soqt
        PUBLIC
        ${Coin_INCLUDE_DIR}
        ${SoQt_INCLUDE_DIRS}
        ${QT_INC_DIRS}
        ${Python_INCLUDE_DIRS}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/interfaces
        )

    target_link_libraries(soqt PUBLIC SoQt::SoQt Python::Module)
    install(TARGETS soqt DESTINATION ${PIVY_Python_SITEARCH}/pivy/gui)
endif()
