import FreeCAD
from PySide.QtWidgets import QWidget, QPlainTextEdit
from PySide import QtCore, QtGui


class GCodeHighlighter(QtGui.QSyntaxHighlighter):
    def __init__(self, parent=None):
        def convertcolor(c):
            return QtGui.QColor(int((c >> 24) & 0xFF), int((c >> 16) & 0xFF), int((c >> 8) & 0xFF))

        super(GCodeHighlighter, self).__init__(parent)
        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Editor")
        colors = []
        c = p.GetUnsigned("Number")
        if c:
            colors.append(convertcolor(c))
        else:
            colors.append(QtCore.Qt.red)
        c = p.GetUnsigned("Keyword")
        if c:
            colors.append(convertcolor(c))
        else:
            colors.append(QtGui.QColor(0, 170, 0))
        c = p.GetUnsigned("Define name")
        if c:
            colors.append(convertcolor(c))
        else:
            colors.append(QtGui.QColor(160, 160, 164))

        self.highlightingRules = []
        numberFormat = QtGui.QTextCharFormat()
        numberFormat.setForeground(colors[0])
        self.highlightingRules.append((QtCore.QRegularExpression("[\\-0-9\\.]"), numberFormat))
        keywordFormat = QtGui.QTextCharFormat()
        keywordFormat.setForeground(colors[1])
        keywordFormat.setFontWeight(QtGui.QFont.Bold)
        keywordPatterns = ["\\bG[0-9]+\\b", "\\bM[0-9]+\\b"]
        self.highlightingRules.extend(
            [(QtCore.QRegularExpression(pattern), keywordFormat) for pattern in keywordPatterns]
        )
        speedFormat = QtGui.QTextCharFormat()
        speedFormat.setFontWeight(QtGui.QFont.Bold)
        speedFormat.setForeground(colors[2])
        self.highlightingRules.append((QtCore.QRegularExpression("\\bF[0-9\\.]+\\b"), speedFormat))

    def highlightBlock(self, text):
        if self.enable:
            for pattern, fmt in self.highlightingRules:
                expression = QtCore.QRegularExpression(pattern)
                index = expression.match(text)
                while index.hasMatch():
                    length = index.capturedLength()
                    self.setFormat(index.capturedStart(), length, fmt)
                    index = expression.match(text, index.capturedStart() + length)


class LineNumberArea(QWidget):
    def __init__(self, editor):
        super().__init__(editor)
        self.editor = editor

    def sizeHint(self):
        return QtCore.QSize(self.editor.fontMetrics().horizontalAdvance("999") + 4, 0)

    def paintEvent(self, event):
        painter = QtGui.QPainter(self)
        painter.fillRect(event.rect(), QtCore.Qt.black)
        font = QtGui.QFont()
        font.setFamily(self.editor.font().family())
        font.setFixedPitch(self.editor.font().fixedPitch())
        font.setPointSize(self.editor.font().pointSize())
        painter.setFont(font)

        block = self.editor.firstVisibleBlock()
        blockNumber = block.blockNumber()
        top = self.editor.blockBoundingGeometry(block).translated(self.editor.contentOffset()).top()
        bottom = top + self.editor.blockBoundingRect(block).height()

        while block.isValid() and top <= event.rect().bottom():
            if (
                block.isVisible()
                and bottom >= event.rect().top()
                and self.editor.toPlainText() != ""
            ):
                number = str(blockNumber + 1)
                painter.setPen(QtCore.Qt.gray)
                painter.drawText(
                    -6,
                    int(top),
                    self.width(),
                    self.editor.fontMetrics().height(),
                    QtCore.Qt.AlignRight,
                    number,
                )

            block = block.next()
            top = bottom
            bottom = top + self.editor.blockBoundingRect(block).height()
            blockNumber += 1


class CodeEditor(QPlainTextEdit):
    def __init__(self, parent=None):
        super().__init__(parent)

        prefs = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Mod/CAM")
        self.mhs = prefs.GetUnsigned("MaxHighlighterSize", 100000)
        self.highLighter = GCodeHighlighter(self.document())
        self.highLighter.enable = False

        self.lineNumberArea = LineNumberArea(self)
        self.blockCountChanged.connect(self.updateLineNumberAreaWidth)
        self.updateRequest.connect(self.updateLineNumberArea)
        self.verticalScrollBar().valueChanged.connect(self.lineNumberArea.update)
        self.updateLineNumberAreaWidth(0)  # Initial call

        p = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Editor")
        font = QtGui.QFont()
        font.setFamily(p.GetString("Font", "Courier"))
        font.setFixedPitch(True)
        font.setPointSize(p.GetInt("FontSize", 10))
        self.setFont(font)

    def lineNumberAreaWidth(self):
        digits = 2
        max_value = max(1, self.blockCount())
        while max_value >= 10:
            max_value /= 10
            digits += 1
        space = 3 + self.fontMetrics().horizontalAdvance("9") * digits
        return space

    def updateLineNumberAreaWidth(self, newBlockCount):
        self.setViewportMargins(self.lineNumberAreaWidth(), 0, 0, 0)

    def updateLineNumberArea(self, rect, dy):
        if dy:
            self.lineNumberArea.scroll(0, dy)
        else:
            self.lineNumberArea.update(0, rect.y(), self.lineNumberArea.width(), rect.height())

        if rect.contains(self.viewport().rect()):
            self.updateLineNumberAreaWidth(0)

    def resizeEvent(self, event):
        super().resizeEvent(event)
        cr = self.contentsRect()
        self.lineNumberArea.setGeometry(
            QtCore.QRect(cr.left(), cr.top(), self.lineNumberAreaWidth(), cr.height())
        )

    def setText(self, text):
        """Backward compatibility method for setText() calls."""
        self.setPlainText(text)

    def setPlainText(self, text):
        if len(text) > self.mhs:
            self.highLighter.enable = False
        else:
            self.highLighter.enable = True
        super().setPlainText(text)
