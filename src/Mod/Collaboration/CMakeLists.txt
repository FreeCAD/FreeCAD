# SPDX-License-Identifier: LGPL-2.1-or-later

SET(Collaboration_SRCS_python
  collaboration/init.py
  collaboration/app/init.py
  collaboration/app/util.py
  collaboration/gui/init.py
  collaboration/gui/components/init.py
  collaboration/gui/components/task_topic_dialog.py
  Init.py
)

add_subdirectory(App)

if(BUILD_GUI)
  add_subdirectory(Gui)
  list (APPEND Collaboration_SRCS_python
    InitGui.py
  )
endif(BUILD_GUI)

ADD_CUSTOM_TARGET(CollaborationScripts ALL
  SOURCES ${Collaboration_SRCS_python}
)

fc_copy_sources(CollaborationScripts "${CMAKE_BINARY_DIR}/Mod/Collaboration" ${Collaboration_SRCS_python})

INSTALL(
  FILES
  ${Collaboration_SRCS_python}
  DESTINATION
  Mod/Collaboration
)
