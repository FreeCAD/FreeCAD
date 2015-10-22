/***************************************************************************
                          imageconv.cpp  -  description
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

#include "imageconv.h"
#include <QStringList>
#include <QBuffer>
#include <QTextStream>
#include <QImage>
#include <QImageWriter>
#include <QImageReader>
#include <iostream>

using namespace std;

CCmdLineParser::CCmdLineParser(int argc, char** argv)
{
    SplitLine(argc, argv);
}

CCmdParam CCmdLineParser::GetArgumentList(const char* pSwitch)
{
    if (HasSwitch(pSwitch))
    {
        CCmdLineParser::iterator theIterator;

        theIterator = find(pSwitch);
        if (theIterator!=end())
        {
            return (*theIterator).second;
        }
    }

    CCmdParam param;
    return param;
}

// ------------------------------------------------------------

QString CImageConvApp::m_Executable = "ImageConv";
QString CImageConvApp::m_BmpFactory = "BmpFactoryIcons.cpp";

CImageConvApp::CImageConvApp(const QString& sFile)
{
    m_bUpdate = false;
    m_Output = sFile;
    QString filter = "*.png;*.bmp;*.xbm;*.pnm;*.jpg;*.jpeg;*.mng;*.gif"; // not "*.xpm"!
    m_Dir.setNameFilters(filter.split(';'));
}

void CImageConvApp::SetOutputFile(const QString& sFile) 
{
    m_Output = sFile;
}

void CImageConvApp::SetNameFilters(const QStringList& nameFilter)
{
    m_Dir.setNameFilters(nameFilter);
}

bool CImageConvApp::Save(const QString& fn)
{
    int iPos = fn.indexOf(".");

    QString ext  = fn.mid(iPos+1);  // extension of filename
    QString name = fn.mid(0,iPos);  // filename without extension

    if (!m_clPixmap.isNull())
    {
        if (!fn.isEmpty())
        {
            return m_clPixmap.save(fn, ext.toUpper().toAscii());
        }
    }

    return false;
}

bool CImageConvApp::Load(const QString& fn)
{
    QByteArray ext = QImageReader::imageFormat(fn);

    if (!fn.isEmpty())
        return m_clPixmap.load( fn, ext);

    return false;
}

const QPixmap& CImageConvApp::GetPixmap() const
{
    return m_clPixmap;
}

bool CImageConvApp::ConvertToXPM(bool bAppendToFile)
{
    QStringList list = m_Dir.entryList();

    // print to the console
    cout << "Try converting to XPM..." << endl;
    if (list.count() == 0)
    {
        cout << "Cannot find " << (const char*)m_Dir.nameFilters().join(" ").toAscii() << endl;
        return false;
    }

    for (QStringList::Iterator it = list.begin(); it != list.end(); ++it)
    {
        QByteArray ext = QImageReader::imageFormat(*it);
        if (ext.isEmpty())
            continue; // no image format

        if (m_Output == *it)
            continue; // if the file is the output file itself

        cout << "Converting " << (const char*)(*it).toAscii() << " ...";
    
        if (Load(*it) == true)
        {
            QString name(*it);
            name.replace(name.indexOf(".")+1, 4, "xpm");

            bool ok;

            QFileInfo fi(*it);
            if (bAppendToFile)
                ok = AppendToFile(fi.baseName());
            else
                ok = Save(name);

            if (ok)
                cout << "Done" << endl;
            else
                cout << "failed" << endl;
        }
        else
        {
            cout << "failed" << endl;
        }
    }

    return true;
}

void CImageConvApp::CreateBmpFactory()
{
    // empty file
    //
    QFileInfo fi(m_BmpFactory);

    // already exists
    if (fi.exists() && fi.isFile())
        return;

    QFile fw(m_BmpFactory);
    QTextStream tw (&fw);
    if (!fw.open(QIODevice::Text | QIODevice::Unbuffered | QIODevice::WriteOnly))
        return;

    // write header stuff
    tw << "\n";
    tw << "void RegisterIcons()\n";
    tw << "{\n";
    tw << "  Gui::BitmapFactoryInst& rclBmpFactory = Gui::BitmapFactory();\n";
    tw << "}\n";
    fw.close();
}

bool CImageConvApp::AppendToFile(const QString& file)
{
    CreateBmpFactory();

    QString ohead("static char");
    QString nhead("static const char");

    // save as XPM into tmp. buffer
    QByteArray str;
    QBuffer buf(&str);
    buf.open (QIODevice::WriteOnly);
    QImageWriter iio(&buf, "XPM");
    QImage im;
    im = m_clPixmap.toImage();
    iio.write(im);
    buf.close();

    // convert to string and make changes
    QString txt = str;
    txt.replace(ohead, nhead);
    txt.replace(QString("dummy"), file);

    // open file
    bool found = false;
    QFile fw(m_Output);
    if (fw.open(QIODevice::ReadOnly))
    {
        QTextStream tr (&fw);
        QString line;
        do 
        {
            line = tr.readLine();
            if ((line.indexOf(file)) != -1) // icon already registered
            {
                found = true;
            }
        } while (!tr.atEnd() && !found);  

        fw.close();
    }

    // register new icon
    if (!found)
    {
        if (!fw.open(QIODevice::Text | QIODevice::Unbuffered | QIODevice::ReadWrite | QIODevice::Append))
            return false;

        // write into file now
        QTextStream tw (&fw);
        tw << txt << "\n";
        fw.close();
  
        if (m_bUpdate)
        {
            QFile bmp(m_BmpFactory);
            QTextStream ts (&bmp);
            if (!bmp.open(QIODevice::Text | QIODevice::Unbuffered | QIODevice::WriteOnly))
                return false;

            bmp.seek(bmp.size()-3);
            ts << "  rclBmpFactory.addXPM(\"" << file << "\", " << file << ");\n";
            ts << "}\n";
            bmp.close();
        }
    }

    return true;
}

void CImageConvApp::Error()
{
    cerr << "Usage: " << (const char*)m_Executable.toAscii() << " [OPTION(S)] -i input file(s) {-o output file}" << endl;
    cerr << "Try '"   << (const char*)m_Executable.toAscii() << " --help' for more information." << endl;

    exit(0);
}

void CImageConvApp::Version()
{
    cerr << (const char*)m_Executable.toAscii() << " 1.0.0 " << endl;
    exit(0);
}

void CImageConvApp::Usage()
{
    cerr << "Usage: " << (const char*)m_Executable.toAscii() << " [OPTION(S)] -i input file(s) {-o output file}\n" << endl;
    cerr << "Options:" << endl;

    cerr << "  -i       \tSpecify the input file(s).\n"
            "           \tSeveral filenames must be separated by a blank.\n"
            "           \tIf you want to select all files of a format\n"
            "           \tyou also can write \"*.[FORMAT]\" (e.g. *.png).\n"  
            "           \tSpecifying several files only makes sense in\n" 
            "           \taddition with -a or -x." << endl;

    cerr << "  -o       \tSpecify the output file." << endl;

    cerr << "  -x, --xpm\tConvert all specified image files to XPM.\n"
            "           \tFor each specified image file a corresponding\n"
            "           \tXPM file will be created.\n"
            "           \tWith -i you can specify the input files." << endl;

    cerr << "  -a, --append\tConvert all specified image files to XPM and\n"
            "           \tappend the result to the file specified with -o.\n"
            "           \tWith -i you can specify the input files.\n" << endl;


    cerr << "  -u, --update\tUpdate the file \"BmpFactoryIcons.cpp\"\n"
            "           \tThis is a special mode to add icons to the FreeCAD's\n"
            "           \tbitmap factory automatically.\n"
            "           \tThis switch is only available in addition with -a.\n" << endl;

    cerr << "  -v, --version\tPrint the version and exit." << endl;

    cerr << "  -h, --help\tPrint this message and exit.\n" << endl;

    cerr << "This program supports the following image formats:\n"
            " BMP, GIF, JPEG, MNG, PNG, PNM, XBM and XPM\n\n"
         << (const char*)m_Executable.toAscii() << " uses Qt Version " << qVersion() << "\n"
            "Qt can be downloaded at http://www.trolltech.com." << endl;

    exit(0);
}
