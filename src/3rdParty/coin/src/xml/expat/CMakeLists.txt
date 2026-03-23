# source files
set(COIN_EXPAT_FILES
	xmlparse.c
	xmlrole.c
	xmltok.c
	loadlibrary.c
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_EXPAT_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} xml/expat/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(expat OBJECT ${COIN_EXPAT_FILES})
endif()
