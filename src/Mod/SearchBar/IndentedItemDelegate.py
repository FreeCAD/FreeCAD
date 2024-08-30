from PySide import QtGui

# Inspired by https://stackoverflow.com/a/5443220/324969
# Inspired by https://forum.qt.io/topic/69807/qtreeview-indent-entire-row
class IndentedItemDelegate(QtGui.QStyledItemDelegate):
  def __init__(self):
    super(IndentedItemDelegate, self).__init__()
  def paint(self, painter, option, index):
    depth = int(option.widget.model().itemData(index.siblingAtColumn(1))[0])
    indent = 16 * depth
    option.rect.adjust(indent, 0, 0, 0)
    super(IndentedItemDelegate, self).paint(painter, option, index)
