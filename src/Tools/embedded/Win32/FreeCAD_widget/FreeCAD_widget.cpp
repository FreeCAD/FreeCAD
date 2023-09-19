// FreeCAD_widget.cpp : Defines the entry point for the application.
//

#include "FreeCAD_widget.h"
#include "stdafx.h"
#include <Commdlg.h>
#include <Shlobj.h>
#include <sstream>
#include <string>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                      // current instance
TCHAR szTitle[MAX_LOADSTRING];        // The title bar text
TCHAR szWindowClass[MAX_LOADSTRING];  // the main window class name

// Forward declarations of functions included in this code module:
ATOM MyRegisterClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK About(HWND, UINT, WPARAM, LPARAM);
std::string OnFileOpen(HWND, UINT, WPARAM, LPARAM);
void OnLoadFreeCAD(HWND, UINT, WPARAM, LPARAM);
void OnNewDocument(HWND);
void OnEmbedWidget(HWND hWnd);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.
    MSG msg;
    HACCEL hAccelTable;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_FREECAD_WIDGET, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance(hInstance, nCmdShow)) {
        return FALSE;
    }

    hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_FREECAD_WIDGET));

    // Main message loop:
    while (GetMessage(&msg, NULL, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int)msg.wParam;
}


//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_FREECAD_WIDGET));
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDC_FREECAD_WIDGET);
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    HWND hWnd;

    hInst = hInstance;  // Store instance handle in our global variable

    hWnd = CreateWindow(szWindowClass,
                        szTitle,
                        WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT,
                        0,
                        CW_USEDEFAULT,
                        0,
                        NULL,
                        NULL,
                        hInstance,
                        NULL);

    if (!hWnd) {
        return FALSE;
    }

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND	- process the application menu
//  WM_PAINT	- Paint the main window
//  WM_DESTROY	- post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    int wmId, wmEvent;
    PAINTSTRUCT ps;
    HDC hdc;

    switch (message) {
        case WM_COMMAND:
            wmId = LOWORD(wParam);
            wmEvent = HIWORD(wParam);
            // Parse the menu selections:
            switch (wmId) {
                case IDM_ABOUT:
                    DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                    break;
                case IDM_EXIT:
                    DestroyWindow(hWnd);
                    break;
                case ID_FREECAD_LOAD:
                    OnLoadFreeCAD(hWnd, message, wParam, lParam);
                    break;
                case ID_FREECAD_NEWDOCUMENT:
                    OnNewDocument(hWnd);
                    break;
                case ID_FREECAD_EMBEDWINDOW:
                    OnEmbedWidget(hWnd);
                    break;
                default:
                    return DefWindowProc(hWnd, message, wParam, lParam);
            }
            break;
        case WM_PAINT:
            hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code here...
            EndPaint(hWnd, &ps);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message) {
        case WM_INITDIALOG:
            return (INT_PTR)TRUE;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
            break;
    }
    return (INT_PTR)FALSE;
}

#include <Python.h>

// See also https://www.freecad.org/wiki/Embedding_FreeCAD

std::string OnFileOpen(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    std::string path;
    BROWSEINFO bi;
    memset(&bi, 0, sizeof(BROWSEINFO));
    bi.hwndOwner = hWnd;
    bi.lpszTitle = "Select FreeCAD module directory";
    bi.ulFlags = BIF_RETURNONLYFSDIRS;

    ITEMIDLIST* pList = SHBrowseForFolder(&bi);
    if (pList) {
        char szFolder[MAX_PATH + 1];
        SHGetPathFromIDList(pList, szFolder);
        path = szFolder;
        LPMALLOC pMalloc;
        if (S_OK == SHGetMalloc(&pMalloc)) {
            pMalloc->Free(pList);
        }
    }

    return path;
}

void OnLoadFreeCAD(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (!Py_IsInitialized()) {
        Py_SetProgramName(L"CEmbed_FreeCADDlg");
        Py_Initialize();
        static int argc = 1;
        static wchar_t* app = L"CEmbed_FreeCADDlg";
        static wchar_t* argv[2] = {app, 0};
        PySys_SetArgv(argc, argv);
    }

    std::string path = OnFileOpen(hWnd, message, wParam, lParam);
    if (!path.empty()) {
        for (std::string::iterator it = path.begin(); it != path.end(); ++it) {
            if (*it == '\\') {
                *it = '/';
            }
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
            HMENU hMenu = GetMenu(hWnd);
            EnableMenuItem(hMenu, ID_FREECAD_LOAD, MF_DISABLED);
            EnableMenuItem(hMenu, ID_FREECAD_NEWDOCUMENT, MF_ENABLED);
            EnableMenuItem(hMenu, ID_FREECAD_EMBEDWINDOW, MF_ENABLED);
        }
        else {
            PyObject *ptype, *pvalue, *ptrace;
            PyErr_Fetch(&ptype, &pvalue, &ptrace);
            PyObject* pystring = PyObject_Str(pvalue);
            const char* error = PyUnicode_AsUTF8(pystring);
            MessageBox(0, error, "Error", MB_OK);
            Py_DECREF(pystring);
        }
        Py_DECREF(dict);
    }
}

void OnNewDocument(HWND hWnd)
{
    PyObject* main = PyImport_AddModule("__main__");
    PyObject* dict = PyModule_GetDict(main);
    const char* cmd = "FreeCAD.newDocument()\n";

    PyObject* result = PyRun_String(cmd, Py_file_input, dict, dict);
    if (result) {
        Py_DECREF(result);
    }
    else {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch(&ptype, &pvalue, &ptrace);
        PyObject* pystring = PyObject_Str(pvalue);
        const char* error = PyUnicode_AsUTF8(pystring);
        MessageBox(hWnd, error, "Error", MB_OK);
        Py_DECREF(pystring);
    }
    Py_DECREF(dict);
}

void OnEmbedWidget(HWND hWnd)
{
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
        << "FreeCADGui.embedToWindow(\"" << (void*)hWnd << "\")\n"
        << "\n";
    PyObject* result = PyRun_String(cmd.str().c_str(), Py_file_input, dict, dict);
    if (result) {
        Py_DECREF(result);
        HMENU hMenu = GetMenu(hWnd);
        EnableMenuItem(hMenu, ID_FREECAD_EMBEDWINDOW, MF_DISABLED);
    }
    else {
        PyObject *ptype, *pvalue, *ptrace;
        PyErr_Fetch(&ptype, &pvalue, &ptrace);
        PyObject* pystring = PyObject_Str(pvalue);
        const char* error = PyUnicode_AsUTF8(pystring);
        MessageBox(hWnd, error, "Error", MB_OK);
        Py_DECREF(pystring);
    }
    Py_DECREF(dict);
}
