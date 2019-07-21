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

#include <QTextEdit>
#include <QSyntaxHighlighter>
#include <Base/Console.h>
#include <QPointer>
#include <QDockWidget>
#include "DockWindow.h"
#include "Window.h"

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
    ReportView( QWidget* parent = 0);
    ~ReportView();

protected:
    void changeEvent(QEvent *e);

private:
    QTabWidget* tabWidget;
    ReportOutput* tabOutput; /**< Output window */
    PythonConsole* tabPython; /**< Python console */
};

/** Syntax highlighter to write log or normal messages, warnings and errors in different colors.
 * \author Werner Mayer
 */
class GuiExport ReportHighlighter : public QSyntaxHighlighter
{
public: 
    enum Paragraph { 
        Message  = 0, /**< normal text */
        Warning  = 1, /**< Warning */
        Error    = 2, /**< Error text */
        LogText  = 3  /**< Log text */
    };

public:
    ReportHighlighter(QTextEdit* );
    ~ReportHighlighter();

    /** Parses the given text and highlight it in the right colors. */
    void highlightBlock ( const QString & text );
    /** 
     * Sets the current paragraph type used in ReportOutput
     * @see ReportOutput::Message
     * @see ReportOutput::Warning
     * @see ReportOutput::Error
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

private:
    /** @name for internal use only */
    //@{
    Paragraph type;
    QColor txtCol, logCol, warnCol, errCol;
    //@}
};

/** Output window to show messages.
 * @see Base::ConsoleObserver
 * @see QTextEdit
 * \author Werner Mayer
 */
class GuiExport ReportOutput : public QTextEdit, public WindowParameter, public Base::ConsoleObserver
{
    Q_OBJECT

public:
    ReportOutput(QWidget* parent=0);
    virtual ~ReportOutput();

    /** Observes its parameter group. */
    void OnChange(Base::Subject<const char*> &rCaller, const char * sReason);

    /** Writes warnings */
    void Warning(const char * s);
    /** Writes normal text */
    void Message(const char * s);
    /** Writes errors */
    void Error  (const char * s);
    /** Does not do anything */
    void Log (const char * s);

    /// returns the name for observer handling
    const char* Name(void){return "ReportOutput";}

    /** Restore the default font settings. */
    void restoreFont ();

    /** Returns true whether errors are reported. */ 
    bool isError() const;
    /** Returns true whether warnings are reported. */ 
    bool isWarning() const;
    /** Returns true whether log messages are reported. */ 
    bool isLogging() const;

protected:
    /** For internal use only */
    void customEvent ( QEvent* ev );
    /** Handles the change of style sheets */
    void changeEvent(QEvent *);
    /** Pops up the context menu with some extensions */
    void contextMenuEvent ( QContextMenuEvent* e );

public Q_SLOTS:
    /** Save the report messages into a file. */
    void onSaveAs();
    /** Toggles the report of errors. */
    void onToggleError();
    /** Toggles the report of warnings. */
    void onToggleWarning();
    /** Toggles the report of log messages. */
    void onToggleLogging();
    /** Toggles whether to show report view on warnings or errors */
    void onToggleShowReportViewOnWarningOrError();
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
    ReportHighlighter* reportHl; /**< Syntax highlighter */
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
    ReportOutputObserver (ReportOutput* view);
    bool eventFilter(QObject *obj, QEvent *event);

protected:
    QPointer <ReportOutput> reportView;
};

} // namespace DockWnd
} // namespace Gui

#endif //GUI_DOCKWND_REPORTVIEW_H
