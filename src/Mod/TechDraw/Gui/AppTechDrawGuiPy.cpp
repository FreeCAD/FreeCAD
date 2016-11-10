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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QFileInfo>
# include <QIcon>
# include <QImage>
# include <sstream>
#endif

#include "MDIViewPage.h"
#include <Mod/TechDraw/App/DrawPage.h>
#include <Mod/TechDraw/App/DrawViewPart.h>
#include <Mod/Part/App/PartFeature.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Stream.h>
#include <App/Application.h>
#include <App/DocumentObjectPy.h>
#include <Gui/MainWindow.h>
#include <Gui/BitmapFactory.h>

using namespace TechDrawGui;

//TODO: TechDraw does not open/import/export SVG/DXF files like Drawing.  Not sure what belongs here.
//equivalents to GuiCommand for MDIViewPage::saveSVG?, print, printPDF or just make MDIViewPage methods Py accessible?
//
/* module functions */
static PyObject *
tdGuiPlaceholder(PyObject * /*self*/, PyObject *args)
{
    char* Name;
    if (!PyArg_ParseTuple(args, "et","utf-8",&Name))
        return NULL;
    std::string EncodedName = std::string(Name);
    PyMem_Free(Name);

    PY_TRY {
    } PY_CATCH;

    Py_Return;
}

/* registration table  */
struct PyMethodDef TechDrawGui_Import_methods[] = {
    {"tdGuiPlaceholder"     ,tdGuiPlaceholder ,     METH_VARARGS, ""}, /* method name, C func ptr, always-tuple */
    {NULL, NULL, 0, NULL}                    /* end of table marker */
};
