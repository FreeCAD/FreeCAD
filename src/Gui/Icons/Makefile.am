noinst_LTLIBRARIES=libIcons.la

BUILT_SOURCES=\
		qrc_resource.cpp

nodist_libIcons_la_SOURCES=\
		qrc_resource.cpp

data_DATA = freecad.xpm freecad-doc.png

EXTRA_DIST = \
		$(data_DATA) \
		ClassBrowser/const_member.png \
		ClassBrowser/const_member.svg \
		ClassBrowser/member.png \
		ClassBrowser/member.svg \
		ClassBrowser/method.png \
		ClassBrowser/method.svg \
		ClassBrowser/property.png \
		ClassBrowser/property.svg \
		ClassBrowser/type_class.png \
		ClassBrowser/type_class.svg \
		ClassBrowser/type_enum.png \
		ClassBrowser/type_enum.svg \
		ClassBrowser/type_module.png \
		ClassBrowser/type_module.svg \
		BmpFactoryIcons.cpp \
		images.cpp \
		accessories-text-editor.svg \
		add.png \
		add.svg \
		applications-accessories.svg \
		applications-python.svg \
		background.png \
		breakpoint.png \
		breakpoint.svg \
		bulb.svg \
		bulb.xpm \
		button_down.svg \
		button_down.xpm \
		button_invalid.svg \
		button_left.svg \
		button_left.xpm \
		button_right.svg \
		button_right.xpm \
		button_up.svg \
		button_up.xpm \
		button_valid.svg \
		camera-photo.svg \
		debug-marker.png \
		debug-marker.svg \
		debug-start.svg \
		debug-stop.svg \
		delete.png \
		delete.svg \
		Document.svg \
		document-new.svg \
		document-open.svg \
		document-print.svg \
		document-properties.svg \
		document-save.svg \
		document-save-as.svg \
		DrawStyleAsIs.svg \
		DrawStyleFlatLines.svg \
		DrawStylePoints.svg \
		DrawStyleShaded.svg \
		DrawStyleWireFrame.svg \
		edit_Cancel.svg \
		edit_OK.svg \
		edit_remove.png \
		edit_remove.svg \
		edit-copy.svg \
		edit-cut.svg \
		edit-delete.svg \
		edit-edit.svg \
		edit-paste.svg \
		edit-redo.svg \
		edit-select-all.svg \
		edit-select-box.svg \
		edit-undo.svg \
		Feature.svg \
		freecad.svg \
		freecad-icon-16.png \
		freecad-icon-32.png \
		freecad-icon-48.png \
		freecad-icon-64.png \
		freecadsplash.png \
		help-browser.svg \
		internet-web-browser.svg \
		macro-execute.svg \
		macro-record.svg \
		macro-stop.svg \
		Material.svg \
		mouse_pointer.png \
		mouse_pointer.svg \
		Part_Measure_Clear_All.svg \
		Part_Measure_Toggle_All.svg \
		preferences-display.svg \
		preferences-general.svg \
		preferences-system.svg \
		process-stop.svg \
		resource.qrc \
		spaceball_button.svg \
		SpNav-PanLR.png \
		SpNav-PanLR.svg \
		SpNav-PanUD.png \
		SpNav-PanUD.svg \
		SpNav-Roll.png \
		SpNav-Roll.svg \
		SpNav-Spin.png \
		SpNav-Spin.svg \
		SpNav-Tilt.png \
		SpNav-Tilt.svg \
		SpNav-Zoom.png \
		SpNav-Zoom.svg \
		Std_ViewScreenShot.svg \
		system-log-out.svg \
		Tree_Annotation.svg \
		Tree_Dimension.svg \
		Tree_Python.svg \
		utilities-terminal.svg \
		view-axometric.svg \
		view-bottom.svg \
		view-front.svg \
		view-fullscreen.svg \
		view-isometric.svg \
		view-left.svg \
		view-measurement.svg \
		view-perspective.svg \
		view-rear.svg \
		view-refresh.svg \
		view-right.svg \
		view-rotate-left.svg \
		view-rotate-right.svg \
		view-top.svg \
		view-unselectable.svg \
		view-zoom-all.svg \
		view-zoom-border.svg \
		view-zoom-fit.svg \
		view-zoom-in.svg \
		view-zoom-out.svg \
		view-zoom-selection.svg \
		window-new.svg


# rules for Qt Resource Compiler:
qrc_%.cpp: %.qrc
	$(QT_RCC) -name $(*F) $< -o $(@F)


# set the include path found by configure
AM_CXXFLAGS = -I$(top_srcdir)/src -I$(top_builddir)/src $(QT_CXXFLAGS) $(all_includes)

CLEANFILES = $(BUILT_SOURCES)

