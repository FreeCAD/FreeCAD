
SET(Idf_SRCS
    Init.py
    Idf.py
)
SOURCE_GROUP("" FILES ${Idf_SRCS})

SET(IdfLibs_SRCS
    lib/0603_SMD.stp
    lib/0805_SMD.stp
    lib/1206_SMD.stp
    lib/1210_SMD.stp
    lib/1812_SMD.stp
    lib/2225_SMD.stp
    lib/2512_SMD.stp
    lib/CAP_50SGV_8_10.stp
    lib/EPL22_6_16.stp
    lib/footprints_models.csv
    lib/I22_2_5_16.stp
    lib/I22_2_5_16withEPL22_6_16.stp
    lib/MSOP_10.stp
    lib/RLF_12545.stp
    lib/RLF_7030.stp
    lib/SMB_DO_214AA.stp
    lib/SMC_DO_214AB.stp
    lib/SOD_323.igs
    lib/SOD_323.stp
    lib/SOD_523.stp
    lib/SOT23.stp
    lib/SOT404.stp
    lib/SOT428_DPAK.stp
    lib/SOT_323_3.stp
    lib/SOT_96.stp
    lib/TCMT1107_4.stp
    lib/TSM_103_01_L_DV_A.stp
    lib/TSM_104_01_L_DV_A.stp
    lib/TSS0P_8.stp
    lib/VC0603_SMD.stp
)
SOURCE_GROUP("Idflibs" FILES ${IdfLibs_SRCS})


SET(all_files ${Idf_SRCS} ${IdfLibs_SRCS} )

ADD_CUSTOM_TARGET(Idf ALL
    SOURCES ${all_files}
)

fc_copy_sources("Mod/Idf" "Idf" ${all_files})

install(
    FILES
        ${IdfLibs_SRCS}
    DESTINATION
        Mod/Idf/Idflibs
)

install(
    FILES
        ${IdfLang_SRCS}
    DESTINATION
        Mod/Idf/Languages
)

install(
    FILES
        ${Idf_SRCS}
    DESTINATION
        Mod/Idf
 )
