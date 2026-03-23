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
  \class SoFieldData SoFieldData.h Inventor/fields/SoFieldData.h
  \brief The SoFieldData class is a container for a prototype set of fields.

  \ingroup coin_fields

  This class is instantiated once for each class of objects which use
  fields, and which need to be able to import and export them.

  Each field of a class is stored with the name it has been given
  within its "owner" class and a pointer offset to the dynamic
  instance of the field itself.

  Enumeration sets are stored with (name, value) pairs, to make it
  possible to address, read and save enum type fields by name.

  It is unlikely that application programmers should need to use any
  of the methods of this class directly.

  \sa SoField, SoFieldContainer
*/

// *************************************************************************

// FIXME: Some methods related to reading VRML 2 files are
// missing. ????-??-?? pederb.

/* IMPORTANT NOTE:
 * If you make any changes (bugfixes, improvements) in this class,
 * remember to also check the SoEngineOutputData class, as it is
 * heavily based on this class.
 */

// *************************************************************************

#include <Inventor/fields/SoFieldData.h>

#include <cctype>

#include <Inventor/SbName.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/fields/SoFieldContainer.h>
#include <Inventor/lists/SoFieldList.h>
#include <Inventor/misc/SoProto.h>

#include "threads/threadsutilp.h"
#include "io/SoInputP.h"
#include "coindefs.h" // COIN_STUB()

// *************************************************************************

static const char OPEN_BRACE_CHAR = '[';
static const char CLOSE_BRACE_CHAR = ']';
static const char VALUE_SEPARATOR_CHAR = ',';

// *************************************************************************

class SoFieldEntry {
public:
  SoFieldEntry(const char * n, ptrdiff_t o) : name(n), ptroffset(o) { }
  // Copy constructors.
  SoFieldEntry(const SoFieldEntry * fe) { this->copy(fe); }
  SoFieldEntry(const SoFieldEntry & fe) { this->copy(&fe); }

  int operator==(const SoFieldEntry & fe) const {
    // don't consider ptroffset here, since this will not be equal
    // for fields containers with dynamic fields.
    return (this->name == fe.name);
  }
  int operator!=(const SoFieldEntry & fe) const {
    return ! operator==(&fe);
  }

  SbName name;
  ptrdiff_t ptroffset;

private:
  void copy(const SoFieldEntry * fe) {
    this->name = fe->name;
    this->ptroffset = fe->ptroffset;
  }
};

class SoEnumEntry {
public:
  SoEnumEntry(const SbName & name) : nameoftype(name) { }
  // Copy constructors.
  SoEnumEntry(const SoEnumEntry * ee) { this->copy(ee); }
  SoEnumEntry(const SoEnumEntry & ee) { this->copy(&ee); }

  int operator==(const SoEnumEntry & ee) const {
    return ((this->nameoftype == ee.nameoftype) &&
            (this->names == ee.names) && (this->values == ee.values));
  }
  int operator!=(const SoEnumEntry & ee) const { return ! operator==(&ee); }

  SbName nameoftype;
  SbList<SbName> names;
  SbList<int> values;

private:
  void copy(const SoEnumEntry * ee) {
    this->nameoftype = ee->nameoftype;
    this->names = ee->names;
    this->values = ee->values;
  }
};

// *************************************************************************

/*!
  Default constructor.
 */
SoFieldData::SoFieldData(void)
{
}

/*!
  Copy constructor.
 */
SoFieldData::SoFieldData(const SoFieldData & fd)
{
  this->copy(&fd);
}

/*!
  Copy constructor taking a pointer value as an argument. Handles \c
  NULL pointers by behaving like the default constructor.
*/
SoFieldData::SoFieldData(const SoFieldData * fd)
{
  if (fd) this->copy(fd);
}

/*!
  Constructor. Takes an indication on the number of fields which
  should be stored, to make sure the memory handling is efficient.
 */
SoFieldData::SoFieldData(int numfields)
  : fields(numfields)
{
}

/*!
  Destructor.
 */
SoFieldData::~SoFieldData()
{
  this->freeResources();
}

// Empties internal lists, while deallocating the memory used for the
// entries.
void
SoFieldData::freeResources(void)
{
  for (int i=0; i < this->fields.getLength(); i++) delete this->fields[i];
  this->fields.truncate(0);

  for (int j=0; j < this->enums.getLength(); j++) delete this->enums[j];
  this->enums.truncate(0);
}

/*!
  Add a new field to our internal list.

  The \a name will be stored along with an pointer offset between \a
  base and \a field, which will be valid for all instances of the
  class type of \a base.
*/
void
SoFieldData::addField(SoFieldContainer * base, const char * name,
                      const SoField * field)
{
  // FIXME: one peculiar thing I discovered while debugging something
  // else. It appears the GlobalField::realTime is added twice -- how
  // come? 20050708 mortene.
  //
  // FIXME: and another peculiar thing. Why is the SoInfo::string
  // field added upon SoDB::init()? 20050708 mortene.

  CC_GLOBAL_LOCK;

  // Will be called many times, from each node constructor, for every
  // field of the node. We're only interested in in getting this
  // information /once/, however.
  if (!this->hasField(name)) {

#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SoFieldData::addField",
                           "class==%s, fieldname==%s, field==%p, index==%d",
                           base->getTypeId().getName().getString(),
                           name, field, this->fields.getLength());
#endif // debug

    size_t vbase = reinterpret_cast<size_t>(base);
    size_t vfield = reinterpret_cast<size_t>(field);
    const ptrdiff_t offs = vfield - vbase;

    // FIXME: disabled yet, as we should first make a test program to
    // see if this robustness check is ok with the current Coin
    // code. The check should simply run through all
    // SoFieldContainer-derived classes and make an instance of all
    // non-abstract ones. Then see if there'll be any asserting or
    // crashing from the below check.
    //
    // This would smoke out any internal faulty use of
    // SoFieldData::addField().
    //
    // 20050708 mortene.
#if 0
    // Robustness: check whether or not the given field is actually a
    // member of base.
    const SoField * f = (const SoField *)(base + offs);
    // The next is likely to segfault from the isOfType() call if
    // there's an error (and not actually assert).
    assert(f->isOfType(SoField::getClassTypeId()));
#endif

    this->fields.append(new SoFieldEntry(name, offs));
  }
  CC_GLOBAL_UNLOCK;
}

/*!
  Copy fields from container \a from to container \a to. If
  \a copyconnections is \c TRUE, we'll also copy the connections
  field \a from has made.

  If you think the method signature is a bit strange, you're correct.
  This should really have been a static method (the owner \c this
  instance of the method isn't used at all, due to how the internal
  representation of field template list are stored), but for unknown
  reasons this is a dynamic method in Open Inventor. So also in Coin,
  to keep compatibility.
 */
void
SoFieldData::overlay(SoFieldContainer * to, const SoFieldContainer * from,
                     SbBool copyconnections) const
{
  if (to == from) return;

  const SoFieldData * fd0 = to->getFieldData();
  const SoFieldData * fd1 = from->getFieldData();
  if (!fd0 || !fd1) return;

  // The field containers should have equal SoFieldData sets.
  assert(fd0 && fd1 && *fd0==*fd1);

  const int num = fd0->getNumFields();
  for (int i = 0; i < num; i++) {
    SoField * field0 = fd0->getField(to, i);
    SoField * field1 = fd1->getField(from, i);
    // copy value only if necessary (note how SoTexture2::filename and
    // SoTexture2::image would affect each other without this test)
    if ( !field0->isDefault() || !field1->isDefault() ) {
      field0->copyFrom(*field1);
      field0->setDefault(field1->isDefault());
    }
    // copy flags
    field0->setIgnored(field1->isIgnored());
    field0->enableNotify(field1->isNotifyEnabled());
    field0->setFieldType(field1->getFieldType());

    // fix complex fields (node, engine, and path fields)
    field0->fixCopy(copyconnections);
    // handle connections
    if (copyconnections) field0->copyConnection(field1);
  }
}

/*!
  Returns number of fields contained within this instance.
 */
int
SoFieldData::getNumFields(void) const
{
  return this->fields.getLength();
}

/*!
  Returns the name of the field at \a index.
 */
const SbName &
SoFieldData::getFieldName(int index) const
{
  return this->fields[index]->name;
}

/*!
  Returns a pointer to the field at \a index within the \a object
  instance.
 */
SoField *
SoFieldData::getField(const SoFieldContainer * object, int index) const
{
  assert(index >= 0 && index < this->fields.getLength());
  size_t fieldptr = reinterpret_cast<size_t>(object);
  fieldptr += this->fields[index]->ptroffset;
  return reinterpret_cast<SoField *>(fieldptr);
}

/*!
  Returns the internal index value of \a field in \a fc. If \a field
  is not part of \a fc, returns -1.
*/
int
SoFieldData::getIndex(const SoFieldContainer * fc, const SoField * field) const
{
  size_t vbase = reinterpret_cast<size_t>(fc);
  size_t vfield = reinterpret_cast<size_t>(field);
  const ptrdiff_t ptroffset = vfield - vbase;

  for (int i=0; i < this->fields.getLength(); i++)
    if (this->fields[i]->ptroffset == ptroffset) return i;

  return -1;
}

/*!
  Either adds a new enum set (with an initial member), or adds a new value
  member to an existing enum set.
*/
void
SoFieldData::addEnumValue(const char * enumname, const char * valuename,
                          int value)
{
  CC_GLOBAL_LOCK;
  if (!this->hasEnumValue(enumname, valuename)) {
    SoEnumEntry * e = NULL;

    for (int i=0; !e && (i < this->enums.getLength()); i++) {
      if (this->enums[i]->nameoftype == enumname) e = this->enums[i];
    }

    if (e == NULL) {
      e = new SoEnumEntry(enumname);
      this->enums.append(e);
    }

#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SoFieldData::addEnumValue",
                           "enumname: %s, valuename: %s, value: %d",
                           enumname, valuename, value);
#endif // debug

    assert(e->names.find(valuename) == -1);
    e->names.append(valuename);
    // Note that an enum can have several names mapping to the same
    // value. 20000101 mortene.
    e->values.append(value);
  }
  CC_GLOBAL_UNLOCK;
}

/*!
  Returns the \a names and \a values of enumeration entry with the \a
  enumname. The number of (name, value) pairs available in the
  enumeration is returned in \a num.
*/
void
SoFieldData::getEnumData(const char * enumname, int & num,
                         const int *& values, const SbName *& names)
{
  num = 0;
  values = NULL;
  names = NULL;

  for (int i=0; i < this->enums.getLength(); i++) {
    SoEnumEntry * e = this->enums[i];
    if (e->nameoftype == enumname) {
      num = e->names.getLength();
      if (num) {
        assert(e->names.getLength() == e->values.getLength());
        names = e->names.getArrayPtr();
        values = e->values.getArrayPtr();
      }
      return;
    }
  }
}

/*!
  Read field data from the \a in stream for fields belonging to \a
  object. Returns \c TRUE if everything went OK, or \c FALSE if any
  error conditions occurs.

  \a erroronunknownfield decides whether or not \c FALSE should be
  returned if a name identifier not recognized as a field name of \a
  object is encountered. Note that \a erroronunknownfield should be \c
  FALSE if \a object is a container with child objects, otherwise the
  code will fail upon the first child name specification.

  If \a notbuiltin is \c TRUE on return, \a object is an unknown node
  or engine type. Unknown nodes are recognized by the \c fields
  keyword first in their file format definition, and unknown engines
  by the \c inputs keyword.

*/
SbBool
SoFieldData::read(SoInput * in, SoFieldContainer * object,
                  SbBool erroronunknownfield, SbBool & notbuiltin) const
{
  notbuiltin = FALSE;

  if (in->isBinary()) {
    unsigned int fieldsval;
    if (!in->read(fieldsval)) {
      SoReadError::post(in, "Premature EOF");
      return FALSE;
    }

    uint8_t numfields = static_cast<uint8_t>(fieldsval & 0xff);
    uint8_t fieldflags = static_cast<uint8_t>(fieldsval >> 8);

    if (SoInputP::debugBinary()) {
      SoDebugError::postInfo("SoFieldData::read",
                             "fieldsval==0x%08x => "
                             "flags==0x%02x numfields==%u (0x%02x)",
                             fieldsval, fieldflags, numfields);
    }

    // Unknown node type, must read field descriptions.
    if (fieldflags & SoFieldData::NOTBUILTIN) {
      if (!this->readFieldDescriptions(in, object, numfields)) return FALSE;
    }

    // Check for more flags, in case there's any we've missed.
    if (fieldflags & ~(SoFieldData::NOTBUILTIN)) {
      SoReadError::post(in,
                        "Unknown flags in control word: 0x%02x, "
                        "please report to coin-support@coin3d.org",
                        fieldflags);
    }

    if (numfields > this->fields.getLength()) {
      SoDebugError::postWarning("SoFieldData::read",
                                "The number of fields to read for a %s "
                                "node in this binary file is given as %d. "
                                "This is suspicious as this node type "
                                "doesn't have more than %d distinct fields. "
                                "The file is likely to be corrupt.",
                                object->getTypeId().getName().getString(),
                                numfields, this->fields.getLength());
    }

    if (numfields == 0) return TRUE;

    for (int i=0; i < numfields; i++) {
      SbName fieldname;
      if (!in->read(fieldname, TRUE) || !fieldname) {
        SoReadError::post(in, "Couldn't read the name of field number %d", i);
        return FALSE;
      }

      if (SoInputP::debugBinary()) {
        SoDebugError::postInfo("SoFieldData::read",
                               "fieldname=='%s'", fieldname.getString());
      }

      SbBool foundname;
      if (!this->read(in, object, fieldname, foundname)) {
        if (!foundname) SoReadError::post(in, "Unknown field \"%s\" in \"%s\"",
                                          fieldname.getString(),
                                          object->getTypeId().getName().getString());
        return FALSE;
      }
    }
  }
  else { // ASCII format.
    SbBool firstidentifier = TRUE;
    SbName ROUTE_KEYWORD("ROUTE");
    SbName PROTO_KEYWORD("PROTO");
    SbName EXTERNPROTO_KEYWORD("EXTERNPROTO");
    while (TRUE) {
      SbName fieldname;
      if (!in->read(fieldname, TRUE)) return TRUE; // Terminates loop on "}"

      if (in->isFileVRML2()) {
        // test for the VRML97 ROUTE keyword
        if (fieldname == ROUTE_KEYWORD) {
          if (!SoBase::readRoute(in)) return FALSE;
          continue; // skip to next field/route
        }
        // test for the VRML97 PROTO/EXTERNPROTO keyword
        if (fieldname == PROTO_KEYWORD || fieldname == EXTERNPROTO_KEYWORD) {
          SoProto * proto = new SoProto(fieldname == EXTERNPROTO_KEYWORD);
          proto->ref();
          if (proto->readInstance(in, 0)) {
            proto->unrefNoDelete();
            in->addProto(proto);
          }
          else {
            proto->unref();
            SoReadError::post(in, "Error while parsing PROTO definition inside node");
            return FALSE;
          }
          continue;  // skip to next field/route
        }
      }

      SbBool readok;
      if (in->checkISReference(object, fieldname, readok)) {
        continue; // skip to next field
      }
      if (!readok) {
        SoReadError::post(in, "Error while searching for IS keyword for field \"%s\"",
                          fieldname.getString());
        return FALSE;
      }
      // This should be caught in SoInput::read(SbName, SbBool).
      assert(fieldname != "");

      SbBool foundname;
      if (!this->read(in, object, fieldname, foundname) && foundname)
        return FALSE;

      if (!foundname) {
        // User extension node with explicit field definitions.
        if (firstidentifier && fieldname == "fields") {
          notbuiltin = TRUE;
          if (!this->readFieldDescriptions(in, object, 0)) return FALSE;
        }
        // User extension engine with explicit input field definitions.
        else if (firstidentifier && fieldname == "inputs") {
          notbuiltin = TRUE;
          // FIXME: read input defs and inputs (and output
          // defs?). 20000102 mortene.
          COIN_STUB();
          return FALSE;
        }
        else if (erroronunknownfield) {
          SoReadError::post(in, "Unknown field \"%s\" in \"%s\"",
                            fieldname.getString(),
                            object->getTypeId().getName().getString());
          return FALSE;
        }
        else {
          in->putBack(fieldname.getString());
          return TRUE;
        }
      }
      firstidentifier = FALSE;
    }
  }

  return TRUE;
}

/*!
  Find field \a fieldname in \a object, and if it is available, set
  \a foundname to \c TRUE and try to read the field specification
  from \a in. If \a foundname is set to \c TRUE, the return value
  says whether or not the field specification could be read without
  any problems.

  If \a fieldname is not part of \a object, returns \c FALSE with \a
  foundname also set to \c FALSE.
*/
SbBool
SoFieldData::read(SoInput * in, SoFieldContainer * object,
                  const SbName & fieldname, SbBool & foundname) const
{
  for (int i = 0; i < this->fields.getLength(); i++) {
    if (fieldname == this->getFieldName(i)) {
      foundname = TRUE;
      return this->getField(object, i)->read(in, fieldname);
    }
  }

  foundname = FALSE;

  // Should return TRUE, according to how this function is supposed to
  // work: it should only return FALSE on actual /parse/ errors, and
  // not "just" when the name of the read field is unknown.
  //
  // An example where this is necessary is where field names don't
  // match up directly for nodekits, but the field is actually in a
  // nested nodekit (i.e. a nodekit within another nodekit's catalog),
  // or is a composite name for a field in a nested nodekit.
  return TRUE;
}

/*!
  Write to \a out field names and field values for the fields of
  \a object.
 */
void
SoFieldData::write(SoOutput * out, const SoFieldContainer * object) const
{
  // In Coin, we always write field description for all fields in
  // extension nodes. This means that we also need to write all fields
  // for the binary format, since the number of fields and field
  // descriptions is printed in a byte before the field
  // descriptions. Phew, the OIV binary format sucks....
  SbBool writeallfields = out->isBinary() && ! object->getIsBuiltIn();

  if (out->getStage() == SoOutput::COUNT_REFS) {
    // Handle first stage of write operations.
    for (int i=0; i < this->getNumFields(); i++) {
      SoField * f = this->getField(object, i);
      if (writeallfields || f->shouldWrite()) {
        f->write(out, this->getFieldName(i));
      }
    }
    return;
  }
  // Ok, we've passed the first write stage and is _really_ writing.
  assert((out->getStage() == SoOutput::WRITE) && "unknown write stage");

  // FIXME: is this really the best place to write the flags +
  // numfields value? 20000102 mortene.
  if (out->isBinary()) {
    // Check how many fields will be written.
    uint8_t numfields = 0;
    for (int j = 0; j < this->getNumFields(); j++) {
      const SoField * f = this->getField(object, j);
      if (writeallfields || f->shouldWrite()) {
        // This is an amazingly lame limitation, but we can't really
        // fix it without breaking compatibility with the SGI binary
        // .iv format.  (The moral of the story is: avoid binary
        // .iv-files.)
        assert((numfields < 255) &&
               "too many fields to handle with binary .iv format");
        numfields++;
      }
    }

    uint16_t fieldflags = 0x0000;
    // FIXME: take care of setting flags for SoUnknownEngines, if
    // necessary. 20000102 mortene.
    if (!object->getIsBuiltIn()) fieldflags |= SoFieldData::NOTBUILTIN;

    // use unsigned int to match an SoOutput::write method
    unsigned int w = static_cast<unsigned int>(fieldflags);
    w <<= 8;
    w |= numfields;

    out->write(w);
  }

  // FIXME: write descriptions for SoUnknownEngine, if
  // necessary. 20000102 mortene.
  if (!object->getIsBuiltIn()) this->writeFieldDescriptions(out, object);

  SoProto * proto = out->getCurrentProto();

  for (int i = 0; i < this->getNumFields(); i++) {
    SoField * f = this->getField(object, i);
    // Test if field has a PROTO IS reference
    SbName pname = proto ?
      proto->findISReference(object, this->getFieldName(i)) : SbName::empty();
    if (pname.getLength()) {
      out->indent();
      out->write(this->getFieldName(i).getString());
      out->write(" IS ");
      out->write(pname.getString());
      out->write("\n");
    }
    else if (writeallfields || f->shouldWrite()) {
      f->write(out, this->getFieldName(i));
    }
  }
}


/*!
  Copy contents of \a src into this instance.

  If there was any data set up in this instance before the method was
  called, the old data is removed first.

  Note that this only copies the field set template specification from
  \a src, \e not actual field contents. For copying field contents,
  see the SoFieldData::overlay() method.
*/
void
SoFieldData::copy(const SoFieldData * src)
{
  this->freeResources();

  int i;
  for (i=0; i < src->fields.getLength(); i++) {
    this->fields.append(new SoFieldEntry(src->fields[i]));
  }
  for (i=0; i < src->enums.getLength(); i++) {
    this->enums.append(new SoEnumEntry(src->enums[i]));
  }
}

/*!
  Compares \a c1 and \a c2 to see if they have the same field data set
  and if the fields of \a c1 have the same values as the fields of \a c2.

  Field connections are not considered (i.e. we will return \c TRUE if
  the values of the fields of \a c1 are equal to the fields of \a c2,
  even if they differ in how they have made connections to other
  fields).

  If you think the method signature is a bit strange, you're correct.
  This should really have been a static method (the owner \c this
  instance of the method isn't used at all, due to how the internal
  representations of field template lists are stored), but for unknown
  reasons this is a dynamic method in Open Inventor. So also in Coin,
  to keep compatibility.
*/
SbBool
SoFieldData::isSame(const SoFieldContainer * c1,
                    const SoFieldContainer * c2) const
{
  if (c1 == c2) return TRUE;

  const SoFieldData * fd1 = c1->getFieldData();
  const SoFieldData * fd2 = c2->getFieldData();
  if (!fd1 && !fd2) return TRUE;
  if (!fd1 || !fd2) return FALSE;
  if (*fd1 != *fd2) return FALSE;

  int num = fd1->getNumFields();
  for (int i=0; i < num; i++)
    if (*(fd1->getField(c1, i)) != *(fd2->getField(c2, i))) return FALSE;

  return TRUE;
}

/*!
  Reads a set of field specifications from \a in for an unknown node class type,
  in the form "[ FIELDCLASS FIELDNAME, FIELDCLASS FIELDNAME, ... ]".

  \a numdescriptionsexpected is used for binary format import to know
  how many descriptions should be parsed.

  If \a readfieldvalues is \e TRUE (the default), the field initial value
  is expected after the field name in the SoInput stream.

*/
SbBool
SoFieldData::readFieldDescriptions(SoInput * in, SoFieldContainer * object,
                                   int numdescriptionsexpected,
                                   const SbBool readfieldvalues) const
{
  // These two macros are convenient for reading with error detection.
#define READ_CHAR(c) \
    if (!in->read(c)) { \
      SoReadError::post(in, "Premature end of file"); \
      return FALSE; \
    }

  const SbName EVENTIN("eventIn");
  const SbName EVENTOUT("eventOut");
  const SbName FIELD("field");
  const SbName EXPOSEDFIELD("exposedField");
  const SbName IS("IS");

  char c;
  if (!in->isBinary()) {
    READ_CHAR(c);
    if (c != OPEN_BRACE_CHAR) {
      SoReadError::post(in, "Expected '%c', got '%c'", OPEN_BRACE_CHAR, c);
      return FALSE;
    }
  }

  for (int j=0; !in->isBinary() || (j < numdescriptionsexpected); j++) {

    if (!in->isBinary()) {
      READ_CHAR(c);
      if (c == CLOSE_BRACE_CHAR) return TRUE;
      else in->putBack(c);
    }

    SbName fieldtypename;

    if (!in->read(fieldtypename, TRUE)) {
      SoReadError::post(in, "Couldn't read name of field type");
      return FALSE;
    }

    SbName fieldtype("");
    if (fieldtypename == EVENTIN ||
        fieldtypename == EVENTOUT ||
        fieldtypename == FIELD ||
        fieldtypename == EXPOSEDFIELD) {
      fieldtype = fieldtypename;
      if (!in->read(fieldtypename, TRUE)) {
        SoReadError::post(in, "Couldn't read name of field type");
        return FALSE;
      }
    }

    SoType type = SoType::fromName(fieldtypename.getString());
    if ((type == SoType::badType()) ||
        !type.isDerivedFrom(SoField::getClassTypeId())) {
      SoReadError::post(in, "Unknown field type '%s'",
                        fieldtypename.getString());
      return FALSE;
    }
    else if (!type.canCreateInstance()) {
      SoReadError::post(in, "Abstract class type '%s'", fieldtypename.getString());
      return FALSE;
    }

    SbName fieldname;
    if (!in->read(fieldname, TRUE)) {
      SoReadError::post(in, "Couldn't read name of field");
      return FALSE;
    }


#if COIN_DEBUG && 0 // debug
    SoDebugError::postInfo("SoFieldData::readFieldDescriptions",
                           "type: \"%s\", name: \"%s\"",
                           fieldtypename.getString(), fieldname.getString());
#endif // debug

    SoField * newfield = NULL;
    for (int i=0; !newfield && (i < this->fields.getLength()); i++) {
      if (this->fields[i]->name == fieldname) {
        newfield = this->getField(object, i);
      }
    }
    if (!newfield) {
      // Cast away const -- ugly.
      SoFieldData * that = const_cast<SoFieldData *>(this);
      newfield = static_cast<SoField *>(type.createInstance());
      newfield->setContainer(object);
      newfield->setDefault(TRUE);
      that->addField(object, fieldname.getString(), newfield);
    }

    if (fieldtype == EVENTIN || fieldtype == EVENTOUT) {
      if (fieldtype == EVENTIN) {
        newfield->setFieldType(SoField::EVENTIN_FIELD);
      }
      else {
        newfield->setFieldType(SoField::EVENTOUT_FIELD);
      }
      SbBool readok;
      (void) in->checkISReference(object, fieldname.getString(), readok);
      if (!readok) {
        SoReadError::post(in, "Error while searching for IS keyword for field '%s'",
                          fieldname.getString());
        return FALSE;
      }
    }
    else if (fieldtype == FIELD || fieldtype == EXPOSEDFIELD) {
      if (fieldtype == EXPOSEDFIELD) {
        newfield->setFieldType(SoField::EXPOSED_FIELD);
      }
      if (readfieldvalues && !newfield->read(in, fieldname)) {
        SoFieldContainer * fc = newfield->getContainer();
        SbString s("");
        if (fc) { s.sprintf(" of %s", fc->getTypeId().getName().getString()); }
        SoReadError::post(in, "Unable to read value for field '%s'%s",
                          fieldname.getString(), s.getString());
        return FALSE;
      }
    }

    SbBool readok;
    (void) in->checkISReference(object, fieldname, readok);
    if (!readok) {
      SoReadError::post(in, "Unable to search for IS keyword");
      return FALSE;
    }
    if (!in->isBinary()) {
      READ_CHAR(c);
      if (c != VALUE_SEPARATOR_CHAR) in->putBack(c);
      // (Allow missing value separators (i.e. no "," character
      // between two field descriptions)).
    }
  }

#undef READ_CHAR

  return TRUE;
}


/*!
  Write a set of field specifications to \a out for an unknown node class type,
  in the form "[ FIELDCLASS FIELDNAME, FIELDCLASS FIELDNAME, ... ]".
 */
void
SoFieldData::writeFieldDescriptions(SoOutput * out,
                                    const SoFieldContainer * object) const
{
  SoFieldList forwardlist;

  if (!out->isBinary()) {
    out->indent();
    out->write("fields [ ");
  }

  SbBool atleastonewritten = FALSE;
  for (int i = 0; i < this->getNumFields(); i++) {
    const SoField * f = this->getField(object, i);
    if (!out->isBinary() && atleastonewritten) out->write(", ");
    out->write(static_cast<const char *>(f->getTypeId().getName()));
    if (!out->isBinary()) out->write(' ');
    out->write(static_cast<const char *>(this->getFieldName(i)));
    atleastonewritten = TRUE;
  }

  if (!out->isBinary()) out->write(" ]\n");
}

// Check for equality.
int
SoFieldData::operator==(const SoFieldData * fd) const
{
  int i, n;
  n = this->enums.getLength();
  if (n != fd->enums.getLength()) return FALSE;
  for (i = 0; i < n; i++) {
    if (*(this->enums[i]) != *(fd->enums[i])) return FALSE;
  }

  n = this->fields.getLength();
  if (n != fd->fields.getLength()) return FALSE;
  for (i = 0; i < n; i++) {
    if (*(this->fields[i]) != *(fd->fields[i])) return FALSE;
  }

  return TRUE;
}

/*!
  \internal
  \since Coin 2.3
*/
SbBool
SoFieldData::hasField(const char * name) const
{
  for (int i = 0; i < this->fields.getLength(); i++) {
    if (this->fields[i]->name == name) return TRUE;
  }
  return FALSE;
}

/*!
  \internal
  \since Coin 2.3
*/
SbBool
SoFieldData::hasEnumValue(const char * enumname, const char * valuename)
{
  SoEnumEntry * e = NULL;

  for (int i=0; !e && (i < this->enums.getLength()); i++) {
    if (this->enums[i]->nameoftype == enumname) e = this->enums[i];
  }
  if (e == NULL) return FALSE;
  return e->names.find(valuename) != -1;
}
