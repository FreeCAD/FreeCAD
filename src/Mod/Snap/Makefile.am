# Change data dir from default ($(prefix)/share) to actual dir
datadir = $(prefix)/Mod/Snap

data_DATA = InitGui.py \
	SnapGui.py \
	Tools.py

nobase_data_DATA = \
	Icons/Ico.png \
	Icons/Ico.xcf \
	Icons/Ico.xpm \
    snapWidget/__init__.py \
    snapWidget/Widget.py \
	snapUtils/__init__.py \
	snapUtils/Paths.py \
	snapUtils/Translator.py \
	snapTest/__init__.py \
	snapTest/TaskPanel.py \
	snapTest/TaskPanel.ui

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(data_DATA) \
		$(nobase_data_DATA) \
		CMakeLists.txt \
		README \
		snap.dox

