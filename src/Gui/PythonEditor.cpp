/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QContextMenuEvent>
# include <QMenu>
# include <QPainter>
# include <QShortcut>
# include <QTextCursor>
#endif

#include <Base/Parameter.h>

#include "PythonEditor.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "Macro.h"
#include "PythonDebugger.h"


using namespace Gui;

namespace Gui {
struct PythonEditorP
{
    int   debugLine{-1};
    QRect debugRect;
    QPixmap breakpoint;
    QPixmap debugMarker;
    QString filename;
    PythonDebugger* debugger;
    PythonEditorP()
        : breakpoint(BitmapFactory().iconFromTheme("breakpoint").pixmap(16,16)),
          debugMarker(BitmapFactory().iconFromTheme("debug-marker").pixmap(16,16))
    {
        debugger = Application::Instance->macroManager()->debugger();
    }
};
} // namespace Gui

/* TRANSLATOR Gui::PythonEditor */

/**
 *  Constructs a PythonEditor which is a child of 'parent' and does the
 *  syntax highlighting for the Python language.
 */
PythonEditor::PythonEditor(QWidget* parent)
  : TextEditor(parent)
{
    d = new PythonEditorP();
    this->setSyntaxHighlighter(new PythonSyntaxHighlighter(this));

    // set accelerators
    auto comment = new QShortcut(this);
    comment->setKey(QKeySequence(QString::fromLatin1("ALT+C")));

    auto uncomment = new QShortcut(this);
    uncomment->setKey(QKeySequence(QString::fromLatin1("ALT+U")));

    connect(comment, &QShortcut::activated, this, &PythonEditor::onComment);
    connect(uncomment, &QShortcut::activated, this, &PythonEditor::onUncomment);
}

/** Destroys the object and frees any allocated resources */
PythonEditor::~PythonEditor()
{
    delete d;
}

void PythonEditor::setFileName(const QString& fn)
{
    d->filename = fn;
}

void PythonEditor::startDebug()
{
    if (d->debugger->start()) {
        d->debugger->runFile(d->filename);
        d->debugger->stop();
    }
}

void PythonEditor::toggleBreakpoint()
{
    QTextCursor cursor = textCursor();
    int line = cursor.blockNumber() + 1;
    d->debugger->toggleBreakpoint(line, d->filename);
    getMarker()->update();
}

void PythonEditor::showDebugMarker(int line)
{
    d->debugLine = line;
    getMarker()->update();
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::StartOfBlock);
    int cur = cursor.blockNumber() + 1;
    if (cur > line) {
        for (int i=line; i<cur; i++)
            cursor.movePosition(QTextCursor::Up);
    }
    else if (cur < line) {
        for (int i=cur; i<line; i++)
            cursor.movePosition(QTextCursor::Down);
    }
    setTextCursor(cursor);
}

void PythonEditor::hideDebugMarker()
{
    d->debugLine = -1;
    getMarker()->update();
}

void PythonEditor::drawMarker(int line, int x, int y, QPainter* p)
{
    Breakpoint bp = d->debugger->getBreakpoint(d->filename);
    if (bp.checkLine(line)) {
        p->drawPixmap(x, y, d->breakpoint);
    }
    if (d->debugLine == line) {
        p->drawPixmap(x, y+2, d->debugMarker);
        d->debugRect = QRect(x, y+2, d->debugMarker.width(), d->debugMarker.height());
    }
}

void PythonEditor::contextMenuEvent ( QContextMenuEvent * e )
{
    QMenu* menu = createStandardContextMenu();
    if (!isReadOnly()) {
        menu->addSeparator();
        QAction* comment = menu->addAction( tr("Comment"), this, &PythonEditor::onComment);
        comment->setShortcut(QKeySequence(QString::fromLatin1("ALT+C")));
        QAction* uncomment = menu->addAction( tr("Uncomment"), this, &PythonEditor::onUncomment);
        uncomment->setShortcut(QKeySequence(QString::fromLatin1("ALT+U")));
    }

    menu->exec(e->globalPos());
    delete menu;
}

void PythonEditor::onComment()
{
    QTextCursor cursor = textCursor();
    int selStart = cursor.selectionStart();
    int selEnd = cursor.selectionEnd();
    QTextBlock block;
    cursor.beginEditBlock();
    for (block = document()->begin(); block.isValid(); block = block.next()) {
        int pos = block.position();
        int off = block.length()-1;
        // at least one char of the block is part of the selection
        if ( pos >= selStart || pos+off >= selStart) {
            if ( pos+1 > selEnd )
                break; // end of selection reached
            cursor.setPosition(block.position());
            cursor.insertText(QLatin1String("#"));
                selEnd++;
        }
    }

    cursor.endEditBlock();
}

void PythonEditor::onUncomment()
{
    QTextCursor cursor = textCursor();
    int selStart = cursor.selectionStart();
    int selEnd = cursor.selectionEnd();
    QTextBlock block;
    cursor.beginEditBlock();
    for (block = document()->begin(); block.isValid(); block = block.next()) {
        int pos = block.position();
        int off = block.length()-1;
        // at least one char of the block is part of the selection
        if ( pos >= selStart || pos+off >= selStart) {
            if ( pos+1 > selEnd )
                break; // end of selection reached
            if (block.text().startsWith(QLatin1String("#"))) {
                cursor.setPosition(block.position());
                cursor.deleteChar();
                selEnd--;
            }
        }
    }

    cursor.endEditBlock();
}

// ------------------------------------------------------------------------

namespace Gui {
class PythonSyntaxHighlighterP
{
public:
    PythonSyntaxHighlighterP()
    {
        keywords << QLatin1String("and") << QLatin1String("as")
                 << QLatin1String("assert") << QLatin1String("break")
                 << QLatin1String("class") << QLatin1String("continue")
                 << QLatin1String("def") << QLatin1String("del")
                 << QLatin1String("elif") << QLatin1String("else")
                 << QLatin1String("except") << QLatin1String("exec")
                 << QLatin1String("False") << QLatin1String("finally")
                 << QLatin1String("for") << QLatin1String("from")
                 << QLatin1String("global") << QLatin1String("if")
                 << QLatin1String("import") << QLatin1String("in")
                 << QLatin1String("is") << QLatin1String("lambda")
                 << QLatin1String("None") << QLatin1String("nonlocal")
                 << QLatin1String("not") << QLatin1String("or")
                 << QLatin1String("pass") << QLatin1String("print")
                 << QLatin1String("raise") << QLatin1String("return")
                 << QLatin1String("True") << QLatin1String("try")
                 << QLatin1String("while") << QLatin1String("with")
                 << QLatin1String("yield");
    }

    QStringList keywords;
};
} // namespace Gui

/**
 * Constructs a Python syntax highlighter.
 */
PythonSyntaxHighlighter::PythonSyntaxHighlighter(QObject* parent)
    : SyntaxHighlighter(parent)
{
    d = new PythonSyntaxHighlighterP;
}

/** Destroys this object. */
PythonSyntaxHighlighter::~PythonSyntaxHighlighter()
{
    delete d;
}

/**
 * Detects all kinds of text to highlight them in the correct color.
 */
void PythonSyntaxHighlighter::highlightBlock (const QString & text)
{
  int i = 0;
  QChar prev, ch;

  const int Standard      = 0;     // Standard text
  const int Digit         = 1;     // Digits
  const int Comment       = 2;     // Comment begins with #
  const int Literal1      = 3;     // String literal beginning with "
  const int Literal2      = 4;     // Other string literal beginning with '
  const int Blockcomment1 = 5;     // Block comments beginning and ending with """
  const int Blockcomment2 = 6;     // Other block comments beginning and ending with '''
  const int ClassName     = 7;     // Text after the keyword class
  const int DefineName    = 8;     // Text after the keyword def

  int endStateOfLastPara = previousBlockState();
  if (endStateOfLastPara < 0 || endStateOfLastPara > maximumUserState())
    endStateOfLastPara = Standard;

  while ( i < text.length() )
  {
    ch = text.at( i );

    switch ( endStateOfLastPara )
    {
    case Standard:
      {
        switch ( ch.unicode() )
        {
        case '#':
          {
            // begin a comment
            setFormat( i, 1, this->colorByType(SyntaxHighlighter::Comment));
            endStateOfLastPara=Comment;
          } break;
        case '"':
          {
            // Begin either string literal or block comment
            if ((i>=2) && text.at(i-1) == QLatin1Char('"') &&
                text.at(i-2) == QLatin1Char('"'))
            {
              setFormat( i-2, 3, this->colorByType(SyntaxHighlighter::BlockComment));
              endStateOfLastPara=Blockcomment1;
            }
            else
            {
              setFormat( i, 1, this->colorByType(SyntaxHighlighter::String));
              endStateOfLastPara=Literal1;
            }
          } break;
        case '\'':
          {
            // Begin either string literal or block comment
            if ((i>=2) && text.at(i-1) == QLatin1Char('\'') &&
                text.at(i-2) == QLatin1Char('\''))
            {
              setFormat( i-2, 3, this->colorByType(SyntaxHighlighter::BlockComment));
              endStateOfLastPara=Blockcomment2;
            }
            else
            {
              setFormat( i, 1, this->colorByType(SyntaxHighlighter::String));
              endStateOfLastPara=Literal2;
            }
          } break;
        case ' ':
        case '\t':
          {
            // ignore whitespaces
          } break;
        case '(': case ')': case '[': case ']':
        case '+': case '-': case '*': case '/':
        case ':': case '%': case '^': case '~':
        case '!': case '=': case '<': case '>': // possibly two characters
          {
            setFormat(i, 1, this->colorByType(SyntaxHighlighter::Operator));
            endStateOfLastPara=Standard;
          } break;
        default:
          {
            // Check for normal text
            if ( ch.isLetter() || ch == QLatin1Char('_') )
            {
              QString buffer;
              int j=i;
              while ( ch.isLetterOrNumber() || ch == QLatin1Char('_') ) {
                buffer += ch;
                ++j;
                if (j >= text.length())
                  break; // end of text
                ch = text.at(j);
              }

              if ( d->keywords.contains( buffer ) != 0 ) {
                if ( buffer == QLatin1String("def"))
                  endStateOfLastPara = DefineName;
                else if ( buffer == QLatin1String("class"))
                  endStateOfLastPara = ClassName;

                QTextCharFormat keywordFormat;
                keywordFormat.setForeground(this->colorByType(SyntaxHighlighter::Keyword));
                keywordFormat.setFontWeight(QFont::Bold);
                setFormat( i, buffer.length(), keywordFormat);
              }
              else {
                setFormat( i, buffer.length(),this->colorByType(SyntaxHighlighter::Text));
              }

              // increment i
              if ( !buffer.isEmpty() )
                i = j-1;
            }
            // this is the beginning of a number
            else if ( ch.isDigit() )
            {
              setFormat(i, 1, this->colorByType(SyntaxHighlighter::Number));
              endStateOfLastPara=Digit;
            }
            // probably an operator
            else if ( ch.isSymbol() || ch.isPunct() )
            {
              setFormat( i, 1, this->colorByType(SyntaxHighlighter::Operator));
            }
          }
        }
      } break;
    case Comment:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::Comment));
      } break;
    case Literal1:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::String));
        if ( ch == QLatin1Char('"') )
          endStateOfLastPara = Standard;
      } break;
    case Literal2:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::String));
        if ( ch == QLatin1Char('\'') )
          endStateOfLastPara = Standard;
      } break;
    case Blockcomment1:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::BlockComment));
        if ( i>=2 && ch == QLatin1Char('"') &&
            text.at(i-1) == QLatin1Char('"') &&
            text.at(i-2) == QLatin1Char('"'))
          endStateOfLastPara = Standard;
      } break;
    case Blockcomment2:
      {
        setFormat( i, 1, this->colorByType(SyntaxHighlighter::BlockComment));
        if ( i>=2 && ch == QLatin1Char('\'') &&
            text.at(i-1) == QLatin1Char('\'') &&
            text.at(i-2) == QLatin1Char('\''))
          endStateOfLastPara = Standard;
      } break;
    case DefineName:
      {
        if ( ch.isLetterOrNumber() || ch == QLatin1Char(' ') || ch == QLatin1Char('_') )
        {
          setFormat( i, 1, this->colorByType(SyntaxHighlighter::Defname));
        }
        else
        {
          if ( ch.isSymbol() || ch.isPunct() )
            setFormat(i, 1, this->colorByType(SyntaxHighlighter::Operator));
          endStateOfLastPara = Standard;
        }
      } break;
    case ClassName:
      {
        if ( ch.isLetterOrNumber() || ch == QLatin1Char(' ') || ch == QLatin1Char('_') )
        {
          setFormat( i, 1, this->colorByType(SyntaxHighlighter::Classname));
        }
        else
        {
          if (ch.isSymbol() || ch.isPunct() )
            setFormat( i, 1, this->colorByType(SyntaxHighlighter::Operator));
          endStateOfLastPara = Standard;
        }
      } break;
    case Digit:
      {
        if (ch.isDigit() || ch == QLatin1Char('.'))
        {
          setFormat( i, 1, this->colorByType(SyntaxHighlighter::Number));
        }
        else
        {
          if ( ch.isSymbol() || ch.isPunct() )
            setFormat( i, 1, this->colorByType(SyntaxHighlighter::Operator));
          endStateOfLastPara = Standard;
        }
      }break;
    }

    prev = ch;
    i++;
  }

  // only block comments can have several lines
  if ( endStateOfLastPara != Blockcomment1 && endStateOfLastPara != Blockcomment2 )
  {
    endStateOfLastPara = Standard ;
  }

  setCurrentBlockState(endStateOfLastPara);
}

#include "moc_PythonEditor.cpp"
