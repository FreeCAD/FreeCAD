/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QAbstractTextDocumentLayout>
# include <QApplication>
# include <QClipboard>
# include <QDateTime>
# include <QHBoxLayout>
# include <QMessageBox>
# include <QPainter>
# include <QPrinter>
# include <QPrintDialog>
# include <QScrollBar>
# include <QPlainTextEdit>
# include <QPrintPreviewDialog>
# include <QTextBlock>
# include <QTextCodec>
# include <QTextStream>
# include <QTimer>
#endif

#include "EditorView.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "FileDialog.h"
#include "Macro.h"
#include "PythonDebugger.h"
#include "PythonEditor.h"

#include <Base/Interpreter.h>
#include <Base/Parameter.h>

using namespace Gui;
namespace Gui {
class EditorViewP {
public:
    QPlainTextEdit* textEdit;
    QString fileName;
    QTimer*  activityTimer;
    uint timeStamp;
    bool lock;
    QStringList undos;
    QStringList redos;
};
}

// -------------------------------------------------------

/* TRANSLATOR Gui::EditorView */

/**
 *  Constructs a EditorView which is a child of 'parent', with the
 *  name 'name'.
 */
EditorView::EditorView(QPlainTextEdit* editor, QWidget* parent)
    : MDIView(0,parent,0), WindowParameter( "Editor" )
{
    d = new EditorViewP;
    d->lock = false;

    // create the editor first
    d->textEdit = editor;
    d->textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    // Create the layout containing the workspace and a tab bar
    QFrame* hbox = new QFrame(this);
    hbox->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    QHBoxLayout* layout = new QHBoxLayout();
    layout->setMargin(1);
    layout->addWidget(d->textEdit);
    d->textEdit->setParent(hbox);
    hbox->setLayout(layout);
    setCentralWidget(hbox);

    setCurrentFileName(QString());
    d->textEdit->setFocus();

    setWindowIcon(d->textEdit->windowIcon());

    ParameterGrp::handle hPrefGrp = getWindowParameter();
    hPrefGrp->Attach( this );
    hPrefGrp->NotifyAll();

    d->activityTimer = new QTimer(this);
    connect(d->activityTimer, SIGNAL(timeout()),
            this, SLOT(checkTimestamp()) );
    connect(d->textEdit->document(), SIGNAL(modificationChanged(bool)),
            this, SLOT(setWindowModified(bool)));
    connect(d->textEdit->document(), SIGNAL(undoAvailable(bool)),
            this, SLOT(undoAvailable(bool)));
    connect(d->textEdit->document(), SIGNAL(redoAvailable(bool)),
            this, SLOT(redoAvailable(bool)));
    connect(d->textEdit->document(), SIGNAL(contentsChange(int, int, int)),
            this, SLOT(contentsChange(int, int, int)));
}

/** Destroys the object and frees any allocated resources */
EditorView::~EditorView()
{
    d->activityTimer->stop();
    delete d->activityTimer;
    delete d;
    getWindowParameter()->Detach( this );
}

QPlainTextEdit* EditorView::getEditor() const
{
    return d->textEdit;
}

void EditorView::OnChange(Base::Subject<const char*> &rCaller,const char* rcReason)
{
    ParameterGrp::handle hPrefGrp = getWindowParameter();
    if (strcmp(rcReason, "EnableLineNumber") == 0) {
        //bool show = hPrefGrp->GetBool( "EnableLineNumber", true );
    }
}

void EditorView::checkTimestamp()
{
    QFileInfo fi(d->fileName);
    uint timeStamp =  fi.lastModified().toTime_t();
    if (timeStamp != d->timeStamp) {
        switch( QMessageBox::question( this, tr("Modified file"), 
                tr("%1.\n\nThis has been modified outside of the source editor. Do you want to reload it?").arg(d->fileName),
                QMessageBox::Yes|QMessageBox::Default, QMessageBox::No|QMessageBox::Escape) )
        {
            case QMessageBox::Yes:
                // updates time stamp and timer
                open( d->fileName );
                return;
            case QMessageBox::No:
                d->timeStamp = timeStamp;
                break;
        }
    }

    d->activityTimer->setSingleShot(true);
    d->activityTimer->start(3000);
}

/**
 * Runs the action specified by \a pMsg.
 */
bool EditorView::onMsg(const char* pMsg,const char** ppReturn)
{
    if (strcmp(pMsg,"Save")==0){
        saveFile();
        return true;
    } else if (strcmp(pMsg,"SaveAs")==0){
        saveAs();
        return true;
    } else if (strcmp(pMsg,"Cut")==0){
        cut();
        return true;
    } else if (strcmp(pMsg,"Copy")==0){
        copy();
        return true;
    } else if (strcmp(pMsg,"Paste")==0){
        paste();
        return true;
    } else if (strcmp(pMsg,"Undo")==0){
        undo();
        return true;
    } else if (strcmp(pMsg,"Redo")==0){
        redo();
        return true;
    } else if (strcmp(pMsg,"ViewFit")==0){
        // just ignore this
        return true;
    }

    return false;
}

/**
 * Checks if the action \a pMsg is available. This is for enabling/disabling
 * the corresponding buttons or menu items for this action.
 */
bool EditorView::onHasMsg(const char* pMsg) const
{
    if (strcmp(pMsg,"Run")==0)  return true;
    if (strcmp(pMsg,"DebugStart")==0)  return true;
    if (strcmp(pMsg,"DebugStop")==0)  return true;
    if (strcmp(pMsg,"SaveAs")==0)  return true;
    if (strcmp(pMsg,"Print")==0) return true;
    if (strcmp(pMsg,"PrintPreview")==0) return true;
    if (strcmp(pMsg,"PrintPdf")==0) return true;
    if (strcmp(pMsg,"Save")==0) { 
        return d->textEdit->document()->isModified();
    } else if (strcmp(pMsg,"Cut")==0) {
        bool canWrite = !d->textEdit->isReadOnly();
        return (canWrite && (d->textEdit->textCursor().hasSelection()));
    } else if (strcmp(pMsg,"Copy")==0) {
        return ( d->textEdit->textCursor().hasSelection() );
    } else if (strcmp(pMsg,"Paste")==0) {
        QClipboard *cb = QApplication::clipboard();
        QString text;

        // Copy text from the clipboard (paste)
        text = cb->text();

        bool canWrite = !d->textEdit->isReadOnly();
        return ( !text.isEmpty() && canWrite );
    } else if (strcmp(pMsg,"Undo")==0) {
        return d->textEdit->document()->isUndoAvailable ();
    } else if (strcmp(pMsg,"Redo")==0) {
        return d->textEdit->document()->isRedoAvailable ();
    }

    return false;
}

/** Checking on close state. */
bool EditorView::canClose(void)
{
    if ( !d->textEdit->document()->isModified() )
        return true;
    this->setFocus(); // raises the view to front
    switch( QMessageBox::question(this, tr("Unsaved document"), 
                                    tr("The document has been modified.\n"
                                       "Do you want to save your changes?"),
                                     QMessageBox::Yes|QMessageBox::Default, QMessageBox::No, 
                                     QMessageBox::Cancel|QMessageBox::Escape))
    {
        case QMessageBox::Yes:
            return saveFile();
        case QMessageBox::No:
            return true;
        case QMessageBox::Cancel:
            return false;
        default:
            return false;
    }
}

/**
 * Saves the content of the editor to a file specified by the appearing file dialog.
 */
bool EditorView::saveAs(void)
{
    QString fn = FileDialog::getSaveFileName(this, QObject::tr("Save Macro"),
        QString::null, tr("FreeCAD macro (*.FCMacro);;Python (*.py)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return saveFile();
}

/**
 * Opens the file \a fileName.
 */
bool EditorView::open(const QString& fileName)
{
    if (!QFile::exists(fileName))
        return false;
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly))
        return false;

    d->lock = true;
    d->textEdit->setPlainText(QString::fromUtf8(file.readAll()));
    d->lock = false;
    d->undos.clear();
    d->redos.clear();
    file.close();

    QFileInfo fi(fileName);
    d->timeStamp =  fi.lastModified().toTime_t();
    d->activityTimer->setSingleShot(true);
    d->activityTimer->start(3000);

    setCurrentFileName(fileName);
    return true;
}

/**
 * Copies the selected text to the clipboard and deletes it from the text edit.
 * If there is no selected text nothing happens.
 */
void EditorView::cut(void)
{
    d->textEdit->cut();
}

/**
 * Copies any selected text to the clipboard.
 */
void EditorView::copy(void)
{
    d->textEdit->copy();
}

/**
 * Pastes the text from the clipboard into the text edit at the current cursor position. 
 * If there is no text in the clipboard nothing happens.
 */
void EditorView::paste(void)
{
    d->textEdit->paste();
}

/**
 * Undoes the last operation.
 * If there is no operation to undo, i.e. there is no undo step in the undo/redo history, nothing happens.
 */
void EditorView::undo(void)
{
    d->lock = true;
    if (!d->undos.isEmpty()) {
        d->redos << d->undos.back();
        d->undos.pop_back();
    }
    d->textEdit->document()->undo();
    d->lock = false;
}

/**
 * Redoes the last operation.
 * If there is no operation to undo, i.e. there is no undo step in the undo/redo history, nothing happens.
 */
void EditorView::redo(void)
{
    d->lock = true;
    if (!d->redos.isEmpty()) {
        d->undos << d->redos.back();
        d->redos.pop_back();
    }
    d->textEdit->document()->redo();
    d->lock = false;
}

/**
 * Shows the printer dialog.
 */
void EditorView::print()
{
    QPrinter printer(QPrinter::ScreenResolution);
    printer.setFullPage(true);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted) {
        d->textEdit->document()->print(&printer);
    }
}

void EditorView::printPreview()
{
    QPrinter printer(QPrinter::ScreenResolution);
    QPrintPreviewDialog dlg(&printer, this);
    connect(&dlg, SIGNAL(paintRequested (QPrinter *)),
            this, SLOT(print(QPrinter *)));
    dlg.exec();
}

void EditorView::print(QPrinter* printer)
{
    d->textEdit->document()->print(printer);
}

/**
 * Prints the document into a Pdf file.
 */
void EditorView::printPdf()
{
    QString filename = FileDialog::getSaveFileName(this, tr("Export PDF"), QString(), tr("PDF file (*.pdf)"));
    if (!filename.isEmpty()) {
        QPrinter printer(QPrinter::ScreenResolution);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        d->textEdit->document()->print(&printer);
    }
}

void EditorView::setCurrentFileName(const QString &fileName)
{
    d->fileName = fileName;
    /*emit*/ changeFileName(d->fileName);
    d->textEdit->document()->setModified(false);

    QString shownName;
    if (fileName.isEmpty())
        shownName = tr("untitled[*]");
    else
        shownName = QString::fromAscii("%1[*]").arg(fileName);
    shownName += tr(" - Editor");
    setWindowTitle(shownName);
    setWindowModified(false);
}

QString EditorView::fileName() const
{
    return d->fileName;
}

/**
 * Saves the contents to a file.
 */
bool EditorView::saveFile()
{
    if (d->fileName.isEmpty())
        return saveAs();

    QFile file(d->fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << d->textEdit->document()->toPlainText();
    file.close();
    d->textEdit->document()->setModified(false);

    QFileInfo fi(d->fileName);
    d->timeStamp =  fi.lastModified().toTime_t();
    return true;
}

void EditorView::undoAvailable(bool undo)
{
    if (!undo)
        d->undos.clear();
}

void EditorView::redoAvailable(bool redo)
{
    if (!redo)
        d->redos.clear();
}

void EditorView::contentsChange(int position, int charsRemoved, int charsAdded)
{
    if (d->lock)
        return;
    if (charsRemoved > 0 && charsAdded > 0)
        return; // syntax highlighting
    else if (charsRemoved > 0)
        d->undos << tr("%1 chars removed").arg(charsRemoved);
    else if (charsAdded > 0)
        d->undos << tr("%1 chars added").arg(charsAdded);
    else
        d->undos << tr("Formatted");
    d->redos.clear();
}

/**
 * Get the undo history.
 */
QStringList EditorView::undoActions() const
{
    return d->undos;
}

/**
 * Get the redo history.
 */
QStringList EditorView::redoActions() const
{
    return d->redos;;
}

void EditorView::focusInEvent (QFocusEvent * e)
{
    d->textEdit->setFocus();
}

// ---------------------------------------------------------

PythonEditorView::PythonEditorView(PythonEditor* editor, QWidget* parent)
  : EditorView(editor, parent), _pye(editor)
{
    connect(this, SIGNAL(changeFileName(const QString&)),
            editor, SLOT(setFileName(const QString&)));
}

PythonEditorView::~PythonEditorView()
{
}

/**
 * Runs the action specified by \a pMsg.
 */
bool PythonEditorView::onMsg(const char* pMsg,const char** ppReturn)
{
    if (strcmp(pMsg,"Run")==0) {
        executeScript();
        return true;
    }
    else if (strcmp(pMsg,"StartDebug")==0) {
        QTimer::singleShot(300, this, SLOT(startDebug()));
        return true;
    }
    else if (strcmp(pMsg,"ToggleBreakpoint")==0) {
        toggleBreakpoint();
        return true;
    }
    return EditorView::onMsg(pMsg, ppReturn);
}

/**
 * Checks if the action \a pMsg is available. This is for enabling/disabling
 * the corresponding buttons or menu items for this action.
 */
bool PythonEditorView::onHasMsg(const char* pMsg) const
{
    if (strcmp(pMsg,"Run")==0)  return true;
    if (strcmp(pMsg,"StartDebug")==0)  return true;
    if (strcmp(pMsg,"ToggleBreakpoint")==0)  return true;
    return EditorView::onHasMsg(pMsg);
}

/**
 * Runs the opened script in the macro manager.
 */
void PythonEditorView::executeScript()
{
    Application::Instance->macroManager()->run(Gui::MacroManager::File,fileName().toUtf8());
}

void PythonEditorView::startDebug()
{
    _pye->startDebug();
}

void PythonEditorView::toggleBreakpoint()
{
    _pye->toggleBreakpoint();
}

void PythonEditorView::showDebugMarker(int line)
{
    _pye->showDebugMarker(line);
}

void PythonEditorView::hideDebugMarker()
{
    _pye->hideDebugMarker();
}

#include "moc_EditorView.cpp"
