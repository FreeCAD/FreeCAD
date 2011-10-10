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

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qmessagebox.h>
#include <qtextedit.h>

RegExpDialog::RegExpDialog( QWidget* parent, const char* name, bool modal, WFlags f )
	: RegExpDialogBase( parent, name, modal, f )
{
  rxhilighter = new RegExpSyntaxHighlighter( textEdit1 );
}

RegExpDialog::~RegExpDialog()
{
}

void RegExpDialog::performRegExp()
{
  QString txt = lineEdit1->text();
  if ( txt.isEmpty() )
  {
    rxhilighter->resethighlight();
    return;
  }

  QRegExp rx(txt);
  rx.setCaseSensitive( checkBox1->isChecked() );
  rx.setWildcard( checkBox2->isChecked() );
  rx.setMinimal( checkBox3->isChecked() );

  // evaluate regular expression
  textLabel4->setText( rx.errorString() );
  if ( !rx.isValid() )
  {
    rxhilighter->resethighlight();
    return; // invalid expression
  }

  rxhilighter->highlightMatchedText( rx );
}

void RegExpDialog::about()
{
  QString msg = "This is a tool for playing around with regular expressions.";
  QMessageBox::information(this, "RegExp Explorer", msg);
}

// -------------------------------------------------------------

RegExpSyntaxHighlighter::RegExpSyntaxHighlighter ( QTextEdit * textEdit )
  : QSyntaxHighlighter( textEdit )
{
}

RegExpSyntaxHighlighter::~RegExpSyntaxHighlighter()
{
}

int RegExpSyntaxHighlighter::highlightParagraph ( const QString & text, int /*endStateOfLastPara*/ )
{
  // reset format
  QFont font = textEdit()->currentFont();
  font.setBold( false );
  setFormat(0, text.length(), font, Qt::black );

  if ( regexp.isEmpty() ) 
    return 0; // empty regular expression

  int pos = 0;
  int last = -1;
  font.setBold( true );

  while ( (pos = regexp.search(text, pos)) != -1 ) 
  {
    if ( last == pos )
      break;
    QString sub = text.mid( pos, regexp.matchedLength());
    if ( !sub.isEmpty() )
    {
      setFormat(pos, sub.length(), font, Qt::blue );
    }

    pos += regexp.matchedLength();
    last=pos;
  }

  return 0;
}

void RegExpSyntaxHighlighter::highlightMatchedText( const QRegExp& rx )
{
  regexp = rx;
  rehighlight();
}

void RegExpSyntaxHighlighter::resethighlight()
{
  regexp.setPattern("");
  rehighlight();
}
