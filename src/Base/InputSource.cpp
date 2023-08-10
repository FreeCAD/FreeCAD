/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <qglobal.h>
#if QT_VERSION < 0x060000
#include <QTextCodec>
#else
#include <QByteArray>
#include <QStringDecoder>
#include <QStringEncoder>
#endif

#include "InputSource.h"
#include "XMLTools.h"


XERCES_CPP_NAMESPACE_USE

using namespace Base;
using namespace std;


// ---------------------------------------------------------------------------
//  StdInputStream: Constructors and Destructor
// ---------------------------------------------------------------------------

#if QT_VERSION < 0x060000
struct StdInputStream::TextCodec
{
    QTextCodec::ConverterState state;
    TextCodec() {
        state.flags |= QTextCodec::IgnoreHeader;
        state.flags |= QTextCodec::ConvertInvalidToNull;
    }

    void validateBytes(XMLByte* const  toFill, std::streamsize len) {
        QTextCodec *textCodec = QTextCodec::codecForName("UTF-8");
        if (textCodec) {
            const QString text = textCodec->toUnicode(reinterpret_cast<char *>(toFill), static_cast<int>(len), &state);
            if (state.invalidChars > 0) {
                // In case invalid characters were found decode back to 'utf-8' and replace
                // them with '?'
                // First, Qt replaces invalid characters with '\0' (see ConvertInvalidToNull)
                // but Xerces doesn't like this because it handles this as termination. Thus,
                // we have to go through the array and replace '\0' with '?'.
                std::streamsize pos = 0;
                QByteArray ba = textCodec->fromUnicode(text);
                for (int i=0; i<ba.length(); i++, pos++) {
                    if (pos < len && ba[i] == '\0') {
                        toFill[i] = '?';
                    }
                }
            }
        }
    }
};
#else
struct StdInputStream::TextCodec
{
    void validateBytes(XMLByte* const  toFill, std::streamsize len) {
        QByteArray encodedString(reinterpret_cast<char *>(toFill), static_cast<int>(len));
        auto toUtf16 = QStringDecoder(QStringDecoder::Utf8);
        QString text = toUtf16(encodedString);
        if (toUtf16.hasError()) {
            // In case invalid characters were found decode back to 'utf-8' and replace
            // them with '?'
            // First, Qt replaces invalid characters with '\0'
            // but Xerces doesn't like this because it handles this as termination. Thus,
            // we have to go through the array and replace '\0' with '?'.
            std::streamsize pos = 0;
            auto fromUtf16 = QStringEncoder(QStringEncoder::Utf8);
            QByteArray ba = fromUtf16(text);
            for (int i=0; i<ba.length(); i++, pos++) {
                if (pos < len && ba[i] == '\0') {
                    toFill[i] = '?';
                }
            }
        }
    }
};
#endif

StdInputStream::StdInputStream( std::istream& Stream, XERCES_CPP_NAMESPACE_QUALIFIER MemoryManager* const manager )
  : stream(Stream)
  , codec(new TextCodec)
{
    (void)manager;
}


StdInputStream::~StdInputStream() = default;


// ---------------------------------------------------------------------------
//  StdInputStream: Implementation of the input stream interface
// ---------------------------------------------------------------------------
XMLFilePos StdInputStream::curPos() const
{
  return static_cast<XMLFilePos>(stream.tellg());
}

XMLSize_t StdInputStream::readBytes(XMLByte* const  toFill, const XMLSize_t maxToRead)
{
  //
  //  Read up to the maximum bytes requested. We return the number
  //  actually read.
  //

  stream.read(reinterpret_cast<char *>(toFill), static_cast<std::streamsize>(maxToRead));
  std::streamsize len = stream.gcount();

  codec->validateBytes(toFill, len);

  return static_cast<XMLSize_t>(len);
}


// ---------------------------------------------------------------------------
//  StdInputSource: Constructors and Destructor
// ---------------------------------------------------------------------------
StdInputSource::StdInputSource ( std::istream& Stream, const char* filePath, XERCES_CPP_NAMESPACE_QUALIFIER MemoryManager* const manager )
  : InputSource(manager),stream(Stream)
{
  // we have to set the file name in case an error occurs
  XStr tmpBuf(filePath);
  setSystemId(tmpBuf.unicodeForm());
}


StdInputSource::~StdInputSource() = default;


// ---------------------------------------------------------------------------
//  StdInputSource: InputSource interface implementation
// ---------------------------------------------------------------------------
BinInputStream* StdInputSource::makeStream() const
{
  StdInputStream* retStrm = new StdInputStream(stream /*, getMemoryManager()*/);
  return retStrm;
}

