noinst_LTLIBRARIES=libResources.la

BUILT_SOURCES=\
		qrc_Robot.cpp 

nodist_libResources_la_SOURCES=\
		qrc_Robot.cpp 

EXTRA_DIST = \
		icons/Robot_CreateRobot.svg \
		icons/Robot_CreateTrajectory.svg \
		icons/Robot_Edge2Trac.svg \
		icons/Robot_Export.svg \
		icons/Robot_InsertWaypoint.svg \
		icons/Robot_InsertWaypointPre.svg \
		icons/Robot_RestoreHomePos.svg \
		icons/Robot_SetDefaultOrientation.svg \
		icons/Robot_SetDefaultValues.svg \
		icons/Robot_SetHomePos.svg \
		icons/Robot_Simulate.svg \
		icons/Robot_TrajectoryCompound.svg \
		icons/Robot_TrajectoryDressUp.svg \
		translations/Robot_af.qm \
		translations/Robot_af.ts \
		translations/Robot_de.qm \
		translations/Robot_de.ts \
		translations/Robot_es-ES.qm \
		translations/Robot_es-ES.ts \
		translations/Robot_fi.qm \
		translations/Robot_fi.ts \
		translations/Robot_fr.qm \
		translations/Robot_fr.ts \
		translations/Robot_hr.qm \
		translations/Robot_hr.ts \
		translations/Robot_it.qm \
		translations/Robot_it.ts \
		translations/Robot_nl.qm \
		translations/Robot_nl.ts \
		translations/Robot_no.qm \
		translations/Robot_no.ts \
		translations/Robot_pl.qm \
		translations/Robot_pl.ts \
		translations/Robot_pt-BR.qm \
		translations/Robot_pt-BR.ts \
		translations/Robot_ru.qm \
		translations/Robot_ru.ts \
		translations/Robot_sv-SE.qm \
		translations/Robot_sv-SE.ts \
		translations/Robot_uk.qm \
		translations/Robot_uk.ts \
		translations/Robot_zh-CN.qm \
		translations/Robot_zh-CN.ts \
		translations/Robot_zh-TW.qm \
		translations/Robot_zh-TW.ts \
		translations/Robot_cs.qm \
		translations/Robot_cs.ts \
		translations/Robot_ro.qm \
		translations/Robot_ro.ts \
		translations/Robot_ja.qm \
		translations/Robot_ja.ts \
		translations/Robot_hu.qm \
		translations/Robot_hu.ts \
		translations/Robot_sk.qm \
		translations/Robot_sk.ts \
		translations/Robot_tr.qm \
		translations/Robot_tr.ts \
		Robot.qrc \
		UpdateResources.bat


# rule for Qt MetaObject Compiler:
moc_%.cpp: %.h
	$(QT_MOC) $< -o $(@F)

# rule for Qt MetaObject Compiler:
%.moc: %.h
	$(QT_MOC) $< -o $(@F)

# rules for Qt Resource Compiler:
qrc_%.cpp: %.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)


# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(srcdir)/.. $(all_includes) $(QT_CXXFLAGS)

CLEANFILES = $(BUILT_SOURCES)

