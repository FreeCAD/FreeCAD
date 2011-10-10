/***************************************************************************
 *   Copyright (c) Riegel         <juergen.riegel@web.de>                  *
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
# include <xercesc/sax/SAXParseException.hpp>
# include <xercesc/sax/SAXException.hpp>
# include <xercesc/sax2/XMLReaderFactory.hpp>
#endif

// ---------------------------------------------------------------------------
//  Includes
// ---------------------------------------------------------------------------
#include <xercesc/util/Janitor.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLExceptMsgs.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/util/XMLString.hpp>
#include <xercesc/util/XMLUniDefs.hpp>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "InputSource.h"
#include "Exception.h"
#include "XMLTools.h"

XERCES_CPP_NAMESPACE_USE

using namespace Base;
using namespace std;


// ---------------------------------------------------------------------------
//  StdInputStream: Constructors and Destructor
// ---------------------------------------------------------------------------
StdInputStream::StdInputStream( std::istream& Stream, XERCES_CPP_NAMESPACE_QUALIFIER MemoryManager* const manager ) 
  : stream(Stream), fMemoryManager(manager)
{
}


StdInputStream::~StdInputStream()
{
}


// ---------------------------------------------------------------------------
//  StdInputStream: Implementation of the input stream interface
// ---------------------------------------------------------------------------
#if (XERCES_VERSION_MAJOR == 2)
unsigned int StdInputStream::curPos() const
{
  return stream.tellg();
}

unsigned int StdInputStream::readBytes( XMLByte* const  toFill, const unsigned int maxToRead )
{
  //
  //  Read up to the maximum bytes requested. We return the number
  //  actually read.
  //
  
  stream.read((char *)toFill,maxToRead);
  XMLSize_t len = stream.gcount();

  // See http://de.wikipedia.org/wiki/UTF-8#Kodierung
  for (XMLSize_t i=0; i<len; i++) {
      XMLByte& b = toFill[i];
      int seqlen = 0;

      if ((b & 0x80) == 0) {
          seqlen = 1;
      }
      else if ((b & 0xE0) == 0xC0) {
          seqlen = 2;
          if (b == 0xC0 || b == 0xC1)
              b = '?'; // these both values are not allowed
      }
      else if ((b & 0xF0) == 0xE0) {
          seqlen = 3;
      }
      else if ((b & 0xF8) == 0xF0) {
          seqlen = 4;
      }
      else {
          b = '?';
      }

      for(int j = 1; j < seqlen; ++j) {
          i++;
          XMLByte& c = toFill[i];
          // range of second, third or fourth byte
          if ((c & 0xC0) != 0x80) {
              b = '?';
              c = '?';
          }
      }
  }

  return len;
}
#else
XMLFilePos StdInputStream::curPos() const
{
  return stream.tellg();
}

XMLSize_t StdInputStream::readBytes( XMLByte* const  toFill, const XMLSize_t maxToRead )
{
  //
  //  Read up to the maximum bytes requested. We return the number
  //  actually read.
  //
  
  stream.read((char *)toFill,maxToRead);
  XMLSize_t len = stream.gcount();

  // See http://de.wikipedia.org/wiki/UTF-8#Kodierung
  for (XMLSize_t i=0; i<len; i++) {
      XMLByte& b = toFill[i];
      int seqlen = 0;

      if ((b & 0x80) == 0) {
          seqlen = 1;
      }
      else if ((b & 0xE0) == 0xC0) {
          seqlen = 2;
          if (b == 0xC0 || b == 0xC1)
              b = '?'; // these both values are not allowed
      }
      else if ((b & 0xF0) == 0xE0) {
          seqlen = 3;
      }
      else if ((b & 0xF8) == 0xF0) {
          seqlen = 4;
      }
      else {
          b = '?';
      }

      for(int j = 1; j < seqlen; ++j) {
          i++;
          XMLByte& c = toFill[i];
          // range of second, third or fourth byte
          if ((c & 0xC0) != 0x80) {
              b = '?';
              c = '?';
          }
      }
  }

  return len;
}
#endif


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


StdInputSource::~StdInputSource()
{
}


// ---------------------------------------------------------------------------
//  StdInputSource: InputSource interface implementation
// ---------------------------------------------------------------------------
BinInputStream* StdInputSource::makeStream() const
{
  StdInputStream* retStrm = new StdInputStream(stream /*, getMemoryManager()*/);
  return retStrm;
}

