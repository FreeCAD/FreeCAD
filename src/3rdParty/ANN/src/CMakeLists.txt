add_definitions(-D_USRDLL -DDLL_EXPORTS -DANN_PERF -DANN_NO_RANDOM)

include_directories(../include)

set(ANN_SRCS
		 ANN.cpp brute.cpp kd_tree.cpp kd_util.cpp kd_split.cpp
		 kd_dump.cpp kd_search.cpp kd_pr_search.cpp kd_fix_rad_search.cpp
		 bd_tree.cpp bd_search.cpp bd_pr_search.cpp bd_fix_rad_search.cpp
		 perf.cpp bd_tree.h kd_tree.h kd_split.h kd_util.h kd_search.h
		 kd_pr_search.h kd_fix_rad_search.h pr_queue.h pr_queue_k.h
		 ../include/ANN/ANN.h ../include/ANN/ANNperf.h ../include/ANN/ANNx.h
)

if(WIN32)
	set(ANN_LIBS
		debug MSVCRTD.LIB
        debug MSVCPRTD.LIB
		optimized MSVCRT.LIB
        optimized MSVCPRT.LIB
		Rpcrt4.lib
	)
endif(WIN32)

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/src/Main)

add_library(ANN SHARED ${ANN_SRCS})
target_link_libraries(ANN ${ANN_LIBS})

if(WIN32)
		 set_target_properties(ANN PROPERTIES DEBUG_OUTPUT_NAME "ANN_D")
		 set_target_properties(ANN PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/)
		 set_target_properties(ANN PROPERTIES PREFIX "../")
else(WIN32)
		 set_target_properties(ANN PROPERTIES PREFIX "")
endif(WIN32)

