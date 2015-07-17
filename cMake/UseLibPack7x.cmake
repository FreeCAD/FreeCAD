# ================================================================================
# == Win32 is default behaviour use the LibPack copied in Source tree ============

# --------------------------------------------------------------------------------
# General includes 

link_directories(${FREECAD_LIBPACK_DIR}/lib)
include_directories(${FREECAD_LIBPACK_DIR}/include)

# OpenGL
set(OPENGL_gl_LIBRARY opengl32 glu32)

# Python
set(PYTHON_LIBRARIES optimized python26.lib debug python26_d.lib)
set(PYTHON_INCLUDE_PATH ${FREECAD_LIBPACK_DIR}/include/python)
set(PYTHON_EXECUTABLE   ${FREECAD_LIBPACK_DIR}/bin/python.exe)
set(PYTHONLIBS_FOUND TRUE) 
	
# XercesC
set(XERCESC_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/xercesc)
set(XERCESC_LIBRARIES       xerces-c_2.lib)
set(XERCESC_DEBUG_LIBRARIES xerces-c_2D.lib)
set(XERCESC_FOUND TRUE) 
	
# Boost
set(Boost_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/boost)
set(Boost_LIBRARIES 
	optimized boost_filesystem-vc90-mt-1_39.lib
    optimized boost_system-vc90-mt-1_39.lib 
	optimized boost_graph-vc90-mt-1_39.lib 
	optimized boost_program_options-vc90-mt-1_39.lib
	optimized boost_regex-vc90-mt-1_39.lib
	optimized boost_signals-vc90-mt-1_39.lib
	optimized boost_thread-vc90-mt-1_39.lib
	debug boost_filesystem-vc90-mt-gd-1_39.lib 
	debug boost_date_time-vc90-mt-gd-1_39.lib
	debug boost_filesystem-vc90-mt-gd-1_39.lib
	debug boost_iostreams-vc90-mt-gd-1_39.lib
	debug boost_math_c99f-vc90-mt-gd-1_39.lib
	debug boost_math_tr1f-vc90-mt-gd-1_39.lib
	debug boost_thread-vc90-mt-gd-1_39.lib
	debug boost_system-vc90-mt-gd-1_39.lib
	debug boost_graph-vc90-mt-gd-1_39.lib 
	debug boost_program_options-vc90-mt-gd-1_39.lib
	debug boost_regex-vc90-mt-gd-1_39.lib
	debug boost_signals-vc90-mt-gd-1_39.lib
)
set(Boost_FOUND TRUE) 

# Zlib
set(ZLIB_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/zlib)
set(ZLIB_LIBRARIES  zdll.lib)
set(ZLIB_FOUND TRUE) 

# SMESH
set(SMESH_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/smesh)
set(SMESH_LIBRARIES   
 StdMeshers.lib
 MEFISTO2.lib
 SMESH.lib
 DriverUNV.lib
 SMESHDS.lib
 DriverSTL.lib
 DriverDAT.lib
 Driver.lib
 SMDS.lib
 )

set(SMESH_FOUND TRUE) 
	
# Coin3D
set(COIN3D_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/coin)
set(COIN3D_LIBRARY_DEBUG  coin3d.lib)
set(COIN3D_LIBRARY_RELEASE  coin3.lib)
set(COIN3D_FOUND TRUE) 


# QT
set(QT_INCLUDE_DIR 
    ${FREECAD_LIBPACK_DIR}/include/QT/
    ${FREECAD_LIBPACK_DIR}/include/QT/Qt
    ${FREECAD_LIBPACK_DIR}/include/QT/QtCore
    ${FREECAD_LIBPACK_DIR}/include/QT/QtGui
    ${FREECAD_LIBPACK_DIR}/include/QT/QtDesigner
    ${FREECAD_LIBPACK_DIR}/include/QT/QtSvg
    ${FREECAD_LIBPACK_DIR}/include/QT/QtNetwork
    ${FREECAD_LIBPACK_DIR}/include/QT/QtSql
    ${FREECAD_LIBPACK_DIR}/include/QT/QtTest
    ${FREECAD_LIBPACK_DIR}/include/QT/QtUiTools
    ${FREECAD_LIBPACK_DIR}/include/QT/QtXml
    ${FREECAD_LIBPACK_DIR}/include/QT/QtOpenGl
    ${FREECAD_LIBPACK_DIR}/include/QT/QtWebKit
	)
	
set(QT_QTCORE_INCLUDE_DIR
    ${FREECAD_LIBPACK_DIR}/include/QT/
    ${FREECAD_LIBPACK_DIR}/include/QT/QtCore
    )
    
set(QT_LIBRARIES 
    optimized QtCore4.lib
    optimized QtGui4.lib
    optimized QtDesigner4.lib
    optimized QtSvg4.lib
    optimized QtNetwork4.lib
    optimized QtSql4.lib
    optimized QtTest4.lib
    optimized QtXml4.lib
    optimized QtOpenGl4.lib
    optimized QtWebKit4.lib
    debug QtCored4.lib
    debug QtGuid4.lib
    debug QtDesignerd4.lib
    debug QtSvgd4.lib
    debug QtNetworkd4.lib
    debug QtSqld4.lib
    debug QtTestd4.lib
    debug QtXmld4.lib
    debug QtOpenGld4.lib
    debug QtWebKitd4.lib
)

set(QT_QTCORE_LIBRARY 
    optimized QtCore4.lib
    debug QtCored4.lib
)

set(QT_QTXML_LIBRARY 
    optimized QtXml4.lib
    debug QtXmld4.lib
)

set(QT_QTUITOOLS_LIBRARY 
    optimized QtUiTools.lib
    debug QtUiToolsd.lib
)

set(QT_QTMAIN_LIBRARY 
    debug qtmaind.lib
    optimized qtmain.lib
)

set(QT_UIC_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/uic.exe)
set(QT_MOC_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/moc.exe)
set(QT_RCC_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/rcc.exe)
set(QT_HELPCOMPILER_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/qhelpgenerator.exe)
set(QT_COLLECTIOMGENERATOR_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/qcollectiongenerator.exe)



MACRO (QT4_EXTRACT_OPTIONS _qt4_files _qt4_options)
	SET(${_qt4_files})
	SET(${_qt4_options})
	#SET(_QT4_DOING_OPTIONS FALSE)
	FOREACH(_currentArg ${ARGN})
	#  IF ("${_currentArg}" STREQUAL "OPTIONS")
	#	SET(_QT4_DOING_OPTIONS TRUE)
	#  ELSE ("${_currentArg}" STREQUAL "OPTIONS")
	#	IF(_QT4_DOING_OPTIONS) 
	#	  LIST(APPEND ${_qt4_options} "${_currentArg}")
	#	ELSE(_QT4_DOING_OPTIONS)
		  LIST(APPEND ${_qt4_files} "${_currentArg}")
	#	ENDIF(_QT4_DOING_OPTIONS)
	#  ENDIF ("${_currentArg}" STREQUAL "OPTIONS")
	ENDFOREACH(_currentArg)  
ENDMACRO (QT4_EXTRACT_OPTIONS)
 
# macro used to create the names of output files preserving relative dirs
MACRO (QT4_MAKE_OUTPUT_FILE infile prefix ext outfile )
  STRING(LENGTH ${CMAKE_CURRENT_BINARY_DIR} _binlength)
  STRING(LENGTH ${infile} _infileLength)
  SET(_checkinfile ${CMAKE_CURRENT_SOURCE_DIR})
  IF(_infileLength GREATER _binlength)
    STRING(SUBSTRING "${infile}" 0 ${_binlength} _checkinfile)
    IF(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
      FILE(RELATIVE_PATH rel ${CMAKE_CURRENT_BINARY_DIR} ${infile})
    ELSE(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
      FILE(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
    ENDIF(_checkinfile STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  ELSE(_infileLength GREATER _binlength)
    FILE(RELATIVE_PATH rel ${CMAKE_CURRENT_SOURCE_DIR} ${infile})
  ENDIF(_infileLength GREATER _binlength)
  SET(_outfile "${CMAKE_CURRENT_BINARY_DIR}/${rel}")
  STRING(REPLACE ".." "__" _outfile ${_outfile})
  GET_FILENAME_COMPONENT(outpath ${_outfile} PATH)
  GET_FILENAME_COMPONENT(_outfile ${_outfile} NAME_WE)
  FILE(MAKE_DIRECTORY ${outpath})
  SET(${outfile} ${outpath}/${prefix}${_outfile}.${ext})
ENDMACRO (QT4_MAKE_OUTPUT_FILE )

MACRO (QT4_WRAP_CPP outfiles )
	QT4_EXTRACT_OPTIONS(moc_files moc_options ${ARGN})
	SET(ARGN)
	foreach(it ${moc_files})
		get_filename_component(it ${it} ABSOLUTE)
		QT4_MAKE_OUTPUT_FILE(${it} moc_ cpp outfile)
		ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
			COMMAND ${QT_MOC_EXECUTABLE}
			ARGS ${moc_options} ${it} -o ${outfile}
			MAIN_DEPENDENCY ${it}
		)
		SET(${outfiles} ${${outfiles}} ${outfile})
	endforeach(it)
ENDMACRO (QT4_WRAP_CPP)


# This is a special version of the built in macro qt4_wrap_cpp
# It is required since moc'ed files are now included instead of being added to projects directly
# It adds a reverse dependency to solve this
# This has the unfortunate side effect that some files are always rebuilt
# There is probably a cleaner solution than this

include(AddFileDependencies)

macro(fc_wrap_cpp outfiles )
	QT4_EXTRACT_OPTIONS(moc_files moc_options ${ARGN})
	# fixes bug 0000585: bug with boost 1.48
	SET(moc_options ${moc_options} -DBOOST_TT_HAS_OPERATOR_HPP_INCLUDED)
	SET(ARGN)
	foreach(it ${moc_files})
		get_filename_component(it ${it} ABSOLUTE)
		QT4_MAKE_OUTPUT_FILE(${it} moc_ cpp outfile)
		ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
			COMMAND ${QT_MOC_EXECUTABLE}
			ARGS ${moc_options} ${it} -o ${outfile}
			MAIN_DEPENDENCY ${it}
		)
		SET(${outfiles} ${${outfiles}} ${outfile})
		add_file_dependencies(${it} ${outfile})
	endforeach(it)
endmacro(fc_wrap_cpp)


MACRO (QT4_ADD_RESOURCES outfiles )
	QT4_EXTRACT_OPTIONS(rcc_files rcc_options ${ARGN})
	SET(ARGN)
	FOREACH (it ${rcc_files})
	  GET_FILENAME_COMPONENT(outfilename ${it} NAME_WE)
	  GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
	  GET_FILENAME_COMPONENT(rc_path ${infile} PATH)
	  SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/qrc_${outfilename}.cxx)
	  #  parse file for dependencies 
	  #  all files are absolute paths or relative to the location of the qrc file
	  FILE(READ "${infile}" _RC_FILE_CONTENTS)
	  STRING(REGEX MATCHALL "<file[^<]+" _RC_FILES "${_RC_FILE_CONTENTS}")
	  SET(_RC_DEPENDS)
	  FOREACH(_RC_FILE ${_RC_FILES})
		STRING(REGEX REPLACE "^<file[^>]*>" "" _RC_FILE "${_RC_FILE}")
		STRING(REGEX MATCH "^/|([A-Za-z]:/)" _ABS_PATH_INDICATOR "${_RC_FILE}")
		IF(NOT _ABS_PATH_INDICATOR)
		  SET(_RC_FILE "${rc_path}/${_RC_FILE}")
		ENDIF(NOT _ABS_PATH_INDICATOR)
		SET(_RC_DEPENDS ${_RC_DEPENDS} "${_RC_FILE}")
	  ENDFOREACH(_RC_FILE)
	  ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
		COMMAND ${QT_RCC_EXECUTABLE}
		ARGS ${rcc_options} -name ${outfilename} -o ${outfile} ${infile}
		MAIN_DEPENDENCY ${infile}
		DEPENDS ${_RC_DEPENDS})
	  SET(${outfiles} ${${outfiles}} ${outfile})
	ENDFOREACH (it)
ENDMACRO (QT4_ADD_RESOURCES)

MACRO (QT4_WRAP_UI outfiles  )
QT4_EXTRACT_OPTIONS(ui_files ui_options ${ARGN})

FOREACH (it ${ui_files})
	  GET_FILENAME_COMPONENT(outfile ${it} NAME_WE)
	  GET_FILENAME_COMPONENT(infile ${it} ABSOLUTE)
	  SET(outfile ${CMAKE_CURRENT_BINARY_DIR}/ui_${outfile}.h)
	  ADD_CUSTOM_COMMAND(OUTPUT ${outfile}
		COMMAND ${QT_UIC_EXECUTABLE}
		ARGS -o ${outfile} ${infile}
		MAIN_DEPENDENCY ${infile})
	  SET(${outfiles} ${${outfiles}} ${outfile})
	ENDFOREACH (it)
ENDMACRO (QT4_WRAP_UI)


set(QT4_FOUND TRUE) 

# SoQt
set(SOQT_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/soqt)
set(SOQT_LIBRARY_RELEASE  soqt1.lib)
set(SOQT_LIBRARY_DEBUG  soqt1d.lib)
set(SOQT_FOUND TRUE) 

# OpenCV
set(OPENCV_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/opencv)
set(OPENCV_LIBRARIES  cv.lib cvaux.lib cxcore.lib cxts.lib highgui.lib)
set(OPENCV_FOUND TRUE) 

# OCC
set(OCC_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/OpenCascade)
set(OCC_LIBRARY_DIR
    ${FREECAD_LIBPACK_DIR}/lib
)
set(OCC_LIBRARIES
    TKFillet
    TKMesh
    TKernel
    TKG2d
    TKG3d
    TKMath
    TKIGES
    TKSTL
    TKShHealing
    TKXSBase
    TKBool
    TKBO
    TKBRep
    TKTopAlgo
    TKGeomAlgo
    TKGeomBase
    TKOffset
    TKPrim
    TKSTEP
    TKSTEPBase
    TKSTEPAttr
    TKHLR
    TKFeat
)
set(OCC_OCAF_LIBRARIES
    TKCAF
    TKXCAF
    TKLCAF
    TKXDESTEP
    TKXDEIGES
)
set(OCC_FOUND TRUE) 

SET(EIGEN2_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/eigen2)
set(EIGEN2_FOUND TRUE)

SET(EIGEN3_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/eigen3)
set(EIGEN3_FOUND TRUE)




