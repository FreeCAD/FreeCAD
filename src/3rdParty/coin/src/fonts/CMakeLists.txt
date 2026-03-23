# source files
set(COIN_FONTS_FILES
	fontlib_wrapper.cpp
	win32.cpp
	freetype.cpp
	coretext.cpp
	fontspec.cpp
	glyph.cpp
	glyph2d.cpp
	glyph3d.cpp
	default3dfont.cpp
	default2dfont.cpp
	common.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_FONTS_INTERNAL_FILES
	fontlib_wrapper.h
	fontlib_wrapper.cpp
	win32.h
	win32.cpp
	freetype.h
	freetype.cpp
	coretext.h
	coretext.cpp
	fontspec.h
	fontspec.cpp
	glyph.h
	glyph.cpp
	glyph2d.h
	glyph2d.cpp
	glyph3d.h
	glyph3d.cpp
	defaultfonts.h
	default3dfont.cpp
	default2dfont.cpp
	common.h
	common.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_FONTS_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} fonts/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(fonts OBJECT ${COIN_FONTS_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_FONTS_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/fonts/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_FONTS_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/fonts/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
