SUBDIRS=App Gui Scripts

# Create the file __init__.py so that we can import as
# from PartDesign.Scripts import ...
#install-exec-hook:
#		echo "import PartDesign" > $(DESTDIR)$(datadir)/__init__.py
#
#uninstall-hook:
#		rm -f $(DESTDIR)$(datadir)/__init__.py

# Change data dir from default ($(prefix)/share) to $(prefix)
datadir = $(prefix)/Mod/PartDesign

data_DATA = \
		__init__.py \
		Init.py \
		InitGui.py \
		InvoluteGearFeature.py \
		InvoluteGearFeature.ui \
		TestPartDesignApp.py \
		TestPartDesignGui.py \
		WizardShaft/__init__.py \
		WizardShaft/SegmentFunction.py \
		WizardShaft/ShaftDiagram.py \
		WizardShaft/ShaftFeature.py \
		WizardShaft/Shaft.py \
		WizardShaft/WizardShaft.py \
		WizardShaft/WizardShaft.svg \
		WizardShaft/WizardShaftTable.py

EXTRA_DIST = \
		$(data_DATA) \
		fcgear/fcgeardialog.py \
		fcgear/fcgear.py \
		fcgear/__init__.py \
		fcgear/involute.py \
		fcgear/README \
		fcgear/svggear.py \
		CMakeLists.txt \
		PartDesign_Model.xml \
		partdesign.dox 

