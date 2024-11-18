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
#include <qvalidator.h>

RegExpDialog::RegExpDialog(QWidget* parent)
    : QDialog(parent)
    , ui(new Ui_RegExpDialog())
{
    ui->setupUi(this);
    rxhilighter = new RegExpSyntaxHighlighter(ui->textEdit1);

    validator = new QRegularExpressionValidator(this);
    ui->lineEdit->setValidator(validator);

    connect(ui->lineEditRegExp, &QLineEdit::textChanged, this, &RegExpDialog::performRegExp);
    connect(ui->caseInsensitiveOption, &QCheckBox::toggled, this, &RegExpDialog::performRegExp);
    connect(ui->invertedGreedinessOption, &QCheckBox::toggled, this, &RegExpDialog::performRegExp);
    connect(ui->dotMatchesEverythingOption,
            &QCheckBox::toggled,
            this,
            &RegExpDialog::performRegExp);
    connect(ui->multilineOption, &QCheckBox::toggled, this, &RegExpDialog::performRegExp);
    connect(ui->extendedPatternSyntaxOption,
            &QCheckBox::toggled,
            this,
            &RegExpDialog::performRegExp);
    connect(ui->dontCaptureOption, &QCheckBox::toggled, this, &RegExpDialog::performRegExp);
    connect(ui->useUnicodePropertiesOption,
            &QCheckBox::toggled,
            this,
            &RegExpDialog::performRegExp);
}

RegExpDialog::~RegExpDialog()
{
    delete ui;
}

void RegExpDialog::performRegExp()
{
    QString txt = ui->lineEditRegExp->text();
    if (txt.isEmpty()) {
        rxhilighter->resethighlight();
        return;
    }

    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (ui->caseInsensitiveOption->isChecked()) {
        options |= QRegularExpression::CaseInsensitiveOption;
    }

    if (ui->invertedGreedinessOption->isChecked()) {
        options |= QRegularExpression::InvertedGreedinessOption;
    }

    if (ui->dotMatchesEverythingOption->isChecked()) {
        options |= QRegularExpression::DotMatchesEverythingOption;
    }

    if (ui->multilineOption->isChecked()) {
        options |= QRegularExpression::MultilineOption;
    }

    if (ui->extendedPatternSyntaxOption->isChecked()) {
        options |= QRegularExpression::ExtendedPatternSyntaxOption;
    }

    if (ui->dontCaptureOption->isChecked()) {
        options |= QRegularExpression::DontCaptureOption;
    }

    if (ui->useUnicodePropertiesOption->isChecked()) {
        options |= QRegularExpression::UseUnicodePropertiesOption;
    }

    QRegularExpression rx(txt, options);

    // evaluate regular expression
    ui->textLabel4->setText(rx.errorString());
    if (!rx.isValid()) {
        rxhilighter->resethighlight();
        return;  // invalid expression
    }

    rxhilighter->highlightMatchedText(rx);
    validator->setRegularExpression(rx);
}

void RegExpDialog::about()
{
    QString msg = "This is a tool for playing around with regular expressions.";
    QMessageBox::information(this, "RegExp Explorer", msg);
}

// -------------------------------------------------------------

RegExpSyntaxHighlighter::RegExpSyntaxHighlighter(QTextEdit* textEdit)
    : QSyntaxHighlighter(textEdit)
{}

RegExpSyntaxHighlighter::~RegExpSyntaxHighlighter()
{}

void RegExpSyntaxHighlighter::highlightBlock(const QString& text)
{
    QTextCharFormat regFormat;
    regFormat.setForeground(Qt::black);
    regFormat.setFontWeight(QFont::Normal);
    setFormat(0, text.length(), regFormat);

    if (regexp.pattern().isEmpty()) {
        return;  // empty regular expression
    }

    int pos = 0;
    int last = -1;
    regFormat.setFontWeight(QFont::Bold);
    regFormat.setForeground(Qt::blue);

    QRegularExpressionMatch match;
    while ((pos = text.indexOf(regexp, pos, &match)) != -1) {
        if (last == pos) {
            break;
        }
        QString sub = text.mid(pos, match.capturedLength());
        if (!sub.isEmpty()) {
            setFormat(pos, sub.length(), regFormat);
        }

        pos += match.capturedLength();
        last = pos;
    }
}

void RegExpSyntaxHighlighter::highlightMatchedText(const QRegularExpression& rx)
{
    regexp = rx;
    rehighlight();
}

void RegExpSyntaxHighlighter::resethighlight()
{
    regexp.setPattern("");
    rehighlight();
}
