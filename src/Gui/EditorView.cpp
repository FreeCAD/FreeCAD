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
# include <QApplication>
# include <QCheckBox>
# include <QClipboard>
# include <QDateTime>
# include <QHBoxLayout>
# include <QVBoxLayout>
# include <QLineEdit>
# include <QMessageBox>
# include <QPrinter>
# include <QPrintDialog>
# include <QPlainTextEdit>
# include <QPrintPreviewDialog>
# include <QSpacerItem>
# include <QStyle>
# include <QTextCursor>
# include <QTextDocument>
# include <QTextStream>
# include <QTimer>
# include <QToolButton>
#endif

#include "EditorView.h"
#include "Application.h"
#include "FileDialog.h"
#include "Macro.h"
#include "MainWindow.h"
#include "PythonEditor.h"
#include "PythonTracing.h"
#include "WaitCursor.h"

#include <Base/Exception.h>
#include <Base/Interpreter.h>
#include <Base/Parameter.h>


using namespace Gui;
namespace Gui {
class EditorViewP {
public:
    TextEdit* textEdit;
    SearchBar* searchBar;
    QString fileName;
    EditorView::DisplayName displayName;
    QTimer*  activityTimer;
    qint64 timeStamp;
    bool lock;
    bool aboutToClose;
    QStringList undos;
    QStringList redos;
};
}

// -------------------------------------------------------

/* TRANSLATOR Gui::EditorView */

TYPESYSTEM_SOURCE_ABSTRACT(Gui::EditorView, Gui::MDIView)

/**
 *  Constructs a EditorView which is a child of 'parent', with the
 *  name 'name'.
 */
EditorView::EditorView(TextEdit* editor, QWidget* parent)
    : MDIView(nullptr, parent, Qt::WindowFlags())
    , WindowParameter( "Editor" )
{
    d = new EditorViewP;
    d->lock = false;
    d->aboutToClose = false;
    d->displayName = EditorView::FullName;

    // create the editor first
    d->textEdit = editor;
    d->textEdit->setLineWrapMode(QPlainTextEdit::NoWrap);

    d->searchBar = new SearchBar();
    d->searchBar->setEditor(editor);

    // update editor actions on request
    Gui::MainWindow* mw = Gui::getMainWindow();
    connect(editor, &QPlainTextEdit::undoAvailable, mw, &MainWindow::updateEditorActions);
    connect(editor, &QPlainTextEdit::redoAvailable, mw, &MainWindow::updateEditorActions);
    connect(editor, &QPlainTextEdit::copyAvailable, mw, &MainWindow::updateEditorActions);

    connect(editor, &TextEdit::showSearchBar, d->searchBar, &SearchBar::activate);
    connect(editor, &TextEdit::findNext, d->searchBar, &SearchBar::findNext);
    connect(editor, &TextEdit::findPrevious, d->searchBar, &SearchBar::findPrevious);

    // Create the layout containing the workspace and a tab bar
    auto hbox = new QFrame(this);
    hbox->setFrameShape(QFrame::StyledPanel);
    hbox->setFrameShadow(QFrame::Sunken);
    auto layout = new QVBoxLayout();
    layout->setContentsMargins(1, 1, 1, 1);
    layout->addWidget(d->textEdit);
    layout->addWidget(d->searchBar);
    d->textEdit->setParent(hbox);
    d->searchBar->setParent(hbox);
    hbox->setLayout(layout);
    setCentralWidget(hbox);

    setCurrentFileName(QString());
    d->textEdit->setFocus();

    setWindowIcon(d->textEdit->windowIcon());

    ParameterGrp::handle hPrefGrp = getWindowParameter();
    hPrefGrp->Attach( this );
    hPrefGrp->NotifyAll();

    d->activityTimer = new QTimer(this);
    connect(d->activityTimer, &QTimer::timeout,
            this, &EditorView::checkTimestamp);
    connect(d->textEdit->document(), &QTextDocument::modificationChanged,
            this, &EditorView::setWindowModified);
    connect(d->textEdit->document(), &QTextDocument::undoAvailable,
            this, &EditorView::undoAvailable);
    connect(d->textEdit->document(), &QTextDocument::redoAvailable,
            this, &EditorView::redoAvailable);
    connect(d->textEdit->document(), &QTextDocument::contentsChange,
            this, &EditorView::contentsChange);
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

void EditorView::showEvent(QShowEvent* event)
{
    Gui::MainWindow* mw = Gui::getMainWindow();
    mw->updateEditorActions();
    MDIView::showEvent(event);
}

void EditorView::hideEvent(QHideEvent* event)
{
    MDIView::hideEvent(event);
}

void EditorView::closeEvent(QCloseEvent* event)
{
    MDIView::closeEvent(event);
    if (event->isAccepted()) {
        d->aboutToClose = true;
        Gui::MainWindow* mw = Gui::getMainWindow();
        mw->updateEditorActions();
    }
}

void EditorView::OnChange(Base::Subject<const char*> &rCaller,const char* rcReason)
{
    Q_UNUSED(rCaller);
    ParameterGrp::handle hPrefGrp = getWindowParameter();
    if (strcmp(rcReason, "EnableLineNumber") == 0) {
        //bool show = hPrefGrp->GetBool( "EnableLineNumber", true );
    }
}

void EditorView::checkTimestamp()
{
    QFileInfo fi(d->fileName);
    qint64 timeStamp =  fi.lastModified().toSecsSinceEpoch();
    if (timeStamp != d->timeStamp) {
        switch( QMessageBox::question( this, tr("Modified file"),
                tr("%1.\n\nThis has been modified outside of the source editor. Do you want to reload it?").arg(d->fileName),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) )
        {
            case QMessageBox::Yes:
                // updates time stamp and timer
                open( d->fileName );
                return;
            case QMessageBox::No:
                d->timeStamp = timeStamp;
                break;
            default:
                break;
        }
    }

    d->activityTimer->setSingleShot(true);
    d->activityTimer->start(3000);
}

/**
 * Runs the action specified by \a pMsg.
 */
bool EditorView::onMsg(const char* pMsg,const char** /*ppReturn*/)
{
    // don't allow any actions if the editor is being closed
    if (d->aboutToClose)
        return false;

    if (strcmp(pMsg, "Save") == 0) {
        saveFile();
        return true;
    }
    else if (strcmp(pMsg, "SaveAs") == 0) {
        saveAs();
        return true;
    }
    else if (strcmp(pMsg, "Cut") == 0) {
        cut();
        return true;
    }
    else if (strcmp(pMsg, "Copy") == 0) {
        copy();
        return true;
    }
    else if (strcmp(pMsg, "Paste") == 0) {
        paste();
        return true;
    }
    else if (strcmp(pMsg, "Undo") == 0) {
        undo();
        return true;
    }
    else if (strcmp(pMsg, "Redo") == 0) {
        redo();
        return true;
    }
    else if (strcmp(pMsg, "ViewFit") == 0) {
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
    // don't allow any actions if the editor is being closed
    if (d->aboutToClose)
        return false;
    if (strcmp(pMsg, "Run") == 0)
        return true;
    if (strcmp(pMsg, "DebugStart") == 0)
        return true;
    if (strcmp(pMsg, "DebugStop") == 0)
        return true;
    if (strcmp(pMsg, "SaveAs") == 0)
        return true;
    if (strcmp(pMsg, "Print") == 0)
        return true;
    if (strcmp(pMsg, "PrintPreview") == 0)
        return true;
    if (strcmp(pMsg, "PrintPdf") == 0)
        return true;
    if (strcmp(pMsg, "Save") == 0) {
        return d->textEdit->document()->isModified();
    }
    else if (strcmp(pMsg, "Cut") == 0) {
        bool canWrite = !d->textEdit->isReadOnly();
        return (canWrite && (d->textEdit->textCursor().hasSelection()));
    }
    else if (strcmp(pMsg, "Copy") == 0) {
        return ( d->textEdit->textCursor().hasSelection() );
    }
    else if (strcmp(pMsg, "Paste") == 0) {
        QClipboard *cb = QApplication::clipboard();
        QString text;

        // Copy text from the clipboard (paste)
        text = cb->text();

        bool canWrite = !d->textEdit->isReadOnly();
        return ( !text.isEmpty() && canWrite );
    }
    else if (strcmp(pMsg, "Undo") == 0) {
        return d->textEdit->document()->isUndoAvailable ();
    }
    else if (strcmp(pMsg, "Redo") == 0) {
        return d->textEdit->document()->isRedoAvailable ();
    }

    return false;
}

/** Checking on close state. */
bool EditorView::canClose()
{
    if ( !d->textEdit->document()->isModified() )
        return true;
    this->setFocus(); // raises the view to front
    switch( QMessageBox::question(this, tr("Unsaved document"),
                                    tr("The document has been modified.\n"
                                       "Do you want to save your changes?"),
                                     QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel, QMessageBox::Cancel))
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

void EditorView::setDisplayName(EditorView::DisplayName type)
{
    d->displayName = type;
}

/**
 * Saves the content of the editor to a file specified by the appearing file dialog.
 */
bool EditorView::saveAs()
{
    QString fn = FileDialog::getSaveFileName(this, QObject::tr("Save Macro"),
        QString(), QString::fromLatin1("%1 (*.FCMacro);;Python (*.py)").arg(tr("FreeCAD macro")));
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
    d->timeStamp =  fi.lastModified().toSecsSinceEpoch();
    d->activityTimer->setSingleShot(true);
    d->activityTimer->start(3000);

    setCurrentFileName(fileName);
    return true;
}

/**
 * Copies the selected text to the clipboard and deletes it from the text edit.
 * If there is no selected text nothing happens.
 */
void EditorView::cut()
{
    d->textEdit->cut();
}

/**
 * Copies any selected text to the clipboard.
 */
void EditorView::copy()
{
    d->textEdit->copy();
}

/**
 * Pastes the text from the clipboard into the text edit at the current cursor position.
 * If there is no text in the clipboard nothing happens.
 */
void EditorView::paste()
{
    d->textEdit->paste();
}

/**
 * Undoes the last operation.
 * If there is no operation to undo, i.e. there is no undo step in the undo/redo history, nothing happens.
 */
void EditorView::undo()
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
void EditorView::redo()
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
    connect(&dlg, &QPrintPreviewDialog::paintRequested,
            this, qOverload<QPrinter *>(&EditorView::print));
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
    QString filename = FileDialog::getSaveFileName(this, tr("Export PDF"), QString(),
        QString::fromLatin1("%1 (*.pdf)").arg(tr("PDF file")));
    if (!filename.isEmpty()) {
        QPrinter printer(QPrinter::ScreenResolution);
        // setPdfVersion sets the printied PDF Version to comply with PDF/A-1b, more details under: https://www.kdab.com/creating-pdfa-documents-qt/
        printer.setPdfVersion(QPagedPaintDevice::PdfVersion_A1b);
        printer.setOutputFormat(QPrinter::PdfFormat);
        printer.setOutputFileName(filename);
        d->textEdit->document()->print(&printer);
    }
}

void EditorView::setCurrentFileName(const QString &fileName)
{
    d->fileName = fileName;
    Q_EMIT changeFileName(d->fileName);
    d->textEdit->document()->setModified(false);

    QString name;
    QFileInfo fi(fileName);
    switch (d->displayName) {
    case FullName:
        name = fileName;
        break;
    case FileName:
        name = fi.fileName();
        break;
    case BaseName:
        name = fi.baseName();
        break;
    }

    QString shownName;
    if (fileName.isEmpty())
        shownName = tr("untitled[*]");
    else
        shownName = QString::fromLatin1("%1[*]").arg(name);
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
#if QT_VERSION < 0x060000
    ts.setCodec("UTF-8");
#endif
    ts << d->textEdit->document()->toPlainText();
    file.close();
    d->textEdit->document()->setModified(false);

    QFileInfo fi(d->fileName);
    d->timeStamp =  fi.lastModified().toSecsSinceEpoch();
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
    Q_UNUSED(position);
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

void EditorView::focusInEvent (QFocusEvent *)
{
    d->textEdit->setFocus();
}

// ---------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Gui::PythonEditorView, Gui::EditorView)

PythonEditorView::PythonEditorView(PythonEditor* editor, QWidget* parent)
  : EditorView(editor, parent), _pye(editor)
{
    connect(this, &PythonEditorView::changeFileName,
            editor, &PythonEditor::setFileName);
    watcher = new PythonTracingWatcher(this);
}

PythonEditorView::~PythonEditorView()
{
    delete watcher;
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
        QTimer::singleShot(300, this, &PythonEditorView::startDebug);
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
    if (strcmp(pMsg,"Run")==0)
        return true;
    if (strcmp(pMsg,"StartDebug")==0)
        return true;
    if (strcmp(pMsg,"ToggleBreakpoint")==0)
        return true;
    return EditorView::onHasMsg(pMsg);
}

/**
 * Runs the opened script in the macro manager.
 */
void PythonEditorView::executeScript()
{
    // always save the macro when it is modified
    if (EditorView::onHasMsg("Save"))
        EditorView::onMsg("Save", nullptr);
    try {
        WaitCursor wc;
        PythonTracingLocker tracelock(watcher->getTrace());
        Application::Instance->macroManager()->run(Gui::MacroManager::File,fileName().toUtf8());
    }
    catch (const Base::SystemExitException&) {
        // handle SystemExit exceptions
        Base::PyGILStateLocker locker;
        Base::PyException e;
        e.ReportException();
    }
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

// ----------------------------------------------------------------------------

SearchBar::SearchBar(QWidget* parent)
    : QWidget(parent)
    , textEditor(nullptr)
{
    horizontalLayout = new QHBoxLayout(this);
    horizontalLayout->setSpacing(3);

    closeButton = new QToolButton(this);
    closeButton->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    closeButton->setAutoRaise(true);
    connect(closeButton, &QToolButton::clicked, this, &SearchBar::deactivate);

    horizontalLayout->addWidget(closeButton);

    searchText = new QLineEdit(this);
    searchText->setClearButtonEnabled(true);
    horizontalLayout->addWidget(searchText);
    connect(searchText, &QLineEdit::returnPressed, this, &SearchBar::findNext);
    connect(searchText, &QLineEdit::textChanged, this, &SearchBar::findCurrent);
    connect(searchText, &QLineEdit::textChanged, this, &SearchBar::updateButtons);

    prevButton = new QToolButton(this);
    prevButton->setIcon(style()->standardIcon(QStyle::SP_ArrowBack));
    prevButton->setAutoRaise(true);
    prevButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    horizontalLayout->addWidget(prevButton);
    connect(prevButton, &QToolButton::clicked, this, &SearchBar::findPrevious);

    nextButton = new QToolButton(this);
    nextButton->setIcon(style()->standardIcon(QStyle::SP_ArrowForward));
    nextButton->setAutoRaise(true);
    nextButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    horizontalLayout->addWidget(nextButton);
    connect(nextButton, &QToolButton::clicked, this, &SearchBar::findNext);

    matchCase = new QCheckBox(this);
    horizontalLayout->addWidget(matchCase);
    connect(matchCase, &QCheckBox::toggled, this, &SearchBar::findCurrent);

    matchWord = new QCheckBox(this);
    horizontalLayout->addWidget(matchWord);
    connect(matchWord, &QCheckBox::toggled, this, &SearchBar::findCurrent);

    horizontalSpacer = new QSpacerItem(192, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    horizontalLayout->addItem(horizontalSpacer);

    retranslateUi();

    setMinimumWidth(minimumSizeHint().width());
    updateButtons();
    hide();
}

void SearchBar::setEditor(QPlainTextEdit* textEdit)
{
    textEditor = textEdit;
}

void SearchBar::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        hide();
        return;
    }

    QWidget::keyPressEvent(event);
}

void SearchBar::retranslateUi()
{
    prevButton->setText(tr("Previous"));
    nextButton->setText(tr("Next"));
    matchCase->setText(tr("Case sensitive"));
    matchWord->setText(tr("Whole words"));
}

void SearchBar::activate()
{
    show();
    searchText->selectAll();
    searchText->setFocus(Qt::ShortcutFocusReason);
}

void SearchBar::deactivate()
{
    if (textEditor)
        textEditor->setFocus();
    hide();
}

void SearchBar::findPrevious()
{
    findText(true, false, searchText->text());
}

void SearchBar::findNext()
{
    findText(true, true, searchText->text());
}

void SearchBar::findCurrent()
{
    findText(false, true, searchText->text());
}

void SearchBar::findText(bool skip, bool next, const QString& str)
{
    if (!textEditor)
        return;

    QTextCursor cursor = textEditor->textCursor();
    QTextDocument *doc = textEditor->document();
    if (!doc || cursor.isNull())
        return;

    if (cursor.hasSelection())
        cursor.setPosition((skip && next) ? cursor.position() : cursor.anchor());

    bool found = true;
    QTextCursor newCursor = cursor;
    if (!str.isEmpty()) {
        QTextDocument::FindFlags options;
        if (!next)
            options |= QTextDocument::FindBackward;
        if (matchCase->isChecked())
            options |= QTextDocument::FindCaseSensitively;
        if (matchWord->isChecked())
            options |= QTextDocument::FindWholeWords;

        newCursor = doc->find(str, cursor, options);
        if (newCursor.isNull()) {
            QTextCursor ac(doc);
            ac.movePosition(options & QTextDocument::FindBackward ? QTextCursor::End : QTextCursor::Start);
            newCursor = doc->find(str, ac, options);
            if (newCursor.isNull()) {
                found = false;
                newCursor = cursor;
            }
        }
    }

    if (!isVisible())
        show();

    textEditor->setTextCursor(newCursor);

    QString styleSheet;
    if (!found) {
        styleSheet = QString::fromLatin1(
            " QLineEdit {\n"
            "     background-color: rgb(221,144,161);\n"
            " }\n"
        );
    }

    searchText->setStyleSheet(styleSheet);
}

void SearchBar::updateButtons()
{
    bool empty = searchText->text().isEmpty();
    prevButton->setDisabled(empty);
    nextButton->setDisabled(empty);
}

void SearchBar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }

    QWidget::changeEvent(event);
}

#include "moc_EditorView.cpp"
