/*
** Copyright (C) 2013 Jiří Procházka (Hobrasoft)
** Contact: http://www.hobrasoft.cz/
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file is under the terms of the GNU Lesser General Public License
** version 2.1 as published by the Free Software Foundation and appearing
** in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the
** GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
*/

/********************************
 * includes changes by wandererfan@gmail.com
 * for FreeCAD project https://www.freecad.org/
 ********************************/


# include <algorithm>
# include <iostream>
# include <QApplication>
# include <QBitmap>
# include <QClipboard>
# include <QColorDialog>
# include <QDialog>
# include <QFileDialog>
# include <QFontDatabase>
# include <QImageReader>
# include <QInputDialog>
# include <QMenu>
# include <QMimeData>
# include <QPlainTextEdit>
# include <QRegularExpression>
# include <QSettings>
# include <QTextList>


#include <App/Application.h>
#include <Base/Console.h>
#include <Gui/FileDialog.h>
#include <Mod/TechDraw/App/Preferences.h>

#include "mrichtextedit.h"
#include "PreferencesGui.h"


using namespace TechDrawGui;
using namespace TechDraw;

MRichTextEdit::MRichTextEdit(QWidget *parent, QString textIn) : QFrame(parent) {
    setupUi(this);

    f_fontsize->setEditable(true);
    f_fontsize->setMinimumContentsLength(3);

    m_lastBlockList = nullptr;
    f_textedit->setTabStopDistance(40);
    setDefFontSize(TechDrawGui::PreferencesGui::labelFontSizePX());
    m_defFont = getDefFont().family();
    f_textedit->setFont(getDefFont());

    connect(f_save, &QToolButton::clicked,
            this, &MRichTextEdit::onSave);
    connect(f_exit, &QToolButton::clicked,
            this, &MRichTextEdit::onExit);

    connect(f_textedit, &MTextEdit::currentCharFormatChanged,
            this, &MRichTextEdit::slotCurrentCharFormatChanged);
    connect(f_textedit, &MTextEdit::cursorPositionChanged,
            this, &MRichTextEdit::slotCursorPositionChanged);
    connect(f_textedit, &MTextEdit::selectionChanged,
            this, &MRichTextEdit::onSelectionChanged);


    m_fontsize_h1 = m_defFontSize + 8;
    m_fontsize_h2 = m_defFontSize + 6;
    m_fontsize_h3 = m_defFontSize + 4;
    m_fontsize_h4 = m_defFontSize + 2;

//TODO: should check for existing text and set font to match
    fontChanged(getDefFont());
    bgColorChanged(f_textedit->textColor());

    // paragraph formatting
    m_paragraphItems    << tr("Standard")
                        << tr("Heading 1")
                        << tr("Heading 2")
                        << tr("Heading 3")
                        << tr("Heading 4")
                        << tr("Monospace")
                        << QString::fromUtf8(" ");
    f_paragraph->addItems(m_paragraphItems);

    connect(f_paragraph, qOverload<int>(&QComboBox::activated),
            this, &MRichTextEdit::textStyle);

    // undo & redo

    f_undo->setShortcut(QKeySequence::Undo);
    f_redo->setShortcut(QKeySequence::Redo);

    connect(f_textedit->document(), &QTextDocument::undoAvailable,
            f_undo, &QToolButton::setEnabled);
    connect(f_textedit->document(), &QTextDocument::redoAvailable,
            f_redo, &QToolButton::setEnabled);

    f_undo->setEnabled(f_textedit->document()->isUndoAvailable());
    f_redo->setEnabled(f_textedit->document()->isRedoAvailable());

    connect(f_undo, &QToolButton::clicked, f_textedit, &MTextEdit::undo);
    connect(f_redo, &QToolButton::clicked, f_textedit, &MTextEdit::redo);

    // cut, copy & paste

    f_cut->setShortcut(QKeySequence::Cut);
    f_copy->setShortcut(QKeySequence::Copy);
    f_paste->setShortcut(QKeySequence::Paste);

    f_cut->setEnabled(false);
    f_copy->setEnabled(false);

    connect(f_cut, &QToolButton::clicked, f_textedit, &MTextEdit::cut);
    connect(f_copy, &QToolButton::clicked, f_textedit, &MTextEdit::copy);
    connect(f_paste, &QToolButton::clicked, f_textedit, &MTextEdit::paste);

    connect(f_textedit, &MTextEdit::copyAvailable, f_cut, &QToolButton::setEnabled);
    connect(f_textedit, &MTextEdit::copyAvailable, f_copy, &QToolButton::setEnabled);

#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &MRichTextEdit::slotClipboardDataChanged);
#endif

    // link

    f_link->setShortcut(QKeySequence(QString::fromUtf8("CTRL+L")));

    connect(f_link, &QToolButton::clicked, this, &MRichTextEdit::textLink);

    // bold, italic & underline

    f_bold->setShortcut(QKeySequence(QString::fromUtf8("CTRL+B")));
    f_italic->setShortcut(QKeySequence(QString::fromUtf8("CTRL+I")));
    f_underline->setShortcut(QKeySequence(QString::fromUtf8("CTRL+U")));

    connect(f_bold, &QToolButton::clicked, this, &MRichTextEdit::textBold);
    connect(f_italic, &QToolButton::clicked, this, &MRichTextEdit::textItalic);
    connect(f_underline, &QToolButton::clicked, this, &MRichTextEdit::textUnderline);
    connect(f_strikeout, &QToolButton::clicked, this, &MRichTextEdit::textStrikeout);

    QAction *removeFormat = new QAction(tr("Remove character formatting"), this);
    removeFormat->setShortcut(QKeySequence(QString::fromUtf8("CTRL+M")));
    connect(removeFormat, &QAction::triggered, this, &MRichTextEdit::textRemoveFormat);
    f_textedit->addAction(removeFormat);

    QAction *removeAllFormat = new QAction(tr("Remove all formatting"), this);
    connect(removeAllFormat, &QAction::triggered, this, &MRichTextEdit::textRemoveAllFormat);
    f_textedit->addAction(removeAllFormat);

    QAction *textsource = new QAction(tr("Edit document source"), this);
    textsource->setShortcut(QKeySequence(QString::fromUtf8("CTRL+O")));
    connect(textsource, &QAction::triggered, this, &MRichTextEdit::textSource);
    f_textedit->addAction(textsource);

    QMenu *menu = new QMenu(this);
    menu->addAction(removeAllFormat);
    menu->addAction(removeFormat);
    menu->addAction(textsource);
    f_menu->setMenu(menu);
    f_menu->setPopupMode(QToolButton::InstantPopup);

    // lists

    f_list_bullet->setShortcut(QKeySequence(QString::fromUtf8("CTRL+-")));
    f_list_ordered->setShortcut(QKeySequence(QString::fromUtf8("CTRL+=")));

    connect(f_list_bullet, &QToolButton::clicked, this, &MRichTextEdit::listBullet);
    connect(f_list_ordered, &QToolButton::clicked, this, &MRichTextEdit::listOrdered);

    // indentation

    f_indent_dec->setShortcut(QKeySequence(QString::fromUtf8("CTRL+, ")));
    f_indent_inc->setShortcut(QKeySequence(QString::fromUtf8("CTRL+.")));

    connect(f_indent_inc, &QToolButton::clicked, this, &MRichTextEdit::increaseIndentation);
    connect(f_indent_dec, &QToolButton::clicked, this, &MRichTextEdit::decreaseIndentation);

    // font size

    const auto sizes = QFontDatabase::standardSizes();
    for(int size: sizes) {
        f_fontsize->addItem(QString::number(size));
    }

    connect(f_fontsize, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        textSize(f_fontsize->itemText(index));
    });

    // text foreground color

    connect(f_fgcolor, &QToolButton::clicked, this, &MRichTextEdit::textFgColor);

    // text background color

    connect(f_bgcolor, &QToolButton::clicked, this, &MRichTextEdit::textBgColor);

    // images
    connect(f_image, &QToolButton::clicked, this, &MRichTextEdit::insertImage);

    //set initial font size when editing existing text
    if (!textIn.isEmpty()) {
        //insert existing text with cursor at beginning
        QTextCursor cursor = f_textedit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        cursor.insertHtml(textIn);
        cursor.movePosition(QTextCursor::Start);
        f_textedit->setTextCursor(cursor);

        //set current font size to match inserted text at cursor pos
        QTextCharFormat fmt = cursor.charFormat();
        double currSize = fmt.fontPointSize();
        int intSize = round(currSize);
        QString qsSize = QString::number(intSize);
        addFontSize(qsSize);
    } else {
        QTextCursor cursor = f_textedit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        f_textedit->setTextCursor(cursor);

        QTextCharFormat fmt = cursor.charFormat();
        fmt.setFontPointSize(getDefFontSizeNum());

        addFontSize(getDefFontSize());
    }

    updateFontSizeDisplay();
}


void MRichTextEdit::textSource() {
    QDialog *dialog = new QDialog(this);
    QPlainTextEdit *pte = new QPlainTextEdit(dialog);
    pte->setPlainText( f_textedit->toHtml() );
    QGridLayout *gl = new QGridLayout(dialog);
    gl->addWidget(pte, 0,0, 1,1);
    dialog->setWindowTitle(tr("Document source"));
    dialog->setMinimumWidth (400);
    dialog->setMinimumHeight(600);
    dialog->exec();

    f_textedit->setHtml(pte->toPlainText());

    delete dialog;
}


void MRichTextEdit::textRemoveFormat() {
    QTextCharFormat fmt;
    fmt.setFontWeight(QFont::Normal);
    fmt.setFontUnderline  (false);
    fmt.setFontStrikeOut  (false);
    fmt.setFontItalic     (false);
    fmt.setFontPointSize  (m_defFontSize);

    f_bold      ->setChecked(false);
    f_underline ->setChecked(false);
    f_italic    ->setChecked(false);
    f_strikeout ->setChecked(false);
    f_fontsize  ->setCurrentIndex(f_fontsize->findText(getDefFontSize()));

    fmt.clearBackground();

    mergeFormatOnWordOrSelection(fmt);
}


void MRichTextEdit::textRemoveAllFormat() {
    f_bold      ->setChecked(false);
    f_underline ->setChecked(false);
    f_italic    ->setChecked(false);
    f_strikeout ->setChecked(false);
    f_fontsize  ->setCurrentIndex(f_fontsize->findText(getDefFontSize()));
    QString text = f_textedit->toPlainText();
    f_textedit->setPlainText(text);
}


void MRichTextEdit::textBold() {
    QTextCharFormat fmt;
    fmt.setFontWeight(f_bold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}


void MRichTextEdit::focusInEvent(QFocusEvent *) {
    f_textedit->setFocus(Qt::TabFocusReason);
}

void MRichTextEdit::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Return && event->modifiers() == Qt::ControlModifier) {
        onSave();
        return;
    }

    QWidget::keyPressEvent(event);
}


void MRichTextEdit::textUnderline() {
    QTextCharFormat fmt;
    fmt.setFontUnderline(f_underline->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textItalic() {
    QTextCharFormat fmt;
    fmt.setFontItalic(f_italic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textStrikeout() {
    QTextCharFormat fmt;
    fmt.setFontStrikeOut(f_strikeout->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textSize(const QString &pointsAsString) {
//    qDebug() << "MRTE::textSize(" << p << ")";
    qreal pointSize = pointsAsString.toFloat();
    if (pointsAsString.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        mergeFormatOnWordOrSelection(fmt);
    }
}

void MRichTextEdit::textLink(bool checked) {
    bool unlink = false;
    QTextCharFormat fmt;
    if (checked) {
        QString url = f_textedit->currentCharFormat().anchorHref();
        bool ok;
        QString newUrl = QInputDialog::getText(this, tr("Create a link"),
                                        tr("Link URL:"), QLineEdit::Normal,
                                        url,
                                        &ok,
                                        Qt::MSWindowsFixedSizeDialogHint);
        if (ok) {
            fmt.setAnchor(true);
            fmt.setAnchorHref(newUrl);
            fmt.setForeground(QApplication::palette().color(QPalette::Link));
            fmt.setFontUnderline(true);
          } else {
            unlink = true;
            }
      } else {
        unlink = true;
        }
    if (unlink) {
        fmt.setAnchor(false);
        fmt.setForeground(QApplication::palette().color(QPalette::Text));
        fmt.setFontUnderline(false);
        }
    mergeFormatOnWordOrSelection(fmt);
}

void MRichTextEdit::textStyle(int index) {
    //TODO: would prefer select font vs paragraph style.
    QTextCursor cursor = f_textedit->textCursor();
    cursor.beginEditBlock();

    // standard
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::BlockUnderCursor);
        }
    QTextCharFormat fmt;
    cursor.setCharFormat(fmt);
    f_textedit->setCurrentCharFormat(fmt);

    if (index == ParagraphHeading1
            || index == ParagraphHeading2
            || index == ParagraphHeading3
            || index == ParagraphHeading4 ) {
        if (index == ParagraphHeading1) {
            fmt.setFontPointSize(m_fontsize_h1);
            }
        if (index == ParagraphHeading2) {
            fmt.setFontPointSize(m_fontsize_h2);
            }
        if (index == ParagraphHeading3) {
            fmt.setFontPointSize(m_fontsize_h3);
            }
        if (index == ParagraphHeading4) {
            fmt.setFontPointSize(m_fontsize_h4);
            }
        if (index == ParagraphHeading2 || index == ParagraphHeading4) {
            fmt.setFontItalic(true);
            }

        fmt.setFontWeight(QFont::Bold);
        }
    if (index == ParagraphMonospace) {
        fmt = cursor.charFormat();
        fmt.setFontFamilies(QStringList() << QString::fromUtf8("Monospace"));
        fmt.setFontStyleHint(QFont::Monospace);
        fmt.setFontFixedPitch(true);
        }
    cursor.setCharFormat(fmt);
    f_textedit->setCurrentCharFormat(fmt);

    cursor.endEditBlock();
}

void MRichTextEdit::textFgColor() {
    QColor col;
    if (Gui::DialogOptions::dontUseNativeColorDialog()){
        col = QColorDialog::getColor(f_textedit->textColor(), this, QLatin1String(""), QColorDialog::DontUseNativeDialog);
    } else {
        col = QColorDialog::getColor(f_textedit->textColor(), this);
    }
    QTextCursor cursor = f_textedit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
        }
    QTextCharFormat fmt = cursor.charFormat();
    if (col.isValid()) {
        fmt.setForeground(col);
      } else {
        fmt.clearForeground();
        }
    cursor.setCharFormat(fmt);
    f_textedit->setCurrentCharFormat(fmt);
    fgColorChanged(col);
}

void MRichTextEdit::textBgColor() {
    QColor col;
    if (Gui::DialogOptions::dontUseNativeColorDialog()){
        col = QColorDialog::getColor(f_textedit->textBackgroundColor(), this, QLatin1String(""), QColorDialog::DontUseNativeDialog);
    } else {
        col = QColorDialog::getColor(f_textedit->textBackgroundColor(), this);
    }
    QTextCursor cursor = f_textedit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
        }
    QTextCharFormat fmt = cursor.charFormat();
    if (col.isValid()) {
        fmt.setBackground(col);
      } else {
        fmt.clearBackground();
        }
    cursor.setCharFormat(fmt);
    f_textedit->setCurrentCharFormat(fmt);
    bgColorChanged(col);
}

void MRichTextEdit::listBullet(bool checked) {
    if (checked) {
        f_list_ordered->setChecked(false);
        }
    list(checked, QTextListFormat::ListDisc);
}

void MRichTextEdit::listOrdered(bool checked) {
    if (checked) {
        f_list_bullet->setChecked(false);
        }
    list(checked, QTextListFormat::ListDecimal);
}

void MRichTextEdit::list(bool checked, QTextListFormat::Style style) {
    QTextCursor cursor = f_textedit->textCursor();
    cursor.beginEditBlock();
    if (!checked) {
        QTextBlockFormat obfmt = cursor.blockFormat();
        QTextBlockFormat bfmt;
        bfmt.setIndent(obfmt.indent());
        cursor.setBlockFormat(bfmt);
    }
    else {
        QTextListFormat listFmt;
        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
            }
        listFmt.setStyle(style);
        cursor.createList(listFmt);
    }
    cursor.endEditBlock();
}

void MRichTextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format) {
    QTextCursor cursor = f_textedit->textCursor();
    if (!cursor.hasSelection()) {
        cursor.select(QTextCursor::WordUnderCursor);
        }
    cursor.mergeCharFormat(format);
    f_textedit->mergeCurrentCharFormat(format);
    f_textedit->setFocus(Qt::TabFocusReason);
}

void MRichTextEdit::slotCursorPositionChanged()
{
    updateFontSizeDisplay();

    //why do we change text style when selecting text?
    QTextCursor cursor = f_textedit->textCursor();
    if (cursor.hasSelection()) {                       //let selection logic handle this
        return;
    }

    QTextList *l = f_textedit->textCursor().currentList();

    if (m_lastBlockList &&
        (l == m_lastBlockList ||
        (l && m_lastBlockList && l->format().style() == m_lastBlockList->format().style()) ) ) {
        return;
    }
    m_lastBlockList = l;
    if (l) {
        QTextListFormat lfmt = l->format();
        if (lfmt.style() == QTextListFormat::ListDisc) {
            f_list_bullet->setChecked(true);
            f_list_ordered->setChecked(false);
          } else if (lfmt.style() == QTextListFormat::ListDecimal) {
            f_list_bullet->setChecked(false);
            f_list_ordered->setChecked(true);
          } else {
            f_list_bullet->setChecked(false);
            f_list_ordered->setChecked(false);
            }
      } else {
        f_list_bullet->setChecked(false);
        f_list_ordered->setChecked(false);
        }
}

void MRichTextEdit::fontChanged(const QFont &font) {
    //TODO: change this to real font selector
    f_fontsize->setCurrentIndex(f_fontsize->findText(QString::number(font.pointSize())));
    f_bold->setChecked(font.bold());
    f_italic->setChecked(font.italic());
    f_underline->setChecked(font.underline());
    f_strikeout->setChecked(font.strikeOut());
    if (font.pointSize() == m_fontsize_h1) {
        f_paragraph->setCurrentIndex(ParagraphHeading1);
      } else if (font.pointSize() == m_fontsize_h2) {
        f_paragraph->setCurrentIndex(ParagraphHeading2);
      } else if (font.pointSize() == m_fontsize_h3) {
        f_paragraph->setCurrentIndex(ParagraphHeading3);
      } else if (font.pointSize() == m_fontsize_h4) {
        f_paragraph->setCurrentIndex(ParagraphHeading4);
      } else {
        if (font.fixedPitch() && font.family() == QString::fromUtf8("Monospace")) {
            f_paragraph->setCurrentIndex(ParagraphMonospace);
          } else {
            f_paragraph->setCurrentIndex(ParagraphStandard);
            }
        }
    if (f_textedit->textCursor().currentList()) {
        QTextListFormat lfmt = f_textedit->textCursor().currentList()->format();
        if (lfmt.style() == QTextListFormat::ListDisc) {
            f_list_bullet->setChecked(true);
            f_list_ordered->setChecked(false);
          } else if (lfmt.style() == QTextListFormat::ListDecimal) {
            f_list_bullet->setChecked(false);
            f_list_ordered->setChecked(true);
          } else {
            f_list_bullet->setChecked(false);
            f_list_ordered->setChecked(false);
            }
      } else {
        f_list_bullet->setChecked(false);
        f_list_ordered->setChecked(false);
      }
}

void updateColorButtonIcon(QToolButton* button, const QColor& color)
{
    if (!button) {
        return;
    }

    QIcon icon = button->icon();
    QList<QSize> availableSizes = icon.availableSizes();
    if (availableSizes.isEmpty()) {
        return;
    }

    // Use the largest available size for the best quality
    QSize actualIconSize = availableSizes.last();
    QPixmap originalPixmap = icon.pixmap(actualIconSize);

    // Create a new pixmap to be filled with the color
    QPixmap coloredPixmap(originalPixmap.size());
    coloredPixmap.setDevicePixelRatio(originalPixmap.devicePixelRatio());

    if (color.isValid()) {
        coloredPixmap.fill(color);
        // Apply the original icon's transparency mask to the colored pixmap
        coloredPixmap.setMask(originalPixmap.mask());
        button->setIcon(QIcon(coloredPixmap));
    }
}

void MRichTextEdit::fgColorChanged(const QColor& color)
{
    updateColorButtonIcon(f_fgcolor, color);
}

void MRichTextEdit::bgColorChanged(const QColor& color)
{
    updateColorButtonIcon(f_bgcolor, color);
}

void MRichTextEdit::slotCurrentCharFormatChanged(const QTextCharFormat &format) {
    Q_UNUSED(format);
}

void MRichTextEdit::slotClipboardDataChanged() {
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        f_paste->setEnabled(md->hasText());
#endif
}

QString MRichTextEdit::toHtml() const {
    QString s = f_textedit->toHtml();
    // convert emails to links
    s = s.replace(QRegularExpression(
                QString::fromUtf8("(<[^a][^>]+>(?:<span[^>]+>)?|\\s)([a-zA-Z\\d]+@[a-zA-Z\\d]+\\.[a-zA-Z]+)")),
                QString::fromUtf8("\\1<a href=\"mailto:\\2\">\\2</a>"));
    // convert links
    s = s.replace(QRegularExpression(
                QString::fromUtf8("(<[^a][^>]+>(?:<span[^>]+>)?|\\s)((?:https?|ftp|file)://[^\\s'\"<>]+)")),
                QString::fromUtf8( "\\1<a href=\"\\2\">\\2</a>"));
    // see also: Utils::linkify()
    return s;
}

void MRichTextEdit::increaseIndentation() {
    indent(+1);
}

void MRichTextEdit::decreaseIndentation() {
    indent(-1);
}

void MRichTextEdit::indent(int delta) {
    QTextCursor cursor = f_textedit->textCursor();
    cursor.beginEditBlock();
    QTextBlockFormat bfmt = cursor.blockFormat();
    int ind = bfmt.indent();
    if (ind + delta >= 0) {
        bfmt.setIndent(ind + delta);
        }
    cursor.setBlockFormat(bfmt);
    cursor.endEditBlock();
}

void MRichTextEdit::setText(const QString& text) {
    if (text.isEmpty()) {
        setPlainText(text);
        return;
        }
    if (text[0] == QChar::fromLatin1('<')) {
        setHtml(text);
      } else {
        setPlainText(text);
        }
}

void MRichTextEdit::insertImage() {
    QSettings s;
    QString attdir = s.value(QString::fromUtf8("general/filedialog-path")).toString();
    QString file = QFileDialog::getOpenFileName(this,
                                    tr("Select an image"),
                                    attdir,
                                    tr("JPEG (*.jpg);; GIF (*.gif);; PNG (*.png);; BMP (*.bmp);; All (*)"));
    QImage image = QImageReader(file).read();

    f_textedit->dropImage(image,
                QFileInfo(file).suffix().toUpper());
}

void MRichTextEdit::onSave()
{
    QString result = toHtml();
    Q_EMIT saveText(result);
}

void MRichTextEdit::onExit()
{
    Q_EMIT editorFinished();
}

void MRichTextEdit::updateFontSizeDisplay()
{
    if (hasMultipleSizes()) {
        f_fontsize->setEditText(QString());
        f_fontsize->setCurrentIndex(-1);
    }
    else {
        QTextCursor cursor = f_textedit->textCursor();
        QTextCharFormat fmt = cursor.charFormat();
        double currSize = fmt.fontPointSize();
        f_fontsize->setEditText(QString::number(currSize, 'g', 4));
    }
}

void MRichTextEdit::onSelectionChanged()
{
    updateFontSizeDisplay();
}

//does selection have multiple sizes?
bool MRichTextEdit::hasMultipleSizes()
{
    QTextCursor cursor = f_textedit->textCursor();
    if (cursor.hasSelection()) {
        int begin = cursor.selectionStart();
        int end   = cursor.selectionEnd();
        int currPos;
        std::vector<QString> foundSizes;
        std::map<QString, int> countMap;
        for (currPos = begin; currPos < end; currPos++) {
            cursor.setPosition(currPos);
            QTextCharFormat fmt = cursor.charFormat();
            double currSize = fmt.fontPointSize();
            QString asQS = QString::number(currSize, 'f', 2);
            foundSizes.push_back(asQS);
            auto ret = countMap.insert(std::pair<QString, int>(asQS, 1));
            if (!ret.second) {            //already have this size
                ret.first->second++;      //bump count
            }
        }
        if (countMap.size() > 1) {
            return true;
        }
    }
    return false;
}

void MRichTextEdit::setDefFontSize(int fontSize)
{
    m_defFontSize = fontSize;
    m_fontsize_h1 = fontSize + 8;
    m_fontsize_h2 = fontSize + 6;
    m_fontsize_h3 = fontSize + 4;
    m_fontsize_h4 = fontSize + 2;

    QString newSize = QString::number(fontSize);
    f_fontsize->findText(newSize);
    int idx = f_fontsize->findText(newSize);
    if (idx > -1) {
        f_fontsize->setCurrentIndex(idx);
    } else {
        f_fontsize->setCurrentIndex(0);
    }
    textSize(newSize);
}

int MRichTextEdit::getDefFontSizeNum()
{
    double fontSize = TechDraw::Preferences::dimFontSizeMM();

    //this conversion is only approximate. the factor changes for different fonts.
//    double mmToPts = 2.83;  //theoretical value
    double mmToPts = 2.00;  //practical value. seems to be reasonable for common fonts.

    return round(fontSize * mmToPts);
}

QString MRichTextEdit::getDefFontSize()
{
    return QString::number(getDefFontSizeNum());
}

//not used.
void MRichTextEdit::setDefFont(QString fontName)
{
    m_defFont = fontName;
}

QFont MRichTextEdit::getDefFont()
{
    QString family = QString::fromStdString(Preferences::labelFont());
    m_defFont = family;
    QFont result;
    result.setFamily(family);
    return result;
}

// add a new fontSize to the list
// this seems like massive overkill for integer point<->mm conversion factor
// if the conversion factor is float, will generate non-standard sizes
void MRichTextEdit::addFontSize(QString fontSize)
{
    bool ok;
    const double newSize = fontSize.toDouble(&ok);
    if (!ok) {
        return;  // Ignore invalid numbers
    }

    // 1. Collect all existing sizes as doubles
    QList<double> sizes;
    for (int i = 0; i < f_fontsize->count(); ++i) {
        sizes.append(f_fontsize->itemText(i).toDouble());
    }

    // 2. Check if the new size is already in the list (using fuzzy comparison for doubles)
    for (double existingSize : qAsConst(sizes)) {
        if (qFuzzyCompare(existingSize, newSize)) {
            // Already exists, just make sure it's the current text
            f_fontsize->setCurrentText(QString::number(newSize, 'g', 4));
            return;
        }
    }

    // 3. Add the new size and re-sort
    sizes.append(newSize);
    std::sort(sizes.begin(), sizes.end());

    // 4. Repopulate the combobox with the sorted, correctly formatted list
    QStringList newList;
    for (double size : qAsConst(sizes)) {
        newList << QString::number(size, 'g', 4);
    }

    const QString currentText = f_fontsize->currentText();  // Save current text
    f_fontsize->blockSignals(true);
    f_fontsize->clear();
    f_fontsize->addItems(newList);
    f_fontsize->setCurrentText(currentText);  // Restore current text
    f_fontsize->blockSignals(false);

    // 5. Set the new size as the current item
    f_fontsize->setCurrentText(fontSize);
}

void MRichTextEdit::setMinimalMode(bool on)
{
    f_save->setVisible(!on);
    f_exit->setVisible(!on);
    f_cut->setVisible(!on);
    f_copy->setVisible(!on);
    f_paste->setVisible(!on);
}

#include <Mod/TechDraw/Gui/moc_mrichtextedit.cpp>

