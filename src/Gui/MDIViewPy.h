/***************************************************************************
 *   Copyright (c) 2019 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_MDIVIEWPY_H
#define GUI_MDIVIEWPY_H

#include <Base/PyObjectBase.h>
#include <CXX/Extensions.hxx>
#include <QPointer>
#include <FCGlobal.h>

namespace Gui {
class MDIView;

class GuiExport MDIViewPy : public Py::PythonExtension<MDIViewPy>
{
public:
    static void init_type();    // announce properties and methods
    static PyObject *extension_object_new( PyTypeObject *subtype, PyObject * /*args*/, PyObject * /*kwds*/ );

    static Py::Object type();
    static Py::ExtensionObject<MDIViewPy> create(MDIView *mdi);

    explicit MDIViewPy(MDIView *mdi);
    ~MDIViewPy() override;

    Py::Object repr() override;

    /** @name Printing */
    //@{
    Py::Object printView(const Py::Tuple&);
    Py::Object printPdf(const Py::Tuple&);
    Py::Object printPreview(const Py::Tuple&);
    //@}

    /** @name Undo/Redo actions */
    //@{
    Py::Object undoActions(const Py::Tuple&);
    Py::Object redoActions(const Py::Tuple&);
    //@}

    Py::Object sendMessage(const Py::Tuple&);
    Py::Object supportMessage(const Py::Tuple&);
    Py::Object fitAll(const Py::Tuple&);
    Py::Object setActiveObject(const Py::Tuple&);
    Py::Object getActiveObject(const Py::Tuple&);
    Py::Object cast_to_base(const Py::Tuple&);

    MDIView* getMDIViewPtr() {return _view.data();}

private:
    QPointer<MDIView> _view;
};

} // namespace Gui

#endif //GUI_MDIVIEWPY_H
