# SPDX-License-Identifier: LGPL-2.1-or-later

SET(Diff_SRCS_python
  diff/init.py
  diff/app/init.py
  diff/app/report_diff.py
  diff/app/compute_diff.py
  diff/app/util.py
  diff/app/test/test_objs.py
  Init.py
  TestDiffApp.py
)

ADD_CUSTOM_TARGET(DiffScripts ALL
  SOURCES ${Diff_SRCS_python}
)

fc_copy_sources(DiffScripts "${CMAKE_BINARY_DIR}/Mod/Diff" ${Diff_SRCS_python})

INSTALL(
  FILES
  ${Diff_SRCS_python}
  DESTINATION
  Mod/Diff
)
