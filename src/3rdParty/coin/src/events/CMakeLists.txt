# source files
set(COIN_EVENTS_FILES
	SoButtonEvent.cpp
	SoEvent.cpp
	SoKeyboardEvent.cpp
	SoLocation2Event.cpp
	SoMotion3Event.cpp
	SoMouseButtonEvent.cpp
	SoSpaceballButtonEvent.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_EVENTS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} events/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(events OBJECT ${COIN_EVENTS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_EVENTS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/events/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
