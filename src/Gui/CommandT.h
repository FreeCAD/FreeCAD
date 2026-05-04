/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Exception.h>
#include <Gui/Command.h>
#include <type_traits>
#include <typeinfo>
#include <boost/format.hpp>


namespace Gui
{

class FormatString
{
public:
    static std::string str(const std::string& s)
    {
        return s;
    }
    static std::string str(const char* s)
    {
        return s;
    }
    static std::string str(const QString& s)
    {
        return s.toStdString();
    }
    static std::string str(const std::stringstream& s)
    {
        return s.str();
    }
    static std::string str(const std::ostringstream& s)
    {
        return s.str();
    }
    static std::string str(const std::ostream& s)
    {
        if (typeid(s) == typeid(std::ostringstream)) {
            return dynamic_cast<const std::ostringstream&>(s).str();
        }
        else if (typeid(s) == typeid(std::stringstream)) {
            return dynamic_cast<const std::stringstream&>(s).str();
        }
        throw Base::TypeError("Not a std::stringstream or std::ostringstream");
    }
    static std::string toStr(boost::format& f)
    {
        return f.str();
    }

    template<typename T, typename... Args>
    static std::string toStr(boost::format& f, T&& t, Args&&... args)
    {
        return toStr(f % std::forward<T>(t), std::forward<Args>(args)...);
    }
};

/** @defgroup CommandFuncs Helper functions for running commands through Python interpreter */
//@{

/** Runs a command for accessing document attribute or method
 * This function is an alternative to _FCMD_DOC_CMD
 * @param doc: pointer to a document
 * @param mod: module name, "Gui" or "App"
 * @param cmd: command string, streamable
 *
 * Example:
 * @code{.cpp}
 *      _cmdDocument(Gui::Command::Gui, doc, "Gui", std::stringstream() << "getObject('" << objName
 * << "')");
 * @endcode
 *
 * Translates to command (assuming doc's name is 'DocName', and
 * and objName contains value 'ObjName'):
 * @code{.py}
 *       Gui.getDocument('DocName').getObject('ObjName')
 * @endcode
 */
template<typename T>
void _cmdDocument(
    Gui::Command::DoCmd_Type cmdType,
    const App::Document* doc,
    const std::string& mod,
    T&& cmd
)
{
    if (doc && doc->getName()) {
        std::stringstream str;
        str << mod << ".getDocument('" << doc->getName() << "')." << FormatString::str(cmd);
        Gui::Command::runCommand(cmdType, str.str().c_str());
    }
}

/** Runs a command for accessing document attribute or method
 * This function is an alternative to _FCMD_DOC_CMD
 * @param doc: document name
 * @param mod: module name, "Gui" or "App"
 * @param cmd: command string, streamable
 *
 * Example:
 * @code{.cpp}
 *      _cmdDocument(Gui::Command::Gui, doc, "Gui", std::stringstream() << "getObject('" << objName
 * << "')");
 * @endcode
 *
 * Translates to command (assuming doc's name is 'DocName', and
 * and objName contains value 'ObjName'):
 * @code{.py}
 *       Gui.getDocument('DocName').getObject('ObjName')
 * @endcode
 */
template<typename T>
void _cmdDocument(Gui::Command::DoCmd_Type cmdType, const std::string& doc, const std::string& mod, T&& cmd)
{
    if (!doc.empty()) {
        std::stringstream str;
        str << mod << ".getDocument('" << doc << "')." << FormatString::str(cmd);
        Gui::Command::runCommand(cmdType, str.str().c_str());
    }
}

/** Runs a command for accessing App.Document attribute or method
 * This function is an alternative to FCMD_DOC_CMD
 *
 * @param doc: pointer to a document
 * @param cmd: command string, streamable
 * @sa _cmdDocument()
 *
 * Example:
 * @code{.cpp}
 *      cmdAppDocument(doc, std::stringstream() << "getObject('" << objName << "')");
 * @endcode
 *
 * Translates to command (assuming doc's name is 'DocName', and
 * and objName contains value 'ObjName'):
 * @code{.py}
 *       App.getDocument('DocName').getObject('ObjName')
 * @endcode
 */
template<typename T>
inline void cmdAppDocument(const App::Document* doc, T&& cmd)
{
    _cmdDocument(Gui::Command::Doc, doc, "App", std::forward<T>(cmd));
}

/** Runs a command for accessing App.Document attribute or method
 * This function is an alternative to FCMD_DOC_CMD
 *
 * @param doc: document name
 * @param cmd: command string, streamable
 * @sa _cmdDocument()
 *
 * Example:
 * @code{.cpp}
 *      cmdAppDocument(doc, std::stringstream() << "getObject('" << objName << "')");
 * @endcode
 *
 * Translates to command (assuming doc's name is 'DocName', and
 * and objName contains value 'ObjName'):
 * @code{.py}
 *       App.getDocument('DocName').getObject('ObjName')
 * @endcode
 */
template<typename T>
inline void cmdAppDocument(const std::string& doc, T&& cmd)
{
    _cmdDocument(Gui::Command::Doc, doc, "App", std::forward<T>(cmd));
}

/** Runs a command for accessing App.Document attribute or method
 *
 * @param doc: pointer to a document
 * @param cmd: command string, streamable
 * @sa _cmdDocument()
 *
 * Example:
 * @code{.cpp}
 *      cmdGuiDocument(doc, std::stringstream() << "getObject('" << objName << "')");
 * @endcode
 *
 * Translates to command (assuming doc's name is 'DocName', and
 * and objName contains value 'ObjName'):
 * @code{.py}
 *       Gui.getDocument('DocName').getObject('ObjName')
 * @endcode
 */
template<typename T>
inline void cmdGuiDocument(const App::Document* doc, T&& cmd)
{
    _cmdDocument(Gui::Command::Gui, doc, "Gui", std::forward<T>(cmd));
}

/** Runs a command for accessing App.Document attribute or method
 *
 * @param doc: document name
 * @param cmd: command string, streamable
 * @sa _cmdDocument()
 *
 * Example:
 * @code{.cpp}
 *      cmdGuiDocument(doc, std::stringstream() << "getObject('" << objName << "')");
 * @endcode
 *
 * Translates to command (assuming doc's name is 'DocName', and
 * and objName contains value 'ObjName'):
 * @code{.py}
 *       Gui.getDocument('DocName').getObject('ObjName')
 * @endcode
 */
template<typename T>
inline void cmdGuiDocument(const std::string& doc, T&& cmd)
{
    _cmdDocument(Gui::Command::Gui, doc, "Gui", std::forward<T>(cmd));
}

/** Runs a command for accessing an object's document attribute or method
 * This function is an alternative to _FCMD_OBJ_DOC_CMD
 * @param obj: pointer to a DocumentObject
 * @param mod: module name, "Gui" or "App"
 * @param cmd: command string, streamable
 */
template<typename T>
inline void _cmdDocument(
    Gui::Command::DoCmd_Type cmdType,
    const App::DocumentObject* obj,
    const std::string& mod,
    T&& cmd
)
{
    if (obj) {
        _cmdDocument(cmdType, obj->getDocument(), mod, std::forward<T>(cmd));
    }
}

/** Runs a command for accessing an object's App::Document attribute or method
 * This function is an alternative to FCMD_OBJ_DOC_CMD
 * @param obj: pointer to a DocumentObject
 * @param cmd: command string, streamable
 */
template<typename T>
inline void cmdAppDocument(const App::DocumentObject* obj, T&& cmd)
{
    _cmdDocument(Gui::Command::Doc, obj, "App", std::forward<T>(cmd));
}

/** Runs a command for accessing a document's attribute or method
 * @param doc: pointer to a Document
 * @param cmd: command string, supporting printf like formatter
 *
 * Example:
 * @code{.cpp}
 *      cmdAppDocumentArgs(obj, "addObject('%s')", "Part::Feature");
 * @endcode
 *
 * Translates to command (assuming obj's document name is 'DocName':
 * @code{.py}
 *       App.getDocument('DocName').addObject('Part::Feature')
 * @endcode
 */
template<typename... Args>
void cmdAppDocumentArgs(const App::Document* doc, const std::string& cmd, Args&&... args)
{
    std::string _cmd;
    try {
        boost::format fmt(cmd);
        _cmd = FormatString::toStr(fmt, std::forward<Args>(args)...);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.getDocument('%s').%s",
            doc->getName(),
            _cmd.c_str()
        );
    }
    catch (const std::exception& e) {
        Base::Console().developerError(doc->Label.getStrValue(), "%s: %s\n", e.what(), cmd.c_str());
    }
    catch (const Base::Exception&) {
        Base::Console().developerError(
            doc->Label.getStrValue(),
            "App.getDocument('%s').%s\n",
            doc->getName(),
            _cmd.c_str()
        );
        throw;
    }
}

/** Runs a command for accessing an object's Gui::Document attribute or method
 * This function is an alternative to FCMD_VOBJ_DOC_CMD
 * @param obj: pointer to a DocumentObject
 * @param cmd: command string, streamable
 */
template<typename T>
inline void cmdGuiDocument(const App::DocumentObject* obj, T&& cmd)
{
    _cmdDocument(Gui::Command::Gui, obj, "Gui", std::forward<T>(cmd));
}

/** Runs a command for accessing a document/view object's attribute or method
 * This function is an alternative to _FCMD_OBJ_CMD
 * @param cmdType: Command type
 * @param obj: pointer to a DocumentObject
 * @param mod: module name, "Gui" or "App"
 * @param cmd: command string, streamable
 *
 * Example:
 * @code{.cpp}
 *      _cmdObject(Command::Gui,obj,"Gui", "Visibility = " << (visible?"True":"False"));
 * @endcode
 *
 * Translates to command (assuming obj's document name is 'DocName', obj's name
 * is 'ObjName', and visible is true):
 * @code{.py}
 *       Gui.getDocument('DocName').getObject('ObjName').Visibility = True
 * @endcode
 */
template<typename T>
void _cmdObject(
    Gui::Command::DoCmd_Type cmdType,
    const App::DocumentObject* obj,
    const std::string& mod,
    T&& cmd
)
{
    if (obj && obj->isAttachedToDocument()) {
        std::ostringstream str;
        str << mod << ".getDocument('" << obj->getDocument()->getName()
            << "')"
               ".getObject('"
            << obj->getNameInDocument() << "')." << FormatString::str(cmd);
        Gui::Command::runCommand(cmdType, str.str().c_str());
    }
}

/** Runs a command for accessing an document object's attribute or method
 * This function is an alternative to FCMD_OBJ_CMD
 * @param obj: pointer to a DocumentObject
 * @param cmd: command string, streamable
 * @sa _cmdObject()
 */
template<typename T>
inline void cmdAppObject(const App::DocumentObject* obj, T&& cmd)
{
    _cmdObject(Gui::Command::Doc, obj, "App", std::forward<T>(cmd));
}

/** Runs a command for accessing an view object's attribute or method
 * This function is an alternative to FCMD_VOBJ_CMD
 * @param obj: pointer to a DocumentObject
 * @param cmd: command string, streamable
 * @sa _cmdObject()
 */
template<typename T>
inline void cmdGuiObject(const App::DocumentObject* obj, T&& cmd)
{
    _cmdObject(Gui::Command::Gui, obj, "Gui", std::forward<T>(cmd));
}

/// Hides an object
inline void cmdAppObjectHide(const App::DocumentObject* obj)
{
    cmdAppObject(obj, "Visibility = False");
}

/// Shows an object
inline void cmdAppObjectShow(const App::DocumentObject* obj)
{
    cmdAppObject(obj, "Visibility = True");
}

/** Runs a command to start editing a give object
 * This function is an alternative to FCMD_SET_EDIT
 * @param obj: pointer to a DocumentObject
 *
 * Unlike other helper functions, this one editing the object using the current
 * active document, instead of the object's owner document. This allows
 * in-place editing an object, which may be brought in through linking to an
 * external group.
 */
inline void cmdSetEdit(const App::DocumentObject* obj, int mod = 0)
{
    if (obj && obj->isAttachedToDocument()) {
        Gui::Command::doCommand(
            Gui::Command::Gui,
            "Gui.ActiveDocument.setEdit(App.getDocument('%s').getObject('%s'), %d)",
            obj->getDocument()->getName(),
            obj->getNameInDocument(),
            mod
        );
    }
}

/** Runs a command for accessing a document object's attribute or method
 * This function is an alternative to FCMD_OBJ_CMD2
 * @param cmd: command string, supporting printf like formatter
 * @param obj: pointer to a DocumentObject
 *
 * Example:
 * @code{.cpp}
 *      cmdAppObjectArgs(obj, "Visibility = %s", visible ? "True" : "False");
 * @endcode
 *
 * Translates to command (assuming obj's document name is 'DocName', obj's name
 * is 'ObjName', and visible is true):
 * @code{.py}
 *       App.getDocument('DocName').getObject('ObjName').Visibility = True
 * @endcode
 */
template<typename... Args>
void cmdAppObjectArgs(const App::DocumentObject* obj, const std::string& cmd, Args&&... args)
{
    std::string _cmd;
    try {
        boost::format fmt(cmd);
        _cmd = FormatString::toStr(fmt, std::forward<Args>(args)...);
        Gui::Command::doCommand(
            Gui::Command::Doc,
            "App.getDocument('%s').getObject('%s').%s",
            obj->getDocument()->getName(),
            obj->getNameInDocument(),
            _cmd.c_str()
        );
    }
    catch (const std::exception& e) {
        Base::Console().developerError(obj->getFullLabel(), "%s: %s\n", e.what(), cmd.c_str());
    }
    catch (const Base::Exception&) {
        Base::Console().developerError(
            obj->getFullLabel(),
            "App.getDocument('%s').getObject('%s').%s\n",
            obj->getDocument()->getName(),
            obj->getNameInDocument(),
            _cmd.c_str()
        );
        throw;
    }
}

/** Runs a command for accessing a view object's attribute or method
 * This function is an alternative to FCMD_VOBJ_CMD2
 * @param cmd: command string, supporting printf like formatter
 * @param obj: pointer to a DocumentObject
 * @sa cmdAppObjectArgs()
 */
template<typename... Args>
void cmdGuiObjectArgs(const App::DocumentObject* obj, const std::string& cmd, Args&&... args)
{
    std::string _cmd;
    try {
        boost::format fmt(cmd);
        _cmd = FormatString::toStr(fmt, std::forward<Args>(args)...);
        Gui::Command::doCommand(
            Gui::Command::Gui,
            "Gui.getDocument('%s').getObject('%s').%s",
            obj->getDocument()->getName(),
            obj->getNameInDocument(),
            _cmd.c_str()
        );
    }
    catch (const std::exception& e) {
        Base::Console().developerError(obj->getFullLabel(), "%s: %s\n", e.what(), cmd.c_str());
    }
    catch (const Base::Exception&) {
        Base::Console().developerError(
            obj->getFullLabel(),
            "Gui.getDocument('%s').getObject('%s').%s\n",
            obj->getDocument()->getName(),
            obj->getNameInDocument(),
            _cmd.c_str()
        );
        throw;
    }
}

/** Runs a command
 * @param cmdType: command type
 * @param cmd: command string, supporting printf like formatter
 *
 * Example:
 * @code{.cpp}
 *      doCommandT(Gui::Command::Gui, "Gui.getDocument(%s).getObject(%s).Visibility = %s",
 * "DocName", "ObjName", visible?"True":"False");
 * @endcode
 *
 * Translates to command (assuming obj's document name is 'DocName', obj's name
 * is 'ObjName', and visible is true):
 * @code{.py}
 *       Gui.getDocument('DocName').getObject('ObjName').Visibility = True
 * @endcode
 */
template<typename... Args>
void doCommandT(Gui::Command::DoCmd_Type cmdType, const std::string& cmd, Args&&... args)
{
    std::string _cmd;
    try {
        boost::format fmt(cmd);
        _cmd = FormatString::toStr(fmt, std::forward<Args>(args)...);
        Gui::Command::doCommand(cmdType, "%s", _cmd.c_str());
    }
    catch (const std::exception& e) {
        Base::Console().developerError("doCommandT", "%s: %s\n", e.what(), cmd.c_str());
    }
    catch (const Base::Exception&) {
        Base::Console().developerError("doCommandT", "%s\n", _cmd.c_str());
        throw;
    }
}

/** Copy visual attributes from a source to a target object
 */
template<typename... Args>
void copyVisualT(Args&&... args)
{
    // file and line number is useless here. Check C++'s source location function once available
    Gui::Command::_copyVisual(__FILE__, __LINE__, std::forward<Args>(args)...);
}

//@}

};  // namespace Gui
