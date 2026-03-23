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
#include <config.h>
#endif // HAVE_CONFIG_H

#ifdef HAVE_VRML97

/*!
  \class SoVRMLFontStyle SoVRMLFontStyle.h Inventor/VRMLnodes/SoVRMLFontStyle.h
  \brief The SoVRMLFontStyle class is used to define the current font.

  \ingroup coin_VRMLnodes

  Important note: currently, the SoVRMLText node implementation is not
  complete, and some of the features mentioned in the documentation
  below may not be working yet.

  \WEB3DCOPYRIGHT
  
  \verbatim
  FontStyle { 
    field MFString family       "SERIF"
    field SFBool   horizontal   TRUE
    field MFString justify      "BEGIN"
    field SFString language     ""
    field SFBool   leftToRight  TRUE
    field SFFloat  size         1.0          # (0,)
    field SFFloat  spacing      1.0          # [0,)
    field SFString style        "PLAIN"
    field SFBool   topToBottom  TRUE
  }
  \endverbatim

  <strong>Introduction</strong>

  The FontStyle node defines the size, family, and style used for Text
  nodes, as well as the direction of the text strings and any
  language-specific rendering techniques used for non-English
  text. See SoVRMLText, for a description of the Text node.

  The size field specifies the nominal height, in the local coordinate
  system of the Text node, of glyphs rendered and determines the
  spacing of adjacent lines of text. Values of the size field shall be
  greater than zero.

  The spacing field determines the line spacing between adjacent lines
  of text. The distance between the baseline of each line of text is
  (spacing ? size) in the appropriate direction (depending on other
  fields described below). The effects of the size and spacing field
  are depicted in Figure 6.7 (spacing greater than 1.0). Values of the
  spacing field shall be non-negative.

  FontStyle node example

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/FontStylespacing.gif">
  Figure 6.7 -- Text size and spacing fields
  </center>


  <strong>Font family and style</strong>

  Font attributes are defined with the family and style fields. The
  browser shall map the specified font attributes to an appropriate
  available font as described below.

  The family field contains a case-sensitive MFString value that
  specifies a sequence of font family names in preference order. The
  browser shall search the MFString value for the first font family
  name matching a supported font family. If none of the string values
  matches a supported font family, the default font family "SERIF"
  shall be used. All browsers shall support at least "SERIF" (the
  default) for a serif font such as Times Roman; "SANS" for a
  sans-serif font such as Helvetica; and "TYPEWRITER" for a
  fixed-pitch font such as Courier. An empty family value is identical
  to ["SERIF"].

  The style field specifies a case-sensitive SFString value that may
  be "PLAIN" (the default) for default plain type; "BOLD" for boldface
  type; "ITALIC" for italic type; or "BOLDITALIC" for bold and italic
  type. An empty style value ("") is identical to "PLAIN".


  <strong>Direction and justification</strong>

  The horizontal, leftToRight, and topToBottom fields indicate the
  direction of the text. The horizontal field indicates whether the
  text advances horizontally in its major direction (horizontal =
  TRUE, the default) or vertically in its major direction (horizontal
  = FALSE). The leftToRight and topToBottom fields indicate direction
  of text advance in the major (characters within a single string) and
  minor (successive strings) axes of layout. Which field is used for
  the major direction and which is used for the minor direction is
  determined by the horizontal field.

  For horizontal text (horizontal = TRUE), characters on each line of
  text advance in the positive X direction if leftToRight is TRUE or
  in the negative X direction if leftToRight is FALSE. Characters are
  advanced according to their natural advance width. Each line of
  characters is advanced in the negative Y direction if topToBottom is
  TRUE or in the positive Y direction if topToBottom is FALSE. Lines
  are advanced by the amount of size ? spacing.

  For vertical text (horizontal = FALSE), characters on each line of
  text advance in the negative Y direction if topToBottom is TRUE or
  in the positive Y direction if topToBottom is FALSE. Characters are
  advanced according to their natural advance height. Each line of
  characters is advanced in the positive X direction if leftToRight is
  TRUE or in the negative X direction if leftToRight is FALSE. Lines
  are advanced by the amount of size ? spacing.

  The justify field determines alignment of the above text layout
  relative to the origin of the object coordinate system. The justify
  field is an MFString which can contain 2 values. The first value
  specifies alignment along the major axis and the second value
  specifies alignment along the minor axis, as determined by the
  horizontal field. An empty justify value ("") is equivalent to the
  default value. If the second string, minor alignment, is not
  specified, minor alignment defaults to the value "FIRST". Thus,
  justify values of "", "BEGIN", and ["BEGIN" "FIRST"] are equivalent.

  The major alignment is along the X-axis when horizontal is TRUE and
  along the Y-axis when horizontal is FALSE. The minor alignment is
  along the Y-axis when horizontal is TRUE and along the X-axis when
  horizontal is FALSE. The possible values for each enumerant of the
  justify field are "FIRST", "BEGIN", "MIDDLE", and "END". For major
  alignment, each line of text is positioned individually according to
  the major alignment enumerant. For minor alignment, the block of
  text representing all lines together is positioned according to the
  minor alignment enumerant. Tables 6.2-6.5 at
  <http://www.web3d.org/documents/specifications/14772/V2.0/part1/nodesRef.html#Table6.2>
  describe the behaviour in terms of which portion of the text is at
  the origin.

  The default minor alignment is "FIRST". This is a special case of
  minor alignment when horizontal is TRUE. Text starts at the baseline
  at the Y-axis. In all other cases, "FIRST" is identical to
  "BEGIN". In Tables 6.6 and 6.7, each colour-coded cross-hair
  indicates where the X-axis and Y-axis shall be in relation to the
  text. Figure 6.8 describes the symbols used in Tables 6.6 and 6.7.

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/key.gif">
  Figure 6.8 -- Key for Tables 6.6 and 6.7
  </center>



  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/horizontal.gif">
  Table 6.6 -- horizontal = TRUE
  </center>

  <center>
  <img src="http://www.web3d.org/documents/specifications/14772/V2.0/Images/vertical.gif">
  Table 6.7 -- horizontal = FALSE
  </center>

  <strong>Language</strong>

  The language field specifies the context of the language for the
  text string. Due to the multilingual nature of the ISO/IEC
  10646-1:1993, the language field is needed to provide a proper
  language attribute of the text string. The format is based on RFC
  1766: language[_territory]
  <http://www.web3d.org/documents/specifications/14772/V2.0/part1/references.html#[1766]>.
  The value for the language tag is based on ISO 639:1988 (e.g., 'zh'
  for Chinese, 'jp' for Japanese, and 'sc' for Swedish.) The territory
  tag is based on ISO 3166:1993 country codes (e.g., 'TW' for Taiwan
  and 'CN' for China for the 'zh' Chinese language tag). If the
  language field is empty (""), local language bindings are used.

  See
  <http://www.web3d.org/documents/specifications/14772/V2.0/part1/references.html>,
  for more information on RFC 1766
  <http://www.web3d.org/documents/specifications/14772/V2.0/part1/references.html#[1766]>,
  ISO/IEC 10646:1993
  <http://www.web3d.org/documents/specifications/14772/V2.0/part1/references.html#[UTF8]>,
  ISO/IEC 639:1998
  <http://www.web3d.org/documents/specifications/14772/V2.0/part1/references.html#[I639]>,
  and ISO 3166:1993 <http://www.web3d.org/documents/specifications/14772/V2.0/part1/references.html#[I3166]>.
*/

/*!
  SoSFFloat SoVRMLFontStyle::size
  Size of font.
*/

/*!
  SoMFString SoVRMLFontStyle::family
  Font family. All browsers must support "SANS", "SERIF" and "TYPEWRITER". Default value is "SERIF".
*/

/*!
  SoMFString SoVRMLFontStyle::style
  Font style. Can be one of "PLAIN", "BOLD", "ITALIC" or "BOLDITALIC". Default value is "PLAIN".
*/

/*!
  SoSFBool SoVRMLFontStyle::horizontal
  TRUE if strings should be rendered horizontally. Default value is TRUE.
*/

/*!
  SoSFBool SoVRMLFontStyle::leftToRight
  TRUE if strings should be rendered left to right. Default value is TRUE.
*/

/*!
  SoSFBool SoVRMLFontStyle::topToBottom
  True if strings should be rendered top to bottom. Default value is TRUE.
*/

/*!
  SoSFString SoVRMLFontStyle::language
  Text language. Empty by default.
*/

/*!
  SoMFString SoVRMLFontStyle::justify
  Text justification. Can be "BEGIN", "FIRST" "MIDDLE" or "END". Default value is "BEGIN".
*/

/*!
  SoSFFloat SoVRMLFontStyle::spacing
  Spacing constant. Default value is 1.0.
*/


#include <Inventor/VRMLnodes/SoVRMLFontStyle.h>
#include "coindefs.h"

#include <Inventor/VRMLnodes/SoVRMLMacros.h>

#include "nodes/SoSubNodeP.h"

SO_NODE_SOURCE(SoVRMLFontStyle);

/*!
  \copydetails SoNode::initClass(void)
*/
void
SoVRMLFontStyle::initClass(void) // static
{
  SO_NODE_INTERNAL_INIT_CLASS(SoVRMLFontStyle, SO_VRML97_NODE_TYPE);
}

/*!
  Constructor.
*/
SoVRMLFontStyle::SoVRMLFontStyle(void)
{
  SO_VRMLNODE_INTERNAL_CONSTRUCTOR(SoVRMLFontStyle);

  SO_VRMLNODE_ADD_FIELD(size, (1.0f));
  SO_VRMLNODE_ADD_FIELD(family, ("SERIF"));
  SO_VRMLNODE_ADD_FIELD(style, ("PLAIN"));
  SO_VRMLNODE_ADD_FIELD(horizontal, (TRUE));
  SO_VRMLNODE_ADD_FIELD(leftToRight, (TRUE));
  SO_VRMLNODE_ADD_FIELD(topToBottom, (TRUE));
  SO_VRMLNODE_ADD_FIELD(language, (""));
  SO_VRMLNODE_ADD_FIELD(justify, ("BEGIN"));
  SO_VRMLNODE_ADD_FIELD(spacing, (1.0f));
}

/*!
  Destructor.
*/
SoVRMLFontStyle::~SoVRMLFontStyle()
{
}

SbString
SoVRMLFontStyle::getFontName(void)
{
  return SbString("");
}

// Doc in parent
void
SoVRMLFontStyle::doAction(SoAction * COIN_UNUSED_ARG(action))
{
}

// Doc in parent
void
SoVRMLFontStyle::callback(SoCallbackAction * COIN_UNUSED_ARG(action))
{
}

// Doc in parent
void
SoVRMLFontStyle::GLRender(SoGLRenderAction * COIN_UNUSED_ARG(action))
{
}

// Doc in parent
void
SoVRMLFontStyle::getBoundingBox(SoGetBoundingBoxAction * COIN_UNUSED_ARG(action))
{
}

// Doc in parent

void
SoVRMLFontStyle::pick(SoPickAction * COIN_UNUSED_ARG(action))
{
}

// Doc in parent
void
SoVRMLFontStyle::getPrimitiveCount(SoGetPrimitiveCountAction * COIN_UNUSED_ARG(action))
{
}

#endif // HAVE_VRML97
