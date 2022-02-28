from PySide import QtGui
from PySide import QtCore
import FreeCAD as App
import FreeCADGui
import SafeViewer
import SearchBox

def documentAction(nfo):
  act = nfo['action']
    # Todo: this should also select the document in the tree view
  print('switch to document ' + act['document'])
  App.setActiveDocument(act['document'])

def documentObjectAction(nfo):
  act = nfo['action']
  print('select object ' + act['document'] + '.' + act['object'])
  FreeCADGui.Selection.addSelection(act['document'], act['object'])

# For some reason, the viewer always works except when used for two consecutive items in the search results: it then disappears after a short zoom-in+zoom-out animation.
# I'm giving up on getting this viewer to work in a clean way, and will try swapping two instances so that the same one is never used twice in a row.
# Also, in order to avoid segfaults when the module is reloaded (which causes the previous viewer to be garbage collected at some point), we're using a global property that will survive module reloads.
if not hasattr(App, '_SearchBar3DViewer'):
  # Toggle between 
  App._SearchBar3DViewer = None
  App._SearchBar3DViewerB = None

class DocumentObjectToolTipWidget(QtGui.QWidget):
  def __init__(self, nfo, setParent):
    import pivy
    super(DocumentObjectToolTipWidget, self).__init__()
    html = '<p>' + nfo['toolTip']['label'] + '</p><p><code>App.getDocument(' + repr(str(nfo['toolTip']['docName'])) + ').getObject(' + repr(str(nfo['toolTip']['name'])) + ')</code></p>'
    description = QtGui.QTextEdit()
    description.setReadOnly(True)
    description.setAlignment(QtCore.Qt.AlignTop)
    description.setText(html)

    if App._SearchBar3DViewer is None:
      oldFocus = QtGui.QApplication.focusWidget()
      SearchBox.globalIgnoreFocusOut
      SearchBox.globalIgnoreFocusOut = True
      App._SearchBar3DViewer = SafeViewer.SafeViewer()
      App._SearchBar3DViewerB = SafeViewer.SafeViewer()
      oldFocus.setFocus()
      SearchBox.globalIgnoreFocusOut = False
      # Tried setting the preview to a fixed size to prevent it from disappearing when changing its contents, this sets it to a fixed size but doesn't actually pick the size, .resize does that but isn't enough to fix the bug.
      #safeViewerInstance.setSizePolicy(QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Fixed))
    self.preview = App._SearchBar3DViewer
    App._SearchBar3DViewer, App._SearchBar3DViewerB = App._SearchBar3DViewerB, App._SearchBar3DViewer

    obj = App.getDocument(str(nfo['toolTip']['docName'])).getObject(str(nfo['toolTip']['name']))

    # This is really a bad way to do this… to prevent the setExtraInfo function from
    # finalizing the object, we remove the parent ourselves.
    oldParent = self.preview.parent()
    lay = QtGui.QVBoxLayout()
    lay.setContentsMargins(0,0,0,0)
    lay.setSpacing(0)
    self.setLayout(lay)
    lay.addWidget(description)
    lay.addWidget(self.preview)
    #if oldParent is not None:
    #  oldParent.hide() # hide before detaching, or we have widgets floating as their own window that appear for a split second in some cases.
    #  oldParent.setParent(None)
  
    # Tried hiding/detaching the preview to prevent it from disappearing when changing its contents
    #self.preview.viewer.stopAnimating()
    self.preview.showSceneGraph(obj.ViewObject.RootNode)

    setParent(self)
    # Let the GUI recompute the side of the description based on its horizontal size.
    FreeCADGui.updateGui()
    siz = description.document().size().toSize()
    description.setFixedHeight(siz.height() + 5)

  def finalizer(self):
    #self.preview.finalizer()
    # Detach the widget so that it may be reused without getting deleted
    self.preview.setParent(None)

def documentToolTip(nfo, setParent):
  return '<p>' + nfo['toolTip']['label'] + '</p><p><code>App.getDocument(' + repr(str(nfo['toolTip']['name'])) + ')</code></p><p><img src="data:image/png;base64,.............."></p>'

def documentObjectToolTip(nfo, setParent):
  return DocumentObjectToolTipWidget(nfo, setParent)

def documentResultsProvider():
  itemGroups = []
  def document(doc):
    group = []
    for o in doc.Objects:
      #all_actions.append(lambda: )
      action = { 'handler': 'documentObject', 'document': o.Document.Name, 'object': o.Name }
      item = {
        'icon': o.ViewObject.Icon if o.ViewObject and o.ViewObject.Icon else None,
        'text': o.Label + ' (' + o.Name + ')',
        # TODO: preview of the object
        'toolTip': { 'label': o.Label, 'name': o.Name, 'docName': o.Document.Name},
        'action': action,
        'subitems': []
      }
      group.append(item)

    action = { 'handler': 'document', 'document': doc.Name }
    itemGroups.append({
      'icon': QtGui.QIcon(':/icons/Document.svg'),
      'text': doc.Label + ' (' + doc.Name + ')',
      # TODO: preview of the document
      'toolTip': { 'label': doc.Label, 'name': doc.Name},
      'action':action,
      'subitems': group })
  if App.ActiveDocument:
    document(App.ActiveDocument)
  for docname, doc in App.listDocuments().items():
    if not App.activeDocument or docname != App.ActiveDocument.Name:
      document(doc)
  return itemGroups
