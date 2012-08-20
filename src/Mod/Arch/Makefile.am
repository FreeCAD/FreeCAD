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
		ArchComponent.py \
		ArchBuilding.py \
		ArchCell.py \
		ArchFloor.py \
		ifcReader.py \
		importDAE.py \
		importIFC.py \
		importOBJ.py \
		Init.py \
		InitGui.py \
		ArchSite.py \
		ArchStructure.py \
		ArchWall.py \
		ArchSectionPlane.py \
		ArchWindow.py \
		ArchCommands.py \
		ArchAxis.py \
		ArchVRM.py \
		ArchRoof.py

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
        Resources/icons/Arch_Wall_Tree.svg \
        Resources/icons/Arch_Cell_Tree.svg \
        Resources/icons/Arch_Building_Tree.svg \
        Resources/icons/Arch_Floor_Tree.svg \
        Resources/icons/Arch_SectionPlane_Tree.svg \
        Resources/icons/Arch_Site_Tree.svg \
        Resources/icons/Arch_Structure_Tree.svg \
        Resources/icons/Arch_Window_Tree.svg \
		Resources/icons/Arch_Axis.svg \
		Resources/icons/Arch_Axis_Tree.svg \
		Resources/icons/Arch_Roof.svg \
		Resources/icons/Arch_Roof_Tree.svg \
		Resources/icons/Arch_CloseHoles.svg \
		Resources/icons/Arch_Check.svg \
		Resources/icons/Arch_SelectNonManifold.svg \
		Resources/ui/archprefs-base.ui

