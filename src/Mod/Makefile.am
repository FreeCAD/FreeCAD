#SUBDIRS=Part Mesh Points Raytracing Image Drawing Complete Draft Test TemplatePyMod
SUBDIRS=Points Complete Draft Test TemplatePyMod Web Start Idf Arch Surfaces Ship

#if HAVE_OPENCV
SUBDIRS += Image
#endif

#if HAVE_GTS
SUBDIRS += Mesh
#endif

if HAVE_OPENCASCADE
SUBDIRS += Part Import PartDesign Raytracing Drawing
endif

if HAVE_EIGEN3
if HAVE_OPENCASCADE
SUBDIRS += Robot
endif
endif

if HAVE_EIGEN3
if HAVE_OPENCASCADE
SUBDIRS += Sketcher
endif
endif

#if HAVE_GTS
if HAVE_OPENCASCADE
SUBDIRS += ReverseEngineering MeshPart Fem Inspection
endif
#endif

if BUILD_SANDBOX
SUBDIRS += Sandbox
endif

EXTRA_DIST = \
		__init__.py \
		CMakeLists.txt \
		mod.dox

