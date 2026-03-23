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
  \class SoEngineOutput SoEngine.h Inventor/engines/SoEngine.h
  \brief The SoEngineOutput class is the output slots in SoEngine instances.

  \ingroup coin_engines

  SoEngineOutput has methods for convenient handling of the
  connections made from SoEngine objects to SoField objects.

  \sa SoEngine, SoField
 */

#include <Inventor/engines/SoEngineOutput.h>

#include "SbBasicP.h"

#include <Inventor/engines/SoEngine.h>
#include <Inventor/engines/SoNodeEngine.h>
#include <Inventor/engines/SoOutputData.h>
#include <Inventor/engines/SoFieldConverter.h>
#include <Inventor/fields/SoField.h>
#include <Inventor/misc/SoProtoInstance.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  Constructor. The SoEngineOutput will initially not be contained
  within an SoEngine nor will it have any slave fields attached.

  \sa setContainer()
*/
SoEngineOutput::SoEngineOutput(void)
{
  this->enabled = TRUE;
  this->container = NULL;
}

/*!
  Destructor.
*/
SoEngineOutput::~SoEngineOutput()
{
#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoEngineOutput::~SoEngineOutput", "start %p", this);
#endif // debug

  // Avoids evaluation from the fields in SoField::disconnect() (which
  // would again lead to problems with the pure virtual
  // SoEngine::evaluate() function during destruction of our
  // container).
  this->enabled = FALSE;

  // Disconnect all fields.
  SoFieldList fl;
  int nr = this->getForwardConnections(fl);
  for (int i=0; i < nr; i++) fl[i]->disconnect(this);

#if COIN_DEBUG && 0 // debug
  SoDebugError::postInfo("SoEngineOutput::~SoEngineOutput", "done %p", this);
#endif // debug
}

/*!
  Returns the type of the engine output.
*/
SoType
SoEngineOutput::getConnectionType(void) const
{
  assert(this->container != NULL);
  const SoEngineOutputData * outputs =
    this->isNodeEngineOutput() ?
    this->getNodeContainer()->getOutputData() :
    this->getContainer()->getOutputData();
  assert(outputs);
  int idx = outputs->getIndex(this->container, this);
  assert(idx >= 0 && "an engine should have at least one output");
  return outputs->getType(idx);
}

/*!
  Adds all slave fields connected to this output to \a fl.  Returns
  the number of slaves.

  \sa addConnection()
  \sa removeConnection()
*/
int
SoEngineOutput::getForwardConnections(SoFieldList & fl) const
{
  int numc = 0;
  for (int i = 0; i < this->slaves.getLength(); i++) {
    SoField * field = this->slaves[i];
    SoFieldContainer * fc = field->getContainer();
    // test for and skip field converters. Get the real field this
    // output is connected to.
    if (fc && fc->isOfType(SoFieldConverter::getClassTypeId())) {
      SoFieldConverter * converter = coin_assert_cast<SoFieldConverter *>(fc);
      numc += converter->getForwardConnections(fl);
    }
    else {
      fl.append(field);
      numc++;
    }
  }
  return numc;
}

/*!
  Sets the enabled flag. If output is disabled, the fields connected
  to this output will not be changed when the engine is evaluated.

  \sa isEnabled().
*/
void
SoEngineOutput::enable(const SbBool flag)
{
  this->enabled = flag;
  if (flag) this->touchSlaves(NULL, FALSE);
}

/*!
  Returns status of the enabled flag.

  \sa enable().
*/
SbBool
SoEngineOutput::isEnabled(void) const
{
  return this->enabled;
}

/*!
  Returns the engine containing this output. If the engine
  containing this output is a NodeEngine, this method returns NULL.

  \sa setContainer(), getNodeContainer()
*/
SoEngine *
SoEngineOutput::getContainer(void) const
{
  if (this->isNodeEngineOutput()) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoEngineOutput::getContainer",
                              "Container is not an Engine");
#endif // COIN_DEBUG
    return NULL;
  }
  return this->container;
}

/*!

  Returns the node engine containing this output. If the engine
  containing this output is not a NodeEgine, this method returns NULL.

  \COIN_FUNCTION_EXTENSION

  \sa setNodeContainer(), getContainer()
  \since Coin 2.0
*/

SoNodeEngine *
SoEngineOutput::getNodeContainer(void) const
{
  if (!this->isNodeEngineOutput()) {
#if COIN_DEBUG
    SoDebugError::postWarning("SoEngineOutput::getContainer",
                              "Container is not a NodeEngine");
#endif // COIN_DEBUG
    return NULL;
  }
  return coin_assert_cast<SoNodeEngine*>(this->container);
}

/*!
  Returns \e TRUE if the container is a NodeEngine.

  \COIN_FUNCTION_EXTENSION

  \sa getNodeContainer(), getContainer()
  \since Coin 2.0
*/
SbBool
SoEngineOutput::isNodeEngineOutput(void) const
{
  assert(this->container);
  return this->container->getTypeId().isDerivedFrom(SoNodeEngine::getClassTypeId());
}

/*!
  Sets the engine containing this output.

  \sa getContainer()
*/
void
SoEngineOutput::setContainer(SoEngine * engine)
{
  this->container = engine;
}


/*!
  Sets the NodeEngine containing this output.

  \COIN_FUNCTION_EXTENSION

  \sa getNodeContainer()
  \since Coin 2.0
*/

void
SoEngineOutput::setNodeContainer(SoNodeEngine * nodeengine)
{
  // FIXME: hack cast to SoEngine. The type of the container member
  // needs to be SoFieldContainer, not SoEngine.
  this->container = reinterpret_cast<SoEngine*> (nodeengine);
}

/*!
  Adds \a f to the list of connections from this output.

  Adds 1 to the reference count of the container engine.

  \sa removeConnection(), getForwardConnections()
*/
void
SoEngineOutput::addConnection(SoField * f)
{
#if COIN_DEBUG
  if (this->slaves.find(f) != -1) {
    SoDebugError::postWarning("SoEngineOutput::addConnection",
                              "connection from %p already made", f);
    return;
  }
#endif // COIN_DEBUG

  this->slaves.append(f);

  // An engine's reference count increases with the number of
  // connections it has.

  // SoProtoInstance has some special memory handling. Don't ref
  // and unref if the connection is to an SoProtoInstance
  SoFieldContainer * fc = f->getContainer();

  if (fc && !fc->isOfType(SoProtoInstance::getClassTypeId()) &&
	  // don't reference if the slave and master are in the same container
	  // else the corresponding unref can never happen RHW
	  fc != this->getFieldContainer ()) {
    this->getFieldContainer()->ref();
  }

  // Trigger re-evaluation of engine.
  if (!this->isNodeEngineOutput()) {
    this->getContainer()->setDirty();
  }
}

/*!
  Removes \a f from the list of connections from this output.

  Subtracts 1 from the reference count of the container engine. If the
  reference count reaches zero (which will happen if this was the last
  connection and the application programmer did not explicitly \e ref
  the engine), the container engine will be deallocated.

  \sa addConnection(), getForwardConnections()
*/
void
SoEngineOutput::removeConnection(SoField * f)
{
  int i = this->slaves.find(f);
#if COIN_DEBUG
  if (i == -1) {
    SoDebugError::postWarning("SoEngineOutput::removeConnection",
                              "no connection from %p present", f);
    return;
  }
#endif // COIN_DEBUG
  this->slaves.remove(i);

  // SoProtoInstance has some special memory handling. Don't ref
  // and unref if the connection is to an SoProtoInstance
  SoFieldContainer * fc = f->getContainer();

  if (fc && !fc->isOfType(SoProtoInstance::getClassTypeId()) &&
	  // we don't want to unref when connected to self RHW
	  fc != this->getFieldContainer ()) {
    this->getFieldContainer()->unref();
  }
}

/*!
  Returns the number of fields in the list of connections.

  \sa operator[], addConnection()
*/
int
SoEngineOutput::getNumConnections(void) const
{
  return this->slaves.getLength();
}

/*!
  Returns the field at index \a i in the list of connections.

  Please note that this might not be the same field as the one the
  output was originally connected to, since a field converted might
  have been inserted to converted the output to match the field type.

  \sa getNumConnections()
*/
SoField *
SoEngineOutput::operator[](int i) const
{
  return this->slaves[i];
}

/*!
  Disables notifications on fields connected to this output.  This is
  done before the engine is evaluated, since the fields we are going
  to write into have already been notified.

  \sa doneWriting()
*/
void
SoEngineOutput::prepareToWrite(void) const
{
  SoEngineOutput * that = const_cast<SoEngineOutput *>(this);
  that->fieldnotiflist.truncate(0);

  int n = this->slaves.getLength();
  for (int i = 0; i < n; i++) {
    SoField * f = this->slaves[i];
    that->fieldnotiflist.append(f->isNotifyEnabled());
    f->enableNotify(FALSE);
  }
}

/*!
  Restores the notification flags on fields connected to this output
  after evaluating.

  \sa prepareToWrite()
*/
void
SoEngineOutput::doneWriting(void) const
{
  // We should have the exact same set of slave fields now as on entry
  // to prepareToWrite(), as the field writing is supposed be a
  // "closed" operation.  (All notifications on the fields are
  // disabled, so no application code should be able to intervene in
  // SoEngine evaluation.)

  int n = this->slaves.getLength();
  assert(n == this->fieldnotiflist.getLength());

  const SbBool * notifs = this->fieldnotiflist.getArrayPtr();

  for (int i = 0; i < n; i++) {
    this->slaves[i]->enableNotify(notifs[i]);
  }
}

/*!
  Notify the fields attached to this engine output that the output
  value has changed.

  If \a donotify is \c TRUE, propagate the notification to the
  fields. Otherwise just mark the fields "dirty" for re-evaluation.

  Note that this method is not part of the original Open Inventor API.
 */
void
SoEngineOutput::touchSlaves(SoNotList * nl, SbBool donotify)
{
  if (this->isEnabled()) {
    // use a copy of the notification list so that the connections are
    // not added to the original list
    SoNotList listcopy;
    if (nl && donotify) listcopy = *nl;
    int numconnections = this->getNumConnections();
    for (int j = 0; j < numconnections; j++) {
      SoField * field = (*this)[j];
      if (donotify) {
        field->notify(&listcopy);
        if (j < numconnections-1) {
          listcopy = *nl;
        }
      }
      else field->setDirty(TRUE);
    }
  }
}

/*!
  Convenience method that returns a field container. This method can
  be used both for NodeEngine and Engine outputs.

  \COIN_FUNCTION_EXTENSION

  \since Coin 2.0
*/
SoFieldContainer *
SoEngineOutput::getFieldContainer(void)
{
  return coin_assert_cast<SoFieldContainer *>(this->container);
}
