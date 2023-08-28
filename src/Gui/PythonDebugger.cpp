/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <QCoreApplication>
# include <QEventLoop>
#endif

#include <Base/Console.h>
#include <Base/Interpreter.h>

#include "PythonDebugger.h"
#include "BitmapFactory.h"
#include "EditorView.h"
#include "MainWindow.h"
#include "PythonEditor.h"


using namespace Gui;

Breakpoint::Breakpoint() = default;

Breakpoint::Breakpoint(const Breakpoint& rBp)
{
    setFilename(rBp.filename());
    for (int it : rBp._linenums)
        _linenums.insert(it);
}

Breakpoint& Breakpoint::operator= (const Breakpoint& rBp)
{
    if (this == &rBp)
        return *this;
    setFilename(rBp.filename());
    _linenums.clear();
    for (int it : rBp._linenums)
        _linenums.insert(it);
    return *this;
}

Breakpoint::~Breakpoint() = default;

void Breakpoint::setFilename(const QString& fn)
{
    _filename = fn;
}

void Breakpoint::addLine(int line)
{
    _linenums.insert(line);
}

void Breakpoint::removeLine(int line)
{
    _linenums.erase(line);
}

bool Breakpoint::checkLine(int line)
{
    return (_linenums.find(line) != _linenums.end());
}

int Breakpoint::lineIndex(int ind)const
{
    int i = 0;
    for (int it : _linenums)
    {
        if (ind == i++)
            return it;
    }
    return -1;
}

// -----------------------------------------------------

void PythonDebugModule::init_module()
{
    PythonDebugStdout::init_type();
    PythonDebugStderr::init_type();
    PythonDebugExcept::init_type();
    Base::Interpreter().addModule(new PythonDebugModule);
}

PythonDebugModule::PythonDebugModule()
  : Py::ExtensionModule<PythonDebugModule>("FreeCADDbg")
{
    add_varargs_method("getFunctionCallCount", &PythonDebugModule::getFunctionCallCount,
        "Get the total number of function calls executed and the number executed since the last call to this function.");
    add_varargs_method("getExceptionCount", &PythonDebugModule::getExceptionCount,
        "Get the total number of exceptions and the number executed since the last call to this function.");
    add_varargs_method("getLineCount", &PythonDebugModule::getLineCount,
        "Get the total number of lines executed and the number executed since the last call to this function.");
    add_varargs_method("getFunctionReturnCount", &PythonDebugModule::getFunctionReturnCount,
        "Get the total number of function returns executed and the number executed since the last call to this function.");

    initialize( "The FreeCAD Python debug module" );

    Py::Dict d(moduleDictionary());
    Py::Object out(Py::asObject(new PythonDebugStdout()));
    d["StdOut"] = out;
    Py::Object err(Py::asObject(new PythonDebugStderr()));
    d["StdErr"] = err;
}

PythonDebugModule::~PythonDebugModule()
{
    Py::Dict d(moduleDictionary());
    d["StdOut"] = Py::None();
    d["StdErr"] = Py::None();
}

Py::Object PythonDebugModule::getFunctionCallCount(const Py::Tuple &)
{
    return Py::None();
}

Py::Object PythonDebugModule::getExceptionCount(const Py::Tuple &)
{
    return Py::None();
}

Py::Object PythonDebugModule::getLineCount(const Py::Tuple &)
{
    return Py::None();
}

Py::Object PythonDebugModule::getFunctionReturnCount(const Py::Tuple &)
{
    return Py::None();
}

// -----------------------------------------------------

void PythonDebugStdout::init_type()
{
    behaviors().name("PythonDebugStdout");
    behaviors().doc("Redirection of stdout to FreeCAD's Python debugger window");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&PythonDebugStdout::write,"write to stdout");
    add_varargs_method("flush",&PythonDebugStdout::flush,"flush the output");
}

PythonDebugStdout::PythonDebugStdout() = default;

PythonDebugStdout::~PythonDebugStdout() = default;

Py::Object PythonDebugStdout::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonDebugStdout";
    return Py::String(s_out.str());
}

Py::Object PythonDebugStdout::write(const Py::Tuple& args)
{
    char *msg;
    //PyObject* pObj;
    ////args contains a single parameter which is the string to write.
    //if (!PyArg_ParseTuple(args.ptr(), "Os:OutputString", &pObj, &msg))
    if (!PyArg_ParseTuple(args.ptr(), "s:OutputString", &msg))
        throw Py::Exception();

    if (strlen(msg) > 0)
    {
        //send it to our stdout
        printf("%s\n",msg);

        //send it to the debugger as well
        //g_DebugSocket.SendMessage(eMSG_OUTPUT, msg);
    }
    return Py::None();
}

Py::Object PythonDebugStdout::flush(const Py::Tuple&)
{
    return Py::None();
}

// -----------------------------------------------------

void PythonDebugStderr::init_type()
{
    behaviors().name("PythonDebugStderr");
    behaviors().doc("Redirection of stderr to FreeCAD's Python debugger window");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("write",&PythonDebugStderr::write,"write to stderr");
}

PythonDebugStderr::PythonDebugStderr() = default;

PythonDebugStderr::~PythonDebugStderr() = default;

Py::Object PythonDebugStderr::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonDebugStderr";
    return Py::String(s_out.str());
}

Py::Object PythonDebugStderr::write(const Py::Tuple& args)
{
    char *msg;
    //PyObject* pObj;
    //args contains a single parameter which is the string to write.
    //if (!PyArg_ParseTuple(args.ptr(), "Os:OutputDebugString", &pObj, &msg))
    if (!PyArg_ParseTuple(args.ptr(), "s:OutputDebugString", &msg))
        throw Py::Exception();

    if (strlen(msg) > 0)
    {
        //send the message to our own stderr
        //dprintf(msg);

        //send it to the debugger as well
        //g_DebugSocket.SendMessage(eMSG_TRACE, msg);
        Base::Console().Error("%s", msg);
    }

    return Py::None();
}

// -----------------------------------------------------

void PythonDebugExcept::init_type()
{
    behaviors().name("PythonDebugExcept");
    behaviors().doc("Custom exception handler");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    add_varargs_method("fc_excepthook",&PythonDebugExcept::excepthook,"Custom exception handler");
}

PythonDebugExcept::PythonDebugExcept() = default;

PythonDebugExcept::~PythonDebugExcept() = default;

Py::Object PythonDebugExcept::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "PythonDebugExcept";
    return Py::String(s_out.str());
}

Py::Object PythonDebugExcept::excepthook(const Py::Tuple& args)
{
    PyObject *exc, *value, *tb;
    if (!PyArg_UnpackTuple(args.ptr(), "excepthook", 3, 3, &exc, &value, &tb))
        throw Py::Exception();

    PyErr_NormalizeException(&exc, &value, &tb);

    PyErr_Display(exc, value, tb);
/*
    if (eEXCEPTMODE_IGNORE != g_eExceptionMode)
    {
        assert(tb);

        if (tb && (tb != Py_None))
        {
            //get the pointer to the frame held by the bottom traceback object - this
            //should be where the exception occurred.
            tracebackobject* pTb = (tracebackobject*)tb;
            while (pTb->tb_next != NULL)
            {
                pTb = pTb->tb_next;
            }
            PyFrameObject* frame = (PyFrameObject*)PyObject_GetAttr((PyObject*)pTb, PyString_FromString("tb_frame"));
            EnterBreakState(frame, (PyObject*)pTb);
        }
    }*/

    return Py::None();
}

// -----------------------------------------------------

namespace Gui {
class PythonDebuggerPy : public Py::PythonExtension<PythonDebuggerPy>
{
public:
    explicit PythonDebuggerPy(PythonDebugger* d) : dbg(d), depth(0) { }
    ~PythonDebuggerPy() override = default;
    PythonDebugger* dbg;
    int depth;
};

class RunningState
{
public:
    explicit RunningState(bool& s) : state(s)
    { state = true; }
    ~RunningState()
    { state = false; }
private:
    bool& state;
};

struct PythonDebuggerP {
    PyObject* out_o{nullptr};
    PyObject* err_o{nullptr};
    PyObject* exc_o{nullptr};
    PyObject* out_n{nullptr};
    PyObject* err_n{nullptr};
    PyObject* exc_n{nullptr};
    PythonDebugExcept* pypde;
    bool init{false}, trystop{false}, running{false};
    QEventLoop loop;
    PyObject* pydbg{nullptr};
    std::vector<Breakpoint> bps;

    explicit PythonDebuggerP(PythonDebugger* that)
    {
        Base::PyGILStateLocker lock;
        out_n = new PythonDebugStdout();
        err_n = new PythonDebugStderr();
        pypde = new PythonDebugExcept();
        Py::Object func = pypde->getattr("fc_excepthook");
        exc_n = Py::new_reference_to(func);
        pydbg = new PythonDebuggerPy(that);
    }
    ~PythonDebuggerP()
    {
        Base::PyGILStateLocker lock;
        Py_DECREF(out_n);
        Py_DECREF(err_n);
        Py_DECREF(exc_n);
        Py_DECREF(pypde);
        Py_DECREF(pydbg);
    }
};
}

PythonDebugger::PythonDebugger()
  : d(new PythonDebuggerP(this))
{
}

PythonDebugger::~PythonDebugger()
{
    delete d;
}

Breakpoint PythonDebugger::getBreakpoint(const QString& fn) const
{
    for (const Breakpoint& it : d->bps) {
        if (fn == it.filename()) {
            return it;
        }
    }

    return {};
}

bool PythonDebugger::toggleBreakpoint(int line, const QString& fn)
{
    for (Breakpoint& it : d->bps) {
        if (fn == it.filename()) {
            if (it.checkLine(line)) {
                it.removeLine(line);
                return false;
            }
            else {
                it.addLine(line);
                return true;
            }
        }
    }

    Breakpoint bp;
    bp.setFilename(fn);
    bp.addLine(line);
    d->bps.push_back(bp);
    return true;
}

void PythonDebugger::runFile(const QString& fn)
{
    try {
        RunningState state(d->running);
        QByteArray pxFileName = fn.toUtf8();
#ifdef FC_OS_WIN32
        Base::FileInfo fi((const char*)pxFileName);
        FILE *fp = _wfopen(fi.toStdWString().c_str(),L"r");
#else
        FILE *fp = fopen((const char*)pxFileName,"r");
#endif
        if (!fp)
            return;

        Base::PyGILStateLocker locker;
        PyObject *module, *dict;
        module = PyImport_AddModule("__main__");
        dict = PyModule_GetDict(module);
        dict = PyDict_Copy(dict);
        if (!PyDict_GetItemString(dict, "__file__")) {
            PyObject *pyObj = PyUnicode_FromString((const char*)pxFileName);
            if (!pyObj) {
                fclose(fp);
                return;
            }
            if (PyDict_SetItemString(dict, "__file__", pyObj) < 0) {
                Py_DECREF(pyObj);
                fclose(fp);
                return;
            }
            Py_DECREF(pyObj);
        }

        PyObject *result = PyRun_File(fp, (const char*)pxFileName, Py_file_input, dict, dict);
        fclose(fp);
        Py_DECREF(dict);

        if (!result)
            PyErr_Print();
        else
            Py_DECREF(result);
    }
    catch (const Base::PyException&/* e*/) {
        //PySys_WriteStderr("Exception: %s\n", e.what());
    }
    catch (...) {
        Base::Console().Warning("Unknown exception thrown during macro debugging\n");
    }
}

bool PythonDebugger::isRunning() const
{
    return d->running;
}

bool PythonDebugger::start()
{
    if (d->init)
        return false;
    d->init = true;
    d->trystop = false;
    Base::PyGILStateLocker lock;
    d->out_o = PySys_GetObject("stdout");
    d->err_o = PySys_GetObject("stderr");
    d->exc_o = PySys_GetObject("excepthook");

    PySys_SetObject("stdout", d->out_n);
    PySys_SetObject("stderr", d->err_n);
    PySys_SetObject("excepthook", d->exc_o);

    PyEval_SetTrace(tracer_callback, d->pydbg);
    return true;
}

bool PythonDebugger::stop()
{
    if (!d->init)
        return false;
    Base::PyGILStateLocker lock;
    PyEval_SetTrace(nullptr, nullptr);
    PySys_SetObject("stdout", d->out_o);
    PySys_SetObject("stderr", d->err_o);
    PySys_SetObject("excepthook", d->exc_o);
    d->init = false;
    return true;
}

void PythonDebugger::tryStop()
{
    d->trystop = true;
    Q_EMIT signalNextStep();
}

void PythonDebugger::stepOver()
{
    Q_EMIT signalNextStep();
}

void PythonDebugger::stepInto()
{
    Q_EMIT signalNextStep();
}

void PythonDebugger::stepRun()
{
    Q_EMIT signalNextStep();
}

void PythonDebugger::showDebugMarker(const QString& fn, int line)
{
    PythonEditorView* edit = nullptr;
    QList<QWidget*> mdis = getMainWindow()->windows();
    for (const auto & mdi : mdis) {
        edit = qobject_cast<PythonEditorView*>(mdi);
        if (edit && edit->fileName() == fn)
            break;
    }

    if (!edit) {
        auto editor = new PythonEditor();
        editor->setWindowIcon(Gui::BitmapFactory().iconFromTheme("applications-python"));
        edit = new PythonEditorView(editor, getMainWindow());
        edit->open(fn);
        edit->resize(400, 300);
        getMainWindow()->addWindow(edit);
    }

    getMainWindow()->setActiveWindow(edit);
    edit->showDebugMarker(line);
}

void PythonDebugger::hideDebugMarker(const QString& fn)
{
    PythonEditorView* edit = nullptr;
    QList<QWidget*> mdis = getMainWindow()->windows();
    for (const auto & mdi : mdis) {
        edit = qobject_cast<PythonEditorView*>(mdi);
        if (edit && edit->fileName() == fn) {
            edit->hideDebugMarker();
            break;
        }
    }
}

#if PY_VERSION_HEX < 0x030900B1
static PyCodeObject* PyFrame_GetCode(PyFrameObject *frame)
{
    Py_INCREF(frame->f_code);
    return frame->f_code;
}
#endif

// http://www.koders.com/cpp/fidBA6CD8A0FE5F41F1464D74733D9A711DA257D20B.aspx?s=PyEval_SetTrace
// http://code.google.com/p/idapython/source/browse/trunk/python.cpp
// http://www.koders.com/cpp/fid191F7B13CF73133935A7A2E18B7BF43ACC6D1784.aspx?s=PyEval_SetTrace

int PythonDebugger::tracer_callback(PyObject *obj, PyFrameObject *frame, int what, PyObject * /*arg*/)
{
    auto self = static_cast<PythonDebuggerPy*>(obj);
    PythonDebugger* dbg = self->dbg;
    if (dbg->d->trystop)
        PyErr_SetInterrupt();
    QCoreApplication::processEvents();
    PyCodeObject* code = PyFrame_GetCode(frame);
    QString file = QString::fromUtf8(PyUnicode_AsUTF8(code->co_filename));
    Py_DECREF(code);
    switch (what) {
    case PyTrace_CALL:
        self->depth++;
        return 0;
    case PyTrace_RETURN:
        if (self->depth > 0)
            self->depth--;
        return 0;
    case PyTrace_LINE:
        {
            PyCodeObject* f_code = PyFrame_GetCode(frame);
            int f_lasti = PyFrame_GetLineNumber(frame);
            int line = PyCode_Addr2Line(f_code, f_lasti);
            Py_DECREF(f_code);

            if (!dbg->d->trystop) {
                Breakpoint bp = dbg->getBreakpoint(file);
                if (bp.checkLine(line)) {
                    dbg->showDebugMarker(file, line);
                    QEventLoop loop;
                    QObject::connect(dbg, &PythonDebugger::signalNextStep, &loop, &QEventLoop::quit);
                    loop.exec();
                    dbg->hideDebugMarker(file);
                }
            }
            return 0;
        }
    case PyTrace_EXCEPTION:
        return 0;
    case PyTrace_C_CALL:
        return 0;
    case PyTrace_C_EXCEPTION:
        return 0;
    case PyTrace_C_RETURN:
        return 0;
    default:
        /* ignore PyTrace_EXCEPTION */
        break;
    }
    return 0;
}

#include "moc_PythonDebugger.cpp"
