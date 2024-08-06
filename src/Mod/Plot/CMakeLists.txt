set(PlotMain_SRCS Plot.py)
source_group("" FILES ${PlotMain_SRCS})

set(all_files ${PlotMain_SRCS})

add_custom_target(Plot ALL SOURCES ${all_files})

fc_copy_sources(Plot "${CMAKE_BINARY_DIR}/Mod/Plot" ${all_files})

install(FILES ${PlotMain_SRCS} DESTINATION Mod/Plot)
