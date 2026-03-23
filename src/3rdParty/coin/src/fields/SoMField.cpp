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
  \class SoMField SoMField.h Inventor/fields/SoMField.h
  \brief The SoMField class is the base class for fields which can contain multiple values.

  \ingroup coin_fields

  All field types which may contain more than one member
  value inherits this class. SoMField is an abstract class.

  Use setValue(), setValues() or set1Value() to set the values of
  fields inheriting SoMField, and use getValues() or the index
  operator [] to read values. Example code:

  \code
    SoText2 * textnode = new SoText2;
    textnode->ref();

    // Setting multi-field values. /////////////////////////////

    // Set the first value of the SoMFString field of the SoText2 node.
    // The field array will be truncated to only contain this single value.
    textnode->string.setValue("Morten");
    // Full contents of the SoMFString is: [ "Morten" ]

    // The setValue() method and the = operator is interchangeable,
    // so this code line does the same as the previous line.
    textnode->string = "Peder";
    // Full contents of the SoMFString is now: [ "Peder" ]

    // Set the value at index 2. If the field value array contained
    // less than 3 elements before this call, first expand it to contain
    // 3 elements.
    textnode->string.set1Value(2, "Lars");
    // Full contents of the SoMFString is: [ "Peder", <undefined>, "Lars" ]

    // This sets 3 values of the array, starting at index 5. If the
    // array container had less than 8 elements before the setValues()
    // call, the array will first be expanded.
    SbString s[3] = { "Eriksen", "Blekken", "Aas" };
    textnode->string.setValues(5, sizeof(s) / sizeof(s[0]), s);
    // Full contents of the SoMFString is now:
    //     [ "Peder", <undefined>, "Lars", <undefined>, <undefined>,
    //       "Eriksen", "Blekken", "Aas" ]

    // Note also that the setValues() call will *not* truncate a field
    // container if you use it to change a subset at the start:
    SbString n[4] = { "Dixon", "Adams", "Bould", "Winterburn" };
    textnode->string.setValues(0, sizeof(n) / sizeof(n[0]), n);
    // Full contents of the SoMFString is now:
    //     [ "Dixon", "Adams", "Bould", "Winterburn", <undefined>,
    //       "Eriksen", "Blekken", "Aas" ]


    // Inspecting multi-field values. //////////////////////////

    // This will read the second element (counting from zero) if the
    // multivalue field and place it in "val".
    SbString val = textnode->string[2];

    // Gives us a pointer to the array which the multiple-value field
    // is using to store the values. Note that the return value is
    // "const", so you can only read from the array, not write to
    // it.
    const SbString * vals = textnode->string.getValues(0);


    // Modifying multi-field values. ///////////////////////////

    // You can of course modify multifield-values by using the set-
    // and get-methods shown above, but when you're working with
    // big sets of data, this will be ineffective. Then use this
    // technique instead:
    SbString * modvals = textnode->string.startEditing();

    // ... lots of modifications to the "modvals" array here ...

    // Calling the finishEditing() method is necessary for the
    // scene graph to be updated (and re-rendered).
    textnode->string.finishEditing();

  \endcode

  The reason it is more effective to wrap many modifications within
  startEditing() / finishEditing() is because we avoid the stream of
  notification messages which would otherwise be sent for each and
  every modification done. Instead there will just be a single
  notification about the changes, triggered by the finishEditing()
  call.

  The correct manner in which to pre-allocate a specific number of
  field values in one chunk is to use the SoMField::setNum() method,
  for instance in advance of using the startEditing() and
  finishEditing() methods. The field values will be uninitialized
  after expanding the field with the setNum() call.

  Be aware that your application code must be careful to not do silly
  things during a setValues()-triggered notification. If you have code
  that looks for instance like this:

  \code
    // update set of coordinate indices at the start of e.g.
    // an SoIndexedFaceSet's coordIndex field..
    ifs->coordIndex.setValues(0, runner->numIndices, runner->indices);
    // ..then truncate to make sure it's the correct size.
    ifs->coordIndex.setNum(runner->numIndices);
  \endcode

  As setValues() might leave some elements at the end of the array
  that typically can be invalid indices after the first statement is
  executed, something can go wrong during notification if you have
  application code monitoring changes, and the application code then
  for instance triggers an action or something that tries to use the
  coordIndex field before it is updated to its correct size with the
  setNum() call.

  (Notification can in this case, as always, be temporarily disabled
  to be on the safe side:

  \code
  somefield.enableNotify(FALSE);
  somefield.setValues(...);
  somefield.setNum(...);
  somefield.enableNotify(TRUE);
  somefield.touch();
  \endcode

  This will guarantee that the setValues() and setNum() pair will be
  executed as an atomic operation.)


  When nodes, engines or other types of field containers are written
  to file, their multiple-value fields are written to file in this
  format:

  \code
    containerclass {
      fieldname [ value0, value1, value2, ...]
      ...
    }
  \endcode

  ..like this, for instance, a Coordinate3 node providing 6 vertex
  coordinates in the form of SbVec3f values in its "point" field for
  e.g. a faceset, lineset or pointset:

  \code
     Coordinate3 {
        point [
           -1 1 0, -1 -1 0, 1 -1 0,
           0 2 -1, -2 0 -1, 0 -2 -1,
        ]
     }
  \endcode

  Some fields support application data sharing through a
  setValuesPointer() method. setValuesPointer() makes it possible to
  set the data pointer directly in the field. Normally (when using
  setValues()), Coin makes a copy of your data, so this method can be
  very useful if your application needs the data internally and you're
  just using Coin for the visualization. Example code:

  \code

  myapp->calculateCoordinates(SOME_LARGE_VALUE);
  SbVec3f * mycoords = myapp->getCoordinates();

  SoCoordinate3 * mynode = myapp->getCoordinateNode();
  mynode->point.setValuesPointer(SOME_LARGE_VALUE, mycoords);

  \endcode

  Be aware that your field should be a read-only field when you set
  the data like this. If you write to the field, the values in your
  application array will be overwritten. If you append values to the
  field, a new array will be allocated, and the data will be copied
  into it before appending the new values. The array pointer will then
  be discarded.

  Also note that whenever you change some value(s) in the array, you
  must manually notify Coin about this by calling
  SoField::touch(). For our example:

  \code

  SbVec3f * mycoords = myapp->getCoordinate();
  myapp->updateCoordinate(mycoords);
  SoCoordinate3 * mynode = myapp->getCoordinateNode();
  mynode->point.touch(); // this will notify Coin that field has changed

  \endcode

  You can use SoMField::enableDeleteValues() to make Coin delete the
  array for you when the field is destructed or the array pointer is
  discarded because it isn't needed anymore (e.g. when the array size
  is changed). The array will be deleted using the C++ \e delete[]
  operator, so if you use it, your array must be allocated using the
  C++ \e new[] operator.

  SoMField::enableDeleteValues() is supported only to be compatible
  with later versions of TGS Inventor and we don't recommend using
  it. It can have undefined results on the Microsoft Windows
  platform. Allocating memory in the application and destructing it in
  a DLL can be a bad thing, causing mysterious crashes, if you're not
  very careful about how your application and DLLs are linked to the
  underlying C library.

  \sa SoSField
*/

// *************************************************************************

#include <Inventor/fields/SoMField.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoSubField.h>

#include "threads/threadsutilp.h"
#include "tidbitsp.h"
#include "coindefs.h" // COIN_WORKAROUND_*

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::memcpy;
using std::memset;
using std::strlen;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

/*!
  \fn int SoMField::getNum(void) const

  Returns number of values in this field.
*/

/*!
  \var int SoMField::num
  Number of available values.
*/
/*!
  \var int SoMField::maxNum
  Number of array "slots" allocated for this field.
*/
/*!
  \var SbBool SoMField::userDataIsUsed
  Is \c TRUE if data have been set through a setValuesPointer() call
  and set to \c FALSE through a enableDeleteValues() call.
*/

// *************************************************************************

SoType SoMField::classTypeId STATIC_SOTYPE_INIT;

// need one static mutex for field_buffer in SoMField::get1(SbString &)
static void * somfield_mutex = NULL;

static void
somfield_mutex_cleanup(void)
{
  CC_MUTEX_DESTRUCT(somfield_mutex);
}

// *************************************************************************


/*!
  \copydetails SoField::getClassTypeId(void)
*/
SoType
SoMField::getClassTypeId(void)
{
  return SoMField::classTypeId;
}

/*!
  \copydetails SoField::initClass(void)
*/
void
SoMField::initClass(void)
{
  PRIVATE_FIELD_INIT_CLASS(SoMField, "MField", inherited, NULL);

  CC_MUTEX_CONSTRUCT(somfield_mutex);
  coin_atexit(somfield_mutex_cleanup, CC_ATEXIT_NORMAL);
}

void
SoMField::atexit_cleanup(void)
{
  SoType::removeType(SoMField::classTypeId.getName());
  SoMField::classTypeId STATIC_SOTYPE_INIT;
}

/*!
  Constructor. Initializes number of values in field to zero.
*/
SoMField::SoMField(void)
{
  this->maxNum = this->num = 0;
  this->userDataIsUsed = FALSE;
}

/*!
  Destructor in SoMField does nothing. Resource deallocation needs to
  be done from subclasses.
*/
SoMField::~SoMField()
{
}

/*!
  Make room in the field to store \a newnum values.
*/
void
SoMField::makeRoom(int newnum)
{
  assert(newnum >= 0);
  if (newnum != this->num) this->allocValues(newnum);
}

/*!
  Set the value at \a index to the value contained in \a valuestring.
  Returns \c TRUE if a valid value for this field can be extracted
  from \a valuestring, otherwise \c FALSE.

  If \a index is larger than the current number of elements in the
  field, this method will automatically expand the field to accommodate
  the new value.
*/
SbBool
SoMField::set1(const int index, const char * const valuestring)
{
  int oldnum = this->num;
  // make sure the array has room for the new item
  if (index >= this->maxNum) this->allocValues(index+1);
  else if (index >= this->num) this->num = index+1;

  SoInput in;
  in.setBuffer(const_cast<char *>(valuestring), strlen(valuestring));
  if (!this->read1Value(&in, index)) {
    this->num = oldnum; // restore old number of items in field
    return FALSE;
  }
  this->setChangedIndex(index);
  this->valueChanged();
  this->setChangedIndices();
  return TRUE;
}

static void * mfield_buffer = NULL;
static size_t mfield_buffer_size = 0;

static void
mfield_buffer_cleanup(void)
{
  free(mfield_buffer);
  mfield_buffer = NULL;
  mfield_buffer_size = 0;
}

static void *
mfield_buffer_realloc(void * bufptr, size_t size)
{
  void * newbuf = realloc(bufptr, size);
  mfield_buffer = newbuf;
  mfield_buffer_size = size;
  return newbuf;
}

/*!
  Return the value at \a index in the \a valuestring string.
*/
void
SoMField::get1(const int index, SbString & valuestring)
{
  CC_MUTEX_LOCK(somfield_mutex); // need to lock since a static array is used

  // Note: this code has an almost verbatim copy in SoField::get(), so
  // remember to update both places if any fixes are done.

  // Initial buffer setup.
  SoOutput out;
  const size_t STARTSIZE = 32;
  // if buffer grow bigger than 1024 bytes, free memory
  // at end of method. Otherwise, just keep using the allocated
  // memory the next time this method is called.
  const size_t MAXSIZE = 1024;

  if (mfield_buffer_size < STARTSIZE) {
    mfield_buffer = malloc(STARTSIZE);
    mfield_buffer_size = STARTSIZE;
    coin_atexit(mfield_buffer_cleanup, CC_ATEXIT_NORMAL);
  }

  out.setBuffer(mfield_buffer, mfield_buffer_size,
                mfield_buffer_realloc);

  // Record offset to skip header.
  out.write("");
  size_t offset;
  void * buffer;
  out.getBuffer(buffer, offset);

  // Write field..
  out.setStage(SoOutput::COUNT_REFS);
  this->countWriteRefs(&out);
  out.setStage(SoOutput::WRITE);
  this->write1Value(&out, index);

  // ..then read it back into the SbString.
  size_t size;
  out.getBuffer(buffer, size);
  valuestring = static_cast<char *>(buffer) + offset;

  // check if buffer grew too big
  if (mfield_buffer_size >= MAXSIZE) {
    // go back to startsize
    (void) mfield_buffer_realloc(mfield_buffer, STARTSIZE);
  }
  CC_MUTEX_UNLOCK(somfield_mutex);
}

/*!
  Read and set all values for this field from input stream \a in.
  Returns \c TRUE if import went ok, otherwise \c FALSE.
*/
SbBool
SoMField::readValue(SoInput * in)
{
  // FIXME: temporary disable notification (if on) during reading the
  // field elements. 20000429 mortene.

  // This macro is convenient for reading with error detection.
#define READ_VAL(val) \
  if (!in->read(val)) { \
    SoReadError::post(in, "Premature end of file"); \
    return FALSE; \
  }

  // ** Binary format ******************************************************
  if (in->isBinary()) {
    int numtoread;
    READ_VAL(numtoread);

    // Sanity checking on the value, to avoid barfing on corrupt
    // files.
    if (numtoread < 0) {
      SoReadError::post(in, "invalid number of values in field: %d",
                        numtoread);
      return FALSE;
    }
#if 0 // tmp disabled until we come up with something better
    // FIXME: this limit is way too low. Not sure if a limit is a good
    // thing at all. 20000405 mortene.
    else if (numtoread > 32768) {
      SoReadError::post(in, "%d values in field, file probably corrupt",
                        numtoread);
      return FALSE;
    }
#endif // disabled

    this->makeRoom(numtoread);
    if (!this->readBinaryValues(in, numtoread)) { return FALSE; }
  }

  // ** ASCII format *******************************************************
  else {
    char c;
    READ_VAL(c);
    if (c == '[') {
      int currentidx = 0;

      READ_VAL(c);
      if (c == ']') {
        // Zero values -- done. :^)
        this->makeRoom(0);
      }
      else {
        in->putBack(c);

        while (TRUE) {
          // makeRoom() makes sure the allocation strategy is decent.
          if (currentidx >= this->num) this->makeRoom(currentidx + 1);

          if (!this->read1Value(in, currentidx++)) return FALSE;

          READ_VAL(c);
          if (c == ',') { READ_VAL(c); } // Treat trailing comma as whitespace.

          // That was the last array element, we're done.
          if (c == ']') { break; }

          if (c == '}') {
            SoReadError::post(in, "Premature end of array, got '%c'", c);
            return FALSE;
          }

          in->putBack(c);
        }
      }

      // Fit array to number of items.
      this->makeRoom(currentidx);
    }
    else {
      in->putBack(c);
      this->makeRoom(1);
      if (!this->read1Value(in, 0)) return FALSE;
    }
  }

#undef READ_VAL

  // We need to trigger the notification chain here, as this function
  // can be used on a node in a scene graph in any state -- not only
  // during initial scene graph import.
  //
  // FIXME: this might cause major slowdowns at import, and we should
  // probably disable notification at the container level during full
  // scene graph import operations (probably best done from somewhere
  // in SoBase::readInstance() or some such). Should investigate.
  // 20031203 mortene.
  this->valueChanged();

  return TRUE;
}

/*!
  Write all field values to \a out.
*/
void
SoMField::writeValue(SoOutput * out) const
{
  if (out->isBinary()) {
    this->writeBinaryValues(out);
    return;
  }

  const int count = this->getNum();
  if ((count > 1) || (count == 0)) out->write("[ ");

  out->incrementIndent();

  for (int i=0; i < count; i++) {
    this->write1Value(out, i);

    if (i != count-1) {
      if (((i+1) % this->getNumValuesPerLine()) == 0) {
        out->write(",\n");
        out->indent();
        // for alignment
        out->write("  ");
      }
      else {
        out->write(", ");
      }
    }
  }
  if ((count > 1) || (count == 0)) out->write(" ]");

  out->decrementIndent();
}

/*!
  Read \a num binary format values from \a in into this field.
*/
SbBool
SoMField::readBinaryValues(SoInput * in, int numarg)
{
  assert(in->isBinary());
  assert(numarg >= 0);

  for (int i=0; i < numarg; i++) if (!this->read1Value(in, i)) return FALSE;
  return TRUE;
}

/*!
  Write all values of field to \a out in binary format.
*/
void
SoMField::writeBinaryValues(SoOutput * out) const
{
  assert(out->isBinary());

  const int count = this->getNum();
  out->write(count);
  for (int i=0; i < count; i++) this->write1Value(out, i);
}

// Number of values written to each line during export to ASCII format
// files. Override this in subclasses for prettier formatting.
int
SoMField::getNumValuesPerLine(void) const
{
  return 1;
}

/*!
  Set number of values to \a num.

  If the current number of values is larger than \a num, the array of
  values will be truncated from the end. But if \a num is larger, the
  array will automatically be expanded (and random values will be set
  for the new array items).
 */
void
SoMField::setNum(const int numarg)
{
  // Don't use getNum(), as that could trigger evaluate(), which is
  // _not_ supposed to be called recursively (which means setNum()
  // wouldn't have been available from within an evaluate() session).
  int oldnum = this->num;

  // Note: this method is implemented in terms of the virtual methods
  // deleteValues() and insertSpace() so the more "complex" fields
  // (like SoMFNode and SoMFEngine) can also handle setNum() in a
  // correct way.

  if (numarg < oldnum) {
    this->deleteValues(numarg, -1);
    // deleteValues() also handles notification.
  }
  else if (numarg > oldnum) {
    this->insertSpace(oldnum, numarg - oldnum);
    // insertSpace() also handles notification.
  }
  // else no change.
}

/*!
  Remove value elements from index \a start up to and including index
  \a start + \a num - 1.

  Elements with indices larger than the last deleted element will
  be moved downwards in the value array.

  If \a num equals -1, delete from index \a start and to the end of
  the array.
*/
void
SoMField::deleteValues(int start, int numarg)
{
  // Note: this function is overridden in SoMFNode, SoMFEngine and
  // SoMFPath, so if you do any changes here, take a look at those
  // methods as well (they are collected in the common template
  // MFNodeEnginePath.tpl).

  // Don't use getNum(), so we avoid recursive evaluate() calls.
  int oldnum = this->num;

  if (numarg == -1) numarg = oldnum - start;
  if (numarg == 0) return;
  int end = start + numarg; // First element behind the delete block.

#if COIN_DEBUG
  if (start < 0 || start >= oldnum || end > oldnum || numarg < -1) {
    SoDebugError::post("SoMField::deleteValues",
                       "invalid indices [%d, %d] for array of size %d",
                       start, end - 1, oldnum);
    return;
  }
#endif // COIN_DEBUG

  // Move elements downward to fill the gap.
  for (int i = 0; i < oldnum-(start+numarg); i++)
    this->copyValue(start+i, start+numarg+i);

  // Truncate array.
  this->allocValues(oldnum - numarg);

  // Send notification.
  this->valueChanged();
}

/*!
  Can be used to make Coin delete the array pointer set through
  a setValuesPointer() call. See SoMField documentation for
  information about the setValuesPointer() function.

  This method is a TGS extension (introduced in TGS OIV v3.0) and is
  supported only for compatibility. We suggest that you don't use it
  since it can lead to hard-to-find bugs.

  \since Coin 2.0
  \since TGS Inventor 3.0
*/
void
SoMField::enableDeleteValues(void)
{
  this->userDataIsUsed = FALSE;
}

/*!
  Returns whether SoMField::enableDeleteValues() has been
  called on a field. The result is only valid if setValuesPointer()
  has been called on the field first.

  This method is a TGS extension (introduced in TGS OIV v3.0) and is
  supported only for compatibility. We suggest that you don't use it
  since it can lead to hard-to-find bugs.

  \since Coin 2.0
  \since TGS Inventor 3.0
*/
SbBool
SoMField::isDeleteValuesEnabled(void) const
{
  return !this->userDataIsUsed;
}

/*!
  Insert \a num "slots" for new value elements from \a start.
  The elements already present from \a start will be moved
  "upward" in the extended array.
*/
void
SoMField::insertSpace(int start, int numarg)
{
  if (numarg == 0) return;

  // Don't use getNum(), so we avoid recursive evaluate() calls.
  int oldnum = this->num;
#if COIN_DEBUG
  if (start < 0 || start > oldnum || numarg < 0) {
    SoDebugError::post("SoMField::insertSpace",
                       "invalid indices [%d, %d] for array of size %d",
                       start, start + numarg, oldnum);
    return;
  }
#endif // COIN_DEBUG

  // Expand array.
  this->allocValues(oldnum + numarg);

  // Copy values upward.
  for (int i = oldnum - start - 1; i >= 0; i--) {
    this->copyValue(start+numarg+i, start+i);
  }

  // Send notification.
  // FIXME: It looks like a lot of unnecessary work is being done here
  // if notifications are disabled. Look into shortcutting either here
  // or somewhere not too far from here. kintel 20070103.
  this->valueChanged();
}

#ifndef DOXYGEN_SKIP_THIS // Internal method.
void
SoMField::allocValues(int newnum)
{
  // Important notice: the "non-realloc"-version of this method is
  // found in SoSubField.h. If you make modifications here, do check
  // whether or not they should be matched with modifications in that
  // method as well.

  assert(newnum >= 0);

  if (newnum == 0) {
    if (!this->userDataIsUsed) {
      delete[] static_cast<unsigned char *>(this->valuesPtr());
    }
    this->setValuesPtr(NULL);
    this->userDataIsUsed = FALSE;
    this->maxNum = 0;
  }
  else if (newnum > this->maxNum || newnum < this->num) {
    int fsize = this->fieldSizeof();
    if (this->valuesPtr()) {

      // Allocation strategy is to repeatedly double the size of the
      // allocated block until it will at least match the requested
      // size.  (Unless the requested size is less than what we've
      // got, then we'll repeatedly halve the allocation size.)
      //
      // I think this will handle both cases quite gracefully: 1)
      // newnum > this->maxNum, 2) newnum < num
      int oldmaxnum = this->maxNum;
      while (newnum > this->maxNum) this->maxNum *= 2;
      while ((this->maxNum / 2) >= newnum) this->maxNum /= 2;

#if COIN_DEBUG && 0 // debug
      SoDebugError::postInfo("SoMField::allocValues",
                             "'%s' newnum==%d, old/new %p->maxNum==%d/%d",
                             this->getTypeId().getName().getString(),
                             newnum, this, oldmaxnum, this->maxNum);

#endif // debug

      if (oldmaxnum != this->maxNum) {
        // FIXME: Umm.. aren't we supposed to use realloc() here?
        // 20000915 mortene.
        size_t buffersize = size_t(this->maxNum) * size_t(fsize);
        unsigned char * newblock = new unsigned char[this->maxNum * fsize];
        size_t copysize = size_t(fsize) * size_t(SbMin(this->num, newnum));
        (void)memcpy(newblock, this->valuesPtr(), copysize);
        // we have to dereference old values in SoMFNode, SoMFPath and
        // SoMFEngine, so we just initialize the part of the array
        // with no defined values to NULL.
        if (buffersize > copysize) {
          (void)memset(newblock + copysize, 0, buffersize - copysize);
        }
        if (!this->userDataIsUsed) {
          delete[] static_cast<unsigned char *>(this->valuesPtr());
        }
        this->setValuesPtr(newblock);
        this->userDataIsUsed = FALSE;
      }
    }
    else {
      size_t buffersize = size_t(newnum) * size_t(fsize);
      unsigned char * data = new unsigned char[buffersize];
      // we have to dereference old values in SoMFNode, SoMFPath and
      // SoMFEngine, so we just initialize the array to NULL.
      (void)memset(data, 0, buffersize);
      this->setValuesPtr(data);
      this->userDataIsUsed = FALSE;
      this->maxNum = newnum;
    }
  }

  this->num = newnum;
}
#endif // DOXYGEN_SKIP_THIS

SoNotRec
SoMField::createNotRec(SoBase * cont)
{
  SoNotRec rec(inherited::createNotRec(cont));
  rec.setIndex(this->changedIndex);
  rec.setFieldNumIndices(this->numChangedIndices);
  return rec;
}

void
SoMField::setChangedIndex(const int chgidx)
{
  this->changedIndex = chgidx;
  this->numChangedIndices = 1;
}

void
SoMField::setChangedIndices(const int chgidx, const int numchgind)
{
  this->changedIndex = chgidx;
  this->numChangedIndices = numchgind;
}
