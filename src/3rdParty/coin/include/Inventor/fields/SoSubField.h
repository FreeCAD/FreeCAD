#ifndef COIN_SOSUBFIELD_H
#define COIN_SOSUBFIELD_H

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

#include <Inventor/SbBasic.h> // for SO__QUOTE() definition
#include <Inventor/SbName.h> // SoType::createType() needs to know SbName.
#include <Inventor/C/tidbits.h>
#include <cassert>

#ifndef COIN_INTERNAL
// Added to be Inventor compliant.
#include <Inventor/fields/SoField.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#endif // !COIN_INTERNAL

/**************************************************************************
 *
 * Header macros for single-value fields.
 *
 **************************************************************************/

#define SO_SFIELD_CONSTRUCTOR_HEADER(_class_) \
public: \
  _class_(void); \
  virtual ~_class_()


#define SO_SFIELD_REQUIRED_HEADER(_class_) \
private: \
  static SoType classTypeId; \
  static void atexit_cleanup(void) { SoType::removeType(_class_::classTypeId.getName()); _class_::classTypeId STATIC_SOTYPE_INIT; } \
public: \
  static void * createInstance(void); \
  static SoType getClassTypeId(void); \
  virtual SoType getTypeId(void) const; \
 \
  virtual void copyFrom(const SoField & field); \
  const _class_ & operator=(const _class_ & field); \
  virtual SbBool isSame(const SoField & field) const


#define PRIVATE_SFIELD_IO_HEADER() \
private: \
  virtual SbBool readValue(SoInput * in); \
  virtual void writeValue(SoOutput * out) const


#define SO_SFIELD_VALUE_HEADER(_class_, _valtype_, _valref_) \
  PRIVATE_SFIELD_IO_HEADER(); \
protected: \
  _valtype_ value; \
 \
public: \
  _valref_ getValue(void) const { this->evaluate(); return this->value; } \
  void setValue(_valref_ newvalue); \
  _valref_ operator=(_valref_ newvalue) { this->setValue(newvalue); return this->value; } \
 \
  int operator==(const _class_ & field) const; \
  int operator!=(const _class_ & field) const { return ! operator==(field); }


// FIXME: is really the operator=() definition below necessary?
// 19991226 mortene.
#define SO_SFIELD_DERIVED_VALUE_HEADER(_class_, _valtype_, _valref_) \
  PRIVATE_SFIELD_IO_HEADER(); \
public: \
  _valref_ operator=(_valref_ newvalue) { this->setValue(newvalue); return this->value; }



#define SO_SFIELD_HEADER(_class_, _valtype_, _valref_) \
  SO_SFIELD_CONSTRUCTOR_HEADER(_class_); \
  SO_SFIELD_REQUIRED_HEADER(_class_); \
  SO_SFIELD_VALUE_HEADER(_class_, _valtype_, _valref_)


#define SO_SFIELD_DERIVED_HEADER(_class_, _valtype_, _valref_) \
  SO_SFIELD_CONSTRUCTOR_HEADER(_class_); \
  SO_SFIELD_REQUIRED_HEADER(_class_); \
  SO_SFIELD_DERIVED_VALUE_HEADER(_class_, _valtype_, _valref_)



/**************************************************************************
 *
 * Source macros for single-value fields.
 *
 **************************************************************************/

#define PRIVATE_FIELD_INIT_CLASS(_class_, _classname_, _parent_, _createfunc_) \
  do { \
    /* Make sure superclass get initialized before subclass. */ \
    assert(_parent_::getClassTypeId() != SoType::badType()); \
    /* Make sure we only initialize once. */ \
    assert(_class_::classTypeId == SoType::badType()); \
    _class_::classTypeId = \
      SoType::createType(_parent_::getClassTypeId(), _classname_, _createfunc_); \
    cc_coin_atexit_static_internal \
      ( \
       _class_::atexit_cleanup \
       ); \
  } WHILE_0



#define SO_SFIELD_INIT_CLASS(_class_, _parent_) \
  do { \
    const char * classname = SO__QUOTE(_class_); \
    PRIVATE_FIELD_INIT_CLASS(_class_, classname, _parent_, &_class_::createInstance); \
  } WHILE_0

#define SO_SFIELD_CONSTRUCTOR_SOURCE(_class_) \
_class_::_class_(void) { assert(_class_::classTypeId != SoType::badType()); } \
_class_::~_class_() { }



#define SO_SFIELD_VALUE_SOURCE(_class_, _valtype_, _valref_) \
void \
_class_::setValue(_valref_ valuearg) { \
  this->value = valuearg; \
  this->valueChanged(); \
} \
 \
SbBool \
_class_::operator==(const _class_ & field) const \
{ \
  return (this->getValue() == field.getValue()); \
}


#define PRIVATE_TYPEID_SOURCE(_class_) \
 \
/*!
  Returns the type identification of an object derived from a \
  class inheriting SoField. This is used for runtime type checking and \
  "downward" casting. \
*/ \
SoType _class_::getTypeId(void) const { return _class_::classTypeId; } \
 \
/*! \
  This static method returns the SoType object associated with \
  objects of this class. \
*/ \
SoType _class_::getClassTypeId(void) { return _class_::classTypeId; } \
/*! \COININTERNAL \
 \
  Creates a new instance of the class type corresponding to the SoType object. \
*/ \
void * _class_::createInstance(void) { return new _class_; } \
SoType _class_::classTypeId STATIC_SOTYPE_INIT


#define PRIVATE_EQUALITY_SOURCE(_class_) \
void \
_class_::copyFrom(const SoField & field) \
{ \
  this->operator=(static_cast<const _class_ &>(field)); \
} \
 \
SbBool \
_class_::isSame(const SoField & field) const \
{ \
  if (field.getTypeId() != this->getTypeId()) return FALSE; \
  return this->operator==(static_cast<const _class_ &>(field)); \
}



#define SO_SFIELD_REQUIRED_SOURCE(_class_) \
PRIVATE_TYPEID_SOURCE(_class_); \
PRIVATE_EQUALITY_SOURCE(_class_); \
 \
const _class_ & \
_class_::operator=(const _class_ & field) \
{ \
  this->setValue(field.getValue()); \
  return *this; \
}



#define SO_SFIELD_SOURCE(_class_, _valtype_, _valref_) \
  SO_SFIELD_CONSTRUCTOR_SOURCE(_class_); \
  SO_SFIELD_VALUE_SOURCE(_class_, _valtype_, _valref_); \
  SO_SFIELD_REQUIRED_SOURCE(_class_)



#define SO_SFIELD_DERIVED_SOURCE(_class_, _valtype_, _valref_) \
  SO_SFIELD_CONSTRUCTOR_SOURCE(_class_); \
  SO_SFIELD_REQUIRED_SOURCE(_class_)


/**************************************************************************
 *
 * Header macros for multiple-value fields.
 *
 **************************************************************************/

#define PRIVATE_MFIELD_IO_HEADER() \
private: \
  virtual SbBool read1Value(SoInput * in, int idx); \
  virtual void write1Value(SoOutput * out, int idx) const



#define SO_MFIELD_VALUE_HEADER(_class_, _valtype_, _valref_) \
  PRIVATE_MFIELD_IO_HEADER(); \
protected: \
  virtual void deleteAllValues(void); \
  virtual void copyValue(int to, int from); \
  virtual int fieldSizeof(void) const; \
  virtual void * valuesPtr(void); \
  virtual void setValuesPtr(void * ptr); \
  virtual void allocValues(int num); \
 \
  _valtype_ * values; \
public: \
  _valref_ operator[](const int idx) const \
    { this->evaluate(); return this->values[idx]; } \
/*! \
  Returns a pointer to the values array. \
*/ \
  const _valtype_ * getValues(const int start) const \
    { this->evaluate(); return const_cast<const _valtype_ *>(this->values + start); } \
  int find(_valref_ value, SbBool addifnotfound = FALSE); \
  void setValues(const int start, const int num, const _valtype_ * newvals); \
  void set1Value(const int idx, _valref_ value); \
  void setValue(_valref_ value); \
  _valref_ operator=(_valref_ val) { this->setValue(val); return val; } \
  SbBool operator==(const _class_ & field) const; \
  SbBool operator!=(const _class_ & field) const { return !operator==(field); } \
  _valtype_ * startEditing(void) { this->evaluate(); return this->values; } \
  void finishEditing(void) { this->valueChanged(); }

#define SO_MFIELD_DERIVED_VALUE_HEADER(_class_, _valtype_, _valref_) \
  PRIVATE_MFIELD_IO_HEADER(); \
public: \
  _valref_ operator=(_valref_ val) { this->setValue(val); return val; }



#define SO_MFIELD_HEADER(_class_, _valtype_, _valref_) \
  SO_SFIELD_CONSTRUCTOR_HEADER(_class_); \
  SO_SFIELD_REQUIRED_HEADER(_class_); \
  SO_MFIELD_VALUE_HEADER(_class_, _valtype_, _valref_)



#define SO_MFIELD_DERIVED_HEADER(_class_, _valtype_, _valref_) \
  SO_SFIELD_CONSTRUCTOR_HEADER(_class_); \
  SO_SFIELD_REQUIRED_HEADER(_class_); \
  SO_MFIELD_DERIVED_VALUE_HEADER(_class_, _valtype_, _valref_)

#define SO_MFIELD_SETVALUESPOINTER_HEADER(_valtype_) \
  void setValuesPointer(const int num, const _valtype_ * userdata); \
  void setValuesPointer(const int num, _valtype_ * userdata)


/**************************************************************************
 *
 * Source macros for multiple-value fields.
 *
 **************************************************************************/


#define SO_MFIELD_INIT_CLASS(_class_, _parent_) \
  SO_SFIELD_INIT_CLASS(_class_, _parent_)



#define SO_MFIELD_CONSTRUCTOR_SOURCE(_class_) \
_class_::_class_(void) \
{ \
  assert(_class_::classTypeId != SoType::badType()); \
  this->values = NULL; \
} \
 \
_class_::~_class_(void) \
{ \
  this->enableNotify(FALSE); /* Avoid notifying destructed containers. */ \
  this->deleteAllValues(); \
}



#define SO_MFIELD_DERIVED_CONSTRUCTOR_SOURCE(_class_) \
_class_::_class_(void) { } \
_class_::~_class_(void) { }



#define SO_MFIELD_REQUIRED_SOURCE(_class_) \
PRIVATE_TYPEID_SOURCE(_class_); \
PRIVATE_EQUALITY_SOURCE(_class_); \
const _class_ & \
_class_::operator=(const _class_ & field) \
{ \
  /* The allocValues() call is needed, as setValues() doesn't */ \
  /* necessarily make the field's getNum() size become the same */ \
  /* as the second argument (only if it expands on the old size). */ \
  this->allocValues(field.getNum()); \
  \
  this->setValues(0, field.getNum(), field.getValues(0)); \
  return *this; \
}



#define SO_MFIELD_VALUE_SOURCE(_class_, _valtype_, _valref_) \
int \
_class_::fieldSizeof(void) const \
{ \
  return sizeof(_valtype_); \
} \
 \
void * \
_class_::valuesPtr(void) \
{ \
  return static_cast<void *>(this->values); \
} \
 \
void \
_class_::setValuesPtr(void * ptr) \
{ \
  this->values = static_cast<_valtype_ *>(ptr); \
} \
 \
int \
_class_::find(_valref_ value, SbBool addifnotfound) \
{ \
  evaluate(); \
  for (int i=0; i < this->num; i++) if (this->values[i] == value) return i; \
 \
  if (addifnotfound) this->set1Value(this->num, value); \
  return -1; \
} \
 \
void \
_class_::setValues(const int start, const int numarg, const _valtype_ * newvals) \
{ \
  if (start+numarg > this->maxNum) this->allocValues(start+numarg); \
  else if (start+numarg > this->num) this->num = start+numarg; \
 \
  for (int i=0; i < numarg; i++) \
    this->values[i+start] = static_cast<const _valtype_>(newvals[i]); \
  this->setChangedIndices(start, numarg); \
  this->valueChanged(); \
  this->setChangedIndices(); \
} \
 \
void \
_class_::set1Value(const int idx, _valref_ value) \
{ \
  if (idx+1 > this->maxNum) this->allocValues(idx+1); \
  else if (idx+1 > this->num) this->num = idx+1; \
  this->values[idx] = value; \
  this->setChangedIndex(idx); \
  this->valueChanged(); \
  this->setChangedIndices(); \
} \
 \
void \
_class_::setValue(_valref_ value) \
{ \
  this->allocValues(1); \
  this->values[0] = value; \
  this->setChangedIndex(0); \
  this->valueChanged(); \
  this->setChangedIndices(); \
} \
 \
SbBool \
_class_::operator==(const _class_ & field) const \
{ \
  if (this == &field) return TRUE; \
  if (this->getNum() != field.getNum()) return FALSE; \
 \
  const _valtype_ * const lhs = this->getValues(0); \
  const _valtype_ * const rhs = field.getValues(0); \
  for (int i = 0; i < this->num; i++) if (lhs[i] != rhs[i]) return FALSE; \
  return TRUE; \
} \
 \
/*! \COININTERNAL */ \
void \
_class_::deleteAllValues(void) \
{ \
  this->setNum(0); \
} \
 \
/*! This method is used for moving values around internally within a multivalue field. It needs to be overridden in each field so it automatically takes care of running copy constructors where necessary. */ \
void \
_class_::copyValue(int to, int from) \
{ \
  this->values[to] = this->values[from]; \
}


#define SO_MFIELD_ALLOC_SOURCE(_class_, _valtype_) \
void \
_class_::allocValues(int newnum) \
{ \
  /* Important notice: the "malloc-version" of this method is found */ \
  /* in SoMField.cpp. If you make modifications here, do check whether */ \
  /* or not they should be matched with modifications in that method */ \
  /* aswell. */ \
 \
  /* these must be declared here as a gcc 4.0.0 bug workaround */ \
  int i; \
  int oldmaxnum; \
  _valtype_ * newblock; \
  assert(newnum >= 0); \
 \
  this->setChangedIndices(); \
  if (newnum == 0) { \
    if (!this->userDataIsUsed) delete[] this->values; /* don't fetch pointer through valuesPtr() (avoids void* cast) */ \
    this->setValuesPtr(NULL); \
    this->maxNum = 0; \
    this->userDataIsUsed = FALSE; \
  } \
  else if (newnum > this->maxNum || newnum < this->num) { \
    if (this->valuesPtr()) { \
 \
      /* Allocation strategy is to repeatedly double the size of the */ \
      /* allocated block until it will at least match the requested size. */ \
      /* (Unless the requested size is less than what we've got, */ \
      /* then we'll repeatedly halve the allocation size.) */ \
      /* */ \
      /* I think this will handle both cases quite gracefully: */ \
      /* 1) newnum > this->maxNum, 2) newnum < num */ \
      oldmaxnum = this->maxNum; \
      while (newnum > this->maxNum) this->maxNum *= 2; \
      while ((this->maxNum / 2) >= newnum) this->maxNum /= 2; \
 \
      if (oldmaxnum != this->maxNum) { \
        newblock = new _valtype_[this->maxNum]; \
 \
        for (i=0; i < SbMin(this->num, newnum); i++) \
          newblock[i] = this->values[i]; \
 \
        delete[] this->values; /* don't fetch pointer through valuesPtr() (avoids void* cast) */ \
        this->setValuesPtr(newblock); \
        this->userDataIsUsed = FALSE; \
      } \
    } \
    else { \
      this->setValuesPtr(new _valtype_[newnum]); \
      this->userDataIsUsed = FALSE; \
      this->maxNum = newnum; \
    } \
  } \
 \
  this->num = newnum; \
}



#define SO_MFIELD_MALLOC_SOURCE(_class_, _valtype_) \
void \
_class_::allocValues(int number) \
{ \
  SoMField::allocValues(number); \
}



#define SO_MFIELD_SOURCE_MALLOC(_class_, _valtype_, _valref_) \
  SO_MFIELD_REQUIRED_SOURCE(_class_); \
  SO_MFIELD_CONSTRUCTOR_SOURCE(_class_); \
  SO_MFIELD_MALLOC_SOURCE(_class_, _valtype_); \
  SO_MFIELD_VALUE_SOURCE(_class_, _valtype_, _valref_)



#define SO_MFIELD_SOURCE(_class_, _valtype_, _valref_) \
  SO_MFIELD_REQUIRED_SOURCE(_class_); \
  SO_MFIELD_CONSTRUCTOR_SOURCE(_class_); \
  SO_MFIELD_ALLOC_SOURCE(_class_, _valtype_); \
  SO_MFIELD_VALUE_SOURCE(_class_, _valtype_, _valref_)


#define SO_MFIELD_DERIVED_SOURCE(_class_, _valtype_, _valref_) \
  SO_MFIELD_REQUIRED_SOURCE(_class_); \
  SO_MFIELD_DERIVED_CONSTRUCTOR_SOURCE(_class_)

#define SO_MFIELD_SETVALUESPOINTER_SOURCE(_class_, _valtype_, _usertype_) \
void \
_class_::setValuesPointer(const int numarg, _usertype_ * userdata) \
{ \
  this->makeRoom(0); \
  if (numarg > 0 && userdata) { \
    this->values = reinterpret_cast<_valtype_*>(userdata); /* reinterpret_cast is needed for certain special uses of this function, such as SoMFColor */ \
    this->userDataIsUsed = TRUE; \
    this->num = this->maxNum = numarg; \
    this->valueChanged(); \
  } \
} \
void \
_class_::setValuesPointer(const int numarg, const _usertype_ * userdata) \
{ \
  this->setValuesPointer(numarg, const_cast<_usertype_*>(userdata)); \
}

#endif // !COIN_SOSUBFIELD_H
