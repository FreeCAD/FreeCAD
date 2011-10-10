/***************************************************************
 * Name:      wxWidgetsApp.h
 * Purpose:   Defines Application Class
 * Author:    Werner Mayer ()
 * Created:   2011-03-12
 * Copyright: Werner Mayer ()
 * License:   LGPL
 **************************************************************/

#ifndef WXWIDGETSAPP_H
#define WXWIDGETSAPP_H

#include <wx/app.h>

class wxWidgetsApp : public wxApp
{
    public:
        virtual bool OnInit();
};

#endif // WXWIDGETSAPP_H
