/***************************************************************************
                          main.cpp  -  description
                             -------------------
    begin                : Die Apr 23 21:02:14 CEST 2002
    copyright            : (C) 2002 by Werner Mayer
    email                : 
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *   Werner Mayer 2002                                                     *
 *                                                                         *
 ***************************************************************************/

#include <iostream>
#include <QApplication>
#include "imageconv.h"

using namespace std;

int main( int argc, char **argv )
{
    QApplication app( argc, argv );
    CImageConvApp cICApp;
    CCmdLineParser cCmdP(argc, argv);

    try
    {
        // look for the specified switches and arguments
        //
        //
        // show help message and exit
        if (cCmdP.HasSwitch("-h") || cCmdP.HasSwitch("--help"))
        {
            CImageConvApp::Usage();
        }
        // show version and exit
        else if (cCmdP.HasSwitch("-v") || cCmdP.HasSwitch("--version"))
        {
            CImageConvApp::Version();
        }

        // convert all given/found image files to XPM
        if (cCmdP.HasSwitch("-x") || cCmdP.HasSwitch("--xpm"))
        {
            // search for input files
            if (cCmdP.GetArgumentCount("-i") > 0)
            {
                QStringList nameFilters;
                CCmdParam para = cCmdP.GetArgumentList("-i");
                for (TVector<TString>::iterator it = para.m_strings.begin(); it != para.m_strings.end(); it++)
                {
                    cout << "Search for " << it->c_str() << endl;
                    nameFilters.append(it->c_str());
                }

                cICApp.SetNameFilters(nameFilters);
                cICApp.ConvertToXPM(false);
            }
            else
                throw CICException("No input file specified.");
        }

        // convert all given/found image files to XPM and write the result into a text file
        else if (cCmdP.HasSwitch("-a") || cCmdP.HasSwitch("--append"))
        {
            // search for input fíles
            if (cCmdP.GetArgumentCount("-i") > 0)
            {
                cICApp.SetUpdateBmpFactory(cCmdP.HasSwitch("-a") || cCmdP.HasSwitch("--update"));
                QStringList nameFilters;
                CCmdParam para = cCmdP.GetArgumentList("-i");
                for (TVector<TString>::iterator it = para.m_strings.begin(); it != para.m_strings.end(); it++)
                {
                    cout << "Search for " << it->c_str() << endl;
                    nameFilters.append(it->c_str());
                }

                cICApp.SetNameFilters(nameFilters);
            }
            else
                throw CICException("No input files specified.");

            // search for output file
            if (cCmdP.GetArgumentCount("-o") > 0)
            {
                cICApp.SetOutputFile(QString(cCmdP.GetArgument("-o", 0).c_str()));
                cICApp.ConvertToXPM(true);
            }
            else
                throw CICException("No output file specified.");
        }

        // convert one image file to another image file
        else if (cCmdP.HasSwitch("-i") && cCmdP.HasSwitch("-o"))
        {
            // input and output file specified
            CCmdParam p1 = cCmdP.GetArgumentList("-i");
            CCmdParam p2 = cCmdP.GetArgumentList("-o");

            if (p1.m_strings.size() > 1)
                throw CICException("Too much input files specified.");
            if (p1.m_strings.size() < 1)
                throw CICException("No input file specified.");
            if (p2.m_strings.size() > 1)
                throw CICException("Too much output files specified.");
            if (p2.m_strings.size() < 1)
                throw CICException("No output file specified.");

            TString fil1(*p1.m_strings.begin());
            TString fil2(*p2.m_strings.begin());

            if (cICApp.Load(QString(fil1.c_str())) == false)
            {
                cout << "Loading of " << fil1.c_str() << " failed!" << endl;
                cout << "Perhaps the file does not exist or QT does not support this format." << endl;
                return -1;
            }
            if (cICApp.Save(QString(fil2.c_str())) == false)
            {
                cout << "Saving of " << fil2.c_str() << " failed!" << endl;
                cout << "Perhaps QT does not support this format." << endl;
            }
            else
            {
                cout << "Converted successfully!" << endl;
            }
        }
        // no/wrong arguments 
        else
            throw CICException("Wrong arguments.");
    }
    catch(const CICException& e)
    {
        cerr << (const char*)e.what().toAscii() << endl;
        CImageConvApp::Error();
    }
    catch(...)
    {
        cerr << "An unknown exception has occured!!!" << endl;
    }

    return 0;
}
