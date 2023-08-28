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
# include <QApplication>
# include <QContextMenuEvent>
# include <QGridLayout>
# include <QMenu>
# include <QTextCursor>
# include <QTextStream>
# include <QTime>
#endif

#include <Base/Interpreter.h>
#include <App/Color.h>

#include "ReportView.h"
#include "Application.h"
#include "BitmapFactory.h"
#include "DockWindowManager.h"
#include "FileDialog.h"
#include "PythonConsole.h"
#include "PythonConsolePy.h"
#include "Tools.h"


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
    auto tabLayout = new QGridLayout( this );
    tabLayout->setSpacing( 0 );
    tabLayout->setContentsMargins( 0, 0, 0, 0 );

    tabWidget = new QTabWidget( this );
    tabWidget->setObjectName(QString::fromUtf8("tabWidget"));
    tabWidget->setTabPosition(QTabWidget::South);
    tabWidget->setTabShape(QTabWidget::Rounded);
    tabLayout->addWidget( tabWidget, 0, 0 );


    // create the output window for 'Report view'
    tabOutput = new ReportOutput();
    tabOutput->setWindowTitle(tr("Output"));
    tabOutput->setWindowIcon(BitmapFactory().pixmap("MacroEditor"));
    int output = tabWidget->addTab(tabOutput, tabOutput->windowTitle());
    tabWidget->setTabIcon(output, tabOutput->windowIcon());

    // create the python console
    tabPython = new PythonConsole();
    tabPython->setWordWrapMode(QTextOption::NoWrap);
    tabPython->setWindowTitle(tr("Python console"));
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
ReportView::~ReportView() = default;

void ReportView::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        tabOutput->setWindowTitle(tr("Output"));
        tabPython->setWindowTitle(tr("Python console"));
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

ReportHighlighter::~ReportHighlighter() = default;

void ReportHighlighter::highlightBlock (const QString & text)
{
    if (text.isEmpty())
        return;
    auto ud = static_cast<TextBlockData*>(this->currentBlockUserData());
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
    for (const auto & it : block) {
        switch (it.type)
        {
        case Message:
            setFormat(start, it.length-start, txtCol);
            break;
        case Warning:
            setFormat(start, it.length-start, warnCol);
            break;
        case Error:
            setFormat(start, it.length-start, errCol);
            break;
        case LogText:
            setFormat(start, it.length-start, logCol);
            break;
        case Critical:
            setFormat(start, it.length-start, criticalCol);
            break;
        default:
            break;
        }

        start = it.length;
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

void ReportHighlighter::setCriticalColor( const QColor& col )
{
    criticalCol = col;
}

// ----------------------------------------------------------------------------

namespace Gui {
class ReportOutputParameter
{
public:
    static bool showOnLogMessage()
    {
        return getGroup()->GetBool("checkShowReportViewOnLogMessage", false);
    }
    static void toggleShowOnLogMessage()
    {
        bool show = showOnLogMessage();
        getGroup()->SetBool("checkShowReportViewOnLogMessage", !show);
    }
    static bool showOnMessage()
    {
        return getGroup()->GetBool("checkShowReportViewOnNormalMessage", false);
    }
    static void toggleShowOnMessage()
    {
        bool show = showOnMessage();
        getGroup()->SetBool("checkShowReportViewOnNormalMessage", !show);
    }
    static bool showOnWarning()
    {
        return getGroup()->GetBool("checkShowReportViewOnWarning", false);
    }
    static void toggleShowOnWarning()
    {
        bool show = showOnWarning();
        getGroup()->SetBool("checkShowReportViewOnWarning", !show);
    }
    static bool showOnError()
    {
        return getGroup()->GetBool("checkShowReportViewOnError", true);
    }
    static void toggleShowOnError()
    {
        bool show = showOnError();
        getGroup()->SetBool("checkShowReportViewOnError", !show);
    }
    static bool showOnCritical()
    {
        return getGroup()->GetBool("checkShowReportViewOnCritical", false);
    }
    static void toggleShowOnCritical()
    {
        bool show = showOnMessage();
        getGroup()->SetBool("checkShowReportViewOnCritical", !show);
    }

private:
    static ParameterGrp::handle getGroup()
    {
        return App::GetApplication().GetUserParameter().
                GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("OutputWindow");
    }
};
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
    ~CustomReportEvent() override = default;
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
  : QObject(report)
{
    this->reportView = report;
}

void ReportOutputObserver::showReportView()
{
    // get the QDockWidget parent of the report view
    DockWindowManager::instance()->activate(reportView);
}

bool ReportOutputObserver::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::User && obj == reportView.data()) {
        auto cr = dynamic_cast<CustomReportEvent*>(event);
        if (cr) {
            ReportHighlighter::Paragraph msgType = cr->messageType();
            if (msgType == ReportHighlighter::Warning) {
                if (ReportOutputParameter::showOnWarning()) {
                    showReportView();
                }
            }
            else if (msgType == ReportHighlighter::Error) {
                if (ReportOutputParameter::showOnError()) {
                    showReportView();
                }
            }
            else if (msgType == ReportHighlighter::Message) {
                if (ReportOutputParameter::showOnMessage()) {
                    showReportView();
                }
            }
            else if (msgType == ReportHighlighter::LogText) {
                if (ReportOutputParameter::showOnLogMessage()) {
                    showReportView();
                }
            }
            else if (msgType == ReportHighlighter::Critical) {
                if (ReportOutputParameter::showOnCritical()) {
                    showReportView();
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
            default_stdout = PySys_GetObject("stdout");
            replace_stdout = new OutputStdout();
            redirected_stdout = false;
        }

        if (!default_stderr) {
            Base::PyGILStateLocker lock;
            default_stderr = PySys_GetObject("stderr");
            replace_stderr = new OutputStderr();
            redirected_stderr = false;
        }
    }
    ~Data()
    {
        if (replace_stdout) {
            Py_DECREF(replace_stdout);
            replace_stdout = nullptr;
        }

        if (replace_stderr) {
            Py_DECREF(replace_stderr);
            replace_stderr = nullptr;
        }
    }

    // make them static because redirection should done only once
    static bool redirected_stdout;
    static PyObject* default_stdout;
    static PyObject* replace_stdout;

    static bool redirected_stderr;
    static PyObject* default_stderr;
    static PyObject* replace_stderr;
#ifdef FC_DEBUG
    long logMessageSize = 0;
#else
    long logMessageSize = 2048;
#endif
};

bool ReportOutput::Data::redirected_stdout = false;
PyObject* ReportOutput::Data::default_stdout = nullptr;
PyObject* ReportOutput::Data::replace_stdout = nullptr;

bool ReportOutput::Data::redirected_stderr = false;
PyObject* ReportOutput::Data::default_stderr = nullptr;
PyObject* ReportOutput::Data::replace_stderr = nullptr;

/* TRANSLATOR Gui::DockWnd::ReportOutput */

/**
 *  Constructs a ReportOutput which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
ReportOutput::ReportOutput(QWidget* parent)
  : QTextEdit(parent)
  , WindowParameter("OutputWindow")
  , d(new Data)
  , gotoEnd(false)
  , blockStart(true)
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
    // do this explicitly because the keys below might not yet be part of a group
    getWindowParameter()->Notify("RedirectPythonOutput");
    getWindowParameter()->Notify("RedirectPythonErrors");

    _prefs = WindowParameter::getDefaultParameter()->GetGroup("Editor");
    _prefs->Attach(this);
    _prefs->Notify("FontSize");

    messageSize = _prefs->GetInt("LogMessageSize", d->logMessageSize);

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

void ReportOutput::SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level,
                           Base::IntendedRecipient recipient, Base::ContentType content)
{
    (void) notifiername;

    // Do not log translated messages, or messages intended only to the user to the Report View
    if( recipient == Base::IntendedRecipient::User ||
        content == Base::ContentType::Translated)
        return;

    ReportHighlighter::Paragraph style = ReportHighlighter::LogText;
    switch (level) {
        case Base::LogStyle::Warning:
            style = ReportHighlighter::Warning;
            break;
        case Base::LogStyle::Message:
            style = ReportHighlighter::Message;
            break;
        case Base::LogStyle::Error:
            style = ReportHighlighter::Error;
            break;
        case Base::LogStyle::Log:
            style = ReportHighlighter::LogText;
            break;
        case Base::LogStyle::Critical:
            style = ReportHighlighter::Critical;
            break;
        default:
            break;
    }

    QString qMsg = QString::fromUtf8(msg.c_str());

    // This truncates log messages that are too long
    if (style == ReportHighlighter::LogText) {
        if (messageSize > 0 && qMsg.size()>messageSize) {
            qMsg.truncate(messageSize);
            qMsg += QString::fromLatin1("...\n");
        }
    }

    // Send the event to itself to allow thread-safety. Qt will delete it when done.
    auto ev = new CustomReportEvent(style, qMsg);
    QApplication::postEvent(this, ev);
}

void ReportOutput::customEvent ( QEvent* ev )
{
    // Appends the text stored in the event to the text view
    if ( ev->type() ==  QEvent::User ) {
        auto ce = static_cast<CustomReportEvent*>(ev);
        reportHl->setParagraphType(ce->messageType());

        bool showTimecode = getWindowParameter()->GetBool("checkShowReportTimecode", true);
        QString text = ce->message();

        // The time code can only be set when the cursor is at the block start
        if (showTimecode && blockStart) {
            QTime time = QTime::currentTime();
            text.prepend(time.toString(QLatin1String("hh:mm:ss  ")));
        }

        QTextCursor cursor(this->document());
        cursor.beginEditBlock();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(text);
        cursor.endEditBlock();

        blockStart = cursor.atBlockStart();
        if (gotoEnd) {
            setTextCursor(cursor);
        }
        ensureCursorVisible();
    }
}


bool ReportOutput::event(QEvent* event)
{
    if (event && event->type() == QEvent::ShortcutOverride) {
        auto kevent = static_cast<QKeyEvent*>(event);
        if (kevent == QKeySequence::Copy)
            kevent->accept();
    }
    return QTextEdit::event(event);
}

void ReportOutput::changeEvent(QEvent *ev)
{
    if (ev->type() == QEvent::StyleChange) {
        QPalette pal = qApp->palette();
        QColor color = pal.windowText().color();
        unsigned int text = App::Color::asPackedRGB<QColor>(color);
        auto value = static_cast<unsigned long>(text);
        // if this parameter is not already set use the style's window text color
        value = getWindowParameter()->GetUnsigned("colorText", value);
        getWindowParameter()->SetUnsigned("colorText", value);
    }
    QTextEdit::changeEvent(ev);
}

void ReportOutput::contextMenuEvent ( QContextMenuEvent * e )
{
    bool bShowOnLog = ReportOutputParameter::showOnLogMessage();
    bool bShowOnNormal = ReportOutputParameter::showOnMessage();
    bool bShowOnWarn = ReportOutputParameter::showOnWarning();
    bool bShowOnError = ReportOutputParameter::showOnError();
    bool bShowOnCritical = ReportOutputParameter::showOnCritical();

    auto menu = new QMenu(this);
    auto optionMenu = new QMenu( menu );
    optionMenu->setTitle(tr("Options"));
    menu->addMenu(optionMenu);
    menu->addSeparator();

    auto displayMenu = new QMenu(optionMenu);
    displayMenu->setTitle(tr("Display message types"));
    optionMenu->addMenu(displayMenu);

    QAction* logMsg = displayMenu->addAction(tr("Normal messages"), this, &ReportOutput::onToggleNormalMessage);
    logMsg->setCheckable(true);
    logMsg->setChecked(bMsg);

    QAction* logAct = displayMenu->addAction(tr("Log messages"), this, &ReportOutput::onToggleLogMessage);
    logAct->setCheckable(true);
    logAct->setChecked(bLog);

    QAction* wrnAct = displayMenu->addAction(tr("Warnings"), this, &ReportOutput::onToggleWarning);
    wrnAct->setCheckable(true);
    wrnAct->setChecked(bWrn);

    QAction* errAct = displayMenu->addAction(tr("Errors"), this, &ReportOutput::onToggleError);
    errAct->setCheckable(true);
    errAct->setChecked(bErr);

    QAction* logCritical = displayMenu->addAction(tr("Critical messages"), this, &ReportOutput::onToggleCritical);
    logCritical->setCheckable(true);
    logCritical->setChecked(bCritical);

    auto showOnMenu = new QMenu (optionMenu);
    showOnMenu->setTitle(tr("Show Report view on"));
    optionMenu->addMenu(showOnMenu);

    QAction* showNormAct = showOnMenu->addAction(tr("Normal messages"), this, &ReportOutput::onToggleShowReportViewOnNormalMessage);
    showNormAct->setCheckable(true);
    showNormAct->setChecked(bShowOnNormal);

    QAction* showLogAct = showOnMenu->addAction(tr("Log messages"), this, &ReportOutput::onToggleShowReportViewOnLogMessage);
    showLogAct->setCheckable(true);
    showLogAct->setChecked(bShowOnLog);

    QAction* showWrnAct = showOnMenu->addAction(tr("Warnings"), this, &ReportOutput::onToggleShowReportViewOnWarning);
    showWrnAct->setCheckable(true);
    showWrnAct->setChecked(bShowOnWarn);

    QAction* showErrAct = showOnMenu->addAction(tr("Errors"), this, &ReportOutput::onToggleShowReportViewOnError);
    showErrAct->setCheckable(true);
    showErrAct->setChecked(bShowOnError);

    QAction* showCriticalAct = showOnMenu->addAction(tr("Critical messages"), this, SLOT(onToggleShowReportViewOnCritical()));
    showCriticalAct->setCheckable(true);
    showCriticalAct->setChecked(bShowOnCritical);

    optionMenu->addSeparator();

    QAction* stdoutAct = optionMenu->addAction(tr("Redirect Python output"), this, &ReportOutput::onToggleRedirectPythonStdout);
    stdoutAct->setCheckable(true);
    stdoutAct->setChecked(d->redirected_stdout);

    QAction* stderrAct = optionMenu->addAction(tr("Redirect Python errors"), this, &ReportOutput::onToggleRedirectPythonStderr);
    stderrAct->setCheckable(true);
    stderrAct->setChecked(d->redirected_stderr);

    optionMenu->addSeparator();
    QAction* botAct = optionMenu->addAction(tr("Go to end"), this, &ReportOutput::onToggleGoToEnd);
    botAct->setCheckable(true);
    botAct->setChecked(gotoEnd);

    // Use Qt's internal translation of the Copy & Select All commands
    const char* context = "QWidgetTextControl";
    QString copyStr = QCoreApplication::translate(context, "&Copy");
    QAction* copy = menu->addAction(copyStr, this, &ReportOutput::copy);
    copy->setShortcut(QKeySequence(QKeySequence::Copy));
    copy->setEnabled(textCursor().hasSelection());
    QIcon icon = QIcon::fromTheme(QString::fromLatin1("edit-copy"));
    if (!icon.isNull())
        copy->setIcon(icon);

    menu->addSeparator();
    QString selectStr = QCoreApplication::translate(context, "Select All");
    QAction* select = menu->addAction(selectStr, this, &ReportOutput::selectAll);
    select->setShortcut(QKeySequence(QKeySequence::SelectAll));

    menu->addAction(tr("Clear"), this, &ReportOutput::clear);
    menu->addSeparator();
    menu->addAction(tr("Save As..."), this, &ReportOutput::onSaveAs);

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

bool ReportOutput::isLogMessage() const
{
    return bLog;
}

bool ReportOutput::isNormalMessage() const
{
    return bMsg;
}


bool ReportOutput::isCritical() const
{
    return bCritical;
}

void ReportOutput::onToggleError()
{
    bErr = bErr ? false : true;
    getWindowParameter()->SetBool( "checkError", bErr );
}

void ReportOutput::onToggleWarning()
{
    bWrn = bWrn ? false : true;
    getWindowParameter()->SetBool( "checkWarning", bWrn );
}

void ReportOutput::onToggleLogMessage()
{
    bLog = bLog ? false : true;
    getWindowParameter()->SetBool( "checkLogging", bLog );
}

void ReportOutput::onToggleNormalMessage()
{
    bMsg = bMsg ? false : true;
    getWindowParameter()->SetBool( "checkMessage", bMsg );
}

void ReportOutput::onToggleCritical()
{
    bCritical = bCritical ? false : true;
    getWindowParameter()->SetBool( "checkCritical", bCritical );
}

void ReportOutput::onToggleShowReportViewOnWarning()
{
    ReportOutputParameter::toggleShowOnWarning();
}

void ReportOutput::onToggleShowReportViewOnError()
{
    ReportOutputParameter::toggleShowOnError();
}

void ReportOutput::onToggleShowReportViewOnNormalMessage()
{
    ReportOutputParameter::toggleShowOnMessage();
}

void ReportOutput::onToggleShowReportViewOnCritical()
{
    ReportOutputParameter::toggleShowOnCritical();
}

void ReportOutput::onToggleShowReportViewOnLogMessage()
{
    ReportOutputParameter::toggleShowOnLogMessage();
}

void ReportOutput::onToggleRedirectPythonStdout()
{
    if (d->redirected_stdout) {
        d->redirected_stdout = false;
        Base::PyGILStateLocker lock;
        PySys_SetObject("stdout", d->default_stdout);
    }
    else {
        d->redirected_stdout = true;
        Base::PyGILStateLocker lock;
        PySys_SetObject("stdout", d->replace_stdout);
    }

    getWindowParameter()->SetBool("RedirectPythonOutput", d->redirected_stdout);
}

void ReportOutput::onToggleRedirectPythonStderr()
{
    if (d->redirected_stderr) {
        d->redirected_stderr = false;
        Base::PyGILStateLocker lock;
        PySys_SetObject("stderr", d->default_stderr);
    }
    else {
        d->redirected_stderr = true;
        Base::PyGILStateLocker lock;
        PySys_SetObject("stderr", d->replace_stderr);
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
    else if (strcmp(sReason, "checkMessage") == 0) {
        bMsg = rclGrp.GetBool( sReason, bMsg );
    }
    else if (strcmp(sReason, "checkCritical") == 0) {
        bMsg = rclGrp.GetBool( sReason, bMsg );
    }
    else if (strcmp(sReason, "colorText") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        reportHl->setTextColor(App::Color::fromPackedRGB<QColor>(col));
    }
    else if (strcmp(sReason, "colorCriticalText") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        reportHl->setTextColor( QColor( (col >> 24) & 0xff,(col >> 16) & 0xff,(col >> 8) & 0xff) );
    }
    else if (strcmp(sReason, "colorLogging") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        reportHl->setLogColor(App::Color::fromPackedRGB<QColor>(col));
    }
    else if (strcmp(sReason, "colorWarning") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        reportHl->setWarningColor(App::Color::fromPackedRGB<QColor>(col));
    }
    else if (strcmp(sReason, "colorError") == 0) {
        unsigned long col = rclGrp.GetUnsigned( sReason );
        reportHl->setErrorColor(App::Color::fromPackedRGB<QColor>(col));
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
        int width = QtTools::horizontalAdvance(metric, QLatin1String("0000"));
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
        setTabStopWidth(width);
#else
        setTabStopDistance(width);
#endif
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
    else if (strcmp(sReason, "LogMessageSize") == 0) {
        messageSize = rclGrp.GetInt(sReason, d->logMessageSize);
    }
}

#include "moc_ReportView.cpp"
