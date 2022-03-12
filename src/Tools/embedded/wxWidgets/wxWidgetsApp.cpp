/***************************************************************
 * Name:      wxWidgetsApp.cpp
 * Purpose:   Code for Application Class
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

#include "wxWidgetsApp.h"
#include "wxWidgetsMain.h"

IMPLEMENT_APP(wxWidgetsApp);

bool wxWidgetsApp::OnInit()
{
    wxWidgetsFrame* frame = new wxWidgetsFrame(0L, _("wxWidgets Application Template"));

    frame->Show();

    return true;
}
