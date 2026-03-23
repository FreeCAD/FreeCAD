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

/*
 * Simple example that demonstrates how to read in and write out an
 * Open Inventor model, essentially copying it.  This process has no use
 * per se, but can be used to sanity-check the import/export-related
 * parts of Coin, and can be used as a starting point for utilities doing
 * scene graph rewrites.
 *
 * Build the example using this command:
 *
 *   coin-config --build ivcp ivcp.cpp
 *
 */

#include <cassert>
#include <cstdio>

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/SoPath.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodekits/SoNodeKit.h>
#include <Inventor/SoInteraction.h>
#include <Inventor/actions/SoWriteAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/annex/ForeignFiles/SoForeignFileKit.h>

enum FileType { INVENTOR, VRML1, VRML2 };

int
main(int argc, char ** argv)
{
  fprintf(stderr, "ivcp v0.1\n");

  SoDB::init();
  SoNodeKit::init();
  SoInteraction::init();

  if (argc != 3 ) {
    fprintf(stdout, "Usage: %s infile outfile\n", argv[0]);
    return 0;
  }

  SoInput * in = new SoInput;
  if (!in->openFile(argv[1])) {
    fprintf(stderr, "error: could not open file '%s'\n", argv[1]);
    delete in;
    SoDB::cleanup();
    return -1;
  }

  SoNode * scene = SoDB::readAll(in);

  if (!scene) {
    fprintf(stderr, "error: could not read file '%s'\n", argv[1]);
    delete in;
    SoDB::cleanup();
    return -1;
  }
  FileType inputFileType;
  if (in->isFileVRML1())
    inputFileType = VRML1;
  else if (in->isFileVRML2())
    inputFileType = VRML2;
  else
    inputFileType = INVENTOR;

  delete in;
  scene->ref();

  SoNode * firstChild = static_cast<SoSeparator*>(scene)->getNumChildren()?
    static_cast<SoSeparator*>(scene)->getChild(0)
    :NULL;

  if (firstChild && firstChild->isOfType(SoForeignFileKit::getClassTypeId())) {
    SoForeignFileKit * kit = (SoForeignFileKit *) firstChild;
    if (kit->canWriteScene() ) {
      SoNode * subscene = NULL;
      kit->writeScene(subscene);
      if (!subscene ) {
        return -1;
      }
      subscene->ref();
      scene->unref();
      scene = subscene;
    }
  }

  SoOutput * out = new SoOutput;
  if (!out->openFile(argv[2])) {
    fprintf(stderr, "error: could not open file '%s' for writing\n");
    scene->unref();
    delete out;
    SoDB::cleanup();
    return -1;
  }
  switch (inputFileType) {
  case VRML1:
    out->setHeaderString("#VRML V1.0 ascii");
    break;
  case VRML2:
    out->setHeaderString("#VRML V2.0 utf8");
  }

  SoWriteAction wa(out);
  wa.apply(scene);

  out->closeFile();
  delete out;

  scene->unref();

  // with actions on the stack, cleanup can't be called...
  // SoDB::cleanup();
  return 0;
}
