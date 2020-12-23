macro(SetupPCL)
# -------------------------------- pcl ----------------------------------

    # Can be used by ReverseEngineering module"
    #
    # PCL needs to be found before boost because the PCLConfig also calls find_package(Boost ...),
    # but with different components
    if(FREECAD_USE_PCL)
        find_package(PCL REQUIRED COMPONENTS common kdtree features surface io filters segmentation sample_consensus)
    endif(FREECAD_USE_PCL)
endmacro(SetupPCL)
