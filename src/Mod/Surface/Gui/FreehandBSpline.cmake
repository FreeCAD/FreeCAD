# SPDX-License-Identifier: LGPL-2.1-or-later
qt_add_resources(FreehandBSpline_QRC_SRCS Resources/FreehandBSpline.qrc OPTIONS ${FREECAD_RCC_OPTIONS})
target_sources(SurfaceGui PRIVATE
    ${FreehandBSpline_QRC_SRCS}
    AppFreehandBSplineGui.cpp
    CommandFreehandBSpline.cpp
    FreehandBSplineEditor.cpp FreehandBSplineEditor.h
    ViewProviderFreehandBSpline.cpp ViewProviderFreehandBSpline.h
)
