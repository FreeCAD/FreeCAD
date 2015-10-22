/***************************************************************
 * Name:      wxWidgetsMain.cpp
 * Purpose:   Code for Application Frame
 * Author:    Werner Mayer ()
 * Created:   2011-03-12
 * Copyright: Werner Mayer ()
 * License:   LGPL
 **************************************************************/

#ifdef WX_PRECOMP
#include "wx_pch.h"
#endif

#ifdef __BORLANDC__
#pragma hdrstop
#endif //__BORLANDC__

#include "wxWidgetsMain.h"
#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#include <Python.h>
#include <string>
#include <sstream>
#include <vector>
#ifdef __WXGTK__
#   include "wx/gtk/win_gtk.h"
#endif

//helper functions
enum wxbuildinfoformat {
    short_f, long_f };

wxString wxbuildinfo(wxbuildinfoformat format)
{
    wxString wxbuild(wxVERSION_STRING);

    if (format == long_f )
    {
#if defined(__WXMSW__)
        wxbuild << _T("-Windows");
#elif defined(__WXMAC__)
        wxbuild << _T("-Mac");
#elif defined(__UNIX__)
        wxbuild << _T("-Linux");
#endif

#if wxUSE_UNICODE
        wxbuild << _T("-Unicode build");
#else
        wxbuild << _T("-ANSI build");
#endif // wxUSE_UNICODE
    }

    return wxbuild;
}

BEGIN_EVENT_TABLE(wxWidgetsFrame, wxFrame)
    EVT_CLOSE(wxWidgetsFrame::OnClose)
    EVT_MENU(idMenuQuit, wxWidgetsFrame::OnQuit)
    EVT_MENU(idMenuLoad, wxWidgetsFrame::OnLoad)
    EVT_MENU(idMenuNewDocument, wxWidgetsFrame::OnNewDocument)
    EVT_MENU(idMenuEmbed, wxWidgetsFrame::OnEmbed)
    EVT_MENU(idMenuAbout, wxWidgetsFrame::OnAbout)
END_EVENT_TABLE()

wxWidgetsFrame::wxWidgetsFrame(wxFrame *frame, const wxString& title)
    : wxFrame(frame, -1, title)
{
    if (!Py_IsInitialized()) {
        static std::vector<char*> argv;
        argv.push_back((char*)"wxwidgetsDialog");
        static int argc = 1;
        Py_SetProgramName(argv[0]);
        Py_Initialize();
        PySys_SetArgv(argc, &(argv[0]));
    }

#if wxUSE_MENUS
    // create a menu bar
    wxMenuBar* mbar = new wxMenuBar();
    wxMenu* fileMenu = new wxMenu(_T(""));
    fileMenu->Append(idMenuQuit, _("&Quit\tAlt-F4"), _("Quit the application"));
    mbar->Append(fileMenu, _("&File"));

    wxMenu* freecadMenu = new wxMenu(_T(""));
    freecadMenu->Append(idMenuLoad, _("Load"), _("Load FreeCAD module"));
    freecadMenu->Append(idMenuNewDocument, _("New document"), _("Create new document in the FreeCAD module"));
    freecadMenu->Append(idMenuEmbed, _("Embed"), _("Embed FreeCAD as child window"));
    mbar->Append(freecadMenu, _("FreeCAD"));
    freecadMenu->Enable(idMenuLoad, true);
    freecadMenu->Enable(idMenuNewDocument, false);
    freecadMenu->Enable(idMenuEmbed, false);

    wxMenu* helpMenu = new wxMenu(_T(""));
    helpMenu->Append(idMenuAbout, _("&About\tF1"), _("Show info about this application"));
    mbar->Append(helpMenu, _("&Help"));

    SetMenuBar(mbar);
#endif // wxUSE_MENUS

#if wxUSE_STATUSBAR
    // create a status bar with some information about the used wxWidgets version
    CreateStatusBar(2);
    SetStatusText(_("Hello Code::Blocks user!"),0);
    SetStatusText(wxbuildinfo(short_f), 1);
#endif // wxUSE_STATUSBAR

}


wxWidgetsFrame::~wxWidgetsFrame()
{
}

void wxWidgetsFrame::OnClose(wxCloseEvent &event)
{
    Destroy();
}

void wxWidgetsFrame::OnQuit(wxCommandEvent &event)
{
    Destroy();
}

void wxWidgetsFrame::OnLoad(wxCommandEvent &event)
{
    const wxString& dir = wxDirSelector("FreeCAD module path");
    std::string path = dir.ToAscii();
    if (!path.empty()) {
        for (std::string::iterator it = path.begin(); it != path.end(); ++it) {
            if (*it == '\\')
                *it = '/';
        }
        PyObject* main = PyImport_AddModule("__main__");
        PyObject* dict = PyModule_GetDict(main);
        std::stringstream cmd;
        cmd << "import sys,os\n"
            << "sys.path.append(\"" << path << "\")\n"
            << "os.chdir(\"" << path << "\")\n"
            << "import FreeCADGui\n"
            << "FreeCADGui.showMainWindow()\n";

        PyObject* result = PyRun_String(cmd.str().c_str(), Py_file_input, dict, dict);
        if (result) {
            Py_DECREF(result);
            wxMenuBar* mbar = GetMenuBar();
            wxMenu* menu = mbar->GetMenu(1);
            menu->Enable(idMenuLoad, false);
            menu->Enable(idMenuNewDocument, true);
            menu->Enable(idMenuEmbed, true);
        }
        else {
            PyObject *ptype, *pvalue, *ptrace;
            PyErr_Fetch(&ptype, &pvalue, &ptrace);
            PyObject* pystring = PyObject_Str(pvalue);
            const char* error = PyString_AsString(pystring);
            wxString msg = wxString::FromAscii(error);
            wxMessageBox(msg, _("Error"));
            Py_DECREF(pystring);
        }
        Py_DECREF(dict);
    }
}

void wxWidgetsFrame::OnNewDocument(wxCommandEvent &event)
{
    PyObject* main = PyImport_AddModule("__main__");
    PyObject* dict = PyModule_GetDict(main);
    const char* cmd =
        "FreeCAD.newDocument()\n";

    PyObject* result = PyRun_String(cmd, Py_file_input, dict, dict);
    if (result) {
        Py_DECREF(result);
    }
    else {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch(&ptype, &pvalue, &ptrace);
        PyObject* pystring = PyObject_Str(pvalue);
        const char* error = PyString_AsString(pystring);
        wxString msg = wxString::FromAscii(error);
        wxMessageBox(msg, _("Error"));
        Py_DECREF(pystring);
    }
    Py_DECREF(dict);
}

void wxWidgetsFrame::OnEmbed(wxCommandEvent &event)
{
    void* hwnd = 0;
#if defined (_WIN32)
    hwnd = this->GetHWND();
#elif defined(__WXGTK__)
    //http://old.nabble.com/wxWindow-and-x11-td2853615.html
    //GdkWindow *window = GTK_PIZZA()->bin_window;
    //GDK_WINDOW_XWINDOW( window );
    //hwnd = window;
#endif
    PyObject* main = PyImport_AddModule("__main__");
    PyObject* dict = PyModule_GetDict(main);
    std::stringstream cmd;
    cmd << "class BlankWorkbench (Workbench):\n"
        << "   MenuText = \"Blank\"\n"
        << "   ToolTip = \"Blank workbench\"\n"
        << "\n"
        << "   def Initialize(self):\n"
        << "      return\n"
        << "   def GetClassName(self):\n"
        << "      return \"Gui::BlankWorkbench\"\n"
        << "\n"
        << "FreeCADGui.addWorkbench(BlankWorkbench)\n"
        << "FreeCADGui.activateWorkbench(\"BlankWorkbench\")\n"
        << "FreeCADGui.embedToWindow(\"" << hwnd << "\")\n"
        << "\n";
    PyObject* result = PyRun_String(cmd.str().c_str(), Py_file_input, dict, dict);
    if (result) {
        Py_DECREF(result);
        wxMenuBar* mbar = GetMenuBar();
        wxMenu* menu = mbar->GetMenu(1);
        menu->Enable(idMenuEmbed, true);
    }
    else {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch(&ptype, &pvalue, &ptrace);
        PyObject* pystring = PyObject_Str(pvalue);
        const char* error = PyString_AsString(pystring);
        wxString msg = wxString::FromAscii(error);
        wxMessageBox(msg, _("Error"));
        Py_DECREF(pystring);
    }
    Py_DECREF(dict);
}

void wxWidgetsFrame::OnAbout(wxCommandEvent &event)
{
    wxString msg = wxbuildinfo(long_f);
    wxMessageBox(msg, _("Welcome to..."));
}
