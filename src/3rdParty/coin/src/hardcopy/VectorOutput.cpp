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

#include <Inventor/annex/HardCopy/SoVectorOutput.h>

#include <Inventor/SbBasic.h>

#include "tidbitsp.h"

// *************************************************************************

/*!
  \class SoVectorOutput SoVectorOutput.h Inventor/annex/HardCopy/SoVectorOutput.h
  \brief The SoVectorOutput class is used for setting vector output file.

  \ingroup coin_hardcopy

  SoVectorizeAction will create an SoVectorOutput which will output
  to stdout by default. SoVectorizeAction::getOutput() can be used to
  fetch this output, and the user will probably want to set a
  file to output to.

  \since Coin 2.1
  \since TGS provides HardCopy support as a separate extension for TGS Inventor.
*/

class SoVectorOutputP {
public:
  FILE * fp;
  SbBool didopen;
};

#define PRIVATE(p) (p->pimpl)
#define PUBLIC(p) (p->publ)

// *************************************************************************

/*!
  Constructor. Will make this instance output to \e stdout.
*/
SoVectorOutput::SoVectorOutput(void)
{
  PRIVATE(this) = new SoVectorOutputP;
  PRIVATE(this)->fp = coin_get_stdout();
  PRIVATE(this)->didopen = FALSE;
}

/*!
  Destructor. Will close the file opened in openFile().
*/
SoVectorOutput::~SoVectorOutput()
{
  this->closeFile();
  delete PRIVATE(this);
}

// *************************************************************************

/*!
  Opens \a filename for output. Returns \e FALSE if file could not be
  opened for writing. If the file already exists, it will be
  truncated.
 */
SbBool
SoVectorOutput::openFile(const char * filename)
{
  this->closeFile();

  FILE * fp = fopen(filename, "wb");
  if (fp) {
    PRIVATE(this)->fp = fp;
    PRIVATE(this)->didopen = TRUE;
  }
  return fp != NULL;
}

/*!
  Closes the file opened in openFile()
*/
void
SoVectorOutput::closeFile(void)
{
  if (PRIVATE(this)->didopen) {
    fclose(PRIVATE(this)->fp);
    PRIVATE(this)->fp = stdout;
    PRIVATE(this)->didopen = FALSE;
  }
}

/*!
  Returns the \e stdio file handle pointer.
*/
FILE *
SoVectorOutput::getFilePointer(void)
{
  return PRIVATE(this)->fp;
}

#undef PRIVATE
#undef PUBLIC
