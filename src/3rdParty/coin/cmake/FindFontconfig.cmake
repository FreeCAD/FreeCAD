# - Try to find the  Fontconfig
# Once done this will define
#
#  FONTCONFIG_FOUND - system has Fontconfig
#  FONTCONFIG_INCLUDE_DIR - The include directory to use for the fontconfig headers
#  FONTCONFIG_LIBRARIES - Link these to use FONTCONFIG
#  FONTCONFIG_DEFINITIONS - Compiler switches required for using FONTCONFIG

# Copyright (c) 2006,2007 Laurent Montel, <montel@kde.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. The name of the author may not be used to endorse or promote products 
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

if (FONTCONFIG_LIBRARIES AND FONTCONFIG_INCLUDE_DIR)

  # in cache already
  set(FONTCONFIG_FOUND TRUE)

else (FONTCONFIG_LIBRARIES AND FONTCONFIG_INCLUDE_DIR)

  if (NOT WIN32)
    # use pkg-config to get the directories and then use these values
    # in the FIND_PATH() and FIND_LIBRARY() calls
    find_package(PkgConfig)
    pkg_check_modules(PC_FONTCONFIG fontconfig)

    set(FONTCONFIG_DEFINITIONS ${PC_FONTCONFIG_CFLAGS_OTHER})
  endif (NOT WIN32)

  find_path(FONTCONFIG_INCLUDE_DIR fontconfig/fontconfig.h
    PATHS
    ${PC_FONTCONFIG_INCLUDEDIR}
    ${PC_FONTCONFIG_INCLUDE_DIRS}
    /usr/X11/include
  )

  find_library(FONTCONFIG_LIBRARIES NAMES fontconfig
    PATHS
    ${PC_FONTCONFIG_LIBDIR}
    ${PC_FONTCONFIG_LIBRARY_DIRS}
  )

  include(FindPackageHandleStandardArgs)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(Fontconfig DEFAULT_MSG FONTCONFIG_LIBRARIES FONTCONFIG_INCLUDE_DIR )
  
  mark_as_advanced(FONTCONFIG_LIBRARIES FONTCONFIG_INCLUDE_DIR)

endif (FONTCONFIG_LIBRARIES AND FONTCONFIG_INCLUDE_DIR)
