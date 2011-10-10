add_definitions(-DFCAppImport -DFC_DEBUG)

include_directories(
#		 ${CMAKE_SOURCE_DIR}/src
#		 ${OPENCV_INCLUDE_DIR}
		 ${OCC_INCLUDE_DIR}
#		 ${PYTHON_INCLUDE_PATH}
 		 ${ZLIB_INCLUDE_DIR}
		 ${XERCES_INCLUDE_DIR})

if(WIN32)
		 set(Import_LIBS
#		 		 ${OPENCV_LIBRARIES}
#		 		 -lTKIGES \
#		 		 -lTKSTEP \
		 		 Part
		 		 FreeCADApp)
else(WIN32)
		 set(Import_LIBS
#		 		 -lTKIGES \
#		 		 -lTKSTEP \
#		 		 -lFreeCADBase \
#		 		 -lFreeCADApp \
#		 		 -lPart
)
endif(WIN32)

set(Import_SRCS
		 AppImport.cpp
		 AppImportPy.cpp
		 FeatureImportIges.cpp
		 FeatureImportIges.h
		 FeatureImportStep.cpp
		 FeatureImportStep.h
		 PreCompiled.cpp
		 PreCompiled.h
)

add_library(Import SHARED ${Import_SRCS})

target_link_libraries(Import ${Import_LIBS})

set_target_properties(Import PROPERTIES SUFFIX ".pyd")

install(TARGETS Import DESTINATION lib)
