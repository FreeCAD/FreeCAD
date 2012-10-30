
lib_LTLIBRARIES=libImageGui.la ImageGui.la

BUILT_SOURCES=\
		moc_GLImageBox.cpp \
		moc_ImageView.cpp \
		qrc_Image.cpp 

libImageGui_la_SOURCES=\
		AppImageGuiPy.cpp \
		Command.cpp \
		GLImageBox.cpp \
		ImageView.cpp \
		PreCompiled.cpp \
		PreCompiled.h \
		ViewProviderImagePlane.cpp \
		ViewProviderImagePlane.h \
		Workbench.cpp \
		XpmImages.h

includedir = @includedir@/Mod/Image/Gui

include_HEADERS=\
		GLImageBox.h \
		ImageView.h \
		Workbench.h

# the library search path.
libImageGui_la_LDFLAGS = -L../../../Base -L../../../App -L../../../Gui -L../App \
		$(QT_LIBS) $(GL_LIBS) $(all_libraries) \
		$(sim_ac_coin_ldflags) $(sim_ac_coin_libs) \
		$(sim_ac_soqt_ldflags) $(sim_ac_soqt_libs) \
		-version-info @LIB_CURRENT@:@LIB_REVISION@:@LIB_AGE@
libImageGui_la_CPPFLAGS = -DImageAppExport= -DImageGuiExport=

#		$(opencv_LIBS)
libImageGui_la_LIBADD   = \
		@BOOST_SYSTEM_LIB@ \
		-l@PYTHON_LIB@ \
		-lxerces-c \
		-lFreeCADBase \
		-lFreeCADApp \
		-lFreeCADGui \
		-lImage

#--------------------------------------------------------------------------------------
# Loader of libImageGui

ImageGui_la_SOURCES=\
		AppImageGui.cpp

# the library search path.
ImageGui_la_LDFLAGS = $(libImageGui_la_LDFLAGS) -module -avoid-version
ImageGui_la_CPPFLAGS = $(libImageGui_la_CPPFLAGS)

ImageGui_la_LIBADD   = \
		$(libImageGui_la_LIBADD) \
		-lImageGui

ImageGui_la_DEPENDENCIES = libImageGui.la

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
qrc_%.cpp: Resources/%.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)

# set the include path found by configure
# $(opencv_CFLAGS)
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(all_includes) $(QT_CXXFLAGS) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)

libdir = $(prefix)/Mod/Image

CLEANFILES = $(BUILT_SOURCES)

EXTRA_DIST = \
		CMakeLists.txt \
		Resources/Image.qrc \
		Resources/translations/Image_af.qm \
		Resources/translations/Image_af.ts \
		Resources/translations/Image_de.qm \
		Resources/translations/Image_de.ts \
		Resources/translations/Image_es-ES.qm \
		Resources/translations/Image_es-ES.ts \
		Resources/translations/Image_fi.qm \
		Resources/translations/Image_fi.ts \
		Resources/translations/Image_fr.qm \
		Resources/translations/Image_fr.ts \
		Resources/translations/Image_hr.qm \
		Resources/translations/Image_hr.ts \
		Resources/translations/Image_it.qm \
		Resources/translations/Image_it.ts \
		Resources/translations/Image_nl.qm \
		Resources/translations/Image_nl.ts \
		Resources/translations/Image_no.qm \
		Resources/translations/Image_no.ts \
		Resources/translations/Image_pl.qm \
		Resources/translations/Image_pl.ts \
		Resources/translations/Image_pt-BR.qm \
		Resources/translations/Image_pt-BR.ts \
		Resources/translations/Image_ru.qm \
		Resources/translations/Image_ru.ts \
		Resources/translations/Image_sv-SE.qm \
		Resources/translations/Image_sv-SE.ts \
		Resources/translations/Image_uk.qm \
		Resources/translations/Image_uk.ts \
		Resources/translations/Image_zh-CN.qm \
		Resources/translations/Image_zh-CN.ts \
		Resources/translations/Image_hu.qm \
		Resources/translations/Image_hu.ts \
		Resources/translations/Image_ja.qm \
		Resources/translations/Image_ja.ts \
		Resources/translations/Image_ro.qm \
		Resources/translations/Image_ro.ts \
		Resources/translations/Image_zh-TW.qm \
		Resources/translations/Image_zh-TW.ts \
		Resources/translations/Image_cs.qm \
		Resources/translations/Image_cs.ts \
		Resources/translations/Image_tr.qm \
		Resources/translations/Image_tr.ts \
		Resources/translations/Image_sk.qm \
		Resources/translations/Image_sk.ts \
		Resources/icons/image-import.svg

