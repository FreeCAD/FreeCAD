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
  \class SoCallbackList SoCallbackList.h Inventor/lists/SoCallbackList.h
  \brief The SoCallbackList is a container for callback function pointers.

  \ingroup coin_general

  This list stores callback function pointers (along with
  user-specified extra data to pass to the callbacks) and provides a
  method for triggering the callback functions.
*/

#include <Inventor/lists/SoCallbackList.h>

#if COIN_DEBUG
#include <Inventor/errors/SoDebugError.h>
#endif // COIN_DEBUG

/*!
  \typedef void SoCallbackListCB(void * userdata, void * callbackdata)

  The type definition for callback functions. The \a userdata is supplied
  alongside the function pointer when it is added to the list. When the
  callback is invoked the specific \a userdata for that function is supplied,
  as well as the \a callbackdata that is sent to all functions invoked.
*/

/*!
  Default constructor.
*/
SoCallbackList::SoCallbackList(void)
{
}

/*!
  Destructor.
*/
SoCallbackList::~SoCallbackList(void)
{
}

/*!
  Append the callback function \a f to the list. It will be passed the
  \a userdata upon invocation.
*/
void
SoCallbackList::addCallback(SoCallbackListCB * f, void * userdata)
{
  // FIXME: Shouldn't we check if the callback is already in the list?
  // 20050723 kyrah.
  this->funclist.append((void*)f);
  this->datalist.append(userdata);
}

/*!
  Remove callback \a f from the list.
*/
void
SoCallbackList::removeCallback(SoCallbackListCB * f, void * userdata)
{
  int idx = this->getNumCallbacks() - 1;

  // FIXME: Why are we not using a dictionary here? (The question is
  // of course whether it should be allowed to have the same callback
  // entry in the list twice...) 20050723 kyrah.
  while (idx != -1) {
    if ((this->funclist[idx] == (void*)f) && (this->datalist[idx] == userdata)) {
      this->funclist.remove(idx);
      this->datalist.remove(idx);
      break;
    }
    idx--;
  }

#if COIN_DEBUG
  // FIXME: Is this warning really necessary? Shouldn't it be possible
  // to do the equivalent of setCallback(NULL,NULL) -- i.e. "remove if
  // already exists, else do nothing"? 20050723 kyrah.
  if (idx == -1) {
    SoDebugError::post("SoCallbackList::removeCallback",
                       "Tried to remove non-existent callback function.");
    return;
  }
#endif // COIN_DEBUG
}

/*!
  Remove all callbacks in the list.
*/
void
SoCallbackList::clearCallbacks(void)
{
  this->funclist.truncate(0);
  this->datalist.truncate(0);
}

/*!
  Returns number of callback functions.
*/
int
SoCallbackList::getNumCallbacks(void) const
{
  return this->funclist.getLength();
}

/*!
  Invoke all callback functions, passing the userdata and the \a
  callbackdata as the first and second argument, respectively.

  All callbacks registered when the method is invoked will be
  triggered, even though if the code in one callback removes another
  callback.

  It is safe for a callback to remove itself or any other callbacks
  during execution.
*/
void
SoCallbackList::invokeCallbacks(void * callbackdata)
{
  SbPList flcopy(this->funclist);
  SbPList dlcopy(this->datalist);

  for (int idx=0; idx < flcopy.getLength(); idx++) {
    SoCallbackListCB * func = (SoCallbackListCB*) flcopy[idx];
    func(dlcopy.operator[](idx), callbackdata);
  }
}
