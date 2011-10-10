/***************************************************************************
                          imageconv.h  -  description
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

#ifndef IMAGECONV_H
#define IMAGECONV_H

// includes
#include "CmdLine.h"
#include <string>
#include <map>
#include <string>
#include <vector>

#include <QPixmap>
#include <QDir>

// defines
#define TString std::string
#define TVector std::vector
#define TMap    std::map
#define TPair   std::pair


// the command line parser class
class CCmdLineParser : public CCmdLine
{
public:
    CCmdLineParser (int argc, char** argv);
    ~CCmdLineParser () {}

    CCmdParam  GetArgumentList(const char* pSwitch);
};

// ------------------------------------------------------------

class CICException
{
public:
    CICException(const QString& text) 
      : msg(text) {}
    CICException(const CICException& e) 
    { *this = e; }
    ~CICException()
    { }
    QString what() const
    { return msg; }

private:
    QString msg;
};

// ------------------------------------------------------------

class CImageConvApp
{
public:
    CImageConvApp(const QString& sFile = "Images.cpp");
    void SetOutputFile(const QString& sFile);
    void SetNameFilters(const QStringList& nameFilter);
    bool Save(const QString& fn);
    bool Load(const QString& fn);
    bool ConvertToXPM(bool bAppendToFile = false);
    bool AppendToFile(const QString& file);
    void SetUpdateBmpFactory(bool b)
    { m_bUpdate = b; }
    void CreateBmpFactory();

    const QPixmap& GetPixmap() const;

    static void Usage();
    static void Error();
    static void Version();

private:
    bool m_bUpdate;
    static QString  m_Executable;
    static QString  m_BmpFactory;
    QPixmap  m_clPixmap;      // pixmap
    QString  m_Output;        // specified output file
    QDir     m_Dir;           // directory
};

#endif // IMAGECONV_H
