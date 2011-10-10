noinst_LTLIBRARIES=libpropertyeditor.la

BUILT_SOURCES=\
		moc_PropertyEditor.cpp \
		moc_PropertyItem.cpp \
		moc_PropertyItemDelegate.cpp \
		moc_PropertyModel.cpp

libpropertyeditor_la_SOURCES=\
		PropertyEditor.cpp \
		PropertyItem.cpp \
		PropertyItemDelegate.cpp \
		PropertyModel.cpp


includedir = @includedir@/Gui/propertyeditor

include_HEADERS=\
		PropertyEditor.h \
		PropertyItem.h \
		PropertyItemDelegate.h \
		PropertyModel.h

# rule for Qt MetaObject Compiler:
moc_%.cpp: %.h
	$(QT_MOC) $< -o $(@F)

# rule for Qt MetaObject Compiler:
%.moc: %.h
	$(QT_MOC) $< -o $(@F)


# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src -I$(srcdir)/.. $(QT_CXXFLAGS) $(all_includes) \
		-I$(sim_ac_coin_includedir) -I$(sim_ac_soqt_includedir)

CLEANFILES = $(BUILT_SOURCES)

