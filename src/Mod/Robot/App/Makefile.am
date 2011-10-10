
lib_LTLIBRARIES=libRobot.la Robot.la
noinst_LTLIBRARIES=libkdl.la

BUILT_SOURCES=\
		Robot6AxisPy.cpp \
		RobotObjectPy.cpp \
		TrajectoryPy.cpp \
		WaypointPy.cpp

libRobot_la_BUILT=\
		Robot6AxisPy.h \
		RobotObjectPy.h \
		TrajectoryPy.h \
		WaypointPy.h

libkdl_la_SOURCES=\
		kdl_cp/utilities/error.h \
		kdl_cp/utilities/error_stack.cxx \
		kdl_cp/utilities/error_stack.h \
		kdl_cp/utilities/kdl-config.h \
		kdl_cp/utilities/rall1d.h \
		kdl_cp/utilities/rall1d_io.h \
		kdl_cp/utilities/rall2d.h \
		kdl_cp/utilities/rall2d_io.h \
		kdl_cp/utilities/rallNd.h \
		kdl_cp/utilities/svd_eigen_HH.cpp \
		kdl_cp/utilities/svd_eigen_HH.hpp \
		kdl_cp/utilities/svd_eigen_Macie.hpp \
		kdl_cp/utilities/svd_HH.cpp \
		kdl_cp/utilities/svd_HH.hpp \
		kdl_cp/utilities/traits.h \
		kdl_cp/utilities/utility.cxx \
		kdl_cp/utilities/utility.h \
		kdl_cp/utilities/utility_io.cxx \
		kdl_cp/utilities/utility_io.h \
		kdl_cp/articulatedbodyinertia.cpp \
		kdl_cp/articulatedbodyinertia.hpp \
		kdl_cp/chain.cpp \
		kdl_cp/chain.hpp \
		kdl_cp/chaindynparam.cpp \
		kdl_cp/chaindynparam.hpp \
		kdl_cp/chainfksolver.hpp \
		kdl_cp/chainfksolverpos_recursive.cpp \
		kdl_cp/chainfksolverpos_recursive.hpp \
		kdl_cp/chainfksolvervel_recursive.cpp \
		kdl_cp/chainfksolvervel_recursive.hpp \
		kdl_cp/chainidsolver.hpp \
		kdl_cp/chainidsolver_recursive_newton_euler.cpp \
		kdl_cp/chainidsolver_recursive_newton_euler.hpp \
		kdl_cp/chainiksolver.hpp \
		kdl_cp/chainiksolverpos_nr.cpp \
		kdl_cp/chainiksolverpos_nr.hpp \
		kdl_cp/chainiksolverpos_nr_jl.cpp \
		kdl_cp/chainiksolverpos_nr_jl.hpp \
		kdl_cp/chainiksolvervel_pinv.cpp \
		kdl_cp/chainiksolvervel_pinv_givens.cpp \
		kdl_cp/chainiksolvervel_pinv_givens.hpp \
		kdl_cp/chainiksolvervel_pinv.hpp \
		kdl_cp/chainiksolvervel_pinv_nso.cpp \
		kdl_cp/chainiksolvervel_pinv_nso.hpp \
		kdl_cp/chainiksolvervel_wdls.cpp \
		kdl_cp/chainiksolvervel_wdls.hpp \
		kdl_cp/chainjnttojacsolver.cpp \
		kdl_cp/chainjnttojacsolver.hpp \
		kdl_cp/frameacc.cpp \
		kdl_cp/frameacc.hpp \
		kdl_cp/frameacc.inl \
		kdl_cp/frameacc_io.hpp \
		kdl_cp/frames.cpp \
		kdl_cp/frames.hpp \
		kdl_cp/frames.inl \
		kdl_cp/frames_io.cpp \
		kdl_cp/frames_io.hpp \
		kdl_cp/framevel.cpp \
		kdl_cp/framevel.hpp \
		kdl_cp/framevel.inl \
		kdl_cp/framevel_io.hpp \
		kdl_cp/jacobian.cpp \
		kdl_cp/jacobian.hpp \
		kdl_cp/jntarrayacc.cpp \
		kdl_cp/jntarrayacc.hpp \
		kdl_cp/jntarray.cpp \
		kdl_cp/jntarray.hpp \
		kdl_cp/jntarrayvel.cpp \
		kdl_cp/jntarrayvel.hpp \
		kdl_cp/jntspaceinertiamatrix.cpp \
		kdl_cp/jntspaceinertiamatrix.hpp \
		kdl_cp/joint.cpp \
		kdl_cp/joint.hpp \
		kdl_cp/kdl.hpp \
		kdl_cp/kinfam.hpp \
		kdl_cp/kinfam_io.cpp \
		kdl_cp/kinfam_io.hpp \
		kdl_cp/motion.hpp \
		kdl_cp/path_circle.cpp \
		kdl_cp/path_circle.hpp \
		kdl_cp/path_composite.cpp \
		kdl_cp/path_composite.hpp \
		kdl_cp/path.cpp \
		kdl_cp/path_cyclic_closed.cpp \
		kdl_cp/path_cyclic_closed.hpp \
		kdl_cp/path.hpp \
		kdl_cp/path_line.cpp \
		kdl_cp/path_line.hpp \
		kdl_cp/path_point.cpp \
		kdl_cp/path_point.hpp \
		kdl_cp/path_roundedcomposite.cpp \
		kdl_cp/path_roundedcomposite.hpp \
		kdl_cp/rigidbodyinertia.cpp \
		kdl_cp/rigidbodyinertia.hpp \
		kdl_cp/rotationalinertia.cpp \
		kdl_cp/rotationalinertia.hpp \
		kdl_cp/rotational_interpolation.cpp \
		kdl_cp/rotational_interpolation.hpp \
		kdl_cp/rotational_interpolation_sa.cpp \
		kdl_cp/rotational_interpolation_sa.hpp \
		kdl_cp/segment.cpp \
		kdl_cp/segment.hpp \
		kdl_cp/stiffness.hpp \
		kdl_cp/trajectory_composite.cpp \
		kdl_cp/trajectory_composite.hpp \
		kdl_cp/trajectory.cpp \
		kdl_cp/trajectory.hpp \
		kdl_cp/trajectory_segment.cpp \
		kdl_cp/trajectory_segment.hpp \
		kdl_cp/trajectory_stationary.cpp \
		kdl_cp/trajectory_stationary.hpp \
		kdl_cp/tree.cpp \
		kdl_cp/treefksolver.hpp \
		kdl_cp/treefksolverpos_recursive.cpp \
		kdl_cp/treefksolverpos_recursive.hpp \
		kdl_cp/tree.hpp \
		kdl_cp/treeiksolver.hpp \
		kdl_cp/treeiksolverpos_nr_jl.cpp \
		kdl_cp/treeiksolverpos_nr_jl.hpp \
		kdl_cp/treeiksolvervel_wdls.cpp \
		kdl_cp/treeiksolvervel_wdls.hpp \
		kdl_cp/treejnttojacsolver.cpp \
		kdl_cp/treejnttojacsolver.hpp \
		kdl_cp/velocityprofile.cpp \
		kdl_cp/velocityprofile_dirac.cpp \
		kdl_cp/velocityprofile_dirac.hpp \
		kdl_cp/velocityprofile.hpp \
		kdl_cp/velocityprofile_rect.cpp \
		kdl_cp/velocityprofile_rect.hpp \
		kdl_cp/velocityprofile_trap.cpp \
		kdl_cp/velocityprofile_traphalf.cpp \
		kdl_cp/velocityprofile_traphalf.hpp \
		kdl_cp/velocityprofile_trap.hpp

libRobot_la_SOURCES=\
		AppRobotPy.cpp \
		Edge2TracObject.cpp \
		Edge2TracObject.h \
		TrajectoryDressUpObject.cpp \
		TrajectoryDressUpObject.h \
		TrajectoryCompound.cpp \
		TrajectoryCompound.h \
		PreCompiled.cpp \
		PreCompiled.h \
		PropertyTrajectory.cpp \
		PropertyTrajectory.h \
		Robot6Axis.cpp \
		Robot6Axis.h \
		Robot6AxisPyImp.cpp \
		RobotAlgos.cpp \
		RobotAlgos.h \
		RobotObject.cpp \
		RobotObject.h \
		RobotObjectPyImp.cpp \
		Simulation.cpp \
		Simulation.h \
		Trajectory.cpp \
		Trajectory.h \
		TrajectoryObject.cpp \
		TrajectoryObject.h \
		TrajectoryPyImp.cpp \
		Waypoint.cpp \
		Waypoint.h \
		WaypointPyImp.cpp 

%.cpp: %.xml $(top_srcdir)/src/Tools/generateTemplates/templateClassPyExport.py
	$(PYTHON) $(top_srcdir)/src/Tools/generate.py --outputPath $(@D) $<

# the library search path.
libRobot_la_LDFLAGS = -L../../../Base -L../../../App -L../../Part/App \
	-L$(OCC_LIB) $(QT4_CORE_LIBS) $(all_libraries) \
	-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libRobot_la_CPPFLAGS = -DRobotAppExport= -DEIGEN2_SUPPORT
libkdl_la_CPPFLAGS = -DEIGEN2_SUPPORT
#libkdl_la_CPPFLAGS += -Wno-sign-compare -Wno-reorder

libRobot_la_LIBADD   = \
		@BOOST_REGEX_LIB@ @BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lkdl \
		-lTKBRep \
		-lTKernel \
		-lTKFillet \
		-lTKG2d \
		-lTKG3d \
		-lTKMath \
		-lTKMesh \
		-lTKSTEP \
		-lTKSTEPAttr \
		-lTKSTEPBase \
		-lTKXSBase \
		-lTKTopAlgo \
		-lTKGeomAlgo \
		-lTKGeomBase \
		-lTKOffset \
		-lTKPrim \
		-lPart \
		-lFreeCADBase \
		-lFreeCADApp

#--------------------------------------------------------------------------------------
# Loader of libRobot

Robot_la_SOURCES=\
		AppRobot.cpp

# the library search path.
Robot_la_LDFLAGS = $(libRobot_la_LDFLAGS) -module -avoid-version
Robot_la_CPPFLAGS = $(libRobot_la_CPPFLAGS)

Robot_la_LIBADD   = \
		$(libRobot_la_LIBADD) \
		-lRobot

libRobot_la_DEPENDENCIES = libkdl.la
Robot_la_DEPENDENCIES = libRobot.la

#--------------------------------------------------------------------------------------

# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(top_srcdir)/src/Mod/Robot/App \
		$(all_includes) -I$(EIGEN3_INC) $(QT4_CORE_CXXFLAGS) -I$(OCC_INC)


libdir = $(prefix)/Mod/Robot

CLEANFILES = $(BUILT_SOURCES) $(libRobot_la_BUILT)

EXTRA_DIST = \
		kdl_cp/utilities/header.txt \
		kdl_cp/CMakeLists.txt \
		kdl_cp/README.txt \
		kdl_cp/TODO.txt \
		Robot6AxisPy.xml \
		RobotObjectPy.xml \
		TrajectoryPy.xml \
		WaypointPy.xml \
		CMakeLists.txt

