# SPDX-License-Identifier: LGPL-2.1-or-later

#
# Build and install QuickLook for FCStd files (org.freecad.fcstd)
# This is used by Homebrew and Conda/Mamba scripts alike.
#
if(APPLE AND BUILD_WITH_CONDA)
    add_subdirectory(QuickLook)
    # Installation handled by QuickLook/CMakeLists.txt
endif()