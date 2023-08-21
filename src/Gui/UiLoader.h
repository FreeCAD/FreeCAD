/***************************************************************************
 *   Copyright (c) 2021 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_UILOADER_H
#define GUI_UILOADER_H

#if !defined (__MINGW32__)
#define HAVE_QT_UI_TOOLS
#endif

#if defined (HAVE_QT_UI_TOOLS)
#include <QUiLoader>
#else
#include <QObject>
#endif

#include <CXX/Extensions.hxx>
#include <memory>


QT_BEGIN_NAMESPACE
class QLayout;
class QAction;
class QActionGroup;
class QDir;
class QIODevice;
class QWidget;
QT_END_NAMESPACE


#if !defined (HAVE_QT_UI_TOOLS)
class QUiLoader : public QObject
{
    Q_OBJECT
public:
    explicit QUiLoader(QObject* parent = nullptr);
    ~QUiLoader();

    QStringList pluginPaths() const;
    void clearPluginPaths();
    void addPluginPath(const QString& path);

    QWidget* load(QIODevice* device, QWidget* parentWidget = nullptr);
    QStringList availableWidgets() const;
    QStringList availableLayouts() const;

    virtual QWidget* createWidget(const QString& className, QWidget* parent = nullptr, const QString& name = QString());
    virtual QLayout* createLayout(const QString& className, QObject* parent = nullptr, const QString& name = QString());
    virtual QActionGroup* createActionGroup(QObject* parent = nullptr, const QString& name = QString());
    virtual QAction* createAction(QObject* parent = nullptr, const QString& name = QString());

    void setWorkingDirectory(const QDir& dir);
    QDir workingDirectory() const;

    void setLanguageChangeEnabled(bool enabled);
    bool isLanguageChangeEnabled() const;

    void setTranslationEnabled(bool enabled);
    bool isTranslationEnabled() const;

    QString errorString() const;

private:
    Py::Object uiloader;
};
#endif

namespace Gui {

class PySideUicModule : public Py::ExtensionModule<PySideUicModule>
{

public:
    PySideUicModule();
    ~PySideUicModule() override = default;

private:
    Py::Object loadUiType(const Py::Tuple& args);
    Py::Object loadUi(const Py::Tuple& args);
    Py::Object createCustomWidget(const Py::Tuple&);
};

/**
 * The UiLoader class provides the abitlity to use the widget factory
 * framework of FreeCAD within the framework provided by Qt. This class
 * extends QUiLoader by the creation of FreeCAD specific widgets.
 * @author Werner Mayer
 */
class UiLoader : public QUiLoader
{
protected:
    /**
     * A protected construct for UiLoader.
     * To create an instance of UiLoader @see UiLoader::newInstance()
     */
    explicit UiLoader(QObject* parent=nullptr);

public:
    /**
     * Creates a new instance of a UiLoader.
     *
     * Due to its flaw the QUiLoader upon creation loads every available Qt
     * designer plugin it can find in QApplication::libraryPaths(). Some of
     * those plugins may perform some unexpected actions upon load which may
     * interfere with FreeCAD's functionality. Only way to avoid such behaviour
     * is to reset QApplication::libraryPaths, create a QUiLoader and then
     * restore the libs paths. Hence need for this function to wrap
     * construction.
     *
     * @see https://github.com/FreeCAD/FreeCAD/issues/8708
     */
    static std::unique_ptr<UiLoader> newInstance(QObject *parent=nullptr);

    ~UiLoader() override;

    /**
     * Creates a widget of the type \a className with the parent \a parent.
     * For more details see the documentation to QWidgetFactory.
     */
    QWidget* createWidget(const QString & className, QWidget * parent=nullptr,
                          const QString& name = QString()) override;

private:
    QStringList cw;
};

// --------------------------------------------------------------------

class UiLoaderPy : public Py::PythonExtension<UiLoaderPy>
{
public:
    static void init_type();    // announce properties and methods

    UiLoaderPy();
    ~UiLoaderPy() override;

    Py::Object repr() override;
    Py::Object createWidget(const Py::Tuple&);
    Py::Object load(const Py::Tuple&);

    Py::Object addPluginPath(const Py::Tuple&);
    Py::Object clearPluginPaths(const Py::Tuple&);
    Py::Object pluginPaths(const Py::Tuple&);
    Py::Object availableWidgets(const Py::Tuple&);
    Py::Object errorString(const Py::Tuple&);
    Py::Object isLanguageChangeEnabled(const Py::Tuple&);
    Py::Object setLanguageChangeEnabled(const Py::Tuple&);
    Py::Object setWorkingDirectory(const Py::Tuple&);
    Py::Object workingDirectory(const Py::Tuple&);

private:
    static PyObject *PyMake(struct _typeobject *, PyObject *, PyObject *);

private:
    std::unique_ptr<UiLoader> loader;
};

} // namespace Gui

#endif // GUI_UILOADER_H
