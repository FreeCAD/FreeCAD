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
  \class SoFont SoFont.h Inventor/nodes/SoFont.h
  \brief The SoFont class is an appearance node for setting fonts.

  \ingroup coin_nodes

  Successive text rendering nodes (like SoText2, SoText3, SoAsciiText,
  etc) will use the font specified from an SoFont node when
  visualizing text.

  The font name is recognized with the form "family:style". The range of
  supported font families depends on which fonts are installed on the
  system. A typical font family might be "Arial" or "Times New
  Roman". 

  Style is either "Bold", "Italic" or "Bold Italic".

  Spaces before or after the ":" will be ignored.

  If a good match for the chosen font cannot be found, the default
  font will be loaded instead. The default 2D font is a built-in 8x12
  points font. The 3D font is a serif font ala Times New Roman. It is
  not possible to apply styles to any of the built-in fonts. It is not
  possible to specify a size for the default 2D font.
  
  One can explicitly select the default font by setting "defaultFont"
  as the font name, or by just leaving the SoFont::name field alone,
  as it has that value by default. In that regard, note that SoFont
  does not inherit / accumulate any parts from earlier SoFont-nodes,
  even when one or both of its fields are left blank in e.g. an
  iv-file, like this:

  \verbatim
  #Inventor V2.1 ascii
  
  Font { name "Helvetica"  size 50 }
  Text2 { string "hepp" }
  
  Translation { translation 5 80 0 }
  
  BaseColor { rgb 1 1 0 }
  
  Font { size 30 }
  Text2 { string "svupp" }
  \endverbatim

  (The second string in the above example will be rendered with the
  default built-in font, not in Helvetica.)


  The default fonts are always accessible by Coin as they are embedded
  into the runtime library. If one needs to guarantee that the text
  will have the same appearance under all circumstances, the default
  font will be a safe choice. Another solution might be to use the \e
  FreeType font engine and explicitly name a font-file. This is
  described in more detail below.

  Here is a simple example on how to print a string using a bold and
  italic Arial font:

  \verbatim
  #Inventor V2.1 ascii
  
  Separator {
     Font {
       name "Arial:Bold Italic"
       size 14
     }
     Text2 {
       string ["This is a", "Coin font test"]
       justification CENTER
     }    
  }
  \endverbatim

  Coin has support for two different font APIs. On non-Windows
  platforms, the \e FreeType library together with the optional \e
  Fontconfig library is used.

  If the \e Fontconfig library is installed the font file on the system
  for a given font name will be located through it. For more
  information on \e Fontconfig see
  http://freedesktop.org/software/fontconfig . Additionally \e
  Fontconfig allows to match specific fonts through its own pattern
  matching format (see
  http://pdx.freedesktop.org/~fontconfig/fontconfig-user.html).
  Please note that the point size value in the textual representation
  of the Fontconfig pattern is currently overridden by the size field
  of the SoFont node and if no size field is specified the default
  value of the size field is in effect. In case you intend to use your
  application on systems where the \e Fontconfig library is expected
  to be not installed you should not make use of \e Fontconfig's font
  pattern syntax. \e Fontconfig usage can be prevented by setting the
  "COIN_FORCE_FONTCONFIG_OFF" environment variable to 1.

  Here is an example on how to print a string using the \e Fontconfig
  pattern matching syntax using a bold & italic font where Times New
  Roman is the preferred font family.

  \verbatim
  #Inventor V2.1 ascii
  
  Separator {
     Font {
       name "Times New Roman,Arial,Helvetica:italic:bold"
       size 24
     }
     Text2 {
       string ["This is a", "Coin font test"]
       justification CENTER
     }    
  }
  \endverbatim


  On Windows the Win32 GDI library is used. \e FreeType and \e
  Fontconfig are dynamically loaded on demand by Coin if font support
  is requested by a node. When font support is loaded on Windows,
  FreeType will have precedence over Win32 if located. This can be
  prevented by setting the "COIN_FORCE_FREETYPE_OFF" environment
  variable to 1. When using FreeType, you need FreeType version 2.1 or
  later. On Mac OS X, version 2.1.7 or later is required.

  If Coin cannot load the \e FreeType library, and is not running on
  Microsoft Windows, only the default fonts will be accessible.

  It is possible to specify the TrueType font file directly if \e
  FreeType is used as the font engine. This is done by including the
  ".ttf" in the filename, i.e. "Comic_Sans_MS.ttf". Coin will then
  search the local path for the running program and then the path
  specified by the "COIN_FONT_PATH" environment variable. If the
  program is using \e FreeType on a Windows platform, the
  "$WINDIR/Fonts" directory will also be searched.

  It is not possible to directly specify a TrueType font file if
  Windows is handling the fonts. This is due to the way Windows is
  accessing the fonts through the system registry. All fonts must
  therefore be properly installed and given a system name. Open the
  "Control Panel" and double click on the "Fonts" icon for an overview
  of installed fonts and their names.

  Beware that some non-English versions of Windows are using different
  name for the styles (i.e. "Italique" instead of "Italic"). These
  names are supported in Coin, but it is recommended for portability
  purposes to only use the English terms. Please note that there is
  still a possibility that there are no fonts installed using the
  terms "Bold" or "Italic" on the Windows platform. To guarantee that
  a font is accessible you must either use the \e FreeType library and
  include a TrueType font in your distribution, or you must avoid
  using styles and stick to the standard Windows fonts.

  If the "COIN_DEBUG_FONTSUPPORT" environment variable is set to 1, an
  extensive amount of information about loading, initializing and
  using fonts will be output. Issues like missing fonts and other
  related problems will then be reported, so we advice you to first
  try to use that debugging option when something does not work quite
  as expected.

  <b>FILE FORMAT/DEFAULTS:</b>
  \code
    Font {
        name "defaultFont"
        size 10
    }
  \endcode

  \sa SoFontStyle, SoText2, SoText3, SoAsciiText
*/

#include <Inventor/nodes/SoFont.h>

#include <cstring>

#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoGetPrimitiveCountAction.h>
#include <Inventor/actions/SoPickAction.h>
#include <Inventor/elements/SoFontNameElement.h>
#include <Inventor/elements/SoFontSizeElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/errors/SoDebugError.h>

#include "nodes/SoSubNodeP.h"

// *************************************************************************

/*!
  \var SoSFName SoFont::name

  Name of font.

  Which font names are available is rather system dependent, not only
  on whether or not you are running on a UNIX/Linux system, Microsoft
  Windows or whatever, but also on which fonts and font \e types (like
  TrueType) are installed on a particular user's system.

  All font rendering nodes have a built-in fallback font to use,
  though, so even though Coin cannot find a font on the system of the
  specified type, the text should be rendered somehow.

  In summation, consider this node type and this particular field as a
  \e hint to the font rendering engines of Coin, and do \e not base
  your models on a particular font being available.
*/

/*!
  \var SoSFFloat SoFont::size

  Size of font. Defaults to 10.0.

  For 2D rendered bitmap fonts (like for SoText2), this value is the
  height of a character in screen pixels. For 3D text, this value is
  the world space coordinates height of a character in the current
  units setting (see documentation for SoUnits node).
*/

// *************************************************************************

SO_NODE_SOURCE(SoFont);

/*!
  Constructor.
*/
SoFont::SoFont(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoFont);

  SO_NODE_ADD_FIELD(name, ("defaultFont"));
  SO_NODE_ADD_FIELD(size, (10.0f));
}

/*!
  Destructor.
*/
SoFont::~SoFont()
{
}

/*!
  \copybrief SoBase::initClass(void)
*/
void
SoFont::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoFont, SO_FROM_INVENTOR_1);

  SO_ENABLE(SoCallbackAction, SoFontNameElement);
  SO_ENABLE(SoCallbackAction, SoFontSizeElement);
  SO_ENABLE(SoGLRenderAction, SoFontNameElement);
  SO_ENABLE(SoGLRenderAction, SoFontSizeElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoFontNameElement);
  SO_ENABLE(SoGetBoundingBoxAction, SoFontSizeElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoFontNameElement);
  SO_ENABLE(SoGetPrimitiveCountAction, SoFontSizeElement);
  SO_ENABLE(SoPickAction, SoFontNameElement);
  SO_ENABLE(SoPickAction, SoFontSizeElement);
}

// Doc from superclass.
void
SoFont::doAction(SoAction * action)
{
  SoState * state = action->getState();
  uint32_t flags = SoOverrideElement::getFlags(state);
    
#define TEST_OVERRIDE(bit) ((SoOverrideElement::bit & flags) != 0)
  
  if (!name.isIgnored() && !TEST_OVERRIDE(FONT_NAME)) {
    SoFontNameElement::set(state, this, this->name.getValue());
    if (this->isOverride()) {
      SoOverrideElement::setFontNameOverride(state, this, TRUE);
    }
  }
  if (!size.isIgnored() && !TEST_OVERRIDE(FONT_SIZE)) {
    SoFontSizeElement::set(state, this, this->size.getValue());
    if (this->isOverride()) {
      SoOverrideElement::setFontSizeOverride(state, this, TRUE);
    }
  }

#undef TEST_OVERRIDE
}

// Doc from superclass.
void
SoFont::getBoundingBox(SoGetBoundingBoxAction * action)
{
  SoFont::doAction(action);
}

// Doc from superclass.
void
SoFont::GLRender(SoGLRenderAction * action)
{
  SoFont::doAction(action);
}

// Doc from superclass.
void
SoFont::callback(SoCallbackAction * action)
{
  SoFont::doAction((SoAction *)action);
}

// Doc from superclass.
void
SoFont::pick(SoPickAction * action)
{
  SoFont::doAction(action);
}

// Doc from superclass.
void
SoFont::getPrimitiveCount(SoGetPrimitiveCountAction * action)
{
  SoFont::doAction(action);
}
