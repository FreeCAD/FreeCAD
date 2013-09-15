/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/fields/SoSFImage.h>
# include <QBuffer>
# include <QDateTime>
# include <QFile>
# include <QImageWriter>
#endif

//gcc
# include <iomanip>
# include <ios>
# include <sstream>

#include <Base/FileInfo.h>
#include <Base/Exception.h>
#include <Base/Console.h>
#include <App/Application.h>

#include "SoFCOffscreenRenderer.h"
#include "BitmapFactory.h"

using namespace Gui;
using namespace std;


void writeJPEGComment(const std::string&, QByteArray&);

// ---------------------------------------------------------------

SoFCOffscreenRenderer* SoFCOffscreenRenderer::inst = 0;


SoFCOffscreenRenderer& SoFCOffscreenRenderer::instance()
{
    if (inst==0)
        inst = new SoFCOffscreenRenderer(SbViewportRegion());
    return *inst;
}

SoFCOffscreenRenderer::SoFCOffscreenRenderer (const SbViewportRegion &viewportregion)
  : SoOffscreenRenderer(viewportregion)
{
}

SoFCOffscreenRenderer::SoFCOffscreenRenderer (SoGLRenderAction *action)
  : SoOffscreenRenderer(action)
{
}

SoFCOffscreenRenderer::~SoFCOffscreenRenderer()
{
}

void SoFCOffscreenRenderer::writeToImage (QImage& img) const
{
    const unsigned char * bytes = getBuffer();
    SbVec2s size = getViewportRegion().getViewportSizePixels();
    int numcomponents = (int) this->getComponents();

    SoSFImage image;
    image.setValue(size, numcomponents, bytes, SoSFImage::NO_COPY);
    BitmapFactory().convert(image, img);
}

void SoFCOffscreenRenderer::writeToImageFile(const char* filename, const char* comment, const SbMatrix& mat, const QImage& image)
{
    Base::FileInfo file(filename);
    if (file.hasExtension("JPG") || file.hasExtension("JPEG")) {
        // writing comment in case of jpeg (Qt ignores setText() in case of jpeg)
        std::string com;
        if (strcmp(comment,"")==0)
            com = "Screenshot created by FreeCAD";
        else if (strcmp(comment,"$MIBA")==0)
            com = createMIBA(mat);
        else
            com = comment;

        // write into memory
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        image.save(&buffer, "JPG");
        writeJPEGComment(com, ba);

        QFile file(QString::fromUtf8(filename));
        if (file.open(QFile::WriteOnly)) {
            file.write(ba);
            file.close();
        }
        else {
            std::stringstream str;
            str << "Cannot open file '" << filename << "' for writing.";
            throw Base::Exception(str.str());
        }
    }
    else {
        // check for all QImage formats
        bool supported = false;
        QByteArray format;
        QList<QByteArray> qtformats = QImageWriter::supportedImageFormats();
        for (QList<QByteArray>::Iterator it = qtformats.begin(); it != qtformats.end(); ++it) {
            if (file.hasExtension((*it).data())) {
                format = *it;
                supported = true;
                break;
            }
        }

        // Supported by Qt
        if (supported) {
            QImage img = image;
            // set keywords for PNG format
            if (file.hasExtension("PNG")) {
                img.setText(QLatin1String("Title"), QString::fromUtf8(filename));
                img.setText(QLatin1String("Author"), QLatin1String("FreeCAD (http://www.freecadweb.org)"));
                if (strcmp(comment,"")==0)
                    img.setText(QLatin1String("Description"), QLatin1String("Screenshot created by FreeCAD"));
                else if (strcmp(comment,"$MIBA")==0)
                    img.setText(QLatin1String("Description"), QLatin1String(createMIBA(mat).c_str()));
                else 
                    img.setText(QLatin1String("Description"), QString::fromUtf8(comment));
                img.setText(QLatin1String("Creation Time"), QDateTime::currentDateTime().toString());
                img.setText(QLatin1String("Software"), 
                    QString::fromUtf8(App::GetApplication().getExecutableName()));
            }

            QFile f(QString::fromUtf8(filename));
            if (f.open(QFile::WriteOnly)) {
                if (img.save(&f, format.data())) {
                    f.close();
                }
                else {
                    f.close();
                    std::stringstream str;
                    str << "Cannot save image to file '" << filename << "'.";
                    throw Base::Exception(str.str());
                }
            }
            else {
                std::stringstream str;
                str << "Cannot open file '" << filename << "' for writing.";
                throw Base::Exception(str.str());
            }
        }
        //
        // Use internal buffer instead of QImage
        //
        else if (isWriteSupported(file.extension().c_str())) {
            // Any format which is supported by Coin only
            if (!writeToFile(filename, file.extension().c_str()))
                throw Base::FileException("Error writing image file", filename);
        }
        else if (file.hasExtension("EPS") || file.hasExtension("PS")) {
            // Any format which is supported by Coin only
#ifdef FC_OS_WIN32
            FILE* fd = _wfopen(file.toStdWString().c_str(), L"w");
#else
            FILE* fd = fopen(filename, "w");
#endif
            bool ok = writeToPostScript(fd);
            fclose(fd);
            if (!ok)
                throw Base::FileException("Error writing image file", filename);
        }
        else if (file.hasExtension("RGB") || file.hasExtension("SGI")) {
            // Any format which is supported by Coin only
#ifdef FC_OS_WIN32
            FILE* fd = _wfopen(file.toStdWString().c_str(), L"w");
#else
            FILE* fd = fopen(filename, "w");
#endif
            bool ok = writeToRGB(fd);
            fclose(fd);
            if (!ok)
                throw Base::FileException("Error writing image file", filename);
        }
    }
}

QStringList SoFCOffscreenRenderer::getWriteImageFiletypeInfo()
{
    QStringList formats;

    // get all supported formats by Coin3D
    int num = getNumWriteFiletypes();
    for (int i=0; i < num; i++) {
#if   (COIN_MAJOR_VERSION < 2) // Coin3D <= 1.x
        SbList<SbName> extlist;
#elif (COIN_MAJOR_VERSION < 3) // Coin3D <= 2.x
# if  (COIN_MINOR_VERSION < 3) // Coin3D <= 2.2.x
        SbList<SbName> extlist;
# else                         // Coin3D >= 2.3.x
        SbPList extlist;
# endif                        
#else                          // Coin3D >= 3.x
        SbPList extlist;
#endif

        SbString fullname, description;
        getWriteFiletypeInfo(i, extlist, fullname, description);

        for (int j=0; j < extlist.getLength(); j++) {
            QString ext = QLatin1String((const char*) extlist[j]);
            if (formats.indexOf(ext.toUpper()) == -1)
                formats << ext.toUpper();
        }
    }

    // add now all further QImage formats
    QList<QByteArray> qtformats = QImageWriter::supportedImageFormats();
    for (QList<QByteArray>::Iterator it = qtformats.begin(); it != qtformats.end(); ++it) {
        // not supported? then append
        if (!isWriteSupported((*it).data()) && formats.indexOf(QLatin1String(*it)) == -1)
            formats << QLatin1String(*it);
    }

    // now add PostScript and SGI RGB
    if (formats.indexOf(QLatin1String("EPS")) == -1)
        formats << QLatin1String("EPS");
    else if (formats.indexOf(QLatin1String("SGI")) == -1)
        formats << QLatin1String("SGI");

    formats.sort();

    return formats;
}

std::string SoFCOffscreenRenderer::createMIBA(const SbMatrix& mat) const
{
    std::stringstream com;
    const std::map<std::string, std::string>& cfg = App::Application::Config();
    std::map<std::string, std::string>::const_iterator it;
    it = cfg.find("BuildVersionMajor");
    std::string major = (it != cfg.end() ? it->second : "");
    it = cfg.find("BuildVersionMinor");
    std::string minor = (it != cfg.end() ? it->second : "");

    com << setw(7) << setfill(' ') << fixed;
    com << "<?xml version=\"1.0\" encoding=\"UTF-8\"?> \n" ;
    com << "<MIBA xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://juergen-riegel.net/Miba/Miba2.xsd\" Version=\"2\"> \n" ;
    com << " <View>\n"; 
    com << "  <Matrix \n"; 
    com << "     a11=\"" << mat[0][0] <<"\" a12=\"" << mat[1][0] <<"\" a13=\"" << mat[2][0] <<"\" a14=\"" << mat[3][0] << "\"\n";
    com << "     a21=\"" << mat[0][1] <<"\" a22=\"" << mat[1][1] <<"\" a23=\"" << mat[2][1] <<"\" a24=\"" << mat[3][1] << "\"\n";
    com << "     a31=\"" << mat[0][2] <<"\" a32=\"" << mat[1][2] <<"\" a33=\"" << mat[2][2] <<"\" a34=\"" << mat[3][2] << "\"\n";
    com << "     a41=\"" << mat[0][3] <<"\" a42=\"" << mat[1][3] <<"\" a43=\"" << mat[2][3] <<"\" a44=\"" << mat[3][3] << "\"\n";
    com << "   />\n" ; 
    com << " </View>\n" ; 
    com << " <Source>\n" ; 
    com << "  <Creator>Unknown</Creator>\n" ;  
    com << "  <CreationDate>" << QDateTime::currentDateTime().toString().toAscii().constData() << "</CreationDate>\n" ;  
    com << "  <CreatingSystem>" << App::GetApplication().getExecutableName() << " " << major << "." << minor << "</CreatingSystem>\n" ;
    com << "  <PartNumber>Unknown</PartNumber>\n";
    com << "  <Revision>1.0</Revision>\n";
    com << " </Source>\n" ;
    com << "</MIBA>\n" ;

    return com.str();
}

void writeJPEGComment(const std::string& comment, QByteArray& ba)
{
    const unsigned char M_SOF0  = 0xc0;
    const unsigned char M_SOF1  = 0xc1;
    const unsigned char M_SOF2  = 0xc2;
    const unsigned char M_SOF3  = 0xc3;
    const unsigned char M_SOF5  = 0xc5;
    const unsigned char M_SOF6  = 0xc6;
    const unsigned char M_SOF7  = 0xc7;
    const unsigned char M_SOF9  = 0xc9;
    const unsigned char M_SOF10 = 0xcA;
    const unsigned char M_SOF11 = 0xcb;
    const unsigned char M_SOF13 = 0xcd;
    const unsigned char M_SOF14 = 0xce;
    const unsigned char M_SOF15 = 0xcf;
    const unsigned char M_SOI   = 0xd8;
    const unsigned char M_EOI   = 0xd9;
    const unsigned char M_COM   = 0xfe;

    union Byte {
        char c; unsigned char u;
    };

    if (comment.empty() || ba.length() < 2)
        return;

    // first marker
    Byte a,b;
    a.c = ba[0];
    b.c = ba[1];
    if (a.u == 0xff && b.u == M_SOI) {
        int index = 2;
        int len = ba.length();
        while (index < len) {
            // next marker
            a.c = ba[index++];
            while (a.u != 0xff && index < len) {
                a.c = ba[index++];
            }
            do {
                b.c = ba[index++];
            } while (b.u == 0xff && index < len);
            switch (b.u) {
                case M_SOF0:
                case M_SOF1:
                case M_SOF2:
                case M_SOF3:
                case M_SOF5:
                case M_SOF6:
                case M_SOF7:
                case M_SOF9:
                case M_SOF10:
                case M_SOF11:
                case M_SOF13:
                case M_SOF14:
                case M_SOF15:
                case M_EOI:
                    {
                        Byte a, b;
                        a.u = 0xff;
                        b.u = M_COM;
                        index -= 2; // insert comment before marker
                        ba.insert(index++, a.c);
                        ba.insert(index++, b.c);
                        int val = comment.size() + 2;
                        ba.insert(index++,(val >> 8) & 0xff);
                        ba.insert(index++,val & 0xff);
                        ba.insert(index, comment.c_str());
                        index = len; // finished
                    }   break;
                case M_COM:
                default:
                    {
                        Byte a, b;
                        a.c = ba[index++];
                        b.c = ba[index++];
                        int off = ((unsigned int)a.u << 8) + (unsigned int)b.u;
                        index += off;
                        index -= 2; // next marker
                    }   break;
            }
        }
    }
}
