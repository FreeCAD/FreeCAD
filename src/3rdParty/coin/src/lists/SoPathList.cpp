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
  \class SoPathList SoPathList.h Inventor/lists/SoPathList.h
  \brief The SoPathList class is a container for pointers to SoPath objects.

  \ingroup coin_general

  As this class inherits SoBaseList, referencing and dereferencing
  will default be done on the objects at append(), remove(), insert()
  etc.
*/

#include <Inventor/lists/SoPathList.h>
#include <Inventor/SoPath.h>
#include <Inventor/SoFullPath.h>
#include <Inventor/C/tidbits.h>
#include <cassert>


/*!
  Default constructor.
*/
SoPathList::SoPathList(void)
  : SoBaseList()
{
}

/*!
  Constructor with a hint about the number of elements the list will
  hold.

  \sa SoBaseList::SoBaseList(const int)
*/
SoPathList::SoPathList(const int size)
  : SoBaseList(size)
{
}

/*!
  Copy constructor.

  Does a shallow copy of the SoPath pointer values, but updates
  reference count.

  \sa SoBaseList::SoBaseList(const SoBaseList &)
*/
SoPathList::SoPathList(const SoPathList & pl)
  : SoBaseList(pl)
{
}

/*!
  Destructor.

  \sa SoBaseList::~SoBaseList()
*/
SoPathList::~SoPathList()
{
}

/*!
  Append \a ptr to the list.

  \sa SoBaseList::append()
*/
void
SoPathList::append(SoPath * const path)
{
  SoBaseList::append((SoBase *)path);
}

/*!
  Return node pointer at index \a i.

  \sa SoBaseList::operator[]()
*/
SoPath *
SoPathList::operator[](const int i) const
{
  return (SoPath *)SoBaseList::operator[](i);
}

/*!
  Shallow copy of contents of list \a pl to this list.

  \sa SoBaseList::operator=()
*/
SoPathList &
SoPathList::operator=(const SoPathList & pl)
{
  SoBaseList::copy(pl);
  return *this;
}

/*!
  Find and return index of first item equal to \a path.
*/
int
SoPathList::findPath(const SoPath & path) const
{
  int i, n = this->getLength();
  for (i = 0; i < n; i++) if (*(*this)[i] == path) return i;
  return -1;

}

// Return a negative number if the path pointed to by v0 is considered
// "less than" v1, a positive number if v0 is considered "larger than"
// v1 and zero if they are equal.
//
// "extern C" wrapper is needed with the OSF1/cxx compiler (probably a
// bug in the compiler, but it doesn't seem to hurt to do this
// anyway).
extern "C" {
static int
compare_paths(const void * v0, const void * v1)
{
  SoFullPath * p0 = *((SoFullPath**)v0);
  SoFullPath * p1 = *((SoFullPath**)v1);

  const ptrdiff_t diff = (char *)p0->getHead() - (char *)p1->getHead();
  if (diff != 0) { return (int)diff; }

  int n = SbMin(p0->getLength(), p1->getLength());
  int i;
  for (i = 1; i < n; i++) {
    const int diff = p0->getIndex(i) - p1->getIndex(i);
    if (diff != 0) { return (int)diff; }
  }
  // shortest path first
  return p0->getLength() - p1->getLength();
}
}

/*!
  Sort paths in list according to how early they are run into when
  traversing the scene graph.
*/
void
SoPathList::sort(void)
{
  qsort((void **)this->getArrayPtr(), this->getLength(), sizeof(void *), compare_paths);
}

/*!
  Removes identical paths and paths that go through the tail of
  another path.

  Note that this method assumes the list to be in a sorted order.

  \sa sort()
*/
void
SoPathList::uniquify(void)
{
  SoFullPath ** array = (SoFullPath**) this->getArrayPtr();
  
  for (int i = this->getLength()-2; i >= 0; i--) {
    SoFullPath * p = array[i];
    int j = i+1;
    // if fork is at tail of current path, remove next path. We might
    // have more than one path that go through this path's tail, so do
    // the test in a while loop
    while ((j < this->getLength()) && (p->findFork(array[j]) == p->getLength()-1)) {
      this->remove(j);
      // get array pointer again even though it shouldn't change, but
      // it might do in the future so...
      array = (SoFullPath**) this->getArrayPtr();
    }
  }
}
