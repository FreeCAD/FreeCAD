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
  \class SoWriteAction SoWriteAction.h Inventor/actions/SoWriteAction.h
  \brief The SoWriteAction class writes a scene graph to file.

  \ingroup coin_actions

  When applied to a scene, this action writes its contents to the
  stream contained within an SoOutput instance. This can be a file, a
  memory buffer or a system file handle like \c stdout, for instance.

  \e All information considered part of the scene graph should be
  written out, including not only nodes, but also the nodes' field
  values, global fields (at least those with connections inside the
  scene the action is applied to), engines in the scene, paths, etc.

  The scene is written in the Open Inventor file format. Files in this
  format can be parsed into their scene graph structures by using the
  SoDB::readAll() method (SoDB also contains a few other import
  methods you can use).

  Here's a complete, standalone usage example which shows how to
  write a scene graph to a memory buffer:

  \code
  #include <Inventor/SoDB.h>
  #include <Inventor/actions/SoWriteAction.h>
  #include <Inventor/nodes/SoCone.h>
  #include <Inventor/nodes/SoSeparator.h>

  static char * buffer;
  static size_t buffer_size = 0;

  static void *
  buffer_realloc(void * bufptr, size_t size)
  {
    buffer = (char *)realloc(bufptr, size);
    buffer_size = size;
    return buffer;
  }

  static SbString
  buffer_writeaction(SoNode * root)
  {
    SoOutput out;
    buffer = (char *)malloc(1024);
    buffer_size = 1024;
    out.setBuffer(buffer, buffer_size, buffer_realloc);

    SoWriteAction wa(&out);
    wa.apply(root);

    SbString s(buffer);
    free(buffer);
    return s;
  }

  int
  main(int argc, char ** argv)
  {
    SoDB::init();

    SoSeparator * root = new SoSeparator;
    root->ref();

    root->addChild(new SoCone);

    SbString s = buffer_writeaction(root);
    (void)fprintf(stdout, "%s\n", s.getString());

    root->unref();
    return 0;
  }
  \endcode

  \sa SoOutput
*/

#include <Inventor/actions/SoWriteAction.h>

#include <Inventor/SoOutput.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/errors/SoDebugError.h>

#include "coindefs.h"
#include "actions/SoSubActionP.h"
#include "io/SoWriterefCounter.h"

class SoWriteActionP {
public:
};

SO_ACTION_SOURCE(SoWriteAction);


/*!
  \copydetails SoAction::initClass(void)
*/
void
SoWriteAction::initClass(void)
{
  SO_ACTION_INTERNAL_INIT_CLASS(SoWriteAction, SoAction);
}

/*!
  Default constructor. Output will be written to \c stdout in ASCII
  format.
*/
SoWriteAction::SoWriteAction(void)
{
  this->commonConstructor(new SoOutput);
  this->localoutputalloc = TRUE;
}

/*!
  Constructor. Output will be written via the \a out object.
*/
SoWriteAction::SoWriteAction(SoOutput * out)
{
  this->commonConstructor(out);
  this->localoutputalloc = FALSE;
}

void
SoWriteAction::commonConstructor(SoOutput * out)
{
  SO_ACTION_CONSTRUCTOR(SoWriteAction);

  this->outobj = out;
  this->continuing = FALSE;
}

/*!
  Destructor.
*/
SoWriteAction::~SoWriteAction(void)
{
  if (this->localoutputalloc) delete this->outobj;
}

/*!
  Returns a pointer to the SoOutput object we're using when writing
  the scene graph.
 */
SoOutput *
SoWriteAction::getOutput(void) const
{
  return this->outobj;
}

/*!
  Applies the write method to the subgraph starting at \a node with
  the current SoOutput instance, without resetting any of the internal
  state of the action instance.

  This should normally be for internal use only.
*/
void
SoWriteAction::continueToApply(SoNode * node)
{
  SbBool wascontinuing = this->continuing;
  this->continuing = TRUE;
  this->apply(node);
  this->continuing = wascontinuing;
}

/*!
  Applies the write method to \a path with the current SoOutput
  instance, without resetting any of the internal state of the action
  instance.

  This should normally be for internal use only.
*/
void
SoWriteAction::continueToApply(SoPath * path)
{
  SbBool wascontinuing = this->continuing;
  this->continuing = TRUE;
  this->apply(path);
  this->continuing = wascontinuing;
}

#if COIN_DEBUG
static void sensorCB(void * COIN_UNUSED_ARG(data), SoSensor * COIN_UNUSED_ARG(sensor))
{
  SoDebugError::postWarning("SoWriteAction::SoWriteAction",
                            "Scenegraph changed during SoWriteAction().");
}
#endif

// Documented for Doxygen in superclass.
//
// Overridden from parent class, as the write action is actually done
// in two passes.
//
// The first pass is done to count the references of the objects in
// the scene graph and otherwise prepare instance in the scene for
// export.  The second pass does the actual writing.
void
SoWriteAction::beginTraversal(SoNode * node)
{
#if COIN_DEBUG
  SoNodeSensor *sensor = NULL;
#endif
  if (this->continuing == FALSE) { // Run through both stages.
    // call SoWriterefCounter::instance() before traversing to set the
    // "current" pointer in SoWriterefCounter. This is needed to be
    // backwards compatible with old code that uses the writeref
    // system in SoBase.

#if COIN_DEBUG
    if (SoWriterefCounter::debugWriterefs()) {
      sensor = new SoNodeSensor(sensorCB, NULL);
      sensor->setPriority(0);
      sensor->attach(node);
    }
#endif

    (void) SoWriterefCounter::instance(this->getOutput());
    this->outobj->setStage(SoOutput::COUNT_REFS);
    this->traverse(node);
    this->outobj->setStage(SoOutput::WRITE);
  }
  this->traverse(node);
  if (!this->outobj->isBinary() && !this->continuing) {
    outobj->write('\n');
    outobj->resolveRoutes();
  }
  if (!this->continuing) {
    SoWriterefCounter::instance(this->getOutput())->debugCleanup();
#if COIN_DEBUG
    delete sensor;
#endif
  }
}

/*!
  \COININTERNAL

  Compact path lists are not implemented in Coin (yet), but if they
  are, SoWriteAction should return \c FALSE here -- it would only be
  extra overhead for the SoWriteAction to have path lists compacted
  before traversal.

  Seems like a silly optimization to me, though.. :^/  20000306 mortene.
*/
SbBool
SoWriteAction::shouldCompactPathLists(void) const
{
  return FALSE;
}

#ifdef COIN_TEST_SUITE

// check that the realTime GlobalField is written if it has any
// forward connections.

#include <Inventor/SoDB.h>
#include <Inventor/SoInput.h>
#include <Inventor/SoOutput.h>
#include <Inventor/fields/SoSFTime.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/C/tidbits.h>

BOOST_AUTO_TEST_CASE(GlobalField)
{
  SoDB::init();

  //Store away the state before we mess with the global realTime field
  SoSFTime * realtime = static_cast<SoSFTime *>(SoDB::getGlobalField("realTime"));
  assert(realtime);
  SbTime realTimeStorage = realtime->getValue();

  static const char inlinescenegraph[] =
    "#Inventor V2.1 ascii\n"
    "\n"
    "\n"
    "Separator {\n"
    "\n"
    "  Text2 { \n"
    "    string \"\" = GlobalField { \n"
    "      type \"SFTime\" realTime 0 \n"
    "\n"
    "    } . realTime \n"
    "  }\n"
    "}\n";

  // read scene
  SoInput in;
  in.setBuffer(inlinescenegraph, strlen(inlinescenegraph));
  SoSeparator * top = SoDB::readAll(&in);
  BOOST_REQUIRE(top);
  top->ref();

  // write scene
  SoOutput out;
  const int buffer_size = 1024;
  char * buffer = (char *)malloc(buffer_size);
  out.setBuffer(buffer, buffer_size, NULL);

  SoWriteAction wa(&out);
  wa.apply((SoNode *)top);

  top->unref();
  top = NULL;

  // read scene again to check if realTime field was written
  in.setBuffer(buffer, strlen(buffer));
  top = SoDB::readAll(&in);
  BOOST_REQUIRE(top);
  top->ref();

  SoText2 * text = (SoText2 *)top->getChild(0);
  BOOST_REQUIRE(text);

  SoField * string = text->getField("string");
  BOOST_REQUIRE(string);
  BOOST_CHECK_MESSAGE(string->isConnected(), "String field not connected to realTime field in written scene graph");

  free(buffer);

  top->unref();

  //Restore state
  realtime->setValue(realTimeStorage);

}

#endif // COIN_TEST_SUITE
