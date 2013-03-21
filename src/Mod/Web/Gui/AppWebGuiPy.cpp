/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Python.h>
# include <QMdiArea>
# include <QMdiSubWindow>
# include <QUrl>
#endif

#include "BrowserView.h"
#include <Gui/Application.h>
#include <Gui/MainWindow.h>

/* module functions */
static PyObject * 
openBrowser(PyObject *self, PyObject *args) 
{
    const char* Url;
    if (! PyArg_ParseTuple(args, "s",&Url))
        return NULL; 
    
    PY_TRY {

        WebGui::BrowserView* pcBrowserView;

        pcBrowserView = new WebGui::BrowserView(Gui::getMainWindow());   
        pcBrowserView->setWindowTitle(QObject::tr("Browser"));
        pcBrowserView->resize(400, 300);
        pcBrowserView->load(Url);
        Gui::getMainWindow()->addWindow(pcBrowserView);

    } PY_CATCH;

    Py_Return; 
}

static PyObject * 
openBrowserHTML(PyObject *self, PyObject *args) 
{
    const char* HtmlCode;
    const char* BaseUrl;
    const char* TabName = "Browser";
    if (! PyArg_ParseTuple(args, "ss|s",&HtmlCode,&BaseUrl,&TabName))
        return NULL; 
    
    PY_TRY {
        QMdiSubWindow* browserView = 0;
        QMdiArea* mdiArea = Gui::getMainWindow()->findChild<QMdiArea*>();
        QList<QMdiSubWindow *> mdiViews = mdiArea->subWindowList();
        for (QList<QMdiSubWindow *>::iterator it = mdiViews.begin(); it != mdiViews.end(); ++it) {
            if (qobject_cast<WebGui::BrowserView*>((*it)->widget())) {
                browserView = *it;
                break;
            }
        }

        if (!browserView) {
            WebGui::BrowserView* pcBrowserView = 0;
            pcBrowserView = new WebGui::BrowserView(Gui::getMainWindow());
            pcBrowserView->resize(400, 300);
            pcBrowserView->setHtml(QString::fromUtf8(HtmlCode),QUrl(QString::fromAscii(BaseUrl)),QString::fromUtf8(TabName));
            Gui::getMainWindow()->addWindow(pcBrowserView);
        }
        else {
            mdiArea->setActiveSubWindow(browserView);
        }
    } PY_CATCH;

    Py_Return; 
}

/* registration table  */
struct PyMethodDef WebGui_Import_methods[] = {
    {"openBrowser"       ,openBrowser     ,  1},				/* method name, C func ptr, always-tuple */
    {"openBrowserHTML"   ,openBrowserHTML ,  1},				/* method name, C func ptr, always-tuple */
    {NULL, NULL}                   /* end of table marker */
};
