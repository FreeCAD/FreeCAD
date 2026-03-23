# source files
set(COIN_GLUE_FILES
	cg.cpp
	spidermonkey.cpp
	dl.cpp
	gl.cpp
	gl_wgl.cpp
	gl_agl.cpp
	gl_cgl.cpp
	gl_egl.cpp
	gl_glx.cpp
	GLUWrapper.cpp
	simage_wrapper.cpp
	openal_wrapper.cpp
	win32api.cpp
	zlib.cpp
	bzip2.cpp
	freetype.cpp
	normalization_cubemap.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_GLUE_INTERNAL_FILES
	cg.h
	cg.cpp
	spidermonkey.cpp
	gl_wgl.h
	gl_wgl.cpp
	gl_agl.h
	gl_agl.cpp
	gl_cgl.h
	gl_cgl.cpp
	gl_egl.h
	gl_egl.cpp
	gl_glx.h
	gl_glx.cpp
	GLUWrapper.h
	GLUWrapper.cpp
	simage_wrapper.h
	simage_wrapper.cpp
	openal_wrapper.h
	openal_wrapper.cpp
	win32api.h
	win32api.cpp
	zlib.h
	zlib.cpp
	bzip2.h
	bzip2.cpp
	freetype.h
	freetype.cpp
	normalization_cubemap.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_GLUE_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} glue/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(glue OBJECT ${COIN_GLUE_FILES})
	if(HAVE_LIBSIMAGE)
		target_compile_definitions(glue PRIVATE $<TARGET_PROPERTY:simage::simage,INTERFACE_COMPILE_DEFINITIONS>) 
		target_include_directories(glue PRIVATE $<TARGET_PROPERTY:simage::simage,INTERFACE_INCLUDE_DIRECTORIES>)
	endif()
endif()

# define documentation
foreach(SRCFILE ${COIN_GLUE_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/glue/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_GLUE_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/glue/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
