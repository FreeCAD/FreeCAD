/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_MDIVIEW_H
#define GUI_MDIVIEW_H

#include <boost_signals2.hpp>
#include <QMainWindow>
#include <Gui/ActiveObjectList.h>
#include <Gui/View.h>


QT_BEGIN_NAMESPACE
class QPrinter;
QT_END_NAMESPACE

namespace Gui
{
class Document;
class MainWindow;
class ViewProvider;
class ViewProviderDocumentObject;

/** Base class of all windows belonging to a document.
 * There are two ways of belonging to a document:
 * \li belong to a fix document
 * \li always belong to the active document
 * The latter means whenever the active document is changing the view belongs to
 * this document. It also means that the view belongs sometimes to no document at
 * all.
 * @see TreeView
 * @see Gui::Document
 * @see Application
 * @author Jürgen Riegel, Werner Mayer
 */
class GuiExport MDIView : public QMainWindow, public BaseView
{
    Q_OBJECT

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /** View constructor
     * Attach the view to the given document. If the document is zero
     * the view will attach to the active document. Be aware, there isn't
     * always an active document.
     */
    MDIView(Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags=Qt::WindowFlags());
    /** View destructor
     * Detach the view from the document, if attached.
     */
    ~MDIView() override;

    /// get called when the document is updated
    void onRelabel(Gui::Document *pDoc) override;
    virtual void viewAll();

    /// build window title
    QString buildWindowTitle();

    /// Message handler
    bool onMsg(const char* pMsg,const char** ppReturn) override;
    /// Message handler test
    bool onHasMsg(const char* pMsg) const override;
    /// overwrite when checking on close state
    bool canClose() override;
    /// delete itself
    void deleteSelf() override;
    PyObject *getPyObject() override;
    /** @name Printing */
    //@{
public Q_SLOTS:
    virtual void print(QPrinter* printer);

public:
    /** Print content of view */
    virtual void print();
    /** Print to PDF file */
    virtual void printPdf();
    /** Show a preview dialog */
    virtual void printPreview();
    /** Save the printer configuration */
    void savePrinterSettings(QPrinter* printer);
    /** Restore the printer configuration */
    void restorePrinterSettings(QPrinter* printer);
    //@}

    /** @name Undo/Redo actions */
    //@{
    virtual QStringList undoActions() const;
    virtual QStringList redoActions() const;
    //@}

    QSize minimumSizeHint () const override;

    /// MDI view mode enum
    enum ViewMode {
        Child,      /**< Child viewing, view is docked inside the MDI application window */
        TopLevel,   /**< The view becomes a top level window and can be moved outsinde the application window */
        FullScreen  /**< The view goes to full screen viewing */
    };
    /**
     * If \a b is set to \a FullScreen the MDI view is displayed in full screen mode, if \a b
     * is set to \a TopLevel then it is displayed as an own top-level window, otherwise (\a Normal)
     * as tabbed window. For more hints refer to the Qt documentation to
     * QWidget::showFullScreen ().
     */
    virtual void setCurrentViewMode(ViewMode mode);
    ViewMode currentViewMode() const { return currentMode; }


    /// access getter for the active object list
    template<typename _T>
    inline _T getActiveObject(const char* name, App::DocumentObject **parent=nullptr, std::string *subname=nullptr) const
    {
        return ActiveObjects.getObject<_T>(name,parent,subname);
    }
    void setActiveObject(App::DocumentObject*o, const char*n, const char *subname=nullptr)
    {
        ActiveObjects.setObject(o, n, subname);
    }
    bool hasActiveObject(const char*n) const
    {
        return ActiveObjects.hasObject(n);
    }
    bool isActiveObject(App::DocumentObject*o, const char*n, const char *subname=nullptr) const
    {
        return ActiveObjects.hasObject(o,n,subname);
    }

    /*!
     * \brief containsViewProvider
     * Checks if the given view provider is part of this view. The default implementation
     * returns false.
     * \return bool
     */
    virtual bool containsViewProvider(const ViewProvider*) const {
        return false;
    }

public Q_SLOTS:
    virtual void setOverrideCursor(const QCursor&);
    virtual void restoreOverrideCursor();

Q_SIGNALS:
    void message(const QString&, int);

protected Q_SLOTS:
    /** This method gets called from the main window this view is attached to
     * whenever the window state of the active view changes.
     * The default implementation does nothing.
     */
    virtual void windowStateChanged(QWidget*);

protected:
    void closeEvent(QCloseEvent *e) override;
    /** \internal */
    void changeEvent(QEvent *e) override;

protected:
    PyObject* pythonObject;

private:
    ViewMode currentMode;
    Qt::WindowStates wstate;
    // list of active objects of this view
    ActiveObjectList ActiveObjects;
    using Connection = boost::signals2::connection;
    Connection connectDelObject; //remove active object upon delete.

    friend class MainWindow;
};

} // namespace Gui

#endif // GUI_MDIVIEW_H
