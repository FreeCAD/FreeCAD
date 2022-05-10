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
 * for FreeCAD project https://www.freecadweb.org/
 ********************************/
#include "PreCompiled.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QFontDatabase>
#include <QInputDialog>
#include <QColorDialog>
#include <QTextList>
#include <QtDebug>
#include <QFileDialog>
#include <QImageReader>
#include <QSettings>
#include <QBuffer>
#include <QUrl>
#include <QPlainTextEdit>
#include <QMenu>
#include <QDialog>
#include <QBitmap>

#include <iostream>
#include <algorithm>

#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>
#include <Gui/FileDialog.h>

#include <App/Application.h>
#include "PreferencesGui.h"
#include "mrichtextedit.h"

using namespace TechDrawGui;
using namespace TechDraw;

MRichTextEdit::MRichTextEdit(QWidget *parent, QString textIn) : QWidget(parent) {
    setupUi(this);
    m_lastBlockList = nullptr;
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    f_textedit->setTabStopWidth(40);
#else
    f_textedit->setTabStopDistance(40);
#endif
//    setDefFontSize(getDefFontSizeNum());
    setDefFontSize(TechDrawGui::PreferencesGui::labelFontSizePX());
    m_defFont = getDefFont().family();
    f_textedit->setFont(getDefFont());

    connect(f_save, SIGNAL(clicked()),
            this,     SLOT(onSave()));
    connect(f_exit, SIGNAL(clicked()),
            this,     SLOT(onExit()));

    connect(f_textedit, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
            this,     SLOT(slotCurrentCharFormatChanged(QTextCharFormat)));
    connect(f_textedit, SIGNAL(cursorPositionChanged()),
            this,     SLOT(slotCursorPositionChanged()));
    connect(f_textedit, SIGNAL(selectionChanged()),
            this,     SLOT(onSelectionChanged()));


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

    connect(f_paragraph, SIGNAL(activated(int)),
            this, SLOT(textStyle(int)));

    // undo & redo

    f_undo->setShortcut(QKeySequence::Undo);
    f_redo->setShortcut(QKeySequence::Redo);

    connect(f_textedit->document(), SIGNAL(undoAvailable(bool)),
            f_undo, SLOT(setEnabled(bool)));
    connect(f_textedit->document(), SIGNAL(redoAvailable(bool)),
            f_redo, SLOT(setEnabled(bool)));

    f_undo->setEnabled(f_textedit->document()->isUndoAvailable());
    f_redo->setEnabled(f_textedit->document()->isRedoAvailable());

    connect(f_undo, SIGNAL(clicked()), f_textedit, SLOT(undo()));
    connect(f_redo, SIGNAL(clicked()), f_textedit, SLOT(redo()));

    // cut, copy & paste

    f_cut->setShortcut(QKeySequence::Cut);
    f_copy->setShortcut(QKeySequence::Copy);
    f_paste->setShortcut(QKeySequence::Paste);

    f_cut->setEnabled(false);
    f_copy->setEnabled(false);

    connect(f_cut, SIGNAL(clicked()), f_textedit, SLOT(cut()));
    connect(f_copy, SIGNAL(clicked()), f_textedit, SLOT(copy()));
    connect(f_paste, SIGNAL(clicked()), f_textedit, SLOT(paste()));

    connect(f_textedit, SIGNAL(copyAvailable(bool)), f_cut, SLOT(setEnabled(bool)));
    connect(f_textedit, SIGNAL(copyAvailable(bool)), f_copy, SLOT(setEnabled(bool)));

#ifndef QT_NO_CLIPBOARD
    connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()));
#endif

    // link

    f_link->setShortcut(QKeySequence(QString::fromUtf8("CTRL+L")));

    connect(f_link, SIGNAL(clicked(bool)), this, SLOT(textLink(bool)));

    // bold, italic & underline

    f_bold->setShortcut(QKeySequence(QString::fromUtf8("CTRL+B")));
    f_italic->setShortcut(QKeySequence(QString::fromUtf8("CTRL+I")));
    f_underline->setShortcut(QKeySequence(QString::fromUtf8("CTRL+U")));

    connect(f_bold, SIGNAL(clicked()), this, SLOT(textBold()));
    connect(f_italic, SIGNAL(clicked()), this, SLOT(textItalic()));
    connect(f_underline, SIGNAL(clicked()), this, SLOT(textUnderline()));
    connect(f_strikeout, SIGNAL(clicked()), this, SLOT(textStrikeout()));

    QAction *removeFormat = new QAction(tr("Remove character formatting"), this);
    removeFormat->setShortcut(QKeySequence(QString::fromUtf8("CTRL+M")));
    connect(removeFormat, SIGNAL(triggered()), this, SLOT(textRemoveFormat()));
    f_textedit->addAction(removeFormat);

    QAction *removeAllFormat = new QAction(tr("Remove all formatting"), this);
    connect(removeAllFormat, SIGNAL(triggered()), this, SLOT(textRemoveAllFormat()));
    f_textedit->addAction(removeAllFormat);

    QAction *textsource = new QAction(tr("Edit document source"), this);
    textsource->setShortcut(QKeySequence(QString::fromUtf8("CTRL+O")));
    connect(textsource, SIGNAL(triggered()), this, SLOT(textSource()));
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

    connect(f_list_bullet, SIGNAL(clicked(bool)), this, SLOT(listBullet(bool)));
    connect(f_list_ordered, SIGNAL(clicked(bool)), this, SLOT(listOrdered(bool)));

    // indentation

    f_indent_dec->setShortcut(QKeySequence(QString::fromUtf8("CTRL+,")));
    f_indent_inc->setShortcut(QKeySequence(QString::fromUtf8("CTRL+.")));

    connect(f_indent_inc, SIGNAL(clicked()), this, SLOT(increaseIndentation()));
    connect(f_indent_dec, SIGNAL(clicked()), this, SLOT(decreaseIndentation()));

    // font size

    QFontDatabase db;
    for(int size: db.standardSizes()) {
        f_fontsize->addItem(QString::number(size));
    }
    //TODO: void QComboBox::setEditText(const QString &text) to " " when multiple select
    connect(f_fontsize, SIGNAL(currentIndexChanged(QString)),
            this, SLOT(textSize(QString)));

    // text foreground color

    connect(f_fgcolor, SIGNAL(clicked()), this, SLOT(textFgColor()));

    // text background color

    connect(f_bgcolor, SIGNAL(clicked()), this, SLOT(textBgColor()));

    // images
    connect(f_image, SIGNAL(clicked()), this, SLOT(insertImage()));
    
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
        int fSize = f_fontsize->findText(qsSize);
        f_fontsize  ->setCurrentIndex(fSize);
    } else {
        QTextCursor cursor = f_textedit->textCursor();
        cursor.movePosition(QTextCursor::Start);
        f_textedit->setTextCursor(cursor);

        QTextCharFormat fmt = cursor.charFormat();
        fmt.setFontPointSize(getDefFontSizeNum());

        addFontSize(getDefFontSize());

        f_fontsize->setCurrentIndex(f_fontsize->findText(getDefFontSize()));
    }
       
}


void MRichTextEdit::textSource() {
    QDialog *dialog = new QDialog(this);
    QPlainTextEdit *pte = new QPlainTextEdit(dialog);
    pte->setPlainText( f_textedit->toHtml() );
    QGridLayout *gl = new QGridLayout(dialog);
    gl->addWidget(pte,0,0,1,1);
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
//  fmt.setFontFamily     ("Helvetica");
//  fmt.setFontStyleHint  (QFont::SansSerif);
//  fmt.setFontFixedPitch (true);

    f_bold      ->setChecked(false);
    f_underline ->setChecked(false);
    f_italic    ->setChecked(false);
    f_strikeout ->setChecked(false);
    f_fontsize  ->setCurrentIndex(f_fontsize->findText(getDefFontSize()));

//  QTextBlockFormat bfmt = cursor.blockFormat();
//  bfmt->setIndent(0);

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

void MRichTextEdit::textSize(const QString &p) {
//    qDebug() << "MRTE::textSize(" << p << ")";
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
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
        fmt.setFontFamily(QString::fromUtf8("Monospace"));
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
        col = QColorDialog::getColor(f_textedit->textColor(),this, QLatin1String(""), QColorDialog::DontUseNativeDialog);
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
        col = QColorDialog::getColor(f_textedit->textBackgroundColor(),this, QLatin1String(""), QColorDialog::DontUseNativeDialog);
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
      } else {
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

void MRichTextEdit::slotCursorPositionChanged() {
//    qDebug() << "MRTE::slotCursorPositionChanged()";
    //why do we change text style when selecting text?
    QTextCursor cursor = f_textedit->textCursor();
    if (cursor.hasSelection()) {                       //let selection logic handle this
        return;
    }
    //set font size widget to match curr char size
    QTextCharFormat fmt = cursor.charFormat();
    double currSize = fmt.fontPointSize();
    int fSize = f_fontsize->findText(QString::number(currSize));
    f_fontsize  ->setCurrentIndex(fSize);

    QTextList *l = f_textedit->textCursor().currentList();

    if (m_lastBlockList && 
        (l == m_lastBlockList || 
        (l != nullptr && m_lastBlockList != nullptr && l->format().style() == m_lastBlockList->format().style()) ) ) {
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

void MRichTextEdit::fontChanged(const QFont &f) {
//    qDebug() << "MRTE::fontChanged()";
    //TODO: change this to real font selector
    f_fontsize->setCurrentIndex(f_fontsize->findText(QString::number(f.pointSize())));
    f_bold->setChecked(f.bold());
    f_italic->setChecked(f.italic());
    f_underline->setChecked(f.underline());
    f_strikeout->setChecked(f.strikeOut());
    if (f.pointSize() == m_fontsize_h1) {
        f_paragraph->setCurrentIndex(ParagraphHeading1);
      } else if (f.pointSize() == m_fontsize_h2) {
        f_paragraph->setCurrentIndex(ParagraphHeading2);
      } else if (f.pointSize() == m_fontsize_h3) {
        f_paragraph->setCurrentIndex(ParagraphHeading3);
      } else if (f.pointSize() == m_fontsize_h4) {
        f_paragraph->setCurrentIndex(ParagraphHeading4);
      } else {
        if (f.fixedPitch() && f.family() == QString::fromUtf8("Monospace")) {
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

void MRichTextEdit::fgColorChanged(const QColor &c) {
    QSize iconSize(16,16);
    QIcon fgIcon = f_fgcolor->icon();
    QPixmap fgPix = fgIcon.pixmap(iconSize,QIcon::Mode::Normal, QIcon::State::On);
    QPixmap filler(iconSize);
    if (c.isValid() ) {
        filler.fill(c);
        filler.setMask(fgPix.createMaskFromColor(Qt::transparent, Qt::MaskInColor) );
        f_fgcolor->setIcon(filler);
    }
}

void MRichTextEdit::bgColorChanged(const QColor &c) {
    QSize iconSize(16,16);
    QIcon bgIcon = f_bgcolor->icon();
    QPixmap bgPix = bgIcon.pixmap(iconSize,QIcon::Mode::Normal, QIcon::State::On);
    QPixmap filler(iconSize);
    if (c.isValid() ) {
        filler.fill(c);
        filler.setMask(bgPix.createMaskFromColor(Qt::transparent, Qt::MaskOutColor) );
        f_bgcolor->setIcon(filler);
    }
}

void MRichTextEdit::slotCurrentCharFormatChanged(const QTextCharFormat &format) {
//    qDebug() << "MRTE::slotCurrentCharFormatChanged()";
    Q_UNUSED(format);
    //again, why do we do all this just because the cursor moved?
    //can it be this simple???
//    fontChanged(format.font());
//    bgColorChanged((format.background().isOpaque()) ? format.background().color() : QColor());
//    fgColorChanged((format.foreground().isOpaque()) ? format.foreground().color() : QColor());
//    f_link->setChecked(format.isAnchor());
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
    s = s.replace(QRegExp(
                QString::fromUtf8("(<[^a][^>]+>(?:<span[^>]+>)?|\\s)([a-zA-Z\\d]+@[a-zA-Z\\d]+\\.[a-zA-Z]+)")),
                QString::fromUtf8("\\1<a href=\"mailto:\\2\">\\2</a>"));
    // convert links
    s = s.replace(QRegExp(
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

void MRichTextEdit::onSave(void)
{
    QString result = toHtml();
    Q_EMIT saveText(result);
}

void MRichTextEdit::onExit(void)
{
    Q_EMIT editorFinished();
}

void MRichTextEdit::onSelectionChanged(void)
{
//    qDebug() << "MRTE::onSelectionChanged()";
    if (hasMultipleSizes()) {
        f_fontsize->setEditText(QString());
        f_fontsize->setCurrentIndex(-1);
    } else {
        QTextCursor cursor = f_textedit->textCursor();
        QTextCharFormat fmt = cursor.charFormat();
        double currSize = fmt.fontPointSize();
        int fSize = f_fontsize->findText(QString::number(currSize));
        f_fontsize  ->setCurrentIndex(fSize);
    }
}

//does selection have multiple sizes?
bool MRichTextEdit::hasMultipleSizes(void)
{
//    qDebug() << "MRTE::hasMultipleSizes()";
    bool result = false;
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
            QString asQS = QString::number(currSize,'f',2);
            foundSizes.push_back(asQS);
            auto ret = countMap.insert(std::pair<QString, int>(asQS, 1));
            if (ret.second == false) {            //already have this size
                ret.first->second++;              //bump count
            }
        }
        if (countMap.size() > 1) {
            result = true;
        }
    }
    return result;  
} 

void MRichTextEdit::setDefFontSize(int fs)
{
//    Base::Console().Message("MRTE::setDefFontSize(%d)\n", fs);
    m_defFontSize = fs;
    m_fontsize_h1 = fs + 8;
    m_fontsize_h2 = fs + 6;
    m_fontsize_h3 = fs + 4;
    m_fontsize_h4 = fs + 2;

    QString newSize = QString::number(fs);
    f_fontsize->findText(newSize);
    int idx = f_fontsize->findText(newSize);
    if (idx > -1) {
        f_fontsize->setCurrentIndex(idx);
    } else {
        f_fontsize->setCurrentIndex(0);
    }
    textSize(newSize);
}

int MRichTextEdit::getDefFontSizeNum(void)
{
//    Base::Console().Message("MRTE::getDefFontSizeNum()\n");
    double fontSize = TechDraw::Preferences::dimFontSizeMM();

    //this conversion is only approximate. the factor changes for different fonts.
//    double mmToPts = 2.83;  //theoretical value
    double mmToPts = 2.00;  //practical value. seems to be reasonable for common fonts.
    
    int ptsSize = round(fontSize * mmToPts);
    return ptsSize;
}

QString MRichTextEdit::getDefFontSize(void)
{
//    Base::Console().Message("MRTE::getDefFontSize()\n");
    QString result = QString::number(getDefFontSizeNum());
    return result;
}

//not used.
void MRichTextEdit::setDefFont(QString f)
{
    m_defFont = f;
}

QFont MRichTextEdit::getDefFont(void)
{
    QString family = Base::Tools::fromStdString(Preferences::labelFont());
    m_defFont = family;
    QFont result;
    result.setFamily(family);
    return result;
}

// add a new fontSize to the list
// this seems like massive overkill for integer point<->mm conversion factor
// if the conversion factor is float, will generate non-standard sizes 
void MRichTextEdit::addFontSize(QString fs)
{
//    Base::Console().Message("MRTE::addFontSize(%s)\n", qPrintable(fs));
    QStringList newList;
    int fsVal = fs.toInt();
    int size = f_fontsize->count();
    bool added = false;
    for (int i = 0; i < size; i++) {
        QString item = f_fontsize->itemText(i);
        int itemVal = item.toInt();
        if (added) {
            newList << item;
            continue;
        }
        if (itemVal < fsVal) {
            newList << item;
        } else if (itemVal == fsVal) {  // already in list
            newList << item;
            added = true;
        } else {             //item > fs, add both
            newList << fs;
            newList << item; 
            added = true;
        }
    }
    if (!added) {   //bigger than all choices, add to end of list
        newList << fs;
    }
    f_fontsize->clear();
    f_fontsize->addItems(newList);
}

#include <Mod/TechDraw/Gui/moc_mrichtextedit.cpp>

