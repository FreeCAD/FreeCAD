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


#ifndef GUI_VIEW3DINVENTOR_H
#define GUI_VIEW3DINVENTOR_H

#include "MDIView.h"

#include <Base/Parameter.h>
#include <QImage>

class SoNode;
class QPrinter;
class QStackedWidget;

namespace Gui {

class Document;
class View3DInventorViewer;
class View3DPy;

class GuiExport GLOverlayWidget : public QWidget
{
    Q_OBJECT;

public:
    GLOverlayWidget(QWidget* parent=0) : QWidget(parent)
    {}
    ~GLOverlayWidget()
    {}
    virtual void setImage(const QImage& img)
    { image = img; }
    void paintEvent(QPaintEvent*);

protected:
    QImage image;
};

/** The 3D view window
 *  It consists out of the 3D view 
 *  \author Jürgen Riegel
 */
class GuiExport View3DInventor : public MDIView, public ParameterGrp::ObserverType
{
    Q_OBJECT;

    TYPESYSTEM_HEADER();

public:
    View3DInventor(Gui::Document* pcDocument, QWidget* parent, Qt::WFlags wflags=0);
    ~View3DInventor();

    /// Mesage handler
    virtual bool onMsg(const char* pMsg, const char** ppReturn);
    virtual bool onHasMsg(const char* pMsg) const;
    virtual void deleteSelf();
    /// Observer message from the ParameterGrp
    virtual void OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason);
    /// get called when the document is updated
    virtual void onRename(Gui::Document *pDoc);
    virtual void onUpdate(void);
    virtual void viewAll();
    virtual const char *getName(void) const;

    /// print function of the view
    virtual void print();
    virtual void printPdf();
    virtual void printPreview();
    virtual void print(QPrinter*);

    virtual PyObject *getPyObject(void);
    /**
     * If \a b is set to \a FullScreen the MDI view is displayed in full screen mode, if \a b
     * is set to \a TopLevel then it is displayed as an own top-level window, otherwise (\a Normal)
     * as tabbed window. 
     * This method is reimplemented from MDIView to set the this widget as the proxy of the embedded
     * GL widget to get all key events in \a TopLevel or \a Fullscreen mode.
     */
    void setCurrentViewMode(ViewMode b);
    bool setCamera(const char* pCamera);
    void toggleClippingPlane();
    bool hasClippingPlane() const;

    void setOverlayWidget(QWidget*);
    void removeOverlayWidget();

    View3DInventorViewer *getViewer(void) const {return _viewer;}
  
public Q_SLOTS:
    /// override the cursor in this view
    void setOverrideCursor(const QCursor&);
    void restoreOverrideCursor();

    void dump(const char* filename);

protected Q_SLOTS:
    void stopAnimating();

public:
    bool eventFilter(QObject*, QEvent* );

protected:
    void windowStateChanged(MDIView* );
    void dropEvent        (QDropEvent      * e);
    void dragEnterEvent   (QDragEnterEvent * e);
    void keyPressEvent    (QKeyEvent       * e);
    void keyReleaseEvent  (QKeyEvent       * e);
    void focusInEvent     (QFocusEvent     * e);
    void customEvent      (QEvent          * e);
    void contextMenuEvent (QContextMenuEvent*e);
    void previewFromFramebuffer(const QRect&, QImage&);

    /// handle to the viewer parameter group
    ParameterGrp::handle hGrp;

private:
    View3DInventorViewer * _viewer;
    PyObject *_viewerPy;
    QTimer * stopSpinTimer;
    QStackedWidget* stack;

    // friends
    friend class View3DPy;
};

} // namespace Gui

#endif  // GUI_VIEW3DINVENTOR_H

