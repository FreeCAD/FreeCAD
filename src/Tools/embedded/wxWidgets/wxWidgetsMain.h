/***************************************************************
 * Name:      wxWidgetsMain.h
 * Purpose:   Defines Application Frame
 * Author:    Werner Mayer ()
 * Created:   2011-03-12
 * Copyright: Werner Mayer ()
 * License:   LGPL
 **************************************************************/

#ifndef WXWIDGETSMAIN_H
#define WXWIDGETSMAIN_H

#ifndef WX_PRECOMP
    #include <wx/wx.h>
#endif

#include "wxWidgetsApp.h"

class wxWidgetsFrame: public wxFrame
{
    public:
        wxWidgetsFrame(wxFrame *frame, const wxString& title);
        ~wxWidgetsFrame();
    private:
        enum
        {
            idMenuQuit = 1000,
            idMenuLoad,
            idMenuNewDocument,
            idMenuEmbed,
            idMenuAbout
        };
        void OnClose(wxCloseEvent& event);
        void OnQuit(wxCommandEvent& event);
        void OnLoad(wxCommandEvent& event);
        void OnNewDocument(wxCommandEvent& event);
        void OnEmbed(wxCommandEvent& event);
        void OnAbout(wxCommandEvent& event);
        DECLARE_EVENT_TABLE()
};


#endif // WXWIDGETSMAIN_H
