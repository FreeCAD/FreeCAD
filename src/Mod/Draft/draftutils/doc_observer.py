import FreeCAD as App

if App.GuiUp:
    import FreeCADGui as Gui
    from draftutils.todo import ToDo

    class _Draft_DocObserver():

        # See: /src/Gui/DocumentObserverPython.h

        def slotDeletedDocument(self, gui_doc):
            _finish_command_on_doc_close(gui_doc)

    _doc_observer = None

    def _doc_observer_start():
        global _doc_observer
        if _doc_observer is None:
            _doc_observer = _Draft_DocObserver()
            Gui.addDocumentObserver(_doc_observer)

    def _doc_observer_stop():
        global _doc_observer
        try:
            if _doc_observer is not None:
                Gui.removeDocumentObserver(_doc_observer)
        except:
            pass
        _doc_observer = None

    def _finish_command_on_doc_close(gui_doc):
        """Finish the active Draft or BIM command if the related document has been
        closed. Only works for commands that have set `App.activeDraftCommand.doc`
        and use a task panel.
        """
        if getattr(App, "activeDraftCommand", None) \
                and getattr(App.activeDraftCommand, "doc", None) == gui_doc.Document \
                and getattr(App.activeDraftCommand, "ui", None):
            if hasattr(App.activeDraftCommand.ui, "reject"):
                ToDo.delay(App.activeDraftCommand.ui.reject, None)
            elif hasattr(App.activeDraftCommand.ui, "escape"):
                ToDo.delay(App.activeDraftCommand.ui.escape, None)
