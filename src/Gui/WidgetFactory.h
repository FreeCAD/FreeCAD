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


#ifndef GUI_WIDGETFACTORY_H
#define GUI_WIDGETFACTORY_H

#include <vector>
#include <QUiLoader>

#include <Base/Factory.h>
#include <Base/PyObjectBase.h>
#include "DlgPreferencesImp.h"
#include "DlgCustomizeImp.h"
#include "PropertyPage.h"
#include <CXX/Extensions.hxx>

namespace Gui {
  namespace Dialog{
    class PreferencePage;
  }

class GuiExport PythonWrapper
{
public:
    PythonWrapper();
    bool loadCoreModule();
    bool loadGuiModule();
    bool loadWidgetsModule();

    bool toCString(const Py::Object&, std::string&);
    QObject* toQObject(const Py::Object&);
    Py::Object fromQWidget(QWidget*, const char* className=0);
    const char* getWrapperName(QObject*) const;
    /*!
      Create a Python wrapper for the icon. The icon must be created on the heap
      and the Python wrapper takes ownership of it.
     */
    Py::Object fromQIcon(const QIcon*);
    QIcon *toQIcon(PyObject *pyobj);
    static void createChildrenNameAttributes(PyObject* root, QObject* object);
    static void setParent(PyObject* pyWdg, QObject* parent);
};

class PySideUicModule : public Py::ExtensionModule<PySideUicModule>
{

public:
    PySideUicModule();
    virtual ~PySideUicModule() {}

private:
    Py::Object loadUiType(const Py::Tuple& args);
    Py::Object loadUi(const Py::Tuple& args);
};

/** 
 * The widget factory provides methods for the dynamic creation of widgets.
 * To create these widgets once they must be registered to the factory.
 * To register them use WidgetProducer or any subclasses; to register a
 * preference page use PrefPageProducer instead.
 * \author Werner Mayer
 */
class GuiExport WidgetFactoryInst : public Base::Factory
{
public:
    static WidgetFactoryInst& instance();
    static void destruct ();

    QWidget* createWidget (const char* sName, QWidget* parent=0) const;
    Gui::Dialog::PreferencePage* createPreferencePage (const char* sName, QWidget* parent=0) const;
    QWidget* createPrefWidget(const char* sName, QWidget* parent, const char* sPref);

private:
    static WidgetFactoryInst* _pcSingleton;

    WidgetFactoryInst(){}
    ~WidgetFactoryInst(){}
};

inline WidgetFactoryInst& WidgetFactory()
{
    return WidgetFactoryInst::instance();
}

// --------------------------------------------------------------------

/**
 * The UiLoader class provides the abitlity to use the widget factory 
 * framework of FreeCAD within the framework provided by Qt. This class
 * extends QUiLoader by the creation of FreeCAD specific widgets.
 * @author Werner Mayer
 */
class UiLoader : public QUiLoader
{
public:
    UiLoader(QObject* parent=0);
    virtual ~UiLoader();

    /**
     * Creates a widget of the type \a className with the parent \a parent.
     * For more details see the documentation to QWidgetFactory.
     */
    QWidget* createWidget(const QString & className, QWidget * parent=0, 
                          const QString& name = QString());
private:
    QStringList cw;
};

// --------------------------------------------------------------------

class UiLoaderPy : public Py::PythonExtension<UiLoaderPy> 
{
public:
    static void init_type(void);    // announce properties and methods

    UiLoaderPy();
    ~UiLoaderPy();

    Py::Object repr();
    Py::Object createWidget(const Py::Tuple&);
    Py::Object load(const Py::Tuple&);

private:
    static PyObject *PyMake(struct _typeobject *, PyObject *, PyObject *);

private:
    UiLoader loader;
};

// --------------------------------------------------------------------

/**
 * The WidgetProducer class is a value-based template class that provides 
 * the ability to create widgets dynamically. 
 * \author Werner Mayer
 */
template <class CLASS>
class WidgetProducer : public Base::AbstractProducer
{
public:
    /** 
     * Register a special type of widget to the WidgetFactoryInst.
     */
    WidgetProducer ()
    {
        const char* cname = CLASS::staticMetaObject.className();
        WidgetFactoryInst::instance().AddProducer(cname, this);
    }

    virtual ~WidgetProducer (){}

    /** 
     * Creates an instance of the specified widget.
     */
    virtual void* Produce () const
    {
        return (new CLASS);
    }
};

// --------------------------------------------------------------------

/**
 * The PrefPageProducer class is a value-based template class that provides 
 * the ability to create preference pages dynamically. 
 * \author Werner Mayer
 */
template <class CLASS>
class PrefPageProducer : public Base::AbstractProducer
{
public:
    /** 
     * Register a special type of preference page to the WidgetFactoryInst.
     */
    PrefPageProducer (const char* group)
    {
        const char* cname = CLASS::staticMetaObject.className();
        if (strcmp(cname, Gui::Dialog::PreferencePage::staticMetaObject.className()) == 0)
            qWarning("The class '%s' lacks of Q_OBJECT macro", typeid(CLASS).name());
        if (WidgetFactoryInst::instance().CanProduce(cname)) {
            qWarning("The preference page class '%s' is already registered", cname);
        }
        else {
            WidgetFactoryInst::instance().AddProducer(cname, this);
            Gui::Dialog::DlgPreferencesImp::addPage(cname, group);
        }
    }

    virtual ~PrefPageProducer (){}

    /**
     * Creates an instance of the specified widget.
     */
    virtual void* Produce () const
    {
        return (new CLASS);
    }
};

/**
 * The PrefPageUiProducer class provides the ability to create preference pages
 * dynamically from an external UI file.
 * @author Werner Mayer
 */
class GuiExport PrefPageUiProducer : public Base::AbstractProducer
{
public:
    /** 
     * Register a special type of preference page to the WidgetFactoryInst.
     */
    PrefPageUiProducer (const char* filename, const char* group);
    virtual ~PrefPageUiProducer ();
    /**
     * Creates an instance of the specified widget.
     */
    virtual void* Produce () const;

private:
    QString fn;
};

/**
 * The PrefPagePyProducer class provides the ability to create preference pages
 * dynamically from a Python class.
 * @author Werner Mayer
 */
class GuiExport PrefPagePyProducer : public Base::AbstractProducer
{
public:
    /** 
     * Register a special type of preference page to the WidgetFactoryInst.
     */
    PrefPagePyProducer (const Py::Object&, const char* group);
    virtual ~PrefPagePyProducer ();
    /**
     * Creates an instance of the specified widget.
     */
    virtual void* Produce () const;

private:
    Py::Object type;
};

// --------------------------------------------------------------------

/**
 * The CustomPageProducer class is a value-based template class that provides 
 * the ability to create custom pages dynamically. 
 * \author Werner Mayer
 */
template <class CLASS>
class CustomPageProducer : public Base::AbstractProducer
{
public:
    /** 
     * Register a special type of customize page to the WidgetFactoryInst.
     */
    CustomPageProducer ()
    {
        const char* cname = CLASS::staticMetaObject.className();
        if (strcmp(cname, Gui::Dialog::CustomizeActionPage::staticMetaObject.className()) == 0)
            qWarning("The class '%s' lacks of Q_OBJECT macro", typeid(CLASS).name());
        if (WidgetFactoryInst::instance().CanProduce(cname)) {
            qWarning("The preference page class '%s' is already registered", cname);
        }
        else {
            WidgetFactoryInst::instance().AddProducer(cname, this);
            Gui::Dialog::DlgCustomizeImp::addPage(cname);
        }
    }

    virtual ~CustomPageProducer (){}

    /** 
     * Creates an instance of the specified widget.
     */
    virtual void* Produce () const
    {
        return (new CLASS);
    }
};

// --------------------------------------------------------------------

/**
 * The widget factory supplier class registers all kinds of
 * preference pages and widgets.
 * \author Werner Mayer
 */
class WidgetFactorySupplier
{
private:
    // Singleton
    WidgetFactorySupplier();
    static WidgetFactorySupplier *_pcSingleton;

public:
    static WidgetFactorySupplier &instance();
    static void destruct();
    friend WidgetFactorySupplier &GetWidgetFactorySupplier();
};

inline WidgetFactorySupplier &GetWidgetFactorySupplier()
{
    return WidgetFactorySupplier::instance();
}

// ----------------------------------------------------

/**
 * The ContainerDialog class acts as a container to embed any kinds of widgets that
 * do not inherit from QDialog. This class also provides an "Ok" and a "Cancel" button.
 * At most this class is used to embed widgets which are created from .ui files.
 * \author Werner Mayer
 */
class ContainerDialog : public QDialog
{
    Q_OBJECT

public:
    ContainerDialog( QWidget* templChild );
    ~ContainerDialog();

    QPushButton* buttonOk; /**< The Ok button. */
    QPushButton* buttonCancel; /**< The cancel button. */

private:
    QGridLayout* MyDialogLayout;
};

// ----------------------------------------------------

/**
 * The PyResource class provides an interface to create widgets or to load .ui files from Python.
 * With
 * \code
 * d = Gui.createDialog("test.ui")
 * \endcode
 *
 * you can create a PyResource object containing the widget. If a relative file name 
 * is given PyResource looks first in the current working directory and afterwards in 
 * the home path where FreeCAD resides.
 * 
 * If the appropriate .ui file cannot be found or creation fails an exception is thrown.
 * In case the widget in the .ui file does not inherit from QDialog it is embedded in a
 * \ref ContainerDialog object.
 * To show the widget you can call
 * \code
 * d.Show()
 * \endcode
 *
 * Furthermore it is possible to get or set values from any widgets inside
 * the parent widget or to connect a Python callback function with a widget.
 * \remark The callback function must have exactly one parameter. This parameter
 * points to the dialog you have just created.
 * \code
 * # define a callback function with one argument
 * def TestCall(obj):
 *      # sets the value from lineedit if "Button_Name" was pressed
 *      obj.setValue("lineedit", "text", "Show this text here!")
 *      print "Button clicked"
 *
 * d = Gui.createDialog("test.ui")
 * d.connect("Button_Name", "clicked()", TestCall)
 * d.show()
 * \endcode
 *
 * If the button with the name "Button_Name" is clicked the message "Button clicked" is
 * printed.
 * For example if you have a QLineEdit inside your widget you can set the text with
 * \code
 * # sets "Show this text here!" to the text property
 * d.setValue("lineedit", "text", "Show this text here!")
 * d.show()
 * \endcode
 *
 * or retrieve the entered text with
 * \code
 * f = d.getValue("lineedit", "text")
 * print f
 * \endcode
 *
 * \author Werner Mayer
 */

class PyResource : public Py::PythonExtension<PyResource>
{
public:
    static void init_type(void);    // announce properties and methods

    PyResource();
    ~PyResource();

    void load(const char* name);
    bool connect(const char* sender, const char* signal, PyObject* cb);

    Py::Object repr();

    Py::Object value(const Py::Tuple&);
    Py::Object setValue(const Py::Tuple&);
    Py::Object show(const Py::Tuple&);
    Py::Object connect(const Py::Tuple&);

private:
    std::vector<class SignalConnect*> mySignals;
    QDialog* myDlg;
};

/**
 * The SignalConnect class provides the abitlity to make a connection
 * between the callback function of a Python object and the slot onExecute().
 * This mechanism is used in the Python/Qt framework.
 * \author Werner Mayer
 */
class SignalConnect : public QObject
{
    Q_OBJECT

public:
    SignalConnect(PyObject* res, PyObject* cb);
    ~SignalConnect();

public Q_SLOTS:
    void onExecute();

private:
    PyObject* myResource;
    PyObject* myCallback;
};

// ----------------------------------------------------
namespace Dialog {

/** Subclass that embeds a form from a Python class.
 * \author Werner Mayer
 */
class GuiExport PreferencePagePython : public PreferencePage
{
    Q_OBJECT

public:
    PreferencePagePython(const Py::Object& dlg, QWidget* parent = 0);
    virtual ~PreferencePagePython();

    void loadSettings();
    void saveSettings();

protected:
    void changeEvent(QEvent *e);

private:
    Py::Object page;
};

} // namespace Dialog

} // namespace Gui

#endif // GUI_WIDGETFACTORY_H
