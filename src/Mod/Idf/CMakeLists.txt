
SET(Idf_SRCS
    Init.py
    Idf.py
)
SOURCE_GROUP("" FILES ${Idf_SRCS})

SET(IdfLibs_SRCS
    Idflibs/License.txt
    Idflibs/0603_SMD.stp
    Idflibs/0805_SMD.stp
    Idflibs/1206_SMD.stp
    Idflibs/1210_SMD.stp
    Idflibs/1812_SMD.stp
    Idflibs/2225_SMD.stp
    Idflibs/2512_SMD.stp
    Idflibs/CAP_50SGV_8_10.stp
    Idflibs/EPL22_6_16.stp
    Idflibs/footprints_models.csv
    Idflibs/I22_2_5_16.stp
    Idflibs/I22_2_5_16withEPL22_6_16.stp
    Idflibs/MSOP_10.stp
    Idflibs/RLF_12545.stp
    Idflibs/RLF_7030.stp
    Idflibs/SMB_DO_214AA.stp
    Idflibs/SMC_DO_214AB.stp
    Idflibs/SOD_323.igs
    Idflibs/SOD_323.stp
    Idflibs/SOD_523.stp
    Idflibs/SOT23.stp
    Idflibs/SOT404.stp
    Idflibs/SOT428_DPAK.stp
    Idflibs/SOT_323_3.stp
    Idflibs/SOT_96.stp
    Idflibs/TCMT1107_4.stp
    Idflibs/TSM_103_01_L_DV_A.stp
    Idflibs/TSM_104_01_L_DV_A.stp
    Idflibs/TSS0P_8.stp
    Idflibs/VC0603_SMD.stp
    Idflibs/RLF_12545.igs
    Idflibs/SOT23.igs
    Idflibs/SOT404.igs
)
SOURCE_GROUP("Idflibs" FILES ${IdfLibs_SRCS})


SET(all_files ${Idf_SRCS} ${IdfLibs_SRCS} )

ADD_CUSTOM_TARGET(Idf ALL
    SOURCES ${all_files}
)

fc_copy_sources(Idf "${CMAKE_BINARY_DIR}/Mod/Idf" ${all_files})

INSTALL(
    FILES
        ${IdfLibs_SRCS}
    DESTINATION
        Mod/Idf/Idflibs
)
INSTALL(
    FILES
        ${IdfLang_SRCS}
    DESTINATION
        Mod/Idf/Languages
)
INSTALL(
    FILES
        ${Idf_SRCS}
    DESTINATION
        Mod/Idf
)
