
_addLibrary( asymptote FORCE_STATIC
    PRC.h
    PRCbitStream.cc
    PRCbitStream.h
    PRCdouble.cc
    PRCdouble.h
    oPRCFile.cc
    oPRCFile.h
    writePRC.cc
    writePRC.h
)

# Set Position Independent Code flag for static library to allow linking into shared libraries
set_target_properties( asymptote PROPERTIES POSITION_INDEPENDENT_CODE ON )
