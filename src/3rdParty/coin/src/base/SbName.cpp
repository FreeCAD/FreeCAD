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
  \class SbName SbName.h Inventor/SbName.h
  \brief The SbName class stores strings by reference.

  \ingroup coin_base

  The class is used by Coin for storing keywords, names and other
  strings. They are stored in a manner where identical strings are
  guaranteed to map to the same memory address (as returned by the
  SbName::getString() method).

  The main advantage of storing identical strings to the same memory
  address is that it simplifies comparison operations, and
  particularly when working with string data as keys in other data
  structures, like e.g. in hash (dictionary) tables.

  Apart from that, mapping identical strings to the same memory
  address can also save on memory resources, and provide runtime
  optimizations. String comparisons for SbName objects are very
  efficient, for instance.


  There is an aspect of using SbName instances that it is important to
  be aware of: since strings are stored \e permanently, using SbName
  instances in code where there is continually changing strings or the
  continual addition of new unique strings will in the end swamp
  memory resources. So where possible, use SbString instances instead.

  \sa SbString
*/

// *************************************************************************

#include <Inventor/SbName.h>

#include <cctype>
#include <cstring>

#include <Inventor/C/tidbits.h>
#include <Inventor/SbString.h>

#include "tidbitsp.h"
#include "base/namemap.h"
#include "coindefs.h"

#ifndef COIN_WORKAROUND_NO_USING_STD_FUNCS
using std::strlen;
using std::strchr;
using std::strcmp;
using std::isdigit;
using std::isalnum;
using std::isalpha;
#endif // !COIN_WORKAROUND_NO_USING_STD_FUNCS

// *************************************************************************

/*!
  This is the default constructor.
*/
SbName::SbName(void)
{
  this->permaaddress = cc_namemap_get_address("");
}

/*!
  Constructor. Adds the \a namestring string to the name table.
*/
SbName::SbName(const char * namestring)
{
  assert(namestring);
  this->permaaddress = cc_namemap_get_address(namestring);
}

/*!
  Constructor. Adds \a str to the name table.
*/
SbName::SbName(const SbString & str)
{
  this->permaaddress = cc_namemap_get_address(str.getString());
}

/*!
  Copy constructor.
*/
SbName::SbName(const SbName & name)
{
  this->permaaddress = name.permaaddress;
}

/*!
  The destructor.
*/
SbName::~SbName()
{
  // No unreferences of memory resources happens here, because strings
  // will be stored permanently for the remaining life of the process.
  //
  // This is how the string mapping feature of SbName is *supposed* to
  // work. The strings should be stored _permanently_, so the return
  // value from SbName::getString() will be valid even after all its
  // SbName-instances have been destructed.
}

/*!
  This method returns pointer to character array for the name.

  The returned memory pointer for the character string is guaranteed
  to be valid for the remaining life time of the process, even after
  all SbName instances referencing the string have been destructed.
*/
const char *
SbName::getString(void) const
{
  return this->permaaddress;
}

/*!
  This method returns the number of characters in the name.
*/
int
SbName::getLength(void) const
{
  // FIXME: shouldn't we cache this value for subsequent faster
  // execution? 20010909 mortene.
  //
  // UPDATE 20030606 mortene: this can easily be done by storing an
  // extra value in the memory chunk allocated inside namemap.c, right
  // before the string itself.
  return static_cast<int>(strlen(this->permaaddress));
}

/*!
  This method checks if the \a c character is a valid identifier start
  character for a name.

  \sa SbBool SbName::isIdentChar(const char c)

*/
SbBool
SbName::isIdentStartChar(const char c)
{
  // There is an important reason why the cast below is necessary:
  // isdigit() et al takes an "int" as input argument. A _signed_ char
  // value for any character above the 127-value ASCII character set
  // will be promoted to a negative integer, which can cause the
  // function to make an array reference that's out of bounds.
  //
  // FIXME: this needs to be fixed other places isdigit() is used,
  // as well as for other is*() function. 20021124 mortene.
  const unsigned char uc = static_cast<unsigned char>(c);

  if (isdigit(uc)) return FALSE;
  return SbName::isIdentChar(c);
}

/*!
  This method checks if the \a c character is a valid character for a
  name.

  \sa SbBool SbName::isIdentStartChar(const char c)
*/
SbBool
SbName::isIdentChar(const char c)
{
  // FIXME: isalnum() takes the current locale into account. This can
  // lead to "interesting" artifacts. We very likely need to audit and
  // fix our isalnum() calls in the Coin sourcecode to behave in the
  // exact manner that we expect them to. 20020319 mortene.
  return (isalnum(c) || c == '_');
}

/*!
  Returns \c TRUE if the given character is valid for use as the first
  character of a name for an object derived from a class inheriting
  SoBase.

  SoBase derived objects needs to be named in a manner which will not
  clash with the special characters reserved as tokens in the syntax
  rules of Open Inventor and VRML files.

  Legal characters for the first character of an SoBase object name is
  underscore ("_") and any uppercase and lowercase alphabetic
  character from the ASCII character set (i.e. A-Z and a-z).

  This method is not part of the original Open Inventor API.

  \sa isBaseNameChar()
*/
SbBool
SbName::isBaseNameStartChar(const char c)
{
  // FIXME: it seems silly to have this function here, instead of in
  // SoBase. 20040611 mortene.

  // FIXME: isalpha() takes the current locale into account. This can
  // lead to "interesting" artifacts. We very likely need to audit and
  // fix our isalpha() calls in the Coin sourcecode to behave in the
  // exact manner that we expect them to. 20020319 mortene.
  if (c == '_' || (coin_isascii(c) && isalpha(c))) return TRUE;
  return FALSE;
}

/*!
  Returns \c TRUE if the given character is valid for use in naming
  object instances of classes derived from SoBase.

  SoBase derived objects needs to be named in a manner which will not
  clash with the special characters reserved as tokens in the syntax
  rules of Open Inventor and VRML files.

  Legal characters to use for an SoBase object name is any character
  from the ASCII character set from and including character 33 (hex
  0x21) to and including 126 (hex 0x7e), \e except single and double
  apostrophes, the plus sign and punctuation, backslash and the curly
  braces.

  This method is not part of the original Open Inventor API.

  \sa isBaseNameStartChar()
*/
SbBool
SbName::isBaseNameChar(const char c)
{
  // FIXME: it seems silly to have this function here, instead of in
  // SoBase. 20040611 mortene.

  static const char invalid[] = "\"\'+.\\{}";
  if (c <= 0x20 || c >= 0x7f || strchr(invalid, c)) return FALSE;
  return TRUE;
}

/*!
  This unary operator results in \c FALSE if the SbName object is
  non-empty and \c TRUE if the SbName object is empty.  An empty name
  contains a null-length string.
*/
int
SbName::operator!(void) const
{
  return this->permaaddress[0] == '\0';
}

/*!
  This operator checks for equality and returns \c TRUE if so, and \c
  FALSE otherwise.
*/
int
operator==(const SbName & lhs, const char * rhs)
{
  return (strcmp(lhs.permaaddress, rhs) == 0);
}

/*!
  This operator checks for equality and returns \c TRUE if so, and \c
  FALSE otherwise.
*/
int
operator==(const char * lhs, const SbName & rhs)
{
  return (strcmp(rhs.permaaddress, lhs) == 0);
}

/*!
  This operator checks for equality and returns \c TRUE if so, and \c
  FALSE otherwise.
*/
int
operator==(const SbName & lhs, const SbName & rhs)
{
  // Due to the nature of permanent unique mappings of same strings to
  // same address in the name hash, we can simple compare pointer
  // addresses.
  return lhs.permaaddress == rhs.permaaddress;
}

/*!
  This operator checks for inequality and returns \c TRUE if so, and
  \c FALSE if the names are equal.
*/
int
operator!=(const SbName & lhs, const char * rhs)
{
  return !(lhs == rhs);
}

/*!
  This operator checks for inequality and returns \c TRUE if so, and
  \c FALSE if the names are equal.
*/
int
operator!=(const char * lhs, const SbName & rhs)
{
  return !(lhs == rhs);
}

/*!
  This operator checks for inequality and returns \c TRUE if so, and
  \c FALSE if the names are equal.
*/
int
operator!=(const SbName & lhs, const SbName & rhs)
{
  return !(lhs == rhs);
}

/*!
  This operator returns a pointer to the character array for the name
  string.  It is intended for implicit use.  Use SbName::getString()
  explicitly instead of this operator - it might be removed later.

  \sa const char * SbName::getString(void)
*/
SbName::operator const char * (void) const
{
  return this->permaaddress;
}


/* anonymous namespace for management of the empty SbName instance */
namespace {

  SbName * emptyname = NULL;

  void
  SbName_atexit(void) {
    delete emptyname;
    emptyname = NULL;
  }
}


/*!
  Returns an empty-string SbName instance.

  \since Coin 3.0
*/

const SbName &
SbName::empty(void) // static
{
  if (emptyname == NULL) {
    emptyname = new SbName("");
    coin_atexit(SbName_atexit, CC_ATEXIT_SBNAME);
  }
  return *emptyname;
}
