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
# include <QClipboard>
# include <QDockWidget>
# include <QGridLayout>
# include <QHBoxLayout>
# include <QKeyEvent>
# include <QMenu>
# include <QMessageBox>
# include <QPushButton>
# include <QSpacerItem>
# include <QTextCursor>
# include <QTextDocumentFragment>
# include <QTextStream>
# include <QUrl>
#endif

#include "PythonConsole.h"
#include "PythonConsolePy.h"
#include "CallTips.h"
#include "Application.h"
#include "Action.h"
#include "Command.h"
#include "DlgEditorImp.h"
#include "FileDialog.h"
#include "MainWindow.h"


#include <Base/Interpreter.h>
#include <Base/Exception.h>
#include <CXX/Exception.hxx>

using namespace Gui;

namespace Gui
{

static size_t promptLength = 4; //< length of prompt string: ">>> " or "... ", in either case 4 characters

struct PythonConsoleP
{
    enum Output {Error = 20, Message = 21};
    enum CopyType {Normal, History, Command};
    CopyType type;
    PyObject *_stdoutPy, *_stderrPy, *_stdinPy, *_stdin;
    InteractiveInterpreter* interpreter;
    CallTipsList* callTipsList;
    ConsoleHistory history;
    QString output, error, info;
    QStringList statements;
    bool interactive;
    QMap<QString, QColor> colormap; // Color map
    PythonConsoleP()
    {
        type = Normal;
        interpreter = 0;
        colormap[QLatin1String("Text")] = Qt::black;
        colormap[QLatin1String("Bookmark")] = Qt::cyan;
        colormap[QLatin1String("Breakpoint")] = Qt::red;
        colormap[QLatin1String("Keyword")] = Qt::blue;
        colormap[QLatin1String("Comment")] = QColor(0, 170, 0);
        colormap[QLatin1String("Block comment")] = QColor(160, 160, 164);
        colormap[QLatin1String("Number")] = Qt::blue;
        colormap[QLatin1String("String")] = Qt::red;
        colormap[QLatin1String("Character")] = Qt::red;
        colormap[QLatin1String("Class name")] = QColor(255, 170, 0);
        colormap[QLatin1String("Define name")] = QColor(255, 170, 0);
        colormap[QLatin1String("Operator")] = QColor(160, 160, 164);
        colormap[QLatin1String("Python output")] = QColor(170, 170, 127);
        colormap[QLatin1String("Python error")] = Qt::red;
    }
};
struct InteractiveInterpreterP
{
    PyObject* interpreter;
    PyObject* sysmodule;
    QStringList buffer;
};
} // namespace Gui

InteractiveInterpreter::InteractiveInterpreter()
{
    // import code.py and create an instance of InteractiveInterpreter
    Base::PyGILStateLocker lock;
    PyObject* module = PyImport_ImportModule("code");
    if (!module)
    throw Base::PyException();
    PyObject* func = PyObject_GetAttrString(module, "InteractiveInterpreter");
    PyObject* args = Py_BuildValue("()");
    d = new InteractiveInterpreterP;
    d->interpreter = PyEval_CallObject(func,args);
    Py_DECREF(args);
    Py_DECREF(func);
    Py_DECREF(module);

    setPrompt();
}

InteractiveInterpreter::~InteractiveInterpreter()
{
    Base::PyGILStateLocker lock;
    Py_XDECREF(d->interpreter);
    Py_XDECREF(d->sysmodule);
    delete d;
}

/**
 * Set the ps1 and ps2 members of the sys module if not yet defined.
 */
void InteractiveInterpreter::setPrompt()
{
    // import code.py and create an instance of InteractiveInterpreter
    Base::PyGILStateLocker lock;
    d->sysmodule = PyImport_ImportModule("sys");
    if (!PyObject_HasAttrString(d->sysmodule, "ps1"))
        PyObject_SetAttrString(d->sysmodule, "ps1", PyString_FromString(">>> "));
    if (!PyObject_HasAttrString(d->sysmodule, "ps2"))
        PyObject_SetAttrString(d->sysmodule, "ps2", PyString_FromString("... "));
}

/**
 * Compile a command and determine whether it is incomplete.
 * 
 * The source string may contain line feeds and/or carriage returns. \n
 * Return value / exceptions raised:
 * - Return a code object if the command is complete and valid
 * - Return None if the command is incomplete
 * - Raise SyntaxError, ValueError or OverflowError if the command is a
 * syntax error (OverflowError and ValueError can be produced by
 * malformed literals).
 */
PyObject* InteractiveInterpreter::compile(const char* source) const
{
    Base::PyGILStateLocker lock;
    PyObject* func = PyObject_GetAttrString(d->interpreter, "compile");
    PyObject* args = Py_BuildValue("(s)", source);
    PyObject* eval = PyEval_CallObject(func,args);  // must decref later

    Py_DECREF(args);
    Py_DECREF(func);

    if (eval){
        return eval;
    } else {
        // do not throw Base::PyException as this clears the error indicator
        throw Base::Exception();
    }

    // can never happen
    return 0;
}

/**
 * Compile a command and determine whether it is incomplete.
 * 
 * The source string may contain line feeds and/or carriage returns. \n
 * Return value:
 * - Return  1 if the command is incomplete
 * - Return  0 if the command is complete and valid
 * - Return -1 if the command is a syntax error
 * .
 * (OverflowError and ValueError can be produced by malformed literals).
 */
int InteractiveInterpreter::compileCommand(const char* source) const
{
    Base::PyGILStateLocker lock;
    PyObject* func = PyObject_GetAttrString(d->interpreter, "compile");
    PyObject* args = Py_BuildValue("(s)", source);
    PyObject* eval = PyEval_CallObject(func,args);  // must decref later

    Py_DECREF(args);
    Py_DECREF(func);

    int ret = 0;
    if (eval){
        if (PyObject_TypeCheck(Py_None, eval->ob_type))
            ret = 1; // incomplete
        else
            ret = 0; // complete
        Py_DECREF(eval);
    } else {
        ret = -1;    // invalid
    }

    return ret;
}

/**
 * Compile and run some source in the interpreter.
 *
 * One several things can happen:
 *
 * - The input is incorrect; compile() raised an exception (SyntaxError or OverflowError).  
 *   A syntax traceback will be printed by calling Python's PyErr_Print() method to the redirected stderr.
 *
 * - The input is incomplete, and more input is required; compile() returned 'None'. 
 *   Nothing happens.
 *
 * - The input is complete; compile() returned a code object.  The code is executed by calling 
 *   runCode() (which also handles run-time exceptions, except for SystemExit).
 * .
 * The return value is True if the input is incomplete, False in the other cases (unless
 * an exception is raised). The return value can be used to decide whether to use sys.ps1 
 * or sys.ps2 to prompt the next line.
 */
bool InteractiveInterpreter::runSource(const char* source) const
{
    Base::PyGILStateLocker lock;
    PyObject* code;
    try {
        code = compile(source);
    } catch (const Base::Exception&) {
        // A system, overflow or value error was raised.
        // We clear the traceback info as this might be a longly
        // message we don't need.
        PyObject *errobj, *errdata, *errtraceback;
        PyErr_Fetch(&errobj, &errdata, &errtraceback);
        PyErr_Restore(errobj, errdata, 0);
        // print error message
        if (PyErr_Occurred()) PyErr_Print();
            return false;
    }

    // the command is incomplete
    if (PyObject_TypeCheck(Py_None, code->ob_type)) {
        Py_DECREF(code);
        return true;
    }

    // run the code and return false
    runCode((PyCodeObject*)code);
    return false;
}

/* Execute a code object.
 *
 * When an exception occurs,  a traceback is displayed.
 * All exceptions are caught except SystemExit, which is reraised.
 */
void InteractiveInterpreter::runCode(PyCodeObject* code) const
{
    Base::PyGILStateLocker lock;
    PyObject *module, *dict, *presult;           /* "exec code in d, d" */
    module = PyImport_AddModule("__main__");     /* get module, init python */
    if (module == NULL) 
        throw Base::PyException();                 /* not incref'd */
    dict = PyModule_GetDict(module);             /* get dict namespace */
    if (dict == NULL) 
        throw Base::PyException();                 /* not incref'd */

    // It seems that the return value is always 'None' or Null
    presult = PyEval_EvalCode(code, dict, dict); /* run compiled bytecode */
    Py_XDECREF(code);                            /* decref the code object */
    if (!presult) {
        if (PyErr_ExceptionMatches(PyExc_SystemExit)) {
            // throw SystemExit exception
            throw Base::SystemExitException();
        }
        if ( PyErr_Occurred() )                    /* get latest python exception information */
            PyErr_Print();                           /* and print the error to the error output */
    } else {
        Py_DECREF(presult);
    }
}

/**
 * Store the line into the internal buffer and compile the total buffer.
 * In case it is a complete Python command the buffer is emptied.
 */
bool InteractiveInterpreter::push(const char* line)
{
    d->buffer.append(QString::fromAscii(line));
    QString source = d->buffer.join(QLatin1String("\n"));
    try {
        // Source is already UTF-8, so we can use toAscii()
        bool more = runSource(source.toAscii());
        if (!more)
            d->buffer.clear();
        return more;
    } catch (const Base::SystemExitException&) {
        d->buffer.clear();
        throw;
    } catch (...) {
        // indication of unhandled exception
        d->buffer.clear();
        if (PyErr_Occurred())
            PyErr_Print();
        throw;
    }

    return false;
}

bool InteractiveInterpreter::hasPendingInput( void ) const
{
    return (!d->buffer.isEmpty());
}

QStringList InteractiveInterpreter::getBuffer() const
{
    return d->buffer;
}

void InteractiveInterpreter::setBuffer(const QStringList& buf)
{
    d->buffer = buf;
}

void InteractiveInterpreter::clearBuffer()
{
    d->buffer.clear();
}

/* TRANSLATOR Gui::PythonConsole */

/**
 *  Constructs a PythonConsole which is a child of 'parent'. 
 */
PythonConsole::PythonConsole(QWidget *parent)
  : TextEdit(parent), WindowParameter( "Editor" )
{
    d = new PythonConsoleP();
    d->interactive = false;

    // create an instance of InteractiveInterpreter
    try { 
        d->interpreter = new InteractiveInterpreter();
    } catch (const Base::Exception& e) {
        setPlainText(QString::fromAscii(e.what()));
        setEnabled(false);
    }

    // use the console highlighter
    pythonSyntax = new PythonConsoleHighlighter(this);
    pythonSyntax->setDocument(this->document());

    // create the window for call tips
    d->callTipsList = new CallTipsList(this);
    d->callTipsList->setFrameStyle(QFrame::Box|QFrame::Raised);
    d->callTipsList->setLineWidth(2);
    installEventFilter(d->callTipsList);
    viewport()->installEventFilter(d->callTipsList);
    d->callTipsList->setSelectionMode( QAbstractItemView::SingleSelection );
    d->callTipsList->hide();

    QFont serifFont(QLatin1String("Courier"), 10, QFont::Normal);
    setFont(serifFont);
    
    // set colors and font from settings
    ParameterGrp::handle hPrefGrp = getWindowParameter();
    hPrefGrp->Attach( this );
    hPrefGrp->NotifyAll();

    // disable undo/redo stuff
    setUndoRedoEnabled( false );
    setAcceptDrops( true );

    // try to override Python's stdout/err
    Base::PyGILStateLocker lock;
    d->_stdoutPy = new PythonStdout(this);
    d->_stderrPy = new PythonStderr(this);
    d->_stdinPy  = new PythonStdin (this);
    d->_stdin  = PySys_GetObject("stdin");
    PySys_SetObject("stdin", d->_stdinPy);

    const char* version  = PyString_AsString(PySys_GetObject("version"));
    const char* platform = PyString_AsString(PySys_GetObject("platform"));
    d->info = QString::fromAscii("Python %1 on %2\n"
    "Type 'help', 'copyright', 'credits' or 'license' for more information.")
    .arg(QString::fromAscii(version)).arg(QString::fromAscii(platform));
    d->output = d->info;
    printPrompt(PythonConsole::Complete);
}

/** Destroys the object and frees any allocated resources */
PythonConsole::~PythonConsole()
{
    Base::PyGILStateLocker lock;
    getWindowParameter()->Detach( this );
    delete pythonSyntax;
    Py_XDECREF(d->_stdoutPy);
    Py_XDECREF(d->_stderrPy);
    Py_XDECREF(d->_stdinPy);
    delete d->interpreter;
    delete d;
}

/** Set new font and colors according to the paramerts. */  
void PythonConsole::OnChange( Base::Subject<const char*> &rCaller,const char* sReason )
{
    ParameterGrp::handle hPrefGrp = getWindowParameter();

    if (strcmp(sReason, "FontSize") == 0 || strcmp(sReason, "Font") == 0) {
        int fontSize = hPrefGrp->GetInt("FontSize", 10);
        QString fontFamily = QString::fromAscii(hPrefGrp->GetASCII("Font", "Courier").c_str());
        
        QFont font(fontFamily, fontSize);
        setFont(font);
        QFontMetrics metric(font);
        int width = metric.width(QLatin1String("0000"));
        setTabStopWidth(width);
    } else {
        QMap<QString, QColor>::ConstIterator it = d->colormap.find(QString::fromAscii(sReason));
        if (it != d->colormap.end()) {
            QColor color = it.value();
            unsigned long col = (color.red() << 24) | (color.green() << 16) | (color.blue() << 8);
            col = hPrefGrp->GetUnsigned( sReason, col);
            color.setRgb((col>>24)&0xff, (col>>16)&0xff, (col>>8)&0xff);
            pythonSyntax->setColor(QString::fromAscii(sReason), color);
        }
    }
}

/**
 * Checks the input of the console to make the correct indentations.
 * After a command is prompted completely the Python interpreter is started.
 */
void PythonConsole::keyPressEvent(QKeyEvent * e)
{
    bool restartHistory = true;
    QTextCursor  cursor = this->textCursor();

    // construct reference cursor at begin of input line ...
    QTextCursor inputLineBegin = cursor;
    inputLineBegin.movePosition( QTextCursor::End );
    inputLineBegin.movePosition( QTextCursor::StartOfLine );
    inputLineBegin.movePosition( QTextCursor::Right, QTextCursor::MoveAnchor, promptLength );

    if (cursor < inputLineBegin)
    {
        /**
         * The cursor is placed not on the input line (or within the prompt string)
         * So we handle key input as follows:
         *   - don't allow changing previous lines.
         *   - allow full movement (no prompt restriction)
         *   - allow copying content (Ctrl+C)
         *   - "escape" to end of input line
         */
        switch (e->key())
        {
          case Qt::Key_Return:
          case Qt::Key_Enter:
          case Qt::Key_Escape:
              this->moveCursor( QTextCursor::End );
              break;

          default:
              if (e->text().isEmpty() ||
                  e->matches(QKeySequence::Copy) ||
                  e->matches(QKeySequence::SelectAll)) {
                  TextEdit::keyPressEvent(e);
              }
              else if (!e->text().isEmpty() && 
                  (e->modifiers() == Qt::NoModifier || 
                   e->modifiers() == Qt::ShiftModifier)) {
                  this->moveCursor(QTextCursor::End);
                  TextEdit::keyPressEvent(e);
              }
              break;
        }
    }
    else
    {
        /**
         * The cursor sits somewhere on the input line (after the prompt)
         * Here we handle key input a bit different:
         *   - restrict cursor movement to input line range (excluding the prompt characters)
         *   - roam the history by Up/Down keys
         *   - show call tips on period
         */
        QTextBlock inputBlock = inputLineBegin.block();              //< get the last paragraph's text
        QString    inputLine  = inputBlock.text().mid(promptLength); //< and skip prompt characters

        switch (e->key())
        {
          case Qt::Key_Escape:
          {
              // disable current input line - i.e. put it to history but don't execute it.
              if (!inputLine.isEmpty())
              {
                  d->history.append( QLatin1String("# ") + inputLine );  //< put line to history ...
                  inputLineBegin.insertText( QString::fromAscii("# ") ); //< but comment it on console
                  setTextCursor( inputLineBegin );
                  printPrompt(d->interpreter->hasPendingInput()      //< print adequate prompt
                      ? PythonConsole::Incomplete
                      : PythonConsole::Complete);
              }
          }   break;

          case Qt::Key_Return:
          case Qt::Key_Enter:
          {
              runSource( inputLine );         //< commit input line
              d->history.append( inputLine ); //< put statement to history
          }   break;

          case Qt::Key_Period:
          {
              // analyse context and show available call tips
              int contextLength = cursor.position() - inputLineBegin.position();
              TextEdit::keyPressEvent(e);
              d->callTipsList->showTips( inputLine.left( contextLength ) );
          }   break;

          case Qt::Key_Home:
          {
              QTextCursor::MoveMode mode = (e->modifiers() & Qt::ShiftModifier)? QTextCursor::KeepAnchor
                                                                    /* else */ : QTextCursor::MoveAnchor;
              cursor.setPosition( inputBlock.position() + promptLength, mode );
              setTextCursor( cursor );
              ensureCursorVisible();
          }   break;

          case Qt::Key_Up:
          {
              // if possible, move back in history
              if (d->history.prev( inputLine ))
                  { overrideCursor( d->history.value() ); }
              restartHistory = false;
          }   break;

          case Qt::Key_Down:
          {
              // if possible, move forward in history
              if (d->history.next())
                  { overrideCursor( d->history.value() ); }
              restartHistory = false;
          }   break;

          case Qt::Key_Left:
          {
              if (cursor > inputLineBegin)
                  { TextEdit::keyPressEvent(e); }
              restartHistory = false;
          }   break;

          case Qt::Key_Right:
          {
              TextEdit::keyPressEvent(e);
              restartHistory = false;
          }   break;

          case Qt::Key_Backspace:
          {
              if (cursor > inputLineBegin)
                  { TextEdit::keyPressEvent(e); }
          }   break;

          default: 
          {
              TextEdit::keyPressEvent(e);
          }   break;
        }
        // This can't be done in CallTipsList::eventFilter() because we must first perform
        // the event and afterwards update the list widget
        if (d->callTipsList->isVisible())
            { d->callTipsList->validateCursor(); }
    }
    // any cursor move resets the history to its latest item.
    if (restartHistory)
        { d->history.restart(); }
}

/**
 * Insert an output message to the console. This message comes from
 * the Python interpreter and is redirected from sys.stdout.
 */
void PythonConsole::insertPythonOutput( const QString& msg )
{
    d->output += msg;
}

/**
 * Insert an error message to the console. This message comes from
 * the Python interpreter and is redirected from sys.stderr.
 */
void PythonConsole::insertPythonError ( const QString& err )
{
    d->error += err;
}

void PythonConsole::onFlush()
{
    printPrompt(PythonConsole::Flush);
}

/** Prints the ps1 prompt (>>> ) for complete and ps2 prompt (... ) for
 * incomplete commands to the console window. 
 */ 
void PythonConsole::printPrompt(PythonConsole::Prompt mode)
{
    // write normal messages
    if (!d->output.isEmpty()) {
        appendOutput(d->output, (int)PythonConsoleP::Message);
        d->output = QString::null;
    }

    // write error messages
    if (!d->error.isEmpty()) {
        appendOutput(d->error, (int)PythonConsoleP::Error);
        d->error = QString::null;
    }

    // Append the prompt string 
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::End);
    QTextBlock block = cursor.block();

    // Python's print command appends a trailing '\n' to the system output.
    // In this case, however, we should not add a new text block. We force
    // the current block to be normal text (user state = 0) to be highlighted 
    // correctly and append the '>>> ' or '... ' to this block.
    if (block.length() > 1)
        cursor.insertBlock(cursor.blockFormat(), cursor.charFormat());
    else
        block.setUserState(0);

    switch (mode)
    {
    case PythonConsole::Incomplete:
        cursor.insertText(QString::fromAscii("... "));
        break;
    case PythonConsole::Complete:
        cursor.insertText(QString::fromAscii(">>> "));
        break;
    default:
        break;
    }
    cursor.endEditBlock();

    // move cursor to the end
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

/**
 * Appends \a output to the console and set \a state as user state to
 * the text block which is needed for the highlighting.
 */
void PythonConsole::appendOutput(const QString& output, int state)
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    int pos = cursor.position() + 1;
    
    // delay rehighlighting
    cursor.beginEditBlock();
    appendPlainText(output);

    QTextBlock block = this->document()->findBlock(pos);
    while (block.isValid()) {
        block.setUserState(state);
        block = block.next();
    }
    cursor.endEditBlock(); // start highlightiong
}

/**
 * Builds up the Python command and pass it to the interpreter.
 */
void PythonConsole::runSource(const QString& line)
{
    bool incomplete = false;
    Base::PyGILStateLocker lock;
    PyObject* default_stdout = PySys_GetObject("stdout");
    PyObject* default_stderr = PySys_GetObject("stderr");
    PySys_SetObject("stdout", d->_stdoutPy);
    PySys_SetObject("stderr", d->_stderrPy);
    d->interactive = true;
    
    try {
        // launch the command now
        incomplete = d->interpreter->push(line.toUtf8());
        setFocus(); // if focus was lost
    }
    catch (const Base::SystemExitException&) {
        ParameterGrp::handle hPrefGrp = getWindowParameter();
        bool check = hPrefGrp->GetBool("CheckSystemExit",true);
        if (!check)  Base::Interpreter().systemExit();
        int ret = QMessageBox::question(this, tr("System exit"), tr("The application is still running.\nDo you want to exit without saving your data?"),
        QMessageBox::Yes, QMessageBox::No|QMessageBox::Escape|QMessageBox::Default);
        if (ret == QMessageBox::Yes) {
            Base::Interpreter().systemExit();
        } else {
            PyErr_Clear();
        }
    }
    catch (const Py::Exception&) {
        QMessageBox::critical(this, tr("Python console"), tr("Unhandled PyCXX exception."));
    }
    catch (const Base::Exception&) {
        QMessageBox::critical(this, tr("Python console"), tr("Unhandled FreeCAD exception."));
    }
    catch (const std::exception&) {
        QMessageBox::critical(this, tr("Python console"), tr("Unhandled std C++ exception."));
    }
    catch (...) {
        QMessageBox::critical(this, tr("Python console"), tr("Unhandled unknown C++ exception."));
    }

    printPrompt(incomplete ? PythonConsole::Incomplete
                           : PythonConsole::Complete);
    PySys_SetObject("stdout", default_stdout);
    PySys_SetObject("stderr", default_stderr);
    d->interactive = false;
    for (QStringList::Iterator it = d->statements.begin(); it != d->statements.end(); ++it)
        printStatement(*it);
    d->statements.clear();
}

bool PythonConsole::isComment(const QString& source) const
{
    if (source.isEmpty())
        return false;
    int i=0;
    while (i < source.length()) {
        QChar ch = source.at(i++);
        if (ch.isSpace())
            continue;
        if (ch == QLatin1Char('#'))
            return true;
    }

    return false;
}

/**
 * Prints the Python statement cmd to the console.
 * @note The statement gets only printed and added to the history but not invoked.
 */
void PythonConsole::printStatement( const QString& cmd )
{
    // If we are in interactive mode we have to wait until the command is finished,
    // afterwards we can print the statements.
    if (d->interactive) {
        d->statements << cmd;
        return;
    }

    QTextCursor cursor = textCursor();
    QStringList statements = cmd.split(QLatin1String("\n"));
    for (QStringList::Iterator it = statements.begin(); it != statements.end(); ++it) {
        // go to the end before inserting new text 
        cursor.movePosition(QTextCursor::End);
        cursor.insertText( *it );
        d->history.append( *it );
        printPrompt(PythonConsole::Complete);
    }
}

/**
 * Shows the Python window and sets the focus to set text cursor.
 */
void PythonConsole::showEvent (QShowEvent * e)
{
    TextEdit::showEvent(e);
    // set also the text cursor to the edit field
    setFocus();
}

void PythonConsole::visibilityChanged (bool visible)
{
    if (visible)
        setFocus();
}

void PythonConsole::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::ParentChange) {
        QDockWidget* dw = qobject_cast<QDockWidget*>(this->parentWidget());
        if (dw) {
            connect(dw, SIGNAL(visibilityChanged(bool)),
                    this, SLOT(visibilityChanged(bool)));
        }
    }
    TextEdit::changeEvent(e);
}

/**
 * Drops the event \a e and writes the right Python command.
 */
void PythonConsole::dropEvent (QDropEvent * e)
{
    const QMimeData* mimeData = e->mimeData();
    if (mimeData->hasFormat(QLatin1String("text/x-action-items"))) {
        QByteArray itemData = mimeData->data(QLatin1String("text/x-action-items"));
        QDataStream dataStream(&itemData, QIODevice::ReadOnly);

        int ctActions; dataStream >> ctActions;
        for (int i=0; i<ctActions; i++) {
            QString action;
            dataStream >> action;
            printStatement(QString::fromAscii("Gui.runCommand(\"%1\")").arg(action));
        }

        e->setDropAction(Qt::CopyAction);
        e->accept();
    }
    else // this will call insertFromMimeData
        QPlainTextEdit::dropEvent(e);
}

/** Dragging of action objects is allowed. */ 
void PythonConsole::dragMoveEvent( QDragMoveEvent *e )
{
    const QMimeData* mimeData = e->mimeData();
    if (mimeData->hasFormat(QLatin1String("text/x-action-items")))
        e->accept();
    else // this will call canInsertFromMimeData
        QPlainTextEdit::dragMoveEvent(e);
}

/** Dragging of action objects is allowed. */ 
void PythonConsole::dragEnterEvent (QDragEnterEvent * e)
{
    const QMimeData* mimeData = e->mimeData();
    if (mimeData->hasFormat(QLatin1String("text/x-action-items")))
        e->accept();
    else // this will call canInsertFromMimeData
        QPlainTextEdit::dragEnterEvent(e);
}

bool PythonConsole::canInsertFromMimeData (const QMimeData * source) const
{
    if (source->hasText())
        return true;
    if (source->hasUrls()) {
        QList<QUrl> uri = source->urls();
        for (QList<QUrl>::ConstIterator it = uri.begin(); it != uri.end(); ++it) {
            QFileInfo info((*it).toLocalFile());
            if (info.exists() && info.isFile()) {
                QString ext = info.suffix().toLower();
                if (ext == QLatin1String("py") || ext == QLatin1String("fcmacro"))
                    return true;
            }
        }
    }

    return false;
}

/**
 * Allow to paste plain text or urls of text files.
 */
void PythonConsole::insertFromMimeData (const QMimeData * source)
{
    if (!source)
        return;
    // First check on urls instead of text otherwise it may happen that a url
    // is handled as text
    if (source->hasUrls()) {
        QList<QUrl> uri = source->urls();
        for (QList<QUrl>::ConstIterator it = uri.begin(); it != uri.end(); ++it) {
            // get the file name and check the extension
            QFileInfo info((*it).toLocalFile());
            QString ext = info.suffix().toLower();
            if (info.exists() && info.isFile() && 
                (ext == QLatin1String("py") || ext == QLatin1String("fcmacro"))) {
                // load the file and read-in the source code
                QFile file(info.absoluteFilePath());
                if (file.open(QIODevice::ReadOnly)) {
                    QTextStream str(&file);
                    runSourceFromMimeData(str.readAll());
                }
                file.close();
            }
        }

        return;
    }
    if (source->hasText()) {
        runSourceFromMimeData(source->text());
        return;
    }
}

QMimeData * PythonConsole::createMimeDataFromSelection () const
{
    QMimeData* mime = new QMimeData();
    
    switch (d->type) {
        case PythonConsoleP::Normal:
            {
                const QTextDocumentFragment fragment(textCursor());
                mime->setText(fragment.toPlainText());
            }   break;
        case PythonConsoleP::Command:
            {
                QTextCursor cursor = textCursor();
                int s = cursor.selectionStart();
                int e = cursor.selectionEnd();
                QTextBlock b;
                QStringList lines;
                for (b = document()->begin(); b.isValid(); b = b.next()) {
                    int pos = b.position();
                    if ( pos >= s && pos <= e ) {
                        if (b.userState() > -1 && b.userState() < pythonSyntax->maximumUserState()) {
                            QString line = b.text();
                            // and skip the prompt characters consisting of either ">>> " or "... "
                            line = line.mid(promptLength);
                            lines << line;
                        }
                    }
                }

                QString text = lines.join(QLatin1String("\n"));
                mime->setText(text);
            }   break;
        case PythonConsoleP::History:
            {
                const QStringList& hist = d->history.values();
                QString text = hist.join(QLatin1String("\n"));
                mime->setText(text);
            }   break;
    }

    return mime;
}

void PythonConsole::runSourceFromMimeData(const QString& source)
{
    // When inserting a big text block we must break it down into several command
    // blocks instead of processing the text block as a whole or each single line.
    // If we processed the complete block as a whole only the first valid Python
    // command would be executed and the rest would be ignored. However, if we 
    // processed each line separately the interpreter might be confused that a block 
    // is complete but it might be not. This is for instance, if a class or method 
    // definition contains several empty lines which leads to error messages (almost
    // indentation errors) later on.
    QString text = source;
    if (text.isNull())
        return;

#if defined (Q_OS_LINUX)
    // Need to convert CRLF to LF
    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
#elif defined(Q_OS_WIN32)
    // Need to convert CRLF to LF
    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
#elif defined(Q_OS_MAC)
    //need to convert CR to LF
    text.replace(QLatin1Char('\r'), QLatin1Char('\n'));
#endif

    // separate the lines and get the last one
    QStringList lines = text.split(QLatin1Char('\n'));
    QString last = lines.back();
    lines.pop_back();

    QTextCursor cursor = textCursor();
    QStringList buffer = d->interpreter->getBuffer();
    d->interpreter->clearBuffer();

    int countNewlines = lines.count(), i = 0;
    for (QStringList::Iterator it = lines.begin(); it != lines.end(); ++it, ++i) {
        QString line = *it;

        // insert the text to the current cursor position
        cursor.insertText(*it);

        // for the very first line get the complete block
        // because it may differ from the inserted text
        if (i == 0) {
            // get the text from the current cursor position to the end, remove it
            // and add it to the last line
            cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
            QString select = cursor.selectedText();
            cursor.removeSelectedText();
            last = last + select;
            line = cursor.block().text();
            line = line.mid(promptLength);
        }

        // put statement to the history
        d->history.append(line);

        buffer.append(line);
        int ret = d->interpreter->compileCommand(buffer.join(QLatin1String("\n")).toUtf8());
        if (ret == 1) { // incomplete
            printPrompt(PythonConsole::Incomplete);
        }
        else if (ret == 0) { // complete
            // check if the following lines belong to the previous block
            int k=i+1;
            QString nextline;
            while ((nextline.isEmpty() || isComment(nextline)) && k < countNewlines) {
                nextline = lines[k];
                k++;
            }
            
            int ret = d->interpreter->compileCommand(nextline.toUtf8());

            // If the line is valid, i.e. complete or incomplete the previous block
            // is finished
            if (ret == -1) {
                // the command is not finished yet
                printPrompt(PythonConsole::Incomplete);
            }
            else {
                runSource(buffer.join(QLatin1String("\n")));
                buffer.clear();
            }
        }
        else { // invalid
            runSource(buffer.join(QLatin1String("\n")));
            ensureCursorVisible();
            return; // exit the method on error
        }
    }

    // set the incomplete block to the interpreter and insert the last line
    d->interpreter->setBuffer(buffer);
    cursor.insertText(last);
    ensureCursorVisible();
}

/**
 * Overwrites the text of the cursor.
 */
void PythonConsole::overrideCursor(const QString& txt)
{
    // Go to the last line and the fourth position, right after the prompt
    QTextCursor cursor = textCursor();
    QTextBlock block = cursor.block();
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfLine);
    cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, promptLength);
    cursor.movePosition(QTextCursor::Right, QTextCursor::KeepAnchor, block.text().length());
    cursor.removeSelectedText();
    cursor.insertText(txt);
    // move cursor to the end
    cursor.movePosition(QTextCursor::End);
    setTextCursor(cursor);
}

void PythonConsole::contextMenuEvent ( QContextMenuEvent * e )
{
    QMenu menu(this);
    QAction *a;
    // construct reference cursor at begin of input line ...
    QTextCursor  cursor = this->textCursor();
    QTextCursor inputLineBegin = cursor;
    inputLineBegin.movePosition(QTextCursor::End);
    inputLineBegin.movePosition(QTextCursor::StartOfLine);
    inputLineBegin.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, promptLength);

    a = menu.addAction(tr("&Copy"), this, SLOT(copy()), Qt::CTRL+Qt::Key_C);
    a->setEnabled(textCursor().hasSelection());

    a = menu.addAction(tr("&Copy command"), this, SLOT(onCopyCommand()));
    a->setEnabled(textCursor().hasSelection());

    a = menu.addAction(tr("&Copy history"), this, SLOT(onCopyHistory()));
    a->setEnabled(!d->history.isEmpty());

    a = menu.addAction( tr("Save history as..."), this, SLOT(onSaveHistoryAs()));
    a->setEnabled(!d->history.isEmpty());

    menu.addSeparator();

    a = menu.addAction(tr("&Paste"), this, SLOT(paste()), Qt::CTRL+Qt::Key_V);
    const QMimeData *md = QApplication::clipboard()->mimeData();
    a->setEnabled(cursor >= inputLineBegin && md && canInsertFromMimeData(md));

    a = menu.addAction(tr("Select All"), this, SLOT(selectAll()), Qt::CTRL+Qt::Key_A);
    a->setEnabled(!document()->isEmpty());

    a = menu.addAction(tr("Clear console"), this, SLOT(onClearConsole()));
    a->setEnabled(!document()->isEmpty());

    menu.addSeparator();
    menu.addAction( tr("Insert file name..."), this, SLOT(onInsertFileName()));
    menu.addSeparator();

    QAction* wrap = menu.addAction(tr("Word wrap"));
    wrap->setCheckable(true);
    wrap->setChecked(this->wordWrapMode() != QTextOption::NoWrap);

    QAction* exec = menu.exec(e->globalPos());
    if (exec == wrap) {
        this->setWordWrapMode(wrap->isChecked()
            ? QTextOption::WrapAtWordBoundaryOrAnywhere : QTextOption::NoWrap);
    }
}

void PythonConsole::onClearConsole()
{
    clear();
    d->output = d->info;
    printPrompt(PythonConsole::Complete);
}

void PythonConsole::onSaveHistoryAs()
{
    QString cMacroPath = QString::fromUtf8(getDefaultParameter()->GetGroup( "Macro" )->
        GetASCII("MacroPath",App::Application::getUserAppDataDir().c_str()).c_str());
    QString fn = FileDialog::getSaveFileName(this, tr("Save History"), cMacroPath,
        tr("Macro Files (*.FCMacro *.py)"));
    if (!fn.isEmpty()) {
        int dot = fn.indexOf(QLatin1Char('.'));
        if (dot != -1) {
            QFile f(fn);
            if (f.open(QIODevice::WriteOnly)) {
                QTextStream t (&f);
                const QStringList& hist = d->history.values();
                for (QStringList::ConstIterator it = hist.begin(); it != hist.end(); ++it)
                    t << *it << "\n";
                f.close();
            }
        }
    }
}

void PythonConsole::onInsertFileName()
{
    QString fn = Gui::FileDialog::getOpenFileName(Gui::getMainWindow(), tr("Insert file name"), QString::null, tr("All Files (*.*)") );
    if ( fn.isEmpty() )
        return;
    insertPlainText(fn);
}

/**
 * Copy the history of the console into the clipboard.
 */
void PythonConsole::onCopyHistory()
{
    if (d->history.isEmpty())
        return;
    d->type = PythonConsoleP::History;
    QMimeData *data = createMimeDataFromSelection();
    QApplication::clipboard()->setMimeData(data);
    d->type = PythonConsoleP::Normal;
}

/**
 * Copy the selected commands into the clipboard. This is a subset of the history.
 */
void PythonConsole::onCopyCommand()
{
    d->type = PythonConsoleP::Command;
    copy();
    d->type = PythonConsoleP::Normal;
}

// ---------------------------------------------------------------------

PythonConsoleHighlighter::PythonConsoleHighlighter(QObject* parent)
  : PythonSyntaxHighlighter(parent)
{
}

PythonConsoleHighlighter::~PythonConsoleHighlighter()
{
}

void PythonConsoleHighlighter::highlightBlock(const QString& text)
{
    const int ErrorOutput   = (int)PythonConsoleP::Error;
    const int MessageOutput = (int)PythonConsoleP::Message;

    // Get user state to re-highlight the blocks in the appropriate format
    int stateOfPara = currentBlockState();

    switch (stateOfPara)
    {
    case ErrorOutput:
        {
            // Error output
            QTextCharFormat errorFormat;
            errorFormat.setForeground(color(QLatin1String("Python error")));
            errorFormat.setFontItalic(true);
            setFormat( 0, text.length(), errorFormat);
        }   break;
    case MessageOutput:
        {
            // Normal output
            QTextCharFormat outputFormat;
            outputFormat.setForeground(color(QLatin1String("Python output")));
            setFormat( 0, text.length(), outputFormat);
        }   break;
    default:
        {
            PythonSyntaxHighlighter::highlightBlock(text);
        }   break;
    }
}

void PythonConsoleHighlighter::colorChanged(const QString& type, const QColor& col)
{
}

// ---------------------------------------------------------------------

ConsoleHistory::ConsoleHistory()
{
    _it = _history.end();
}

ConsoleHistory::~ConsoleHistory()
{
}

void ConsoleHistory::first()
{
    _it = _history.begin();
}

bool ConsoleHistory::more()
{
    return (_it != _history.end());
}

/**
 * next switches the history pointer to the next item.
 * While searching the next item, the routine respects the search prefix set by prev().
 * @return true if the pointer was switched to a later item, false otherwise.
 */
bool ConsoleHistory::next() 
{
    bool wentNext = false;

    // if we didn't reach history's end ...
    if (_it != _history.end())
    {
      // we go forward until we find an item matching the prefix.
      for (++_it; _it != _history.end(); ++_it)
      {
        if (!_it->isEmpty() && _it->startsWith( _prefix ))
          { break; }
      }
      // we did a step - no matter of a matching prefix.
      wentNext = true;
    }
    return wentNext;
}

/**
 * prev switches the history pointer to the previous item.
 * The optional parameter prefix allows to search the history selectively for commands that start
 *   with a certain character sequence.
 * @param prefix - prefix string for searching backwards in history, empty string by default
 * @return true if the pointer was switched to an earlier item, false otherwise.
 */
bool ConsoleHistory::prev( const QString &prefix )
{
    bool wentPrev = false;

    // store prefix if it's the first history access
    if (_it == _history.end())
      { _prefix = prefix; }
    
    // while we didn't go back or reach history's begin ...
    while (!wentPrev && _it != _history.begin())
    {
      // go back in history and check if item matches prefix
      // Skip empty items
      --_it;
      wentPrev = (!_it->isEmpty() && _it->startsWith( _prefix ));
    }
    return wentPrev;
}

bool ConsoleHistory::isEmpty() const
{
    return _history.isEmpty();
}

const QString& ConsoleHistory::value() const
{
    return ((_it != _history.end())? *_it
                        /* else */ :  _prefix);
}

void ConsoleHistory::append( const QString& item )
{
    _history.append( item );
    // reset iterator to make the next history
    //   access begin with the latest item.
    _it = _history.end();
}

const QStringList& ConsoleHistory::values() const
{
    return this->_history;
}

/**
 * restart resets the history access to the latest item.
 */
void ConsoleHistory::restart( void )
{
    _it = _history.end();
}

// -----------------------------------------------------

/* TRANSLATOR Gui::PythonInputField */

PythonInputField::PythonInputField(QWidget* parent)
  : QWidget(parent)
{
    QGridLayout* gridLayout = new QGridLayout(this);
    gridLayout->setSpacing(6);
    gridLayout->setMargin(9);

    editField = new PythonEditor(this);
    gridLayout->addWidget(editField, 0, 0, 1, 1);
    setFocusProxy(editField);

    QHBoxLayout* hboxLayout = new QHBoxLayout();
    hboxLayout->setSpacing(6);
    hboxLayout->setMargin(0);

    QSpacerItem* spacerItem = new QSpacerItem(131, 31, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hboxLayout->addItem(spacerItem);

    okButton = new QPushButton(this);
    hboxLayout->addWidget(okButton);
    clearButton = new QPushButton(this);
    hboxLayout->addWidget(clearButton);
    gridLayout->addLayout(hboxLayout, 1, 0, 1, 1);


    this->setWindowTitle(Gui::PythonConsole::tr("Python Input Dialog"));
    okButton->setText(tr("OK"));
    clearButton->setText(tr("Clear"));

    QObject::connect(okButton, SIGNAL(clicked()), this, SIGNAL(textEntered()));
    QObject::connect(clearButton, SIGNAL(clicked()), editField, SLOT(clear()));
}

PythonInputField::~PythonInputField()
{
}

QString PythonInputField::getText() const
{
    return editField->toPlainText();
}

void PythonInputField::clear()
{
    return editField->clear();
}

void PythonInputField::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        this->setWindowTitle(Gui::PythonConsole::tr("Python Input Dialog"));
        okButton->setText(tr("OK"));
        clearButton->setText(tr("Clear"));
    }
    else {
        QWidget::changeEvent(e);
    }
}

void PythonInputField::showEvent(QShowEvent* e)
{
    editField->setFocus();
}

#include "moc_PythonConsole.cpp"
