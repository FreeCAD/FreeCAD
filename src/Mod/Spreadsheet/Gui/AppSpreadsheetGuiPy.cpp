/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen (eivind@kvedalen.name)             *
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
# include <QIcon>
# include <QImage>
# include <QFileInfo>
#endif

#include "SpreadsheetView.h"
#include <Mod/Spreadsheet/App/Sheet.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <App/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>

using namespace SpreadsheetGui;
using namespace Spreadsheet;

/* module functions */
static PyObject * 
open(PyObject *self, PyObject *args) 
{
    const char* Name;
    const char* DocName=0;
    if (!PyArg_ParseTuple(args, "s|s",&Name,&DocName))
        return NULL; 
    
    PY_TRY {
        Base::FileInfo file(Name);
        App::Document *pcDoc = App::GetApplication().newDocument(DocName ? DocName : QT_TR_NOOP("Unnamed"));
        Sheet *pcSheet = (Sheet *)pcDoc->addObject("Spreadsheet::Sheet", file.fileNamePure().c_str());

        pcSheet->importFromFile(Name, '\t', '"', '\\');
        pcSheet->execute();
    } PY_CATCH;

    Py_Return; 
}

/* registration table  */
struct PyMethodDef SpreadsheetGui_Import_methods[] = {
    {"open"       ,open ,       1}, /* method name, C func ptr, always-tuple */
    {NULL, NULL}                   /* end of table marker */
};
