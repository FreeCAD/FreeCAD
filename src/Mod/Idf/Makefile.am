
# Change data dir from default ($(prefix)/share) to $(prefix)
datadir = $(prefix)/Mod/Idf
data_DATA = \
		Idf.py \
		Init.py

if MAKE_NO_DFSG_PACKAGE
data_DATA += \
		ISOL.emn \
		ISOL.emp

nobase_data_DATA = \
		lib/0603_SMD.stp \
		lib/0805_SMD.stp \
		lib/1206_SMD.stp \
		lib/1210_SMD.stp \
		lib/1812_SMD.stp \
		lib/2225_SMD.stp \
		lib/2512_SMD.stp \
		lib/CAP_50SGV_8_10.stp \
		lib/EPL22_6_16.stp \
		lib/footprints_models.csv \
		lib/I22_2_5_16.stp \
		lib/I22_2_5_16withEPL22_6_16.stp \
		lib/MSOP_10.stp \
		lib/RLF_12545.stp \
		lib/RLF_7030.stp \
		lib/SMB_DO_214AA.stp \
		lib/SMC_DO_214AB.stp \
		lib/SOD_323.igs \
		lib/SOD_323.stp \
		lib/SOD_523.stp \
		lib/SOT23.stp \
		lib/SOT404.stp \
		lib/SOT428_DPAK.stp \
		lib/SOT_323_3.stp \
		lib/SOT_96.stp \
		lib/TCMT1107_4.stp \
		lib/TSM_103_01_L_DV_A.stp \
		lib/TSM_104_01_L_DV_A.stp \
		lib/TSS0P_8.stp \
		lib/VC0603_SMD.stp
endif

EXTRA_DIST = \
		$(data_DATA) $(nobase_data_DATA) \
		CMakeLists.txt

