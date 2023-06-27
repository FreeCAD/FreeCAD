macro(SetupSalomeSMESH)
# -------------------------------- Salome SMESH --------------------------

    # Salome SMESH sources are under src/3rdParty now
    if(BUILD_SMESH)
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
            find_package(VTK COMPONENTS vtkCommonCore REQUIRED NO_MODULE)
            if(${VTK_MAJOR_VERSION} LESS 9)
                list(APPEND VTK_COMPONENTS vtkIOMPIParallel vtkParallelMPI vtkhdf5 vtkFiltersParallelDIY2 vtkRenderingCore vtkInteractionStyle vtkRenderingFreeType vtkRenderingOpenGL2)
                foreach(_module ${VTK_COMPONENTS})
                    list (FIND VTK_MODULES_ENABLED ${_module} _index)
                    if(${_index} GREATER -1)
                        list(APPEND AVAILABLE_VTK_COMPONENTS ${_module})
                    endif()
                endforeach()
            else()
                set(VTK_COMPONENTS "CommonCore;CommonDataModel;FiltersVerdict;IOXML;FiltersCore;FiltersGeneral;IOLegacy;FiltersExtraction;FiltersSources;FiltersGeometry")
                list(APPEND VTK_COMPONENTS "IOMPIParallel;ParallelMPI;hdf5;FiltersParallelDIY2;RenderingCore;InteractionStyle;RenderingFreeType;RenderingOpenGL2")
                foreach(_module ${VTK_COMPONENTS})
                    list (FIND VTK_AVAILABLE_COMPONENTS ${_module} _index)
                    if(${_index} GREATER -1)
                        list(APPEND AVAILABLE_VTK_COMPONENTS ${_module})
                    endif()
                endforeach()
            endif()
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
            # See https://www.hdfgroup.org/HDF5/release/cmakebuild.html
            if (WIN32)
                find_package(HDF5 COMPONENTS NO_MODULE REQUIRED static)
            else()
                find_package(PkgConfig)
                file(READ ${meddotH} TMPTXT)
                string(FIND "${TMPTXT}" "#define MED_HAVE_MPI" matchres)
                if(${matchres} EQUAL -1)
                    message(STATUS "We guess that libmed was built using hdf5-serial version")
                    set(HDF5_VARIANT "hdf5-serial")
                else()
                    message(STATUS "We guess that libmed was built using hdf5-openmpi version")
                    set(HDF5_VARIANT "hdf5-openmpi")
                    set(HDF5_PREFER_PARALLEL TRUE) # if pkg-config fails, find_package(HDF5) needs this
                endif()
                pkg_search_module(HDF5 ${HDF5_VARIANT})
                if(NOT HDF5_FOUND)
                    find_package(HDF5 REQUIRED)
                else()
                    add_compile_options(${HDF5_CFLAGS})
                    link_directories(${HDF5_LIBRARY_DIRS})
                    link_libraries(${HDF5_LIBRARIES})
                    find_file(Hdf5dotH hdf5.h PATHS ${HDF5_INCLUDE_DIRS} NO_DEFAULT_PATH)
                    if(NOT Hdf5dotH)
                        message( FATAL_ERROR "${HDF5_VARIANT} development header not found.")
                    endif()
                endif()
                check_include_file_cxx(hdf5.h HDF5_FOUND)
                if(NOT HDF5_FOUND)
                    message( FATAL_ERROR "hdf5.h was not found.")
                endif()

                # Med Fichier can require MPI
                pkg_search_module(OPENMPI ompi-cxx)
                add_compile_options(${OPENMPI_CFLAGS})
                link_directories(${OPENMPI_LIBRARY_DIRS})
                link_libraries(${OPENMPI_LIBRARIES})
                if(NOT OPENMPI_FOUND)
                    message( WARNING "ompi-cxx was not found. Check for error above.")
                endif()
            endif()
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
    endif(BUILD_SMESH)

endmacro(SetupSalomeSMESH)
