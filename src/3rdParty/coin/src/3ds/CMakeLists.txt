# source files
set(COIN_3DS_FILES
	3dsLoader.cpp
	SoStream.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_3DS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} 3ds/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(3ds OBJECT ${COIN_3DS_FILES})
endif()
