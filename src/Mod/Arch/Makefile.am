#BUILT_SOURCES=\
#		Arch_rc.py

# rules for Qt Resource Compiler:
#%_rc.py: Resources/%.qrc
#	$(PYRCC4) -name $(*F) $< -o $(@F)

# Change data dir from default ($(prefix)/share) to $(prefix)
datadir = $(prefix)/Mod/Arch
data_DATA = \
		Arch_rc.py \
		Arch.py \
		Component.py \
		Building.py \
		Cell.py \
		Floor.py \
        IFC2X3_TC1.exp \
		ifcReader.py \
		importDAE.py \
		importIFC.py \
		importOBJ.py \
		Init.py \
		InitGui.py \
		Site.py \
		Structure.py \
		Wall.py \
		SectionPlane.py \
		Window.py \
		Commands.py

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(data_DATA) \
		CMakeLists.txt \
		arch.dox \
		Resources/Arch.qrc \
		Resources/icons/preferences-arch.svg \
		Resources/icons/Arch_Building.svg \
        Resources/icons/Arch_Cell.svg \
        Resources/icons/Arch_Floor.svg \
        Resources/icons/Arch_Site.svg \
        Resources/icons/Arch_Structure.svg \
        Resources/icons/Arch_Wall.svg \
		Resources/icons/Arch_Add.svg \
		Resources/icons/Arch_Remove.svg \
		Resources/icons/Arch_MeshToShape.svg \
		Resources/icons/Arch_SplitMesh.svg \
		Resources/icons/Arch_RemoveShape.svg \
		Resources/icons/Arch_SectionPlane.svg \
		Resources/icons/Arch_Window.svg \
		Resources/ui/archprefs-base.ui

