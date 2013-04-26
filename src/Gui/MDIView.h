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

#include "View.h"
#include <QMainWindow>

namespace Gui 
{
class Document;

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
    Q_OBJECT;

    TYPESYSTEM_HEADER();


public:
    /** View constructor
     * Attach the view to the given document. If the document is zero
     * the view will attach to the active document. Be aware, there isn't
     * always an active document.
     */
    MDIView(Gui::Document* pcDocument, QWidget* parent, Qt::WFlags wflags=0);
    /** View destructor
     * Detach the view from the document, if attached.
     */
    ~MDIView();

    /// get called when the document is updated
    virtual void onRelabel(Gui::Document *pDoc);
    virtual void viewAll();

    /// Message handler
    virtual bool onMsg(const char* pMsg,const char** ppReturn);
    /// Message handler test
    virtual bool onHasMsg(const char* pMsg) const;
    /// overwrite when checking on close state
    virtual bool canClose(void);
    /// delete itself
    virtual void deleteSelf();
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
    //@}

    QSize minimumSizeHint () const;

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
    virtual void windowStateChanged(MDIView*);

protected:
    void closeEvent(QCloseEvent *e);
    /** \internal */
    void changeEvent(QEvent *e);

private:
    ViewMode currentMode;
    Qt::WindowStates wstate;
};

} // namespace Gui

#endif // GUI_MDIVIEW_H 
