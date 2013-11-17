noinst_LTLIBRARIES=libIcons.la

BUILT_SOURCES=\
		qrc_resource.cpp

nodist_libIcons_la_SOURCES=\
		qrc_resource.cpp

data_DATA = freecad.xpm freecad-doc.png

EXTRA_DIST = \
		$(data_DATA) \
		ClassBrowser/const_member.png \
		ClassBrowser/member.png \
		ClassBrowser/method.png \
		ClassBrowser/property.png \
		ClassBrowser/type_class.png \
		ClassBrowser/type_enum.png \
		ClassBrowser/type_module.png \
		ClassBrowser/const_member.svg \
		ClassBrowser/member.svg \
		ClassBrowser/method.svg \
		ClassBrowser/property.svg \
		ClassBrowser/type_class.svg \
		ClassBrowser/type_enum.svg \
		ClassBrowser/type_module.svg \
		add.png \
		delete.png \
		edit_remove.png \
		mouse_pointer.png \
		BmpFactoryIcons.cpp \
		background.xpm \
		bulb.xpm \
		button_down.xpm \
		button_left.xpm \
		button_right.xpm \
		button_up.xpm \
		Document.xpm \
		Feature.xpm \
		freecadsplash.png \
		freecad.svg \
		images.cpp \
		macro-execute.svg \
		macro-record.svg \
		macro-stop.svg \
		preferences-display.svg \
		preferences-general.svg \
		Std_ViewScreenShot.svg \
		utilities-terminal.svg \
		breakpoint.png \
		debug-marker.png \
		debug-start.svg \
		debug-stop.svg \
		document-new.svg \
		document-open.svg \
		document-save.svg \
		document-save-as.svg \
		document-print.svg \
		document-properties.svg \
		system-log-out.svg \
		edit_Cancel.svg \
		edit_OK.svg \
		edit-copy.svg \
		edit-cut.svg \
		edit-delete.svg \
		edit-edit.svg \
		edit-paste.svg \
		edit-select-all.svg \
		edit-select-box.svg \
		edit-redo.svg \
		edit-undo.svg \
		preferences-system.svg \
		window-new.svg \
		camera-photo.svg \
		applications-accessories.svg \
		accessories-text-editor.svg \
		help-browser.svg \
		spaceball_button.svg \
		SpNav-PanLR.png \
		SpNav-PanUD.png \
		SpNav-Roll.png \
		SpNav-Spin.png \
		SpNav-Tilt.png \
		SpNav-Zoom.png \
		view-unselectable.svg \
		view-refresh.svg \
		view-measurement.svg \
		view-fullscreen.svg \
		view-axometric.svg \
		view-isometric.svg \
		view-perspective.svg \
		view-bottom.svg \
		view-front.svg \
		view-left.svg \
		view-rear.svg \
		view-right.svg \
		view-top.svg \
		view-zoom-all.svg \
		view-zoom-border.svg \
		view-zoom-fit.svg \
		view-zoom-in.svg \
		view-zoom-out.svg \
		view-zoom-selection.svg \
		view-rotate-left.svg \
		view-rotate-right.svg \
		Tree_Annotation.svg \
		Tree_Dimension.svg \
		Tree_Python.svg \
		resource.qrc


# rules for Qt Resource Compiler:
qrc_%.cpp: %.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)


# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(QT_CXXFLAGS) $(all_includes)

CLEANFILES = $(BUILT_SOURCES)

