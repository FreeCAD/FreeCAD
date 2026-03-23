# source files
set(COIN_PROFILER_FILES
	SoProfiler.cpp
	SoProfilerElement.cpp
	SoProfilerOverlayKit.cpp
	SoProfilerStats.cpp
	SoProfilingReportGenerator.cpp
	SoProfilerTopEngine.cpp
	SoScrollingGraphKit.cpp
	SoNodeVisualize.cpp
	SoProfilerTopKit.cpp
	SoProfilerVisualizeKit.cpp
	SbProfilingData.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_PROFILER_INTERNAL_FILES
	SoNodeProfiling.h
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_PROFILER_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} profiler/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(profiler OBJECT ${COIN_PROFILER_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_PROFILER_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/profiler/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_PROFILER_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/profiler/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)

# SoProfilerP is not quite private, it is included from VRML nodes. The symbol is excluded from public API in doc/CMakeList.txt.
set(COIN_DOCUMENTATION_FILES
	${COIN_DOCUMENTATION_FILES}
	${PROJECT_SOURCE_DIR}/src/profiler/SoProfilerP.h
	CACHE INTERNAL "COIN_DOCUMENTATION_FILES"
)
