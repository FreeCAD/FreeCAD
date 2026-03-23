#ifndef COIN_SBSTRING_H
#define COIN_SBSTRING_H

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

#include <cstdarg>
#include <cstdio>

#include <Inventor/system/inttypes.h>
#include <Inventor/C/base/string.h>

#ifdef COIN_INTERNAL
 #define COIN_ALLOW_SBINTLIST
 #include <Inventor/lists/SbIntList.h>
 #undef COIN_ALLOW_SBINTLIST
#else
 #include <Inventor/lists/SbIntList.h>
#endif // COIN_INTERNAL

// *************************************************************************

class COIN_DLL_API SbString {
public:
  SbString(void) { cc_string_construct(&this->str); }

  SbString(const char * s)
  { cc_string_construct(&this->str); cc_string_set_text(&this->str, s); }

  SbString(const wchar_t * s)
  { cc_string_construct(&this->str); cc_string_set_wtext(&this->str, s); }

  SbString(const char * s, int start, int end)
  { cc_string_construct(&this->str); cc_string_set_subtext(&this->str, s, start, end); }

  SbString(const SbString & s)
  { cc_string_construct(&this->str); cc_string_set_string(&this->str, &s.str); }

  SbString(const int digits)
  { cc_string_construct(&this->str); cc_string_set_integer(&this->str, digits); }

  ~SbString() { cc_string_clean(&this->str); }

  uint32_t hash(void) const { return cc_string_hash(&this->str); }
  static uint32_t hash(const char * s) { return cc_string_hash_text(s); }

  int getLength(void) const { return cc_string_length(&this->str); }

  void makeEmpty(SbBool freeold = TRUE)
  {
    if ( freeold ) cc_string_clear(&this->str);
    else cc_string_clear_no_free(&this->str);
  }

  const char * getString(void) const { return cc_string_get_text(&this->str); }

  SbString getSubString(int startidx, int endidx = -1) const
  {
    SbString s;
    cc_string_set_subtext(&s.str, cc_string_get_text(&this->str), startidx, endidx);
    return s;
  }
  void deleteSubString(int startidx, int endidx = -1)
  {
    cc_string_remove_substring(&this->str, startidx, endidx);
  }

  void addIntString(const int value) { cc_string_append_integer(&this->str, value); }

  char operator[](int index) const { return this->str.pointer[index]; }

  SbString & operator=(const char * s)
  { cc_string_set_text(&this->str, s); return *this; }
  SbString & operator=(const SbString & s)
  { cc_string_set_text(&this->str, s.str.pointer); return *this; }

  SbString & operator+=(const char * s)
  { cc_string_append_text(&this->str, s); return *this; }
  SbString & operator+=(const SbString & s)
  { cc_string_append_string(&this->str, &s.str); return *this; }
  SbString & operator+=(const char c)
  { cc_string_append_char(&this->str, c); return *this; }

  int operator!(void) const { return ! cc_string_is(&this->str); }

  int compareSubString(const char * text, int offset = 0) const
  { return cc_string_compare_subtext(&this->str, text, offset); }

  SbString & sprintf(const char * formatstr, ...)
  {
    va_list args; va_start(args, formatstr);
    cc_string_vsprintf(&this->str, formatstr, args);
    va_end(args); return *this;
  }
  SbString & vsprintf(const char * formatstr, va_list args)
  { cc_string_vsprintf(&this->str, formatstr, args); return *this; }

  void apply(char (*func)(char input)) {
    cc_string_apply(&this->str, reinterpret_cast<cc_apply_f>(func));
  }

  int find(const SbString & s) const;
  SbBool findAll(const SbString & s, SbIntList & found) const;

  SbString lower() const;
  SbString upper() const;

  void print(std::FILE * fp) const;

  friend int operator==(const SbString & sbstr, const char * s);
  friend int operator==(const char * s, const SbString & sbstr);
  friend int operator==(const SbString & str1, const SbString & str2);
  friend int operator!=(const SbString & sbstr, const char * s);
  friend int operator!=(const char * s, const SbString & sbstr);
  friend int operator!=(const SbString & str1, const SbString & str2);
  friend int operator<(const SbString & sbstr, const char * s);
  friend int operator<(const char * s, const SbString & sbstr);
  friend int operator<(const SbString & str1, const SbString & str2);
  friend int operator>(const SbString & sbstr, const char * s);
  friend int operator>(const char * s, const SbString & sbstr);
  friend int operator>(const SbString & str1, const SbString & str2);
  friend const SbString operator+(const SbString & str1, const SbString & str2);
  friend const SbString operator+(const SbString & sbstr, const char * s);
  friend const SbString operator+(const char * s, const SbString & sbstr);

private:
  struct cc_string str;
};

inline int operator==(const SbString & sbstr, const char * s)
{ return (cc_string_compare_text(sbstr.str.pointer, s) == 0); }
inline int operator==(const char * s, const SbString & sbstr)
{ return (cc_string_compare_text(s, sbstr.str.pointer) == 0); }
inline int operator==(const SbString & str1, const SbString & str2)
{ return (cc_string_compare_text(str1.str.pointer, str2.str.pointer) == 0); }

inline int operator!=(const SbString & sbstr, const char * s)
{ return (cc_string_compare_text(sbstr.str.pointer, s) != 0); }
inline int operator!=(const char * s, const SbString & sbstr)
{ return (cc_string_compare_text(s, sbstr.str.pointer) != 0); }
inline int operator!=(const SbString & str1, const SbString & str2)
{ return (cc_string_compare_text(str1.str.pointer, str2.str.pointer) != 0); }

inline int operator<(const SbString & sbstr, const char * s)
{ return (cc_string_compare_text(sbstr.str.pointer, s) < 0); }
inline int operator<(const char * s, const SbString & sbstr)
{ return (cc_string_compare_text(s, sbstr.str.pointer) < 0); }
inline int operator<(const SbString & str1, const SbString & str2)
{ return (cc_string_compare_text(str1.str.pointer, str2.str.pointer) < 0); }

inline int operator>(const SbString & sbstr, const char * s)
{ return (cc_string_compare_text(sbstr.str.pointer, s) > 0); }
inline int operator>(const char * s, const SbString & sbstr)
{ return (cc_string_compare_text(s, sbstr.str.pointer) > 0); }
inline int operator>(const SbString & str1, const SbString & str2)
{ return (cc_string_compare_text(str1.str.pointer, str2.str.pointer) > 0); }

inline const SbString operator+(const SbString & str1, const SbString & str2)
{ 
  SbString newstr(str1);
  newstr += str2;
  return newstr;
}
inline const SbString operator+(const SbString & sbstr, const char * s)
{
  SbString newstr(sbstr);
  newstr += s;
  return newstr;
}
inline const SbString operator+(const char * s, const SbString & sbstr)
{
  SbString newstr(s);
  newstr += sbstr;
  return newstr;
}

#ifndef COIN_INTERNAL
// For Open Inventor compatibility.
#include <Inventor/SbName.h>
#endif // !COIN_INTERNAL

#endif // !COIN_SBSTRING_H
