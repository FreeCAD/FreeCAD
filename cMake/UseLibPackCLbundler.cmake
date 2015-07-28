set(ENV{PATH} "${FREECAD_LIBPACK_DIR};$ENV{PATH}")
set(ENV{CMAKE_PREFIX_PATH} ${FREECAD_LIBPACK_DIR})

set(Boost_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include CACHE PATH "" FORCE)

set(OCE_DIR ${FREECAD_LIBPACK_DIR}/lib/cmake CACHE PATH "" FORCE)

set(SWIG_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/swig/swig.exe CACHE FILEPATH "Swig" FORCE)

set(PYTHON_EXECUTABLE ${FREECAD_LIBPACK_DIR}/bin/python.exe CACHE FILEPATH "" FORCE)
set(PYTHON_LIBRARY ${FREECAD_LIBPACK_DIR}/lib/python27.lib CACHE FILEPATH "" FORCE)
set(PYTHON_DEBUG_LIBRARY ${FREECAD_LIBPACK_DIR}/lib/python27_d.lib CACHE FILEPATH "" FORCE)
set(PYTHON_INCLUDE_DIR ${FREECAD_LIBPACK_DIR}/include/python2.7 CACHE PATH "" FORCE)

find_library(XercesC_LIBRARIES xerces-c_3 "${FREECAD_LIBPACK_DIR}/lib")
find_library(XercesC_DEBUG_LIBRARIES xerces-c_3D "${FREECAD_LIBPACK_DIR}/lib")
set(XercesC_FOUND TRUE) 

find_library(COIN3D_LIBRARY_RELEASE coin4 "${FREECAD_LIBPACK_DIR}/lib")
find_library(COIN3D_LIBRARY_DEBUG coin4d "${FREECAD_LIBPACK_DIR}/lib")
set(COIN3D_LIBRARIES optimized ${COIN3D_LIBRARY_RELEASE}
                     debug ${COIN3D_LIBRARY_DEBUG})
set(COIN3D_FOUND TRUE)

set(NETGENDATA ${FREECAD_LIBPACK_DIR}/include/netgen)

if(FREECAD_USE_FREETYPE)
    set(FREETYPE_INCLUDE_DIR_freetype2 ${FREECAD_LIBPACK_DIR}/include/freetype2)
endif(FREECAD_USE_FREETYPE)

link_directories(${FREECAD_LIBPACK_DIR}/lib)
set(PCL_INCLUDE_DIRS ${FREECAD_LIBPACK_DIR}/include/pcl-1.7)
set(PCL_LIBRARY_DIRS ${FREECAD_LIBPACK_DIR}/lib)

set(PCL_COMMON_LIBRARIES optimized pcl_common_release debug pcl_common_debug)
set(PCL_FEATURES_LIBRARIES optimized pcl_features_release debug pcl_features_debug)
set(PCL_FILTERS_LIBRARIES optimized pcl_filters_release debug pcl_filters_debug)
set(PCL_IO_LIBRARIES optimized pcl_io_release debug pcl_io_debug)
set(PCL_IO_PLY_LIBRARIES optimized pcl_io_ply_release debug pcl_io_ply_debug)
set(PCL_KDTREE_LIBRARIES optimized pcl_kdtree_release debug pcl_kdtree_debug)
set(PCL_KEYPOINTS_LIBRARIES optimized pcl_keypoints_release debug pcl_keypoints_debug)
set(PCL_ML_LIBRARIES optimized pcl_ml_release debug pcl_ml_debug)
set(PCL_OCTREE_LIBRARIES optimized pcl_octree_release debug pcl_octree_debug)
set(PCL_RECOGNITION_LIBRARIES optimized pcl_recognition_release debug pcl_recognition_debug)
set(PCL_REGISTRATION_LIBRARIES optimized pcl_registration_release debug pcl_registration_debug)
set(PCL_SAMPLE_CONSENSUS_LIBRARIES optimized pcl_sample_consensus_release debug pcl_sample_consensus_debug)
set(PCL_SEARCH_LIBRARIES optimized pcl_search_release debug pcl_search_debug)
set(PCL_SEGMENTATION_LIBRARIES optimized pcl_segmentation_release debug pcl_segmentation_debug)
set(PCL_STEREO_LIBRARIES optimized pcl_stereo_release debug pcl_stereo_debug)
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
    ${PCL_ML_LIBRARIES}
    ${PCL_OCTREE_LIBRARIES}
    ${PCL_RECOGNITION_LIBRARIES}
    ${PCL_REGISTRATION_LIBRARIES}
    ${PCL_SAMPLE_CONSENSUS_LIBRARIES}
    ${PCL_SEARCH_LIBRARIES}
    ${PCL_SEGMENTATION_LIBRARIES}
    ${PCL_STEREO_LIBRARIES}
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
set(PCL_ML_FOUND TRUE)
set(PCL_OCTREE_FOUND TRUE)
set(PCL_RECOGNITION_FOUND TRUE)
set(PCL_REGISTRATION_FOUND TRUE)
set(PCL_SAMPLE_CONSENSUS_FOUND TRUE)
set(PCL_SEARCH_FOUND TRUE)
set(PCL_SEGMENTATION_FOUND TRUE)
set(PCL_STEREO_FOUND TRUE)
set(PCL_SURFACE_FOUND TRUE)
set(PCL_TRACKING_FOUND TRUE)
