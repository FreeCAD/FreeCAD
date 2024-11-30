macro(SetupLibFmt)

       # This internet check idea is borrowed from:
       # https://stackoverflow.com/questions/62214621/how-to-check-for-internet-connection-with-cmake-automatically-prevent-fails-if

       if(FREECAD_USE_EXTERNAL_FMT)
              find_package(fmt QUIET)
       endif()

       if(fmt_FOUND)
              message(STATUS "find_package() was used to locate fmt version ${fmt_VERSION}")
       else()

              message(STATUS "Checking for connection to GitHub...")
              if (WIN32)
                     set(ping_command "ping /n 1 /w 3 github.com")
              else()
                     set(ping_command "ping -c 1 -W 3 github.com")
              endif()
              execute_process(
                     COMMAND ${ping_command}
                     RESULT_VARIABLE NO_CONNECTION
              )
              if(NO_CONNECTION GREATER 0)
                     set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
                     message(WARNING "NO INTERNET CONNECTION: Using disconnected mode for FetchContent updates")
              else()
                     message(STATUS "GitHub connection established for FetchContent")
                     set(FETCHCONTENT_UPDATES_DISCONNECTED OFF)
              endif()

              include(FetchContent)
              if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.24.0")
                     cmake_policy(SET CMP0135 NEW)
              endif()
              FetchContent_Declare(fmt
                      URL https://github.com/fmtlib/fmt/archive/refs/tags/9.1.0.zip
                      URL_MD5 e6754011ff56bfc37631fcc90961e377
              )
              FetchContent_MakeAvailable(fmt)
              set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
              if(${fmt_POPULATED})
                     message(STATUS "fmt was downloaded using FetchContent into ${fmt_SOURCE_DIR}")
              else()
                     message(ERROR "Failed to install the fmt library")
              endif()
       endif()
endmacro()
