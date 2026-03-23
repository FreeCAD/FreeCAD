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

// SoGlobalField is an internal class where the instances keeps track
// of the global fields in the application.
//
// Having global fields placed within their own field containers makes
// it easier to handle field-to-field connections during file import
// and export.
//
// Note that this class is not supposed to be used by the application
// programmer -- so the API class definition header file is not
// installed.
//
// The only place within Coin where it is "consciously" used is from
// the SoDB class -- which also contains a complete "front-end" API to
// this class (see the SoDB::createGlobalField(),
// SoDB::getGlobalField() and SoDB::renameGlobalField() methods).

// *************************************************************************

#include "fields/SoGlobalField.h"

#include <cassert>
#include <cstring>

#include <Inventor/SoDB.h>
#include <Inventor/SbName.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/errors/SoReadError.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/fields/SoFieldData.h>
#include <Inventor/lists/SoBaseList.h>

#include "coindefs.h"
#include "tidbitsp.h"
#include "SbBasicP.h"

// *************************************************************************

SoType SoGlobalField::classTypeId STATIC_SOTYPE_INIT;

SoBaseList * SoGlobalField::allcontainers = NULL;

// *************************************************************************

// Constructor. Pass NULL for the field pointer to construct an empty
// SoGlobalField instance.
SoGlobalField::SoGlobalField(const SbName & name, SoField * field)
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoGlobalField::SoGlobalField",
                         "name=='%s', field==%p(%s)",
                         name.getString(), field,
                         field ? field->getTypeId().getName().getString() : "");
#endif // debug

  if (field) {
    // Initialize a fielddata container for the instance.
    this->classfielddata = new SoFieldData;

    field->setContainer(this);

    this->classfielddata->addField(this, name, field);
  }
  else {
    this->classfielddata = NULL;
  }

  this->setName(name);

  SoGlobalField::allcontainers->append(this);
}

// Destructor.
SoGlobalField::~SoGlobalField()
{
  SoGlobalField::allcontainers->removeItem(this);
#if COIN_DEBUG && 0 // debug
  SoField * field = this->classfielddata->getField(this, 0);
  SoDebugError::postInfo("SoGlobalField::~SoGlobalField",
                         "name=='%s', field==%p(%s)",
                         this->getName().getString(),
                         field,
                         field ? field->getTypeId().getName().getString() : "");
#endif // debug

  if (this->classfielddata) delete this->classfielddata->getField(this, 0);
  delete this->classfielddata;
}

// Instantiates and returns a new SoGlobalField instance.
void *
SoGlobalField::createInstance(void)
{
  return new SoGlobalField(SbName::empty(), NULL);
}

// Do common initializations.
void
SoGlobalField::initClass(void)
{
  // Make sure we init this class only once.
  assert(SoGlobalField::classTypeId == SoType::badType());
  // Make sure parent class has been initialized.
  assert(inherited::getClassTypeId() != SoType::badType());

  SoGlobalField::classTypeId =
    SoType::createType(inherited::getClassTypeId(), "GlobalField",
                       SoGlobalField::createInstance);

  SoGlobalField::allcontainers = new SoBaseList;
  // don't reference count items in this list. We need the refcount to
  // go down to 0 to detect when a global field is no longer
  // referenced
  SoGlobalField::allcontainers->addReferences(FALSE);
  coin_atexit(SoGlobalField::clean, CC_ATEXIT_NORMAL);
}

// Free up resources.
void
SoGlobalField::clean(void)
{
#if COIN_DEBUG

  // Warn about all global fields still alive, as they have the
  // potential to cause harm if there for instance are SoFieldSensor
  // instances attached to any of them. (The SoSensorManager is likely
  // to be dead at this point in time, which then causes havoc when
  // the sensors wants to unschedule themselves).
  for (int i=0; i < SoGlobalField::allcontainers->getLength(); i++) {
    SoGlobalField * gf =
      coin_assert_cast<SoGlobalField *>((*SoGlobalField::allcontainers)[i]);
    // Can't use SoDebugError here, as SoError et al might have been
    // "cleaned up" already.
    printf("Global field '%s' not deallocated -- use "
           "SoDB::renameGlobalField() on exit to "
           "accomplish this. Refcount: %d\n",
           gf->getName().getString(), gf->getRefCount());
  }

#endif // COIN_DEBUG

  delete SoGlobalField::allcontainers;
  SoGlobalField::allcontainers = NULL;
  SoGlobalField::classTypeId STATIC_SOTYPE_INIT;
}

// Return index in list of global fields of the global field with the
// given name. Returns -1 if no global field exists with this name.
int
SoGlobalField::getGlobalFieldIndex(const SbName & name)
{
  int idx = SoGlobalField::allcontainers->getLength() - 1;
  while (idx >= 0 && (*SoGlobalField::allcontainers)[idx]->getName() != name)
    idx--;

  return idx;
}

// Add the given global field to the global fieldcontainer list
void
SoGlobalField::addGlobalFieldContainer(SoGlobalField * fieldcontainer)
{
  SoGlobalField::allcontainers->append(fieldcontainer);
}

// Remove the given global field from the internal list.
//
// Note that this will decrease the reference count of the
// SoGlobalField node, causing it to be destructed unless it has been
// ref()'ed outside of this class.
void
SoGlobalField::removeGlobalFieldContainer(SoGlobalField * fieldcontainer)
{
  SoGlobalField::allcontainers->removeItem(fieldcontainer);
}

// Returns SoGlobalField instance with the given name.
SoGlobalField *
SoGlobalField::getGlobalFieldContainer(const SbName & name)
{
  int idx = SoGlobalField::getGlobalFieldIndex(name);
  return
    (idx == -1) ? NULL : coin_assert_cast<SoGlobalField *>((*SoGlobalField::allcontainers)[idx]);
}

// Returns the complete set of SoGlobalField instances.
SoBaseList *
SoGlobalField::getGlobalFieldContainers(void)
{
  return SoGlobalField::allcontainers;
}

// Return id of SoGlobalField class instances.
SoType
SoGlobalField::getClassTypeId(void)
{
  return SoGlobalField::classTypeId;
}

// Virtual method which returns the class type id for this instance.
// Doc in superclass.
SoType
SoGlobalField::getTypeId(void) const
{
  return SoGlobalField::classTypeId;
}


// Returns a pointer to the global field stored in this container.
SoField *
SoGlobalField::getGlobalField(void) const
{
  return this->classfielddata ? this->classfielddata->getField(this, 0) : NULL;
}

// Overridden to also set field name.
void
SoGlobalField::setName(const SbName & newname)
{
  // Set name of this instance.
  inherited::setName(newname);

  if (this->classfielddata) {
    // SoFieldData doesn't have a rename method, so we do a little
    // hack to rename our field.
    SoFieldData fd;
    fd.addField(this, newname, this->getGlobalField());
    this->classfielddata->copy(&fd);
  }
}

// Read data for this SoGlobalField instance.
SbBool
SoGlobalField::readInstance(SoInput * in, unsigned short COIN_UNUSED_ARG(flags))
{
  // A bit more coding and we could let the readInstance() method be
  // called on already initialized SoGlobalField instances, but I
  // don't think there's any point. mortene.
  assert(this->classfielddata == NULL);

  // This macro is convenient for reading with error detection.
#define READ_VAL(val) \
  if (!in->read(val)) { \
    SoReadError::post(in, "Premature end of file"); \
    return FALSE; \
  }

  SbString str;
  READ_VAL(str);
  if (str != "type") {
    SoReadError::post(in, "invalid identifier, expected 'type', got '%s'",
                      str.getString());
    return FALSE;
  }

  SbString typestr;
  READ_VAL(typestr);
  SbName type(typestr);

  SoType fieldtype = SoType::fromName(type);
  if (fieldtype == SoType::badType()) {
    SoReadError::post(in, "invalid field type '%s'", type.getString());
    return FALSE;
  }
  if (!fieldtype.canCreateInstance()) {
    SoReadError::post(in, "abstract type '%s'", type.getString());
    return FALSE;
  }
  if (!fieldtype.isDerivedFrom(SoField::getClassTypeId())) {
    SoReadError::post(in, "'%s' not a field type", type.getString());
    return FALSE;
  }

  if (in->isBinary()) {
    int nrfields;
    READ_VAL(nrfields);
    if (nrfields != 1) {
      SoReadError::post(in, "%d fields for a globalfield node (should "
                        "always be 1)", nrfields);
      return FALSE;
    }
  }

  SbName fieldname;
  READ_VAL(fieldname);
  inherited::setName(fieldname);

  SoField * f = static_cast<SoField *>(fieldtype.createInstance());
  if (!f->read(in, fieldname)) {
    delete f;
    return FALSE;
  }

  f->setContainer(this);
  this->classfielddata = new SoFieldData;
  this->classfielddata->addField(this, fieldname, f);

#undef READ_VAL

  return TRUE;
}

// Overridden from SoBase to make sure we're accounted for, even
// though we -- as a container for a global field -- only exists
// through a field-to-field connection.
void
SoGlobalField::addWriteReference(SoOutput * out, SbBool COIN_UNUSED_ARG(isfromfield))
{
  assert(this->classfielddata);
  inherited::addWriteReference(out, FALSE);
}


// Overridden from parent class to have the field type written.
void
SoGlobalField::writeInstance(SoOutput * out)
{
  assert(this->classfielddata);

  if (this->writeHeader(out, FALSE, FALSE)) return;

  SoField * f = this->getGlobalField();
  assert(f);

  if (!out->isBinary()) out->indent();
  out->write("type");
  if (!out->isBinary()) out->write(' ');
  out->write(f->getTypeId().getName());
  if (!out->isBinary()) out->write('\n');

  SbBool isdefault = f->isDefault();
  f->setDefault(FALSE);
  inherited::writeInstance(out);
  f->setDefault(isdefault);

  this->writeFooter(out);
}

// Returns a pointer to the field data storage object for this
// instance. The SoFieldData object will always contain only a single
// field.
const SoFieldData *
SoGlobalField::getFieldData(void) const
{
  return this->classfielddata;
}
