/***************************************************************************
 *   Copyright (c) 2018 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
 *   write to the Free Software Foundation, Inc., 51 Franklin Street,      *
 *   Fifth Floor, Boston, MA  02110-1301, USA                              *
 *                                                                         *
 ***************************************************************************/

#include "PreCompiled.h"

#include "Translate.h"
#include <QCoreApplication>
#include <QFileInfo>

using namespace Base;


Translate::Translate()
   : Py::ExtensionModule<Translate>("__Translate__")
{
    add_varargs_method("translate",
        &Translate::translate,
        "translate(context, sourcetext, disambiguation = None, n=-1)\n"
        "-- Returns the translation text for sourceText, by querying\n"
        "the installed translation files. The translation files are\n"
        "searched from the most recently installed file back to the\n"
        "first installed file.");
    add_varargs_method("QT_TRANSLATE_NOOP",
        &Translate::translateNoop,
        "QT_TRANSLATE_NOOP(context, sourcetext)\n"
        "Marks the UTF-8 encoded string literal sourcetext for delayed translation in the given context.\n"
        "The context is typically a class name and also needs to be specified as a string literal.");
    add_varargs_method("QT_TRANSLATE_NOOP3",
        &Translate::translateNoop3,
        "QT_TRANSLATE_NOOP3(context, sourcetext, disambiguation)\n"
        "Marks the UTF-8 encoded string literal sourceText for delayed translation in the given context\n"
        "with the given disambiguation. The context is typically a class and also needs to be specified\n"
        "as a string literal. The string literal disambiguation should be a short semantic tag to tell\n"
        "apart otherwise identical strings.");
    add_varargs_method("QT_TRANSLATE_NOOP_UTF8",
        &Translate::translateNoop,
        "QT_TRANSLATE_NOOP_UTF8(context, sourcetext)\n"
        "Same as QT_TRANSLATE_NOOP");
    add_varargs_method("QT_TR_NOOP",
        &Translate::trNoop,
        "QT_TR_NOOP(sourcetext)\n"
        "Marks the UTF-8 encoded string literal sourcetext for delayed translation in the current context");
    add_varargs_method("QT_TR_NOOP_UTF8",
        &Translate::trNoop,
        "QT_TR_NOOP_UTF8(sourcetext)\n"
        "Same as QT_TR_NOOP");
    add_varargs_method("installTranslator",
        &Translate::installTranslator,
        "Install a translator for testing purposes");
    add_varargs_method("removeTranslators",
        &Translate::removeTranslators,
        "Remove test translators");
    initialize("This module is the Translate module"); // register with Python
}

Translate::~Translate() = default;

Py::Object Translate::translate(const Py::Tuple& args)
{
    char *context = nullptr;
    char *source = nullptr;
    char *disambiguation = nullptr;
    int n=-1;
    if (!PyArg_ParseTuple(args.ptr(), "ss|zi", &context, &source, &disambiguation, &n))
        throw Py::Exception();

    QString str = QCoreApplication::translate(context, source, disambiguation, n);
    return Py::asObject(PyUnicode_FromString(str.toUtf8()));
}

Py::Object Translate::translateNoop(const Py::Tuple& args)
{
    PyObject* arg1 = nullptr;
    PyObject* arg2 = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "OO", &arg1, &arg2))
        throw Py::Exception();
    return Py::Object(arg2);
}

Py::Object Translate::translateNoop3(const Py::Tuple& args)
{
    PyObject* arg1 = nullptr;
    PyObject* arg2 = nullptr;
    PyObject* arg3 = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "OOO", &arg1, &arg2, &arg3))
        throw Py::Exception();
    return Py::Object(arg2);
}

Py::Object Translate::trNoop(const Py::Tuple& args)
{
    PyObject* arg1 = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "O", &arg1))
        throw Py::Exception();
    return Py::Object(arg1);
}

Py::Object Translate::installTranslator(const Py::Tuple& args)
{
    char* Name = nullptr;
    if (!PyArg_ParseTuple(args.ptr(), "et","utf-8",&Name))
        throw Py::Exception();
    QString filename = QString::fromUtf8(Name);
    PyMem_Free(Name);

    bool ok = false;
    QFileInfo fi(filename);
    std::shared_ptr<QTranslator> translator(std::make_shared<QTranslator>(nullptr));
    translator->setObjectName(fi.fileName());
    if (translator->load(filename)) {
        qApp->installTranslator(translator.get());
        translators.push_back(translator);
        ok = true;
    }

    return Py::Boolean(ok); // NOLINT
}

Py::Object Translate::removeTranslators(const Py::Tuple& args)
{
    if (!PyArg_ParseTuple(args.ptr(), ""))
        throw Py::Exception();
    bool ok = true;
    for (const auto& it : translators) {
        ok &= QCoreApplication::removeTranslator(it.get());
    }

    translators.clear();
    return Py::Boolean(ok); // NOLINT
}
