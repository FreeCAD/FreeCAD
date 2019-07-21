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
# include <QGridLayout>
# include <QApplication>
# include <QMenu>
# include <QContextMenuEvent>
# include <QTextCursor>
# include <QTextStream>
# include <QDockWidget>
# include <QPointer>
#endif

#include <Base/Interpreter.h>
#include "ReportView.h"
#include "FileDialog.h"
#include "PythonConsole.h"
#include "PythonConsolePy.h"
#include "BitmapFactory.h"
#include "MainWindow.h"
#include "Application.h"

using namespace Gui;
using namespace Gui::DockWnd;

/* TRANSLATOR Gui::DockWnd::ReportView */

/**
 *  Constructs a ReportView which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
ReportView::ReportView( QWidget* parent )
  : QWidget(parent)
{
    setObjectName(QLatin1String("ReportOutput"));

    resize( 529, 162 );
    QGridLayout* tabLayout = new QGridLayout( this );
    tabLayout->setSpacing( 0 );
    tabLayout->setMargin( 0 );

    tabWidget = new QTabWidget( this );
    tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
    tabWidget->setTabPosition(QTabWidget::South);
    tabWidget->setTabShape(QTabWidget::Rounded);
    tabLayout->addWidget( tabWidget, 0, 0 );


    // create the output window
    tabOutput = new ReportOutput();
    tabOutput->setWindowTitle(trUtf8("Output"));
    tabOutput->setWindowIcon(BitmapFactory().pixmap("MacroEditor"));
    int output = tabWidget->addTab(tabOutput, tabOutput->windowTitle());
    tabWidget->setTabIcon(output, tabOutput->windowIcon());

    // create the python console
    tabPython = new PythonConsole();
    tabPython->setWordWrapMode(QTextOption::NoWrap);
    tabPython->setWindowTitle(trUtf8("Python console"));
    tabPython->setWindowIcon(BitmapFactory().iconFromTheme("applications-python"));
    int python = tabWidget->addTab(tabPython, tabPython->windowTitle());
    tabWidget->setTabIcon(python, tabPython->windowIcon());
    tabWidget->setCurrentIndex(0);

    // raise the tab page set in the preferences
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    int index = hGrp->GetInt("AutoloadTab", 0);
    tabWidget->setCurrentIndex(index);
}

/**
 *  Destroys the object and frees any allocated resources
 */
ReportView::~ReportView()
{
    // no need to delete child widgets, Qt does it all for us
}

void ReportView::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        tabOutput->setWindowTitle(trUtf8("Output"));
        tabPython->setWindowTitle(trUtf8("Python console"));
        for (int i=0; i<tabWidget->count();i++)
            tabWidget->setTabText(i, tabWidget->widget(i)->windowTitle());
    }
}

// ----------------------------------------------------------

namespace Gui {
struct TextBlockData : public QTextBlockUserData
{
    struct State {
        int length;
        ReportHighlighter::Paragraph type;
    };
    QVector<State> block;
};
}

ReportHighlighter::ReportHighlighter(QTextEdit* edit)
  : QSyntaxHighlighter(edit), type(Message)
{
    QPalette pal = edit->palette();
    txtCol = pal.windowText().color();
    logCol = Qt::blue;
    warnCol = QColor(255, 170, 0);
    errCol = Qt::red;
}

ReportHighlighter::~ReportHighlighter()
{
}

void ReportHighlighter::highlightBlock (const QString & text)
{
    if (text.isEmpty())
        return;
    TextBlockData* ud = static_cast<TextBlockData*>(this->currentBlockUserData());
    if (!ud) {
        ud = new TextBlockData;
        this->setCurrentBlockUserData(ud);
    }

    TextBlockData::State b;
    b.length = text.length();
    b.type = this->type;
    ud->block.append(b);

    QVector<TextBlockData::State> block = ud->block;
    int start = 0;
    for (QVector<TextBlockData::State>::Iterator it = block.begin(); it != block.end(); ++it) {
        switch (it->type)
        {
        case Message:
            setFormat(start, it->length-start, txtCol);
            break;
        case Warning:
            setFormat(start, it->length-start, warnCol);
            break;
        case Error:
            setFormat(start, it->length-start, errCol);
            break;
        case LogText:
            setFormat(start, it->length-start, logCol);
            break;
        default:
            break;
        }

        start = it->length;
    }
}

void ReportHighlighter::setParagraphType(ReportHighlighter::Paragraph t)
{
    type = t;
}

void ReportHighlighter::setTextColor( const QColor& col )
{
    txtCol = col;
}

void ReportHighlighter::setLogColor( const QColor& col )
{
    logCol = col;
}

void ReportHighlighter::setWarningColor( const QColor& col )
{
    warnCol = col;
}

void ReportHighlighter::setErrorColor( const QColor& col )
{
    errCol = col;
}

// ----------------------------------------------------------

/**
 * The CustomReportEvent class is used to send report events in the methods Log(), 
 * Error(), Warning() and Message() of the ReportOutput class to itself instead of
 * printing the messages directly in its text view.
 *
 * This makes the methods Log(), Error(), Warning() and Message() thread-safe.
 * @author Werner Mayer
 */
class CustomReportEvent : public QEvent
{
public:
    CustomReportEvent(ReportHighlighter::Paragraph p, const QString& s)
    : QEvent(QEvent::Type(QEvent::User)) 
    { par = p; msg = s;}
    ~CustomReportEvent()
    { }
    const QString& message() const
    { return msg; }
    ReportHighlighter::Paragraph messageType() const
    { return par; }
private:
    ReportHighlighter::Paragraph par;
    QString msg;
};

// ----------------------------------------------------------

/**
 * The ReportOutputObserver class is used to check if messages sent to the
 * report view are warnings or errors, and if so and if the user has not
 * disabled this in preferences, the report view is toggled on so the
 * user always gets the warnings/errors
 */

ReportOutputObserver::ReportOutputObserver(ReportOutput *report)
{
    this->reportView = report;
}

bool ReportOutputObserver::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::User && obj == reportView.data()) {
        CustomReportEvent* cr = dynamic_cast<CustomReportEvent*>(event);
        if (cr) {
            ReportHighlighter::Paragraph msgType = cr->messageType();
            if (msgType == ReportHighlighter::Error || msgType == ReportHighlighter::Warning){
                ParameterGrp::handle group = App::GetApplication().GetUserParameter().
                        GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("OutputWindow");

                if (group->GetBool("checkShowReportViewOnWarningOrError", true)) {
                    // get the QDockWidget parent of the report view
                    QDockWidget* dw = nullptr;
                    QWidget* par = reportView->parentWidget();
                    while (par) {
                        dw = qobject_cast<QDockWidget*>(par);
                        if (dw)
                            break;
                        par = par->parentWidget();
                    }

                    if (dw && !dw->toggleViewAction()->isChecked()) {
                        dw->toggleViewAction()->activate(QAction::Trigger);
                    }
                }
            }
        }
        return false;  //true would prevent the messages reaching the report view
    }

    // standard event processing
    return QObject::eventFilter(obj, event);
}

// ----------------------------------------------------------

class ReportOutput::Data
{
public:
    Data()
    {
        if (!default_stdout) {
            Base::PyGILStateLocker lock;
            default_stdout = PySys_GetObject(const_cast<char*>("stdout"));
            replace_stdout = new OutputStdout();
            redirected_stdout = false;
        }

        if (!default_stderr) {
            Base::PyGILStateLocker lock;
            default_stderr = PySys_GetObject(const_cast<char*>("stderr"));
            replace_stderr = new OutputStderr();
            redirected_stderr = false;
        }
    }
    ~Data()
    {
        if (replace_stdout) {
            Py_DECREF(replace_stdout);
            replace_stdout = 0;
        }

        if (replace_stderr) {
            Py_DECREF(replace_stderr);
            replace_stderr = 0;
        }
    }

    // make them static because redirection should done only once
    static bool redirected_stdout;
    static PyObject* default_stdout;
    static PyObject* replace_stdout;

    static bool redirected_stderr;
    static PyObject* default_stderr;
    static PyObject* replace_stderr;
};

bool ReportOutput::Data::redirected_stdout = false;
PyObject* ReportOutput::Data::default_stdout = 0;
PyObject* ReportOutput::Data::replace_stdout = 0;

bool ReportOutput::Data::redirected_stderr = false;
PyObject* ReportOutput::Data::default_stderr = 0;
PyObject* ReportOutput::Data::replace_stderr = 0;

/* TRANSLATOR Gui::DockWnd::ReportOutput */

/**
 *  Constructs a ReportOutput which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
ReportOutput::ReportOutput(QWidget* parent)
  : QTextEdit(parent), WindowParameter("OutputWindow"), d(new Data), gotoEnd(false)
{
    bLog = false;
    reportHl = new ReportHighlighter(this);

    restoreFont();
    setReadOnly(true);
    clear();
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    Base::Console().AttachObserver(this);
    getWindowParameter()->Attach(this);

    getWindowParameter()->NotifyAll();
    _prefs = WindowParameter::getDefaultParameter()->GetGroup("Editor");
    _prefs->Attach(this);
    _prefs->Notify("FontSize");

    // scroll to bottom at startup to make sure that last appended text is visible
    ensureCursorVisible();
}

/**
 *  Destroys the object and frees any allocated resources
 */
ReportOutput::~ReportOutput()
{
    getWindowParameter()->Detach(this);
    _prefs->Detach(this);
    Base::Console().DetachObserver(this);
    delete reportHl;
    delete d;
}

void ReportOutput::restoreFont()
{
    QFont serifFont(QLatin1String("Courier"), 10, QFont::Normal);
    setFont(serifFont);
}

void ReportOutput::Warning(const char * s)
{
    // Send the event to itself to allow thread-safety. Qt will delete it when done.
    CustomReportEvent* ev = new CustomReportEvent(ReportHighlighter::Warning, QString::fromUtf8(s));
    QApplication::postEvent(this, ev);
}

void ReportOutput::Message(const char * s)
{
    // Send the event to itself to allow thread-safety. Qt will delete it when done.
    CustomReportEvent* ev = new CustomReportEvent(ReportHighlighter::Message, QString::fromUtf8(s));
    QApplication::postEvent(this, ev);
}

void ReportOutput::Error  (const char * s)
{
    // Send the event to itself to allow thread-safety. Qt will delete it when done.
    CustomReportEvent* ev = new CustomReportEvent(ReportHighlighter::Error, QString::fromUtf8(s));
    QApplication::postEvent(this, ev);
}

void ReportOutput::Log (const char * s)
{
    QString msg = QString::fromUtf8(s);
    if (msg.length() < 1000){
        // Send the event to itself to allow thread-safety. Qt will delete it when done.
        CustomReportEvent* ev = new CustomReportEvent(ReportHighlighter::LogText, msg);
        QApplication::postEvent(this, ev);
    }
}

void ReportOutput::customEvent ( QEvent* ev )
{
    // Appends the text stored in the event to the text view
    if ( ev->type() ==  QEvent::User ) {
        CustomReportEvent* ce = (CustomReportEvent*)ev;
        reportHl->setParagraphType(ce->messageType());

        QTextCursor cursor(this->document());
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(ce->message());
        cursor.endEditBlock();
        if (gotoEnd) {
            setTextCursor(cursor);
        }
        ensureCursorVisible();
    }
}

void ReportOutput::changeEvent(QEvent *ev)
{
    if (ev->type() == QEvent::StyleChange) {
        QPalette pal = palette();
        QColor color = pal.windowText().color();
        unsigned int text = (color.red() << 24) | (color.green() << 16) | (color.blue() << 8);
        unsigned long value = static_cast<unsigned long>(text);
        // if this parameter is not already set use the style's window text color
        value = getWindowParameter()->GetUnsigned("colorText", value);
        getWindowParameter()->SetUnsigned("colorText", value);
    }
    QTextEdit::changeEvent(ev);
}

void ReportOutput::contextMenuEvent ( QContextMenuEvent * e )
{
    QMenu* menu = createStandardContextMenu();
    QAction* first = menu->actions().front();

    QMenu* submenu = new QMenu( menu );
    QAction* logAct = submenu->addAction(tr("Logging"), this, SLOT(onToggleLogging()));
    logAct->setCheckable(true);
    logAct->setChecked(bLog);

    QAction* wrnAct = submenu->addAction(tr("Warning"), this, SLOT(onToggleWarning()));
    wrnAct->setCheckable(true);
    wrnAct->setChecked(bWrn);

    QAction* errAct = submenu->addAction(tr("Error"), this, SLOT(onToggleError()));
    errAct->setCheckable(true);
    errAct->setChecked(bErr);

    submenu->addSeparator();

    QAction* stdoutAct = submenu->addAction(tr("Redirect Python output"), this, SLOT(onToggleRedirectPythonStdout()));
    stdoutAct->setCheckable(true);
    stdoutAct->setChecked(d->redirected_stdout);

    QAction* stderrAct = submenu->addAction(tr("Redirect Python errors"), this, SLOT(onToggleRedirectPythonStderr()));
    stderrAct->setCheckable(true);
    stderrAct->setChecked(d->redirected_stderr);

    submenu->addSeparator();
    QAction* botAct = submenu->addAction(tr("Go to end"), this, SLOT(onToggleGoToEnd()));
    botAct->setCheckable(true);
    botAct->setChecked(gotoEnd);

    submenu->setTitle(tr("Options"));
    menu->insertMenu(first, submenu);
    menu->insertSeparator(first);

    menu->addAction(tr("Clear"), this, SLOT(clear()));
    menu->addSeparator();
    menu->addAction(tr("Save As..."), this, SLOT(onSaveAs()));

    menu->exec(e->globalPos());
    delete menu;
}

void ReportOutput::onSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save Report Output"), QString(),
        QString::fromLatin1("%1 (*.txt *.log)").arg(tr("Plain Text Files")));
    if (!fn.isEmpty()) {
        QFileInfo fi(fn);
        if (fi.completeSuffix().isEmpty())
            fn += QLatin1String(".log");
        QFile f(fn);
        if (f.open(QIODevice::WriteOnly)) {
            QTextStream t (&f);
            t << toPlainText();
            f.close();
        }
    }
}

bool ReportOutput::isError() const
{
    return bErr;
}

bool ReportOutput::isWarning() const
{
    return bWrn;
}

bool ReportOutput::isLogging() const
{
    return bLog;
}

void ReportOutput::onToggleError()
{
    bErr = bErr ? false : true;
    getWindowParameter()->SetBool( "checkError", bErr );
}

void ReportOutput::onToggleShowReportViewOnWarningOrError(){
    bool show = getWindowParameter()->GetBool("checkShowReportViewOnWarningOrError", true);
    getWindowParameter()->SetBool("checkShowReportViewOnWarningOrError", !show);
}
void ReportOutput::onToggleWarning()
{
    bWrn = bWrn ? false : true;
    getWindowParameter()->SetBool( "checkWarning", bWrn );
}

void ReportOutput::onToggleLogging()
{
    bLog = bLog ? false : true;
    getWindowParameter()->SetBool( "checkLogging", bLog );
}

void ReportOutput::onToggleRedirectPythonStdout()
{
    if (d->redirected_stdout) {
        d->redirected_stdout = false;
        Base::PyGILStateLocker lock;
        PySys_SetObject(const_cast<char*>("stdout"), d->default_stdout);
    }
    else {
        d->redirected_stdout = true;
        Base::PyGILStateLocker lock;
        PySys_SetObject(const_cast<char*>("stdout"), d->replace_stdout);
    }

    getWindowParameter()->SetBool("RedirectPythonOutput", d->redirected_stdout);
}

void ReportOutput::onToggleRedirectPythonStderr()
{
    if (d->redirected_stderr) {
        d->redirected_stderr = false;
        Base::PyGILStateLocker lock;
        PySys_SetObject(const_cast<char*>("stderr"), d->default_stderr);
    }
    else {
        d->redirected_stderr = true;
        Base::PyGILStateLocker lock;
        PySys_SetObject(const_cast<char*>("stderr"), d->replace_stderr);
    }

    getWindowParameter()->SetBool("RedirectPythonErrors", d->redirected_stderr);
}

void ReportOutput::onToggleGoToEnd()
{
    gotoEnd = gotoEnd ? false : true;
    getWindowParameter()->SetBool( "checkGoToEnd", gotoEnd );
}

void ReportOutput::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
    if (strcmp(sReason, "checkLogging") == 0) {
        bLog = rclGrp.GetBool( sReason, bLog );
    }
    else if (strcmp(sReason, "checkWarning") == 0) {
        bWrn = rclGrp.GetBool( sReason, bWrn );
    }
    else if (strcmp(sReason, "checkError") == 0) {
        bErr = rclGrp.GetBool( sReason, bErr );
    }
    else if (strcmp(sReason, "colorText") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        reportHl->setTextColor( QColor( (col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff) );
    }
    else if (strcmp(sReason, "colorLogging") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        reportHl->setLogColor( QColor( (col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff) );
    }
    else if (strcmp(sReason, "colorWarning") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        reportHl->setWarningColor( QColor( (col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff) );
    }
    else if (strcmp(sReason, "colorError") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        reportHl->setErrorColor( QColor( (col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff) );
    }
    else if (strcmp(sReason, "checkGoToEnd") == 0) {
        gotoEnd = rclGrp.GetBool(sReason, gotoEnd);
    }
    else if (strcmp(sReason, "FontSize") == 0 || strcmp(sReason, "Font") == 0) {
        int fontSize = rclGrp.GetInt("FontSize", 10);
        QString fontFamily = QString::fromLatin1(rclGrp.GetASCII("Font", "Courier").c_str());
        
        QFont font(fontFamily, fontSize);
        setFont(font);
        QFontMetrics metric(font);
        int width = metric.width(QLatin1String("0000"));
        setTabStopWidth(width);
    }
    else if (strcmp(sReason, "RedirectPythonOutput") == 0) {
        bool checked = rclGrp.GetBool(sReason, true);
        if (checked != d->redirected_stdout)
            onToggleRedirectPythonStdout();
    }
    else if (strcmp(sReason, "RedirectPythonErrors") == 0) {
        bool checked = rclGrp.GetBool(sReason, true);
        if (checked != d->redirected_stderr)
            onToggleRedirectPythonStderr();
    }
}

#include "moc_ReportView.cpp"
