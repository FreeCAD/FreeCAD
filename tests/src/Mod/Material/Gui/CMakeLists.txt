# SPDX-License-Identifier: LGPL-2.1-or-later

# Qt tests
setup_qt_test(MaterialDelegate)

target_link_libraries(MaterialDelegate_Tests_run
    Materials
    MatGui
)
