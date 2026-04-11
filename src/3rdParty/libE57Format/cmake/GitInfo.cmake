# ---------------------------------------------------------------------------- #
#
# Copyright (c) 2020 C++ Modern Framework
#
# https://github.com/cppmf/GitInfo.cmake
#
# ---------------------------------------------------------------------------- #

# Modified June 2024 - Andy Maloney <asmaloney@gmail.com>
#       - remove some vars we aren't using
#       - fix spelling/grammar

# ---------------------------------------------------------------------------- #
#
# Following variables will be set when calling GitInfo
#
#   GIT_DIR: path to the project .git directory
#   GIT_IS_DIRTY: whether or not the working tree is dirty
#   GIT_HEAD_BRANCH : name of the branch associated to HEAD
#   GIT_REVISION_HASH: current HEAD sha hash
#   GIT_REVISION: short version of GIT_REVISION_HASH
#   GIT_REVISION_NAME: name associated to GIT_REVISION_HASH
#   GIT_REMOTE_ORIGIN_URL : origin remote url
#   GIT_LATEST_TAG_LONG : most recent tag of the current branch
#   GIT_LATEST_TAG : most recent tagname of the current branch
#
# ---------------------------------------------------------------------------- #

# This is the main function to call in project CMakeLists.txt
# source should point to the root project directory
function(GitInfo source)

  # Check is source is a valid path
  if(NOT EXISTS ${source})
    message(FATAL_ERROR "'${source}' is not a valid path")
  endif()

  # Define the possible location of the .git directory
  set(GIT_DIR "${source}/.git")

  # Check if .git folder exist
  if(EXISTS ${GIT_DIR})

    #
    set(GIT_DIR "${GIT_DIR}" CACHE PATH "Project .git directory")

    # Check if git is installed
    if(NOT GIT_FOUND)
      find_package(Git QUIET)
    endif()
    if(NOT GIT_FOUND)
      message(AUTHOR_WARNING "Git not found, cannot get git information")
      return()
    endif()

    # name of the branch associated to HEAD
    execute_process(COMMAND ${GIT_EXECUTABLE} rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${source}
            OUTPUT_VARIABLE GIT_HEAD_BRANCH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(GIT_HEAD_BRANCH "${GIT_HEAD_BRANCH}" CACHE INTERNAL "name of the branch associated to HEAD")

    # git revision full hash
    execute_process(COMMAND ${GIT_EXECUTABLE} show -s "--format=%H" HEAD
            WORKING_DIRECTORY ${source}
            OUTPUT_VARIABLE GIT_REVISION_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(GIT_REVISION_HASH "${GIT_REVISION_HASH}" CACHE INTERNAL "git revision full hash")

    # short version of git revision
    execute_process(COMMAND ${GIT_EXECUTABLE} show -s "--format=%h" HEAD
            WORKING_DIRECTORY ${source}
            OUTPUT_VARIABLE GIT_REVISION OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(GIT_REVISION "${GIT_REVISION}" CACHE INTERNAL "short version of git revision")

    # short version of git revision name
    execute_process(COMMAND ${GIT_EXECUTABLE} show -s "--format=%s" HEAD
            WORKING_DIRECTORY ${source}
            OUTPUT_VARIABLE GIT_REVISION_NAME OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(GIT_REVISION_NAME "${GIT_REVISION_NAME}" CACHE INTERNAL "short version of git revision name")

    # origin remote url
    execute_process(COMMAND ${GIT_EXECUTABLE} config --get remote.origin.url
            WORKING_DIRECTORY ${source}
            OUTPUT_VARIABLE GIT_REMOTE_ORIGIN_URL OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(GIT_REMOTE_ORIGIN_URL "${GIT_REMOTE_ORIGIN_URL}" CACHE INTERNAL "git origin remote url")

    # most recent tag of the current branch
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0 HEAD
            WORKING_DIRECTORY ${source}
            OUTPUT_VARIABLE GIT_LATEST_TAG_LONG OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(GIT_LATEST_TAG_LONG "${GIT_LATEST_TAG_LONG}" CACHE INTERNAL "git most recent tag of the current branch")

    # most recent tagname of the current branch
    execute_process(COMMAND ${GIT_EXECUTABLE} describe --tags HEAD
            WORKING_DIRECTORY ${source}
            OUTPUT_VARIABLE GIT_LATEST_TAG OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(GIT_LATEST_TAG "${GIT_LATEST_TAG}" CACHE INTERNAL "git most recent tagname of the current branch")

  endif()

endfunction()


# Report git information
function(GitInfoReport)
  message(STATUS "")
  message(STATUS "----------------------------------------------------")
  message(STATUS "                 GitInfo.cmake")
  message(STATUS "")
  message(STATUS "GIT_DIR : ${GIT_DIR}")
  message(STATUS "")
  message(STATUS "GIT_HEAD_BRANCH : ${GIT_HEAD_BRANCH}")
  message(STATUS "GIT_REVISION : ${GIT_REVISION}")
  message(STATUS "GIT_REVISION_HASH : ${GIT_REVISION_HASH}")
  message(STATUS "GIT_REVISION_NAME : ${GIT_REVISION_NAME}")
  message(STATUS "")
  message(STATUS "GIT_REMOTE_ORIGIN_URL : ${GIT_REMOTE_ORIGIN_URL}")
  message(STATUS "GIT_LATEST_TAG_LONG : ${GIT_LATEST_TAG_LONG}")
  message(STATUS "GIT_LATEST_TAG : ${GIT_LATEST_TAG}")
  message(STATUS "")
  message(STATUS "----------------------------------------------------")
  message(STATUS "")
endfunction()
