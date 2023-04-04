/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_SPLITVIEW3DINVENTOR_H
#define GUI_SPLITVIEW3DINVENTOR_H

#include "MDIView.h"
#include "MDIViewPy.h"

#include <Base/Parameter.h>
#include <vector>


namespace Gui {
class View3DInventorViewer;
class AbstractSplitViewPy;
class View3DSettings;

/** The SplitView3DInventor class allows to create a window with two or more Inventor views.
 *  \author Werner Mayer
 */
class GuiExport AbstractSplitView : public MDIView
{
    Q_OBJECT

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    AbstractSplitView(Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags=Qt::WindowFlags());
    ~AbstractSplitView() override;

    const char *getName() const override;

    /// Message handler
    bool onMsg(const char* pMsg, const char** ppReturn) override;
    bool onHasMsg(const char* pMsg) const override;
    void onUpdate() override;
    void deleteSelf() override;
    void viewAll() override;

    View3DInventorViewer *getViewer(unsigned int) const;
    void setOverrideCursor(const QCursor&) override;
    bool containsViewProvider(const ViewProvider*) const override;

    PyObject *getPyObject() override;
    void setPyObject(PyObject *) override;
    int getSize();

protected:
    void setDocumentOfViewers(Gui::Document* document);
    void setupSettings();

protected:
    std::vector<View3DInventorViewer*> _viewer;
    PyObject *_viewerPy;
    std::unique_ptr<View3DSettings> viewSettings;
};

class AbstractSplitViewPy : public Py::PythonExtension<AbstractSplitViewPy>
{
public:
    using BaseType = Py::PythonExtension<AbstractSplitViewPy>;
    static void init_type();    // announce properties and methods

    AbstractSplitViewPy(AbstractSplitView *vi);
    ~AbstractSplitViewPy() override;
    AbstractSplitView* getSplitViewPtr();

    Py::Object repr() override;
    Py::Object getattr(const char *) override;
    Py::Object cast_to_base(const Py::Tuple&);

    Py::Object fitAll(const Py::Tuple&);
    Py::Object viewBottom(const Py::Tuple&);
    Py::Object viewFront(const Py::Tuple&);
    Py::Object viewLeft(const Py::Tuple&);
    Py::Object viewRear(const Py::Tuple&);
    Py::Object viewRight(const Py::Tuple&);
    Py::Object viewTop(const Py::Tuple&);
    Py::Object viewIsometric(const Py::Tuple&);
    Py::Object getViewer(const Py::Tuple&);
    Py::Object sequence_item(Py_ssize_t) override;
    Py::Object close(const Py::Tuple&);
    PyCxx_ssize_t sequence_length() override;

private:
    Gui::MDIViewPy base;
};

/** The SplitView3DInventor class allows to create a window with two or more Inventor views.
 *  \author Werner Mayer
 */
class GuiExport SplitView3DInventor : public AbstractSplitView
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    SplitView3DInventor(int views, Gui::Document* pcDocument, QWidget* parent, Qt::WindowFlags wflags=Qt::WindowFlags());
    ~SplitView3DInventor() override;
};

} // namespace Gui

#endif  //GUI_SPLITVIEW3DINVENTOR_H

