/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <werner.wm.mayer@gmx.de>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "regexpdialog.h"
#include "ui_regexpdialog.h"

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qtextedit.h>

RegExpDialog::RegExpDialog(QWidget* parent)
  : QDialog(parent), ui(new Ui_RegExpDialog())
{
    ui->setupUi(this);
    rxhilighter = new RegExpSyntaxHighlighter(ui->textEdit1);

    connect(ui->lineEdit1, SIGNAL(textChanged(const QString &)),
            this, SLOT(performRegExp()));
}

RegExpDialog::~RegExpDialog()
{
    delete ui;
}

void RegExpDialog::performRegExp()
{
    QString txt = ui->lineEdit1->text();
    if (txt.isEmpty()) {
        rxhilighter->resethighlight();
        return;
    }

    QRegExp rx(txt);
    rx.setCaseSensitivity(ui->checkBox1->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    rx.setPatternSyntax(ui->checkBox2->isChecked() ? QRegExp::Wildcard : QRegExp::RegExp);
    rx.setMinimal(ui->checkBox3->isChecked());

    // evaluate regular expression
    ui->textLabel4->setText(rx.errorString());
    if (!rx.isValid()) {
        rxhilighter->resethighlight();
        return; // invalid expression
    }

    rxhilighter->highlightMatchedText(rx);
}

void RegExpDialog::about()
{
    QString msg = "This is a tool for playing around with regular expressions.";
    QMessageBox::information(this, "RegExp Explorer", msg);
}

// -------------------------------------------------------------

RegExpSyntaxHighlighter::RegExpSyntaxHighlighter (QTextEdit * textEdit)
  : QSyntaxHighlighter(textEdit)
{
}

RegExpSyntaxHighlighter::~RegExpSyntaxHighlighter()
{
}

void RegExpSyntaxHighlighter::highlightBlock (const QString & text)
{
    QTextCharFormat regFormat;
    regFormat.setForeground(Qt::black);
    regFormat.setFontWeight(QFont::Normal);
    setFormat(0, text.length(), regFormat);

    if (regexp.isEmpty())
        return; // empty regular expression

    int pos = 0;
    int last = -1;
    regFormat.setFontWeight(QFont::Bold);
    regFormat.setForeground(Qt::blue);

    while ((pos = regexp.indexIn(text, pos)) != -1) {
        if (last == pos)
            break;
        QString sub = text.mid(pos, regexp.matchedLength());
        if (!sub.isEmpty()) {
            setFormat(pos, sub.length(), regFormat);
        }

        pos += regexp.matchedLength();
        last=pos;
    }
}
#if 0
int RegExpSyntaxHighlighter::highlightParagraph ( const QString & text, int /*endStateOfLastPara*/ )
{
    // reset format
    QFont font = textEdit()->font();
    font.setBold( false );
    setFormat(0, text.length(), font, Qt::black );

    if (regexp.isEmpty())
        return 0; // empty regular expression

    int pos = 0;
    int last = -1;
    font.setBold(true);

    while ( (pos = regexp.indexIn(text, pos)) != -1 ) {
        if (last == pos)
            break;
        QString sub = text.mid( pos, regexp.matchedLength());
        if (!sub.isEmpty()) {
            setFormat(pos, sub.length(), font, Qt::blue );
        }

        pos += regexp.matchedLength();
        last=pos;
    }

    return 0;
}
#endif
void RegExpSyntaxHighlighter::highlightMatchedText(const QRegExp& rx)
{
    regexp = rx;
    rehighlight();
}

void RegExpSyntaxHighlighter::resethighlight()
{
    regexp.setPattern("");
    rehighlight();
}
