macro(SetupLibFmt)

       # This internet check idea is borrowed from:
       # https://stackoverflow.com/questions/62214621/how-to-check-for-internet-connection-with-cmake-automatically-prevent-fails-if

       message(STATUS "Checking for connection to GitHub...")
       if (WIN32)
              set(ping_command "ping /n 1 /w 1000 github.com")
       else()
              set(ping_command "ping -c 1 -W 1000 github.com")
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
       FetchContent_Declare(fmt
              GIT_REPOSITORY https://github.com/fmtlib/fmt
              GIT_TAG 9.1.0
       )
       FetchContent_MakeAvailable(fmt)
       set_target_properties(fmt PROPERTIES POSITION_INDEPENDENT_CODE ON)
       if (${fmt_FOUND})
              message(STATUS "find_package() was used to locate the fmt version ${fmt_VERSION}")
       elseif(${fmt_POPULATED})
              message(STATUS "fmt was downloaded using FetchContent into ${fmt_SOURCE_DIR}")
       else()
              message(ERROR "Failed to install the fmt library")
       endif()

endmacro()