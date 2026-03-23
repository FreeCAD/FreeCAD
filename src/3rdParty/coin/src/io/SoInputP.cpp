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


#ifdef HAVE_CONFIG_H
#include "config.h"
#endif // HAVE_CONFIG_H

#include <Inventor/SoInput.h>

#include "io/SoInputP.h"
#include "io/SoInput_FileInfo.h"

// *************************************************************************

SbBool
SoInputP::debug(void)
{
  static int dbg = -1;
  if (dbg == -1) {
    const char * env = coin_getenv("COIN_DEBUG_IMPORT");
    dbg = (env && (atoi(env) > 0)) ? 1 : 0;
  }
  return dbg;
}

SbBool
SoInputP::debugBinary(void)
{
  static int debug = -1;
  if (debug == -1) {
    const char * env = coin_getenv("COIN_DEBUG_BINARY_INPUT");
    debug = (env && (atoi(env) > 0)) ? 1 : 0;
  }
  return debug ? TRUE : FALSE;
}

// *************************************************************************
/*
  Important note: Up until Coin 3.1.1 we used to have a bug in SoInput
  when reading files from other files (SoFile/SoVRMLInline++). The SoInput
  dictionary was global for all files pushed onto the SoInput filestack,
  and DEFs in extra files could therefore overwrite DEFs in the original
  file. This minimal case reproduces this bug:

  #Inventor V2.1 ascii

  Switch {
    whichChild -1
    DEF cube Cube {}
  }

  File { name "minimal_ref.iv" }
  USE cube

  minimal_ref.iv looks something like this:

  #Inventor V2.1 ascii
  DEF cube Info {}

  In older versions of Coin you'll not see the Cube when loading the first
  file.

 */
// *************************************************************************


// Helper function that pops the stack when the current file is at
// EOF.  Then it returns the file at the top of the stack.
SoInput_FileInfo *
SoInputP::getTopOfStackPopOnEOF(void)
{
  SoInput_FileInfo * fi = owner->getTopOfStack();
  assert(fi); // Should always have a top of stack, because the last
              // element on the stack is never removed until the
              // SoInput is closed

  // Pop the stack if end of current file
  if (fi->isEndOfFile()) {
    (void) owner->popFile(); // Only pops if more than one file is on
                             // the stack.
    fi = owner->getTopOfStack();
    assert(fi);
  }

  return fi;
}

// Helperfunctions to handle different filetypes (Inventor, VRML 1.0
// and VRML 2.0).
//
// VRML 1.0 identifiers are defined as:
//
//  VRML 1.0 Node names must not begin with a digit, and must
//  not contain spaces or control characters, single or double
//  quote characters, backslashes, curly braces, the sharp (#)
//  character, the plus (+) character or the period character.
//
//  Field names start with lower case letters, Node types start
//  with upper case. The remainder of the characters may be any
//  printable ascii (21H-7EH) except curly braces {}, square
//  brackets [], single ' or double " quotes, sharp #, backslash
//  \\ plus +, period . or ampersand &.
//
//  In addition to this, we found ',', '(', ')' and '|' to be
//  invalid characters in VRML 1.0 names. This was made apparent 
//  when reading the following fields on an unknown node and
//  bit masks:
//
//   fields [SFString test, SFFloat length]
//
//   FontStyle { family SANS style (BOLD|ITALIC) size 10 }
//
//  If ',' is to be a valid character in a name, then the name
//  of the first field would become 'test,', and not just 'test'.
//  Likewise, the name of the first bit in the bitmask would
//  become 'BOLD|ITALIC)' instead of just 'BOLD'.
//
// The grammar for VRML2 identifiers is:
//
//  nodeNameId ::= Id ;
//  nodeTypeId ::= Id ;
//  fieldId ::= Id ;
//
//  Id ::= IdFirstChar | IdFirstChar IdRestChars ;
//
//  IdFirstChar ::= Any ISO-10646 character encoded using UTF-8
//  except: 0x30-0x39, 0x0-0x20, 0x22, 0x23, 0x27, 0x28, 0x29, 0x2b,
//  0x2c, 0x2d, 0x2e, 0x5b, 0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x7f ;
//
//  IdRestChars ::= Any number of ISO-10646 characters except:
//  0x0-0x20, 0x22, 0x23, 0x27, 0x28, 0x29, 0x2c, 0x2e, 0x5b,
//  0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x7f ;

SbBool
SoInputP::isNameStartChar(unsigned char c, SbBool validIdent)
{
  if (validIdent) return SbName::isIdentStartChar(c);
  return (c > 0x20); // Not control characters
}

SbBool
SoInputP::isNameStartCharVRML1(unsigned char c, SbBool validIdent)
{
  static unsigned char invalid_vrml1_table[256];
  static unsigned char valid_ident_invalid_vrml1_table[256];

  static int isNameStartCharVRML1Initialized = FALSE;
  if (!isNameStartCharVRML1Initialized) {
    const unsigned char invalid_vrml1[] = {
      0x22, 0x23, 0x27, 0x28, 0x29, 0x2b, 0x2c, 0x2e, 0x5c, 0x7b, 0x7c, 0x7d, 0x00 }; // 0x7d = 125
    //'"',  '#',  ''',  '(',  ')',  '+',  ',',  '.',  '\',  '{',  '|',  '}'

    // Differences from invalid_vrml1: '&' , '[', and ']' are now invalid
    const unsigned char valid_ident_invalid_vrml1[] = {
      0x22, 0x23, 0x26, 0x27, 0x28, 0x29, 0x2b, 0x2c, 0x2e, 0x5b, 0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x00 }; // 0x7d = 125
    //'"',  '#',   '&', ''',  '(',  ')',  '+',  ',',  '.',  '[',  '\',   ']',  '{',  '|',  '}'

    (void)memset(invalid_vrml1_table, 0, 256);
    (void)memset(valid_ident_invalid_vrml1_table, 0, 256);

    const unsigned char * ptr = invalid_vrml1;
    while (*ptr) { invalid_vrml1_table[*ptr] = 1; ++ptr; }
    ptr = valid_ident_invalid_vrml1;
    while (*ptr) { valid_ident_invalid_vrml1_table[*ptr] = 1; ++ptr; }

    isNameStartCharVRML1Initialized = TRUE;
  }

  if (c <= 0x20) return FALSE; // Control characters
  if (c >= 0x30 && c <= 0x39) return FALSE; // Digits

  if (validIdent) return (valid_ident_invalid_vrml1_table[c] == 0);
  return (invalid_vrml1_table[c] == 0);
}

SbBool
SoInputP::isNameStartCharVRML2(unsigned char c, SbBool validIdent)
{
  static unsigned char invalid_vrml2_table[256];
  static unsigned char valid_ident_invalid_vrml2_table[256];

  static int isNameStartCharVRML2Initialized = FALSE;
  if (!isNameStartCharVRML2Initialized) {
    const unsigned char invalid_vrml2[] = {
      0x22, 0x23, 0x27, 0x28, 0x29, 0x2b, 0x2c, 0x2d, 0x2e, 0x5b, 0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x7f, 0x00 }; // 0x7f = 127
    //'"',  '#',  ''',  '(',  ')',  '+',  ',',  '-',  '.',  '[',  '\',  ']',  '{',  ,'|',  '}',  ''

    const unsigned char * valid_ident_invalid_vrml2 = invalid_vrml2;

    (void)memset(invalid_vrml2_table, 0, 256);
    (void)memset(valid_ident_invalid_vrml2_table, 0, 256);

    const unsigned char * ptr = invalid_vrml2;
    while (*ptr) { invalid_vrml2_table[*ptr] = 1; ++ptr; }
    ptr = valid_ident_invalid_vrml2;
    while (*ptr) { valid_ident_invalid_vrml2_table[*ptr] = 1; ++ptr; }

    isNameStartCharVRML2Initialized = TRUE;
  }

  if (c <= 0x20) return FALSE; // Control characters
  if (c >= 0x30 && c <= 0x39) return FALSE; // Digits

  if (validIdent) return (valid_ident_invalid_vrml2_table[c] == 0);

  // For Coin to be able to load VRML97 (invalid) files that have
  // been generated with illegal names, '+' is considered a valid
  // startcharacter.
  static int non_strict = -1;
  if (non_strict == -1) {
    const char * env = coin_getenv("COIN_NOT_STRICT_VRML97");
    non_strict = env && (atoi(env) > 0);
  }

  if (c == '+' && non_strict) // '+' is considered valid
    return TRUE;

  return (invalid_vrml2_table[c] == 0);
}

// Helperfunction to handle different filetypes (Inventor, VRML 1.0
// and VRML 2.0).
//
// See SoInputP::isIdentStartChar for more information
SbBool
SoInputP::isNameChar(unsigned char c, SbBool validIdent)
{
  if (validIdent) return SbName::isIdentChar(c);
  return (c > 0x20); // Not control characters
}

SbBool
SoInputP::isNameCharVRML1(unsigned char c, SbBool validIdent)
{
  static unsigned char invalid_vrml1_table[256];
  static unsigned char valid_ident_invalid_vrml1_table[256];

  static int isNameCharVRML1Initialized = FALSE;
  if (!isNameCharVRML1Initialized) {
    const unsigned char invalid_vrml1[] = {
      0x22, 0x23, 0x27, 0x28, 0x29, 0x2b, 0x2c, 0x2e, 0x5c, 0x7b, 0x7c, 0x7d, 0x00 }; // 0x7d = 125
    //'"',  '#',  ''',  '(',  ')',  '+',  ',',  '.',  '\',  '{',  ,'|',  '}'

    // Differences from invalid_vrml1: '&' , '[', and ']' are now invalid
    const unsigned char valid_ident_invalid_vrml1[] = {
      0x22, 0x23, 0x26, 0x27, 0x28, 0x29, 0x2b, 0x2c, 0x2e, 0x5b, 0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x00 }; // 0x7d = 125
    //'"',  '#',   '&', ''',  '(',  ')',  '+',  ',',  '.',  '[',  '\',   ']',  '{',  ,'|',  '}'

    (void)memset(invalid_vrml1_table, 0, 256);
    (void)memset(valid_ident_invalid_vrml1_table, 0, 256);

    const unsigned char * ptr = invalid_vrml1;
    while (*ptr) { invalid_vrml1_table[*ptr] = 1; ++ptr; }
    ptr = valid_ident_invalid_vrml1;
    while (*ptr) { valid_ident_invalid_vrml1_table[*ptr] = 1; ++ptr; }

    isNameCharVRML1Initialized = TRUE;
  }

  if (c <= 0x20) return FALSE; // Control characters

  if (validIdent) return (valid_ident_invalid_vrml1_table[c] == 0);
  return (invalid_vrml1_table[c] == 0);
}

SbBool
SoInputP::isNameCharVRML2(unsigned char c, SbBool validIdent)
{
  static unsigned char invalid_vrml2_table[256];
  static unsigned char valid_ident_invalid_vrml2_table[256];

  static int isNameCharVRML2Initialized = FALSE;
  if (!isNameCharVRML2Initialized) {
    // Compared to isIdentStartChar, '+' and '-' have now become valid characters.
    const unsigned char invalid_vrml2[] = {
      0x22, 0x23, 0x27, 0x28, 0x29, 0x2c, 0x2e, 0x5b, 0x5c, 0x5d, 0x7b, 0x7c, 0x7d, 0x7f, 0x00 }; // 0x7f = 127
    //'"',  '#',  ''',  '(',  ')',  ',',  '.',  '[',  '\',  ']',  '{',  ,'|',  '}',  ''

    const unsigned char * valid_ident_invalid_vrml2 = invalid_vrml2;

    (void)memset(invalid_vrml2_table, 0, 256);
    (void)memset(valid_ident_invalid_vrml2_table, 0, 256);

    const unsigned char * ptr = invalid_vrml2;
    while (*ptr) { invalid_vrml2_table[*ptr] = 1; ++ptr; }
    ptr = valid_ident_invalid_vrml2;
    while (*ptr) { valid_ident_invalid_vrml2_table[*ptr] = 1; ++ptr; }

    isNameCharVRML2Initialized = TRUE;
  }

  if (c <= 0x20) return FALSE; // Control characters

  if (validIdent) return (valid_ident_invalid_vrml2_table[c] == 0);
  return (invalid_vrml2_table[c] == 0);
}
