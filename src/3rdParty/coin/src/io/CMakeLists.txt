# source files
set(COIN_IO_FILES
	SoInput.cpp
	SoInputP.cpp
	SoInput_FileInfo.cpp
	SoInput_Reader.cpp
	SoOutput.cpp
	SoOutput_Writer.cpp
	SoByteStream.cpp
	SoTranSender.cpp
	SoTranReceiver.cpp
	SoWriterefCounter.cpp
	gzmemio.cpp
)

# Files excluded from public API documentation, included in complete documentation.
set(COIN_IO_INTERNAL_FILES
	SoInputP.h
	SoInputP.cpp
	SoInput_FileInfo.h
	SoInput_FileInfo.cpp
	SoInput_Reader.h
	SoInput_Reader.cpp
	SoOutput_Writer.h
	SoOutput_Writer.cpp
	SoWriterefCounter.h
	SoWriterefCounter.cpp
	gzmemio.h
	gzmemio.cpp
)

# build library
if(COIN_BUILD_SINGLE_LIB)
	foreach(SRCFILE ${COIN_IO_FILES})
		set(COIN_SOURCE ${COIN_SOURCE} io/${SRCFILE} CACHE INTERNAL "COIN_SOURCE")
	endforeach(SRCFILE)
else()
	add_library(io OBJECT ${COIN_IO_FILES})
endif()

# define documentation
foreach(SRCFILE ${COIN_IO_FILES})
	set(COIN_DOCUMENTATION_FILES ${COIN_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/io/${SRCFILE} CACHE INTERNAL "COIN_DOCUMENTATION_FILES")
endforeach(SRCFILE)
foreach(SRCFILE ${COIN_IO_INTERNAL_FILES})
	set(COIN_INTERNAL_DOCUMENTATION_FILES ${COIN_INTERNAL_DOCUMENTATION_FILES} ${PROJECT_SOURCE_DIR}/src/io/${SRCFILE} CACHE INTERNAL "COIN_INTERNAL_DOCUMENTATION_FILES")
endforeach(SRCFILE)
