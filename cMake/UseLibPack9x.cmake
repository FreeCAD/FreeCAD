# ================================================================================
# == Win32 is default behaviour use the LibPack copied in Source tree ============

# --------------------------------------------------------------------------------
# General includes 

link_directories(${FREECAD_LIBPACK_DIR}/lib)
include_directories(${FREECAD_LIBPACK_DIR}/include)

# OpenGL
set(OPENGL_gl_LIBRARY opengl32 glu32)

# Python
set(PYTHON_LIBRARIES optimized python27.lib debug python27_d.lib)
set(PYTHON_INCLUDE_PATH ${FREECAD_LIBPACK_DIR}/include/Python-2.7.6)
set(PYTHON_INCLUDE_DIRS ${FREECAD_LIBPACK_DIR}/include/Python-2.7.6)
set(PYTHON_EXECUTABLE   ${FREECAD_LIBPACK_DIR}/bin/python.exe)
set(PYTHONLIBS_FOUND TRUE) 

# XercesC
set(XercesC_INCLUDE_DIRS ${FREECAD_LIBPACK_DIR}/include/xerces-c-3.1.0)
set(XercesC_LIBRARIES       xerces-c_3.lib)
set(XercesC_DEBUG_LIBRARIES xerces-c_3D.lib)
set(XercesC_FOUND TRUE) 

# Boost
set(Boost_INCLUDE_DIRS ${FREECAD_LIBPACK_DIR}/include/boost-1_54)
set(Boost_LIBRARIES 
    optimized boost_filesystem-vc90-mt-1_54.lib
    optimized boost_system-vc90-mt-1_54.lib 
    optimized boost_graph-vc90-mt-1_54.lib 
    optimized boost_program_options-vc90-mt-1_54.lib
    optimized boost_python-vc90-mt-1_54.lib
    optimized boost_regex-vc90-mt-1_54.lib
    optimized boost_signals-vc90-mt-1_54.lib
    optimized boost_thread-vc90-mt-1_54.lib
    debug boost_filesystem-vc90-mt-gd-1_54.lib
    debug boost_system-vc90-mt-gd-1_54.lib
    debug boost_graph-vc90-mt-gd-1_54.lib 
    debug boost_program_options-vc90-mt-gd-1_54.lib
    debug boost_python-vc90-mt-gd-1_54.lib
    debug boost_regex-vc90-mt-gd-1_54.lib
    debug boost_signals-vc90-mt-gd-1_54.lib
    debug boost_thread-vc90-mt-gd-1_54.lib
)
set(Boost_FOUND TRUE) 

# Zlib
set(ZLIB_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/zlib-1.2.3)
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
find_path(COIN3D_INCLUDE_DIRS Inventor/So.h
${FREECAD_LIBPACK_DIR}/include/Coin-4.0.0
)
find_library(COIN3D_LIBRARY_RELEASE coin4
    "${FREECAD_LIBPACK_DIR}/lib"
)
find_library(COIN3D_LIBRARY_DEBUG coin4d
    "${FREECAD_LIBPACK_DIR}/lib"
)
set(COIN3D_LIBRARIES optimized ${COIN3D_LIBRARY_RELEASE}
                     debug ${COIN3D_LIBRARY_DEBUG})

set(COIN3D_FOUND TRUE) 


# QT
set(QT_INCLUDE_DIR 
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/Qt
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtCore
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtGui
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtDesigner
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtSvg
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtNetwork
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtSql
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtTest
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtUiTools
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtXml
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtXmlPatterns
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtOpenGl
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtWebKit
)

set(QT_QTCORE_INCLUDE_DIR
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtCore
)

set(QT_QTXML_INCLUDE_DIR
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/QtXml
)

set(QT_QT3SUPPORT_INCLUDE_DIR
    ${FREECAD_LIBPACK_DIR}/include/Qt-4.8.5/Qt3Support
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
    optimized QtXmlPatterns4.lib
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
    debug QtXmlPatternsd4.lib
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

set(QT_QT3SUPPORT_LIBRARY 
    optimized Qt3Support4.lib
    debug Qt3Supportd4.lib
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
set(SOQT_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/SoQt-1.6.0)
set(SOQT_LIBRARY_RELEASE  soqt1.lib)
set(SOQT_LIBRARY_DEBUG  soqt1d.lib)
set(SOQT_FOUND TRUE) 

# OpenCV
set(OPENCV_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/opencv)
set(OPENCV_LIBRARIES  cv.lib cvaux.lib cxcore.lib cxts.lib highgui.lib)
set(OPENCV_FOUND TRUE) 

# NGLIB (NetGen)

set(NGLIB_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/nglib/include)
set(NGLIB_LIBRARY_DIR
    ${FREECAD_LIBPACK_DIR}/lib
)
set(NGLIB_LIBRARIES
    optimized nglib
)
set(NGLIB_DEBUG_LIBRARIES
    debug nglibd
)

# OCC
#set(OCC_INCLUDE_DIR C:/Projects/LibPack/oce-0.10.0/include/oce)
#set(OCC_LIBRARY_DIR C:/Projects/LibPack/oce-0.10.0/Win64/lib)
#set(OCC_LIBRARIES
#    ${OCC_LIBRARY_DIR}/TKFillet.lib
#    ${OCC_LIBRARY_DIR}/TKMesh.lib
#    ${OCC_LIBRARY_DIR}/TKernel.lib
#    ${OCC_LIBRARY_DIR}/TKG2d.lib
#    ${OCC_LIBRARY_DIR}/TKG3d.lib
#    ${OCC_LIBRARY_DIR}/TKMath.lib
#    ${OCC_LIBRARY_DIR}/TKIGES.lib
#    ${OCC_LIBRARY_DIR}/TKSTL.lib
#    ${OCC_LIBRARY_DIR}/TKShHealing.lib
#    ${OCC_LIBRARY_DIR}/TKXSBase.lib
#    ${OCC_LIBRARY_DIR}/TKBool.lib
#    ${OCC_LIBRARY_DIR}/TKBO.lib
#    ${OCC_LIBRARY_DIR}/TKBRep.lib
#    ${OCC_LIBRARY_DIR}/TKTopAlgo.lib
#    ${OCC_LIBRARY_DIR}/TKGeomAlgo.lib
#    ${OCC_LIBRARY_DIR}/TKGeomBase.lib
#    ${OCC_LIBRARY_DIR}/TKOffset.lib
#    ${OCC_LIBRARY_DIR}/TKPrim.lib
#    ${OCC_LIBRARY_DIR}/TKSTEP.lib
#    ${OCC_LIBRARY_DIR}/TKSTEPBase.lib
#    ${OCC_LIBRARY_DIR}/TKSTEPAttr.lib
#    ${OCC_LIBRARY_DIR}/TKHLR.lib
#    ${OCC_LIBRARY_DIR}/TKFeat.lib
#)
#set(OCC_OCAF_LIBRARIES
#    ${OCC_LIBRARY_DIR}/TKCAF.lib
#    ${OCC_LIBRARY_DIR}/TKXCAF.lib
#    ${OCC_LIBRARY_DIR}/TKLCAF.lib
#    ${OCC_LIBRARY_DIR}/TKXDESTEP.lib
#    ${OCC_LIBRARY_DIR}/TKXDEIGES.lib
#)
set(OCC_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/oce-0.13)
set(OCC_LIBRARY_DIR ${FREECAD_LIBPACK_DIR}/lib)
set(OCC_LIBRARIES
    optimized TKFillet
    optimized TKMesh
    optimized TKernel
    optimized TKG2d
    optimized TKG3d
    optimized TKMath
    optimized TKIGES
    optimized TKSTL
    optimized TKShHealing
    optimized TKXSBase
    optimized TKBin
    optimized TKBool
    optimized TKBO
    optimized TKBRep
    optimized TKTopAlgo
    optimized TKGeomAlgo
    optimized TKGeomBase
    optimized TKOffset
    optimized TKPrim
    optimized TKSTEP
    optimized TKSTEPBase
    optimized TKSTEPAttr
    optimized TKHLR
    optimized TKFeat
)
set(OCC_DEBUG_LIBRARIES
    debug TKFilletd
    debug TKMeshd
    debug TKerneld
    debug TKG2dd
    debug TKG3dd
    debug TKMathd
    debug TKIGESd
    debug TKSTLd
    debug TKShHealingd
    debug TKXSBased
    debug TKBind
    debug TKBoold
    debug TKBOd
    debug TKBRepd
    debug TKTopAlgod
    debug TKGeomAlgod
    debug TKGeomBased
    debug TKOffsetd
    debug TKPrimd
    debug TKSTEPd
    debug TKSTEPBased
    debug TKSTEPAttrd
    debug TKHLRd
    debug TKFeatd
)
set(OCC_OCAF_LIBRARIES
    optimized TKCAF
    optimized TKXCAF
    optimized TKLCAF
    optimized TKXDESTEP
    optimized TKXDEIGES
    optimized TKMeshVS
    optimized TKAdvTools
)
set(OCC_OCAF_DEBUG_LIBRARIES
    debug TKCAFd
    debug TKXCAFd
    debug TKLCAFd
    debug TKXDESTEPd
    debug TKXDEIGESd
    debug TKMeshVSd
    debug TKAdvToolsd
)
set(OCC_FOUND TRUE) 

set(EIGEN3_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/eigen3)
set(EIGEN3_FOUND TRUE)

# FreeType
if(FREECAD_USE_FREETYPE)
    set(FREETYPE_LIBRARIES 
        optimized ${FREECAD_LIBPACK_DIR}/lib/freetype.lib
        debug ${FREECAD_LIBPACK_DIR}/lib/freetyped.lib
    )
    set(FREETYPE_INCLUDE_DIRS
        ${FREECAD_LIBPACK_DIR}/include/FreeType-2.4.12
    )
    set(FREETYPE_VERSION_STRING
        "2.4.12"
    )
    set(FREETYPE_FOUND
        TRUE
    )
endif(FREECAD_USE_FREETYPE)


#  SHIBOKEN_INCLUDE_DIR        - Directories to include to use SHIBOKEN
#  SHIBOKEN_LIBRARY            - Files to link against to use SHIBOKEN
#  SHIBOKEN_BINARY             - Executable name

SET(SHIBOKEN_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/shiboken-1.2.1)
SET(SHIBOKEN_LIBRARY     optimized ${FREECAD_LIBPACK_DIR}/lib/shiboken-python2.7.lib debug ${FREECAD_LIBPACK_DIR}/lib/shiboken-python2.7_d.lib)
set(SHIBOKEN_BINARY      ${FREECAD_LIBPACK_DIR}/bin/shiboken)

#  PYSIDE_INCLUDE_DIR   - Directories to include to use PySide
#  PYSIDE_LIBRARY       - Files to link against to use PySide
#  PYSIDE_PYTHONPATH    - Path to where the PySide Python module files could be found
#  PYSIDE_TYPESYSTEMS   - Type system files that should be used by other bindings extending PySide

SET(PYSIDE_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/PySide-1.2.1)
SET(PYSIDE_LIBRARY     optimized ${FREECAD_LIBPACK_DIR}/lib/pyside-python2.7.lib debug ${FREECAD_LIBPACK_DIR}/lib/pyside-python2.7_d.lib)
SET(PYSIDE_PYTHONPATH  ${FREECAD_LIBPACK_DIR}/pyside/Lib/site-packages)
SET(PYSIDE_TYPESYSTEMS ${FREECAD_LIBPACK_DIR}/pyside/share/PySide/typesystems)
SET(PYSIDE_BIN_DIR     ${FREECAD_LIBPACK_DIR}/pyside-tools/bin)

# Pointscloud library
set(PCL_INCLUDE_DIRS ${FREECAD_LIBPACK_DIR}/include/pcl-1.6)
set(PCL_LIBRARY_DIRS ${FREECAD_LIBPACK_DIR}/lib)

set(PCL_COMMON_LIBRARIES optimized pcl_common_release debug pcl_common_debug)
set(PCL_FEATURES_LIBRARIES optimized pcl_features_release debug pcl_features_debug)
set(PCL_FILTERS_LIBRARIES optimized pcl_filters_release debug pcl_filters_debug)
set(PCL_IO_LIBRARIES optimized pcl_io_release debug pcl_io_debug)
set(PCL_IO_PLY_LIBRARIES optimized pcl_io_ply_release debug pcl_io_ply_debug)
set(PCL_KDTREE_LIBRARIES optimized pcl_kdtree_release debug pcl_kdtree_debug)
set(PCL_KEYPOINTS_LIBRARIES optimized pcl_keypoints_release debug pcl_keypoints_debug)
set(PCL_OCTREE_LIBRARIES optimized pcl_octree_release debug pcl_octree_debug)
set(PCL_REGISTRATION_LIBRARIES optimized pcl_registration_release debug pcl_registration_debug)
set(PCL_SAMPLE_CONSENSUS_LIBRARIES optimized pcl_sample_consensus_release debug pcl_sample_consensus_debug)
set(PCL_SEARCH_LIBRARIES optimized pcl_search_release debug pcl_search_debug)
set(PCL_SEGMENTATION_LIBRARIES optimized pcl_segmentation_release debug pcl_segmentation_debug)
set(PCL_SURFACE_LIBRARIES optimized pcl_surface_release debug pcl_surface_debug)
set(PCL_TRACKING_LIBRARIES optimized pcl_tracking_release debug pcl_tracking_debug)

set(PCL_LIBRARIES
    ${PCL_COMMON_LIBRARIES}
    ${PCL_FEATURES_LIBRARIES}
    ${PCL_FILTERS_LIBRARIES}
    ${PCL_IO_LIBRARIES}
    ${PCL_IO_PLY_LIBRARIES}
    ${PCL_KDTREE_LIBRARIES}
    ${PCL_KEYPOINTS_LIBRARIES}
    ${PCL_OCTREE_LIBRARIES}
    ${PCL_REGISTRATION_LIBRARIES}
    ${PCL_SAMPLE_CONSENSUS_LIBRARIES}
    ${PCL_SEARCH_LIBRARIES}
    ${PCL_SEGMENTATION_LIBRARIES}
    ${PCL_SURFACE_LIBRARIES}
    ${PCL_TRACKING_LIBRARIES}
)
set(PCL_FOUND TRUE)
set(PCL_COMMON_FOUND TRUE)
set(PCL_FEATURES_FOUND TRUE)
set(PCL_FILTERS_FOUND TRUE)
set(PCL_IO_FOUND TRUE)
set(PCL_IO_PLY_FOUND TRUE)
set(PCL_KDTREE_FOUND TRUE)
set(PCL_KEYPOINTS_FOUND TRUE)
set(PCL_OCTREE_FOUND TRUE)
set(PCL_REGISTRATION_FOUND TRUE)
set(PCL_SAMPLE_CONSENSUS_FOUND TRUE)
set(PCL_SEARCH_FOUND TRUE)
set(PCL_SEGMENTATION_FOUND TRUE)
set(PCL_SURFACE_FOUND TRUE)
set(PCL_TRACKING_FOUND TRUE)

set(FLANN_LIBRARIES optimized flann debug flann)
set(FLANN_INCLUDE_DIRS ${FREECAD_LIBPACK_DIR}/include/flann-1.7.1)
