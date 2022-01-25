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

#ifndef BASE_IINPUTSOURCE_H
#define BASE_IINPUTSOURCE_H


#include <iosfwd>

#include <xercesc/sax2/Attributes.hpp>
#include <xercesc/util/BinInputStream.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XercesVersion.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <QTextCodec>
#ifndef FC_GLOBAL_H
#include <FCGlobal.h>
#endif


XERCES_CPP_NAMESPACE_BEGIN
  class BinInputStream;
XERCES_CPP_NAMESPACE_END

namespace Base
{


class BaseExport StdInputStream : public XERCES_CPP_NAMESPACE_QUALIFIER BinInputStream
{
public :
  StdInputStream ( std::istream& Stream, XERCES_CPP_NAMESPACE_QUALIFIER MemoryManager* const manager = XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::fgMemoryManager );
  virtual ~StdInputStream();

  // -----------------------------------------------------------------------
  //  Implementation of the input stream interface
  // -----------------------------------------------------------------------
#if (XERCES_VERSION_MAJOR == 2)
  virtual unsigned int curPos() const;
  virtual unsigned int readBytes( XMLByte* const toFill, const unsigned int maxToRead );
#else
  virtual XMLFilePos curPos() const;
  virtual XMLSize_t readBytes( XMLByte* const toFill, const XMLSize_t maxToRead );
  virtual const XMLCh* getContentType() const {return nullptr;}
#endif

private :
  // -----------------------------------------------------------------------
  //  Unimplemented constructors and operators
  // -----------------------------------------------------------------------
  StdInputStream(const StdInputStream&);
  StdInputStream& operator=(const StdInputStream&);

  // -----------------------------------------------------------------------
  //  Private data members
  //
  //  fSource
  //      The source file that we represent. The FileHandle type is defined
  //      per platform.
  // -----------------------------------------------------------------------
  std::istream            &stream;
  XERCES_CPP_NAMESPACE_QUALIFIER MemoryManager* const    fMemoryManager;
  QTextCodec::ConverterState state;
};


class BaseExport StdInputSource : public XERCES_CPP_NAMESPACE_QUALIFIER InputSource
{
public :
  StdInputSource ( std::istream& Stream, const char* filePath, XERCES_CPP_NAMESPACE_QUALIFIER MemoryManager* const manager = XERCES_CPP_NAMESPACE_QUALIFIER XMLPlatformUtils::fgMemoryManager );
   ~StdInputSource();

  virtual XERCES_CPP_NAMESPACE_QUALIFIER BinInputStream* makeStream() const;

private:
  StdInputSource(const StdInputSource&);
  StdInputSource& operator=(const StdInputSource&);

  std::istream   &stream;
};

}

#endif // BASE_IINPUTSOURCE_H
