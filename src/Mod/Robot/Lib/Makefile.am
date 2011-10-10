# Change data dir from default $(datadir) to $(datadir)/Mod/Robot/Lib
datadir = @datadir@/Mod/Robot/Lib/Kuka

# For Debian based system we mustn't pick up these files
if MAKE_NO_DFSG_PACKAGE

data_DATA = \
		Kuka/kr16.wrl \
		Kuka/kr125_3.wrl \
		Kuka/kr210.WRL \
		Kuka/kr500_1.csv \
		Kuka/kr500_1.wrl \
		Kuka/kr_16.csv \
		Kuka/kr_125.csv \
		Kuka/kr_210_2.csv \
		Kuka/testprog.dat \
		Kuka/testprog.src

doc_DATA = \
		Kuka/kr_16.pdf \
		Kuka/kr125_2.pdf \
		Kuka/kr_210_2.pdf \
		Kuka/kr_500_2.pdf

EXTRA_DIST = \
		$(data_DATA) \
		$(doc_DATA)
endif

