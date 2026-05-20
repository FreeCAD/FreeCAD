macro(SetupSalomeSMESH)
# -------------------------------- Salome SMESH --------------------------
    # Salome SMESH sources are under src/3rdParty now
    if(FREECAD_USE_SMESH)

        # set the internal smesh version:
        # see src/3rdParty/salomonemesh/CMakeLists.txt and commit https://github.com/FreeCAD/FreeCAD/commit/666a3e5 and https://forum.freecad.org/viewtopic.php?f=10&t=30838
        set(SMESH_VERSION_MAJOR 7)
        set(SMESH_VERSION_MINOR 7)
        set(SMESH_VERSION_PATCH 1)
        set(SMESH_VERSION_TWEAK 0)

        #if we use smesh we definitely also need vtk, no matter of external or internal smesh
        set (VTK_COMPONENTS
            vtkCommonCore
            vtkCommonDataModel
            vtkFiltersVerdict
            vtkIOXML
            vtkFiltersCore
            vtkFiltersGeneral
            vtkIOLegacy
            vtkFiltersExtraction
            vtkFiltersSources
            vtkFiltersGeometry
        )

        # check which modules are available
        if(UNIX OR WIN32)
            # Module names changed between 8 and 9, so do a QUIET find for 9 and its module name first, and fall back
            # to v7 minimum with the old component name if it is not found.
            find_package(VTK 9 COMPONENTS CommonCore QUIET NO_MODULE)
            if(NOT VTK_FOUND)
                message(STATUS "Did not find VTK 9, trying for an older version")
                find_package(VTK COMPONENTS vtkCommonCore REQUIRED NO_MODULE)
            endif()
            if(${VTK_MAJOR_VERSION} LESS 9)
                list(APPEND VTK_COMPONENTS vtkIOMPIParallel vtkParallelMPI vtkhdf5 vtkFiltersParallelDIY2 vtkRenderingCore vtkInteractionStyle vtkRenderingFreeType vtkRenderingOpenGL2)
                foreach(_module ${VTK_COMPONENTS})
                    list (FIND VTK_MODULES_ENABLED ${_module} _index)
                    if(${_index} GREATER -1)
                        list(APPEND AVAILABLE_VTK_COMPONENTS ${_module})
                    endif()
                endforeach()
            else()
                set(VTK_COMPONENTS "CommonCore;CommonDataModel;FiltersVerdict;IOXML;FiltersCore;FiltersGeneral;IOLegacy;FiltersExtraction;FiltersSources;FiltersGeometry;WrappingPythonCore")
                list(APPEND VTK_COMPONENTS "IOMPIParallel;ParallelMPI;hdf5;FiltersParallelDIY2;RenderingCore;InteractionStyle;RenderingFreeType;RenderingOpenGL2")
                foreach(_module ${VTK_COMPONENTS})
                    list (FIND VTK_AVAILABLE_COMPONENTS ${_module} _index)
                    if(${_index} GREATER -1)
                        list(APPEND AVAILABLE_VTK_COMPONENTS ${_module})
                    endif()
                endforeach()
            endif()
            # VTK's HDF5 component was requested, it might have imported HDF5 and added
            # _FORTIFY_SOURCE defines to the build.
            hdf5_clean_fortify_source()
        endif()

        # don't check VERSION 6 as this would exclude VERSION 7
        if(AVAILABLE_VTK_COMPONENTS)
            message(STATUS "VTK components: ${AVAILABLE_VTK_COMPONENTS}")
            find_package(VTK COMPONENTS ${AVAILABLE_VTK_COMPONENTS} REQUIRED NO_MODULE)
        else()
            message(STATUS "VTK components: not found or used")
            find_package(VTK REQUIRED NO_MODULE)
        endif()

        set(BUILD_FEM_VTK ON)

        # Check if PythonWrapperCore was found
        # Note: VTK 9 only, as the implementations use the VTK modules introduced in 8.1
        #       VTK_WrappingPythonCore_FOUND is named differently for versions <9.0
        if (${VTK_WrappingPythonCore_FOUND})
            set(BUILD_FEM_VTK_PYTHON 1)
            message(STATUS "VTK python wrapper: available")
        else()
            message(STATUS "VTK python wrapper: NOT available")
        endif()

        if(${VTK_MAJOR_VERSION} LESS 6)
            message( FATAL_ERROR "Found VTK version is <6, this is not compatible" )
        endif()
        if(${VTK_MAJOR_VERSION} EQUAL 6)
            if(${VTK_MINOR_VERSION} LESS 2)
                set(VTK_OPTIONS -DVTK_NO_QUAD_POLY)
            endif()
            if(${VTK_MINOR_VERSION} EQUAL 0)
                message(WARNING "VTK equal to 6.0 cannot be used with c++11, FEM postprocessing is disabled")
                set(BUILD_FEM_VTK OFF)
            endif()
        endif()
        # on openSUSE 13.1 VTK_LIBRARIES ends with "optimized" keyword
        list(REMOVE_ITEM VTK_LIBRARIES "optimized" "debug")

        if(NOT FREECAD_USE_EXTERNAL_SMESH)
            find_package(MEDFile REQUIRED)

            set(SMESH_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/src/3rdParty/salomesmesh/inc)

        else(NOT FREECAD_USE_EXTERNAL_SMESH)
            find_package(SMESH CONFIG)
            # If this definition is not set, linker errors will occur against SMESH on 64 bit machines.
            if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                add_definitions(-DSALOME_USE_64BIT_IDS)
            endif(CMAKE_SIZEOF_VOID_P EQUAL 8)
            if(NOT SMESH_FOUND)
                find_package(SMESH REQUIRED)
                if(NOT SMESH_FOUND)
                    message(ERROR "================\n"
                                "SMESH not found.\n"
                                "================\n")
                endif()
            endif()
            set (SMESH_INCLUDE_DIR ${SMESH_INCLUDE_PATH})
            set(EXTERNAL_SMESH_LIBS ${SMESH_LIBRARIES})

            include_directories(${SMESH_INCLUDE_DIR})
        endif()

        set(SMESH_FOUND TRUE)
        configure_file(${CMAKE_SOURCE_DIR}/src/SMESH_Version.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/SMESH_Version.h)
    endif(FREECAD_USE_SMESH)

endmacro(SetupSalomeSMESH)
