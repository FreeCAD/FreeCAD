SUBDIRS=Resources

lib_LTLIBRARIES=libRobotGui.la RobotGui.la

BUILT_SOURCES=\
		moc_TaskDlgEdge2Trac.cpp \
		moc_TaskDlgSimulate.cpp \
		moc_TaskEdge2TracParameter.cpp \
		moc_TaskRobot6Axis.cpp \
		moc_TaskRobotControl.cpp \
		moc_TaskRobotMessages.cpp \
		moc_TaskTrajectory.cpp \
		moc_TrajectorySimulate.cpp \
		moc_TaskWatcher.cpp \
		moc_TaskDlgTrajectoryDressUp.cpp \
		moc_TaskDlgTrajectoryCompound.cpp \
		moc_TaskTrajectoryDressUpParameter.cpp \
		ui_TaskEdge2TracParameter.h \
		ui_TaskRobot6Axis.h \
		ui_TaskTrajectoryDressUpParameter.h \
		ui_TaskRobotControl.h \
		ui_TaskRobotMessages.h \
		ui_TaskTrajectory.h \
		ui_TrajectorySimulate.h


libRobotGui_la_UI=\
		TaskEdge2TracParameter.ui \
		TaskRobot6Axis.ui \
		TaskRobotControl.ui \
		TaskRobotMessages.ui \
		TaskTrajectory.ui \
		TaskTrajectoryDressUpParameter.ui \
		TrajectorySimulate.ui

libRobotGui_la_SOURCES=\
		AppRobotGuiPy.cpp \
		Command.cpp \
		CommandExport.cpp \
		CommandInsertRobot.cpp \
		CommandTrajectory.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		TaskDlgEdge2Trac.cpp \
		TaskDlgEdge2Trac.h \
		TaskDlgSimulate.cpp \
		TaskDlgSimulate.h \
		TaskEdge2TracParameter.cpp \
		TaskEdge2TracParameter.h \
		TaskRobot6Axis.cpp \
		TaskRobot6Axis.h \
		TaskRobotControl.cpp \
		TaskRobotControl.h \
		TaskRobotMessages.cpp \
		TaskRobotMessages.h \
		TaskTrajectory.cpp \
		TaskTrajectory.h \
		TaskTrajectoryDressUpParameter.cpp \
		TaskTrajectoryDressUpParameter.h \
		TaskDlgTrajectoryCompound.cpp \
		TaskDlgTrajectoryCompound.h \
		TaskDlgTrajectoryDressUp.cpp \
		TaskDlgTrajectoryDressUp.h \
		TrajectorySimulate.cpp \
		TrajectorySimulate.h \
		ViewProviderEdge2TracObject.cpp \
		ViewProviderEdge2TracObject.h \
		ViewProviderTrajectoryDressUp.cpp \
		ViewProviderTrajectoryDressUp.h \
		ViewProviderTrajectoryCompound.cpp \
		ViewProviderTrajectoryCompound.h \
		ViewProviderRobotObject.cpp \
		ViewProviderRobotObject.h \
		ViewProviderTrajectory.cpp \
		ViewProviderTrajectory.h \
		TaskWatcher.cpp \
		TaskWatcher.h \
		Workbench.cpp \
		Workbench.h

# the library search path.
libRobotGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../../Part/App -L../App \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) $(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		$(QT_LIBS) $(GL_LIBS) $(all_libraries) -version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libRobotGui_la_CPPFLAGS = -DRobotAppExport= -DRobotGuiExport= -DEIGEN2_SUPPORT

libRobotGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lPart \
		-lRobot

#--------------------------------------------------------------------------------------
# Loader of libRobotGui

RobotGui_la_SOURCES=\
		AppRobotGui.cpp

# the library search path.
RobotGui_la_LDFLAGS = $(libRobotGui_la_LDFLAGS) -module -avoid-version
RobotGui_la_CPPFLAGS = $(libRobotGui_la_CPPFLAGS)

RobotGui_la_LIBADD   = \
		$(libRobotGui_la_LIBADD) \
		Resources/libResources.la \
		-lRobotGui

RobotGui_la_DEPENDENCIES = libRobotGui.la

#--------------------------------------------------------------------------------------

# rule for Qt MetaObject Compiler:
moc_%.cpp: %.h
	$(QT_MOC) $< -o $(@F)

# rule for Qt MetaObject Compiler:
%.moc: %.h
	$(QT_MOC) $< -o $(@F)

# rules for Qt User Interface Compiler:
ui_%.h: %.ui
	$(QT_UIC) $< -o $(@F)

# rules for Qt Resource Compiler:
qrc_%.cpp: %.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(QT_CXXFLAGS) $(all_includes) -I$(OCC_INC) -I$(EIGEN3_INC) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)

libdir = $(prefix)/Mod/Robot

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		$(libRobotGui_la_UI) \
		CMakeLists.txt

