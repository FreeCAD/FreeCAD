/************
*
*   This file is part of a tool for producing 3D content in the PRC format.
*   Copyright (C) 2008  Orest Shardt <shardtor (at) gmail dot com> and
*                       Michail Vidiassov <master@iaas.msu.ru>
*
*   This program is free software: you can redistribute it and/or modify
*   it under the terms of the GNU Lesser General Public License as published by
*   the Free Software Foundation, either version 3 of the License, or
*   (at your option) any later version.
*
*   This program is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*   GNU Lesser General Public License for more details.
*
*   You should have received a copy of the GNU Lesser General Public License
*   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*************/

#include <iostream>
#include <zlib.h>
#include <stdlib.h>
#include <string.h>
#include <cassert>
#include "PRCbitStream.h"
#include "PRCdouble.h"

using std::string;
using std::cerr;
using std::endl;

void PRCbitStream::compress()
{
  const int CHUNK= 1024; // is this reasonable?
  compressedDataSize = 0;

  z_stream strm;
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  if(deflateInit(&strm,Z_DEFAULT_COMPRESSION) != Z_OK)
  {
    cerr << "Compression initialization failed" << endl;
    return;
  }
  unsigned int sizeAvailable = deflateBound(&strm,getSize());
  uint8_t *compressedData = (uint8_t*) malloc(sizeAvailable);
  strm.avail_in = getSize();
  strm.next_in = (unsigned char*)data;
  strm.next_out = (unsigned char*)compressedData;
  strm.avail_out = sizeAvailable;

  int code;
  unsigned int chunks = 0;
  while((code = deflate(&strm,Z_FINISH)) == Z_OK)
  {
    ++chunks;
    // strm.avail_out should be 0 if we got Z_OK
    compressedDataSize = sizeAvailable - strm.avail_out;
    compressedData = (uint8_t*) realloc(compressedData,CHUNK*chunks);
    strm.next_out = (Bytef*)(compressedData + compressedDataSize);
    strm.avail_out += CHUNK;
    sizeAvailable += CHUNK;
  }
  compressedDataSize = sizeAvailable-strm.avail_out;

  if(code != Z_STREAM_END)
  {
    cerr << "Compression error" << endl;
    deflateEnd(&strm);
    free(compressedData);
    return;
  }

  compressed = true;

  free(data);
  data = compressedData;

  deflateEnd(&strm);
}

void PRCbitStream::write(std::ostream &out) const
{
  if(compressed)
  {
    out.write((char*)data,compressedDataSize);
  }
  else
  {
     cerr << "Attempt to write stream before compression." << endl;
     exit(1);
  }
}

unsigned int PRCbitStream::getSize() const
{
  if(compressed)
    return compressedDataSize;
  else
    return byteIndex+1;
}

uint8_t* PRCbitStream::getData()
{
  return data;
}

PRCbitStream& PRCbitStream::operator <<(bool b)
{
  writeBit(b);
  return *this;
}

PRCbitStream& PRCbitStream::operator <<(uint32_t u)
{
  while(u != 0)
  {
    writeBit(1);
    writeByte(u & 0xFF);
    u >>= 8;
  }
  writeBit(0);
  return *this;
}

PRCbitStream& PRCbitStream::operator <<(uint8_t u)
{
  writeByte(u);
  return *this;
}

PRCbitStream& PRCbitStream::operator <<(int32_t i)
{
  uint8_t lastByte = 0;
  //while(!((current value is 0 and last byte was positive) OR (current value is -1 and last value was negative)))
  while(!(((i == 0)&&((lastByte & 0x80)==0))||((i == -1)&&((lastByte & 0x80) != 0))))
  {
    writeBit(1);
    lastByte = i & 0xFF;
    writeByte(lastByte);
    i >>= 8;
  }
  writeBit(0);
  return *this;
}

PRCbitStream& PRCbitStream::operator <<(double value)
{
  // write a double
  if(compressed)
  {
    cerr << "Cannot write to a stream that has been compressed." << endl;
    return *this;
  }
  union ieee754_double *pid=(union ieee754_double *)&value;
  int
        i,
        fSaveAtEnd;
        PRCbyte
        *pb,
        *pbStart,
        *pbStop,
        *pbEnd,
        *pbResult,
        bSaveAtEnd = 0;
  struct sCodageOfFrequentDoubleOrExponent
        cofdoe,
        *pcofdoe;

  cofdoe.u2uod.Value=value;
  pcofdoe = (struct sCodageOfFrequentDoubleOrExponent *)bsearch(
                           &cofdoe,
                           acofdoe,
                           sizeof(acofdoe)/sizeof(pcofdoe[0]),
                           sizeof(pcofdoe[0]),
                           stCOFDOECompare);

  while(pcofdoe>acofdoe && EXPONENT(pcofdoe->u2uod.Value)==EXPONENT((pcofdoe-1)->u2uod.Value))
    pcofdoe--;

  assert(pcofdoe);
  while(pcofdoe->Type==VT_double)
  {
    if(fabs(value)==pcofdoe->u2uod.Value)
      break;
    pcofdoe++;
  }

  for(i=1<<(pcofdoe->NumberOfBits-1);i>=1;i>>=1)
    writeBit((pcofdoe->Bits&i)!=0);

  if
  (
    !memcmp(&value,stadwZero,sizeof(value))
    ||      !memcmp(&value,stadwNegativeZero,sizeof(value))
  )
    return *this;

  writeBit(pid->ieee.negative);

  if(pcofdoe->Type==VT_double)
    return *this;

  if(pid->ieee.mantissa0==0 && pid->ieee.mantissa1==0)
  {
    writeBit(0);
    return *this;
  }

  writeBit(1);

#ifdef WORDS_BIGENDIAN
  pb=((PRCbyte *)&value)+1;
#else
  pb=((PRCbyte *)&value)+6;
#endif
  //add_bits((*pb)&0x0f,4 STAT_V STAT_DOUBLE);
  writeBits((*pb)&0x0F,4);

  NEXTBYTE(pb);
  pbStart=pb;
#ifdef WORDS_BIGENDIAN
  pbEnd=
  pbStop= ((PRCbyte *)(&value+1))-1;
#else
  pbEnd=
  pbStop= ((PRCbyte *)&value);
#endif

  if((fSaveAtEnd=(*pbStop!=*BEFOREBYTE(pbStop)))!=0)
    bSaveAtEnd=*pbEnd;
  PREVIOUSBYTE(pbStop);

  while(*pbStop==*BEFOREBYTE(pbStop))
    PREVIOUSBYTE(pbStop);

  for(;MOREBYTE(pb,pbStop);NEXTBYTE(pb))
  {
    if(pb!=pbStart && (pbResult=SEARCHBYTE(BEFOREBYTE(pb),*pb,DIFFPOINTERS(pb,pbStart)))!=NULL)
    {
      writeBit(0);
      writeBits(DIFFPOINTERS(pb,pbResult),3);
    }
    else
    {
      writeBit(1);
      writeByte(*pb);
    }
  }

  if(!MOREBYTE(BEFOREBYTE(pbEnd),pbStop))
  {
    if(fSaveAtEnd)
    {
      writeBit(0);
      writeBits(6,3);
      writeByte(bSaveAtEnd);
    }
    else
    {
      writeBit(0);
      writeBits(0,3);
    }
  }
  else
  {
    if((pbResult=SEARCHBYTE(BEFOREBYTE(pb),*pb,DIFFPOINTERS(pb,pbStart)))!=NULL)
    {
      writeBit(0);
      writeBits(DIFFPOINTERS(pb,pbResult),3);
    }
    else
    {
      writeBit(1);
      writeByte(*pb);
    }
  }

  return *this;
}

PRCbitStream& PRCbitStream::operator <<(const char* s)
{
  if (s == NULL)
  {
    writeBit(false); // string is NULL
    return *this;
  }
  string str(s);
  *this << str;
  return *this;
}

PRCbitStream& PRCbitStream::operator <<(const string& s)
{
  if(s == "")
  {
    writeBit(false); // string is NULL
    return *this;
  }
  writeBit(true);
  size_t l = s.length();
  *this << static_cast<uint32_t>(l);
  for(size_t i = 0; i < l; ++i)
    writeByte(s[i]);
  return *this;
}

void PRCbitStream::writeBit(bool b)
{
  if(compressed)
  {
    cerr << "Cannot write to a stream that has been compressed." << endl;
    return;
  }

  if(b)
  {
    data[byteIndex] |= (0x80 >> bitIndex);
  }
  nextBit();
}

void PRCbitStream::writeBits(uint32_t u, uint8_t bits)
{
  if(bits > 32)
    return;
  else
  {
    for(uint32_t mask = (1 << (bits-1)); mask != 0; mask >>= 1)
    {
      writeBit((u&mask) != 0);
    }
  }
}

void PRCbitStream::writeByte(uint8_t u)
{
  if(compressed)
  {
    cerr << "Cannot write to a stream that has been compressed." << endl;
    return;
  }

  if(bitIndex == 0)
  {
    data[byteIndex] = u;
    nextByte();
  }
  else
  {
    data[byteIndex] |= (u >> bitIndex);
    unsigned int obi = bitIndex;
    nextByte();
    data[byteIndex] |= (u << (8-obi));
    bitIndex = obi; // bit index is not changed by writing 8 bits
  }
}

void PRCbitStream::nextBit()
{
  ++bitIndex;
  if(bitIndex == 8)
  {
    nextByte();
  }
}

void PRCbitStream::nextByte()
{
  ++byteIndex;
  if(byteIndex >= allocatedLength)
    getAChunk();
  data[byteIndex] = 0; // clear the garbage data
  bitIndex = 0;
}

void PRCbitStream::getAChunk()
{
   if(allocatedLength==0)
     data = (uint8_t*)realloc((void*)data,CHUNK_SIZE);
   else
     data = (uint8_t*)realloc((void*)data,2*allocatedLength);

   if(data != NULL)
   {
     if(allocatedLength==0)
     {
       allocatedLength = CHUNK_SIZE;
       *data = 0; // clear first byte
     }
     else
       allocatedLength *= 2;
   }
   else
   {
     // warn about memory problem!
     cerr << "Memory allocation error." << endl;
     exit(1);
   }
}
