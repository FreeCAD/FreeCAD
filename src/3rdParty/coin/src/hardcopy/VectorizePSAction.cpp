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
  \class SoVectorizePSAction SoVectorizePSAction.h Inventor/annex/HardCopy/SoVectorizePSAction.h
  \brief The SoVectorizePSAction class is used for rendering to a PostScript file.

  \ingroup coin_hardcopy

  \since Coin 2.1
  \since TGS provides HardCopy support as a separate extension for TGS Inventor.
*/

#include <Inventor/annex/HardCopy/SoVectorizePSAction.h>

#include <cstdio>
#include <cmath> // for floor() and ceil()

#include <Inventor/errors/SoDebugError.h>

#include "hardcopy/VectorizeActionP.h"
#include "tidbitsp.h"
#include "actions/SoSubActionP.h"


// *************************************************************************

class SoVectorizePSActionP {
public:
  SoVectorizePSActionP(SoVectorizePSAction * p) {
    this->publ = p;
    this->linepattern = 0xffff;
    this->linewidth = 1.0f;

    this->fontname = "";
    this->fontsize = -1.0f;
    this->dummycnt = 0;
  }

  enum {
    // max setdash vector length
    // FIXME: this seems to be the limit hardcoded into Ghostscript.
    // The PostScript language reference doesn't say anything about
    // the maximum length of the setdash vector. pederb, 2004-10-21
    DASH_LIMIT = 10
  };
  
  SbVec2f convertToPS(const SbVec2f & mm) const;
  float convertToPS(const float mm) const;

  void updateLineAttribs(const SoVectorizeLine * line);
  void updateFont(const SbString & fontname, const float fontsize);
  void printSetdash(uint16_t pattern) const;

  void printCircle(const SbVec3f & v, const SbColor & c, const float radius) const;
  void printSquare(const SbVec3f & v, const SbColor & c, const float size) const;
  void printTriangle(const SbVec3f * v, const SbColor * c);
  void printTriangle(const SoVectorizeTriangle * item);
  void printLine(const SoVectorizeLine * item);
  void printPoint(const SoVectorizePoint * item) const;
  void printText(const SoVectorizeText * item);
  void printImage(const SoVectorizeImage * item) const;

  double gouraudeps;
  SbString default2dfont;
  uint16_t linepattern;
  float linewidth;

  SbString fontname;
  float fontsize;

  // used for gouraud shading workaround
  int dummycnt;

private:
  SoVectorizePSAction * publ;
};


#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->publ)

// *************************************************************************

SO_ACTION_SOURCE(SoVectorizePSAction);

// *************************************************************************

/*!
  \copydetails SoAction::initClass(void)
*/
void
SoVectorizePSAction::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoVectorizePSAction, SoVectorizeAction);
}

/*!
  Default constructor.
*/
SoVectorizePSAction::SoVectorizePSAction(void)
{
  PRIVATE(this) = new SoVectorizePSActionP(this);
  PRIVATE(this)->gouraudeps = 0.0f;
  PRIVATE(this)->default2dfont = "Courier";
  SO_ACTION_CONSTRUCTOR(SoVectorizePSAction);

  this->setOutput(new SoPSVectorOutput);
}

/*!
  Default destructor.
*/
SoVectorizePSAction::~SoVectorizePSAction()
{
  delete PRIVATE(this);
}

/*!
  Sets the Gouraud shading threshold. A threshold of 0.0 will disable
  Gouraud shading. A smaller value will yield more accurate Gouraud
  shading. Default is 0.1.

  Since the PostScript language has no support for Gouraud shaded
  triangles, each triangle will be split into subtriangles
  approximately of size \a eps PostScript units. One PostScript unit
  is approximately 1/72 inch.
*/
void
SoVectorizePSAction::setGouraudThreshold(const double eps)
{
  PRIVATE(this)->gouraudeps = eps;
}

// *************************************************************************

/*!
  Sets the default font name. This font will be used for rendering
  Text2-nodes which have no Font-nodes preceding them. The default
  value is "Courier".
*/
void
SoVectorizePSAction::setDefault2DFont(const SbString & fontname)
{
  PRIVATE(this)->default2dfont = fontname;
}

/*!
  Returns the default font name.

  \sa setDefault2DFont()
*/
const SbString &
SoVectorizePSAction::getDefault2DFont(void) const
{
  return PRIVATE(this)->default2dfont;
}

/*!
  Returns the SoPSVectorOutput used by this instance.
*/
SoPSVectorOutput *
SoVectorizePSAction::getOutput(void) const
{
  return (SoPSVectorOutput*)SoVectorizeAction::getOutput();
}

/*!
  Returns the SoPSVectorOutput used by this instance. Provided
  for API compatibility with TGS HardCopy support.
*/
SoPSVectorOutput *
SoVectorizePSAction::getPSOutput(void) const
{
  return (SoPSVectorOutput*)SoVectorizeAction::getOutput();
}

static const char * gouraudtriangle[] = {
  "% the gouraudtriangle PostScript fragment below is free",
  "% written by Frederic Delhoume (delhoume@ilog.fr)",
  "/bd{bind def}bind def /triangle { aload pop   setrgbcolor  aload pop 5 3",
  "roll 4 2 roll 3 2 roll exch moveto lineto lineto closepath fill } bd",
  "/computediff1 { 2 copy sub abs threshold ge {pop pop pop true} { exch 2",
  "index sub abs threshold ge { pop pop true} { sub abs threshold ge } ifelse",
  "} ifelse } bd /computediff3 { 3 copy 0 get 3 1 roll 0 get 3 1 roll 0 get",
  "computediff1 {true} { 3 copy 1 get 3 1 roll 1 get 3 1 roll 1 get",
  "computediff1 {true} { 3 copy 2 get 3 1 roll  2 get 3 1 roll 2 get",
  "computediff1 } ifelse } ifelse } bd /middlecolor { aload pop 4 -1 roll",
  "aload pop 4 -1 roll add 2 div 5 1 roll 3 -1 roll add 2 div 3 1 roll add 2",
  "div 3 1 roll exch 3 array astore } bd /gouraudtriangle { computediff3 { 4",
  "-1 roll aload 7 1 roll 6 -1 roll pop 3 -1 roll pop add 2 div 3 1 roll add",
  "2 div exch 3 -1 roll aload 7 1 roll exch pop 4 -1 roll pop add 2 div 3 1",
  "roll add 2 div exch 3 -1 roll aload 7 1 roll pop 3 -1 roll pop add 2 div 3",
  "1 roll add 2 div exch 7 3 roll 10 -3 roll dup 3 index middlecolor 4 1 roll",
  "2 copy middlecolor 4 1 roll 3 copy pop middlecolor 4 1 roll 13 -1 roll",
  "aload pop 17 index 6 index 15 index 19 index 6 index 17 index 6 array",
  "astore 10 index 10 index 14 index gouraudtriangle 17 index 5 index 17",
  "index 19 index 5 index 19 index 6 array astore 10 index 9 index 13 index",
  "gouraudtriangle 13 index 16 index 5 index 15 index 18 index 5 index 6",
  "array astore 12 index 12 index 9 index gouraudtriangle 17 index 16 index",
  "15 index 19 index 18 index 17 index 6 array astore 10 index 12 index 14",
  "index gouraudtriangle 18 {pop} repeat } { aload pop 5 3 roll aload pop 7 3",
  "roll aload pop 9 3 roll 4 index 6 index 4 index add add 3 div 10 1 roll 7",
  "index 5 index 3 index add add 3 div 10 1 roll 6 index 4 index 2 index add",
  "add 3 div 10 1 roll 9 {pop} repeat 3 array astore triangle } ifelse } bd",
  NULL
};

static const char * flatshadetriangle[] = {
  "% flatshade a triangle",
  "/flatshadetriangle",
  "{ newpath moveto",
  "lineto",
  "lineto",
  "closepath",
  "setrgbcolor",
  "fill } def",
  NULL
};

static const char * rightshow[] = {
  "% print a right justified string",
  "/rightshow",
  "{ dup stringwidth pop",
  "neg",
  "0 rmoveto",
  "show } def",
  NULL
};

static const char * centershow[] = {
  "% print a center justified string",
  "/centershow",
  "{ dup stringwidth pop",
  "0.5 mul",
  "neg",
  "0 rmoveto",
  "show } def",
  NULL
};

static void print_array(FILE * fp, const char ** array)
{
  for (int i = 0; array[i]; i++) {
    fputs(array[i], fp);
    fputs("\n", fp);
  }
  fputs("\n", fp);
}

// doc in parent
void
SoVectorizePSAction::printHeader(void) const
{
  FILE * file = this->getOutput()->getFilePointer();

  int viewport[4];

  viewport[0] = int(floor(PRIVATE(this)->convertToPS(this->getPageStartpos())[0]));
  viewport[1] = int(floor(PRIVATE(this)->convertToPS(this->getPageStartpos())[1]));
  viewport[2] = int(ceil(PRIVATE(this)->convertToPS(this->getPageSize())[0]));
  viewport[3] = int(ceil(PRIVATE(this)->convertToPS(this->getPageSize())[1]));

  fputs("%!PS-Adobe-2.0 EPSF-2.0\n", file);
  fprintf(file, "%%%%Creator: Coin 2.0\n");
  fprintf(file, "%%%%BoundingBox: %d %d %d %d\n",
          viewport[0], viewport[1], viewport[2], viewport[3]);
  fputs("%%EndComments\n", file);
  fputs("\n", file);
  fputs("gsave\n", file);
  fputs("\n", file);

  fprintf(file, "/threshold %g def %% used by gouraudtriangle\n", PRIVATE(this)->gouraudeps);

  print_array(file, gouraudtriangle);
  print_array(file, flatshadetriangle);
  print_array(file, rightshow);
  print_array(file, centershow);

  if (this->getOrientation() == LANDSCAPE) {
    SbVec2f psize = PRIVATE(this)->convertToPS(this->getPageSize());
    SbVec2f porg = PRIVATE(this)->convertToPS(this->getPageStartpos());
    psize *= 0.5f;
    fputs("% rotate to LANDSCAPE orientation\n", file);
    fprintf(file, "%g %g translate\n", porg[0] + psize[0], porg[1] + psize[1]);
    fprintf(file, "90 rotate\n");
    fprintf(file, "%g %g translate\n\n", -(psize[1]+porg[1]), -(psize[0]+porg[1]));
  }

  // used for gouraud shading workaround
  PRIVATE(this)->dummycnt = 0;
}

// doc in parent
void
SoVectorizePSAction::printFooter(void) const
{
  FILE * file = this->getOutput()->getFilePointer();

  fputs("\ngrestore\n", file);
  fputs("showpage\n", file);
}

// doc in parent
void
SoVectorizePSAction::printViewport(void) const
{
  FILE * file = this->getOutput()->getFilePointer();

  float viewport[4];
  viewport[0] = PRIVATE(this)->convertToPS(this->getRotatedViewportStartpos())[0];
  viewport[1] = PRIVATE(this)->convertToPS(this->getRotatedViewportStartpos())[1];
  viewport[2] = PRIVATE(this)->convertToPS(this->getRotatedViewportSize())[0] + viewport[0];
  viewport[3] = PRIVATE(this)->convertToPS(this->getRotatedViewportSize())[1] + viewport[1];

  fputs("% set up clipping for viewport\n", file);
  fprintf(file, "newpath\n");
  fprintf(file, "%g %g moveto\n", viewport[0], viewport[1]);
  fprintf(file, "%g %g lineto\n", viewport[0], viewport[3]);
  fprintf(file, "%g %g lineto\n", viewport[2], viewport[3]);
  fprintf(file, "%g %g lineto\n", viewport[2], viewport[1]);
  fprintf(file, "closepath clip\n\n");
}

// doc in parent
void
SoVectorizePSAction::printBackground(void) const
{
  FILE * file = this->getOutput()->getFilePointer();

  float viewport[4];
  SbColor bgcol;

  viewport[0] = PRIVATE(this)->convertToPS(this->getRotatedViewportStartpos())[0];
  viewport[1] = PRIVATE(this)->convertToPS(this->getRotatedViewportStartpos())[1];
  viewport[2] = PRIVATE(this)->convertToPS(this->getRotatedViewportSize())[0];
  viewport[3] = PRIVATE(this)->convertToPS(this->getRotatedViewportSize())[1];

  (void) this->getBackgroundColor(bgcol);

  fputs("% clear background\n", file);
  fprintf(file, "newpath\n");
  fprintf(file, "%g %g moveto\n", viewport[0], viewport[1]);
  fprintf(file, "0 %g rlineto\n", viewport[3]);
  fprintf(file, "%g 0 rlineto\n", viewport[2]);
  fprintf(file, "0 %g neg rlineto\n", viewport[3]);
  fprintf(file, "closepath\n");
  fprintf(file, "%g %g %g setrgbcolor\n",
          bgcol[0], bgcol[1], bgcol[2]);
  fprintf(file, "fill\n");
}

// doc in parent
void
SoVectorizePSAction::printItem(const SoVectorizeItem * item) const
{
  switch (item->type) {
  case SoVectorizeItem::TRIANGLE:
    PRIVATE(this)->printTriangle((SoVectorizeTriangle*)item);
    break;
  case SoVectorizeItem::LINE:
    PRIVATE(this)->printLine((SoVectorizeLine*)item);
    break;
  case SoVectorizeItem::POINT:
    PRIVATE(this)->printPoint((SoVectorizePoint*)item);
    break;
  case SoVectorizeItem::TEXT:
    PRIVATE(this)->printText((SoVectorizeText*)item);
    break;
  case SoVectorizeItem::IMAGE:
    PRIVATE(this)->printImage((SoVectorizeImage*)item);
    break;
  default:
    assert(0 && "unsupported item");
    break;
  }
}

static int count_bits(uint16_t mask, int & pos, SbBool onoff)
{
  int cnt = -1;
  pos++;
  SbBool res;
  do {
    cnt++;
    pos--;
    if (pos < 0) break;
    uint16_t bit = 1 << pos;
    res = (bit & mask) ? TRUE : FALSE;
  } while (res == onoff);
  return cnt;
}

//
// Set up line stipple.
//
void
SoVectorizePSActionP::printSetdash(uint16_t pattern) const
{
  FILE * file = PUBLIC(this)->getOutput()->getFilePointer();
  fputs("[", file);

  int pos = 15;
  SbBool onoff = TRUE;
  int dashcnt = 0;

  while (pos >= 0 && dashcnt < DASH_LIMIT) {
    int cnt = count_bits(pattern, pos, onoff);
    fprintf(file," %d", cnt);
    onoff = !onoff;
    dashcnt++;
  }
  if (dashcnt == DASH_LIMIT && pos >= 0) {
    static int didwarn = 0;
    if (!didwarn) {
      SoDebugError::postWarning("SoVectorizeActionP::printSetdash",
                                "linePattern mask is too complex. "
                                "Dash is truncated to %d items.",
                                DASH_LIMIT);
      didwarn = 1;
    }
  }
  if (!onoff) { // need pairs of values
    fputs(" 0] 0 setdash\n", file);
  }
  else {
    fputs("] 0 setdash\n", file);
  }
}

//
// make sure we have the correct font
//
void
SoVectorizePSActionP::updateFont(const SbString & fontnameref, const float fontsizearg)
{
  FILE * file = PUBLIC(this)->getOutput()->getFilePointer();

  if (fontnameref != this->fontname ||
      fontsizearg != this->fontsize) {
    fprintf(file, "/%s findfont\n", fontnameref.getString());
    fprintf(file, "%g scalefont\n", fontsizearg);
    fprintf(file, "setfont\n");

    this->fontname = fontnameref;
    this->fontsize = fontsizearg;
  }
}

//
// Make sure line width and line stipple is correct.
//
void
SoVectorizePSActionP::updateLineAttribs(const SoVectorizeLine * line)
{
  FILE * file = PUBLIC(this)->getOutput()->getFilePointer();

  float lw = line->width;
  uint16_t lp = line->pattern;

  if (lw != this->linewidth) {
    this->linewidth = lw;
    fprintf(file, "%g setlinewidth\n", this->convertToPS(lw * PUBLIC(this)->getNominalWidth()));
  }

  if (lp != this->linepattern) {
    this->linepattern = lp;
    if (lp == 0xffff) {
      fprintf(file, "[] 0 setdash\n");
    }
    else {
      this->printSetdash(lp);
    }
  }
}

//
// will output a line in PostScript format
//
void
SoVectorizePSActionP::printLine(const SoVectorizeLine * item)
{
  FILE * file = PUBLIC(this)->getOutput()->getFilePointer();

  SbVec2f mul = this->convertToPS(PUBLIC(this)->getRotatedViewportSize());
  SbVec2f add = this->convertToPS(PUBLIC(this)->getRotatedViewportStartpos());

  int i;
  const SbBSPTree & bsp = PUBLIC(this)->getBSPTree();

  SbVec3f v[2];
  SbColor c[2];
  float t[2];

  for (i = 0; i < 2; i++) {
    v[i] = bsp.getPoint(item->vidx[i]);
    v[i][0] = (v[i][0] * mul[0]) + add[0];
    v[i][1] = (v[i][1] * mul[1]) + add[1];
    c[i].setPackedValue(item->col[i], t[i]);
  }

  this->updateLineAttribs(item);
  fprintf(file, "%g %g %g setrgbcolor\n", c[0][0], c[0][1], c[0][2]);
  fprintf(file, "newpath\n");
  fprintf(file, "%g %g moveto\n", v[0][0], v[0][1]);
  fprintf(file, "%g %g lineto\n", v[1][0], v[1][1]);
  fprintf(file,"stroke\n\n");
}

//
// will print a PostScript circle
//
void
SoVectorizePSActionP::printCircle(const SbVec3f & v, const SbColor & c, const float radius) const
{
  FILE * file = PUBLIC(this)->getOutput()->getFilePointer();

  fprintf(file, "newpath %g %g %g 0 360 arc closepath\n", v[0], v[1], radius);
  fprintf(file, "%g %g %g setrgbcolor\n", c[0], c[1], c[2]);
  fprintf(file, "fill\n\n");
}

//
// will print a PostScript square centered in 'v'
//
void
SoVectorizePSActionP::printSquare(const SbVec3f & v, const SbColor & c, const float size) const
{
  FILE * file = PUBLIC(this)->getOutput()->getFilePointer();

  float s2 = size * 0.5f;

  fprintf(file, "newpath\n");
  fprintf(file, "%g %g moveto\n", v[0]-s2, v[1]-s2);
  fprintf(file, "0 %g rlineto\n", size);
  fprintf(file, "%g 0 rlineto\n", size);
  fprintf(file, "0 %g neg rlineto\n", size);
  fprintf(file, "closepath\n");
  fprintf(file, "%g %g %g setrgbcolor\n",
          c[0], c[1], c[2]);
  fprintf(file, "fill\n");
}


//
// will output a point in PostScript format
//
void
SoVectorizePSActionP::printPoint(const SoVectorizePoint * item) const
{
  SbVec2f mul = this->convertToPS(PUBLIC(this)->getRotatedViewportSize());
  SbVec2f add = this->convertToPS(PUBLIC(this)->getRotatedViewportStartpos());

  const SbBSPTree & bsp = PUBLIC(this)->getBSPTree();

  SbVec3f v;
  SbColor c;
  float t;

  v = bsp.getPoint(item->vidx);
  v[0] = (v[0] * mul[0]) + add[0];
  v[1] = (v[1] * mul[1]) + add[1];
  c.setPackedValue(item->col, t);

  switch (PUBLIC(this)->getPointStyle()) {
  default:
    assert(0 && "unknown point style");
  case SoVectorizeAction::CIRCLE:
    this->printCircle(v, c, this->convertToPS(item->size * PUBLIC(this)->getNominalWidth() * 0.5f));
    break;
  case SoVectorizeAction::SQUARE:
    this->printSquare(v, c, this->convertToPS(item->size * PUBLIC(this)->getNominalWidth()));
    break;
  }
}

void
SoVectorizePSActionP::printTriangle(const SbVec3f * v, const SbColor * c)
{
  if (v[0] == v[1] || v[1] == v[2] || v[0] == v[2]) return;

  FILE * file = PUBLIC(this)->getOutput()->getFilePointer();

  SbBool flatshade =
    (this->gouraudeps == 0.0f) ||
    ((c[0] == c[1]) && (c[1] == c[2]));

  if (flatshade || this->dummycnt == 0) {
    SbColor a = (c[0] + c[1] + c[2]) / 3.0f;

    // flatshaded
    fprintf(file, "%g %g %g %g %g %g %g %g %g flatshadetriangle\n",
            a[0], a[1], a[2],
            v[2][0], v[2][1],
            v[1][0], v[1][1],
            v[0][0], v[0][1]);
  }
  else {
    // gouraud
    fprintf(file, "[%g %g %g %g %g %g]",
            v[0][0], v[1][0], v[2][0],
            v[0][1], v[1][1], v[2][1]);
    fprintf(file, " [%g %g %g] [%g %g %g] [%g %g %g] gouraudtriangle\n",
            c[0][0], c[0][1], c[0][2],
            c[1][0], c[1][1], c[1][2],
            c[2][0], c[2][1], c[2][2]);
  }
  this->dummycnt++;

  // FIXME: For some reason the gouraud-triangle macro fails if it is
  // the first triangle that is drawn. We work around this by always
  // rendering the first triangle as a flatshaded triangle, and then
  // overwriting it again with the gouraud version... Really strange,
  // pederb, 2003-06-30
  if (this->dummycnt == 1 && !flatshade) {
    this->printTriangle(v, c);
  }
}

//
// will output a triangle in PostScript format
//
void
SoVectorizePSActionP::printTriangle(const SoVectorizeTriangle * item)
{
  SbVec2f mul = this->convertToPS(PUBLIC(this)->getRotatedViewportSize());
  SbVec2f add = this->convertToPS(PUBLIC(this)->getRotatedViewportStartpos());

  int i;
  const SbBSPTree & bsp = PUBLIC(this)->getBSPTree();

  SbVec3f v[3];
  SbColor c[3];
  float t[3];

  for (i = 0; i < 3; i++) {
    v[i] = bsp.getPoint(item->vidx[i]);
    v[i][0] = (v[i][0] * mul[0]) + add[0];
    v[i][1] = (v[i][1] * mul[1]) + add[1];

    c[i].setPackedValue(item->col[i], t[i]);
  }
  this->printTriangle((SbVec3f*)v, (SbColor*)c);
}

//
// will output an image in PostScript format
//
void
SoVectorizePSActionP::printImage(const SoVectorizeImage * item) const
{
  FILE * fp = PUBLIC(this)->getOutput()->getFilePointer();
  SbVec2f mul = this->convertToPS(PUBLIC(this)->getRotatedViewportSize());
  SbVec2f add = this->convertToPS(PUBLIC(this)->getRotatedViewportStartpos());

  fprintf(fp, "gsave\n");
  fprintf(fp, "%% workaround for bug in some PS interpreters\n");
  fprintf(fp, "%% which doesn't skip the ASCII85 EOD marker.\n");
  fprintf(fp, "/~ {currentfile read pop pop} def\n\n");

  SbVec2s size = item->image.size;
  int nc = item->image.nc;
  const unsigned char * src = item->image.data;

  //  fprintf(fp, "0 0 moveto\n");
  fprintf(fp, "%g %g translate\n",
          item->pos[0]*mul[0] + add[0],
          item->pos[1]*mul[1] + add[1]);

  fprintf(fp, "/pix %d string def\n", size[0] * (nc >= 3 ? 3 : 1));

  fprintf(fp, "%g %g scale\n",
          item->size[0] * mul[0],
          item->size[1] * mul[1]);

  fprintf(fp, "%d %d 8 [%d 0 0 %d 0 0] currentfile\n",
          (int)item->image.size[0], (int)item->image.size[1],
          (int)item->image.size[0], (int)item->image.size[1]);

  fprintf(fp, "/ASCII85Decode filter\n");
  if (item->image.nc >= 3) fprintf(fp, "false 3\ncolorimage\n");
  else fprintf(fp,"image\n");

  const int rowlen = 72;
  int num = size[0] * size[1];
  unsigned char tuple[4];
  unsigned char linebuf[rowlen+5];
  int tuplecnt = 0;
  int linecnt = 0;
  int cnt = 0;

  while (cnt < num) {
    switch (nc) {
    default: // avoid warning
    case 1:
      coin_output_ascii85(fp, src[cnt], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      break;
    case 2:
      coin_output_ascii85(fp, src[cnt*2], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      break;
    case 3:
      coin_output_ascii85(fp, src[cnt*3], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      coin_output_ascii85(fp, src[cnt*3+1], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      coin_output_ascii85(fp, src[cnt*3+2], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      break;
    case 4:
      coin_output_ascii85(fp, src[cnt*4], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      coin_output_ascii85(fp, src[cnt*4+1], tuple, linebuf, &tuplecnt, &linecnt,rowlen, FALSE);
      coin_output_ascii85(fp, src[cnt*4+2], tuple, linebuf, &tuplecnt, &linecnt, rowlen, FALSE);
      break;
    }
    cnt++;
  }

  // flush data in ascii85 encoder
  coin_flush_ascii85(fp, tuple, linebuf, &tuplecnt, &linecnt, rowlen);

  fprintf(fp, "~>\n\n"); // ASCII85 EOD marker
  fprintf(fp, "grestore\n");
}

//
// will output text in PostScript format
//
void
SoVectorizePSActionP::printText(const SoVectorizeText * item)
{
  FILE * file = PUBLIC(this)->getOutput()->getFilePointer();

  SbVec2f mul = this->convertToPS(PUBLIC(this)->getRotatedViewportSize());
  SbVec2f add = this->convertToPS(PUBLIC(this)->getRotatedViewportStartpos());

  SbString thefontname = item->fontname.getString();
  if (thefontname == "defaultFont") {
    thefontname = this->default2dfont;
  }

  SbColor c;
  float dummy;
  c.setPackedValue(item->col, dummy);

  this->updateFont(thefontname, item->fontsize * mul[1]);

  fprintf(file, "%g %g %g setrgbcolor\n",
          c[0], c[1], c[2]);
  fprintf(file, "%g %g moveto\n",
          item->pos[0]*mul[0] + add[0],
          item->pos[1]*mul[1] + add[1]);

  SbString op;
  switch (item->justification) {
  default:
  case SoVectorizeText::LEFT:
    op = "show";
    break;
  case SoVectorizeText::CENTER:
    op = "centershow";
    break;
  case SoVectorizeText::RIGHT:
    op = "rightshow";
    break;
  }

  fprintf(file, "(%s) %s\n\n", item->string.getString(), op.getString());
}

// a standard PS unit is 1/72 inch
SbVec2f
SoVectorizePSActionP::convertToPS(const SbVec2f & mm) const
{
  return from_mm(mm, SoVectorizeAction::INCH) * 72.0f;
}

// a standard PS unit is 1/72 inch
float
SoVectorizePSActionP::convertToPS(const float mm) const
{
  return from_mm(mm, SoVectorizeAction::INCH) * 72.0f;
}

#undef PRIVATE
#undef PUBLIC
