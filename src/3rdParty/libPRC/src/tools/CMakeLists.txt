if( OPENSCENEGRAPH_FOUND )
    add_subdirectory( prcconvert )
endif()

if( LIBHARU_FOUND )
   add_subdirectory( prctopdf )
endif()
