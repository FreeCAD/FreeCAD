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

#ifndef GUI_DOCKWND_REPORTVIEW_H
#define GUI_DOCKWND_REPORTVIEW_H

#include <QPointer>
#include <QTextEdit>
#include <QSyntaxHighlighter>

#include "Window.h"
#include <FCGlobal.h>


class QTabWidget;

namespace Gui {
class PythonConsole;
namespace DockWnd {

class ReportOutput;
class ReportHighlighter;
class ReportOutputObserver;

/** Report view containing an output window and a very simple Python console.
 * @see ReportOutput
 * @see PythonConsole
 * \author Werner Mayer
 */
class ReportView : public QWidget
{
    Q_OBJECT

public:
    explicit ReportView( QWidget* parent = nullptr);
    ~ReportView() override;

protected:
    void changeEvent(QEvent *e) override;

private:
    QTabWidget* tabWidget;
    ReportOutput* tabOutput; /**< Output 'Report view' window */
    PythonConsole* tabPython; /**< Python console */
};

/** Syntax highlighter to write log or normal messages, warnings and errors in different colors.
 * \author Werner Mayer
 */
class GuiExport ReportHighlighter : public QSyntaxHighlighter
{
public:
    enum Paragraph {
        Message          = 0, /**< normal text */
        Warning          = 1, /**< Warning */
        Error            = 2, /**< Error text */
        LogText          = 3,  /**< Log text */
        Critical         = 4, /**< critical text */
    };

public:
    explicit ReportHighlighter(QTextEdit* );
    ~ReportHighlighter() override;

    /** Parses the given text and highlight it in the right colors. */
    void highlightBlock ( const QString & text ) override;
    /**
     * Sets the current paragraph type used in ReportOutput
     * @see ReportOutput::Message
     * @see ReportOutput::Warning
     * @see ReportOutput::Error
     * @see ReportOutput::Critical
     */
    void setParagraphType(Paragraph);

    /**
     * Sets the text color to  \a col.
     */
    void setTextColor( const QColor& col );

    /**
     * Sets the color for log messages to  \a col.
     */
    void setLogColor( const QColor& col );

    /**
     * Sets the color for warnings to  \a col.
     */
    void setWarningColor( const QColor& col );

    /**
     * Sets the color for error messages to  \a col.
     */
    void setErrorColor( const QColor& col );

    /**
     * Sets the text color to  \a col.
     */
    void setCriticalColor( const QColor& col );

private:
    /** @name for internal use only */
    //@{
    Paragraph type;
    QColor txtCol, logCol, warnCol, errCol, criticalCol;
    //@}
};

/** Output window 'Report view' to show messages.
 * @see Base::ILogger
 * @see QTextEdit
 * \author Werner Mayer
 */
class GuiExport ReportOutput : public QTextEdit, public WindowParameter, public Base::ILogger
{
    Q_OBJECT

public:
    explicit ReportOutput(QWidget* parent=nullptr);
    ~ReportOutput() override;

    /** Observes its parameter group. */
    void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;

    void SendLog(const std::string& notifiername, const std::string& msg, Base::LogStyle level,
                 Base::IntendedRecipient recipient, Base::ContentType content) override;

    /// returns the name for observer handling
    const char* Name() override {return "ReportOutput";}

    /** Restore the default font settings. */
    void restoreFont ();

    /** Returns true whether errors are reported. */
    bool isError() const;
    /** Returns true whether warnings are reported. */
    bool isWarning() const;
    /** Returns true whether log messages are reported. */
    bool isLogMessage() const;
    /** Returns true whether normal messages are reported. */
    bool isNormalMessage() const;
    /** Returns true whether critical messages are reported. */
    bool isCritical() const;

protected:
    /** For internal use only */
    void customEvent ( QEvent* ev ) override;
    /** Handles the change of style sheets */
    void changeEvent(QEvent *) override;
    /** Pops up the context menu with some extensions */
    void contextMenuEvent ( QContextMenuEvent* e ) override;
    /** Handle shortcut override events */
    bool event(QEvent* event) override;

public Q_SLOTS:
    /** Save the report messages into a file. */
    void onSaveAs();
    /** Toggles the report of errors. */
    void onToggleError();
    /** Toggles the report of warnings. */
    void onToggleWarning();
    /** Toggles the report of log messages. */
    void onToggleLogMessage();
    /** Toggles the report of normal messages. */
    void onToggleNormalMessage();
    /** Toggles the report of normal messages. */
    void onToggleCritical();
    /** Toggles whether to show report view on warnings*/
    void onToggleShowReportViewOnWarning();
    /** Toggles whether to show report view on errors*/
    void onToggleShowReportViewOnError();
    /** Toggles whether to show report view on normal messages*/
    void onToggleShowReportViewOnNormalMessage();
    /** Toggles whether to show report view on normal messages*/
    void onToggleShowReportViewOnCritical();
    /** Toggles whether to show report view on log messages*/
    void onToggleShowReportViewOnLogMessage();
    /** Toggles the redirection of Python stdout. */
    void onToggleRedirectPythonStdout();
    /** Toggles the redirection of Python stderr. */
    void onToggleRedirectPythonStderr();
    /** Toggles the report to go to the end if new messages appear. */
    void onToggleGoToEnd();

private:
    class Data;
    Data* d;
    bool gotoEnd;
    bool blockStart;
    ReportHighlighter* reportHl; /**< Syntax highlighter */
    int messageSize;
    ParameterGrp::handle _prefs;
};

/**
 * Observer to enable report view on warnings / errors if not already
 * enabled.
 */

class ReportOutputObserver : public QObject
{
    Q_OBJECT

public:
    explicit ReportOutputObserver (ReportOutput* view);
    bool eventFilter(QObject *obj, QEvent *event) override;

protected:
    QPointer <ReportOutput> reportView;
    void showReportView();
};

} // namespace DockWnd
} // namespace Gui

#endif //GUI_DOCKWND_REPORTVIEW_H
