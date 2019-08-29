if(MSVC)
    add_definitions(-DHAVE_ACOSH -DHAVE_ASINH -DHAVE_ATANH)
else(MSVC)
    add_definitions(-DHAVE_LIMITS_H -DHAVE_CONFIG_H)
endif(MSVC)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/Mod/Assembly/App
    ${CMAKE_CURRENT_BINARY_DIR}
    ${Boost_INCLUDE_DIRS}
    ${COIN_INCLUDE_DIR}
    ${ZLIB_INCLUDE_DIR}
    ${OCC_INCLUDE_DIR}
    ${QT_INCLUDE_DIR}
    ${PYTHON_INCLUDE_DIRS}
    ${COIN3D_INCLUDE_DIRS}
    ${XercesC_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

link_directories(${OCC_LIBRARY_DIR})

set(AssemblyGui_LIBS
    #${ODE_LIBRARIES}
    Assembly
    PartGui
    FreeCADGui
)

set(AssemblyGui_MOC_HDRS
    #TaskAssemblyConstraints.h
    #TaskDlgAssemblyConstraints.h
)
fc_wrap_cpp(AssemblyGui_MOC_SRCS ${AssemblyGui_MOC_HDRS})
SOURCE_GROUP("Moc" FILES ${AssemblyGui_MOC_SRCS})

if (BUILD_QT5)
	qt5_add_resources(AssemblyGui_SRCS Resources/Assembly.qrc)
else()
	qt4_add_resources(AssemblyGui_SRCS Resources/Assembly.qrc)
endif()

set(AssemblyGui_UIC_SRCS
    #TaskAssemblyConstraints.ui
)
if (BUILD_QT5)
	qt5_wrap_ui(AssemblyGui_UIC_HDRS ${AssemblyGui_UIC_SRCS})
else()
	qt4_wrap_ui(AssemblyGui_UIC_HDRS ${AssemblyGui_UIC_SRCS})
endif()

SET(AssemblyGuiViewProvider_SRCS
    ViewProvider.cpp
    ViewProvider.h
    #ViewProviderPartRef.cpp
    #ViewProviderPartRef.h
    ViewProviderProduct.cpp
    ViewProviderProduct.h
    ViewProviderProductRef.cpp
    ViewProviderProductRef.h
    ViewProviderConstraint.cpp
    ViewProviderConstraint.h
    ViewProviderConstraintGroup.cpp
    ViewProviderConstraintGroup.h
    #TaskDlgAssemblyConstraints.cpp
    #TaskDlgAssemblyConstraints.h
    #TaskAssemblyConstraints.h
    #TaskAssemblyConstraints.cpp
)
SOURCE_GROUP("ViewProvider" FILES ${AssemblyGuiViewProvider_SRCS})

SET(AssemblyGuiModule_SRCS 
    AppAssemblyGui.cpp
    AppAssemblyGuiPy.cpp
    Command.cpp
    CommandConstraints.cpp
    Resources/Assembly.qrc
	#qrc_Assembly.cxx
    PreCompiled.cpp
    PreCompiled.h
    Workbench.cpp
    Workbench.h
)
SOURCE_GROUP("Module" FILES ${AssemblyGuiModule_SRCS})


SET(AssemblyGui_SRCS
    ${AssemblyGui_SRCS}
    ${AssemblyGuiViewProvider_SRCS}
    ${AssemblyGuiModule_SRCS}
    ${AssemblyGui_UIC_HDRS}
)

SET(AssemblyGui_Scripts
    ../InitGui.py
)

add_library(AssemblyGui SHARED ${AssemblyGui_SRCS} ${AssemblyGui_Scripts})
if(CMAKE_COMPILER_IS_GNUCXX)
	set_target_properties(AssemblyGui PROPERTIES COMPILE_FLAGS "-fext-numeric-literals")
endif()
target_link_libraries(AssemblyGui ${AssemblyGui_LIBS})


fc_target_copy_resource_flat(AssemblyGui
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_BINARY_DIR}/Mod/Assembly
    ${AssemblyGui_Scripts})

SET_BIN_DIR(AssemblyGui AssemblyGui /Mod/Assembly)
SET_PYTHON_PREFIX_SUFFIX(AssemblyGui)

INSTALL(TARGETS AssemblyGui DESTINATION ${CMAKE_INSTALL_LIBDIR})

SET(AssemblyGuiIcon_SVG
	Resources/icons/AssemblyWorkbench.svg
)

fc_copy_sources(AssemblyGui "${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_DATADIR}/Mod/Assembly" ${AssemblyGuiIcon_SVG})

INSTALL(FILES ${AssemblyGuiIcon_SVG} DESTINATION "${CMAKE_INSTALL_DATADIR}/Mod/Assembly/Resources/icons")
