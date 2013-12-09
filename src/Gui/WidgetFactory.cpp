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

// Remove this block when activating PySide support!
#undef HAVE_SHIBOKEN
#undef HAVE_PYSIDE

#ifdef FC_OS_WIN32
#undef max
#undef min
#ifdef _MSC_VER
#pragma warning( disable : 4099 )
#pragma warning( disable : 4522 )
#endif
#endif

#ifdef HAVE_SHIBOKEN
# undef _POSIX_C_SOURCE
# undef _XOPEN_SOURCE
# include <basewrapper.h>
# include <sbkmodule.h>
# include <typeresolver.h>
# ifdef HAVE_PYSIDE
# include <pyside_qtcore_python.h>
# include <pyside_qtgui_python.h>
PyTypeObject** SbkPySide_QtCoreTypes=NULL;
PyTypeObject** SbkPySide_QtGuiTypes=NULL;
# endif
#endif

#include <CXX/Objects.hxx>
#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>


#include "WidgetFactory.h"
#include "PrefWidgets.h"
#include "PropertyPage.h"


using namespace Gui;


PythonWrapper::PythonWrapper()
{
}

QObject* PythonWrapper::toQObject(const Py::Object& pyobject)
{
    // http://pastebin.com/JByDAF5Z
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyTypeObject * type = Shiboken::SbkType<QObject>();
    if (type) {
        if (Shiboken::Object::checkType(pyobject.ptr())) {
            SbkObject* sbkobject = reinterpret_cast<SbkObject *>(pyobject.ptr());
            void* cppobject = Shiboken::Object::cppPointer(sbkobject, type);
            return reinterpret_cast<QObject*>(cppobject);
        }
    }
#else
    Py::Module mainmod(PyImport_AddModule((char*)"sip"));
    Py::Callable func = mainmod.getDict().getItem("unwrapinstance");
    Py::Tuple arguments(1);
    arguments[0] = pyobject; //PyQt pointer
    Py::Object result = func.apply(arguments);
    void* ptr = PyLong_AsVoidPtr(result.ptr());
    return reinterpret_cast<QObject*>(ptr);
#endif

    return 0;
}

Py::Object PythonWrapper::fromQWidget(QWidget* widget, const char* className)
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    PyTypeObject * type = Shiboken::SbkType<QWidget>();
    if (type) {
        SbkObjectType* sbk_type = reinterpret_cast<SbkObjectType*>(type);
        std::string typeName;
        if (className)
            typeName = className;
        else
            typeName = widget->metaObject()->className();
        PyObject* pyobj = Shiboken::Object::newObject(sbk_type, widget, false, false, typeName.c_str());
        return Py::asObject(pyobj);
    }
    throw Py::RuntimeError("Failed to wrap widget");
#else
    Py::Module sipmod(PyImport_AddModule((char*)"sip"));
    Py::Callable func = sipmod.getDict().getItem("wrapinstance");
    Py::Tuple arguments(2);
    arguments[0] = Py::asObject(PyLong_FromVoidPtr(widget));
    Py::Module qtmod(PyImport_ImportModule((char*)"PyQt4.Qt"));
    arguments[1] = qtmod.getDict().getItem("QWidget");
    return func.apply(arguments);
#endif
}

bool PythonWrapper::loadCoreModule()
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // QtCore
    if (!SbkPySide_QtCoreTypes) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide.QtCore"));
        if (requiredModule.isNull())
            return false;
        SbkPySide_QtCoreTypes = Shiboken::Module::getTypes(requiredModule);
    }
#endif
    return true;
}

bool PythonWrapper::loadGuiModule()
{
#if defined (HAVE_SHIBOKEN) && defined(HAVE_PYSIDE)
    // QtGui
    if (!SbkPySide_QtGuiTypes) {
        Shiboken::AutoDecRef requiredModule(Shiboken::Module::import("PySide.QtGui"));
        if (requiredModule.isNull())
            return false;
        SbkPySide_QtGuiTypes = Shiboken::Module::getTypes(requiredModule);
    }
#endif
    return true;
}

// ----------------------------------------------------

Gui::WidgetFactoryInst* Gui::WidgetFactoryInst::_pcSingleton = NULL;

WidgetFactoryInst& WidgetFactoryInst::instance()
{
    if (_pcSingleton == 0L)
        _pcSingleton = new WidgetFactoryInst;
    return *_pcSingleton;
}

void WidgetFactoryInst::destruct ()
{
    if (_pcSingleton != 0)
        delete _pcSingleton;
    _pcSingleton = 0;
}

/**
 * Creates a widget with the name \a sName which is a child of \a parent.
 * To create an instance of this widget once it must has been registered. 
 * If there is no appropriate widget registered 0 is returned.
 */
QWidget* WidgetFactoryInst::createWidget (const char* sName, QWidget* parent) const
{
    QWidget* w = (QWidget*)Produce(sName);

    // this widget class is not registered
    if (!w) {
#ifdef FC_DEBUG
        Base::Console().Warning("\"%s\" is not registered\n", sName);
#else
        Base::Console().Log("\"%s\" is not registered\n", sName);
#endif
        return 0;
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
        return 0;
    }

    // set the parent to the widget
    if (parent)
        w->setParent(parent);

    return w;
}

/**
 * Creates a widget with the name \a sName which is a child of \a parent.
 * To create an instance of this widget once it must has been registered. 
 * If there is no appropriate widget registered 0 is returned.
 */
Gui::Dialog::PreferencePage* WidgetFactoryInst::createPreferencePage (const char* sName, QWidget* parent) const
{
    Gui::Dialog::PreferencePage* w = (Gui::Dialog::PreferencePage*)Produce(sName);

    // this widget class is not registered
    if (!w) {
#ifdef FC_DEBUG
        Base::Console().Warning("\"%s\" is not registered\n", sName);
#else
        Base::Console().Log("\"%s\" is not registered\n", sName);
#endif
        return 0;
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
        return 0;
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
 * If there is no appropriate widget registered 0 is returned.
 * After creation of this widget its recent preferences are restored automatically.
 */
QWidget* WidgetFactoryInst::createPrefWidget(const char* sName, QWidget* parent, const char* sPref)
{
    QWidget* w = createWidget(sName);
    // this widget class is not registered
    if (!w)
        return 0; // no valid QWidget object

    // set the parent to the widget
    w->setParent(parent);

    try {
        dynamic_cast<PrefWidget*>(w)->setEntryName(sPref);
        dynamic_cast<PrefWidget*>(w)->restorePreferences();
    }
    catch (...) {
#ifdef FC_DEBUG
        Base::Console().Error("%s does not inherit from \"PrefWidget\"\n", w->metaObject()->className());
#endif
        delete w;
        return 0;
    }

    return w;
}

// ----------------------------------------------------

UiLoader::UiLoader(QObject* parent)
  : QUiLoader(parent)
{
    // do not use the plugins for additional widgets as we don't need them and
    // the application may crash under Linux (tested on Ubuntu 7.04 & 7.10).
    clearPluginPaths();
    this->cw = availableWidgets();
}

UiLoader::~UiLoader()
{
}

QWidget* UiLoader::createWidget(const QString & className, QWidget * parent,
                                const QString& name)
{
    if (this->cw.contains(className))
        return QUiLoader::createWidget(className, parent, name);
    QWidget* w = 0;
    if (WidgetFactory().CanProduce((const char*)className.toAscii()))
        w = WidgetFactory().createWidget((const char*)className.toAscii(), parent);
    if (w) w->setObjectName(name);
    return w;
}

// ----------------------------------------------------

PyObject *UiLoaderPy::PyMake(struct _typeobject *type, PyObject * args, PyObject * kwds)
{
    if (!PyArg_ParseTuple(args, ""))
        return 0;
    return new UiLoaderPy();
}

void UiLoaderPy::init_type()
{
    behaviors().name("UiLoader");
    behaviors().doc("UiLoader to create widgets");
    behaviors().type_object()->tp_new = &PyMake;
    // you must have overwritten the virtual functions
    behaviors().supportRepr();
    behaviors().supportGetattr();
    behaviors().supportSetattr();
    add_varargs_method("createWidget",&UiLoaderPy::createWidget,"createWidget()");
}

UiLoaderPy::UiLoaderPy()
{
}

UiLoaderPy::~UiLoaderPy()
{
}

Py::Object UiLoaderPy::repr()
{
    std::string s;
    std::ostringstream s_out;
    s_out << "Ui loader";
    return Py::String(s_out.str());
}

Py::Object UiLoaderPy::createWidget(const Py::Tuple& args)
{
    Gui::PythonWrapper wrap;

    // 1st argument
    std::string className = (std::string)Py::String(args[0]);

    // 2nd argument
    QWidget* parent = 0;
    if (wrap.loadCoreModule() && args.size() > 1) {
        QObject* object = wrap.toQObject(args[1]);
        if (object)
            parent = qobject_cast<QWidget*>(object);
    }

    // 3rd argument
    std::string objectName;
    if (args.size() > 2) {
        objectName = (std::string)Py::String(args[2]);
    }

    QWidget* widget = loader.createWidget(QString::fromAscii(className.c_str()), parent,
        QString::fromAscii(objectName.c_str()));
    wrap.loadGuiModule();
    return wrap.fromQWidget(widget);
}

// ----------------------------------------------------

WidgetFactorySupplier* WidgetFactorySupplier::_pcSingleton = 0L;

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
    _pcSingleton=0;
}

// ----------------------------------------------------

PrefPageUiProducer::PrefPageUiProducer (const char* filename, const char* group)
  : fn(QString::fromUtf8(filename))
{
    WidgetFactoryInst::instance().AddProducer(filename, this);
    Gui::Dialog::DlgPreferencesImp::addPage(filename, group);
}

PrefPageUiProducer::~PrefPageUiProducer()
{
}

void* PrefPageUiProducer::Produce () const
{
    QWidget* page = new Gui::Dialog::PreferenceUiForm(fn);
    return (void*)page;
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

    setSizeGripEnabled( TRUE );
    MyDialogLayout = new QGridLayout(this);

    buttonOk = new QPushButton(this);
    buttonOk->setObjectName(QLatin1String("buttonOK"));
    buttonOk->setText( tr( "&OK" ) );
    buttonOk->setAutoDefault( TRUE );
    buttonOk->setDefault( TRUE );

    MyDialogLayout->addWidget( buttonOk, 1, 0 );
    QSpacerItem* spacer = new QSpacerItem( 210, 20, QSizePolicy::Expanding, QSizePolicy::Minimum );
    MyDialogLayout->addItem( spacer, 1, 1 );

    buttonCancel = new QPushButton(this);
    buttonCancel->setObjectName(QLatin1String("buttonCancel"));
    buttonCancel->setText( tr( "&Cancel" ) );
    buttonCancel->setAutoDefault( TRUE );

    MyDialogLayout->addWidget( buttonCancel, 1, 2 );

    templChild->setParent(this);

    MyDialogLayout->addWidget( templChild, 0, 0, 0, 2 );
    resize( QSize(307, 197).expandedTo(minimumSizeHint()) );

    // signals and slots connections
    connect( buttonOk, SIGNAL( clicked() ), this, SLOT( accept() ) );
    connect( buttonCancel, SIGNAL( clicked() ), this, SLOT( reject() ) );
}

/** Destroys the object and frees any allocated resources */
ContainerDialog::~ContainerDialog()
{
}

// ----------------------------------------------------

//--------------------------------------------------------------------------
// Type structure
//--------------------------------------------------------------------------

PyTypeObject PyResource::Type = {
                                    PyObject_HEAD_INIT(&PyType_Type)
                                    0,                    /*ob_size*/
                                    "PyResource",         /*tp_name*/
                                    sizeof(PyResource),   /*tp_basicsize*/
                                    0,                    /*tp_itemsize*/
                                    /* methods */
                                    PyDestructor,         /*tp_dealloc*/
                                    0,                    /*tp_print*/
                                    __getattr,            /*tp_getattr*/
                                    __setattr,            /*tp_setattr*/
                                    0,                    /*tp_compare*/
                                    __repr,               /*tp_repr*/
                                    0,                    /*tp_as_number*/
                                    0,                    /*tp_as_sequence*/
                                    0,                    /*tp_as_mapping*/
                                    0,                    /*tp_hash*/
                                    0,                    /*tp_call */
                                  };

//--------------------------------------------------------------------------
// Methods structure
//--------------------------------------------------------------------------
PyMethodDef PyResource::Methods[] = {
                                        {"GetValue",       (PyCFunction) svalue,    Py_NEWARGS},
                                        {"SetValue",       (PyCFunction) ssetValue, Py_NEWARGS},
                                        {"Show",           (PyCFunction) sshow,     Py_NEWARGS},
                                        {"Connect",        (PyCFunction) sconnect,  Py_NEWARGS},

                                        {NULL, NULL}      /* Sentinel */
                                      };

//--------------------------------------------------------------------------
// Parents structure
//--------------------------------------------------------------------------
PyParentObject PyResource::Parents[] = {&PyObjectBase::Type,&PyResource::Type, NULL};

//--------------------------------------------------------------------------
// constructor
//--------------------------------------------------------------------------
PyResource::PyResource(PyTypeObject *T)
    : PyObjectBase(0, T), myDlg(0L)
{
}

PyObject *PyResource::PyMake(PyObject *ignored, PyObject *args) // Python wrapper
{
    //return new FCPyResource();      // Make new Python-able object
    return 0;
}

//--------------------------------------------------------------------------
//  FCPyResource destructor
//--------------------------------------------------------------------------
PyResource::~PyResource()
{
    delete myDlg;
    for (std::vector<SignalConnect*>::iterator it = mySingals.begin(); it != mySingals.end(); ++it) {
        SignalConnect* sc = *it;
        delete sc;
    }
}

//--------------------------------------------------------------------------
// FCPyParametrGrp Attributes
//--------------------------------------------------------------------------
PyObject *PyResource::_getattr(char *attr)        // __getattr__ function: note only need to handle new state
{
    _getattr_up(PyObjectBase);            // send to parent
    return 0;
}

int PyResource::_setattr(char *attr, PyObject *value)   // __setattr__ function: note only need to handle new state
{
    return PyObjectBase::_setattr(attr, value); // send up to parent
    return 0;
}

/**
 * Loads an .ui file with the name \a name. If the .ui file cannot be found or the QWidgetFactory
 * cannot create an instance an exception is thrown. If the created resource does not inherit from
 * QDialog an instance of ContainerDialog is created to embed it.
 */
void PyResource::load( const char* name )
{
    QString fn = QString::fromUtf8(name);
    QFileInfo fi(fn);

    // checks whether it's a relative path
    if (fi.isRelative()) {
        QString cwd = QDir::currentPath ();
        QString home= QDir(QString::fromUtf8(App::GetApplication().GetHomePath())).path();

        // search in cwd and home path for the file
        //
        // file does not reside in cwd, check home path now
        if (!fi.exists()) {
            if (cwd == home) {
                QString what = QObject::tr("Cannot find file %1").arg(fi.absoluteFilePath());
                throw Base::Exception(what.toUtf8().constData());
            }
            else {
                fi.setFile( QDir(home), fn );

                if (!fi.exists()) {
                    QString what = QObject::tr("Cannot find file %1 neither in %2 nor in %3")
                        .arg(fn).arg(cwd).arg(home);
                    throw Base::Exception(what.toUtf8().constData());
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
            throw Base::Exception(what.toUtf8().constData());
        }
    }

    QWidget* w=0;
    try {
        UiLoader loader;
#if QT_VERSION >= 0x040500
        loader.setLanguageChangeEnabled(true);
#endif
        QFile file(fn);
        if (file.open(QFile::ReadOnly))
            w = loader.load(&file, QApplication::activeWindow());
        file.close();
    }
    catch (...) {
        throw Base::Exception("Cannot create resource");
    }

    if (!w)
        throw Base::Exception("Invalid widget.");

    if (w->inherits("QDialog")) {
        myDlg = (QDialog*)w;
    }
    else {
        myDlg = new ContainerDialog(w);
    }
}

/**
 * Makes a connection between the sender widget \a sender and its signal \a signal
 * of the created resource and Python callback function \a cb.
 * If the sender widget does not exist or no resource has been loaded this method returns FALSE, 
 * otherwise it returns TRUE.
 */
bool PyResource::connect(const char* sender, const char* signal, PyObject* cb)
{
    if ( !myDlg )
        return false;

    QObject* objS=0L;
    QList<QWidget*> list = myDlg->findChildren<QWidget*>();
    QList<QWidget*>::const_iterator it = list.begin();
    QObject *obj;
    QString sigStr = QString::fromAscii("2%1").arg(QString::fromAscii(signal));

    while ( it != list.end() ) {
        obj = *it;
        ++it;
        if (obj->objectName() == QLatin1String(sender)) {
            objS = obj;
            break;
        }
    }

    if (objS) {
        SignalConnect* sc = new SignalConnect(this, cb, objS);
        mySingals.push_back(sc);
        return QObject::connect(objS, sigStr.toAscii(), sc, SLOT ( onExecute() )  );
    }
    else
        qWarning( "'%s' does not exist.\n", sender );

    return false;
}

/**
 * Searches for the sender, the signal and the callback function to connect with
 * in the argument object \a args. In the case it fails 0 is returned.
 */
PyObject *PyResource::connect(PyObject *args)
{
    char *psSender;
    char *psSignal;

    PyObject *result = NULL;
    PyObject *temp;

    if (PyArg_ParseTuple(args, "ssOset_callback", &psSender, &psSignal, &temp)) {
        if (!PyCallable_Check(temp)) {
            PyErr_SetString(PyExc_TypeError, "parameter must be callable");
            return NULL;
        }

        Py_XINCREF(temp);         /* Add a reference to new callback */
        std::string sSender = psSender;
        std::string sSignal = psSignal;

        if (!connect(psSender, psSignal, temp)) {
            // no signal object found => dispose the callback object
            Py_XDECREF(temp);  /* Dispose of callback */
        }

        /* Boilerplate to return "None" */
        Py_INCREF(Py_None);
        result = Py_None;
    }

    return result;
}

/**
 * If any resouce has been loaded this methods shows it as a modal dialog.
 */
PyObject *PyResource::show(PyObject *args)
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

    Py_INCREF(Py_None);
    return Py_None;
}

/**
 * Searches for a widget and its value in the argument object \a args
 * to returns its value as Python object.
 * In the case it fails 0 is returned.
 */
PyObject *PyResource::value(PyObject *args)
{
    char *psName;
    char *psProperty;
    if (!PyArg_ParseTuple(args, "ss", &psName, &psProperty))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    QVariant v;
    if (myDlg) {
        QList<QWidget*> list = myDlg->findChildren<QWidget*>();
        QList<QWidget*>::const_iterator it = list.begin();
        QObject *obj;

        bool fnd = false;
        while ( it != list.end() ) {
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

    PyObject *pItem=0;
    switch (v.type())
    {
    case QVariant::StringList:
        {
            QStringList str = v.toStringList();
            int nSize = str.count();
            PyObject* slist = PyList_New(nSize);
            for (int i=0; i<nSize;++i) {
                PyObject* item = PyString_FromString(str[i].toAscii());
                PyList_SetItem(slist, i, item);
            }
        }   break;
    case QVariant::ByteArray:
        break;
    case QVariant::String:
        pItem = PyString_FromString(v.toString().toAscii());
        break;
    case QVariant::Double:
        pItem = PyFloat_FromDouble(v.toDouble());
        break;
    case QVariant::Bool:
        pItem = PyInt_FromLong(v.toBool() ? 1 : 0);
        break;
    case QVariant::UInt:
        pItem = PyLong_FromLong(v.toUInt());
        break;
    case QVariant::Int:
        pItem = PyInt_FromLong(v.toInt());
        break;
    default:
        pItem = PyString_FromString("");
        break;
    }

    return pItem;
}

/**
 * Searches for a widget, its value name and the new value in the argument object \a args
 * to set even this new value.
 * In the case it fails 0 is returned.
 */
PyObject *PyResource::setValue(PyObject *args)
{
    char *psName;
    char *psProperty;
    PyObject *psValue;
    if (!PyArg_ParseTuple(args, "ssO", &psName, &psProperty, &psValue))     // convert args: Python->C
        return NULL;                             // NULL triggers exception

    QVariant v;
    if (PyString_Check(psValue)) {
        v = QString::fromAscii(PyString_AsString(psValue));
    }
    else if (PyInt_Check(psValue)) {
        int val = PyInt_AsLong(psValue);
        v = val;
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
            if (!PyString_Check(item))
                continue;

            char* pItem = PyString_AsString(item);
            str.append(QString::fromAscii(pItem));
        }

        v = str;
    }
    else {
        PyErr_SetString(PyExc_AssertionError, "Unsupported type");
        return NULL;
    }

    if (myDlg) {
        QList<QWidget*> list = myDlg->findChildren<QWidget*>();
        QList<QWidget*>::const_iterator it = list.begin();
        QObject *obj;

        bool fnd = false;
        while ( it != list.end() ) {
            obj = *it;
            ++it;
            if (obj->objectName() == QLatin1String(psName)) {
                fnd = true;
                obj->setProperty(psProperty, v);
                break;
            }
        }

        if ( !fnd )
            qWarning( "'%s' not found.\n", psName );
    }

    Py_INCREF(Py_None);
    return Py_None;
}

// ----------------------------------------------------

SignalConnect::SignalConnect( Base::PyObjectBase* res, PyObject* cb, QObject* sender)
  : myResource(res), myCallback(cb), mySender(sender)
{
}

SignalConnect::~SignalConnect()
{
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
    result = PyEval_CallObject(myCallback, arglist);
    (void)result;
    Py_DECREF(arglist);
}

#include "moc_WidgetFactory.cpp"
