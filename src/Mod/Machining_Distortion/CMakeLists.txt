
SET(MachDist_SRCS
    Init.py
    InitGui.py
    ApplyingBC_IC.py
    mach_dist_gui.py
	machdist_rc.py
	Parameter.py
    postprocess.py
    postprocess_gui.py
	calculix_postprocess.py
	MachiningDistortionCommands.py
	User_Interface_Mach_Dist.py
	machdist_rc.py
    
)
SOURCE_GROUP("" FILES ${MachDist_SRCS})


ADD_CUSTOM_TARGET(Machining_Distortion ALL
    SOURCES ${MachDist_SRCS}
)

fc_copy_sources(Machining_Distortion "${CMAKE_BINARY_DIR}/Mod/Machining_Distortion" ${MachDist_SRCS})


INSTALL(
    FILES
        ${MachDist_SRCS}
    DESTINATION
        Mod/Machining_Distortion
)
