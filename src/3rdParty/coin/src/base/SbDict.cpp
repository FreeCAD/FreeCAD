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
  \class SbDict SbDict.h Inventor/SbDict.h
  \brief The SbDict class organizes a dictionary of keys and values.

  \ingroup coin_base

  It uses hashing to quickly insert and find entries in the dictionary.
  An entry consists of an unique key and a generic pointer.
*/

/*!
  \typedef uintptr_t SbDictKeyType

  The type definition for a dictionary key.
*/

/*!
  \typedef void SbDictApplyFunc(SbDictKeyType key, void * value)

  The type definition of the function to be applied to each entry.
*/

/*!
  \typedef void SbDictApplyDataFunc(SbDictKeyType key, void * value, void * data)

  The type definition of the function with associated data that is to be
  applied to each entry.
*/

/*!
  \typedef SbDictKeyType SbDictHashingFunc(const SbDictKeyType key)

  The type definition of a dictionary hashing function.
*/

// *************************************************************************

#define COIN_ALLOW_SBDICT
#include <Inventor/SbDict.h>
#undef COIN_ALLOW_SBDICT

#include <cassert>

#define COIN_ALLOW_CC_HASH /* Hack to get around include protection
                              for obsoleted ADT. */
#include <Inventor/C/base/hash.h>
#undef COIN_ALLOW_CC_HASH
#include <Inventor/lists/SbPList.h>
#include <Inventor/C/base/memalloc.h>

#include "SbBasicP.h"

// *************************************************************************

/*!
  Constructor with \a entries specifying the initial number of buckets
  in the hash list -- so it need to be larger than 0. Other than this,
  no special care needs to be taken in choosing the value since it is
  always rounded up to the nearest power of two.
*/
SbDict::SbDict(const int entries)
{
  assert(entries > 0);
  this->hashtable = cc_hash_construct(entries, 0.75f);
}

/*!
  Copy constructor.
*/
SbDict::SbDict(const SbDict & from)
{
  this->hashtable = NULL;
  this->operator=(from);
}

/*!
  Destructor.
*/
SbDict::~SbDict()
{
  cc_hash_destruct(this->hashtable);
}

extern "C" {

/*
  Callback for copying values from one SbDict to another.
*/
static
void
copyval(SbDictKeyType key, void * value, void * data)
{
  SbDict * thisp = static_cast<SbDict *>(data);
  thisp->enter(key, value);
}

} // extern "C"

/*!
  Make a shallow copy of the contents of dictionary \a from into this
  dictionary.
*/
SbDict &
SbDict::operator=(const SbDict & from)
{
  if (this->hashtable) {
    // clear old values
    this->clear();
    cc_hash_destruct(this->hashtable);
  }
  this->hashtable = cc_hash_construct(cc_hash_get_num_elements(from.hashtable), 0.75f);
  from.applyToAll(copyval, this);
  return *this;
}

/*!
  Clear all entries in the dictionary.
*/
void
SbDict::clear(void)
{
  cc_hash_clear(this->hashtable);
}

/*!
  Inserts a new entry into the dictionary. \a key should be
  a unique number, and \a value is the generic user data.

  If \a key does not exist in the dictionary, a new entry
  is created and \c TRUE is returned. Otherwise, the generic user
  data is changed to \a value, and \c FALSE is returned.
*/
SbBool
SbDict::enter(const Key key, void * const value)
{
  return cc_hash_put(this->hashtable, key, value);
}

/*!
  Searches for \a key in the dictionary. If an entry with this
  key exists, \c TRUE is returned and the entry value is returned
  in \a value. Otherwise, \c FALSE is returned.
*/
SbBool
SbDict::find(const Key key, void *& value) const
{
  return cc_hash_get(this->hashtable, key, &value);
}

/*!
  Removes the entry with key \a key. \c TRUE is returned if an entry
  with this key was present, \c FALSE otherwise.
*/
SbBool
SbDict::remove(const Key key)
{
  return cc_hash_remove(this->hashtable, key);
}


// needed to support the extra applyToAll function. The actual
// function pointer is supplied as the closure pointer, and we just
// call that function from our dummy callback. This is needed since
// cc_hash only supports one apply function type.
extern "C" {
typedef void sbdict_dummy_apply_func(SbDict::Key, void *);

static void
sbdict_dummy_apply(SbDict::Key key, void * value, void * closure)
{
  sbdict_dummy_apply_func * func = (sbdict_dummy_apply_func*) closure;
  func(key, value);
}
}
/*!
  Applies \a rtn to all entries in the dictionary.
*/
void
SbDict::applyToAll(SbDictApplyFunc * rtn) const
{
  cc_hash_apply(this->hashtable, sbdict_dummy_apply, function_to_object_cast<void *>(rtn));
}

/*!
  \overload
*/
void
SbDict::applyToAll(SbDictApplyDataFunc * rtn, void * data) const
{
  cc_hash_apply(this->hashtable, static_cast<cc_hash_apply_func *>(rtn), data);
}

typedef struct {
  SbPList * keys;
  SbPList * values;
} sbdict_makeplist_data;

extern "C" {

static void
sbdict_makeplist_cb(SbDict::Key key, void * value, void * closure)
{
  sbdict_makeplist_data * data = static_cast<sbdict_makeplist_data *>(closure);
  data->keys->append(reinterpret_cast<void *>(key));
  data->values->append(value);
}

} // extern "C"

/*!
  Creates lists with all entries in the dictionary.
*/
void
SbDict::makePList(SbPList & keys, SbPList & values)
{
  sbdict_makeplist_data applydata;
  applydata.keys = &keys;
  applydata.values = &values;

  cc_hash_apply(this->hashtable, static_cast<cc_hash_apply_func *>(sbdict_makeplist_cb), &applydata);
}

/*!
  Sets a new hashing function for this dictionary. Default
  hashing function just returns the key.

  If you find that items entered into the dictionary seems to make
  clusters in only a few buckets, you should try setting a hashing
  function. If you're for instance using strings, you could use the
  static SbString::hash() function (you'd need to make a static function
  that will cast from SbDict::Key to char * of course).

  This function is not part of the OIV API.
*/
void
SbDict::setHashingFunction(SbDictHashingFunc * func)
{
  cc_hash_set_hash_func(this->hashtable, static_cast<cc_hash_func *>(func));
}
