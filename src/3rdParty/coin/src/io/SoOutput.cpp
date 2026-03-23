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

/*!
  \class SoOutput SoOutput.h Inventor/SoOutput.h
  \brief The SoOutput class is an abstraction of an output stream.

  \ingroup coin_general

  SoOutput offers the ability to write basic types to a file or a
  memory buffer in either ASCII or binary format.

  \sa SoInput, SoWriteAction
*/

/*!
  \page coin_compression_overview File compression

  Coin can support reading and writing Inventor and VRML files compressed
  with either gzip or bzip2 (as of yet - more formats may come in the
  future).  This functionality is of course only enabled when Coin can link
  with the corresponding compression libraries.

  Reading of compressed files happens in a transparent manner - there is
  nothing the developer has to do to support this.

  Writing of compressed files is controlled through the
  SoOutput::setCompression method.

  The list of available compression methods can be fetched with the
  SoOutput::getAvailableCompressionMethods method.

  You cannot use file compression together with I/O to memory buffers,
  except for reading from memory buffers containing gzip-compressed files.

  For backwards compatibility with Coin 2.0 and Coin 1.0, compressed files
  must not be used.  Compressed files works only from Coin 2.1 and
  upwards.

  \since Coin 2.1
  \sa SoOutput::setCompression, SoOutput::getAvailableCompressionMethods
*/

// *************************************************************************

#include <Inventor/SoOutput.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <cassert>
#include <cstring>

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif // HAVE_WINDOWS_H

#include <Inventor/C/tidbits.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/SbName.h>
#include <Inventor/SbString.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/lists/SoFieldList.h>
#include <Inventor/fields/SoFieldContainer.h>
#include <Inventor/fields/SoField.h>

#include "tidbitsp.h"
#include "misc/SbHash.h"
#include "glue/zlib.h"
#include "glue/bzip2.h"
#include "io/SoOutput_Writer.h"
#include "io/SoWriterefCounter.h"

// *************************************************************************

/*! \enum SoOutput::Stage
  Enumerates the possible stages of a write operation (writing needs to be
  done in multiple passes).

  \sa setStage(), getStage()
*/
/*! \var SoOutput::COUNT_REFS
  Not writing, just counting the internal references in the scene graph.
*/
/*! \var SoOutput::WRITE
  Signifies that actual data export should take place during this pass.
*/

/*! \enum SoOutput::Annotations
  Values from this enum is used for debugging purposes to annotate the
  output from a write operation.
*/
/*! \var SoOutput::ADDRESSES
  Annotate output with pointer address information.
*/
/*! \var SoOutput::REF_COUNTS
  Annotate output with reference counts of the objects written.
*/

/*! \var SbBool SoOutput::wroteHeader
  Indicates whether or not the file format header has been written out.
  As long as this is \a FALSE, the header will be written once upon the
  first invocation of any write method in the class.
*/

// *************************************************************************

// FIXME: need to fix EOL on other platforms? 19990621 mortene.
static const char EOLSTR[] = "\n";

// FIXME: I guess this should be modified on non-32 bit platforms? Or?
// Wouldn't that puck up cross-platform compatibility of binary files?
// 19990627 mortene.
static const size_t HOSTWORDSIZE = 4;

// *************************************************************************

// helper classes for storing ROUTEs
class SoOutputROUTE {
public:
  SoFieldContainer * from, * to;
  SbName fromfield, tofield;
};

class SoOutputROUTEList : public SbList<SoOutputROUTE> {
public:
  SoOutputROUTEList(void) : SbList<SoOutputROUTE>() { }
  SoOutputROUTEList(const int sizehint) : SbList<SoOutputROUTE>(sizehint) { }
  SoOutputROUTEList(const SoOutputROUTEList & l) : SbList<SoOutputROUTE>(l) { }

  void set(const int index, SoOutputROUTE item) { (*this)[index] = item; }
};

// *************************************************************************

unsigned int SbHashFunc(const SoOutput * key) {
  return SbHashFunc(reinterpret_cast<size_t>(key));
}

// FIXME: should use a real set datatype -- the object mapped to is
// just a dummy. 20050524 mortene.
typedef SbHash<const char *, void *> BogusSet;

class SoOutputP {
public:
  SoOutputP(void) {
    this->writer = NULL;
  }
  ~SoOutputP() {
    delete this->writer;
  }

  SbBool binarystream;
  SbBool usercalledopenfile;
  SbString fltprecision;
  SbString dblprecision;
  int indentlevel;
  SbBool writecompact;
  SbBool disabledwriting;
  SbString * headerstring;
  SoOutput::Stage stage;
  uint32_t annotationbits;
  SbList <SoProto*> protostack;
  SbList <BogusSet *> defstack;
  SbList <SoOutputROUTEList *> routestack;
  SoWriterefCounter * counter;

  SbName compmethod;
  float complevel;

  void pushRoutes(const SbBool copyprev) {
    const int oldidx = this->routestack.getLength() - 1;
    assert(oldidx >= 0);
    SoOutputROUTEList * newlist;
    SoOutputROUTEList * oldlist = this->routestack[oldidx];
    if (copyprev && oldlist && oldlist->getLength()) {
      newlist = new SoOutputROUTEList(*oldlist);
    }
    else newlist = new SoOutputROUTEList;
    this->routestack.push(newlist);
  }
  SoOutputROUTEList * getCurrentRoutes(const SbBool createifnull) {
    const int n = this->routestack.getLength();
    assert(n);
    SoOutputROUTEList * list = this->routestack[n-1];
    if (list == NULL && createifnull) {
      list = new SoOutputROUTEList;
      this->routestack[n-1] = list;
    }
    return list;
  }

  void popRoutes(void) {
    const int idx = this->routestack.getLength()-1;
    assert(idx >= 0);
    delete this->routestack[idx];
    this->routestack.remove(idx);
  }

  void pushDefNames(const SbBool copyprev) {
    const int n = this->defstack.getLength();
    assert(n);
    BogusSet * prev = this->defstack[n-1];
    if (copyprev && prev) {
      this->defstack.append(new BogusSet(*prev));
    }
    else this->defstack.append(NULL);
  }
  void popDefNames(void) {
    assert(this->defstack.getLength());
    delete this->defstack[this->defstack.getLength()-1];
    this->defstack.pop();
  }
  BogusSet * getCurrentDefNames(const SbBool createifnull) {
    const int idx = this->defstack.getLength() - 1;
    assert(idx >= 0);
    BogusSet * dict = this->defstack[idx];
    if (createifnull && dict == NULL) {
      dict = new BogusSet;
      this->defstack[idx] = dict;
    }
    return dict;
  }

  SoOutput_Writer * getWriter(void) {
    if (this->writer == NULL) {
      this->writer = SoOutput_Writer::createWriter(coin_get_stdout(), FALSE,
                                                   this->compmethod, this->complevel);
    }
    return this->writer;
  }
  void setWriter(SoOutput_Writer * writerptr) {
    delete this->writer;
    this->writer = writerptr;
  }
private:
  SoOutput_Writer * writer;

};

static SbList <SbName> * SoOutput_compmethods = NULL;

// *************************************************************************

static void
SoOutput_compression_list_cleanup(void)
{
  delete SoOutput_compmethods;
  SoOutput_compmethods = NULL;
}

static void
SoOutput_compression_list_init(void)
{
  if (SoOutput_compmethods) return;

  SoOutput_compmethods = new SbList <SbName>;
  if (cc_zlibglue_available()) {
    SoOutput_compmethods->append(SbName("GZIP"));
  }
  if (cc_bzglue_available()) {
    SoOutput_compmethods->append(SbName("BZIP2"));
  }
  coin_atexit((coin_atexit_f*) SoOutput_compression_list_cleanup, CC_ATEXIT_NORMAL);
}

#define PRIVATE(obj) (obj->pimpl)

/*!
  The default constructor makes an SoOutput instance which will write
  to the standard output.

  \sa setFilePointer(), openFile(), setBuffer()
*/
SoOutput::SoOutput(void)
{
  this->constructorCommon();
  PRIVATE(this)->defstack.append(NULL);

  SoWriterefCounter::create(this, NULL);
  PRIVATE(this)->counter = SoWriterefCounter::instance(this);
}

/*!
  Constructs an SoOutput which has a copy of the set of named
  references from \a dictOut.
*/
SoOutput::SoOutput(SoOutput * dictOut)
{
  assert(dictOut != NULL);
  this->constructorCommon();

  BogusSet * olddef = PRIVATE(dictOut)->getCurrentDefNames(FALSE);
  PRIVATE(this)->defstack.append(olddef ? new BogusSet(*olddef) : NULL);

  SoWriterefCounter::create(this, dictOut);
  PRIVATE(this)->counter = SoWriterefCounter::instance(this);
}

/*!
  \COININTERNAL
  Common constructor actions.
 */
void
SoOutput::constructorCommon(void)
{
  PRIVATE(this) = new SoOutputP;

  PRIVATE(this)->usercalledopenfile = FALSE;
  PRIVATE(this)->binarystream = FALSE;
  PRIVATE(this)->fltprecision = "%.8g";
  PRIVATE(this)->dblprecision = "%.16lg";
  PRIVATE(this)->disabledwriting = FALSE;
  this->wroteHeader = FALSE;
  PRIVATE(this)->writecompact = FALSE;
  PRIVATE(this)->headerstring = NULL;
  PRIVATE(this)->indentlevel = 0;
  PRIVATE(this)->annotationbits = 0x00;
  PRIVATE(this)->routestack.append(NULL);

  PRIVATE(this)->compmethod = SbName("NONE");
  PRIVATE(this)->complevel = 0.0f;;
}

/*!
  Destructor.
*/
SoOutput::~SoOutput(void)
{
  SoWriterefCounter::destruct(this);
  this->reset();
  delete PRIVATE(this)->headerstring;
  delete PRIVATE(this);
}

/*!
  Set up a new file pointer which we will write to.

  Important note: do \e not use this method when the Coin library has
  been compiled as a Microsoft Windows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for Microsoft Windows DLLs.

  \sa openFile(), setBuffer(), getFilePointer()
 */
void
SoOutput::setFilePointer(FILE * newFP)
{
  this->reset();
  PRIVATE(this)->setWriter(SoOutput_Writer::createWriter(newFP, FALSE,
                                                         PRIVATE(this)->compmethod,
                                                         PRIVATE(this)->complevel));
}

/*!
  Returns the current file pointer. If we're writing to a memory
  buffer, \c NULL is returned.

  Important note: do \e not use this method when the Coin library has
  been compiled as a Microsoft Windows DLL, as passing FILE* instances back
  or forth to DLLs is dangerous and will most likely cause a
  crash. This is an intrinsic limitation for Microsoft Windows DLLs.

  \sa setFilePointer()
 */
FILE *
SoOutput::getFilePointer(void) const
{
  return PRIVATE(this)->getWriter()->getFilePointer();
}

/*!
  Opens a file for writing. If the file cannot be opened or is not
  writable, \a FALSE will be returned.

  Files opened by this method will automatically be closed if the
  user supplies another file pointer, another filename for writing,
  or if the SoOutput instance is deleted.

  \sa setFilePointer(), setBuffer(), closeFile()
 */
SbBool
SoOutput::openFile(const char * const fileName)
{
  this->reset();

  FILE * newfile = fopen(fileName, "wb");
  if (newfile) {
    PRIVATE(this)->setWriter(SoOutput_Writer::createWriter(newfile, TRUE,
                                                           PRIVATE(this)->compmethod,
                                                           PRIVATE(this)->complevel));
    PRIVATE(this)->usercalledopenfile = TRUE;
  }
  else {
    SoDebugError::postWarning("SoOutput::openFile",
                              "Couldn't open file '%s' for writing.",
                              fileName);
  }
  return newfile != NULL;
}

/*!
  Closes the currently opened file, but only if the file was passed to
  SoOutput through the openFile() method.

  \sa openFile()
 */
void
SoOutput::closeFile(void)
{
  if (PRIVATE(this)->usercalledopenfile) {
    PRIVATE(this)->setWriter(NULL);
    PRIVATE(this)->usercalledopenfile = FALSE;
  }
}

/*!

  Sets the compression method and level used when writing the file. \a
  compmethod is the compression library/method to use when
  compressing. \a level is the compression level, where 0.0 means no
  compression and 1.0 means maximum compression.

  Currently \e BZIP2, \e GZIP are the only compression methods
  supported, and you have to compile Coin with zlib and bzip2-support
  to enable them.

  Supply \a compmethod = \e NONE or \e level = 0.0 if you want to
  disable compression. The compression is disabled by default.

  Please note that it's not possible to compress when writing to a
  memory buffer.

  This method will return \e TRUE if the compression method selected
  is available. If it is not available, \e FALSE will be returned and
  compression is disabled.

  \sa getAvailableCompressionMethods()
  \since Coin 2.1
 */
SbBool
SoOutput::setCompression(const SbName & compmethod, const float level)
{
  PRIVATE(this)->complevel = level;
  PRIVATE(this)->compmethod = compmethod;

  if (compmethod == "GZIP") {
    if (cc_zlibglue_available()) return TRUE;
    SoDebugError::postWarning("SoOutput::setCompression",
                              "Requested GZIP compression, but zlib is not available.");

  }
  if (compmethod == "BZIP2") {
    if (cc_bzglue_available()) return TRUE;
    SoDebugError::postWarning("SoOutput::setCompression",
                              "Requested BZIP2 compression, but libbz2 is not available.");
  }

  PRIVATE(this)->compmethod = SbName("NONE");
  PRIVATE(this)->complevel = 0.0f;

  if (compmethod == "NONE" || level == 0.0f) return TRUE;
  SoDebugError::postWarning("SoOutput::setCompression",
                            "Unsupported compression method: %s",
                            compmethod.getString());
  return FALSE;
}

/*!
  Returns the array of available compression methods. The number
  of elements in the array will be stored in \a num.

  \sa setCompression()
  \since Coin 2.1
 */
const SbName *
SoOutput::getAvailableCompressionMethods(unsigned int & num)
{
  SoOutput_compression_list_init();
  num = SoOutput_compmethods->getLength();
  return SoOutput_compmethods->getArrayPtr();
}

/*!
  Sets up a memory buffer of size \a initSize for writing.
  Writing will start at \a bufPointer + \a offset.

  If the buffer is filled up, \a reallocFunc is called to get more
  memory. If \a reallocFunc returns \a NULL, further writing is
  disabled.

  Important note: remember that the resultant memory buffer after
  write operations have completed may reside somewhere else in memory
  than on \a bufPointer if \a reallocFunc is set. It is a good idea to
  make it a habit to always use getBuffer() to retrieve the memory
  buffer pointer after write operations.

  Here's a complete, standalone usage example which shows how to
  write a scene graph to a memory buffer:

  \code
  #include <Inventor/SoDB.h>
  #include <Inventor/actions/SoWriteAction.h>
  #include <Inventor/nodes/SoCone.h>
  #include <Inventor/nodes/SoSeparator.h>

  static char * buffer;
  static size_t buffer_size = 0;

  static void *
  buffer_realloc(void * bufptr, size_t size)
  {
    buffer = (char *)realloc(bufptr, size);
    buffer_size = size;
    return buffer;
  }

  static SbString
  buffer_writeaction(SoNode * root)
  {
    SoOutput out;
    buffer = (char *)malloc(1024);
    buffer_size = 1024;
    out.setBuffer(buffer, buffer_size, buffer_realloc);

    SoWriteAction wa(&out);
    wa.apply(root);

    SbString s(buffer);
    free(buffer);
    return s;
  }

  int
  main(int argc, char ** argv)
  {
    SoDB::init();

    SoSeparator * root = new SoSeparator;
    root->ref();

    root->addChild(new SoCone);

    SbString s = buffer_writeaction(root);
    (void)fprintf(stdout, "%s\n", s.getString());

    root->unref();
    return 0;
  }
  \endcode

  \sa getBuffer(), getBufferSize(), resetBuffer()
*/
void
SoOutput::setBuffer(void * bufPointer, size_t initSize,
                    SoOutputReallocCB * reallocFunc, int32_t offset)
{
  this->reset();
  assert(initSize > 0 && "invalid argument");
  PRIVATE(this)->setWriter(new SoOutput_MemBufferWriter(bufPointer, initSize,
                                                        reallocFunc, offset));
}

/*!
  Returns the current buffer in \a bufPointer and the current
  write position of the buffer in \a nBytes. If we're writing into a
  file and not a memory buffer, \a FALSE is returned and the other return
  values will be undefined.

  \sa getBufferSize()
 */
SbBool
SoOutput::getBuffer(void *& bufPointer, size_t & nBytes) const
{
  if (PRIVATE(this)->getWriter()->getType() == SoOutput_Writer::MEMBUFFER) {
    SoOutput_MemBufferWriter * w = (SoOutput_MemBufferWriter*) PRIVATE(this)->getWriter();
    bufPointer = w->buf;
    nBytes = (size_t) w->offset;
    return TRUE;
  }
  return FALSE;
}

/*!
  Returns total size of memory buffer.

  \sa getBuffer()
 */
size_t
SoOutput::getBufferSize(void) const
{
  if (PRIVATE(this)->getWriter()->getType() == SoOutput_Writer::MEMBUFFER) {
    SoOutput_MemBufferWriter * w = (SoOutput_MemBufferWriter*) PRIVATE(this)->getWriter();
    return w->bufsize;
  }
  return 0;
}

/*!
  Reset the memory buffer write pointer back to the beginning of the
  buffer.
 */
void
SoOutput::resetBuffer(void)
{
  assert(this->isToBuffer());
  if (PRIVATE(this)->getWriter()->getType() == SoOutput_Writer::MEMBUFFER) {
    SoOutput_MemBufferWriter * w = (SoOutput_MemBufferWriter*) PRIVATE(this)->getWriter();
    w->offset = w->startoffset;
  }
}

/*!
  Set whether or not to write the output as a binary stream.

  \sa isBinary()
 */
// FIXME: write doc on endianness, netformat etc -- best thing would
// be to document the format completely in BNF. 19990627 mortene.
void
SoOutput::setBinary(const SbBool flag)
{
  PRIVATE(this)->binarystream = flag;
}

/*!
  Returns a flag which indicates whether or not we're writing the output
  as a binary stream.

  \sa setBinary()
 */
SbBool
SoOutput::isBinary(void) const
{
  return PRIVATE(this)->binarystream;
}

/*!
  Set the output file header string.

  \sa resetHeaderString(), getDefaultASCIIHeader(), getDefaultBinaryHeader()
 */
void
SoOutput::setHeaderString(const SbString & str)
{
  if (PRIVATE(this)->headerstring) *(PRIVATE(this)->headerstring) = str;
  else PRIVATE(this)->headerstring = new SbString(str);
}

/*!
  Reset the header string to the default one.

  \sa setHeaderString(), getDefaultASCIIHeader(), getDefaultBinaryHeader()
 */
void
SoOutput::resetHeaderString(void)
{
  delete PRIVATE(this)->headerstring;
  PRIVATE(this)->headerstring = NULL;
}

/*!
  Return the default header string written to ASCII files.

  \sa setHeaderString(), resetHeaderString(), getDefaultBinaryHeader()
 */
SbString
SoOutput::getDefaultASCIIHeader(void)
{
  return SbString("#Inventor V2.1 ascii");
}

/*!
  Return the default header string written to binary files.

  \sa setHeaderString(), resetHeaderString(), getDefaultASCIIHeader()
 */
SbString
SoOutput::getDefaultBinaryHeader(void)
{
  return SbString("#Inventor V2.1 binary");
}

/*!
  Set the precision used when writing floating point numbers to ASCII
  files. \a precision should be between 0 and 8.  The double precision
  will be set to \a precision * 2.
*/
void
SoOutput::setFloatPrecision(const int precision)
{
  const int fltnum = SbClamp(precision, 0, 8);
  const int dblnum = precision * 2;

  PRIVATE(this)->fltprecision.sprintf("%%.%dg", fltnum);
  PRIVATE(this)->dblprecision.sprintf("%%.%dlg", dblnum);
}

/*!
  Sets an indicator on the current stage. This is necessary to do as writing
  has to be done in multiple stages to account for the export of
  references/connections within the scene graphs.

  This method is basically just used from within SoWriteAction, and
  should usually not be of interest to the application programmer. Do
  not use it unless you \e really know what you are doing.

  \sa getStage()
*/
void
SoOutput::setStage(Stage stage)
{
  PRIVATE(this)->stage = stage;
}

/*!
  Returns an indicator on the current write stage. Writing is done in two
  passes, one to count and check connections, one to do the actual ASCII or
  binary export of data.

  You should not need to use this method, as it is meant for internal
  purposes in Coin.

  \sa setStage()
*/
SoOutput::Stage
SoOutput::getStage(void) const
{
  return PRIVATE(this)->stage;
}


/*!
  Write the character in \a c.

  For binary write, the character plus 3 padding zero characters will be
  written.
 */
void
SoOutput::write(const char c)
{
  this->writeBytesWithPadding(&c, 1);
}

/*!
  Write the character string pointed to by \a s.

  For binary write, a 4-byte MSB-ordered integer with the string length,
  plus the string plus padding zero characters to get on a 4-byte boundary
  (if necessary) will be written.
 */
void
SoOutput::write(const char * s)
{
  const size_t writelen = strlen(s);
  if (this->isBinary()) { this->write((int)writelen); }
  this->writeBytesWithPadding(s, writelen);
}

/*!
  Write the character string in \a s. The string will be written with
  apostrophes. Cast SbString to char * to write without apostrophes.

  If we are supposed to write in binary format, no apostrophes will be
  added, and writing will be done in the exact same manner as with
  SoOutput::write(const char * s).
 */
void
SoOutput::write(const SbString & s)
{
  if (this->isBinary()) {
    this->write(s.getString());
  }
  else {
    // Backslash-quote all apostrophe-characters, i.e. " -> \".
    //
    // Note that VRML97 also needs backslashes themselves to be
    // backslash-quoted (like in e.g. C strings), but this is taken
    // care of upstream (in SoSFString's write method), since we can't
    // know here whether or not we're writing a VRML97 node.
    //
    // FIXME: SbString should have had a replaceAll() method, so we
    // wouldn't have to spell out the iteration loop below. 20040614 mortene.
    SbString ws("\"");
    for (int i=0;i<s.getLength();i++) {
      if (s[i] == '"') ws += "\\";
      ws += s[i];
    }
    ws += "\"";
    this->write(ws.getString());
  }
}

/*!
  Write the character string in \a n. The name will be enclosed by
  apostrophes. If you want to write an SbName instance without the
  apostrophes, cast the argument to a char *.

  If we are supposed to write in binary format, no apostrophes will be
  added, and writing will be done in the exact same manner as with
  SoOutput::write(const char * s).
 */
void
SoOutput::write(const SbName & n)
{
  // Simply use SoOutput::write(const SbString &).
  SbString s(n.getString());
  this->write(s);
}

/*!
  Write \a i as a character string, or as an architecture independent binary
  pattern if the setBinary() flag is activated.
 */
void
SoOutput::write(const int i)
{
  if (!this->isBinary()) {
    // Use portable locale, to make sure we don't write thousands
    // separators for integers.
    cc_string storedlocale;
    SbBool changed = coin_locale_set_portable(&storedlocale);

    SbString s;
    s.sprintf("%d", i);
    this->writeBytesWithPadding(s.getString(), s.getLength());

    if (changed) { coin_locale_reset(&storedlocale); }
  }
  else {
    // FIXME: breaks on 64-bit architectures, which is pretty
    // lame... 19990621 mortene.
    assert(sizeof(int) == sizeof(int32_t));
    int32_t val = i;
    this->writeBinaryArray(&val, 1);
  }
}

/*!
  Write \a i as a character string, or as an architecture independent binary
  pattern if the setBinary() flag is activated.
 */
void
SoOutput::write(const unsigned int i)
{
  if (!this->isBinary()) {
    SbString s;
    s.sprintf("0x%x", i);
    this->writeBytesWithPadding(s.getString(), s.getLength());
  }
  else {
    assert(sizeof(i) == sizeof(int32_t));
    char buff[sizeof(i)];
    this->convertInt32((int32_t)i, buff);
    this->writeBytesWithPadding(buff, sizeof(i));
  }
}

/*!
  Write \a s as a character string, or as an architecture independent binary
  pattern if the setBinary() flag is activated.
 */
void
SoOutput::write(const short s)
{
  if (!this->isBinary()) {
    // Use portable locale, to make sure we don't write thousands
    // separators for integers.
    cc_string storedlocale;
    SbBool changed = coin_locale_set_portable(&storedlocale);

    SbString str;
    str.sprintf("%hd", s);
    this->writeBytesWithPadding(str.getString(), str.getLength());

    if (changed) { coin_locale_reset(&storedlocale); }
  }
  else {
    this->write((int)s);
  }
}

/*!
  Write \a s as a character string, or as an architecture independent binary
  pattern if the setBinary() flag is activated. If we're writing in ASCII
  format, the value will be written in base 16 (hexadecimal).
 */
void
SoOutput::write(const unsigned short s)
{
  if (!this->isBinary()) {
    SbString str;
    str.sprintf("0x%hx", s);
    this->writeBytesWithPadding(str.getString(), str.getLength());
  }
  else {
    this->write((unsigned int)s);
  }
}

/*!
  Write \a f as a character string.
 */
void
SoOutput::write(const float f)
{
  if (!this->isBinary()) {
    // Use portable locale, to make sure we don't write thousands
    // separators for integers.
    cc_string storedlocale;
    SbBool changed = coin_locale_set_portable(&storedlocale);

    SbString s;
    s.sprintf(PRIVATE(this)->fltprecision.getString(), f);
    
    // make sure scientific exponential is written in a platform independent way
    // always with three digits
    int pos = s.find("e");
    if(pos>0)
      {
	SbString exponential;
	exponential.sprintf("%03d",atoi(s.getSubString(pos+2).getString()));
	s=s.getSubString(0,pos+1)+exponential;

      }

    this->writeBytesWithPadding(s.getString(), s.getLength());

    if (changed) { coin_locale_reset(&storedlocale); }
  }
  else {
    char buff[sizeof(f)];
    this->convertFloat(f, buff);
    this->writeBytesWithPadding(buff, sizeof(f));
  }
}

/*!
  Write \a d as a character string.
 */
void
SoOutput::write(const double d)
{
  if (!this->isBinary()) {
    // Use portable locale, to make sure we don't write thousands
    // separators for integers.
    cc_string storedlocale;
    SbBool changed = coin_locale_set_portable(&storedlocale);

    SbString s;
    s.sprintf(PRIVATE(this)->dblprecision.getString(), d);

    // make sure scientific exponential is written in a platform independent way
    // always with three digits
    int pos = s.find("e");
    if(pos>0)
      {
	SbString exponential;
	exponential.sprintf("%03d",atoi(s.getSubString(pos+2).getString()));
	s=s.getSubString(0,pos+1)+exponential;
      }


    this->writeBytesWithPadding(s.getString(), s.getLength());

    if (changed) { coin_locale_reset(&storedlocale); }
  }
  else {
    char buff[sizeof(d)];
    this->convertDouble(d, buff);
    this->writeBytesWithPadding(buff, sizeof(d));
  }
}

#ifdef __CYGWIN__
void
SoOutput::write(long int i)
{
  static_assert(sizeof(long int) == sizeof(int), "long int size must match int on Cygwin");
  write(static_cast<int>(i));
}

void
SoOutput::write(unsigned long int i)
{
  static_assert(sizeof(unsigned long int) == sizeof(unsigned int), "unsigned long int size must match unsigned int on Cygwin");
  write(static_cast<unsigned int>(i));
}
#endif //__CYGWIN__

/*!
  Write the given number of bytes to either a file or a memory buffer in
  binary format.
 */
void
SoOutput::writeBinaryArray(const unsigned char * constc, const int length)
{
  // Note: the "length" argument should really have been "size_t", but
  // this is in the public API, so I've chosen to keep it as this (for
  // now).  -mortene.

  if (PRIVATE(this)->disabledwriting) return;

  this->checkHeader();

  size_t wrote = PRIVATE(this)->getWriter()->write((const char*) constc,
                                                   (size_t) length,
                                                   PRIVATE(this)->binarystream);
  if (wrote != (size_t)length) {
    SoDebugError::postWarning("SoOutput::writeBinaryArray",
                              "Couldn't write to file/memory buffer");
    PRIVATE(this)->disabledwriting = TRUE;
  }
}

/*!
  Write an \a length array of int32_t values in binary format.
 */
void
SoOutput::writeBinaryArray(const int32_t * const l, const int length)
{
  // Slooooow. We can do much better by using convertInt32Array().

  char val[sizeof(int32_t)];
  for (int i=0; i < length; i++) {
    this->convertInt32(l[i], val);
    this->writeBytesWithPadding(val, sizeof(int32_t));
  }
}

/*!
  Write an array of float values in binary format.
 */
void
SoOutput::writeBinaryArray(const float * const f, const int length)
{
  // Slooooow. We can do much better by using convertFloatArray().

  char val[sizeof(float)];
  for (int i=0; i < length; i++) {
    this->convertFloat(f[i], val);
    this->writeBytesWithPadding(val, sizeof(float));
  }
}

/*!
  Write an array of double values in binary format.
 */
void
SoOutput::writeBinaryArray(const double * const d, const int length)
{
  // Slooooow. We can do much better by using convertDoubleArray().

  char val[sizeof(double)];
  for (int i=0; i < length; i++) {
    this->convertDouble(d[i], val);
    this->writeBytesWithPadding(val, sizeof(double));
  }
}

/*!
  Increase indentation level in the file.

  \sa decrementIndent(), indent()
 */
void
SoOutput::incrementIndent(const int levels)
{
  PRIVATE(this)->indentlevel += levels;
}

/*!
  Decrease indentation level in the file.

  \sa incrementIndent(), indent()
 */
void
SoOutput::decrementIndent(const int levels)
{
  PRIVATE(this)->indentlevel -= levels;
#if COIN_DEBUG
  if (PRIVATE(this)->indentlevel < 0) {
    SoDebugError::postInfo("SoOutput::decrementIndent",
                           "indentation level < 0!");
    PRIVATE(this)->indentlevel = 0;
  }
#endif // COIN_DEBUG
}

/*!
  Call this method after writing a newline to a file to indent the next
  line to the correct position.

  \sa incrementIndent(), decrementIndent()
 */
void
SoOutput::indent(void)
{
#if COIN_DEBUG
  if (this->isBinary()) {
    SoDebugError::postWarning("SoOutput::indent",
                              "Don't try to indent when you're doing binary "
                              "format output.");
    return;
  }
#endif // COIN_DEBUG

  if (PRIVATE(this)->writecompact) return;

  static int oldstyle = -1;
  if (oldstyle == -1) {
    oldstyle = coin_getenv("COIN_OLDSTYLE_FORMATTING") ? 1 : 0;
  }

  // Keep the old ugly-bugly formatting style around, in case someone,
  // for some obscure reason, needs it.
  if (oldstyle) {
    int i = PRIVATE(this)->indentlevel;
    while (i > 1) {
      this->write('\t');
      i -= 2;
    }

    if (i == 1) this->write("  ");
  }
  // More sensible formatting.
  else {
    for (int i=0; i < PRIVATE(this)->indentlevel; i++) { this->write("  "); }
  }
}

/*!
  Reset all value and make ready for using another file pointer or buffer.
*/
void
SoOutput::reset(void)
{
  this->closeFile();

  while (PRIVATE(this)->routestack.getLength()) {
    delete PRIVATE(this)->routestack[0];
    PRIVATE(this)->routestack.removeFast(0);
  }
  PRIVATE(this)->routestack.append(NULL);

  PRIVATE(this)->protostack.truncate(0);
  while (PRIVATE(this)->defstack.getLength()) {
    delete PRIVATE(this)->defstack[0];
    PRIVATE(this)->defstack.removeFast(0);
  }
  PRIVATE(this)->defstack.append(NULL);

  PRIVATE(this)->disabledwriting = FALSE;
  this->wroteHeader = FALSE;
  PRIVATE(this)->indentlevel = 0;
}

/*!
  Set up the output to be more compact than with the default write routines.
*/
void
SoOutput::setCompact(SbBool flag)
{
  // FIXME: go through output code and make the output more
  // compact. 19990623 morten.
#if COIN_DEBUG
  if (!PRIVATE(this)->writecompact && flag) {
    SoDebugError::postWarning("SoOutput::setCompact",
                              "compact export is not implemented in Coin yet");
  }
#endif // COIN_DEBUG

  PRIVATE(this)->writecompact = flag;
}

/*!
  Returns whether or not the write routines try to compact the data when
  writing it (i.e. using less whitespace, etc).

  Note that "compact" in this sense does \e not mean "bitwise compression",
  as it could easily be mistaken for.
*/
SbBool
SoOutput::isCompact(void) const
{
  return PRIVATE(this)->writecompact;
}

/*!
  Set up annotation of different aspects of the output data. This is not
  useful for much else than debugging purposes, I suppose.
*/
void
SoOutput::setAnnotation(uint32_t bits)
{
  // FIXME: go through output code and insert annotations where applicable.
  // 19990623 morten.
#if COIN_DEBUG
  if (PRIVATE(this)->annotationbits != bits) {
    SoDebugError::postWarning("SoOutput::setAnnotation",
                              "annotated export is not implemented in Coin "
                              "yet");
  }
#endif // COIN_DEBUG

  PRIVATE(this)->annotationbits = bits;
}

/*!
  Returns the current annotation debug bitflag settings.
*/
uint32_t
SoOutput::getAnnotation(void)
{
  return PRIVATE(this)->annotationbits;
}

/*!
  Check that the current memory buffer has enough space to contain the
  given number of bytes needed for the next write operation.

  Returns \a FALSE if there's not enough space left, otherwise \a TRUE.

  Note that there will automatically be made an attempt at allocating
  more memory if the reallocation callback function argument of
  setBuffer() was not \a NULL.
*/
SbBool
SoOutput::makeRoomInBuf(size_t bytes)
{
  assert(PRIVATE(this)->getWriter()->getType() == SoOutput_Writer::MEMBUFFER);

  SoOutput_MemBufferWriter * w =
    (SoOutput_MemBufferWriter*) PRIVATE(this)->getWriter();

  return w->makeRoomInBuf(bytes);
}

/*!
  \COININTERNAL

  Write the given number of bytes from the array, pad with zeros to get
  on a 4-byte boundary if file format is binary.
*/
void
SoOutput::writeBytesWithPadding(const char * const p, const size_t nr)
{
  this->writeBinaryArray((const unsigned char *)p, (int)nr);

  // Pad binary writes to a 4-byte boundary if necessary.
  if (this->isBinary()) {
    // Static buffer filled with enough bytes of all-zero bits.
    static unsigned char padbytes[HOSTWORDSIZE] = "X";
    if (padbytes[0] == 'X')
      for (size_t i=0; i < HOSTWORDSIZE; i++) padbytes[i] = '\0';

    size_t writeposition = this->bytesInBuf();
    if (PRIVATE(this)->getWriter()->getType() == SoOutput_Writer::MEMBUFFER) {
      writeposition -= ((SoOutput_MemBufferWriter*)PRIVATE(this)->getWriter())->startoffset;
    }
    size_t padsize = HOSTWORDSIZE - (writeposition % HOSTWORDSIZE);
    if (padsize == HOSTWORDSIZE) padsize = 0;
    this->writeBinaryArray(padbytes, (int)padsize);
  }
}

/*!
  \COININTERNAL

  If the file header hasn't been written yet, write it out now.
*/
void
SoOutput::checkHeader(void)
{
  if (!this->wroteHeader) {
    // NB: this flag _must_ be set before we do any writing, or we'll
    // end up in an eternal double-recursive loop.
    this->wroteHeader = TRUE;

    SbString h;
    if (PRIVATE(this)->headerstring) h = *(PRIVATE(this)->headerstring);
    else if (this->isBinary()) h = SoOutput::getDefaultBinaryHeader();
    else h = SoOutput::getDefaultASCIIHeader();

    if (this->isBinary()) h = this->padHeader(h);
    h += EOLSTR;
    if (!this->isBinary()) h += EOLSTR;
    // Note: SoField::get() and SoFieldContainer::get() depends on the
    // fact that the header identification line ends in "\n\n".

    // Write as char * to avoid the addition of any "s.
    const unsigned char * str = (const unsigned char *)h.getString();
    const size_t len = strlen(h.getString());
    this->writeBinaryArray(str, (int)len);
  }
}

/*!
  Returns \a TRUE if we're set up to write to a memory buffer.
*/
SbBool
SoOutput::isToBuffer(void) const
{
  return PRIVATE(this)->getWriter()->getType() == SoOutput_Writer::MEMBUFFER;
}

/*!
  Returns current write position.

  Note that for memory buffer writing, this includes the offset from
  SoOutput::setBuffer(), if any.
*/
size_t
SoOutput::bytesInBuf(void) const
{
  return PRIVATE(this)->getWriter()->bytesInBuf();

//   if (this->isToBuffer()) { return PRIVATE(this)->bufferoffset; }
//   else { return ftell(PRIVATE(this)->filep); }
}

/*!
  Makes a unique id for \a base and adds a mapping into our dictionary.
*/
int
SoOutput::addReference(const SoBase * base)
{
  return PRIVATE(this)->counter->addReference(base);
}

/*!
  Returns the unique identifier for \a base or -1 if not found.
*/
int
SoOutput::findReference(const SoBase * base) const
{
  return PRIVATE(this)->counter->findReference(base);
}

/*!
  Sets the reference for \a base manually.
*/
void
SoOutput::setReference(const SoBase * base, int refid)
{
  PRIVATE(this)->counter->setReference(base, refid);
}

/*!
  Adds \a name to the set of currently DEF'ed node names so far in the output
  process.
*/
void
SoOutput::addDEFNode(SbName name)
{
  void * value = NULL;
  BogusSet * defnames = PRIVATE(this)->getCurrentDefNames(TRUE);
  defnames->put(name.getString(), value);
}

/*!
  Checks whether \a name is already DEF'ed at this point in the output process.
  Returns TRUE if \a name is DEF'ed.
*/
SbBool
SoOutput::lookupDEFNode(SbName name)
{
  void * value;
  BogusSet * defnames = PRIVATE(this)->getCurrentDefNames(TRUE);
  return defnames->get(name.getString(), value);
}

/*!
  Removes \a name from the set of DEF'ed node names. Used after the last
  reference to a DEF'ed node if we want to reuse the DEF at a later point
  in the file.
*/
void
SoOutput::removeDEFNode(SbName name)
{
  BogusSet * defnames = PRIVATE(this)->getCurrentDefNames(FALSE);
  assert(defnames);
#if COIN_DEBUG
  size_t ret = defnames->erase(name.getString());
  assert(ret && "Tried to remove nonexisting DEFnode");
#else
  (void)defnames->erase(name.getString());
#endif
}

/*!
  \COININTERNAL

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
void
SoOutput::pushProto(SoProto * proto)
{
  // FIXME: try to find a better/nicer way to handle PROTO export without
  // adding new methods in SoOutput. For instance, is it possible to
  // add elements in the SoWriteAction state stack? pederb, 2002-06-12

  PRIVATE(this)->pushRoutes(FALSE);
  PRIVATE(this)->protostack.push(proto);
  PRIVATE(this)->pushDefNames(FALSE);
}

/*!
  \COININTERNAL

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SoProto *
SoOutput::getCurrentProto(void) const
{
  // FIXME: try to find a better/nicer way to handle PROTO export without
  // adding new methods in SoOutput. For instance, is it possible to
  // add elements in the SoWriteAction state stack? pederb, 2002-06-12

  if (PRIVATE(this)->protostack.getLength()) {
    return PRIVATE(this)->protostack[PRIVATE(this)->protostack.getLength()-1];
  }
  return NULL;
}

/*!
  \COININTERNAL

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
void
SoOutput::popProto(void)
{
  // FIXME: try to find a better/nicer way to handle PROTO export without
  // adding new methods in SoOutput. For instance, is it possible to
  // add elements in the SoWriteAction state stack? pederb, 2002-06-12

  assert(PRIVATE(this)->protostack.getLength());
  PRIVATE(this)->protostack.pop();
  PRIVATE(this)->popDefNames();
  PRIVATE(this)->popRoutes();
}

/*!
  \COININTERNAL

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/

void
SoOutput::addRoute(SoFieldContainer * from, const SbName & fromfield,
                   SoFieldContainer * to, const SbName & tofield)
{
  SoOutputROUTEList * list = PRIVATE(this)->getCurrentRoutes(TRUE);
  assert(list);
  SoOutputROUTE r;
  r.from = from;
  r.fromfield = fromfield;
  r.to = to;
  r.tofield = tofield;
  list->append(r);
}

/*!
  \COININTERNAL

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
void
SoOutput::resolveRoutes(void)
{
  // FIXME: try to find a better/nicer way to handle ROUTE export without
  // adding new methods in SoOutput. For instance, is it possible to
  // add elements in the SoWriteAction state stack? pederb, 2002-06-12

  SoOutputROUTEList * list = PRIVATE(this)->getCurrentRoutes(FALSE);
  if (list && list->getLength()) {
    const int n = list->getLength();
    for (int i = 0; i < n; i++) {
      SoOutputROUTE r = (*list)[i];

      SoFieldContainer * fromc = r.from;
      SoFieldContainer * toc = r.to;

      SbName fromname = r.fromfield;
      SbName toname = r.tofield;

      this->indent();
      this->write("ROUTE ");
      this->write(PRIVATE(this)->counter->getWriteName(fromc).getString());
      this->write('.');
      this->write(fromname.getString());
      this->write(" TO ");
      this->write(PRIVATE(this)->counter->getWriteName(toc).getString());
      this->write('.');
      this->write(toname.getString());
      this->write("\n");

#if COIN_DEBUG
      if (SoWriterefCounter::debugWriterefs()) {
        int writerefcount = PRIVATE(this)->counter->getWriteref(fromc);
        SoDebugError::postInfo("SoOutput::resolveRoutes",
                               "%p/%s/'%s': %d -> %d",
                               fromc,
                               fromc->getTypeId().getName().getString(),
                               fromc->getName().getString(),
                               writerefcount, writerefcount - 1);
      }
#endif // COIN_DEBUG

#if COIN_DEBUG
      if (SoWriterefCounter::debugWriterefs()) {
        int writerefcount = PRIVATE(this)->counter->getWriteref(toc);
        SoDebugError::postInfo("SoOutput::resolveRoutes",
                               "%p/%s/'%s': %d -> %d",
                               toc,
                               toc->getTypeId().getName().getString(),
                               toc->getName().getString(),
                               writerefcount, writerefcount - 1);
      }
#endif // COIN_DEBUG

      // remove write references again
      PRIVATE(this)->counter->decrementWriteref(fromc);
      PRIVATE(this)->counter->decrementWriteref(toc);

    }
    list->truncate(0);
  }
}

/*!
  Convert the short integer in \a s to most-significant-byte first format
  and put the resulting bytes sequentially at \a to.

  \sa SoInput::convertShort()
*/
void
SoOutput::convertShort(short s, char * to)
{
  assert(sizeof(s) == sizeof(uint16_t));
  *((uint16_t *)to) = coin_hton_uint16((uint16_t)s);
}

/*!
  Convert the 32-bit integer in \a l to most-significant-byte first format
  and put the resulting bytes sequentially at \a to.

  \sa SoInput::convertInt32()
*/
void
SoOutput::convertInt32(int32_t l, char * to)
{
  assert(sizeof(l) == sizeof(uint32_t));
  *((uint32_t *)to) = coin_hton_uint32(l);
}

/*!
  Convert the single-precision floating point number in \a f to
  most-significant-byte first format and put the resulting bytes
  sequentially at \a to.

  \sa SoInput::convertFloat()
*/
void
SoOutput::convertFloat(float f, char * to)
{
  coin_hton_float_bytes(f, to);
}

/*!
  Convert the double-precision floating point number in \a d to
  most-significant-byte first format and put the resulting bytes
  sequentially at \a to.

  \sa SoInput::convertDouble()
*/
void
SoOutput::convertDouble(double d, char * to)
{
  coin_hton_double_bytes(d, to);
}

/*!
  Convert \a len short integer values from the array at \a from into
  the array at \a to from native host format to network independent
  format (i.e. most significant byte first).
*/
void
SoOutput::convertShortArray(short * from, char * to, int len)
{
  for (int i=0; i < len; i++) {
    this->convertShort(*from++, to);
    to += sizeof(short);
  }
}

/*!
  Convert \a len 32-bit integer values from the array at \a from into
  the array at \a to from native host format to network independent
  format (i.e. most significant byte first).
*/
void
SoOutput::convertInt32Array(int32_t * from, char * to, int len)
{
  for (int i=0; i < len; i++) {
    this->convertInt32(*from++, to);
    to += sizeof(int32_t);
  }
}

/*!
  Convert \a len single-precision floating point values from the array at
  \a from into the array at \a to from native host format to network
  independent format (i.e. most significant byte first).
*/
void
SoOutput::convertFloatArray(float * from, char * to, int len)
{
  for (int i=0; i < len; i++) {
    this->convertFloat(*from++, to);
    to += sizeof(float);
  }
}

/*!
  Convert \a len double-precision floating point values from the array at
  \a from into the array at \a to from native host format to network
  independent format (i.e. most significant byte first).
*/
void
SoOutput::convertDoubleArray(double * from, char * to, int len)
{
  for (int i=0; i < len; i++) {
    this->convertDouble(*from++, to);
    to += sizeof(double);
  }
}

/*!
  Pads the header we're writing so it contains the correct amount of bytes
  for the alignment of the following binary writes.
*/
SbString
SoOutput::padHeader(const SbString & inString)
{
  SbString h = inString;
  const size_t EOLLEN = strlen(EOLSTR);
  int hlen = h.getLength();
  size_t pad = HOSTWORDSIZE - ((hlen + EOLLEN) % HOSTWORDSIZE);
  pad = pad == HOSTWORDSIZE ? 0 : pad;
  for (size_t i=0; i < pad; i++) h += ' ';

  return h;
}

//
// Used only by SoBase::writeHeader().
//
void
SoOutput::removeSoBase2IdRef(const SoBase * base)
{
  PRIVATE(this)->counter->removeSoBase2IdRef(base);
}

// FIXME: temporary workaround needed to test if we are currently
// exporting a VRML97 or an Inventor file. Used from
// SoBase::writeHeader(). pederb, 2003-02-18
SbString
SoOutput_getHeaderString(const SoOutputP * pout)
{
  if (pout->headerstring) return *(pout->headerstring);
  else return SoOutput::getDefaultASCIIHeader();
}

#undef PRIVATE
