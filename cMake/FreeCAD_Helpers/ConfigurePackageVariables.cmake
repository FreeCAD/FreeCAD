macro(ConfigurePackageVariables)
    # ================================================================================
    # == Native Packages =============================================================
    #

    if (WIN32)
      if (USE_WIX_TOOLSET)
        set(CPACK_GENERATOR "WIX") # this need WiX Tooset installed and a path to candle.exe
      else ()
        set(CPACK_GENERATOR "NSIS") # this needs NSIS installed, and available
      endif ()
      set(CPACK_SOURCE_GENERATOR "ZIP")
    elseif ( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
      set(CPACK_GENERATOR "PackageMake")
    else ()
      set(CPACK_GENERATOR "DEB")
      set(CPACK_SOURCE_GENERATOR "TGZ")
    endif ()


set(CPACK_PACKAGE_VERSION "${FREECAD_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PACKAGE_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PACKAGE_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PACKAGE_VERSION_PATCH}")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

set(CPACK_SOURCE_IGNORE_FILES "${CMAKE_SOURCE_DIR}/build/;${CMAKE_SOURCE_DIR}/.git/")

# Debian configs

if ($ENV{USERNAME})
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER $ENV{USERNAME})
else()
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER $ENV{USER})
endif()
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Extensible Open Source CAx program")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/package/debian/summary")
string(TIMESTAMP CPACK_DEBIAN_PACKAGE_RELEASE "%Y.%m.%d")
set(CPACK_DEBIAN_PACKAGE_SECTION "science")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "python${PYTHON_VERSION_MAJOR}") # to remove warning
set(CPACK_DEBIAN_PACKAGE_BREAKS "freecad")
set(CPACK_DEBIAN_DEBUGINFO_PACKAGE ON)

include(CPack)

endmacro(ConfigurePackageVariables)
