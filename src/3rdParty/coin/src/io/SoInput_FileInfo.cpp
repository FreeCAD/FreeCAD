/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include "io/SoInput_FileInfo.h"

#include <cstring>
#include <cmath> // pow()

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/misc/SoProto.h>
#include <Inventor/nodes/SoNode.h>

#include "tidbitsp.h"
#include "glue/zlib.h"

// *************************************************************************

const unsigned int READBUFSIZE = 65536*2;

// *************************************************************************

SoInput_FileInfo::SoInput_FileInfo(SoInput_Reader * readerptr,
                                   const SbHash<const char *, SoBase *> & refs)
  : references(refs)
{
  this->reader = readerptr;
#if defined(HAVE_THREADS) && defined(SOINPUT_ASYNC_IO)
  this->mutex = cc_mutex_construct();
  this->condvar = cc_condvar_construct();
  this->sched = cc_sched_construct(1);
  this->threadbuf[0] = new char[READBUFSIZE];
  this->threadbuf[1] = new char[READBUFSIZE];
  this->threadbuflen[0] = -1;
  this->threadbuflen[1] = -1;
  this->threadreadidx = 0;
  this->threadbufidx = 0;
  this->threadeof = FALSE;
  this->readbuf = NULL;
#else // HAVE_THREADS && SOINPUT_ASYNC_IO
  this->readbuf = new char[READBUFSIZE];
#endif // !(HAVE_THREADS && SOINPUT_ASYNC_IO)
  this->readbuflen = 0;
  this->readbufidx = 0;

  this->header = NULL;
  this->headerisread = FALSE;
  this->ivversion = 0.0f;
  this->linenr = 1;
  this->totalread = 0;
  this->lastputback = -1;
  this->lastchar = -1;
  this->eof = FALSE;
  this->isbinary = FALSE;
  this->vrml1file = FALSE;
  this->vrml2file = FALSE;
  this->prefunc = NULL;
  this->postfunc = NULL;
  this->stdinname = "<stdin>";
  this->deletebuffer = NULL;

#if defined(HAVE_THREADS) && defined(SOINPUT_ASYNC_IO)
  if (this->reader) {
    // schedule two buffer reads
    cc_sched_schedule(this->sched, sched_cb, this, 0);
  }
#endif // HAVE_THREADS && SOINPUT_ASYNC_IO
}

SoInput_FileInfo::~SoInput_FileInfo()
{
#if defined(HAVE_THREADS) && defined(SOINPUT_ASYNC_IO)
  cc_sched_destruct(this->sched);
  cc_condvar_destruct(this->condvar);
  cc_mutex_destruct(this->mutex);
  delete[] this->threadbuf[0];
  delete[] this->threadbuf[1];
#else // HAVE_THREADS && SOINPUT_ASYNC_IO
  delete[] this->readbuf;
#endif // !(HAVE_THREADS && SOINPUT_ASYNC_IO)
  delete this->reader;
  // to be safe, delete this after deleting the reader
  delete[] this->deletebuffer;
}

#if defined(HAVE_THREADS) && defined(SOINPUT_ASYNC_IO)
void
SoInput_FileInfo::sched_cb(void * closure)
{
  SoInput_FileInfo * thisp = (SoInput_FileInfo*) closure;
  cc_mutex_lock(thisp->mutex);
  if (!thisp->threadeof) {
    int idx = thisp->threadreadidx;
    assert(thisp->threadbuflen[idx] == -1);
    size_t len = thisp->getReader()->readBuffer(thisp->threadbuf[idx], READBUFSIZE);
    if (len == 0) {
      thisp->threadeof = TRUE;
      thisp->threadbuflen[idx] = 0;
    }
    else {
      thisp->threadbuflen[idx] = len;
      thisp->threadreadidx ^= 1;
    }
  }
  cc_mutex_unlock(thisp->mutex);
  cc_condvar_wake_one(thisp->condvar);
}
#endif // HAVE_THREADS && SOINPUT_ASYNC_IO

// This function will as a side-effect set the EOF-flag, as can be
// queried by SoInput_FileInfo::isEndOfFile().
void
SoInput_FileInfo::doBufferRead(void)
{
  // Make sure that we really do need to read more bytes.
  assert(this->backbuffer.getLength() == 0);
  assert(this->readbufidx == this->readbuflen);

#if defined(HAVE_THREADS) && defined(SOINPUT_ASYNC_IO)
  cc_mutex_lock(this->mutex);
  int idx = this->threadbufidx;
  while (this->threadbuflen[idx] == -1) {
    cc_condvar_wait(this->condvar, this->mutex);
  }
  if (this->threadbuflen[idx] == 0) {
    this->readbufidx = 0;
    this->readbuflen = 0;
    this->eof = TRUE;
#if 0 // debug
    SoDebugError::postInfo("doBufferRead", "met Mr End-of-file");
#endif // debug
    cc_mutex_unlock(this->mutex);
  }
  else {
    this->totalread += this->readbufidx;
    this->readbufidx = 0;
    this->readbuflen = this->threadbuflen[idx];
    this->readbuf = this->threadbuf[idx];
    this->threadbufidx ^= 1;
    // make previous buffer ready for new data
    this->threadbuflen[this->threadbufidx] = -1;
    if (!this->threadeof) {
      cc_sched_schedule(this->sched, sched_cb, this, 0);
    }
    cc_mutex_unlock(this->mutex);
  }

#else // HAVE_THREADS && SOINPUT_ASYNC_IO

  size_t len = this->getReader()->readBuffer(this->readbuf, READBUFSIZE);
  if (len == 0) {
    this->readbufidx = 0;
    this->readbuflen = 0;
    this->eof = TRUE;
#if 0 // debug
    SoDebugError::postInfo("doBufferRead", "met Mr End-of-file");
#endif // debug
  }
  else {
    this->totalread += this->readbufidx;
    this->readbufidx = 0;
    this->readbuflen = len;
  }
#endif // !(HAVE_THREADS && SOINPUT_ASYNC_IO)
}

size_t
SoInput_FileInfo::getNumBytesParsedSoFar(void) const
{
  return this->totalread + this->readbufidx - this->backbuffer.getLength();
}

SbBool
SoInput_FileInfo::getChunkOfBytes(unsigned char * ptr, size_t length)
{
  // Suck out any bytes from the backbuffer first.
  while ((this->readbufidx == 0) && (this->backbuffer.getLength() > 0) && (length > 0)) {
    *ptr++ = this->backbuffer.pop();
    --length;
  }

  do {
    // Grab bytes from the buffer.
    while ((this->readbufidx < this->readbuflen) && (length > 0)) {
      *ptr++ = this->readbuf[this->readbufidx++];
      --length;
    }

    // Fetch more bytes if necessary. doBufferRead() sets the eof-flag
    // as a side-effect.
    if ((length > 0) && !this->eof) { this->doBufferRead(); }

  } while (length && !this->eof);

  return !this->eof;
}

void
SoInput_FileInfo::addReference(const SbName & name, SoBase * base,
                               SbBool /* addToGlobalDict */) // FIXME: why the unused arg?
{
  this->references.put(name.getString(), base);
}

void
SoInput_FileInfo::removeReference(const SbName & name)
{
  this->references.erase(name.getString());
}

SoBase *
SoInput_FileInfo::findReference(const SbName & name) const
{
  SoBase * base;
  if (this->references.get(name.getString(), base)) { return base; }
  return NULL;
}

SbBool
SoInput_FileInfo::get(char & c)
{
  if ((this->readbufidx == 0) && (this->backbuffer.getLength() > 0)) {
    c = this->backbuffer.pop();
  }
  else {
    if (this->readbufidx >= this->readbuflen) {
      // doBufferRead() sets the EOF flag for the stream if there is
      // nothing left of the buffer to read.
      this->doBufferRead();
      if (this->eof) {
        c = (char) EOF;
        return FALSE;
      }
    }

    c = this->readbuf[this->readbufidx++];
  }

  // NB: the line counting is not working 100% if we start putting
  // back and re-reading '\r\n' sequences.
  if ((c == '\r') || ((c == '\n') && (this->lastchar != '\r')))
    this->linenr++;
  this->lastchar = c;
  this->lastputback = -1;

  return TRUE;
}

void
SoInput_FileInfo::putBack(const char c)
{
  // Decrease line count if we put back an end-of-line character.
  // This should take care of Unix-, MSDOS/MSWin- and MacOS-style
  // generated files. NB: the line counting is not working 100% if
  // we start putting back and re-reading multiple parts of '\r\n'
  // sequences.
  if (!this->isbinary && ((c == '\r') || ((c == '\n') &&
                                          (this->lastputback != (int)'\r'))))
    this->linenr--;

  this->lastputback = (int)c;
  this->lastchar = -1;

  if (this->readbufidx > 0) {
    --this->readbufidx;
    // Make sure we write back the same character which was read..
    assert(c == this->readbuf[this->readbufidx]);
  }
  else {
    this->backbuffer.append(c);
  }

  this->eof = FALSE;
}

void
SoInput_FileInfo::putBack(const char * const str)
{
  assert(!this->isbinary);

  const int n = int(strlen(str));
  if (!n) return;

  // Decrease line count if we put back any end-of-line
  // characters. This should take care of Unix-, MSDOS/MSWin- and
  // MacOS-style generated files. What a mess.
  for (int i = 0; i < n; ++i) {
    if ((str[i] == '\r') || ((str[i] == '\n') &&
                             (this->lastputback != (int)'\r')))
      --this->linenr;
    this->lastputback = (int)str[i];
  }

  this->lastchar = -1;

  for (int c = n - 1; c >= 0; --c) {
    if (this->readbufidx > 0) {
      --this->readbufidx;
#if COIN_DEBUG
      assert(this->readbuf[this->readbufidx] == str[c]);
#endif
    }
    else {
      this->backbuffer.append(str[c]);
    }
  }

  this->eof = FALSE;
}

SbBool
SoInput_FileInfo::skipWhiteSpace(void)
{
  const char COMMENT_CHAR = '#';

  while (TRUE) {
    char c;
    SbBool gotchar;
    while ((gotchar = this->get(c)) && this->isSpace(c)) ;

    if (!gotchar) return FALSE;

    if (c == COMMENT_CHAR) {
      while ((gotchar = this->get(c)) && (c != '\n') && (c != '\r')) ;
      if (!gotchar) return FALSE;
      if (c == '\r') {
        gotchar = this->get(c);
        if (!gotchar) return FALSE;
        if (c != '\n') this->putBack(c);
      }
    }
    else {
      this->putBack(c);
      break;
    }
  }
  return TRUE;
}

// Returns TRUE if an attempt at reading the file header went
// without hitting EOF. Check this->ivversion != 0.0f to see if the
// header parse actually succeeded.

// The SoInput parameter is used in the precallback
SbBool
SoInput_FileInfo::readHeaderInternal(SoInput * soinput)
{
  this->headerisread = TRUE;

  this->header = "";
  this->ivversion = 0.0f;
  this->vrml1file = FALSE;
  this->vrml2file = FALSE;

  char c;
  if (!this->get(c)) return FALSE;

  if (c != '#') {
    this->putBack(c);
    return TRUE;
  }

  this->header += c;

  while (this->get(c) && (c != '\n') && (c != '\r')) this->header += c;
  if (this->eof) return FALSE;

  if (!SoDB::getHeaderData(this->header, this->isbinary, this->ivversion,
                           this->prefunc, this->postfunc, this->userdata,
                           TRUE)) {
    SbString putback = this->header;
    putback += c;
    this->putBack(putback.getString()); // put back invalid header
    this->ivversion = 0.0f;
  }
  else {
    SbString vrml1string("#VRML V1.0 ascii");
    SbString vrml2string("#VRML V2.0 utf8");

    if (strncmp(vrml1string.getString(), this->header.getString(),
                vrml1string.getLength()) == 0) {
      this->vrml1file = TRUE;
    }
    else if (strncmp(vrml2string.getString(), this->header.getString(),
                     vrml2string.getLength()) == 0) {
      this->vrml2file = TRUE;
    }
    if (this->prefunc) this->prefunc(this->userdata, soinput);
  }
  return TRUE;
}

void
SoInput_FileInfo::connectRoutes(SoInput * in)
{
  const SbName * routeptr = this->routelist.getArrayPtr();
  const int n = this->routelist.getLength();
  for (int i = 0; i < n; i += 4) {
    SbName fromnodename = routeptr[i];
    SbName fromfieldname = routeptr[i+1];
    SbName tonodename = routeptr[i+2];
    SbName tofieldname = routeptr[i+3];

    SoNode * fromnode = SoNode::getByName(fromnodename);
    SoNode * tonode = SoNode::getByName(tonodename);

    if (!fromnode || !tonode) {
      SoReadError::post(in,
                        "Unable to create ROUTE from %s.%s to %s.%s. "
                        "Couldn't find both node references.",
                        fromnodename.getString(), fromfieldname.getString(),
                        tonodename.getString(), tofieldname.getString());
    }
    else {
      (void)SoBase::connectRoute(in, fromnodename, fromfieldname,
                                 tonodename, tofieldname);
    }
  }
}

// Unrefernce all protos
void
SoInput_FileInfo::unrefProtos(void)
{
  const int n = this->protolist.getLength();
  for (int i = 0; i < n; i++) {
    this->protolist[i]->unref();
  }
  this->protolist.truncate(0);
}

// search for PROTO in this SoInput instance
SoProto *
SoInput_FileInfo::findProto(const SbName & name)
{
  const int n = this->protolist.getLength();
  SoProto * const * ptr = this->protolist.getArrayPtr();
  for (int i = 0; i < n; i++) {
    if (ptr[i]->getProtoName() == name) return ptr[i];
  }
  return NULL;
}

// wrapper around this->reader. We delay creating the reader if we're
// reading from stdin (reader == NULL).
SoInput_Reader *
SoInput_FileInfo::getReader(void)
{
  if (this->reader == NULL) {
    this->reader = SoInput_Reader::createReader(coin_get_stdin(), SbString("<stdin>"));
#if defined(HAVE_THREADS) && defined(SOINPUT_ASYNC_IO)
    // schedule a buffer read
    cc_sched_schedule(this->sched, sched_cb, this, 0);
#endif // HAVE_THREADS && SOINPUT_ASYNC_IO
  }
  return this->reader;
}

SbBool
SoInput_FileInfo::readUnsignedIntegerString(char * str)
{
  assert(!this->isBinary());
  int minSize = 1;
  char * s = str;

  if (this->readChar(s, '0')) {
    if (this->readChar(s + 1, 'x')) {
      s += 2 + this->readHexDigits(s + 2);
      minSize = 3;
    }
    else
      s += 1 + this->readDigits(s + 1);
  }
  else
    s += this->readDigits(s);

  if (s - str < minSize)
    return FALSE;

  *s = '\0';
  return TRUE;
}

SbBool
SoInput_FileInfo::readUnsignedInteger(uint32_t & l)
{
  assert(!this->isBinary());
  // FIXME: fixed size buffer for input of unknown
  // length. Ouch. 19990530 mortene.
  char str[512];
  if (! this->readUnsignedIntegerString(str))
    return FALSE;

  // FIXME: check man page of strtoul and exploit the functionality
  // provided better -- it looks like we are duplicating some of the
  // effort. 19990530 mortene.
  l = strtoul(str, NULL, 0);

  return TRUE;
}

SbBool
SoInput_FileInfo::readInteger(int32_t & l)
{
  assert(!this->isBinary());
  // FIXME: fixed size buffer for input of unknown
  // length. Ouch. 19990530 mortene.
  char str[512];
  char * s = str;
  SbBool minus = FALSE;
  if (this->readChar(s, '-')) {
    minus = TRUE;
    s++;
  }
  else if (this->readChar(s, '+')) s++;
  if (! this->readUnsignedIntegerString(s))
    return FALSE;

  // FIXME: check man page of strtol and exploit the functionality
  // provided better -- it looks like we are duplicating some of the
  // effort. 19990530 mortene.
#if 1 // old code
  l = strtol(str, NULL, 0);
#else // first version of replacement of strtol. Not activated yet
  int i, n = strlen(s);
  if (n >= 3 && s[0] == '0' && s[1] == 'x') {
    int v = 0;
    int mul = 1;
    for (i = 2; i < n; i++) {
      char c = s[(n-1)-i+2];
      if (c >= '0' && c <= '9') {
        v += (c-'0') * mul;
      }
      else if (c >= 'a' && c <= 'f') {
        v += ((c-'a')+10) * mul;
      }
      else {
        v += ((c-'A')+10) * mul;
      }
      mul <<= 4;
    }
    l = v;
  }
  else {
    int v = 0;
    int mul = 1;
    for (i = 0; i < n; i++) {
      char c = s[(n-1)-i];
      v += (c-'0') * mul;
      mul *= 10;
    }
    l = v;
  }
  if (minus) l = -l;
#endif // strtol replacement
  return TRUE;
}

SbBool
SoInput_FileInfo::readReal(double & d)
{
  assert(!this->isBinary());
  const int BUFSIZE = 2048;
  SbBool minus = FALSE;
  SbBool gotNum = FALSE;
  int i, n;
  char str[BUFSIZE];
  char * s = str;

  double number;
  double exponent;

  n = this->readChar(s, '-');
  if (n == 0) {
    n = this->readChar(s, '+');
  }
  else minus = TRUE;
  s += n;

  if ((n = this->readDigits(s)) > 0) {
    gotNum = TRUE;
    number = 0.0;
    double mul = 1.0;
    for (i = 0; i < n; i++) {
      number += (s[(n-1)-i] - '0') * mul;
      mul *= 10.0;
    }
    s += n;
  }
  else {
    number = 0.0;
  }
  if (this->readChar(s, '.') > 0) {
    s++;

    if ((n = this->readDigits(s)) > 0) {
      gotNum = TRUE;
      double mul = 0.1;
      for (i = 0; i < n; i++) {
        number += (s[i]-'0') * mul;
        mul *= 0.1;
      }
      s += n;
    }
  }

  if (! gotNum)
    return FALSE;

  if (minus) number = -number;

  n = this->readChar(s, 'e');
  if (n == 0)
    n = this->readChar(s, 'E');

  if (n > 0) {
    s += n;

    minus = FALSE;
    n = this->readChar(s, '-');
    if (n == 0) {
      n = this->readChar(s, '+');
    }
    else minus = TRUE;
    s += n;

    if ((n = this->readDigits(s)) > 0) {
      exponent = 0.0;
      double mul = 1.0;
      for (i = 0; i < n; i++) {
        exponent += (s[(n-1)-i]-'0') * mul;
        mul *= 10.0;
      }
      if (minus) exponent = -exponent;

      number *= pow(10.0, exponent);
    }
    else
      return FALSE;
  }

  d = number;
  return TRUE;
}

int
SoInput_FileInfo::readChar(char * s, char charToRead)
{
  int ret = 0;
  char c;
  if (this->get(c)) {
    if (c == charToRead) {
      *s = c;
      ret = 1;
    }
    else {
      this->putBack(c);
    }
  }
  return ret;
}

int
SoInput_FileInfo::readDigits(char * str)
{
  assert(!this->isBinary());
  char c, * s = str;

  while (this->get(c)) {
    if (isdigit(c))
      *s++ = c;
    else {
      this->putBack(c);
      break;
    }
  }
  const ptrdiff_t offset = s - str;
  return (int)offset;
}

int
SoInput_FileInfo::readHexDigits(char * str)
{
  assert(!this->isBinary());
  char c, * s = str;
  while (this->get(c)) {

    if (isxdigit(c)) *s++ = c;
    else {
      this->putBack(c);
      break;
    }
  }
  const ptrdiff_t offset = s - str;
  return (int)offset;
}
