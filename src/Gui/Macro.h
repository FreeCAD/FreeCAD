/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#pragma once

#include <stack>
#include <tuple>
#include <QString>
#include <QStringList>
#include <Base/Observer.h>
#include <Base/Parameter.h>


namespace Gui
{
struct ApplicationP;
class PythonConsole;
class PythonDebugger;

class MacroFile
{
public:
    MacroFile();
    void open(const char* sName);
    /// indicates if a macro recording is in progress
    bool isOpen() const
    {
        return openMacro;
    }
    void append(const QString&);
    void append(const QStringList&);
    QString fileName() const
    {
        return macroName;
    }
    bool commit();
    void cancel();

private:
    QStringList macroInProgress; /**< Container for the macro */
    QString macroName;           /**< name of the macro */
    bool openMacro {false};
};

class MacroOutputBuffer
{
public:
    MacroOutputBuffer();
    /// Return the added lines regardless of recording or not
    long getLines() const
    {
        return totalLines;
    }
    /// insert a new pending line in the macro
    void addPendingLine(int type, const char* line);
    bool addPendingLineIfComment(int type, const char* line);
    bool hasPendingLines() const
    {
        return !pendingLine.empty();
    }
    void incrementIfNoComment(int type);

    long totalLines {0};
    std::vector<std::pair<int, std::string>> pendingLine;
};

class MacroOutputOption
{
public:
    MacroOutputOption();
    std::tuple<bool, bool> values(int type) const;

    static bool isComment(int type);
    static bool isGuiCommand(int type);
    static bool isAppCommand(int type);

    bool recordGui {true};
    bool guiAsComment {true};
    bool scriptToPyConsole {true};
};

/** Macro recording and play back management
 * The purpos of this class is to handle record function calls from a command and save it in
 * a macro file (so far).
 * \author Jürgen Riegel
 */
class GuiExport MacroManager: public Base::Observer<const char*>
{
protected:
    MacroManager();
    ~MacroManager() override;

public:
    /** Macro type enumeration  */
    enum MacroType
    {
        File, /**< The macro will be saved in a file */
        User, /**< The macro belongs to the Application and will be saved in the UserParameter */
        Doc /**< The macro belongs to the Document and will be saved and restored with the Document */
    };

    /** Line type enumeration  */
    enum LineType
    {
        App, /**< The line effects only the document and Application (FreeCAD) */
        Gui, /**< The line effects the Gui (FreeCADGui) */
        Cmt, /**< The line is handled as a comment */
    };

    /**  Redirect the macro output temporarily.
     *
     * This is a RAII class to redirect the macro output to a function.
     * Initializing an instance with a redirect function, ensures that
     * MacroManager::addLine() calls are redirected by means of the redirect
     * function.  More than one instances can be used, overriding earlier
     * instances and their redirect functions.
     *
     * @code
     * void myRedirectFunc1(MacroManager::LineType type, const char* line)
     * { // do something with the line }
     *
     * void myRedirectFunc2(MacroManager::LineType type, const char* line)
     * { // do something else with the line }
     *
     * {
     *   MacroRedirector redirect(myRedirectFunc1);
     *   // all macro output will go to myRedirectFunc1
     *   {
     *     MacroRedirector redirect2(myRedirectFunc2);
     *     // all macro output will go to myRedirectFunc2
     *   }
     *   // all macro output will go to myRedirectFunc1
     * }
     * // normal macro output is restored
     * @endcode
     */
    class MacroRedirector
    {
    public:
        explicit MacroRedirector(const std::function<void(LineType, const char*)>& func)
        {
            MacroManager::redirectFuncs.push(func);
        }

        ~MacroRedirector()
        {
            MacroManager::redirectFuncs.pop();
        }
    };

    /** Opens a new Macro recording session
     * Starts a session with the type and the name of the macro.
     * All user interactions will be recorded as long as the commit() or cancel() isn't called.
     * There is only one recording session possible. Trying to open a second one causes an exception:
     * @param eType Type of the macro
     * @param sName Name or path of the macro
     * @see commit()
     * @see cancel()
     */
    void open(MacroType eType, const char* sName);
    /// close (and save) the recording session
    void commit();
    /// cancels the recording session
    void cancel();
    /// indicates if a macro recording is in progress
    bool isOpen() const
    {
        return macroFile.isOpen();
    }

    /** Insert a new line in the macro.
     *
     * The line is added to the macro unless the output is redirected.
     * @see MacroRedirector.
     *
     * @param Type The type of the line
     * @param sLine The line to add
     */
    void addLine(LineType Type, const char* sLine);
    /// insert a new pending line in the macro
    void addPendingLine(LineType type, const char* line);
    /** Set the active module
     * This is normally done by the workbench switch. It sets
     * the actually active application module so when the macro
     * gets started the right import can be issued.
     */
    void setModule(const char* sModule);
    void run(MacroType eType, const char* sName);
    /// Get the Python debugger
    PythonDebugger* debugger() const;
    PythonConsole* getPythonConsole() const;
    /** Observes its parameter group. */
    void OnChange(Base::Subject<const char*>& rCaller, const char* sReason) override;

    /// Return the added lines regardless of recording or not
    long getLines() const
    {
        return buffer.getLines();
    }

private:
    void processPendingLines();
    void makeComment(QStringList& lines) const;
    void addToOutput(LineType type, const char* line);

private:
    MacroFile macroFile;
    MacroOutputBuffer buffer;
    MacroOutputOption option;
    bool localEnv {true};
    mutable PythonConsole* pyConsole {nullptr};  // link to the python console
    PythonDebugger* pyDebugger;
    Base::Reference<ParameterGrp> params;  // link to the Macro parameter group
    static std::stack<std::function<void(LineType, const char*)>> redirectFuncs;

    friend struct ApplicationP;
};

}  // namespace Gui
