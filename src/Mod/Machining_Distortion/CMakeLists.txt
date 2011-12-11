
SET(MachDist_SRCS
    Init.py
    InitGui.py
    ApplyingBC_IC.py
    mach_dist_gui.py
    postprocess.py
    postprocess_gui.py
	calculix_postprocess.py
	MachiningDistortionCommands.py
	User_Interface_Mach_Dist.py
	machdist_rc.py
    
)
SOURCE_GROUP("" FILES ${MachDist_SRCS})


ADD_CUSTOM_TARGET(MachDistortion ALL
    SOURCES ${MachDist_SRCS}
)

fc_copy_sources("Mod/Machining_Distortion" ${MachDist_SRCS})


INSTALL(
    FILES
        ${MachDist_SRCS}
    DESTINATION
        Mod/Machining_Distortion
)
