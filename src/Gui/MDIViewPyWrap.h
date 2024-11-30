/***************************************************************************
 *   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_MDIVIEWPYWRAP_H
#define GUI_MDIVIEWPYWRAP_H

#include <memory>
#include <Gui/MDIView.h>
#include <CXX/Objects.hxx>


namespace Gui
{

class MDIViewPyWrapImp;
class GuiExport MDIViewPyWrap : public MDIView
{
    Q_OBJECT

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    /** View constructor
     * Attach the view to the given document. If the document is zero
     * the view will attach to the active document. Be aware, there isn't
     * always an active document.
     */
    explicit MDIViewPyWrap(const Py::Object& py, Gui::Document* pcDocument, QWidget* parent=nullptr, Qt::WindowFlags wflags=Qt::WindowFlags());
    /** View destructor
     * Detach the view from the document, if attached.
     */
    ~MDIViewPyWrap() override;

    /// Message handler
    bool onMsg(const char* pMsg,const char** ppReturn) override;
    /// Message handler test
    bool onHasMsg(const char* pMsg) const override;
    /// overwrite when checking on close state
    bool canClose() override;
    PyObject *getPyObject() override;
    /** @name Printing */
    //@{
public Q_SLOTS:
    void print(QPrinter* printer) override;

public:
    /** Print content of view */
    void print() override;
    /** Print to PDF file */
    void printPdf() override;
    /** Show a preview dialog */
    void printPreview() override;
    //@}

    /** @name Undo/Redo actions */
    //@{
    QStringList undoActions() const override;
    QStringList redoActions() const override;
    //@}

private:
    std::unique_ptr<MDIViewPyWrapImp> ptr;
};

} // namespace Gui

#endif // GUI_MDIVIEWPYWRAP_H
