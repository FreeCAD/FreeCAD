/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <werner.wm.mayer@gmx.de>              *
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


#ifndef REG_EXP_DIALOG_H
#define REG_EXP_DIALOG_H

#include <qdialog.h>
#include <qregexp.h>
#include <qsyntaxhighlighter.h>

class RegExpSyntaxHighlighter;
class Ui_RegExpDialog;
class RegExpDialog : public QDialog
{
    Q_OBJECT
public:
    RegExpDialog(QWidget* parent = 0);
    ~RegExpDialog();

    void about();

protected Q_SLOTS:
    void performRegExp();

private:
    RegExpSyntaxHighlighter* rxhilighter;
    Ui_RegExpDialog* ui;
};

// -------------------------------------------------------------

class RegExpSyntaxHighlighter : public QSyntaxHighlighter
{
public:
    RegExpSyntaxHighlighter (QTextEdit * textEdit);
    ~RegExpSyntaxHighlighter();

    void highlightBlock (const QString & text);
    //int highlightParagraph ( const QString & text, int endStateOfLastPara );
    void highlightMatchedText( const QRegExp& );
    void resethighlight();

private:
    QRegExp regexp;
};

#endif // REG_EXP_DIALOG_H
