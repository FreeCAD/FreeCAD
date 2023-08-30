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
# include <QVBoxLayout>
#endif

#ifdef FC_OS_WIN32
#undef max
#undef min
#ifdef _MSC_VER
#pragma warning( disable : 4099 )
#pragma warning( disable : 4522 )
#endif
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Interpreter.h>

#include "WidgetFactory.h"
#include "PrefWidgets.h"
#include "PythonWrapper.h"
#include "UiLoader.h"


using namespace Gui;

Gui::WidgetFactoryInst* Gui::WidgetFactoryInst::_pcSingleton = nullptr;

WidgetFactoryInst& WidgetFactoryInst::instance()
{
    if (!_pcSingleton)
        _pcSingleton = new WidgetFactoryInst;
    return *_pcSingleton;
}

void WidgetFactoryInst::destruct ()
{
    if (_pcSingleton)
        delete _pcSingleton;
    _pcSingleton = nullptr;
}

/**
 * Creates a widget with the name \a sName which is a child of \a parent.
 * To create an instance of this widget once it must has been registered.
 * If there is no appropriate widget registered nullptr is returned.
 */
QWidget* WidgetFactoryInst::createWidget (const char* sName, QWidget* parent) const
{
    auto w = static_cast<QWidget*>(Produce(sName));

    // this widget class is not registered
    if (!w) {
#ifdef FC_DEBUG
        Base::Console().Warning("\"%s\" is not registered\n", sName);
#else
        Base::Console().Log("\"%s\" is not registered\n", sName);
#endif
        return nullptr;
    }

    try {
#ifdef FC_DEBUG
        const char* cName = dynamic_cast<QWidget*>(w)->metaObject()->className();
        Base::Console().Log("Widget of type '%s' created.\n", cName);
#endif
    }
    catch (...) {
#ifdef FC_DEBUG
        Base::Console().Error("%s does not inherit from \"QWidget\"\n", sName);
#else
        Base::Console().Log("%s does not inherit from \"QWidget\"\n", sName);
#endif
        delete w;
        return nullptr;
    }

    // set the parent to the widget
    if (parent)
        w->setParent(parent);

    return w;
}

/**
 * Creates a widget with the name \a sName which is a child of \a parent.
 * To create an instance of this widget once it must has been registered.
 * If there is no appropriate widget registered nullptr is returned.
 */
Gui::Dialog::PreferencePage* WidgetFactoryInst::createPreferencePage (const char* sName, QWidget* parent) const
{
    auto w = (Gui::Dialog::PreferencePage*)Produce(sName);

    // this widget class is not registered
    if (!w) {
#ifdef FC_DEBUG
        Base::Console().Warning("Cannot create an instance of \"%s\"\n", sName);
#else
        Base::Console().Log("Cannot create an instance of \"%s\"\n", sName);
#endif
        return nullptr;
    }

    if (qobject_cast<Gui::Dialog::PreferencePage*>(w)) {
#ifdef FC_DEBUG
        Base::Console().Log("Preference page of type '%s' created.\n", w->metaObject()->className());
#endif
    }
    else {
#ifdef FC_DEBUG
        Base::Console().Error("%s does not inherit from 'Gui::Dialog::PreferencePage'\n", sName);
#endif
        delete w;
        return nullptr;
    }

    // set the parent to the widget
    if (parent)
        w->setParent(parent);

    return w;
}

/**
 * Creates a preference widget with the name \a sName and the preference name \a sPref
 * which is a child of \a parent.
 * To create an instance of this widget once it must has been registered.
 * If there is no appropriate widget registered nullptr is returned.
 * After creation of this widget its recent preferences are restored automatically.
 */
QWidget* WidgetFactoryInst::createPrefWidget(const char* sName, QWidget* parent, const char* sPref)
{
    QWidget* w = createWidget(sName);
    // this widget class is not registered
    if (!w)
        return nullptr; // no valid QWidget object

    // set the parent to the widget
    w->setParent(parent);

    try {
        auto pw = dynamic_cast<PrefWidget*>(w);
        if (pw) {
            pw->setEntryName(sPref);
            pw->restorePreferences();
        }
    }
    catch (...) {
#ifdef FC_DEBUG
        Base::Console().Error("%s does not inherit from \"PrefWidget\"\n", w->metaObject()->className());
#endif
        delete w;
        return nullptr;
    }

    return w;
}

// ----------------------------------------------------

WidgetFactorySupplier* WidgetFactorySupplier::_pcSingleton = nullptr;

WidgetFactorySupplier & WidgetFactorySupplier::instance()
{
    // not initialized?
    if (!_pcSingleton)
        _pcSingleton = new WidgetFactorySupplier;
    return *_pcSingleton;
}

void WidgetFactorySupplier::destruct()
{
    // delete the widget factory and all its producers first
    WidgetFactoryInst::destruct();
    delete _pcSingleton;
    _pcSingleton=nullptr;
}

// ----------------------------------------------------

PrefPageUiProducer::PrefPageUiProducer (const char* filename, const char* group)
  : fn(QString::fromUtf8(filename))
{
    WidgetFactoryInst::instance().AddProducer(filename, this);
    Gui::Dialog::DlgPreferencesImp::addPage(filename, group);
}

PrefPageUiProducer::~PrefPageUiProducer() = default;

void* PrefPageUiProducer::Produce () const
{
    QWidget* page = new Gui::Dialog::PreferenceUiForm(fn);
    return static_cast<void*>(page);
}

// ----------------------------------------------------

PrefPagePyProducer::PrefPagePyProducer (const Py::Object& p, const char* group)
  : type(p)
{
    std::string str;
    Base::PyGILStateLocker lock;
    if (type.hasAttr("__name__")) {
        str = static_cast<std::string>(Py::String(type.getAttr("__name__")));
    }

    WidgetFactoryInst::instance().AddProducer(str.c_str(), this);
    Gui::Dialog::DlgPreferencesImp::addPage(str, group);
}

PrefPagePyProducer::~PrefPagePyProducer ()
{
    Base::PyGILStateLocker lock;
    type = Py::None();
}

void* PrefPagePyProducer::Produce () const
{
    Base::PyGILStateLocker lock;
    try {
        Py::Callable method(type);
        Py::Tuple args;
        Py::Object page = method.apply(args);
        QWidget* widget = new Gui::Dialog::PreferencePagePython(page);
        if (!widget->layout()) {
            delete widget;
            widget = nullptr;
        }
        return widget;
    }
    catch (Py::Exception&) {
        PyErr_Print();
        return nullptr;
    }
}

// ----------------------------------------------------

using namespace Gui::Dialog;

PreferencePagePython::PreferencePagePython(const Py::Object& p, QWidget* parent)
  : PreferencePage(parent), page(p)
{
    Base::PyGILStateLocker lock;
    Gui::PythonWrapper wrap;
    if (wrap.loadCoreModule()) {

        // old style class must have a form attribute while
        // new style classes can be the widget itself
        Py::Object widget;
        if (page.hasAttr(std::string("form")))
            widget = page.getAttr(std::string("form"));
        else
            widget = page;

        QObject* object = wrap.toQObject(widget);
        if (object) {
            QWidget* form = qobject_cast<QWidget*>(object);
            if (form) {
                this->setWindowTitle(form->windowTitle());
                auto layout = new QVBoxLayout;
                layout->addWidget(form);
                setLayout(layout);
            }
        }
    }
}

PreferencePagePython::~PreferencePagePython()
{
    Base::PyGILStateLocker lock;
    page = Py::None();
}

void PreferencePagePython::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
}

void PreferencePagePython::loadSettings()
{
    Base::PyGILStateLocker lock;
    try {
        if (page.hasAttr(std::string("loadSettings"))) {
            Py::Callable method(page.getAttr(std::string("loadSettings")));
            Py::Tuple args;
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void PreferencePagePython::saveSettings()
{
    Base::PyGILStateLocker lock;
    try {
        if (page.hasAttr(std::string("saveSettings"))) {
            Py::Callable method(page.getAttr(std::string("saveSettings")));
            Py::Tuple args;
            method.apply(args);
        }
    }
    catch (Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

// ----------------------------------------------------

/* TRANSLATOR Gui::ContainerDialog */

/**
 *  Constructs a ContainerDialog which embeds the child \a templChild.
 *  The dialog will be modal.
 */
ContainerDialog::ContainerDialog( QWidget* templChild )
  : QDialog( QApplication::activeWindow())
{
    setModal(true);
    setWindowTitle( templChild->objectName() );
    setObjectName( templChild->objectName() );

    setSizeGripEnabled( true );
    MyDialogLayout = new QGridLayout(this);

    buttonOk = new QPushButton(this);
    buttonOk->setObjectName(QLatin1String("buttonOK"));
    buttonOk->setText( tr( "&OK" ) );
    buttonOk->setAutoDefault( true );
    buttonOk->setDefault( true );

    MyDialogLayout->addWidget( buttonOk, 1, 0 );
    auto spacer = new QSpacerItem( 210, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    MyDialogLayout->addItem( spacer, 1, 1 );

    buttonCancel = new QPushButton(this);
    buttonCancel->setObjectName(QLatin1String("buttonCancel"));
    buttonCancel->setText( tr( "&Cancel" ) );
    buttonCancel->setAutoDefault( true );

    MyDialogLayout->addWidget( buttonCancel, 1, 2 );

    templChild->setParent(this);

    MyDialogLayout->addWidget( templChild, 0, 0, 0, 2 );
    resize( QSize(307, 197).expandedTo(minimumSizeHint()) );

    // signals and slots connections
    connect( buttonOk, &QPushButton::clicked, this, &QDialog::accept);
    connect( buttonCancel, &QPushButton::clicked, this, &QDialog::reject);
}

/** Destroys the object and frees any allocated resources */
ContainerDialog::~ContainerDialog() = default;

// ----------------------------------------------------

void PyResource::init_type()
{
    behaviors().name("PyResource");
    behaviors().doc("PyResource");
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    add_varargs_method("value",&PyResource::value);
    add_varargs_method("setValue",&PyResource::setValue);
    add_varargs_method("show",&PyResource::show);
    add_varargs_method("connect",&PyResource::connect);
}

PyResource::PyResource() : myDlg(nullptr)
{
}

PyResource::~PyResource()
{
    delete myDlg;
    for (auto it : mySignals) {
        SignalConnect* sc = it;
        delete sc;
    }
}

/**
 * Loads an .ui file with the name \a name. If the .ui file cannot be found or the QWidgetFactory
 * cannot create an instance an exception is thrown. If the created resource does not inherit from
 * QDialog an instance of ContainerDialog is created to embed it.
 */
void PyResource::load(const char* name)
{
    QString fn = QString::fromUtf8(name);
    QFileInfo fi(fn);

    // checks whether it's a relative path
    if (fi.isRelative()) {
        QString cwd = QDir::currentPath ();
        QString home= QDir(QString::fromStdString(App::Application::getHomePath())).path();

        // search in cwd and home path for the file
        //
        // file does not reside in cwd, check home path now
        if (!fi.exists()) {
            if (cwd == home) {
                QString what = QObject::tr("Cannot find file %1").arg(fi.absoluteFilePath());
                throw Base::FileSystemError(what.toUtf8().constData());
            }
            else {
                fi.setFile( QDir(home), fn );

                if (!fi.exists()) {
                    QString what = QObject::tr("Cannot find file %1 neither in %2 nor in %3")
                        .arg(fn, cwd, home);
                    throw Base::FileSystemError(what.toUtf8().constData());
                }
                else {
                    fn = fi.absoluteFilePath(); // file resides in FreeCAD's home directory
                }
            }
        }
    }
    else {
        if (!fi.exists()) {
            QString what = QObject::tr("Cannot find file %1").arg(fn);
            throw Base::FileSystemError(what.toUtf8().constData());
        }
    }

    QWidget* w=nullptr;
    try {
        auto loader = UiLoader::newInstance();
        QFile file(fn);
        if (file.open(QFile::ReadOnly))
            w = loader->load(&file, QApplication::activeWindow());
        file.close();
    }
    catch (...) {
        throw Base::RuntimeError("Cannot create resource");
    }

    if (!w)
        throw Base::ValueError("Invalid widget.");

    if (w->inherits("QDialog")) {
        myDlg = static_cast<QDialog*>(w);
    }
    else {
        myDlg = new ContainerDialog(w);
    }
}

/**
 * Makes a connection between the sender widget \a sender and its signal \a signal
 * of the created resource and Python callback function \a cb.
 * If the sender widget does not exist or no resource has been loaded this method returns false,
 * otherwise it returns true.
 */
bool PyResource::connect(const char* sender, const char* signal, PyObject* cb)
{
    if ( !myDlg )
        return false;

    QObject* objS=nullptr;
    QList<QWidget*> list = myDlg->findChildren<QWidget*>();
    QList<QWidget*>::const_iterator it = list.cbegin();
    QObject *obj;
    QString sigStr = QString::fromLatin1("2%1").arg(QString::fromLatin1(signal));

    while ( it != list.cend() ) {
        obj = *it;
        ++it;
        if (obj->objectName() == QLatin1String(sender)) {
            objS = obj;
            break;
        }
    }

    if (objS) {
        auto sc = new SignalConnect(this, cb);
        mySignals.push_back(sc);
        return QObject::connect(objS, sigStr.toLatin1(), sc, SLOT ( onExecute() )  );
    }
    else
        qWarning( "'%s' does not exist.\n", sender );

    return false;
}

Py::Object PyResource::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "Resource object";
    return Py::String(s_out.str());
}

/**
 * Searches for a widget and its value in the argument object \a args
 * to returns its value as Python object.
 * In the case it fails nullptr is returned.
 */
Py::Object PyResource::value(const Py::Tuple& args)
{
    char *psName;
    char *psProperty;
    if (!PyArg_ParseTuple(args.ptr(), "ss", &psName, &psProperty))
        throw Py::Exception();

    QVariant v;
    if (myDlg) {
        QList<QWidget*> list = myDlg->findChildren<QWidget*>();
        QList<QWidget*>::const_iterator it = list.cbegin();
        QObject *obj;

        bool fnd = false;
        while ( it != list.cend() ) {
            obj = *it;
            ++it;
            if (obj->objectName() == QLatin1String(psName)) {
                fnd = true;
                v = obj->property(psProperty);
                break;
            }
        }

        if ( !fnd )
            qWarning( "'%s' not found.\n", psName );
    }

    Py::Object item = Py::None();
    switch (v.userType())
    {
    case QMetaType::QStringList:
        {
            QStringList str = v.toStringList();
            int nSize = str.count();
            Py::List slist(nSize);
            for (int i=0; i<nSize;++i) {
                slist.setItem(i, Py::String(str[i].toLatin1()));
            }
            item = slist;
        }   break;
    case QMetaType::QByteArray:
        break;
    case QMetaType::QString:
        item = Py::String(v.toString().toLatin1());
        break;
    case QMetaType::Double:
        item = Py::Float(v.toDouble());
        break;
    case QMetaType::Bool:
        item = Py::Boolean(v.toBool());
        break;
    case QMetaType::UInt:
        item = Py::Long(static_cast<unsigned long>(v.toUInt()));
        break;
    case QMetaType::Int:
        item = Py::Int(v.toInt());
        break;
    default:
        item = Py::String("");
        break;
    }

    return item;
}

/**
 * Searches for a widget, its value name and the new value in the argument object \a args
 * to set even this new value.
 * In the case it fails nullptr is returned.
 */
Py::Object PyResource::setValue(const Py::Tuple& args)
{
    char *psName;
    char *psProperty;
    PyObject *psValue;
    if (!PyArg_ParseTuple(args.ptr(), "ssO", &psName, &psProperty, &psValue))
        throw Py::Exception();

    QVariant v;
    if (PyUnicode_Check(psValue)) {
        v = QString::fromUtf8(PyUnicode_AsUTF8(psValue));

    }
    else if (PyLong_Check(psValue)) {
        unsigned int val = PyLong_AsLong(psValue);
        v = val;
    }
    else if (PyFloat_Check(psValue)) {
        v = PyFloat_AsDouble(psValue);
    }
    else if (PyList_Check(psValue)) {
        QStringList str;
        int nSize = PyList_Size(psValue);
        for (int i=0; i<nSize;++i) {
            PyObject* item = PyList_GetItem(psValue, i);
            if (!PyUnicode_Check(item))
                continue;
            const char* pItem = PyUnicode_AsUTF8(item);
            str.append(QString::fromUtf8(pItem));
        }

        v = str;
    }
    else {
        throw Py::TypeError("Unsupported type");
    }

    if (myDlg) {
        QList<QWidget*> list = myDlg->findChildren<QWidget*>();
        QList<QWidget*>::const_iterator it = list.cbegin();
        QObject *obj;

        bool fnd = false;
        while ( it != list.cend() ) {
            obj = *it;
            ++it;
            if (obj->objectName() == QLatin1String(psName)) {
                fnd = true;
                obj->setProperty(psProperty, v);
                break;
            }
        }

        if (!fnd)
            qWarning( "'%s' not found.\n", psName );
    }

    return Py::None();
}

/**
 * If any resource has been loaded this methods shows it as a modal dialog.
 */
Py::Object PyResource::show(const Py::Tuple&)
{
    if (myDlg) {
        // small trick to get focus
        myDlg->showMinimized();

#ifdef Q_WS_X11
        // On X11 this may not work. For further information see QWidget::showMaximized
        //
        // workaround for X11
        myDlg->hide();
        myDlg->show();
#endif

        myDlg->showNormal();
        myDlg->exec();
    }

    return Py::None();
}

/**
 * Searches for the sender, the signal and the callback function to connect with
 * in the argument object \a args. In the case it fails nullptr is returned.
 */
Py::Object PyResource::connect(const Py::Tuple& args)
{
    char *psSender;
    char *psSignal;

    PyObject *temp;

    if (PyArg_ParseTuple(args.ptr(), "ssO", &psSender, &psSignal, &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            throw Py::Exception();
        }

        Py_XINCREF(temp);         /* Add a reference to new callback */
        std::string sSender = psSender;
        std::string sSignal = psSignal;

        if (!connect(psSender, psSignal, temp)) {
            // no signal object found => dispose the callback object
            Py_XDECREF(temp);  /* Dispose of callback */
        }

        return Py::None();
    }

    // error set by PyArg_ParseTuple
    throw Py::Exception();
}

// ----------------------------------------------------

SignalConnect::SignalConnect(PyObject* res, PyObject* cb)
  : myResource(res), myCallback(cb)
{
}

SignalConnect::~SignalConnect()
{
    Base::PyGILStateLocker lock;
    Py_XDECREF(myCallback);  /* Dispose of callback */
}

/**
 * Calls the callback function of the connected Python object.
 */
void SignalConnect::onExecute()
{
    PyObject *arglist;
    PyObject *result;

    /* Time to call the callback */
    arglist = Py_BuildValue("(O)", myResource);
#if PY_VERSION_HEX < 0x03090000
    result = PyEval_CallObject(myCallback, arglist);
#else
    result = PyObject_CallObject(myCallback, arglist);
#endif
    Py_XDECREF(result);
    Py_DECREF(arglist);
}

#include "moc_WidgetFactory.cpp"
