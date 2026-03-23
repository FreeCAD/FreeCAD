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

#ifdef HAVE_MANIPULATORS

/*!
  \class SoTrackballManip SoTrackballManip.h Inventor/manips/SoTrackballManip.h
  \brief The SoTrackballManip wraps an SoTrackballDragger for convenience.

  \ingroup coin_manips

  The manipulator class takes care of wrapping up the
  SoTrackballDragger in a simple and convenient API for the
  application programmer, making it automatically surround the
  geometry it influences and taking care of the book-keeping routines
  for its interaction with the relevant fields of an SoTransformation
  node.

  <center>
  \image html trackball.png "Example of Trackball Manipulator"
  </center>

  Here's a dead simple, standalone example on how to set up an
  SoTrackballManip in a scene graph:

  \code
  #include <Inventor/Qt/SoQt.h>
  #include <Inventor/Qt/viewers/SoQtExaminerViewer.h>
  #include <Inventor/nodes/SoCone.h>
  #include <Inventor/nodes/SoSeparator.h>
  #include <Inventor/manips/SoTrackballManip.h>
      
  int
  main(int argc, char* argv[])
  {
    QWidget * mainwin = SoQt::init(argv[0]);
  
    SoSeparator * root = new SoSeparator;
    root->ref();
  
    SoTrackballManip * manip = new SoTrackballManip;
    root->addChild(manip);
  
    root->addChild(new SoCone);
      
    SoQtExaminerViewer * viewer = new SoQtExaminerViewer(mainwin);
    viewer->setSceneGraph(root);
      
    SoQt::show(mainwin);
    SoQt::mainLoop();
  
    root->unref();
      
    return 0;
  }
  \endcode

  Compile and hit 'ESC' to go into scene graph interaction mode in the
  examiner viewer, then change the rotation of the cone by interacting
  with the dragger bands.

  <center>
  \image html trackball-cone.png "Rendering from Example File Above"
  </center>
*/

#include <Inventor/manips/SoTrackballManip.h>

#include <Inventor/nodes/SoSurroundScale.h>
#include <Inventor/draggers/SoTrackballDragger.h>

#include "nodes/SoSubNodeP.h"

class SoTrackballManipP {
public:
};

SO_NODE_SOURCE(SoTrackballManip);


/*!
  \copybrief SoNode::initClass(void)
*/
void
SoTrackballManip::initClass(void)
{
  SO_NODE_INTERNAL_INIT_CLASS(SoTrackballManip, SO_FROM_INVENTOR_1);
}

/*!
  Default constructor. Allocates an SoTrackballDragger and an
  SoSurroundScale node to surround the geometry within our part of the
  scene graph.
*/
SoTrackballManip::SoTrackballManip(void)
{
  SO_NODE_INTERNAL_CONSTRUCTOR(SoTrackballManip);

  SoTrackballDragger *dragger = new SoTrackballDragger;
  this->setDragger(dragger);

  SoSurroundScale *ss = (SoSurroundScale*) dragger->getPart("surroundScale", TRUE);
  ss->numNodesUpToContainer = 4;
  ss->numNodesUpToReset = 3;
}

/*!
  Destructor.
*/
SoTrackballManip::~SoTrackballManip()
{
}

#endif // HAVE_MANIPULATORS
